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
 *
 *=================================================================
 * modified by fduncanh 2021-2023
 */

#ifndef RAOP_RTP_MIRROR_H
#define RAOP_RTP_MIRROR_H

#include <stdint.h>
#include "raop.h"
#include "logger.h"

typedef struct raop_rtp_mirror_s raop_rtp_mirror_t;
typedef struct h264codec_s h264codec_t;

raop_rtp_mirror_t *raop_rtp_mirror_init(logger_t *logger, raop_callbacks_t *callbacks, raop_ntp_t *ntp,
                                        const char *remote, int remotelen, const unsigned char *aeskey);
void raop_rtp_mirror_init_aes(raop_rtp_mirror_t *raop_rtp_mirror, uint64_t *streamConnectionID);
void raop_rtp_mirror_init_chacha(raop_rtp_mirror_t *raop_rtp_mirror, unsigned char *session_key, uint64_t *streamConnectionID);
void raop_rtp_mirror_start(raop_rtp_mirror_t *raop_rtp_mirror, unsigned short *mirror_data_lport, uint8_t show_client_FPS_data);
void raop_rtp_mirror_stop(raop_rtp_mirror_t *raop_rtp_mirror);
void raop_rtp_mirror_destroy(raop_rtp_mirror_t *raop_rtp_mirror);
#endif //RAOP_RTP_MIRROR_H
