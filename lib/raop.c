/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *===================================================================
 * modified by fduncanh 2021-23
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "raop.h"
#include "raop_rtp.h"
#include "pairing.h"
#include "httpd.h"

#include "global.h"
#include "fairplay.h"
#include "netutils.h"
#include "logger.h"
#include "compat.h"
#include "raop_rtp_mirror.h"
#include "raop_ntp.h"

struct raop_s {
    /* Callbacks for audio and video */
    raop_callbacks_t callbacks;

    /* Logger instance */
    logger_t *logger;

    /* Pairing, HTTP daemon and RSA key */
    pairing_t *pairing;
    httpd_t *httpd;

    dnssd_t *dnssd;

    /* local network ports */  
    unsigned short port;
    unsigned short timing_lport;
    unsigned short control_lport;
    unsigned short data_lport;
    unsigned short mirror_data_lport;  

    /* configurable plist items: width, height, refreshRate, maxFPS, overscanned *
     * also clientFPSdata, which controls whether video stream info received     *
     * from the client is shown on terminal monitor.                             */
    uint16_t width;
    uint16_t height;
    uint8_t refreshRate;
    uint8_t maxFPS;
    uint8_t overscanned;
    uint8_t clientFPSdata;

    int audio_delay_micros;
    int max_ntp_timeouts;

     /* for temporary storage of pin during pair-pin start */
     unsigned short pin;
     bool use_pin;
  
     /* public key as string */
     char pk_str[2*ED25519_KEY_SIZE + 1];
};

struct raop_conn_s {
    raop_t *raop;
    raop_ntp_t *raop_ntp;
    raop_rtp_t *raop_rtp;
    raop_rtp_mirror_t *raop_rtp_mirror;
    fairplay_t *fairplay;
    pairing_session_t *session;

    unsigned char *local;
    int locallen;

    unsigned char *remote;
    int remotelen;

    unsigned int zone_id;

    connection_type_t connection_type; 

    bool have_active_remote;
};
typedef struct raop_conn_s raop_conn_t;

#include "raop_handlers.h"

static void *
conn_init(void *opaque, unsigned char *local, int locallen, unsigned char *remote, int remotelen, unsigned int zone_id) {
    raop_t *raop = opaque;
    raop_conn_t *conn;
    char ip_address[40];
    assert(raop);

    conn = calloc(1, sizeof(raop_conn_t));
    if (!conn) {
        return NULL;
    }
    conn->raop = raop;
    conn->raop_rtp = NULL;
    conn->raop_rtp_mirror = NULL;
    conn->raop_ntp = NULL;
    conn->fairplay = fairplay_init(raop->logger);

    if (!conn->fairplay) {
        free(conn);
        return NULL;
    }
    conn->session = pairing_session_init(raop->pairing);
    if (!conn->session) {
        fairplay_destroy(conn->fairplay);
        free(conn);
        return NULL;
    }


    utils_ipaddress_to_string(locallen, local, zone_id, ip_address, (int) sizeof(ip_address));
    logger_log(conn->raop->logger, LOGGER_INFO, "Local : %s", ip_address);

    utils_ipaddress_to_string(remotelen, remote, zone_id, ip_address, (int) sizeof(ip_address));
    logger_log(conn->raop->logger, LOGGER_INFO, "Remote: %s", ip_address);

    conn->local = malloc(locallen);
    assert(conn->local);
    memcpy(conn->local, local, locallen);

    conn->remote = malloc(remotelen);
    assert(conn->remote);
    memcpy(conn->remote, remote, remotelen);

    conn->zone_id = zone_id;

    conn->locallen = locallen;
    conn->remotelen = remotelen;

    conn->connection_type = CONNECTION_TYPE_UNKNOWN;

    conn->have_active_remote = false;
    
    if (raop->callbacks.conn_init) {
        raop->callbacks.conn_init(raop->callbacks.cls);
    }

    return conn;
}

static void
conn_request(void *ptr, http_request_t *request, http_response_t **response) {
    char *response_data = NULL;
    int response_datalen = 0;
    raop_conn_t *conn = ptr;

    bool logger_debug = (logger_get_level(conn->raop->logger) >= LOGGER_DEBUG);

    const char *method = http_request_get_method(request);
    const char *url = http_request_get_url(request);
    const char *protocol = http_request_get_protocol(request);
    const char *cseq = http_request_get_header(request, "CSeq");

    printf("Test");
    if (conn->connection_type == CONNECTION_TYPE_UNKNOWN) {
        printf("Test2");
        if (httpd_count_connection_type(conn->raop->httpd, CONNECTION_TYPE_RAOP)) {
            printf("Test3");
            char ipaddr[40];
            utils_ipaddress_to_string(conn->remotelen, conn->remote, conn->zone_id, ipaddr, (int) (sizeof(ipaddr)));
            if (httpd_nohold(conn->raop->httpd)) {
                logger_log(conn->raop->logger, LOGGER_INFO, "\"nohold\" feature: switch to new connection request from %s", ipaddr);		  
                if (conn->raop->callbacks.video_reset) {
                    printf("**************************video_reset*************************\n");
                    conn->raop->callbacks.video_reset(conn->raop->callbacks.cls);
                }
		        httpd_remove_known_connections(conn->raop->httpd);
            } else {
                logger_log(conn->raop->logger, LOGGER_WARNING, "rejecting new connection request from %s", ipaddr);
                *response = http_response_create();
                http_response_init(*response, protocol, 409, "Conflict: Server is connected to another client");
                goto finish;
	        }
        }      
        httpd_set_connection_type(conn->raop->httpd, ptr, CONNECTION_TYPE_RAOP);
        conn->connection_type = CONNECTION_TYPE_RAOP;
    }

    if (!conn->have_active_remote) {
        const char *active_remote = http_request_get_header(request, "Active-Remote");
        if (active_remote) {
            conn->have_active_remote = true;
            if (conn->raop->callbacks.export_dacp) {
                const char *dacp_id = http_request_get_header(request, "DACP-ID");
                conn->raop->callbacks.export_dacp(conn->raop->callbacks.cls, active_remote, dacp_id);
            }
        }
    }

    if (!method) {
        return;
    }

    /* this rejects unsupported messages from _airplay._tcp for video streaming protocol*/
    // if (!cseq) {
    //     return;
    // }
    // printf("Test4");
    logger_log(conn->raop->logger, LOGGER_DEBUG, "\n%s %s %s", method, url, protocol);
    char *header_str= NULL; 
    http_request_get_header_string(request, &header_str);
    if (header_str) {
        logger_log(conn->raop->logger, LOGGER_DEBUG, "%s", header_str);
        bool data_is_plist = (strstr(header_str,"apple-binary-plist") != NULL);
        bool data_is_text = (strstr(header_str,"text/") != NULL);
        free(header_str);
        int request_datalen;
        const char *request_data = http_request_get_data(request, &request_datalen);
        if (request_data && logger_debug) {
            if (request_datalen > 0) {
	        if (data_is_plist) {
		    plist_t req_root_node = NULL;
		    plist_from_bin(request_data, request_datalen, &req_root_node);
                    char * plist_xml;
                    uint32_t plist_len;
                    plist_to_xml(req_root_node, &plist_xml, &plist_len);
                    logger_log(conn->raop->logger, LOGGER_DEBUG, "%s", plist_xml);
                    free(plist_xml);
                    plist_free(req_root_node);
                } else if (data_is_text) {
                    char *data_str = utils_data_to_text((char *) request_data, request_datalen);
                    logger_log(conn->raop->logger, LOGGER_DEBUG, "%s", data_str);                    
                    free(data_str);
                } else {
                    char *data_str =  utils_data_to_string((unsigned char *) request_data, request_datalen, 16);
                    logger_log(conn->raop->logger, LOGGER_DEBUG, "%s", data_str);
                    free(data_str);
                }
            }
        }
    }

    *response = http_response_create();
    http_response_init(*response, protocol, 200, "OK");

    //http_response_add_header(*response, "Apple-Jack-Status", "connected; type=analog");


    logger_log(conn->raop->logger, LOGGER_DEBUG, "Handling request %s with URL %s", method, url);
    raop_handler_t handler = NULL;
    if (!strcmp(method, "GET") && strstr(url, "/info") != NULL) {
        handler = &raop_handler_info;
        logger_log(conn->raop->logger, LOGGER_DEBUG, "Correct path taken!!!!!");
    } else if (!strcmp(method, "POST") && !strcmp(url, "/pair-pin-start")) {
        handler = &raop_handler_pairpinstart;
    } else if (!strcmp(method, "POST") && !strcmp(url, "/pair-setup-pin")) {
        handler = &raop_handler_pairsetup_pin;
    } else if (!strcmp(method, "POST") && !strcmp(url, "/pair-setup")) {
        handler = &raop_handler_pairsetup;
    } else if (!strcmp(method, "POST") && !strcmp(url, "/pair-verify")) {
        handler = &raop_handler_pairverify;
    } else if (!strcmp(method, "POST") && !strcmp(url, "/fp-setup")) {
        handler = &raop_handler_fpsetup;
    } else if (!strcmp(method, "OPTIONS")) {
        handler = &raop_handler_options;
    } else if (!strcmp(method, "SETUP")) {
        handler = &raop_handler_setup;
    } else if (!strcmp(method, "GET_PARAMETER")) {
        handler = &raop_handler_get_parameter;
    } else if (!strcmp(method, "SET_PARAMETER")) {
        handler = &raop_handler_set_parameter;
    } else if (!strcmp(method, "POST") && !strcmp(url, "/feedback")) {
        handler = &raop_handler_feedback;
    } else if (!strcmp(method, "RECORD")) {
        handler = &raop_handler_record;
    } else if (!strcmp(method, "FLUSH")) {
        handler = &raop_handler_flush;
    } else if (!strcmp(method, "TEARDOWN")) {
        handler = &raop_handler_teardown;
    } else {
        logger_log(conn->raop->logger, LOGGER_INFO, "Unhandled Client Request: %s %s", method, url);
    }

    if (handler != NULL) {
        handler(conn, request, *response, &response_data, &response_datalen);
    }
    finish:;
    http_response_add_header(*response, "Server", "AirTunes/"GLOBAL_VERSION);
    // http_response_add_header(*response, "CSeq", cseq);    
    http_response_finish(*response, response_data, response_datalen);

    int len;
    const char *data = http_response_get_data(*response, &len);
    if (response_data && response_datalen > 0) {
        len -= response_datalen;
    } else {
        len -= 2;
    }
    header_str =  utils_data_to_text(data, len);
    logger_log(conn->raop->logger, LOGGER_DEBUG, "\n%s", header_str);
    bool data_is_plist = (strstr(header_str,"apple-binary-plist") != NULL);
    bool data_is_text = (strstr(header_str,"text/parameters") != NULL);
    free(header_str);
    if (response_data) {
        if (response_datalen > 0 && logger_debug) {
            if (data_is_plist) {
                plist_t res_root_node = NULL;
                plist_from_bin(response_data, response_datalen, &res_root_node);
                char * plist_xml;
                uint32_t plist_len;
                plist_to_xml(res_root_node, &plist_xml, &plist_len);
                plist_free(res_root_node);
                logger_log(conn->raop->logger, LOGGER_DEBUG, "%s", plist_xml);
                free(plist_xml);
            } else if (data_is_text) {
                char *data_str = utils_data_to_text((char*) response_data, response_datalen);
                logger_log(conn->raop->logger, LOGGER_DEBUG, "%s", data_str);                    
                free(data_str);
            } else {
                char *data_str = utils_data_to_string((unsigned char *) response_data, response_datalen, 16);
                logger_log(conn->raop->logger, LOGGER_DEBUG, "%s", data_str);
                free(data_str);
            }
        }
        free(response_data);
        response_data = NULL;
        response_datalen = 0;
    }
}

static void
conn_destroy(void *ptr) {
    raop_conn_t *conn = ptr;

    logger_log(conn->raop->logger, LOGGER_DEBUG, "Destroying connection");

    if (conn->raop->callbacks.conn_destroy) {
        conn->raop->callbacks.conn_destroy(conn->raop->callbacks.cls);
    }

    if (conn->raop_rtp) {
        /* This is done in case TEARDOWN was not called */
        raop_rtp_destroy(conn->raop_rtp);
    }
    if (conn->raop_rtp_mirror) {
        /* This is done in case TEARDOWN was not called */
        raop_rtp_mirror_destroy(conn->raop_rtp_mirror);
    }
    if (conn->raop_ntp) {
        raop_ntp_destroy(conn->raop_ntp);
    }

    if (conn->raop->callbacks.video_flush) {
        conn->raop->callbacks.video_flush(conn->raop->callbacks.cls);
    }

    free(conn->local);
    free(conn->remote);
    pairing_session_destroy(conn->session);
    fairplay_destroy(conn->fairplay);
    free(conn);
}

raop_t *
raop_init(raop_callbacks_t *callbacks) {
    raop_t *raop;

    assert(callbacks);

    /* Initialize the network */
    if (netutils_init() < 0) {
        return NULL;
    }

    /* Validate the callbacks structure */
    if (!callbacks->audio_process ||
        !callbacks->video_process) {
        return NULL;
    }

    /* Allocate the raop_t structure */
    raop = calloc(1, sizeof(raop_t));
    if (!raop) {
        return NULL;
    }

    /* Initialize the logger */
    raop->logger = logger_init();

    /* Copy callbacks structure */
    memcpy(&raop->callbacks, callbacks, sizeof(raop_callbacks_t));

    /* initialize network port list */ 
    raop->port = 0;    
    raop->timing_lport = 0;
    raop->control_lport = 0;
    raop->data_lport = 0;
    raop->mirror_data_lport = 0;

    /* initialize configurable plist parameters */
    raop->width = 1920;
    raop->height = 1080;
    raop->refreshRate = 60;
    raop->maxFPS = 30;
    raop->overscanned = 0;

    /* initialise stored pin */
    raop->pin = 0;
    raop->use_pin = false;

    /* initialize switch for display of client's streaming data records */    
    raop->clientFPSdata = 0;

    raop->max_ntp_timeouts = 0;
    raop->audio_delay_micros = 250000;

    return raop;
}

int
raop_init2(raop_t *raop, int nohold, const char *device_id, const char *keyfile) {
    pairing_t *pairing;
    httpd_t *httpd;
    httpd_callbacks_t httpd_cbs;

    /* create a new public key for pairing */
    int new_key;
    pairing = pairing_init_generate(device_id, keyfile, &new_key);
    if (!pairing) {
        logger_log(raop->logger, LOGGER_ERR, "failed to create new public key for pairing");
        return -1;
    }
    /* store PK as a string in raop->pk_str */
    memset(raop->pk_str, 0, sizeof(raop->pk_str));
#ifdef PK
    strncpy(raop->pk_str, PK, 2*ED25519_KEY_SIZE);
#else
    unsigned char public_key[ED25519_KEY_SIZE];
    pairing_get_public_key(pairing, public_key);
    char *pk_str = utils_pk_to_string(public_key, ED25519_KEY_SIZE);
    strncpy(raop->pk_str, (const char *) pk_str, 2*ED25519_KEY_SIZE);
    free(pk_str);
#endif
    if (new_key) {
        logger_log(raop->logger, LOGGER_INFO,"*** A new Public Key has been created and stored in %s", keyfile);
    }

    /* Set HTTP callbacks to our handlers */
    memset(&httpd_cbs, 0, sizeof(httpd_cbs));
    httpd_cbs.opaque = raop;
    httpd_cbs.conn_init = &conn_init;
    httpd_cbs.conn_request = &conn_request;
    httpd_cbs.conn_destroy = &conn_destroy;

    /* Initialize the http daemon */
    httpd = httpd_init(raop->logger, &httpd_cbs, nohold);
    if (!httpd) {
        logger_log(raop->logger, LOGGER_ERR, "failed to initialize http daemon");
        pairing_destroy(pairing);
        return -1;
    }

    raop->pairing = pairing;
    raop->httpd = httpd;
    return 0;
}

void
raop_destroy(raop_t *raop) {
    if (raop) {
        raop_stop(raop);
        pairing_destroy(raop->pairing);
        httpd_destroy(raop->httpd);
        logger_destroy(raop->logger);
        free(raop);

        /* Cleanup the network */
        netutils_cleanup();
    }
}

int
raop_is_running(raop_t *raop) {
    assert(raop);

    return httpd_is_running(raop->httpd);
}

void
raop_set_log_level(raop_t *raop, int level) {
    assert(raop);

    logger_set_level(raop->logger, level);
}

int raop_set_plist(raop_t *raop, const char *plist_item, const int value) {
    int retval = 0;
    assert(raop);
    assert(plist_item);
    
    if (strcmp(plist_item, "width") == 0) {
        raop->width = (uint16_t) value;
        if ((int) raop->width != value) retval = 1;
    } else if (strcmp(plist_item, "height") == 0) {
        raop->height = (uint16_t) value;
        if ((int) raop->height != value) retval = 1;
    } else if (strcmp(plist_item, "refreshRate") == 0) {
        raop->refreshRate = (uint8_t) value;
        if ((int) raop->refreshRate != value) retval = 1;
    } else if (strcmp(plist_item, "maxFPS") == 0) {
        raop->maxFPS = (uint8_t) value;
        if ((int) raop->maxFPS != value) retval = 1;
    } else if (strcmp(plist_item, "overscanned") == 0) {
        raop->overscanned = (uint8_t) (value ? 1 : 0);
        if ((int) raop->overscanned  != value) retval = 1;
    } else if (strcmp(plist_item, "clientFPSdata") == 0) {
        raop->clientFPSdata = (value ? 1 : 0);
        if ((int) raop->clientFPSdata  != value) retval = 1;
    } else if (strcmp(plist_item, "max_ntp_timeouts") == 0) {
        raop->max_ntp_timeouts = (value > 0 ? value : 0);
        if (raop->max_ntp_timeouts != value) retval = 1;
    } else if (strcmp(plist_item, "audio_delay_micros") == 0) {
        if (value >= 0 && value <= 10 * SECOND_IN_USECS) {     
            raop->audio_delay_micros = value;
        }
        if (raop->audio_delay_micros != value) retval = 1;
    } else if (strcmp(plist_item, "pin") == 0) {
        raop->pin = value;
        raop->use_pin = true;
    } else {
        retval = -1;
    }	  
    return retval;
}

void
raop_set_port(raop_t *raop, unsigned short port) {
    assert(raop);
    raop->port = port;
}

void
raop_set_udp_ports(raop_t *raop, unsigned short udp[3]) {
    assert(raop);
    raop->timing_lport = udp[0]; 
    raop->control_lport = udp[1];
    raop->data_lport = udp[2];
}

void
raop_set_tcp_ports(raop_t *raop, unsigned short tcp[2]) {
    assert(raop);
    raop->mirror_data_lport = tcp[0];
    raop->port = tcp[1];
}

unsigned short
raop_get_port(raop_t *raop) {
    assert(raop);
    return raop->port;
}

void *
raop_get_callback_cls(raop_t *raop) {
    assert(raop);
    return raop->callbacks.cls;
}

void
raop_set_log_callback(raop_t *raop, raop_log_callback_t callback, void *cls) {
    assert(raop);

    logger_set_callback(raop->logger, callback, cls);
}

void
raop_set_dnssd(raop_t *raop, dnssd_t *dnssd) {
    assert(dnssd);
    dnssd_set_pk(dnssd, raop->pk_str);
    raop->dnssd = dnssd;
}


int
raop_start(raop_t *raop, unsigned short *port) {
    assert(raop);
    assert(port);
    return httpd_start(raop->httpd, port);
}

void
raop_stop(raop_t *raop) {
    assert(raop);
    httpd_stop(raop->httpd);
}
