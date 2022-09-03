/**
 * RPiPlay - An open-source AirPlay mirroring server for Raspberry Pi
 * Copyright (C) 2019 Florian Draschbacher
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include <math.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include "audio_renderer.h"

/* GStreamer Caps strings for Airplay-defined audio compression types (ct) */

/* ct = 1; linear PCM (uncompressed): 44100/16/2, S16LE */
static const char lpcm_caps[]="audio/x-raw,rate=(int)44100,channels=(int)2,format=S16LE,layout=interleaved";

/* ct = 2; codec_data is ALAC magic cookie:  44100/16/2 spf = 352 */    
static const char alac_caps[] = "audio/x-alac,mpegversion=(int)4,channnels=(int)2,rate=(int)44100,stream-format=raw,codec_data=(buffer)"
                           "00000024""616c6163""00000000""00000160""0010280a""0e0200ff""00000000""00000000""0000ac44";

/* ct = 4; codec_data from MPEG v4 ISO 14996-3 Section 1.6.2.1:  AAC-LC 44100/2 spf = 1024 */
static const char aac_lc_caps[] ="audio/mpeg,mpegversion=(int)4,channnels=(int)2,rate=(int)44100,stream-format=raw,codec_data=(buffer)1210";

/* ct = 8; codec_data from MPEG v4 ISO 14996-3 Section 1.6.2.1: AAC_ELD 44100/2  spf = 480 */
static const char aac_eld_caps[] ="audio/mpeg,mpegversion=(int)4,channnels=(int)2,rate=(int)44100,stream-format=raw,codec_data=(buffer)f8e85000";

typedef struct audio_renderer_s {
    GstElement *appsrc; 
    GstElement *pipeline;
    GstElement *volume;
    unsigned char ct;
} audio_renderer_t ;


static gboolean check_plugins (void)
{
    int i;
    gboolean ret;
    GstRegistry *registry;
    const gchar *needed[] = { "app", "libav", "playback", "autodetect", "videoparsersbad",  NULL};

    registry = gst_registry_get ();
    ret = TRUE;
    for (i = 0; i < g_strv_length ((gchar **) needed); i++) {
        GstPlugin *plugin;
        plugin = gst_registry_find_plugin (registry, needed[i]);
        if (!plugin) {
            g_print ("Required gstreamer plugin '%s' not found\n", needed[i]);
            ret = FALSE;
            continue;
        }
        gst_object_unref (plugin);
        plugin = NULL;
    }
    return ret;
}

void  gstreamer_init(){
    gst_init(NULL,NULL);
    g_assert(check_plugins ());
}

#define NFORMATS 2     /* set to 4 to enable AAC_LD and PCM:  allowed, but  never seen in real-world use */
static audio_renderer_t *renderer_type[NFORMATS];
static audio_renderer_t *renderer = NULL;
static logger_t *logger = NULL;
const char * format[NFORMATS];

void audio_renderer_init(logger_t *render_logger, const char* audiosink) {
    GError *error = NULL;
    GstCaps *caps = NULL;
    logger = render_logger;

    for (int i = 0; i < NFORMATS ; i++) {
        renderer_type[i] = (audio_renderer_t *)  calloc(1,sizeof(audio_renderer_t));
        g_assert(renderer_type[i]);
        GString *launch = g_string_new("appsrc name=audio_source ! ");
        g_string_append(launch, "queue ! ");
        switch (i) {
        case 0:    /* AAC-ELD */
        case 2:    /* AAC-LC */
            g_string_append(launch, "avdec_aac ! ");
            break;
        case 1:    /* ALAC */
            g_string_append(launch, "avdec_alac ! ");
            break;
        case 3:   /*PCM*/
            break;
        default:
            break;
        }
        g_string_append (launch, "audioconvert ! volume name=volume ! level ! ");
#ifdef _WIN32
        g_string_append (launch, "audioresample ! ");    /* wasapisink must resample from 44.1 kHz to 48 kHz */
#endif
        g_string_append (launch, audiosink);
        g_string_append (launch, " sync=false");
        renderer_type[i]->pipeline  = gst_parse_launch(launch->str, &error);
	if (error) {
          g_error ("gst_parse_launch error (audio %d):\n %s\n", i+1, error->message);
          g_clear_error (&error);
        }
        g_string_free(launch, TRUE);
        g_assert (renderer_type[i]->pipeline);
 
        renderer_type[i]->appsrc = gst_bin_get_by_name (GST_BIN (renderer_type[i]->pipeline), "audio_source");
        renderer_type[i]->volume = gst_bin_get_by_name (GST_BIN (renderer_type[i]->pipeline), "volume");
        switch (i) {
        case 0:
            caps =  gst_caps_from_string(aac_eld_caps);
            renderer_type[i]->ct = 8;
            format[i] = "AAC-ELD 44100/2";
            break;
        case 1:
            caps =  gst_caps_from_string(alac_caps);
            renderer_type[i]->ct = 2;
            format[i] = "ALAC 44100/16/2";
            break;
        case 2:
            caps =  gst_caps_from_string(aac_lc_caps);
            renderer_type[i]->ct = 4;
            format[i] = "AAC-LC 44100/2";
            break;
        case 3:
            caps =  gst_caps_from_string(lpcm_caps);
            renderer_type[i]->ct = 1;
            format[i] = "PCM 44100/16/2 S16LE";
            break;
        default:
            break;
        }
        logger_log(logger, LOGGER_DEBUG, "supported audio format %d: %s",i+1,format[i]);
        g_object_set(renderer_type[i]->appsrc, "caps", caps, "stream-type", 0, "is-live", TRUE, "format", GST_FORMAT_TIME, NULL);
        gst_caps_unref(caps);
    }
}

void audio_renderer_stop() {
    if (renderer) {
        gst_app_src_end_of_stream(GST_APP_SRC(renderer->appsrc));
        gst_element_set_state (renderer->pipeline, GST_STATE_NULL);
        renderer = NULL;
    }
}

void  audio_renderer_start(unsigned char *ct) {
    unsigned char compression_type = 0, id;
    for (int i = 0; i < NFORMATS; i++) {
        if(renderer_type[i]->ct == *ct) {
            compression_type = *ct;
	    id = i;
            break;
        }
    }
    if (compression_type && renderer) {
        if(compression_type != renderer->ct) {
            gst_app_src_end_of_stream(GST_APP_SRC(renderer->appsrc));
            gst_element_set_state (renderer->pipeline, GST_STATE_NULL);
            logger_log(logger, LOGGER_INFO, "changed audio connection, format %s", format[id]);
            renderer = renderer_type[id];
            gst_element_set_state (renderer->pipeline, GST_STATE_PLAYING);
        }
    } else if (compression_type) {
        logger_log(logger, LOGGER_INFO, "start audio connection, format %s", format[id]);
        renderer = renderer_type[id];
        gst_element_set_state (renderer->pipeline, GST_STATE_PLAYING);
    } else {
        logger_log(logger, LOGGER_ERR, "unknown audio compression type ct = %d", *ct);
    }
    
}

void audio_renderer_render_buffer(raop_ntp_t *ntp, unsigned char* data, int data_len, uint64_t ntp_time,
                                  uint64_t rtp_time, unsigned short seqnum) {
    GstBuffer *buffer;
    bool valid;
    if (data_len == 0 || renderer == NULL) return;

    /* all audio received seems to be either ct = 8 (AAC_ELD 44100/2 spf 460 ) AirPlay Mirror protocol *
     * or ct = 2 (ALAC 44100/16/2 spf 352) AirPlay protocol.                                           *
     * first byte data[0] of ALAC frame is 0x20,                                                       *
     * first byte of AAC_ELD is 0x8c, 0x8d or 0x8e: 0x100011(00,01,10) in modern devices               *
     *                   but is 0x80, 0x81 or 0x82: 0x100000(00,01,10) in ios9, ios10 devices          *
     * first byte of AAC_LC should be 0xff (ADTS) (but has never been  seen).                          */
    
    buffer = gst_buffer_new_and_alloc(data_len);
    g_assert(buffer != NULL);
    GST_BUFFER_PTS(buffer) = (GstClockTime) ntp_time;
    gst_buffer_fill(buffer, 0, data, data_len);
    switch (renderer->ct){
    case 8: /*AAC-ELD*/
        switch (data[0]){
        case 0x8c:
        case 0x8d:
        case 0x8e:
        case 0x80:
        case 0x81:
        case 0x82:
            valid = true;
            break;          
        default:
            valid = false;
            break;
        }
        break;
    case 2: /*ALAC*/
        valid = (data[0] == 0x20);
        break;
    case 4:  /*AAC_LC */
        valid = (data[0] == 0xff );
 	break;
    default:
        valid = true;
        break;
    }
    if (valid) {
        gst_app_src_push_buffer(GST_APP_SRC(renderer->appsrc), buffer);
    } else {
        logger_log(logger, LOGGER_ERR, "*** ERROR invalid  audio frame (compression_type %d) skipped ", renderer->ct);
        logger_log(logger, LOGGER_ERR, "***       first byte of invalid frame was  0x%2.2x ", (unsigned int) data[0]);
    }
}

void audio_renderer_set_volume(float volume) {
    float avol;
        if (fabs(volume) < 28) {
	    avol=floorf(((28-fabs(volume))/28)*10)/10;
    	    g_object_set(renderer->volume, "volume", avol, NULL);
        }
}

void audio_renderer_flush() {
}

void audio_renderer_destroy() {
    audio_renderer_stop();
    for (int i = 0; i < NFORMATS ; i++ ) {
        gst_object_unref (renderer_type[i]->volume);
	renderer_type[i]->volume = NULL;
        gst_object_unref (renderer_type[i]->appsrc);
        renderer_type[i]->appsrc = NULL;
	gst_object_unref (renderer_type[i]->pipeline);
        renderer_type[i]->pipeline = NULL;
        free(renderer_type[i]);
    }
}

