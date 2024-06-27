/*
 * Copyright (c) 2024 fduncanh, All Rights Reserved.
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
  */

#ifndef AIRPLAY_VIDEO_H
#define AIRPLAY_VIDEO_H


#include <stdint.h>
#include <stdbool.h>
#include "raop.h"
#include "logger.h"

typedef struct airplay_video_s airplay_video_t;

int set_playback_info_item(airplay_video_t *airplay_video, const char *item, int num, float *val);

const char *get_apple_session_id(airplay_video_t *airplay_video);
void set_start_position_seconds(airplay_video_t *airplay_video, float start_position_seconds);
float get_start_position_seconds(airplay_video_t *airplay_video);
void set_playback_uuid(airplay_video_t *airplay_video, const char *playback_uuid);
  
void airplay_video_service_destroy(airplay_video_t *airplay_video);

//  C wrappers for c++ class MediaDataStore
//create the media_data_store, return a pointer to it.
void* media_data_store_create(void *conn_opaque, uint16_t port);

//delete the media_data_store
void media_data_store_destroy(void *media_data_store);

// called by the POST /action handler:
char *process_media_data(void *media_data_store, const char *url, const char *data, int datalen);

//called by the POST /play handler
bool request_media_data(void *media_data_store, const char *primary_url, const char * session_id);

//called by airplay_video_media_http_connection::get_handler:   &path = req.uri)
char *query_media_data(void *media_data_store, const char *url, int *len);

//called by the post_stop_handler:
void media_data_store_reset(void *media_data_store);

const char *adjust_primary_uri(void *media_data_store, const char *url);
  
#endif //AIRPLAY_VIDEO_H
