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
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#include "raop_rtp.h"
#include "raop.h"
#include "raop_buffer.h"
#include "netutils.h"
#include "compat.h"
#include "logger.h"
#include "byteutils.h"
#include "mirror_buffer.h"
#include "stream.h"
#include "utils.h"

#define NO_FLUSH (-42)

#define RAOP_RTP_SAMPLE_RATE (44100.0 / 1000000.0)
#define RAOP_RTP_SYNC_DATA_COUNT 8
#define SEC 1000000

#define DELAY_AAC 500000 //this is empirical, there is about a 0.5 sec delay in initial audio timing before  a clock sync event
#define DELAY_ALAC 2000000 //this is empirical, there is about a 2.0 sec delay in initial audio timing before  a clock sync event

typedef struct raop_rtp_sync_data_s {
    uint64_t ntp_time;  // The local wall clock time at the time of rtp_time
    uint32_t rtp_time;  // The remote rtp clock time corresponding to ntp_time
    uint32_t rtp_epoch; // The epoch of the remote rtp clock (rtp epoch is about 27 hours) 
} raop_rtp_sync_data_t;

struct raop_rtp_s {
    logger_t *logger;
    raop_callbacks_t callbacks;

    // Time and sync
    raop_ntp_t *ntp;
    double rtp_sync_scale;
    int64_t rtp_sync_offset;
    uint32_t rtp_sync_epoch;
    raop_rtp_sync_data_t sync_data[RAOP_RTP_SYNC_DATA_COUNT];
    int sync_data_index;

    // Transmission Stats, could be used if a playout buffer is needed
    // float interarrival_jitter; // As defined by RTP RFC 3550, Section 6.4.1
    // unsigned int last_packet_transit_time;
    //int transit = (packet_receive_time - packet_send_time);
    // int d = transit - last_packet_transit_time;
    // if (d < 0) d = -d;
    // interarrival_jitter = (1.f / 16.f) * ((double) d - interarrival_jitter);

    /* Buffer to handle all resends */
    raop_buffer_t *buffer;

    /* Remote address as sockaddr */
    struct sockaddr_storage remote_saddr;
    socklen_t remote_saddr_len;

    /* MUTEX LOCKED VARIABLES START */
    /* These variables only edited mutex locked */
    int running;
    int joined;

    float volume;
    int volume_changed;
    unsigned char *metadata;
    int metadata_len;
    unsigned char *coverart;
    int coverart_len;
    char *dacp_id;
    char *active_remote_header;
    unsigned int progress_start;
    unsigned int progress_curr;
    unsigned int progress_end;
    int progress_changed;

    int flush;
    thread_handle_t thread;
    mutex_handle_t run_mutex;
    /* MUTEX LOCKED VARIABLES END */

    /* Remote control and timing ports */
    unsigned short control_rport;

    /* Sockets for control and data */
    int csock, dsock;

    /* Local control, timing and data ports */
    unsigned short control_lport;
    unsigned short data_lport;

    /* Initialized after the first control packet */
    struct sockaddr_storage control_saddr;
    socklen_t control_saddr_len;
    unsigned short control_seqnum;

    /* audio compression type: ct = 2 (ALAC), ct = 8 (AAC_ELD) (ct = 4 would be AAC-MAIN) */
    unsigned char ct;
};

static int
raop_rtp_parse_remote(raop_rtp_t *raop_rtp, const unsigned char *remote, int remotelen)
{
    char current[25];
    int family;
    int ret;
    assert(raop_rtp);
    if (remotelen == 4) {
        family = AF_INET;
    } else if (remotelen == 16) {
        family = AF_INET6;
    } else {
        return -1;
    }
    memset(current, 0, sizeof(current));
    sprintf(current, "%d.%d.%d.%d", remote[0], remote[1], remote[2], remote[3]);
    logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp parse remote ip = %s", current);
    ret = netutils_parse_address(family, current,
                                 &raop_rtp->remote_saddr,
                                 sizeof(raop_rtp->remote_saddr));
    if (ret < 0) {
        return -1;
    }
    raop_rtp->remote_saddr_len = ret;
    return 0;
}

raop_rtp_t *
raop_rtp_init(logger_t *logger, raop_callbacks_t *callbacks, raop_ntp_t *ntp, const unsigned char *remote, 
              int remotelen, const unsigned char *aeskey, const unsigned char *aesiv)
{
    raop_rtp_t *raop_rtp;

    assert(logger);
    assert(callbacks);

    raop_rtp = calloc(1, sizeof(raop_rtp_t));
    if (!raop_rtp) {
        return NULL;
    }
    raop_rtp->logger = logger;
    raop_rtp->ntp = ntp;

    raop_rtp->rtp_sync_offset = 0;
    raop_rtp->rtp_sync_scale = RAOP_RTP_SAMPLE_RATE;
    raop_rtp->sync_data_index = 0;
    raop_rtp->rtp_sync_epoch = 0;
    for (int i = 0; i < RAOP_RTP_SYNC_DATA_COUNT; ++i) {
        raop_rtp->sync_data[i].ntp_time = 0;
        raop_rtp->sync_data[i].rtp_time = 0;
        raop_rtp->sync_data[i].rtp_epoch = 0;
    }

    raop_rtp->dacp_id = NULL;
    raop_rtp->active_remote_header = NULL;
    raop_rtp->metadata = NULL;
    raop_rtp->coverart = NULL;

    memcpy(&raop_rtp->callbacks, callbacks, sizeof(raop_callbacks_t));
    raop_rtp->buffer = raop_buffer_init(logger, aeskey, aesiv);
    if (!raop_rtp->buffer) {
        free(raop_rtp);
        return NULL;
    }
    if (raop_rtp_parse_remote(raop_rtp, remote, remotelen) < 0) {
        free(raop_rtp);
        return NULL;
    }

    raop_rtp->running = 0;
    raop_rtp->joined = 1;
    raop_rtp->flush = NO_FLUSH;

    MUTEX_CREATE(raop_rtp->run_mutex);
    return raop_rtp;
}


void
raop_rtp_destroy(raop_rtp_t *raop_rtp)
{
    if (raop_rtp) {
        raop_rtp_stop(raop_rtp);
        MUTEX_DESTROY(raop_rtp->run_mutex);
        raop_buffer_destroy(raop_rtp->buffer);
        free(raop_rtp->metadata);
        free(raop_rtp->coverart);
        free(raop_rtp->dacp_id);
        free(raop_rtp->active_remote_header);
        free(raop_rtp);
    }
}

static int
raop_rtp_resend_callback(void *opaque, unsigned short seqnum, unsigned short count)
{
    raop_rtp_t *raop_rtp = opaque;
    unsigned char packet[8];
    unsigned short ourseqnum;
    struct sockaddr *addr;
    socklen_t addrlen;
    int ret;

    addr = (struct sockaddr *)&raop_rtp->control_saddr;
    addrlen = raop_rtp->control_saddr_len;

    logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp got resend request %d %d", seqnum, count);
    ourseqnum = raop_rtp->control_seqnum++;

    /* Fill the request buffer */
    packet[0] = 0x80;
    packet[1] = 0x55|0x80;
    packet[2] = (ourseqnum >> 8);
    packet[3] =  ourseqnum;
    packet[4] = (seqnum >> 8);
    packet[5] =  seqnum;
    packet[6] = (count >> 8);
    packet[7] =  count;

    ret = sendto(raop_rtp->csock, (const char *)packet, sizeof(packet), 0, addr, addrlen);
    if (ret == -1) {
        logger_log(raop_rtp->logger, LOGGER_WARNING, "raop_rtp resend failed: %d", SOCKET_GET_ERROR());
    }

    return 0;
}

static int
raop_rtp_init_sockets(raop_rtp_t *raop_rtp, int use_ipv6, int use_udp)
{
    assert(raop_rtp);

    unsigned short cport = raop_rtp->control_lport;
    unsigned short dport = raop_rtp->data_lport;
    int csock = netutils_init_socket(&cport, use_ipv6, 1);
    int dsock = netutils_init_socket(&dport, use_ipv6, 1);

    if (csock == -1 || dsock == -1) {
        goto sockets_cleanup;
    }

    /* Set socket descriptors */
    raop_rtp->csock = csock;
    raop_rtp->dsock = dsock;

    /* Set port values */
    raop_rtp->control_lport = cport;
    raop_rtp->data_lport = dport;
    logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp local control port socket %d port UDP %d", csock, cport);
    logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp local data port    socket %d port UDP %d", dsock, dport);
    return 0;

    sockets_cleanup:
    if (csock != -1) closesocket(csock);
    if (dsock != -1) closesocket(dsock);
    return -1;
}

static int
raop_rtp_process_events(raop_rtp_t *raop_rtp, void *cb_data)
{
    int flush;
    float volume;
    int volume_changed;
    unsigned char *metadata;
    int metadata_len;
    unsigned char *coverart;
    int coverart_len;
    char *dacp_id;
    char *active_remote_header;
    unsigned int progress_start;
    unsigned int progress_curr;
    unsigned int progress_end;
    int progress_changed;

    assert(raop_rtp);

    MUTEX_LOCK(raop_rtp->run_mutex);
    if (!raop_rtp->running) {
        MUTEX_UNLOCK(raop_rtp->run_mutex);
        return 1;
    }

    /* Read the volume level */
    volume = raop_rtp->volume;
    volume_changed = raop_rtp->volume_changed;
    raop_rtp->volume_changed = 0;

    /* Read the flush value */
    flush = raop_rtp->flush;
    raop_rtp->flush = NO_FLUSH;

    /* Read the metadata */
    metadata = raop_rtp->metadata;
    metadata_len = raop_rtp->metadata_len;
    raop_rtp->metadata = NULL;
    raop_rtp->metadata_len = 0;

    /* Read the coverart */
    coverart = raop_rtp->coverart;
    coverart_len = raop_rtp->coverart_len;
    raop_rtp->coverart = NULL;
    raop_rtp->coverart_len = 0;

    /* Read DACP remote control data */
    dacp_id = raop_rtp->dacp_id;
    active_remote_header = raop_rtp->active_remote_header;
    raop_rtp->dacp_id = NULL;
    raop_rtp->active_remote_header = NULL;

    /* Read the progress values */
    progress_start = raop_rtp->progress_start;
    progress_curr = raop_rtp->progress_curr;
    progress_end = raop_rtp->progress_end;
    progress_changed = raop_rtp->progress_changed;
    raop_rtp->progress_changed = 0;

    MUTEX_UNLOCK(raop_rtp->run_mutex);

    /* Call set_volume callback if changed */
    if (volume_changed) {
        raop_buffer_flush(raop_rtp->buffer, flush);
        if (raop_rtp->callbacks.audio_set_volume) {
            raop_rtp->callbacks.audio_set_volume(raop_rtp->callbacks.cls, volume);
        }
    }

    /* Handle flush if requested */
    if (flush != NO_FLUSH) {
        if (raop_rtp->callbacks.audio_flush) {
            raop_rtp->callbacks.audio_flush(raop_rtp->callbacks.cls);
        }
    }

    if (metadata != NULL) {
        if (raop_rtp->callbacks.audio_set_metadata) {
            raop_rtp->callbacks.audio_set_metadata(raop_rtp->callbacks.cls, metadata, metadata_len);
        }
        free(metadata);
        metadata = NULL;
    }

    if (coverart != NULL) {
        if (raop_rtp->callbacks.audio_set_coverart) {
            raop_rtp->callbacks.audio_set_coverart(raop_rtp->callbacks.cls, coverart, coverart_len);
        }
        free(coverart);
        coverart = NULL;
    }
    if (dacp_id && active_remote_header) {
        if (raop_rtp->callbacks.audio_remote_control_id) {
            raop_rtp->callbacks.audio_remote_control_id(raop_rtp->callbacks.cls, dacp_id, active_remote_header);
        }
        free(dacp_id);
        free(active_remote_header);
        dacp_id = NULL;
        active_remote_header = NULL;
    }

    if (progress_changed) {
        if (raop_rtp->callbacks.audio_set_progress) {
            raop_rtp->callbacks.audio_set_progress(raop_rtp->callbacks.cls, progress_start, progress_curr, progress_end);
        }
    }
    return 0;
}

void raop_rtp_sync_clock(raop_rtp_t *raop_rtp, uint64_t ntp_time, int64_t ntp_start_time, uint32_t rtp_time, int shift) {
    const double epoch_length = (double) 0x100000000ULL;
    if (rtp_time < raop_rtp->sync_data[raop_rtp->sync_data_index].rtp_time) {
        raop_rtp->rtp_sync_epoch++;
    }
    raop_rtp->sync_data_index = (raop_rtp->sync_data_index + 1) % RAOP_RTP_SYNC_DATA_COUNT;
    raop_rtp->sync_data[raop_rtp->sync_data_index].rtp_time = rtp_time;
    raop_rtp->sync_data[raop_rtp->sync_data_index].ntp_time = ntp_time;
    raop_rtp->sync_data[raop_rtp->sync_data_index].rtp_epoch = raop_rtp->rtp_sync_epoch;

    uint32_t valid_data_count = 0;
    valid_data_count = 0;
    int64_t total_offsets = 0;
    double rtp_offset;
    for (int i = 0; i < RAOP_RTP_SYNC_DATA_COUNT; ++i) {
        if (raop_rtp->sync_data[i].ntp_time == 0) continue;
        rtp_offset = (((double) raop_rtp->sync_data[i].rtp_time) + ((double) shift));
        if (raop_rtp->sync_data[i].rtp_epoch) {
            rtp_offset += raop_rtp->sync_data[i].rtp_epoch * epoch_length;
        }
        total_offsets += (int64_t) (rtp_offset / raop_rtp-> rtp_sync_scale);
        if (raop_rtp->sync_data[i].ntp_time > ntp_start_time) {
            total_offsets -= raop_rtp->sync_data[i].ntp_time - ntp_start_time;
        } else {
             total_offsets += ntp_start_time - raop_rtp->sync_data[i].ntp_time;
        }
        valid_data_count++;
    }
    int64_t avg_offset = total_offsets / valid_data_count;
    int64_t correction = avg_offset - raop_rtp->rtp_sync_offset;
    raop_rtp->rtp_sync_offset = avg_offset;

    logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp sync correction=%lld", correction);
}

static THREAD_RETVAL
raop_rtp_thread_udp(void *arg)
{
    raop_rtp_t *raop_rtp = arg;
    unsigned char packet[RAOP_PACKET_LEN];
    unsigned int packetlen;
    struct sockaddr_storage saddr;
    socklen_t saddrlen;
    bool have_synced = false;
    bool no_data = true;
    /* the 44.1 kHZ rtp_time epoch is about 27 hours */
    bool have_rtp_time = false;
    int64_t rtp_time;  /* will only change by small amounts to track rtp epoch changes  */
    uint64_t epoch_length =  0x01;
    epoch_length = epoch_length << 32;
    int64_t half_epoch = (int64_t) epoch_length/2;
    uint32_t prev_rtp_timestamp = 0;
    assert(raop_rtp);
    int64_t ntp_start_time = (int64_t) raop_ntp_get_local_time(raop_rtp->ntp);
    logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp start_time = %8.6f (raop_rtp audio)", ((double) ntp_start_time) / SEC);
    while(1) {
        fd_set rfds;
        struct timeval tv;
        int nfds, ret;

        /* Check if we are still running and process callbacks */
        if (raop_rtp_process_events(raop_rtp, NULL)) {
            break;
        }

        /* Set timeout value to 5ms */
        tv.tv_sec = 0;
        tv.tv_usec = 5000;

        /* Get the correct nfds value */
        nfds = raop_rtp->csock+1;
        if (raop_rtp->dsock >= nfds)
            nfds = raop_rtp->dsock+1;

        /* Set rfds and call select */
        FD_ZERO(&rfds);
        FD_SET(raop_rtp->csock, &rfds);
        FD_SET(raop_rtp->dsock, &rfds);

        ret = select(nfds, &rfds, NULL, NULL, &tv);
        if (ret == 0) {
            /* Timeout happened */
            continue;
        } else if (ret == -1) {
            logger_log(raop_rtp->logger, LOGGER_ERR, "raop_rtp error in select");
            break;
        }

        if (FD_ISSET(raop_rtp->csock, &rfds)) {
            saddrlen = sizeof(saddr);
            packetlen = recvfrom(raop_rtp->csock, (char *)packet, sizeof(packet), 0,
                                 (struct sockaddr *)&saddr, &saddrlen);

            memcpy(&raop_rtp->control_saddr, &saddr, saddrlen);
            raop_rtp->control_saddr_len = saddrlen;
            int type_c = packetlen > 1 ? packet[1] & ~0x80 : -1;
            logger_log(raop_rtp->logger, LOGGER_DEBUG, "\nraop_rtp type_c 0x%02x, packetlen = %d", type_c, packetlen);
            if (type_c == 0x56) {
                /* Handle resent data packet */
                const int offset = 4;
                uint32_t rtp_timestamp = byteutils_get_int_be(packet + offset, 4);
                logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp audio resent: rtp=%u", rtp_timestamp);
                int result = raop_buffer_enqueue(raop_rtp->buffer, packet + offset, packetlen - offset, rtp_timestamp, 1);
                if (result < 0) {
                    logger_log(raop_rtp->logger, LOGGER_INFO, "raop_rtp dropping resend of packetlen = %d", packetlen);
                }
            } else if (type_c == 0x54 && packetlen >= 20) {
                /* packet[0] = 0x90 (first sync ?) or 0x80 (subsequent ones)      *
                 * packet[1:3] = 0xd4,  0xd4 && ~0x80 = type 54                   *
                 * packet[2:3] = 0x00 0x04                                        *
                 * packet[4:7] : sync_rtp big-endian uint32_t)                    *
                 * packet[8:15]: remote ntp timestamp (uint64 big-endian)         *
                 * packet[16:20]: next_rtp big-endian uint32_t)                   *
                 * next_rtp = sync_rtp + 7497 =  441 *  17 (0.17 sec) for AAC-ELD *
                 * next_rtp = sync_rtp + 77175  = 441 * 175 (1.75 sec) for ALAC   */

                // The unit for the rtp clock is 1 / sample rate = 1 / 44100
                uint32_t sync_rtp = byteutils_get_int_be(packet, 4);
                if (have_synced == false) {
                    logger_log(raop_rtp->logger, LOGGER_DEBUG, "first audio rtp sync");
                    have_synced = true;
                }
                uint64_t sync_ntp_raw = byteutils_get_long_be(packet, 8);
                uint64_t sync_ntp_remote = raop_ntp_timestamp_to_micro_seconds(sync_ntp_raw, true);
                uint64_t sync_ntp_local = raop_ntp_convert_remote_time(raop_rtp->ntp, sync_ntp_remote);
                int64_t shift;
                switch (raop_rtp->ct) {
                case 0x08:                   /*AAC-ELD */
                    shift = -11025;           /* 44100/4 */
                    break;
                case 0x02:
                default:
                    shift = 0;   /* not needed for ALAC (audio only) */
                    break;
                }
                raop_rtp_sync_clock(raop_rtp, sync_ntp_local, ntp_start_time, sync_rtp, shift);
                char *str = utils_data_to_string(packet, packetlen, 20);
                logger_log(raop_rtp->logger, LOGGER_DEBUG,
                           "raop_rtp sync: client ntp=%8.6f, ntp = %8.6f, ntp_start_time %8.6f, sync_rtp=%u\n%s",
                           ((double) sync_ntp_remote) / SEC, ((double)sync_ntp_local) / SEC, ((double)ntp_start_time) / SEC, sync_rtp, str);
                free(str);
            } else {
                logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp unknown packet");
            }
        }

        /* rtp un-resent audio data packets:                   *
         * packet[0] 0x80    (both AAC-ELD and ALAC)           *
         * packet[1] 0x60 = 96                                 *
         * packet[2:3] seqnum (big-endian unsigned short)      *
         * packet[4:7] rtp timestamp (big-endian unsigned int) *
         * packet[8:11] 0x00 0x00 0x00 0x00                    *
         * packet[12:packetlen - 1] encrypted audio payload    */

        /* consecutive AAC-ELD rtp timestamps differ by spf = 480 *
         * consecutive ALAC rtp timestamps differ by spf = 352    *
         * both have PCM uncompressed sampling rate = 441000 Hz   */

        /* clock time in microseconds advances at (rtp_timestamp * 1000000)/44100 between frames */
	
        /* every AAC-ELD packet is sent three times:  0  0 1  0 1 2  1 2 3  2 3 4 ... *
         * (after decoding AAC-ELD into PCM, the sound frame is three times bigger)   *
         * ALAC packets are sent once only  0 1 2 3 4 5  ...                          */

        if (FD_ISSET(raop_rtp->dsock, &rfds)) {
            //logger_log(raop_rtp->logger, LOGGER_INFO, "Would have data packet in queue");
            // Receiving audio data here
            saddrlen = sizeof(saddr);
            packetlen = recvfrom(raop_rtp->dsock, (char *)packet, sizeof(packet), 0,
                                 (struct sockaddr *)&saddr, &saddrlen);
            // rtp payload type
            //int type_d = packet[1] & ~0x80;
            //logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp_thread_udp type_d 0x%02x, packetlen = %d", type_d, packetlen);

            // Len = 16 appears if there is no time
            if ( packetlen >= 12) {
                int no_resend = (raop_rtp->control_rport == 0); /* true when control_rport is not set */
                uint32_t rtp_timestamp =  byteutils_get_int_be(packet, 4);
                if (packetlen == 16 && packet[12] == 0x00 && packet[13] == 0x68 && packet[14] == 0x34 && packet[15] == 0x00) {
                    /* skip packet */
                } else {
                    if (no_data) {
                        no_data = false;
                        if (have_synced == false) {
                            /* until the first rtp sync occurs, we don't know the exact client ntp timestamp that matches the client rtp timesamp */
                            int64_t ntp_now = (int64_t) raop_ntp_get_local_time(raop_rtp->ntp);
                            int64_t ntp_time = ((int64_t) ntp_now) - ntp_start_time;
                            int64_t latency = 0;
                            switch (raop_rtp->ct) {
                            case 0x02:  
                                latency = -DELAY_ALAC;   /* DELAY = 2000000 (2.0 sec) is empirical choice for ALAC */
                                break;
                            case 0x08:
                                latency = -DELAY_AAC;   /* DELAY = 500000 (0.5 sec) is empirical choice for AAC-ELD */
                                break;
                            default:
                                break;
                            }
                            ntp_time -= latency;
                            logger_log(raop_rtp->logger, LOGGER_DEBUG, "First audio packet received, have_synced = false, using assumed latency %8.6f",
                                      ((double)latency) / SEC);                    
                            raop_rtp->rtp_sync_offset = (int64_t) ((double) rtp_timestamp) / raop_rtp->rtp_sync_scale; 
                            raop_rtp->rtp_sync_offset -= ntp_time;
                            int64_t ntp_timestamp =  ntp_start_time - raop_rtp->rtp_sync_offset;
                            ntp_timestamp += (int64_t) ((double) rtp_timestamp) / raop_rtp->rtp_sync_scale;
                            latency =  ntp_now - ntp_timestamp;
                            unsigned short seqnum = byteutils_get_short_be(packet, 2);
                            logger_log(raop_rtp->logger, LOGGER_DEBUG,
                                       "initial  audio: now = %8.6f, npt = %8.6f, latency = %8.6f, rtp_time=%u seqnum = %u (not from sync)",
                                       ((double) ntp_now ) / SEC, ((double) ntp_timestamp) / SEC, ((double) latency) / SEC, rtp_timestamp, seqnum);
                        }  else {
                            logger_log(raop_rtp->logger, LOGGER_DEBUG, "First audio packet received, have_synced = true");
                        }
                    }
                    int result = raop_buffer_enqueue(raop_rtp->buffer, packet, packetlen, rtp_timestamp, 1);
                    assert(result >= 0);
                }
                // Render continuous buffer entries
                void *payload = NULL;
                unsigned int payload_size;
                uint32_t timestamp;
                unsigned short seqnum;
                while ((payload = raop_buffer_dequeue(raop_rtp->buffer, &payload_size, &timestamp, &seqnum, no_resend))) {
                    if (have_rtp_time == false) {
                        rtp_time = (int64_t) timestamp;
                        have_rtp_time = true;
                        prev_rtp_timestamp = timestamp;
                     }
                    int64_t rtp_time_increment = (int64_t) (timestamp - prev_rtp_timestamp);
                    prev_rtp_timestamp = timestamp;	 
                    if (rtp_time_increment > half_epoch) {
                        rtp_time_increment -= (int64_t) epoch_length ;
                     } else if (rtp_time_increment < -half_epoch) {
                        rtp_time_increment += (int64_t) epoch_length;
                     } 
                    rtp_time += rtp_time_increment;   /* this is rtp_epoch * epoch_length  + timestamp */
                    int64_t ntp_timestamp =  ntp_start_time  - raop_rtp->rtp_sync_offset;
                    ntp_timestamp += (int64_t) ((double) rtp_time) / raop_rtp->rtp_sync_scale;
                    audio_decode_struct audio_data;
                    audio_data.data_len = payload_size;
                    audio_data.data = payload;
                    audio_data.ntp_time = (uint64_t) ntp_timestamp;
                    audio_data.rtp_time = (uint64_t) rtp_time;
                    audio_data.have_synced = have_synced;
                    raop_rtp->callbacks.audio_process(raop_rtp->callbacks.cls, raop_rtp->ntp, &audio_data);
                    free(payload);
                    uint64_t ntp_now = raop_ntp_get_local_time(raop_rtp->ntp);
                    int64_t latency = ((int64_t) ntp_now) - ntp_timestamp;
                    logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp audio: now = %8.6f, npt = %8.6f, latency = %8.6f, rtp_time=%lld seqnum = %u",
                               ((double) ntp_now ) / SEC, ((double) ntp_timestamp) / SEC, ((double) latency) / SEC, rtp_time, seqnum);
                }

                /* Handle possible resend requests */
                if (!no_resend) {
                    raop_buffer_handle_resends(raop_rtp->buffer, raop_rtp_resend_callback, raop_rtp);
                }
            }

        }
    }

    // Ensure running reflects the actual state
    MUTEX_LOCK(raop_rtp->run_mutex);
    raop_rtp->running = false;
    MUTEX_UNLOCK(raop_rtp->run_mutex);

    logger_log(raop_rtp->logger, LOGGER_DEBUG, "raop_rtp exiting thread");

    return 0;
}

// Start rtp service, three udp ports
void
raop_rtp_start_audio(raop_rtp_t *raop_rtp, int use_udp, unsigned short control_rport,
                     unsigned short *control_lport, unsigned short *data_lport, unsigned char ct)
{
    logger_log(raop_rtp->logger, LOGGER_INFO, "raop_rtp starting audio");
    int use_ipv6 = 0;

    assert(raop_rtp);
    assert(control_lport);
    assert(data_lport);

    MUTEX_LOCK(raop_rtp->run_mutex);
    if (raop_rtp->running || !raop_rtp->joined) {
        MUTEX_UNLOCK(raop_rtp->run_mutex);
        return;
    }

    raop_rtp->ct = ct;

    /* Initialize ports and sockets */
    raop_rtp->control_lport = *control_lport;
    raop_rtp->data_lport = *data_lport;
    raop_rtp->control_rport = control_rport;
    if (raop_rtp->remote_saddr.ss_family == AF_INET6) {
        use_ipv6 = 1;
    }
    use_ipv6 = 0;
    if (raop_rtp_init_sockets(raop_rtp, use_ipv6, use_udp) < 0) {
        logger_log(raop_rtp->logger, LOGGER_ERR, "raop_rtp initializing sockets failed");
        MUTEX_UNLOCK(raop_rtp->run_mutex);
        return;
    }
    *control_lport = raop_rtp->control_lport;
    *data_lport = raop_rtp->data_lport;
    /* Create the thread and initialize running values */
    raop_rtp->running = 1;
    raop_rtp->joined = 0;

    THREAD_CREATE(raop_rtp->thread, raop_rtp_thread_udp, raop_rtp);
    MUTEX_UNLOCK(raop_rtp->run_mutex);
}

void
raop_rtp_set_volume(raop_rtp_t *raop_rtp, float volume)
{
    assert(raop_rtp);

    if (volume > 0.0f) {
        volume = 0.0f;
    } else if (volume < -144.0f) {
        volume = -144.0f;
    }

    /* Set volume in thread instead */
    MUTEX_LOCK(raop_rtp->run_mutex);
    raop_rtp->volume = volume;
    raop_rtp->volume_changed = 1;
    MUTEX_UNLOCK(raop_rtp->run_mutex);
}

void
raop_rtp_set_metadata(raop_rtp_t *raop_rtp, const char *data, int datalen)
{
    unsigned char *metadata;

    assert(raop_rtp);

    if (datalen <= 0) {
        return;
    }
    metadata = malloc(datalen);
    assert(metadata);
    memcpy(metadata, data, datalen);

    /* Set metadata in thread instead */
    MUTEX_LOCK(raop_rtp->run_mutex);
    raop_rtp->metadata = metadata;
    raop_rtp->metadata_len = datalen;
    MUTEX_UNLOCK(raop_rtp->run_mutex);
}

void
raop_rtp_set_coverart(raop_rtp_t *raop_rtp, const char *data, int datalen)
{
    unsigned char *coverart;

    assert(raop_rtp);

    if (datalen <= 0) {
        return;
    }
    coverart = malloc(datalen);
    assert(coverart);
    memcpy(coverart, data, datalen);

    /* Set coverart in thread instead */
    MUTEX_LOCK(raop_rtp->run_mutex);
    raop_rtp->coverart = coverart;
    raop_rtp->coverart_len = datalen;
    MUTEX_UNLOCK(raop_rtp->run_mutex);
}

void
raop_rtp_remote_control_id(raop_rtp_t *raop_rtp, const char *dacp_id, const char *active_remote_header)
{
    assert(raop_rtp);

    if (!dacp_id || !active_remote_header) {
        return;
    }

    /* Set dacp stuff in thread instead */
    MUTEX_LOCK(raop_rtp->run_mutex);
    if (raop_rtp->dacp_id) {
      free(raop_rtp->dacp_id);
    }
    raop_rtp->dacp_id = strdup(dacp_id);
    if (raop_rtp->active_remote_header) {
      free(raop_rtp->active_remote_header);
    }
    raop_rtp->active_remote_header = strdup(active_remote_header);
    MUTEX_UNLOCK(raop_rtp->run_mutex);
}

void
raop_rtp_set_progress(raop_rtp_t *raop_rtp, unsigned int start, unsigned int curr, unsigned int end)
{
    assert(raop_rtp);

    /* Set progress in thread instead */
    MUTEX_LOCK(raop_rtp->run_mutex);
    raop_rtp->progress_start = start;
    raop_rtp->progress_curr = curr;
    raop_rtp->progress_end = end;
    raop_rtp->progress_changed = 1;
    MUTEX_UNLOCK(raop_rtp->run_mutex);
}

void
raop_rtp_flush(raop_rtp_t *raop_rtp, int next_seq)
{
    assert(raop_rtp);

    /* Call flush in thread instead */
    MUTEX_LOCK(raop_rtp->run_mutex);
    raop_rtp->flush = next_seq;
    MUTEX_UNLOCK(raop_rtp->run_mutex);
}

void
raop_rtp_stop(raop_rtp_t *raop_rtp)
{
    assert(raop_rtp);

    /* Check that we are running and thread is not
     * joined (should never be while still running) */
    MUTEX_LOCK(raop_rtp->run_mutex);
    if (!raop_rtp->running || raop_rtp->joined) {
        MUTEX_UNLOCK(raop_rtp->run_mutex);
        return;
    }
    raop_rtp->running = 0;
    MUTEX_UNLOCK(raop_rtp->run_mutex);

    /* Join the thread */
    THREAD_JOIN(raop_rtp->thread);

    if (raop_rtp->csock != -1) closesocket(raop_rtp->csock);
    if (raop_rtp->dsock != -1) closesocket(raop_rtp->dsock);

    /* Flush buffer into initial state */
    raop_buffer_flush(raop_rtp->buffer, -1);

    /* Mark thread as joined */
    MUTEX_LOCK(raop_rtp->run_mutex);
    raop_rtp->joined = 1;
    MUTEX_UNLOCK(raop_rtp->run_mutex);
}

int
raop_rtp_is_running(raop_rtp_t *raop_rtp)
{
    assert(raop_rtp);
    MUTEX_LOCK(raop_rtp->run_mutex);
    int running = raop_rtp->running;
    MUTEX_UNLOCK(raop_rtp->run_mutex);
    return running;
}
