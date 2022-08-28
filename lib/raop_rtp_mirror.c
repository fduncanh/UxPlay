/*
 * Copyright (c) 2019 dsafa22, All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "raop_rtp_mirror.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/tcp.h>

#include "raop.h"
#include "netutils.h"
#include "compat.h"
#include "logger.h"
#include "byteutils.h"
#include "mirror_buffer.h"
#include "stream.h"
#include "utils.h"
#include "plist/plist.h"

#ifdef __MINGW64__
#include "ws2ipdef.h"

#define TCP_KEEPIDLE SO_KEEPALIVE
#define TCP_KEEPINTVL SO_KEEPALIVE
#define TCP_KEEPCNT SO_KEEPALIVE

#endif

#define SEC 1000000
/* for MacOS, where SOL_TCP and TCP_KEEPIDLE are not defined */
#if !defined(SOL_TCP) && defined(IPPROTO_TCP)
#define SOL_TCP IPPROTO_TCP
#endif
#if !defined(TCP_KEEPIDLE) && defined(TCP_KEEPALIVE)
#define TCP_KEEPIDLE TCP_KEEPALIVE
#endif

//struct h264codec_s {
//    unsigned char compatibility;
//    short pps_size;
//    short sps_size;
//    unsigned char level;
//    unsigned char number_of_pps;
//    unsigned char* picture_parameter_set;
//    unsigned char profile_high;
//    unsigned char reserved_3_and_sps;
//    unsigned char reserved_6_and_nal;
//    unsigned char* sequence_parameter_set;
//    unsigned char version;
//};

struct raop_rtp_mirror_s {
    logger_t *logger;
    raop_callbacks_t callbacks;
    raop_ntp_t *ntp;

    /* Buffer to handle all resends */
    mirror_buffer_t *buffer;

    /* Remote address as sockaddr */
    struct sockaddr_storage remote_saddr;
    socklen_t remote_saddr_len;

    /* MUTEX LOCKED VARIABLES START */
    /* These variables only edited mutex locked */
    int running;
    int joined;

    int flush;
    thread_handle_t thread_mirror;
    mutex_handle_t run_mutex;

    /* MUTEX LOCKED VARIABLES END */
    int mirror_data_sock;

    unsigned short mirror_data_lport;

     /* switch for displaying client FPS data */
     uint8_t show_client_FPS_data;

    /* SPS and PPS */
    int sps_pps_len;
    unsigned char* sps_pps;
    bool sps_pps_waiting;

};

static int
raop_rtp_parse_remote(raop_rtp_mirror_t *raop_rtp_mirror, const unsigned char *remote, int remotelen)
{
    char current[25];
    int family;
    int ret;
    assert(raop_rtp_mirror);
    if (remotelen == 4) {
        family = AF_INET;
    } else if (remotelen == 16) {
        family = AF_INET6;
    } else {
        return -1;
    }
    memset(current, 0, sizeof(current));
    sprintf(current, "%d.%d.%d.%d", remote[0], remote[1], remote[2], remote[3]);
    logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror parse remote ip = %s", current);
    ret = netutils_parse_address(family, current,
                                 &raop_rtp_mirror->remote_saddr,
                                 sizeof(raop_rtp_mirror->remote_saddr));
    if (ret < 0) {
        return -1;
    }
    raop_rtp_mirror->remote_saddr_len = ret;
    return 0;
}

#define NO_FLUSH (-42)
raop_rtp_mirror_t *raop_rtp_mirror_init(logger_t *logger, raop_callbacks_t *callbacks, raop_ntp_t *ntp,
                                        const unsigned char *remote, int remotelen, const unsigned char *aeskey)
{
    raop_rtp_mirror_t *raop_rtp_mirror;

    assert(logger);
    assert(callbacks);

    raop_rtp_mirror = calloc(1, sizeof(raop_rtp_mirror_t));
    if (!raop_rtp_mirror) {
        return NULL;
    }
    raop_rtp_mirror->logger = logger;
    raop_rtp_mirror->ntp = ntp;
    raop_rtp_mirror->sps_pps_len = 0;
    raop_rtp_mirror->sps_pps = NULL;
    raop_rtp_mirror->sps_pps_waiting = false;

    memcpy(&raop_rtp_mirror->callbacks, callbacks, sizeof(raop_callbacks_t));
    raop_rtp_mirror->buffer = mirror_buffer_init(logger, aeskey);
    if (!raop_rtp_mirror->buffer) {
        free(raop_rtp_mirror);
        return NULL;
    }
    if (raop_rtp_parse_remote(raop_rtp_mirror, remote, remotelen) < 0) {
        free(raop_rtp_mirror);
        return NULL;
    }
    raop_rtp_mirror->running = 0;
    raop_rtp_mirror->joined = 1;
    raop_rtp_mirror->flush = NO_FLUSH;

    MUTEX_CREATE(raop_rtp_mirror->run_mutex);
    return raop_rtp_mirror;
}

void
raop_rtp_init_mirror_aes(raop_rtp_mirror_t *raop_rtp_mirror, uint64_t *streamConnectionID)
{
    mirror_buffer_init_aes(raop_rtp_mirror->buffer, streamConnectionID);
}

//#define DUMP_H264

#define RAOP_PACKET_LEN 32768
/**
 * Mirror
 */
static THREAD_RETVAL
raop_rtp_mirror_thread(void *arg)
{
    raop_rtp_mirror_t *raop_rtp_mirror = arg;
    assert(raop_rtp_mirror);

    int stream_fd = -1;
    unsigned char packet[128];
    memset(packet, 0 , 128);
    unsigned char* payload = NULL;
    unsigned int readstart = 0;
    bool conn_reset = false;
    uint64_t ntp_timestamp_nal = 0;
    uint64_t ntp_timestamp_raw = 0;
    unsigned char nal_start_code[4] = { 0x00, 0x00, 0x00, 0x01 };

#ifdef DUMP_H264
    // C decrypted
    FILE* file = fopen("/home/pi/Airplay.h264", "wb");
    // Encrypted source file
    FILE* file_source = fopen("/home/pi/Airplay.source", "wb");
    FILE* file_len = fopen("/home/pi/Airplay.len", "wb");
#endif

    while (1) {
        fd_set rfds;
        struct timeval tv;
        int nfds, ret;
        MUTEX_LOCK(raop_rtp_mirror->run_mutex);
        if (!raop_rtp_mirror->running) {
            MUTEX_UNLOCK(raop_rtp_mirror->run_mutex);
            logger_log(raop_rtp_mirror->logger, LOGGER_ERR, "raop_rtp_mirror->running is no longer true");
            break;
        }
        MUTEX_UNLOCK(raop_rtp_mirror->run_mutex);

        /* Set timeout valu to 5ms */
        tv.tv_sec = 0;
        tv.tv_usec = 5000;

        /* Get the correct nfds value and set rfds */
        FD_ZERO(&rfds);
        if (stream_fd == -1) {
            FD_SET(raop_rtp_mirror->mirror_data_sock, &rfds);
            nfds = raop_rtp_mirror->mirror_data_sock+1;
        } else {
            FD_SET(stream_fd, &rfds);
            nfds = stream_fd+1;
        }
        ret = select(nfds, &rfds, NULL, NULL, &tv);
        if (ret == 0) {
            /* Timeout happened */
            continue;
        } else if (ret == -1) {
            logger_log(raop_rtp_mirror->logger, LOGGER_ERR, "raop_rtp_mirror error in select");
            break;
        }

        if (stream_fd == -1 && FD_ISSET(raop_rtp_mirror->mirror_data_sock, &rfds)) {
            struct sockaddr_storage saddr;
            socklen_t saddrlen;
            logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror accepting client");
            saddrlen = sizeof(saddr);
            stream_fd = accept(raop_rtp_mirror->mirror_data_sock, (struct sockaddr *)&saddr, &saddrlen);
            if (stream_fd == -1) {
                logger_log(raop_rtp_mirror->logger, LOGGER_ERR, "raop_rtp_mirror error in accept %d %s", errno, strerror(errno));
                break;
            }

            // We're calling recv for a certain amount of data, so we need a timeout
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 5000;
            if (setsockopt(stream_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                logger_log(raop_rtp_mirror->logger, LOGGER_ERR, "raop_rtp_mirror could not set stream socket timeout %d %s", errno, strerror(errno));
                break;
            }
            int option;
            option = 1;
            if (setsockopt(stream_fd, SOL_SOCKET, SO_KEEPALIVE, &option, sizeof(option)) < 0) {
                logger_log(raop_rtp_mirror->logger, LOGGER_WARNING, "raop_rtp_mirror could not set stream socket keepalive %d %s", errno, strerror(errno));
            }
            option = 60;
            if (setsockopt(stream_fd, SOL_TCP, TCP_KEEPIDLE, &option, sizeof(option)) < 0) {
                logger_log(raop_rtp_mirror->logger, LOGGER_WARNING, "raop_rtp_mirror could not set stream socket keepalive time %d %s", errno, strerror(errno));
            }
            option = 10;
            if (setsockopt(stream_fd, SOL_TCP, TCP_KEEPINTVL, &option, sizeof(option)) < 0) {
                logger_log(raop_rtp_mirror->logger, LOGGER_WARNING, "raop_rtp_mirror could not set stream socket keepalive interval %d %s", errno, strerror(errno));
            }
            option = 6;
            if (setsockopt(stream_fd, SOL_TCP, TCP_KEEPCNT, &option, sizeof(option)) < 0) {
                logger_log(raop_rtp_mirror->logger, LOGGER_WARNING, "raop_rtp_mirror could not set stream socket keepalive probes %d %s", errno, strerror(errno));
            }
            readstart = 0;
        }

        if (stream_fd != -1 && FD_ISSET(stream_fd, &rfds)) {

            // The first 128 bytes are some kind of header for the payload that follows
            while (payload == NULL && readstart < 128) {
                ret = recv(stream_fd, packet + readstart, 128 - readstart, 0);
                if (ret <= 0) break;
                readstart = readstart + ret;
            }

            if (payload == NULL && ret == 0) {
                logger_log(raop_rtp_mirror->logger, LOGGER_ERR, "raop_rtp_mirror tcp socket is closed, got %d bytes of 128 byte header",readstart);
                FD_CLR(stream_fd, &rfds);
                stream_fd = -1;
                continue;
            } else if (payload == NULL && ret == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue; // Timeouts can happen even if the connection is fine
                logger_log(raop_rtp_mirror->logger, LOGGER_ERR, "raop_rtp_mirror error  in header recv: %d %s", errno, strerror(errno));
                if (errno == ECONNRESET) conn_reset = true;; 
                break;
            }

            /*packet[0:3] contains the payload size */
            int payload_size = byteutils_get_int(packet, 0);

            /* packet[4] appears to have one of three possible values:                           *
             * 0x00 : encrypted packet                                                           *    
             * 0x01 : unencrypted packet with a SPS and a PPS NAL, sent initially, and also when *
             *        a change in video format (e.g., width, height) subsequently occurs         *
             * 0x05 : unencrypted packet with a "streaming report", sent once per second         */

            /* encrypted packets have packet[5] = 0x00 or 0x10, and packet[6]= packet[7] = 0x00; *
             * encrypted packets immediately following an unencrypted SPS/PPS packet appear to   *
             * be the only ones with packet[5] = 0x10, and almost always have packet[5] = 0x10,  *
             * but occasionally have packet[5] = 0x00.                                           */

            /* unencrypted SPS/PPS packets have packet[4:7] = 0x01 0x00 0x01 0x16                *
             * they are followed by an encrypted packet with the same timestamp in packet[8:15]  */

            /* "streaming report" packages have packet[4:7] = 0x05 0x00 0x00 0x00, and have no    *
             * timestamp in packet[8:15]                                                         */

            //unsigned short payload_type = byteutils_get_short(packet, 4) & 0xff;
            //unsigned short payload_option = byteutils_get_short(packet, 6);

            if (payload == NULL) {
                payload = malloc(payload_size);
                readstart = 0;
            }

            while (readstart < payload_size) {
                // Payload data
                ret = recv(stream_fd, payload + readstart, payload_size - readstart, 0);
                if (ret <= 0) break;
                readstart = readstart + ret;
            }

            if (ret == 0) {
                logger_log(raop_rtp_mirror->logger, LOGGER_ERR, "raop_rtp_mirror tcp socket is closed");
                break;
            } else if (ret == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue; // Timeouts can happen even if the connection is fine
                logger_log(raop_rtp_mirror->logger, LOGGER_ERR, "raop_rtp_mirror error in recv: %d %s", errno, strerror(errno));
                if (errno == ECONNRESET) conn_reset = true;
                break;
            }

	    switch (packet[4]) {
            case  0x00:
                // Normal video data (VCL NAL)

                // Conveniently, the video data is already stamped with the remote wall clock time,
                // so no additional clock syncing needed. The only thing odd here is that the video
                // ntp time stamps don't include the SECONDS_FROM_1900_TO_1970, so it's really just
                // counting micro seconds since last boot.
                ntp_timestamp_raw = byteutils_get_long(packet, 8);
                uint64_t ntp_timestamp_remote = raop_ntp_timestamp_to_micro_seconds(ntp_timestamp_raw, false);
                uint64_t ntp_timestamp = raop_ntp_convert_remote_time(raop_rtp_mirror->ntp, ntp_timestamp_remote);

                uint64_t ntp_now = raop_ntp_get_local_time(raop_rtp_mirror->ntp);
                int64_t latency = ((int64_t) ntp_now) - ((int64_t) ntp_timestamp);
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp video: now = %8.6f, ntp = %8.6f, latency = %8.6f",
                           ((double) ntp_now) / SEC, ((double) ntp_timestamp) / SEC, ((double) latency) / SEC);

#ifdef DUMP_H264
                fwrite(payload, payload_size, 1, file_source);
                fwrite(&readstart, sizeof(readstart), 1, file_len);
#endif
                unsigned char* payload_out;
		unsigned char* payload_decrypted;
                if (!raop_rtp_mirror->sps_pps_waiting && packet[5] != 0x00) {
                    logger_log(raop_rtp_mirror->logger, LOGGER_WARNING, "unexpected: packet[5] = %2.2x, but  not preceded  by SPS+PPS packet", packet[5]);
                }
                /* if a previous unencrypted packet contains an SPS (type 7) and PPS (type 8) NAL which has not 
                 * yet been sent, it should be prepended to the current NAL.    In this case packet[5] is usually 
                 * 0x10; however, the M1 Macs have increased the h264 level, and now the encrypted packet after the
                 * unencrypted SPS+PPS packet may contain a SEI (type 6) NAL prepended to the next VCL NAL, with
                 * packet[5] = 0x00.   Now the flag raop_rtp_mirror->sps_pps_waiting = true will signal that a 
                 * previous packet contained a SPS NAL + a PPS NAL, that has not yet been sent.   This will trigger
                 * prepending it to the current NAL, and the sps_pps_waiting flag will be set to false after
                 * it has been prepended.    It is not clear if the case packet[5] = 0x10 will occur when
                 * raop_rtp_mirror->sps_pps = false, but if it does, the current code will prepend the stored
                 * PPS + SPS NAL to the current encrypted NAL, and issue a warning message */

                bool prepend_sps_pps = (raop_rtp_mirror->sps_pps_waiting || packet[5] != 0x00);
                if (prepend_sps_pps) {
                    assert(raop_rtp_mirror->sps_pps);
                    payload_out = (unsigned char*)  malloc(payload_size + raop_rtp_mirror->sps_pps_len);
                    payload_decrypted = payload_out + raop_rtp_mirror->sps_pps_len;
                    memcpy(payload_out, raop_rtp_mirror->sps_pps, raop_rtp_mirror->sps_pps_len);
                    raop_rtp_mirror->sps_pps_waiting = false;
                } else {
                    payload_out = (unsigned char*)  malloc(payload_size);
                    payload_decrypted = payload_out;
                }
                // Decrypt data
                mirror_buffer_decrypt(raop_rtp_mirror->buffer, payload, payload_decrypted, payload_size);

                // It seems the AirPlay protocol prepends NALs with their size, which we're replacing with the 4-byte
                // start code for the NAL Byte-Stream Format.
                bool valid_data = true;
                int nalu_size = 0;
                int nalus_count = 0;
                int nalu_type;               /* 0x01 non-IDR VCL, 0x05 IDR VCL, 0x06 SEI 0x07 SPS, 0x08 PPS */
                while (nalu_size < payload_size) {
                    int nc_len = byteutils_get_int_be(payload_decrypted, nalu_size);
                    if (nc_len < 0 || nalu_size + 4 > payload_size) {
                        valid_data = false;
                        break;
                    }
                    memcpy(payload_decrypted + nalu_size, nal_start_code, 4);
                    nalu_size += 4;
                    nalus_count++;
                    if (payload_decrypted[nalu_size] & 0x80) valid_data = false;  /* first bit of h264 nalu MUST be 0 ("forbidden_zero_bit") */
                    nalu_type = payload_decrypted[nalu_size] & 0x1f;
                    nalu_size += nc_len;
                    if (nalu_type != 1) {
                         logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "nalu_type = %d, nalu_size = %d,  processed bytes %d, payloadsize = %d nalus_count = %d",
                                    nalu_type, nc_len, nalu_size, payload_size, nalus_count);
                    }
                 }
                if (nalu_size != payload_size) valid_data = false;
                if(!valid_data) {
                    logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "nalu marked as invalid");
                    payload_out[0] = 1; /* mark video data as invalid h264 (failed decryption) */
                }
#ifdef DUMP_H264
                fwrite(payload_decrypted, payload_size, 1, file);
#endif
                payload_decrypted = NULL;
                h264_decode_struct h264_data;
                h264_data.pts = ntp_timestamp;
                h264_data.nal_count = nalus_count;   /*nal_count will be the number of nal units in the packet */
                h264_data.data_len = payload_size;
                h264_data.data = payload_out;
                if (prepend_sps_pps) {
                    h264_data.data_len += raop_rtp_mirror->sps_pps_len;
                    h264_data.nal_count += 2;
                    if (ntp_timestamp_raw != ntp_timestamp_nal) {
                        logger_log(raop_rtp_mirror->logger, LOGGER_WARNING, "raop_rtp_mirror: prepended sps_pps timestamp does not match that of video payload");
                    }
                }
                raop_rtp_mirror->callbacks.video_process(raop_rtp_mirror->callbacks.cls, raop_rtp_mirror->ntp, &h264_data);
                free(payload_out);
                break;
            case 0x01:
                // The information in the payload contains an SPS and a PPS NAL
                // The sps_pps is not encrypted
                ntp_timestamp_nal = byteutils_get_long(packet, 8);
                float width = byteutils_get_float(packet, 16);
                float height = byteutils_get_float(packet, 20);
                float width_source = byteutils_get_float(packet, 40);
                float height_source = byteutils_get_float(packet, 44);
                if (width != width_source || height != height_source) {
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror: Unexpected : data  %f, %f != width_source = %f, height_source = %f",
                           width, height, width_source, height_source);
                }
                width = byteutils_get_float(packet, 48);
                height = byteutils_get_float(packet, 52);
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror: unidentified extra header data  %f, %f", width, height);
                width = byteutils_get_float(packet, 56);
                height = byteutils_get_float(packet, 60);
                if (raop_rtp_mirror->callbacks.video_report_size) {
                    raop_rtp_mirror->callbacks.video_report_size(raop_rtp_mirror->callbacks.cls, &width_source, &height_source, &width, &height);
                }
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror width_source = %f height_source = %f width = %f height = %f",
                           width_source, height_source, width, height);

                short sps_size = byteutils_get_short_be(payload,6);
                unsigned char *sequence_parameter_set = payload + 8;
                short pps_size = byteutils_get_short_be(payload, sps_size + 9);
                unsigned char *picture_parameter_set = payload + sps_size + 11;
                int data_size = 6; 
                char *str = utils_data_to_string(payload, data_size, 16);
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror: sps/pps header size = %d", data_size);		
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror h264 sps/pps header:\n%s", str);
                free(str);
                str = utils_data_to_string(sequence_parameter_set, sps_size,16);
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror sps size = %d",  sps_size);		
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror h264 Sequence Parameter Set:\n%s", str);
                free(str);
                str = utils_data_to_string(picture_parameter_set, pps_size, 16);
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror pps size = %d", pps_size);
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror h264 Picture Parameter Set:\n%s", str);
                free(str);
                data_size = payload_size - sps_size - pps_size - 11; 
                if (data_size > 0) {
                    str = utils_data_to_string (picture_parameter_set + pps_size, data_size, 16);
                    logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "remainder size = %d", data_size);
                    logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "remainder of sps+pps packet:\n%s", str);
                    free(str);
                } else if (data_size < 0) {
                    logger_log(raop_rtp_mirror->logger, LOGGER_ERR, " pps_sps error: packet remainder size = %d < 0", data_size);
                }

                // Copy the sps and pps into a buffer to prepend to the next NAL unit.
                raop_rtp_mirror->sps_pps_len = sps_size + pps_size + 8;
                if (raop_rtp_mirror->sps_pps) {
                    free(raop_rtp_mirror->sps_pps);
                }
                raop_rtp_mirror->sps_pps = (unsigned char*) malloc(raop_rtp_mirror->sps_pps_len);
                assert(raop_rtp_mirror->sps_pps);
                memcpy(raop_rtp_mirror->sps_pps, nal_start_code, 4);
                memcpy(raop_rtp_mirror->sps_pps + 4, sequence_parameter_set, sps_size);
                memcpy(raop_rtp_mirror->sps_pps + sps_size + 4, nal_start_code, 4); 
                memcpy(raop_rtp_mirror->sps_pps + sps_size + 8, payload + sps_size + 11, pps_size);
                raop_rtp_mirror->sps_pps_waiting = true;
#ifdef DUMP_H264
                fwrite(raop_rtp_mirror->sps_pps, raop_rtp_mirror->sps_pps_len, 1, file);
#endif

                // h264codec_t h264;
                // h264.version = payload[0];
                // h264.profile_high = payload[1];
                // h264.compatibility = payload[2];
                // h264.level = payload[3];
                // h264.reserved_6_and_nal = payload[4];
                // h264.reserved_3_and_sps = payload[5];
                // h264.sps_size =  sps_size;
                // h264.sequence_parameter_set = malloc(h264.sps_size);
                // memcpy(h264.sequence_parameter_set, sequence_parameter_set, sps_size);
                // h264.number_of_pps = payload[h264.sps_size + 8];
                // h264.pps_size = pps_size;
                // h264.picture_parameter_set = malloc(h264.pps_size);
                // memcpy(h264.picture_parameter_set, picture_parameter_set, pps_size);

                break;
            case 0x05:
                logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "\nReceived video streaming performance info packet from client");
                /* payloads with packet[4] = 0x05 have no timestamp, and carry video info from the client as a binary plist *
                 * Sometimes (e.g, when the client has a locked screen), there is a 25kB trailer attached to the packet.    *
                 * This 25000 Byte trailer with unidentified content seems to be the same data each time it is sent.        */

                if (payload_size && raop_rtp_mirror->show_client_FPS_data) {
                    //char *str = utils_data_to_string(packet, 128, 16);
                    //logger_log(raop_rtp_mirror->logger, LOGGER_WARNING, "type 5 video packet header:\n%s", str);
                    //free (str);
		    
                    int plist_size = payload_size;
                    if (payload_size > 25000) {
		        plist_size = payload_size - 25000;
                        char *str = utils_data_to_string(payload + plist_size, 16, 16);
                        logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "video_info packet had 25kB trailer; first 16 bytes are:\n%s", str);
                        free(str);
                    }
                    if (plist_size) {
                        char *plist_xml;
                        uint32_t plist_len;
                        plist_t root_node = NULL;
                        plist_from_bin((char *) payload, plist_size, &root_node);
                        plist_to_xml(root_node, &plist_xml, &plist_len);
                        logger_log(raop_rtp_mirror->logger, LOGGER_INFO, "%s", plist_xml);
                        free(plist_xml);
                    }
                }
                break;
            default:
                logger_log(raop_rtp_mirror->logger, LOGGER_WARNING, "\nReceived unexpected TCP packet from client, packet[4] = 0x%2.2x", packet[4]);
                break;
            }

            free(payload);
            payload = NULL;
            memset(packet, 0, 128);
            readstart = 0;
        }
    }

    /* Close the stream file descriptor */
    if (stream_fd != -1) {
        closesocket(stream_fd);
    }

#ifdef DUMP_H264
    fclose(file);
    fclose(file_source);
    fclose(file_len);
#endif

    // Ensure running reflects the actual state
    MUTEX_LOCK(raop_rtp_mirror->run_mutex);
    raop_rtp_mirror->running = false;
    MUTEX_UNLOCK(raop_rtp_mirror->run_mutex);

    logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror exiting TCP thread");
    if (conn_reset && raop_rtp_mirror->callbacks.conn_reset) {
        const bool video_reset = false;   /* leave "frozen video" showing */
        raop_rtp_mirror->callbacks.conn_reset(raop_rtp_mirror->callbacks.cls, 0, video_reset);
    }
    return 0;
}

static int
raop_rtp_init_mirror_sockets(raop_rtp_mirror_t *raop_rtp_mirror, int use_ipv6)
{
    assert(raop_rtp_mirror);

    unsigned short dport = raop_rtp_mirror->mirror_data_lport;
    int dsock = netutils_init_socket(&dport, use_ipv6, 0);
    if (dsock == -1) {
        goto sockets_cleanup;
    }

    /* Listen to the data socket if using TCP */
    if (listen(dsock, 1) < 0) {
        goto sockets_cleanup;
    }

    /* Set socket descriptors */
    raop_rtp_mirror->mirror_data_sock = dsock;

    /* Set port values */
    raop_rtp_mirror->mirror_data_lport = dport;
    logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "raop_rtp_mirror local data port socket %d port TCP %d", dsock, dport);
    return 0;

    sockets_cleanup:
    if (dsock != -1) closesocket(dsock);
    return -1;
}

void
raop_rtp_start_mirror(raop_rtp_mirror_t *raop_rtp_mirror, int use_udp, unsigned short *mirror_data_lport, uint8_t show_client_FPS_data)
{
    logger_log(raop_rtp_mirror->logger, LOGGER_INFO, "raop_rtp_mirror starting mirroring");
    int use_ipv6 = 0;

    assert(raop_rtp_mirror);
    assert(mirror_data_lport);
    raop_rtp_mirror->show_client_FPS_data = show_client_FPS_data;

    MUTEX_LOCK(raop_rtp_mirror->run_mutex);
    if (raop_rtp_mirror->running || !raop_rtp_mirror->joined) {
        MUTEX_UNLOCK(raop_rtp_mirror->run_mutex);
        return;
    }

    if (raop_rtp_mirror->remote_saddr.ss_family == AF_INET6) {
        use_ipv6 = 1;
    }
    use_ipv6 = 0;
     
    raop_rtp_mirror->mirror_data_lport = *mirror_data_lport;
    if (raop_rtp_init_mirror_sockets(raop_rtp_mirror, use_ipv6) < 0) {
        logger_log(raop_rtp_mirror->logger, LOGGER_ERR, "raop_rtp_mirror initializing sockets failed");
        MUTEX_UNLOCK(raop_rtp_mirror->run_mutex);
        return;
    }
    *mirror_data_lport = raop_rtp_mirror->mirror_data_lport;

    /* Create the thread and initialize running values */
    raop_rtp_mirror->running = 1;
    raop_rtp_mirror->joined = 0;

    THREAD_CREATE(raop_rtp_mirror->thread_mirror, raop_rtp_mirror_thread, raop_rtp_mirror);
    MUTEX_UNLOCK(raop_rtp_mirror->run_mutex);
}

void raop_rtp_mirror_stop(raop_rtp_mirror_t *raop_rtp_mirror) {
    assert(raop_rtp_mirror);

    /* Check that we are running and thread is not
     * joined (should never be while still running) */
    MUTEX_LOCK(raop_rtp_mirror->run_mutex);
    if (!raop_rtp_mirror->running || raop_rtp_mirror->joined) {
        MUTEX_UNLOCK(raop_rtp_mirror->run_mutex);
        return;
    }
    raop_rtp_mirror->running = 0;
    MUTEX_UNLOCK(raop_rtp_mirror->run_mutex);

    if (raop_rtp_mirror->mirror_data_sock != -1) {
        closesocket(raop_rtp_mirror->mirror_data_sock);
        raop_rtp_mirror->mirror_data_sock = -1;
    }

    /* Join the thread */
    THREAD_JOIN(raop_rtp_mirror->thread_mirror);

    /* Mark thread as joined */
    MUTEX_LOCK(raop_rtp_mirror->run_mutex);
    raop_rtp_mirror->joined = 1;
    MUTEX_UNLOCK(raop_rtp_mirror->run_mutex);
}

void raop_rtp_mirror_destroy(raop_rtp_mirror_t *raop_rtp_mirror) {
    if (raop_rtp_mirror) {
        raop_rtp_mirror_stop(raop_rtp_mirror);
        MUTEX_DESTROY(raop_rtp_mirror->run_mutex);
        mirror_buffer_destroy(raop_rtp_mirror->buffer);
        if (raop_rtp_mirror->sps_pps) {
            free(raop_rtp_mirror->sps_pps);
        }
	free(raop_rtp_mirror);
    }
}
