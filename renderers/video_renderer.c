/**
 * RPiPlay - An open-source AirPlay mirroring server for Raspberry Pi
 * Copyright (C) 2019 Florian Draschbacher
 * Modified for:
 * UxPlay - An open-source AirPlay mirroring server
 * Copyright (C) 2021-23 F. Duncanh
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

#include "video_renderer.h"
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#define SECOND_IN_NSECS 1000000000UL
#ifdef X_DISPLAY_FIX
#include <gst/video/navigation.h>
#include "x_display_fix.h"
static bool fullscreen = false;
static bool alt_keypress = false;
static unsigned char X11_search_attempts;
#endif

static video_renderer_t *renderer = NULL;
static GstClockTime gst_video_pipeline_base_time = GST_CLOCK_TIME_NONE;
static logger_t *logger = NULL;
static bool logger_debug;
static unsigned short width, height, width_source, height_source;  /* not currently used */
static bool first_packet = false;
static bool sync = false;
static bool auto_videosink;
static bool hls_video = false;
#ifdef X_DISPLAY_FIX
static bool use_x11 = false;
#endif
static bool playbin3;


struct video_renderer_s {
    GstElement *appsrc, *pipeline;
    GstBus *bus;
  // gboolean playing, terminate, seek_enabled, seek_done;
    gboolean terminate;
    gint64 duration;
    gint buffering_level;
#ifdef  X_DISPLAY_FIX
    const char * server_name;
    X11_Window_t * gst_window;
#endif
};

static void append_videoflip (GString *launch, const videoflip_t *flip, const videoflip_t *rot) {
    /* videoflip image transform */
    switch (*flip) {
    case INVERT:
        switch (*rot)  {
        case LEFT:
	    g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_90R ! ");
	    break;
        case RIGHT:
            g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_90L ! ");
            break;
        default:
	    g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_180 ! ");
	    break;
        }
        break;
    case HFLIP:
        switch (*rot) {
        case LEFT:
            g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_UL_LR ! ");
            break;
        case RIGHT:
            g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_UR_LL ! ");
            break;
        default:
            g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_HORIZ ! ");
            break;
        }
        break;
    case VFLIP:
        switch (*rot) {
        case LEFT:
            g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_UR_LL ! ");
            break;
        case RIGHT:
            g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_UL_LR ! ");
            break;
        default:
            g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_VERT ! ");
	  break;
	}
        break;
    default:
        switch (*rot) {
        case LEFT:
            g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_90L ! ");
            break;
        case RIGHT:
            g_string_append(launch, "videoflip video-direction=GST_VIDEO_ORIENTATION_90R ! ");
            break;
        default:
            break;
        }
        break;
    }
}

/* apple uses colorimetry=1:3:5:1                                *
 * (not recognized by v4l2 plugin in Gstreamer  < 1.20.4)        *
 * See .../gst-libs/gst/video/video-color.h in gst-plugins-base  *
 * range = 1   -> GST_VIDEO_COLOR_RANGE_0_255      ("full RGB")  * 
 * matrix = 3  -> GST_VIDEO_COLOR_MATRIX_BT709                   *
 * transfer = 5 -> GST_VIDEO_TRANSFER_BT709                      *
 * primaries = 1 -> GST_VIDEO_COLOR_PRIMARIES_BT709              *
 * closest used by  GStreamer < 1.20.4 is BT709, 2:3:5:1 with    *                            *
 * range = 2 -> GST_VIDEO_COLOR_RANGE_16_235 ("limited RGB")     */  

static const char h264_caps[]="video/x-h264,stream-format=(string)byte-stream,alignment=(string)au";

void video_renderer_size(float *f_width_source, float *f_height_source, float *f_width, float *f_height) {
    width_source = (unsigned short) *f_width_source;
    height_source = (unsigned short) *f_height_source;
    width = (unsigned short) *f_width;
    height = (unsigned short) *f_height;
    logger_log(logger, LOGGER_DEBUG, "begin video stream wxh = %dx%d; source %dx%d", width, height, width_source, height_source);
}

GstElement *make_video_sink(const char *videosink) {
  /* used to build a videosink for playbin, using the user-specified string "videosink" */ 
    GstElement *video_sink = NULL;
    size_t len = strlen(videosink);
    char *videosink_cpy = (char *) malloc(len + 1);
    strncpy(videosink_cpy, videosink, len + 1);

    /* remove any extension begining with "!" */
    char *end = strstr(videosink_cpy, "!");
    if (end) {   
      *end = '\0';
    }

    /* split videosink string into the sink name and any options */
    char *token;
    char *saveptr;

    /* OK in Windows with MSYS2  (POSIX); use strtok_s for MSVC */
    token = strtok_r(videosink_cpy, " ", &saveptr);
    video_sink = gst_element_factory_make(token, "videosink");
    g_print("playbin_videosink: \"%s\"\n", token);
    if (!video_sink) {
        return NULL;
    }
    
    /* add any fullscreen options "property=pval" included in string videosink*/
    while (1) {
        token = strtok_r(NULL, " ", &saveptr);
        if (!token) {
            break;
        }
        char *pval = strstr(token, "=");
        if (pval) {
            *pval = '\0';
            pval++;
            const gchar *property_name = (const gchar *) token;
            const gchar *value = (const gchar *) pval;
	    g_print("playbin_videosink property: \"%s\" \"%s\"\n", property_name, value);
	    gst_util_set_object_arg(G_OBJECT (video_sink), property_name, value);
        }
    }
    free(videosink_cpy);
    return video_sink;
}

void  video_renderer_init(logger_t *render_logger, const char *server_name, videoflip_t videoflip[2], const char *parser,
                          const char *decoder, const char *converter, const char *videosink, const bool *initial_fullscreen,
                          const bool *video_sync, const char *uri) {
    GError *error = NULL;
    GstCaps *caps = NULL;
#ifdef GST_124
    playbin3 = true;
#else
    playbin3 = true;
#endif
    
    GstClock *clock = gst_system_clock_obtain();
    g_object_set(clock, "clock-type", GST_CLOCK_TYPE_REALTIME, NULL);
    /* videosink choices that are auto */
    auto_videosink = (strstr(videosink, "autovideosink") || strstr(videosink, "fpsdisplaysink"));

    logger = render_logger;
    logger_debug = (logger_get_level(logger) >= LOGGER_DEBUG);

    /* this call to g_set_application_name makes server_name appear in the  X11 display window title bar, */
    /* (instead of the program name uxplay taken from (argv[0]). It is only set one time. */

    const gchar *appname = g_get_application_name();
    if (!appname || strcmp(appname,server_name))  g_set_application_name(server_name);
    appname = NULL;

    renderer = calloc(1, sizeof(video_renderer_t));
    g_assert(renderer);
    renderer->duration = GST_CLOCK_TIME_NONE;
    renderer->buffering_level = 0;
    renderer->terminate = FALSE;

    if (!uri) {
        hls_video  = false;
        GString *launch = g_string_new("appsrc name=video_source ! ");
        g_string_append(launch, "queue ! ");
        g_string_append(launch, parser);
        g_string_append(launch, " ! ");
        g_string_append(launch, decoder);
        g_string_append(launch, " ! ");
        append_videoflip(launch, &videoflip[0], &videoflip[1]);
        g_string_append(launch, converter);
        g_string_append(launch, " ! ");
        g_string_append(launch, "videoscale ! ");
        g_string_append(launch, videosink);
        if (*video_sync) {
            g_string_append(launch, " sync=true");
            sync = true;
        } else {
            g_string_append(launch, " sync=false");
            sync = false;
        }
        logger_log(logger, LOGGER_DEBUG, "GStreamer mirror video pipeline will be:\n\"%s\"", launch->str);
        renderer->pipeline = gst_parse_launch(launch->str, &error);
        if (error) {
            g_error ("get_parse_launch error (video) :\n %s\n",error->message);
            g_clear_error (&error);
        }
        g_assert (renderer->pipeline);
        gst_pipeline_use_clock(GST_PIPELINE_CAST(renderer->pipeline), clock);

        renderer->appsrc = gst_bin_get_by_name (GST_BIN (renderer->pipeline), "video_source");
        g_assert(renderer->appsrc);
        caps = gst_caps_from_string(h264_caps);
        g_object_set(renderer->appsrc, "caps", caps, "stream-type", 0, "is-live", TRUE, "format", GST_FORMAT_TIME, NULL);
        g_string_free(launch, TRUE);
        gst_caps_unref(caps);
        gst_object_unref(clock);
    } else {
        hls_video = true;
        if (playbin3) {
            /* use playbin3 with GStreamer >= 1.24 */
	    logger_log(logger, LOGGER_DEBUG, "********************************************GStreamer HLS video pipeline will use playbin3");
            renderer->pipeline = gst_element_factory_make("playbin3", "hls-playbin3");
	} else {
            renderer->pipeline = gst_element_factory_make("playbin", "hls-playbin");
            logger_log(logger, LOGGER_DEBUG, "********************************************GStreamer HLS video pipeline will use playbin");
        }
        g_assert(renderer->pipeline);
	renderer->appsrc = NULL;
        /* if we are not using autovideosink, build a videossink based on the stricng "videosink" */
        if(strcmp(videosink, "autovideosink")) {
            GstElement *playbin_videosink = make_video_sink(videosink);
            if (!playbin_videosink) {
                logger_log(logger, LOGGER_ERR, "video_renderer_init: failed to create playbin_videosink");
           } else {
                logger_log(logger, LOGGER_INFO, "video_renderer_init: create playbin_videosink at %p", playbin_videosink);
                g_object_set(G_OBJECT (renderer->pipeline), "video-sink", playbin_videosink, NULL);
                gst_object_unref(playbin_videosink);
            }
        }
        g_object_set (G_OBJECT (renderer->pipeline), "uri", uri, NULL);
    }
#ifdef X_DISPLAY_FIX
    use_x11 = (strstr(videosink, "xvimagesink") || strstr(videosink, "ximagesink") || auto_videosink);
    fullscreen = *initial_fullscreen;
    renderer->server_name = server_name;
    renderer->gst_window = NULL;
    bool x_display_fix = false;
    X11_search_attempts = 0; 
    if (use_x11) {
        renderer->gst_window = calloc(1, sizeof(X11_Window_t));
        g_assert(renderer->gst_window);
        get_X11_Display(renderer->gst_window);
        if (!renderer->gst_window->display) {
            free(renderer->gst_window);
            renderer->gst_window = NULL;
        }
    }
#endif
    gst_element_set_state (renderer->pipeline, GST_STATE_READY);
    GstState state;
    if (gst_element_get_state (renderer->pipeline, &state, NULL, 0)) {
        if (state == GST_STATE_READY) {
            logger_log(logger, LOGGER_DEBUG, "Initialized GStreamer video renderer");
        } else {
            logger_log(logger, LOGGER_ERR, "Failed to initialize GStreamer video renderer");
        }
    } else {
        logger_log(logger, LOGGER_ERR, "Failed to initialize GStreamer video renderer");
    }
}

void video_renderer_pause() {
    logger_log(logger, LOGGER_DEBUG, "video renderer paused");
    gst_element_set_state(renderer->pipeline, GST_STATE_PAUSED);
}

void video_renderer_resume() {
    if (video_renderer_is_paused()) {
        logger_log(logger, LOGGER_DEBUG, "video renderer resumed");
        gst_element_set_state (renderer->pipeline, GST_STATE_PLAYING);
        if (!hls_video) {
            gst_video_pipeline_base_time = gst_element_get_base_time(renderer->appsrc);
        }
    }
}

bool video_renderer_is_paused() {
    GstState state;
    gst_element_get_state(renderer->pipeline, &state, NULL, 0);
    return (state == GST_STATE_PAUSED);
}

void video_renderer_start() {
    renderer->bus = gst_element_get_bus(renderer->pipeline);
    gst_element_set_state (renderer->pipeline, GST_STATE_PLAYING);
    if (!hls_video) {
        gst_video_pipeline_base_time = gst_element_get_base_time(renderer->appsrc);
        first_packet = true;
    }
#ifdef X_DISPLAY_FIX
    X11_search_attempts = 0;
#endif
}

#ifdef X_DISPLAY_FIX
void find_x11_window() {
    X11_search_attempts++;
    logger_log(logger, LOGGER_DEBUG, "Looking for X11 UxPlay Window, attempt %d", (int) X11_search_attempts);
    get_x_window(renderer->gst_window, renderer->server_name);
    if (renderer->gst_window->window) {
        logger_log(logger, LOGGER_INFO, "\n*** X11 Windows: Use key F11 or (left Alt)+Enter to toggle full-screen mode\n");
        if (fullscreen) {
            set_fullscreen(renderer->gst_window, &fullscreen);
        }
    }
}
#endif

/* used to find any X11 Window used by the playbin (HLS) pipeline after it starts playing. 
*  if use_x11 is true, called every 100 ms after playbin state is READY until the x11 window is found*/
bool waiting_for_x11_window() {
    if (!hls_video) {
        return false;
    }
#ifdef X_DISPLAY_FIX
    if (use_x11 && renderer->gst_window) {
        find_x11_window();
        if (!renderer->gst_window->window) {
	    return true;    /* window still not found */
        }
    }
#endif
    return false;
}

void video_renderer_render_buffer(unsigned char* data, int *data_len, int *nal_count, uint64_t *ntp_time) {
    GstBuffer *buffer;
    GstClockTime pts = (GstClockTime) *ntp_time; /*now in nsecs */
    //GstClockTimeDiff latency = GST_CLOCK_DIFF(gst_element_get_current_clock_time (renderer->appsrc), pts);
    if (sync) {
        if (pts >= gst_video_pipeline_base_time) {
            pts -= gst_video_pipeline_base_time;
        } else {
            logger_log(logger, LOGGER_ERR, "*** invalid ntp_time < gst_video_pipeline_base_time\n%8.6f ntp_time\n%8.6f base_time",
                       ((double) *ntp_time) / SECOND_IN_NSECS, ((double) gst_video_pipeline_base_time) / SECOND_IN_NSECS);
            return;
        }
    }
    g_assert(data_len != 0);
    /* first four bytes of valid  h264  video data are 0x00, 0x00, 0x00, 0x01.    *
     * nal_count is the number of NAL units in the data: short SPS, PPS, SEI NALs *
     * may  precede a VCL NAL. Each NAL starts with 0x00 0x00 0x00 0x01 and is    *
     * byte-aligned: the first byte of invalid data (decryption failed) is 0x01   */
    if (data[0]) {
        logger_log(logger, LOGGER_ERR, "*** ERROR decryption of video packet failed ");
    } else {
        if (first_packet) {
            logger_log(logger, LOGGER_INFO, "Begin streaming to GStreamer video pipeline");
            first_packet = false;
        }
        buffer = gst_buffer_new_allocate(NULL, *data_len, NULL);
        g_assert(buffer != NULL);
        //g_print("video latency %8.6f\n", (double) latency / SECOND_IN_NSECS);
        if (sync) {
            GST_BUFFER_PTS(buffer) = pts;
        }
        gst_buffer_fill(buffer, 0, data, *data_len);
        gst_app_src_push_buffer (GST_APP_SRC(renderer->appsrc), buffer);
#ifdef X_DISPLAY_FIX
        if (renderer->gst_window && !(renderer->gst_window->window) && use_x11) {
            find_x11_window();
        }
#endif
    }
}

void video_renderer_flush() {
}

void video_renderer_stop() {
    if (renderer) {
        if (!hls_video) {
            gst_app_src_end_of_stream (GST_APP_SRC(renderer->appsrc));
        }
        gst_element_set_state (renderer->pipeline, GST_STATE_NULL);
        //gst_element_set_state (renderer->playbin, GST_STATE_NULL);
     }
}

void video_renderer_destroy() {
    if (renderer) {
        GstState state;
        gst_element_get_state(renderer->pipeline, &state, NULL, 0);
        if (state != GST_STATE_NULL) {
            if (!hls_video) {
                gst_app_src_end_of_stream (GST_APP_SRC(renderer->appsrc));
            }
            gst_element_set_state (renderer->pipeline, GST_STATE_NULL);
        }
        gst_object_unref(renderer->bus);
	if (renderer->appsrc) {
            gst_object_unref (renderer->appsrc);
        }
        gst_object_unref (renderer->pipeline);
#ifdef X_DISPLAY_FIX
        if (renderer->gst_window) {
            free(renderer->gst_window);
            renderer->gst_window = NULL;
        }
#endif
        free (renderer);
        renderer = NULL;
    }
}

gboolean gstreamer_pipeline_bus_callback(GstBus *bus, GstMessage *message, gpointer loop) {
    if (logger_debug) {
        if (!hls_video) {
	    g_print("GStreamer bus message %s %s\n", GST_MESSAGE_SRC_NAME(message), GST_MESSAGE_TYPE_NAME(message));
        } else {
            gint64 pos;
            gst_element_query_position (renderer->pipeline, GST_FORMAT_TIME, &pos);
	    if (GST_CLOCK_TIME_IS_VALID(pos)) {
                g_print("GStreamer bus message %s %s; position: %" GST_TIME_FORMAT "\n", GST_MESSAGE_SRC_NAME(message),
                GST_MESSAGE_TYPE_NAME(message), GST_TIME_ARGS(pos));
	    } else {
                g_print("GStreamer bus message %s %s; position: none\n", GST_MESSAGE_SRC_NAME(message),
                        GST_MESSAGE_TYPE_NAME(message));
           }
	}
    }
    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_DURATION:
        renderer->duration = GST_CLOCK_TIME_NONE;
        break;
    case GST_MESSAGE_BUFFERING:
        if (hls_video) {
            gint percent = -1;
            gst_message_parse_buffering(message, &percent);
	    if (percent >= 0) {
                renderer->buffering_level = percent;
                logger_log(logger, LOGGER_DEBUG, "Buffering :%u percent done", percent);
                if (percent < 100) {
                    gst_element_set_state (renderer->pipeline, GST_STATE_PAUSED);
                } else {
                    gst_element_set_state (renderer->pipeline, GST_STATE_PLAYING);
                }
            }
        }
	break;      
    case GST_MESSAGE_ERROR: {
        GError *err;
        gchar *debug;
        gboolean flushing;
        gst_message_parse_error (message, &err, &debug);
        logger_log(logger, LOGGER_INFO, "GStreamer error: %s %s", GST_MESSAGE_SRC_NAME(message),err->message);
        if (!hls_video && strstr(err->message,"Internal data stream error")) {
            logger_log(logger, LOGGER_INFO,
                     "*** This is a generic GStreamer error that usually means that GStreamer\n"
                     "*** was unable to construct a working video pipeline.\n\n"
                     "*** If you are letting the default autovideosink select the videosink,\n"
                     "*** GStreamer may be trying to use non-functional hardware h264 video decoding.\n"
                     "*** Try using option -avdec to force software decoding or use -vs <videosink>\n"
                     "*** to select a videosink of your choice (see \"man uxplay\").\n\n"
                     "*** Raspberry Pi OS with (unpatched) GStreamer-1.18.4 needs \"-bt709\" uxplay option");
        }
	g_error_free (err);
        g_free (debug);
	if (renderer->appsrc) {
            gst_app_src_end_of_stream (GST_APP_SRC(renderer->appsrc));
	}
        gst_bus_set_flushing(renderer->bus, TRUE);
        gst_element_set_state (renderer->pipeline, GST_STATE_READY);
        renderer->terminate = TRUE;
        g_main_loop_quit( (GMainLoop *) loop);
        break;
    }
    case GST_MESSAGE_EOS:
      /* end-of-stream */
        logger_log(logger, LOGGER_INFO, "GStreamer: End-Of-Stream");
    //case GST_MESSAGE_DURATION:
      	//g_print("bus message (hls/playbin): %s\n", GST_MESSAGE_TYPE_NAME(message));
        /* The duration has changed, mark the current one as invalid */
        //renderer->duration = GST_CLOCK_TIME_NONE;
        //break;
	if (hls_video) {
            gst_bus_set_flushing(renderer->bus, TRUE);
            gst_element_set_state (renderer->pipeline, GST_STATE_READY);
	    renderer->terminate = TRUE;
            g_main_loop_quit( (GMainLoop *) loop);
        }
        break;
    case GST_MESSAGE_STATE_CHANGED:
        if (auto_videosink) {
            char *sink = strstr(GST_MESSAGE_SRC_NAME(message), "-actual-sink-");
            if (sink) {
                sink += strlen("-actual-sink-");
                logger_log(logger, LOGGER_DEBUG, "GStreamer: automatically-selected videosink is \"%ssink\"", sink);
                auto_videosink = false;
#ifdef X_DISPLAY_FIX
                use_x11 = (strstr(sink, "ximage") || strstr(sink, "xvimage"));
#endif
            }
        }
        break;
#ifdef  X_DISPLAY_FIX
    case GST_MESSAGE_ELEMENT:
        if (renderer->gst_window && renderer->gst_window->window) {
            GstNavigationMessageType message_type = gst_navigation_message_get_type (message);
            if (message_type == GST_NAVIGATION_MESSAGE_EVENT) {
                GstEvent *event = NULL;
                if (gst_navigation_message_parse_event (message, &event)) {
                    GstNavigationEventType event_type = gst_navigation_event_get_type (event);
                    const gchar *key;
                    switch (event_type) {
                    case GST_NAVIGATION_EVENT_KEY_PRESS:
                        if (gst_navigation_event_parse_key_event (event, &key)) {
                            if ((strcmp (key, "F11") == 0) || (alt_keypress && strcmp (key, "Return") == 0)) {
                                fullscreen = !(fullscreen);
                                set_fullscreen(renderer->gst_window, &fullscreen);
                            } else if (strcmp (key, "Alt_L") == 0) {
                                alt_keypress = true;
                            }
                        }
                        break;
                    case GST_NAVIGATION_EVENT_KEY_RELEASE:
                        if (gst_navigation_event_parse_key_event (event, &key)) {
                            if (strcmp (key, "Alt_L") == 0) {
                                alt_keypress = false;
                            }
                        }
                    default:
                        break;
                    }
                }
                if (event) {
                    gst_event_unref (event);
                }
            }
        }
        break;
#endif
    default:
      /* unhandled message */
        break;
    }
    return TRUE;
}

unsigned int video_renderer_listen(void *loop) {
    return (unsigned int) gst_bus_add_watch(renderer->bus, (GstBusFunc)
                                            gstreamer_pipeline_bus_callback, (gpointer) loop);    
}

//bool video_check_position() {
// if (renderer && renderer->terminate) {
//return false;
//nt64 pos = 0;
//st_element_query_position (renderer->pipeline, GST_FORMAT_TIME, &pos);
// /* use pos > renderer->duration > 0 as a test to see if video has finished
//     * video_check_position  is called repeatedly  while HLS video is playing */
// if (pos > renderer->duration) {
//    video_renderer_pause();
//    gst_bus_set_flushing(renderer->bus, TRUE);
//    g_print("media streaming is complete (duration %" GST_TIME_FORMAT "): terminating HLS session\n",
//           GST_TIME_ARGS(renderer->duration));
//    return false;
//}
// }
// return true;
//}
  
bool video_get_playback_info(double *duration, double *position, float *rate) {
    gint64 pos = 0;
    GstState state;
    
    *duration = 0.0;
    *position = -1.0;
    gst_element_get_state(renderer->pipeline, &state, NULL, 0);
    *rate = 0.0f;
    switch (state) {
    case GST_STATE_PLAYING:
        *rate = 1.0f;
    default:
        break;
    }

    if (!GST_CLOCK_TIME_IS_VALID(renderer->duration)) {
        if (!gst_element_query_duration (renderer->pipeline, GST_FORMAT_TIME, &renderer->duration)) {
            return true;
        }
    }
    *duration = ((double) renderer->duration) / GST_SECOND;
    if (*duration) {
        if (gst_element_query_position (renderer->pipeline, GST_FORMAT_TIME, &pos) &&
                                        GST_CLOCK_TIME_IS_VALID(pos)) {
            *position = ((double) pos) / GST_SECOND;
        }
    }

    logger_log(logger, LOGGER_DEBUG, "********* video_get_playback_info: position %" GST_TIME_FORMAT " duration %" GST_TIME_FORMAT " %s *********",
               GST_TIME_ARGS (pos), GST_TIME_ARGS (renderer->duration), gst_element_state_get_name(state));

    return true;
}

void video_renderer_seek(float position) {
    double pos = (double) position;
    pos *=  GST_SECOND;
    gint64 seek_position = (gint64) pos;
    seek_position =  seek_position < 1000 ? 1000 : seek_position;
    seek_position =  seek_position > renderer->duration  - 1000 ? renderer->duration - 1000: seek_position;
    g_print("SCRUB: seek to %f secs =  %" GST_TIME_FORMAT ", duration = %" GST_TIME_FORMAT "\n", position,
            GST_TIME_ARGS(seek_position),  GST_TIME_ARGS(renderer->duration));
    gboolean result = gst_element_seek_simple(renderer->pipeline, GST_FORMAT_TIME,
                                              (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                                              seek_position);
    if (result) {
        g_print("seek succeeded\n");
        gst_element_set_state (renderer->pipeline, GST_STATE_PLAYING);	
    } else {
        g_print("seek failed\n");
    }
}
