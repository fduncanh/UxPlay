/**
 * RPiPlay - An open-source AirPlay mirroring server for Raspberry Pi
 * Copyright (C) 2019 Florian Draschbacher
 * Modified extensively to become 
 * UxPlay - An open-souce AirPlay mirroring server.
 * Modifications Copyright (C) 2021-23 F. Duncanh
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

#include <stddef.h>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iterator>
#include <sys/stat.h>
#include <cstdio>
#include <stdarg.h>

#ifdef _WIN32  /*modifications for Windows compilation */
#include <glib.h>
#include <unordered_map>
#include <winsock2.h>
#include <iphlpapi.h>
#else
#include <glib-unix.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <pwd.h>
# ifdef __linux__
# include <netpacket/packet.h>
# else
# include <net/if_dl.h>
# endif
#endif

#include "lib/raop.h"
#include "lib/stream.h"
#include "lib/logger.h"
#include "lib/dnssd.h"
#include "renderers/video_renderer.h"
#include "renderers/audio_renderer.h"

#define VERSION "1.67"

#define SECOND_IN_USECS 1000000
#define SECOND_IN_NSECS 1000000000UL
#define DEFAULT_NAME "UxPlay"
#define DEFAULT_DEBUG_LOG false
#define LOWEST_ALLOWED_PORT 1024
#define HIGHEST_PORT 65535
#define NTP_TIMEOUT_LIMIT 5
#define BT709_FIX "capssetter caps=\"video/x-h264, colorimetry=bt709\""

static std::string server_name = DEFAULT_NAME;
static dnssd_t *dnssd = NULL;
static raop_t *raop = NULL;
static logger_t *render_logger = NULL;
static bool audio_sync = false;
static bool video_sync = true;
static int64_t audio_delay_alac = 0;
static int64_t audio_delay_aac = 0;
static bool relaunch_video = false;
static bool reset_loop = false;
static unsigned int open_connections= 0;
static std::string videosink = "autovideosink";
static videoflip_t videoflip[2] = { NONE , NONE };
static bool use_video = true;
static unsigned char compression_type = 0;
static std::string audiosink = "autoaudiosink";
static int  audiodelay = -1;
static bool use_audio = true;
static bool new_window_closing_behavior = true;
static bool close_window;
static std::string video_parser = "h264parse";
static std::string video_decoder = "decodebin";
static std::string video_converter = "videoconvert";
static bool show_client_FPS_data = false;
static unsigned int max_ntp_timeouts = NTP_TIMEOUT_LIMIT;
static FILE *video_dumpfile = NULL;
static std::string video_dumpfile_name = "videodump";
static int video_dump_limit = 0;
static int video_dumpfile_count = 0;
static int video_dump_count = 0;
static bool dump_video = false;
static unsigned char mark[] = { 0x00, 0x00, 0x00, 0x01 };
static FILE *audio_dumpfile = NULL;
static std::string audio_dumpfile_name = "audiodump";
static int audio_dump_limit = 0;
static int audio_dumpfile_count = 0;
static int audio_dump_count = 0;
static bool dump_audio = false;
static unsigned char audio_type = 0x00;
static unsigned char previous_audio_type = 0x00;
static bool fullscreen = false;
static std::string coverart_filename = "";
static bool do_append_hostname = true;
static bool use_random_hw_addr = false;
static unsigned short display[5] = {0}, tcp[3] = {0}, udp[3] = {0};
static bool debug_log = DEFAULT_DEBUG_LOG;
static int log_level = LOGGER_INFO;
static bool bt709_fix = false;
static int max_connections = 2;
static unsigned short raop_port;
static unsigned short airplay_port;
static uint64_t remote_clock_offset = 0;
static std::vector<std::string> allowed_clients;
static std::vector<std::string> blocked_clients;
static bool restrict_clients;
static bool setup_legacy_pairing = false;

/* logging */

void log(int level, const char* format, ...) {
    va_list vargs;
    if (level > log_level) return;
    switch (level) {
    case 0:
    case 1:
    case 2:
    case 3:
        printf("*** ERROR: ");
        break;
    case 4:
        printf("*** WARNING: ");
        break;
    default:
        break;
    }
    va_start(vargs, format);
    vprintf(format, vargs);
    printf("\n");
    va_end(vargs);
}

#define LOGD(...) log(LOGGER_DEBUG, __VA_ARGS__)
#define LOGI(...) log(LOGGER_INFO, __VA_ARGS__)
#define LOGW(...) log(LOGGER_WARNING, __VA_ARGS__)
#define LOGE(...) log(LOGGER_ERR, __VA_ARGS__)

/* 95 byte png file with a 1x1 white square (single pixel): placeholder for coverart*/
static const unsigned char empty_image[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,  0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,  0x01, 0x03, 0x00, 0x00, 0x00, 0x25, 0xdb, 0x56,
    0xca, 0x00, 0x00, 0x00, 0x03, 0x50, 0x4c, 0x54,  0x45, 0x00, 0x00, 0x00, 0xa7, 0x7a, 0x3d, 0xda,
    0x00, 0x00, 0x00, 0x01, 0x74, 0x52, 0x4e, 0x53,  0x00, 0x40, 0xe6, 0xd8, 0x66, 0x00, 0x00, 0x00,
    0x0a, 0x49, 0x44, 0x41, 0x54, 0x08, 0xd7, 0x63,  0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe2,
    0x21, 0xbc, 0x33, 0x00, 0x00, 0x00, 0x00, 0x49,  0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

size_t write_coverart(const char *filename, const void *image, size_t len) {
    FILE *fp = fopen(filename, "wb");
    size_t count = fwrite(image, 1, len, fp);
    fclose(fp);
    return count;
}
  
static void dump_audio_to_file(unsigned char *data, int datalen, unsigned char type) {
    if (!audio_dumpfile && audio_type != previous_audio_type) {
        char suffix[20];
        std::string fn = audio_dumpfile_name;
        previous_audio_type = audio_type;
        audio_dumpfile_count++;
        audio_dump_count = 0;
        /* type 0x20 is lossless ALAC, type 0x80 is compressed AAC-ELD, type 0x10 is "other" */
        if (audio_type == 0x20) {
            snprintf(suffix, sizeof(suffix), ".%d.alac", audio_dumpfile_count);
        } else if (audio_type == 0x80) {
            snprintf(suffix, sizeof(suffix), ".%d.aac", audio_dumpfile_count);
        } else {
            snprintf(suffix, sizeof(suffix), ".%d.aud", audio_dumpfile_count);
        }
        fn.append(suffix);
        audio_dumpfile = fopen(fn.c_str(),"w");
        if (audio_dumpfile == NULL) {
            LOGE("could not open file %s for dumping audio frames",fn.c_str());
        }
    }

    if (audio_dumpfile) {
        fwrite(data, 1, datalen, audio_dumpfile);
        if (audio_dump_limit) {
            audio_dump_count++;
            if (audio_dump_count == audio_dump_limit) {
                fclose(audio_dumpfile);
                audio_dumpfile = NULL;
            }          
        }
    }
}

static void dump_video_to_file(unsigned char *data, int datalen) {
    /*  SPS NAL has (data[4] & 0x1f) = 0x07  */
    if ((data[4] & 0x1f) == 0x07  && video_dumpfile && video_dump_limit) {
        fwrite(mark, 1, sizeof(mark), video_dumpfile);
        fclose(video_dumpfile);
        video_dumpfile = NULL;
        video_dump_count = 0;                     
    }

    if (!video_dumpfile) {
        std::string fn = video_dumpfile_name;
        if (video_dump_limit) {
            char suffix[20];
            video_dumpfile_count++;
            snprintf(suffix, sizeof(suffix), ".%d", video_dumpfile_count);
            fn.append(suffix);
	}
        fn.append(".h264");
        video_dumpfile = fopen (fn.c_str(),"w");
        if (video_dumpfile == NULL) {
            LOGE("could not open file %s for dumping h264 frames",fn.c_str());
        }
    }

    if (video_dumpfile) {
        if (video_dump_limit == 0) {
            fwrite(data, 1, datalen, video_dumpfile);
        } else if (video_dump_count < video_dump_limit) {
            video_dump_count++;
            fwrite(data, 1, datalen, video_dumpfile);
        }
    }
}

static gboolean reset_callback(gpointer loop) {
    if (reset_loop) {
        g_main_loop_quit((GMainLoop *) loop);
    }
    return TRUE;
}

static gboolean  sigint_callback(gpointer loop) {
    relaunch_video = false;
    g_main_loop_quit((GMainLoop *) loop);
    return TRUE;
}

static gboolean  sigterm_callback(gpointer loop) {
    relaunch_video = false;
    g_main_loop_quit((GMainLoop *) loop);
    return TRUE;
}

#ifdef _WIN32
struct signal_handler {
    GSourceFunc handler;
    gpointer user_data;
};

static std::unordered_map<gint, signal_handler> u = {};

void SignalHandler(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        u[signum].handler(u[signum].user_data);
    }
}

guint g_unix_signal_add(gint signum, GSourceFunc handler, gpointer user_data) {
    u[signum] = signal_handler{handler, user_data};
    (void) signal(signum, SignalHandler);
    return 0;
}
#endif

static void main_loop()  {
    guint gst_bus_watch_id = 0;
    GMainLoop *loop = g_main_loop_new(NULL,FALSE);
    relaunch_video = false;
    if (use_video) {
        relaunch_video = true;
        gst_bus_watch_id = (guint) video_renderer_listen((void *)loop);
    }
    guint reset_watch_id = g_timeout_add(100, (GSourceFunc) reset_callback, (gpointer) loop);
    guint sigterm_watch_id = g_unix_signal_add(SIGTERM, (GSourceFunc) sigterm_callback, (gpointer) loop);
    guint sigint_watch_id = g_unix_signal_add(SIGINT, (GSourceFunc) sigint_callback, (gpointer) loop);
    g_main_loop_run(loop);

    if (gst_bus_watch_id > 0) g_source_remove(gst_bus_watch_id);
    if (sigint_watch_id > 0) g_source_remove(sigint_watch_id);
    if (sigterm_watch_id > 0) g_source_remove(sigterm_watch_id);
    if (reset_watch_id > 0) g_source_remove(reset_watch_id);
    g_main_loop_unref(loop);
}    

static int parse_hw_addr (std::string str, std::vector<char> &hw_addr) {
    for (int i = 0; i < (int) str.length(); i += 3) {
        hw_addr.push_back((char) stol(str.substr(i), NULL, 16));
    }
    return 0;
}

static std::string find_uxplay_config_file() {
    std::string no_config_file = "";
    const char *homedir = NULL;
    const char *uxplayrc = NULL;
    std::string config0, config1, config2;
    struct stat sb;
    uxplayrc = getenv("UXPLAYRC");   /* first look for $UXPLAYRC */
    if (uxplayrc) {
        config0 = uxplayrc;
        if (stat(config0.c_str(), &sb) == 0) return config0;
    }
    homedir = getenv("XDG_CONFIG_HOMEDIR");
    if (homedir == NULL) {
        homedir = getenv("HOME");
    }
#ifndef _WIN32
    if (homedir == NULL){
        homedir = getpwuid(getuid())->pw_dir;
    }
#endif
    if (homedir) {
      config1 = homedir;
      config1.append("/.uxplayrc");
      if (stat(config1.c_str(), &sb) == 0) return config1;  /* look for ~/.uxplayrc */
      config2 = homedir;
      config2.append("/.config/uxplayrc"); /* look for ~/.config/uxplayrc */
      if (stat(config2.c_str(), &sb) == 0) return config2;
    }
    return no_config_file;
}

static std::string find_mac () {
/*  finds the MAC address of a network interface *
 *  in a Windows, Linux, *BSD or macOS system.   */
    std::string mac = "";
    char str[3];
#ifdef _WIN32
    ULONG buflen = sizeof(IP_ADAPTER_ADDRESSES);
    PIP_ADAPTER_ADDRESSES addresses = (IP_ADAPTER_ADDRESSES*) malloc(buflen);
    if (addresses == NULL) { 					
        return mac;
    }
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &buflen) == ERROR_BUFFER_OVERFLOW) {
        free(addresses);
        addresses = (IP_ADAPTER_ADDRESSES*) malloc(buflen);
        if (addresses == NULL) {
            return mac;
        }
    }
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &buflen) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES address = addresses; address != NULL; address = address->Next) {
            if (address->PhysicalAddressLength != 6                 /* MAC has 6 octets */
                || (address->IfType != 6 && address->IfType != 71)  /* Ethernet or Wireless interface */
                || address->OperStatus != 1) {                      /* interface is up */
                continue;
            }
            mac.erase();
            for (int i = 0; i < 6; i++) {
                snprintf(str, sizeof(str), "%02x", int(address->PhysicalAddress[i]));
                mac = mac + str;
                if (i < 5) mac = mac + ":";
            }
	    break;
        }
    }
    free(addresses);
    return mac;
#else
    struct ifaddrs *ifap, *ifaptr;
    int non_null_octets = 0;
    unsigned char octet[6];
    if (getifaddrs(&ifap) == 0) {
        for(ifaptr = ifap; ifaptr != NULL; ifaptr = ifaptr->ifa_next) {
            if(ifaptr->ifa_addr == NULL) continue;
#ifdef __linux__
            if (ifaptr->ifa_addr->sa_family != AF_PACKET) continue;
            struct sockaddr_ll *s = (struct sockaddr_ll*) ifaptr->ifa_addr;
            for (int i = 0; i < 6; i++) {
                if ((octet[i] = s->sll_addr[i]) != 0) non_null_octets++;
            }
#else    /* macOS and *BSD */
            if (ifaptr->ifa_addr->sa_family != AF_LINK) continue;
            unsigned char *ptr = (unsigned char *) LLADDR((struct sockaddr_dl *) ifaptr->ifa_addr);
            for (int i= 0; i < 6 ; i++) {
                if ((octet[i] = *ptr) != 0) non_null_octets++;
                ptr++;
            }
#endif
            if (non_null_octets) {
                mac.erase();
                for (int i = 0; i < 6 ; i++) {
                    snprintf(str, sizeof(str), "%02x", octet[i]);
                    mac = mac + str;
                    if (i < 5) mac = mac + ":";
                }
                break;
            }
        }
    }
    freeifaddrs(ifap);
#endif
    return mac;
}

#define MULTICAST 0
#define LOCAL 1
#define OCTETS 6
static std::string random_mac () {
    char str[3];
    int octet = rand() % 64;
    octet = (octet << 1) + LOCAL;
    octet = (octet << 1) + MULTICAST;
    snprintf(str,3,"%02x",octet);
    std::string mac_address(str);
    for (int i = 1; i < OCTETS; i++) {
        mac_address = mac_address + ":";
        octet =  rand() % 256;
        snprintf(str,3,"%02x",octet);
        mac_address = mac_address + str;
    }
    return mac_address;
}

static void print_info (char *name) {
    printf("UxPlay %s: An open-source AirPlay mirroring server.\n", VERSION);
    printf("=========== Website: https://github.com/FDH2/UxPlay ==========\n");
    printf("Usage: %s [-n name] [-s wxh] [-p [n]] [(other options)]\n", name);
    printf("Options:\n");
    printf("-n name   Specify the network name of the AirPlay server\n");
    printf("-nh       Do not add \"@hostname\" at the end of AirPlay server name\n");
    printf("-pair     Support Airplay (legacy) client-pairing (default: not used)\n");
    printf("-vsync [x]Mirror mode: sync audio to video using timestamps (default)\n");
    printf("          x is optional audio delay: millisecs, decimal, can be neg.\n");
    printf("-vsync no Switch off audio/(server)video timestamp synchronization \n");
    printf("-async [x]Audio-Only mode: sync audio to client video (default: no)\n");
    printf("-async no Switch off audio/(client)video timestamp synchronization\n");
    printf("-s wxh[@r]Set display resolution [refresh_rate] default 1920x1080[@60]\n");
    printf("-o        Set display \"overscanned\" mode on (not usually needed)\n");
    printf("-fs       Full-screen (only works with X11, Wayland and VAAPI)\n");
    printf("-p        Use legacy ports UDP 6000:6001:7011 TCP 7000:7001:7100\n");
    printf("-p n      Use TCP and UDP ports n,n+1,n+2. range %d-%d\n", LOWEST_ALLOWED_PORT, HIGHEST_PORT);
    printf("          use \"-p n1,n2,n3\" to set each port, \"n1,n2\" for n3 = n2+1\n");
    printf("          \"-p tcp n\" or \"-p udp n\" sets TCP or UDP ports separately\n");
    printf("-avdec    Force software h264 video decoding with libav decoder\n"); 
    printf("-vp ...   Choose the GSteamer h264 parser: default \"h264parse\"\n");
    printf("-vd ...   Choose the GStreamer h264 decoder; default \"decodebin\"\n");
    printf("          choices: (software) avdec_h264; (hardware) v4l2h264dec,\n");
    printf("          nvdec, nvh264dec, vaapih64dec, vtdec,etc.\n");
    printf("          choices: avdec_h264,vaapih264dec,nvdec,nvh264dec,v4l2h264dec\n");
    printf("-vc ...   Choose the GStreamer videoconverter; default \"videoconvert\"\n");
    printf("          another choice when using v4l2h264dec: v4l2convert\n");
    printf("-vs ...   Choose the GStreamer videosink; default \"autovideosink\"\n");
    printf("          some choices: ximagesink,xvimagesink,vaapisink,glimagesink,\n");
    printf("          gtksink,waylandsink,osxvideosink,kmssink,d3d11videosink etc.\n");
    printf("-vs 0     Streamed audio only, with no video display window\n");
    printf("-v4l2     Use Video4Linux2 for GPU hardware h264 decoding\n");
    printf("-bt709    A workaround (bt709 color) sometimes needed on RPi\n"); 
    printf("-rpi      Same as \"-v4l2\" (for RPi=Raspberry Pi).\n");
    printf("-rpigl    Same as \"-rpi -vs glimagesink\" for RPi.\n");
    printf("-rpifb    Same as \"-rpi -vs kmssink\" for RPi using framebuffer.\n");
    printf("-rpiwl    Same as \"-rpi -vs waylandsink\" for RPi using Wayland.\n");
    printf("-as ...   Choose the GStreamer audiosink; default \"autoaudiosink\"\n");
    printf("          some choices:pulsesink,alsasink,pipewiresink,jackaudiosink,\n");
    printf("          osssink,oss4sink,osxaudiosink,wasapisink,directsoundsink.\n");
    printf("-as 0     (or -a)  Turn audio off, streamed video only\n");
    printf("-al x     Audio latency in seconds (default 0.25) reported to client.\n");
    printf("-ca <fn>  In Airplay Audio (ALAC) mode, write cover-art to file <fn>\n");
    printf("-reset n  Reset after 3n seconds client silence (default %d, 0=never)\n", NTP_TIMEOUT_LIMIT);
    printf("-nc       do Not Close video window when client stops mirroring\n");
    printf("-nohold   Drop current connection when new client connects.\n");
    printf("-restrict Restrict clients to those specified by \"-allow <deviceID>\"\n");
    printf("          UxPlay displays deviceID when a client attempts to connect\n");
    printf("          Use \"-restrict no\" for no client restrictions (default)\n");
    printf("-allow <i>Permit deviceID = <i> to connect if restrictions are imposed\n");
    printf("-block <i>Always block connections from deviceID = <i>\n");
    printf("-FPSdata  Show video-streaming performance reports sent by client.\n");
    printf("-fps n    Set maximum allowed streaming framerate, default 30\n");
    printf("-f {H|V|I}Horizontal|Vertical flip, or both=Inversion=rotate 180 deg\n");
    printf("-r {R|L}  Rotate 90 degrees Right (cw) or Left (ccw)\n");
    printf("-m        Use random MAC address (use for concurrent UxPlay's)\n");
    printf("-vdmp [n] Dump h264 video output to \"fn.h264\"; fn=\"videodump\",change\n");
    printf("          with \"-vdmp [n] filename\". If [n] is given, file fn.x.h264\n");
    printf("          x=1,2,.. opens whenever a new SPS/PPS NAL arrives, and <=n\n");
    printf("          NAL units are dumped.\n");
    printf("-admp [n] Dump audio output to \"fn.x.fmt\", fmt ={aac, alac, aud}, x\n");
    printf("          =1,2,..; fn=\"audiodump\"; change with \"-admp [n] filename\".\n");
    printf("          x increases when audio format changes. If n is given, <= n\n");
    printf("          audio packets are dumped. \"aud\"= unknown format.\n");
    printf("-d        Enable debug logging\n");
    printf("-v        Displays version information\n");
    printf("-h        Displays this help\n");
    printf("Startup options in $UXPLAYRC, ~/.uxplayrc, or ~/.config/uxplayrc are\n");
    printf("applied first (command-line options may modify them): format is one \n");
    printf("option per line, no initial \"-\"; lines starting with \"#\" are ignored.\n");
}

bool option_has_value(const int i, const int argc, std::string option, const char *next_arg) {
    if (i >= argc - 1 || next_arg[0] == '-') {
        LOGE("invalid: \"%s\" had no argument", option.c_str());
        return false;
     }
    return true;
}

static bool get_display_settings (std::string value, unsigned short *w, unsigned short *h, unsigned short *r) {
    // assume str  = wxh@r is valid if w and h are positive decimal integers
    // with no more than 4 digits, r < 256 (stored in one byte).
    char *end;
    std::size_t pos = value.find_first_of("x");
    if (pos == std::string::npos) return false;
    std::string str1 = value.substr(pos+1);
    value.erase(pos);
    if (value.length() == 0 || value.length() > 4 || value[0] == '-') return false;
    *w = (unsigned short) strtoul(value.c_str(), &end, 10);
    if (*end || *w == 0)  return false;
    pos = str1.find_first_of("@");
    if(pos != std::string::npos) {
        std::string str2 = str1.substr(pos+1);
        if (str2.length() == 0 || str2.length() > 3 || str2[0] == '-') return false;
        *r = (unsigned short) strtoul(str2.c_str(), &end, 10);
        if (*end || *r == 0 || *r > 255) return false;
        str1.erase(pos);
    }
    if (str1.length() == 0 || str1.length() > 4 || str1[0] == '-') return false;
    *h = (unsigned short) strtoul(str1.c_str(), &end, 10);
    if (*end || *h == 0) return false;
    return true;
}

static bool get_value (const char *str, unsigned int *n) {
    // if n > 0 str must be a positive decimal <= input value *n  
    // if n = 0, str must be a non-negative decimal
    if (strlen(str) == 0 || strlen(str) > 10 || str[0] == '-') return false;
    char *end;
    unsigned long l = strtoul(str, &end, 10);
    if (*end) return false;
    if (*n && (l == 0 || l > *n)) return false;
    *n = (unsigned int) l;
    return true;
}

static bool get_ports (int nports, std::string option, const char * value, unsigned short * const port) {
    /*valid entries are comma-separated values port_1,port_2,...,port_r, 0 < r <= nports */
    /*where ports are distinct, and are in the allowed range.                            */
    /*missing values are consecutive to last given value (at least one value needed).    */
    char *end;
    unsigned long l;
    std::size_t pos;
    std::string val(value), str;
    for (int i = 0; i <= nports ; i++)  {
        if(i == nports) break;
        pos = val.find_first_of(',');
        str = val.substr(0,pos);
        if(str.length() == 0 || str.length() > 5 || str[0] == '-') break;
        l = strtoul(str.c_str(), &end, 10);
        if (*end || l < LOWEST_ALLOWED_PORT || l > HIGHEST_PORT) break;
         *(port + i) = (unsigned short) l;
        for  (int j = 0; j < i ; j++) {
            if( *(port + j) == *(port + i)) break;
        }
        if(pos == std::string::npos) {
            if (nports + *(port + i) > i + 1 + HIGHEST_PORT) break;
            for (int j = i + 1; j < nports; j++) {
                *(port + j) = *(port + j - 1) + 1;
            }
            return true;
        }
        val.erase(0, pos+1);
    }
    LOGE("invalid \"%s %s\", all %d ports must be in range [%d,%d]",
         option.c_str(), value, nports, LOWEST_ALLOWED_PORT, HIGHEST_PORT);
    return false;
}

static bool get_videoflip (const char *str, videoflip_t *videoflip) {
    if (strlen(str) > 1) return false;
    switch (str[0]) {
        case 'I':
            *videoflip = INVERT;
            break;
        case 'H':
            *videoflip = HFLIP;
            break;
        case 'V':
            *videoflip = VFLIP;
            break;
        default:
            return false;
    }
    return true;
}

static bool get_videorotate (const char *str, videoflip_t *videoflip) {
    if (strlen(str) > 1) return false;
    switch (str[0]) {
        case 'L':
            *videoflip = LEFT;
            break;
        case 'R':
            *videoflip = RIGHT;
            break;
        default:
            return false;
    }
    return true;
}

static void append_hostname(std::string &server_name) {
#ifdef _WIN32   /*modification for compilation on Windows */
    char buffer[256] = "";
    unsigned long size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size)) {
        std::string name = server_name;
        name.append("@");
        name.append(buffer);
        server_name = name;
    }
#else
    struct utsname buf;
    if (!uname(&buf)) {
        std::string name = server_name;
        name.append("@");
        name.append(buf.nodename);
        server_name = name;
    }
#endif
}

static void parse_arguments (int argc, char *argv[]) {
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
	if (arg == "-allow") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            i++;
	    allowed_clients.push_back(argv[i]);
	} else if (arg == "-block") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            i++;
	    blocked_clients.push_back(argv[i]);    
	} else if (arg == "-restrict") {
            if (i <  argc - 1) {
                if (strlen(argv[i+1]) == 2 && strncmp(argv[i+1], "no", 2) == 0) {
                    restrict_clients = false;
                    i++;
                    continue;
                }
	    } 
            restrict_clients = true;
        } else if (arg == "-n") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            server_name = std::string(argv[++i]);
        } else if (arg == "-nh") {
            do_append_hostname = false;
        } else if (arg == "-async") {
            audio_sync = true;
	    if (i <  argc - 1) {
                if (strlen(argv[i+1]) == 2 && strncmp(argv[i+1], "no", 2) == 0) {
                    audio_sync = false;
                    i++;
                    continue;
		}
                char *end;
                int n = (int) (strtof(argv[i + 1], &end) * 1000);
                if (*end == '\0') {
                    i++;
                    if (n > -SECOND_IN_USECS && n < SECOND_IN_USECS) {
                        audio_delay_alac = n * 1000; /* units are nsecs */
                    } else {
		      fprintf(stderr, "invalid -async %s: requested delays must be smaller than +/- 1000 millisecs\n", argv[i] );
                        exit (1);
                    }
                }
            }
        } else if (arg == "-vsync") {
            video_sync = true;
	    if (i <  argc - 1) {
                if (strlen(argv[i+1]) == 2 && strncmp(argv[i+1], "no", 2) == 0) {
                    video_sync = false;
                    i++;
                    continue;
                }
                char *end;
                int n = (int) (strtof(argv[i + 1], &end) * 1000);
                if (*end == '\0') {
                    i++;
                    if (n > -SECOND_IN_USECS && n < SECOND_IN_USECS) {
                        audio_delay_aac = n * 1000;     /* units are nsecs */
                    } else {
		      fprintf(stderr, "invalid -vsync %s: requested delays must be smaller than +/- 1000 millisecs\n", argv[i]);
                        exit (1);
                    }
                }
            }
        } else if (arg == "-s") {
            if (!option_has_value(i, argc, argv[i], argv[i+1])) exit(1);
            std::string value(argv[++i]);
            if (!get_display_settings(value, &display[0], &display[1], &display[2])) {
                fprintf(stderr, "invalid \"-s %s\"; -s wxh : max w,h=9999; -s wxh@r : max r=255\n",
                        argv[i]);
                exit(1);
            }
        } else if (arg == "-fps") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            unsigned int n = 255;
            if (!get_value(argv[++i], &n)) {
                fprintf(stderr, "invalid \"-fps %s\"; -fps n : max n=255, default n=30\n", argv[i]);
                exit(1);
            }
            display[3] = (unsigned short) n;
        } else if (arg == "-o") {
            display[4] = 1;
        } else if (arg == "-f") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            if (!get_videoflip(argv[++i], &videoflip[0])) {
                fprintf(stderr,"invalid \"-f %s\" , unknown flip type, choices are H, V, I\n",argv[i]);
                exit(1);
            }
        } else if (arg == "-r") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            if (!get_videorotate(argv[++i], &videoflip[1])) {
                fprintf(stderr,"invalid \"-r %s\" , unknown rotation  type, choices are R, L\n",argv[i]);
                exit(1);
            }
        } else if (arg == "-p") {
            if (i == argc - 1 || argv[i + 1][0] == '-') {
                tcp[0] = 7100; tcp[1] = 7000; tcp[2] = 7001;
                udp[0] = 7011; udp[1] = 6001; udp[2] = 6000;
                continue;
            }
            std::string value(argv[++i]);
            if (value == "tcp") {
                arg.append(" tcp");
                if(!get_ports(3, arg, argv[++i], tcp)) exit(1);
            } else if (value == "udp") {
                arg.append( " udp");
                if(!get_ports(3, arg, argv[++i], udp)) exit(1);
            } else {
                if(!get_ports(3, arg, argv[i], tcp)) exit(1);
                for (int j = 1; j < 3; j++) {
                    udp[j] = tcp[j];
                }
            }
        } else if (arg == "-m") {
            use_random_hw_addr  = true;
        } else if (arg == "-a") {
            use_audio = false;
        } else if (arg == "-d") {
            debug_log = !debug_log;
        } else if (arg == "-h"  || arg == "--help" || arg == "-?" || arg == "-help") {
            print_info(argv[0]);
            exit(0);
        } else if (arg == "-v") {
            printf("UxPlay version %s; for help, use option \"-h\"\n", VERSION);
            exit(0);
        } else if (arg == "-vp") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            video_parser.erase();
            video_parser.append(argv[++i]);
        } else if (arg == "-vd") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            video_decoder.erase();
            video_decoder.append(argv[++i]);
        } else if (arg == "-vc") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            video_converter.erase();
            video_converter.append(argv[++i]);
        } else if (arg == "-vs") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            videosink.erase();
            videosink.append(argv[++i]);
        } else if (arg == "-as") {
            if (!option_has_value(i, argc, arg, argv[i+1])) exit(1);
            audiosink.erase();
            audiosink.append(argv[++i]);
        } else if (arg == "-t") {
            fprintf(stderr,"The uxplay option \"-t\" has been removed: it was a workaround for an  Avahi issue.\n");
            fprintf(stderr,"The correct solution is to open network port UDP 5353 in the firewall for mDNS queries\n");
            exit(1);
        } else if (arg == "-nc") {
            new_window_closing_behavior = false;
        } else if (arg == "-avdec") {
            video_parser.erase();
            video_parser = "h264parse";
            video_decoder.erase();
            video_decoder = "avdec_h264";
            video_converter.erase();
            video_converter = "videoconvert";
        } else if (arg == "-v4l2" || arg == "-rpi") {
            if (arg == "-rpi") {
                LOGI("*** -rpi no longer includes -bt709: add it if needed");
            }
            video_decoder.erase();
            video_decoder = "v4l2h264dec";
            video_converter.erase();
            video_converter = "v4l2convert";
        } else if (arg == "-rpifb") {
            LOGI("*** -rpifb no longer includes -bt709: add it if needed");
            video_decoder.erase();
            video_decoder = "v4l2h264dec";
            video_converter.erase();
            video_converter = "v4l2convert";
            videosink.erase();
            videosink = "kmssink";
        } else if (arg == "-rpigl") {
            LOGI("*** -rpigl does not include -bt709: add it if needed");
            video_decoder.erase();
            video_decoder = "v4l2h264dec";
            video_converter.erase();
            video_converter = "v4l2convert";
            videosink.erase();
            videosink = "glimagesink";
        } else if (arg == "-rpiwl" ) {
            LOGI("*** -rpiwl no longer includes -bt709: add it if needed");
            video_decoder.erase();
            video_decoder = "v4l2h264dec";
            video_converter.erase();
            video_converter = "v4l2convert";
            videosink.erase();
            videosink = "waylandsink";
        } else if (arg == "-fs" ) {
            fullscreen = true;
	} else if (arg == "-FPSdata") {
            show_client_FPS_data = true;
        } else if (arg == "-reset") {
            max_ntp_timeouts = 0;
            if (!get_value(argv[++i], &max_ntp_timeouts)) {
                fprintf(stderr, "invalid \"-reset %s\"; -reset n must have n >= 0,  default n = %d\n", argv[i], NTP_TIMEOUT_LIMIT);
                exit(1);
            }
        } else if (arg == "-vdmp") {
            dump_video = true;
            if (i < argc - 1 && *argv[i+1] != '-') {
                unsigned int n = 0;
                if (get_value (argv[++i], &n)) {
                    if (n == 0) {
                        fprintf(stderr, "invalid \"-vdmp 0 %s\"; -vdmp n  needs a non-zero value of n\n", argv[i]);
                        exit(1);
                    }
                    video_dump_limit = n;
                    if (option_has_value(i, argc, arg, argv[i+1])) {
                        video_dumpfile_name.erase();
                        video_dumpfile_name.append(argv[++i]);
                    }
                } else {
                    video_dumpfile_name.erase();
                    video_dumpfile_name.append(argv[i]);
                }
            }
        } else if (arg == "-admp") {
            dump_audio = true;
            if (i < argc - 1 && *argv[i+1] != '-') {
                unsigned int n = 0;
                if (get_value (argv[++i], &n)) {
                    if (n == 0) {
                        fprintf(stderr, "invalid \"-admp 0 %s\"; -admp n  needs a non-zero value of n\n", argv[i]);
                        exit(1);
                    }
                    audio_dump_limit = n;
                    if (option_has_value(i, argc, arg, argv[i+1])) {
                        audio_dumpfile_name.erase();
                        audio_dumpfile_name.append(argv[++i]);
                    }
                } else {
                    audio_dumpfile_name.erase();
                    audio_dumpfile_name.append(argv[i]);
                }
            }
        } else if (arg  == "-ca" ) {
            if (option_has_value(i, argc, arg, argv[i+1])) {
                coverart_filename.erase();
                coverart_filename.append(argv[++i]);
            } else {
                fprintf(stderr,"option -ca must be followed by a filename for cover-art output\n");
                exit(1);
            }
        } else if (arg == "-bt709") {
            bt709_fix = true;
        } else if (arg == "-nohold") {
            max_connections = 3;
        } else if (arg == "-al") {
	    int n;
            char *end;
            if (i < argc - 1 && *argv[i+1] != '-') {
	      n = (int) (strtof(argv[++i], &end) * SECOND_IN_USECS);
                if (*end == '\0' && n >=0 && n <= 10 * SECOND_IN_USECS) {
                    audiodelay = n;
                    continue;
                }
            }
            fprintf(stderr, "invalid argument -al %s: must be a decimal time offset in seconds, range [0,10]\n"
                    "(like 5 or 4.8, which will be converted to a whole number of microseconds)\n", argv[i]);
            exit(1);
        } else if (arg == "-pair") {
            setup_legacy_pairing = true;
	} else {
            fprintf(stderr, "unknown option %s, stopping (for help use option \"-h\")\n",argv[i]);
            exit(1);
        }
    }
}

static void process_metadata(int count, const char *dmap_tag, const unsigned char* metadata, int datalen) {
    int dmap_type = 0;
    /* DMAP metadata items can be strings (dmap_type = 9); other types are byte, short, int, long, date, and list.  *
     * The DMAP item begins with a 4-character (4-letter) "dmap_tag" string that identifies the type.               */

    if (debug_log) {
        printf("%d: dmap_tag [%s], %d\n", count, dmap_tag, datalen);
    }

    /* UTF-8 String-type DMAP tags seen in Apple Music Radio are processed here.   *
     * (DMAP tags "asal", "asar", "ascp", "asgn", "minm" ). TODO expand this */  
    
    if (datalen == 0) {
        return;
    }

    if (dmap_tag[0] == 'a' && dmap_tag[1] == 's') {
        dmap_type = 9;
        switch (dmap_tag[2]) {
        case 'a':
            switch (dmap_tag[3]) {
            case 'a':
                printf("Album artist: ");  /*asaa*/
                break;
            case 'l':
                printf("Album: ");  /*asal*/
                break;
            case 'r':
                printf("Artist: ");  /*asar*/
                break;
            default:
                dmap_type = 0;
                break;
            }
            break;    
        case 'c':
            switch (dmap_tag[3]) {
            case 'm':
                printf("Comment: ");  /*ascm*/
                break;
            case 'n':
                printf("Content description: ");  /*ascn*/
                break;
            case 'p':
                printf("Composer: ");  /*ascp*/
                break;
            case 't':
                printf("Category: ");  /*asct*/
                break;
            default:
                dmap_type = 0;
                break;
            }
            break;
        case 's':
            switch (dmap_tag[3]) {
            case 'a':
                printf("Sort Artist: "); /*assa*/
                break;
            case 'c':
                printf("Sort Composer: ");  /*assc*/
                break;
            case 'l':
                printf("Sort Album artist: ");  /*assl*/
                break;
            case 'n':
                printf("Sort Name: ");  /*assn*/
                break;
            case 's':
                printf("Sort Series: ");  /*asss*/
                break;
            case 'u':
                printf("Sort Album: ");  /*assu*/
                break;
            default:
                dmap_type = 0;
                break;
            }
            break;
        default:
	    if (strcmp(dmap_tag, "asdt") == 0) {
                printf("Description: ");
            } else if (strcmp (dmap_tag, "asfm") == 0) {
                printf("Format: ");
            } else if (strcmp (dmap_tag, "asgn") == 0) {
                printf("Genre: ");
            } else if (strcmp (dmap_tag, "asky") == 0) {
                printf("Keywords: ");
            } else if (strcmp (dmap_tag, "aslc") == 0) {
                printf("Long Content Description: ");
            } else {
                dmap_type = 0;
            }
            break;
        }
    } else if (strcmp (dmap_tag, "minm") == 0) {
        dmap_type = 9;
        printf("Title: ");
    }

    if (dmap_type == 9) {
        char *str = (char *) calloc(1, datalen + 1);
        memcpy(str, metadata, datalen);
        printf("%s", str);
        free(str);
    } else if (debug_log) {
        for (int i = 0; i < datalen; i++) {
            if (i > 0 && i % 16 == 0) printf("\n");
            printf("%2.2x ", (int) metadata[i]);
        }
    }
    printf("\n");
}

static int parse_dmap_header(const unsigned char *metadata, char *tag, int *len) {
    const unsigned char *header = metadata;

    bool istag = true;
    for (int i = 0; i < 4; i++) {
        tag[i] =  (char) *header;
	if (!isalpha(tag[i])) {
            istag = false;
        }
        header++;
    }

    *len = 0;
    for (int i = 0; i < 4; i++) {
        *len <<= 8;
        *len += (int) *header;
        header++;
    }
    if (!istag || *len < 0) {
        return 1;
    }
    return 0;
}

static int register_dnssd() {
    int dnssd_error;
    uint64_t features;
    
    if ((dnssd_error = dnssd_register_raop(dnssd, raop_port))) {
        if (dnssd_error == -65537) {
             LOGE("No DNS-SD Server found (DNSServiceRegister call returned kDNSServiceErr_Unknown)");
        } else {
             LOGE("dnssd_register_raop failed with error code %d\n"
                  "mDNS Error codes are in range FFFE FF00 (-65792) to FFFE FFFF (-65537) "
                  "(see Apple's dns_sd.h)", dnssd_error);
        }
        return -3;
    }
    if ((dnssd_error = dnssd_register_airplay(dnssd, airplay_port))) {
        LOGE("dnssd_register_airplay failed with error code %d\n"
             "mDNS Error codes are in range FFFE FF00 (-65792) to FFFE FFFF (-65537) "
             "(see Apple's dns_sd.h)", dnssd_error);
        return -4;
    }

    LOGD("register_dnssd: advertised AirPlay service with \"Features\" code = 0x%X",
         dnssd_get_airplay_features(dnssd));
    return 0;
}

static void unregister_dnssd() {
    if (dnssd) {
        dnssd_unregister_raop(dnssd);
        dnssd_unregister_airplay(dnssd);
    }
    return;
}

static void stop_dnssd() {
    if (dnssd) {
        unregister_dnssd();
        dnssd_destroy(dnssd);
        dnssd = NULL;
	return;
    }	
}

static int start_dnssd(std::vector<char> hw_addr, std::string name) {
    int dnssd_error;
    if (dnssd) {
        LOGE("start_dnssd error: dnssd != NULL");
        return 2;
    }
    dnssd = dnssd_init(name.c_str(), strlen(name.c_str()), hw_addr.data(), hw_addr.size(), &dnssd_error);
    if (dnssd_error) {
        LOGE("Could not initialize dnssd library!: error %d", dnssd_error);
        return 1;
    }
    /* bit 27 of Features determines whether the AirPlay2 client-pairing protocol will be used (1) or not (0) */
    dnssd_set_airplay_features(dnssd, 27, (int) setup_legacy_pairing);
    return 0;
}

static bool check_client(char *deviceid) {
    bool ret = false;
    int list =  allowed_clients.size();
    for (int i = 0; i < list ; i++) {
        if (!strcmp(deviceid,allowed_clients[i].c_str())) {
	    ret = true;
	    break;
        }
    }
    return ret;
}

static bool check_blocked_client(char *deviceid) {
    bool ret = false;
    int list =  blocked_clients.size();
    for (int i = 0; i < list ; i++) {
        if (!strcmp(deviceid,blocked_clients[i].c_str())) {
	    ret = true;
	    break;
        }
    }
    return ret;
}

// Server callbacks
extern "C" void conn_init (void *cls) {
    open_connections++;
    LOGD("Open connections: %i", open_connections);
    //video_renderer_update_background(1);
}

extern "C" void conn_destroy (void *cls) {
    //video_renderer_update_background(-1);
    open_connections--;
    LOGD("Open connections: %i", open_connections);
    if (open_connections == 0) {
        remote_clock_offset = 0;
        if (use_audio) {
            audio_renderer_stop();
        }
    }
}

extern "C" void conn_reset (void *cls, int timeouts, bool reset_video) {
    LOGI("***ERROR lost connection with client (network problem?)");
    if (timeouts) {
        LOGI("   Client no-response limit of %d timeouts (%d seconds) reached:", timeouts, 3*timeouts);
        LOGI("   Sometimes the network connection may recover after a longer delay:\n"
             "   the default timeout limit n = %d can be changed with the \"-reset n\" option", NTP_TIMEOUT_LIMIT);
    }
    printf("reset_video %d\n",(int) reset_video);
    close_window = reset_video;    /* leave "frozen" window open if reset_video is false */
    raop_stop(raop);
    reset_loop = true;
}

extern "C" void conn_teardown(void *cls, bool *teardown_96, bool *teardown_110) {
    if (*teardown_110 && close_window) {
        reset_loop = true;
    }
}

extern "C" void report_client_request(void *cls, char *deviceid, char * model, char *name, bool * admit) {
    LOGI("connection request from %s (%s) with deviceID = %s\n", name, model, deviceid);
    if (restrict_clients) {
        *admit = check_client(deviceid);
	if (*admit == false) {
            LOGI("client connections have been restricted to those with listed deviceID,\nuse \"-allow %s\" to allow this client to connect.\n",
                 deviceid);
	}
    } else {
        *admit = true;
    }
    if (check_blocked_client(deviceid)) {
        *admit = false;
        LOGI("*** attempt to connect by blocked client (clientID %s): DENIED\n", deviceid);
    }
}

extern "C" void audio_process (void *cls, raop_ntp_t *ntp, audio_decode_struct *data) {
    if (dump_audio) {
        dump_audio_to_file(data->data, data->data_len, (data->data)[0] & 0xf0);
    }
    if (use_audio) {
        if (!remote_clock_offset) {
            remote_clock_offset = data->ntp_time_local - data->ntp_time_remote;
        }
        data->ntp_time_remote = data->ntp_time_remote + remote_clock_offset;
        switch (data->ct) {
        case 2:
            if (audio_delay_alac) {
                data->ntp_time_remote = (uint64_t) ((int64_t) data->ntp_time_remote + audio_delay_alac);
            }
            break;
        case 4:
        case 8:
            if (audio_delay_aac) {
                data->ntp_time_remote = (uint64_t) ((int64_t) data->ntp_time_remote + audio_delay_aac);
            }
            break;
        default:
            break;
        }
        audio_renderer_render_buffer(data->data, &(data->data_len), &(data->seqnum), &(data->ntp_time_remote));
    }
}

extern "C" void video_process (void *cls, raop_ntp_t *ntp, h264_decode_struct *data) {
    if (dump_video) {
        dump_video_to_file(data->data, data->data_len);
    }
    if (use_video) {
        if (!remote_clock_offset) {
            remote_clock_offset = data->ntp_time_local - data->ntp_time_remote;
        }
        data->ntp_time_remote = data->ntp_time_remote + remote_clock_offset;
        video_renderer_render_buffer(data->data, &(data->data_len), &(data->nal_count), &(data->ntp_time_remote));
    }
}

extern "C" void video_pause (void *cls) {
    if (use_video) {
        video_renderer_pause();
    }
}

extern "C" void video_resume (void *cls) {
    if (use_video) {
        video_renderer_resume();
    }
}


extern "C" void audio_flush (void *cls) {
    if (use_audio) {
        audio_renderer_flush();
    }
}

extern "C" void video_flush (void *cls) {
    if (use_video) {
        video_renderer_flush();
    }
}

extern "C" void audio_set_volume (void *cls, float volume) {
    if (use_audio) {
        audio_renderer_set_volume(volume);
    }
}

extern "C" void audio_get_format (void *cls, unsigned char *ct, unsigned short *spf, bool *usingScreen, bool *isMedia, uint64_t *audioFormat) {
    unsigned char type;
    LOGI("ct=%d spf=%d usingScreen=%d isMedia=%d  audioFormat=0x%lx",*ct, *spf, *usingScreen, *isMedia, (unsigned long) *audioFormat);
    switch (*ct) {
    case 2:
        type = 0x20;
        break;
    case 8:
        type = 0x80;
        break;
    default:
        type = 0x10;
        break;
    }
    if (audio_dumpfile && type != audio_type) {
        fclose(audio_dumpfile);
        audio_dumpfile = NULL;
    }
    audio_type = type;
    
    if (use_audio) {
      audio_renderer_start(ct);
    }

    if (coverart_filename.length()) {
        write_coverart(coverart_filename.c_str(), (const void *) empty_image, sizeof(empty_image));
    }
}

extern "C" void video_report_size(void *cls, float *width_source, float *height_source, float *width, float *height) {
    if (use_video) {
        video_renderer_size(width_source, height_source, width, height);
    }
}

extern "C" void audio_set_coverart(void *cls, const void *buffer, int buflen) {
    if (buffer && coverart_filename.length()) {
        write_coverart(coverart_filename.c_str(), buffer, buflen);
        LOGI("coverart size %d written to %s", buflen,  coverart_filename.c_str());
    }
}

extern "C" void audio_set_metadata(void *cls, const void *buffer, int buflen) {
    char dmap_tag[5] = {0x0};
    const unsigned char *metadata = (const  unsigned char *) buffer;
    int datalen;
    int count = 0;

    printf("==============Audio Metadata=============\n");

    if (buflen < 8) {
        LOGE("received invalid metadata, length %d < 8", buflen);
        return;
    } else if (parse_dmap_header(metadata, dmap_tag, &datalen)) {
        LOGE("received invalid metadata, tag [%s]  datalen %d", dmap_tag, datalen);
        return;
    }
    metadata += 8;
    buflen -= 8;

    if (strcmp(dmap_tag, "mlit") != 0 || datalen != buflen) {
        LOGE("received metadata with tag %s, but is not a DMAP listingitem, or datalen = %d !=  buflen %d",
             dmap_tag, datalen, buflen);
        return;
    }
    while (buflen >= 8) {
        count++;
        if (parse_dmap_header(metadata, dmap_tag, &datalen)) {
            LOGE("received metadata with invalid DMAP header:  tag = [%s],  datalen = %d", dmap_tag, datalen);
            return;
        }
        metadata += 8;
        buflen -= 8;
        process_metadata(count, (const char *) dmap_tag, metadata, datalen);
        metadata += datalen;
        buflen -= datalen;
    }
    if (buflen != 0) {
      LOGE("%d bytes of metadata were not processed", buflen);
    }
}

extern "C" void log_callback (void *cls, int level, const char *msg) {
    switch (level) {
        case LOGGER_DEBUG: {
            LOGD("%s", msg);
            break;
        }
        case LOGGER_WARNING: {
            LOGW("%s", msg);
            break;
        }
        case LOGGER_INFO: {
            LOGI("%s", msg);
            break;
        }
        case LOGGER_ERR: {
            LOGE("%s", msg);
            break;
        }
        default:
            break;
    }
}

int start_raop_server (unsigned short display[5], unsigned short tcp[3], unsigned short udp[3], bool debug_log) {
    raop_callbacks_t raop_cbs;
    memset(&raop_cbs, 0, sizeof(raop_cbs));
    raop_cbs.conn_init = conn_init;
    raop_cbs.conn_destroy = conn_destroy;
    raop_cbs.conn_reset = conn_reset;
    raop_cbs.conn_teardown = conn_teardown;
    raop_cbs.audio_process = audio_process;
    raop_cbs.video_process = video_process;
    raop_cbs.audio_flush = audio_flush;
    raop_cbs.video_flush = video_flush;
    raop_cbs.video_pause = video_pause;
    raop_cbs.video_resume = video_resume;
    raop_cbs.audio_set_volume = audio_set_volume;
    raop_cbs.audio_get_format = audio_get_format;
    raop_cbs.video_report_size = video_report_size;
    raop_cbs.audio_set_metadata = audio_set_metadata;
    raop_cbs.audio_set_coverart = audio_set_coverart;
    raop_cbs.report_client_request = report_client_request;

    /* set max number of connections = 2 to protect against capture by new client */
    raop = raop_init(max_connections, &raop_cbs);
    if (raop == NULL) {
        LOGE("Error initializing raop!");
        return -1;
    }

    /* write desired display pixel width, pixel height, refresh_rate, max_fps, overscanned.  */
    /* use 0 for default values 1920,1080,60,30,0; these are sent to the Airplay client      */

    if (display[0]) raop_set_plist(raop, "width", (int) display[0]);
    if (display[1]) raop_set_plist(raop, "height", (int) display[1]);
    if (display[2]) raop_set_plist(raop, "refreshRate", (int) display[2]);
    if (display[3]) raop_set_plist(raop, "maxFPS", (int) display[3]);
    if (display[4]) raop_set_plist(raop, "overscanned", (int) display[4]);
 
    if (show_client_FPS_data) raop_set_plist(raop, "clientFPSdata", 1);
    raop_set_plist(raop, "max_ntp_timeouts", max_ntp_timeouts);
    if (audiodelay >= 0) raop_set_plist(raop, "audio_delay_micros", audiodelay);

    /* network port selection (ports listed as "0" will be dynamically assigned) */
    raop_set_tcp_ports(raop, tcp);
    raop_set_udp_ports(raop, udp);
    
    raop_set_log_callback(raop, log_callback, NULL);
    raop_set_log_level(raop, log_level);

    raop_port = raop_get_port(raop);
    raop_start(raop, &raop_port);
    raop_set_port(raop, raop_port);

    if (tcp[2]) {
        airplay_port = tcp[2];
    } else {
        /* is there a problem if this coincides with a randomly-selected tcp raop_mirror_data port? 
         * probably not, as the airplay port is only used for initial client contact */
        airplay_port = (raop_port != HIGHEST_PORT ? raop_port + 1 : raop_port - 1);
    }
    if (dnssd) {
        raop_set_dnssd(raop, dnssd);
    } else {
        LOGE("raop_set failed to set dnssd");
        return -2;
    }
    return 0;
}

static void stop_raop_server () {
    if (raop) {
        raop_destroy(raop);
        raop = NULL;
    }
    return;
}

static void read_config_file(const char * filename, const char * uxplay_name) {
    std::string config_file = filename;
    std::string option_char = "-";
    std::vector<std::string> options;
    options.push_back(uxplay_name);
    std::ifstream file(config_file);
    if (file.is_open()) {
        fprintf(stdout,"UxPlay: reading configuration from  %s\n", config_file.c_str());
        std::string line;
        while (std::getline(file, line)) {
            if (line[0] == '#') continue;
            //  first process line into separate option items with '\0' as delimiter
            bool is_part_of_item, in_quotes;
            char endchar;
            is_part_of_item = false;
            for (int i = 0; i < (int) line.size(); i++) {
                if (is_part_of_item == false) {
                    if (line[i] == ' ') {
                        line[i] = '\0';
                    } else {
                        // start of new item
                        is_part_of_item = true;
                        switch (line[i]) {
                        case '\'':
                        case '\"':
                            endchar = line[i];
                            line[i] = '\0';
                            in_quotes = true;
                            break;
                        default:
                            in_quotes = false;
		            endchar = ' ';
		            break;
                        }
                    }
                } else {
                    /* previous character was inside this item */
                    if (line[i] == endchar) {
                        if (in_quotes) {
                            /* cases where endchar is inside quoted item */
                            if (i > 0 && line[i - 1] == '\\') continue;
                            if (i + 1 < (int) line.size() && line[i + 1] != ' ') continue;
		        }
                        line[i] =  '\0';
                        is_part_of_item = false;
                    }
                }
            }

            // now tokenize the processed line   
            std::istringstream iss(line);
            std::string token;
            bool first = true;
            while (std::getline(iss, token, '\0')) {
                if (token.size() > 0) {
                    if (first) {
                        options.push_back(option_char + token.c_str());
                        first = false;
                    } else {
                        options.push_back(token.c_str());
                    }
		}
	    }
	}
        file.close();
    } else {
        fprintf(stderr,"UxPlay: failed to open configuration file at %s\n", config_file.c_str());
    }
    if (options.size() > 1) {
        int argc = options.size();
        char **argv = (char **) malloc(sizeof(char*) * argc);
        for (int i = 0; i < argc; i++) {
            argv[i] = (char *) options[i].c_str();
        }
        parse_arguments (argc, argv);
        free (argv);
    }
}

int main (int argc, char *argv[]) {
    std::vector<char> server_hw_addr;
    std::string mac_address;
    std::string config_file = "";

#ifdef SUPPRESS_AVAHI_COMPAT_WARNING
    // suppress avahi_compat nag message.  avahi emits a "nag" warning (once)
    // if  getenv("AVAHI_COMPAT_NOWARN") returns null.
    static char avahi_compat_nowarn[] = "AVAHI_COMPAT_NOWARN=1";
    if (!getenv("AVAHI_COMPAT_NOWARN")) putenv(avahi_compat_nowarn);
#endif

    config_file = find_uxplay_config_file();
    if (config_file.length()) {
        read_config_file(config_file.c_str(), argv[0]);
    }
    parse_arguments (argc, argv);

    log_level = (debug_log ? LOGGER_DEBUG : LOGGER_INFO);
    
#ifdef _WIN32    /*  use utf-8 terminal output; don't buffer stdout in WIN32 when debug_log = false */
    SetConsoleOutputCP(CP_UTF8);
    if (!debug_log) {
        setbuf(stdout, NULL);
    }
#endif

    LOGI("UxPlay %s: An Open-Source AirPlay mirroring and audio-streaming server.", VERSION);

    if (audiosink == "0") {
        use_audio = false;
        dump_audio = false;
    }
    if (dump_video) {
        if (video_dump_limit > 0) {
             printf("dump video using \"-vdmp %d %s\"\n", video_dump_limit, video_dumpfile_name.c_str());
	} else {
             printf("dump video using \"-vdmp %s\"\n", video_dumpfile_name.c_str());
        }
    }
    if (dump_audio) {
        if (audio_dump_limit > 0) {
            printf("dump audio using \"-admp %d %s\"\n", audio_dump_limit, audio_dumpfile_name.c_str());
        } else {
            printf("dump audio using \"-admp %s\"\n",  audio_dumpfile_name.c_str());
        }
    }

#if __APPLE__
    /* force use of -nc option on macOS */
    LOGI("macOS detected: use -nc option as workaround for GStreamer problem");
    new_window_closing_behavior = false;
#endif

    if (videosink == "0") {
        use_video = false;
	videosink.erase();
        videosink.append("fakesink");
	LOGI("video_disabled");
        display[3] = 1; /* set fps to 1 frame per sec when no video will be shown */
    }

    if (fullscreen && use_video) {
        if (videosink == "waylandsink" || videosink == "vaapisink") {
            videosink.append(" fullscreen=true");
	}
    }

    if (videosink == "d3d11videosink"  && use_video) {
        videosink.append(" fullscreen-toggle-mode=alt-enter");  
        printf("d3d11videosink is being used with option fullscreen-toggle-mode=alt-enter\n"
               "Use Alt-Enter key combination to toggle into/out of full-screen mode\n");
    }

    if (bt709_fix && use_video) {
        video_parser.append(" ! ");
        video_parser.append(BT709_FIX);
    }

    if (do_append_hostname) {
        append_hostname(server_name);
    }

    if (!gstreamer_init()) {
        LOGE ("stopping");
        exit (1);
    }

    render_logger = logger_init();
    logger_set_callback(render_logger, log_callback, NULL);
    logger_set_level(render_logger, log_level);

    if (use_audio) {
      audio_renderer_init(render_logger, audiosink.c_str(), &audio_sync, &video_sync);
    } else {
        LOGI("audio_disabled");
    }

    if (use_video) {
        video_renderer_init(render_logger, server_name.c_str(), videoflip, video_parser.c_str(),
                            video_decoder.c_str(), video_converter.c_str(), videosink.c_str(), &fullscreen, &video_sync);
        video_renderer_start();
    }

    if (udp[0]) {
        LOGI("using network ports UDP %d %d %d TCP %d %d %d", udp[0], udp[1], udp[2], tcp[0], tcp[1], tcp[2]);
    }

    if (!use_random_hw_addr) {
        mac_address = find_mac();
    }
    if (mac_address.empty()) {
        srand(time(NULL) * getpid());
        mac_address = random_mac();
        LOGI("using randomly-generated MAC address %s",mac_address.c_str());
    } else {
        LOGI("using system MAC address %s",mac_address.c_str());
    }
    parse_hw_addr(mac_address, server_hw_addr);
    mac_address.clear();

    if (coverart_filename.length()) {
        LOGI("any AirPlay audio cover-art will be written to file  %s",coverart_filename.c_str());
        write_coverart(coverart_filename.c_str(), (const void *) empty_image, sizeof(empty_image));
    }

    restart:
    if (start_dnssd(server_hw_addr, server_name)) {
        goto cleanup;
    }
    if (start_raop_server(display, tcp, udp, debug_log)) {
        stop_dnssd();
        goto cleanup;
    }
    if (register_dnssd()) {
        stop_raop_server();
        stop_dnssd();
        goto cleanup;
    }
    reconnect:
    compression_type = 0;
    close_window = new_window_closing_behavior; 
    main_loop();
    if (relaunch_video || reset_loop) {
        if(reset_loop) {
            reset_loop = false;
        } else {
            raop_stop(raop);
        }
        if (use_audio) audio_renderer_stop();
        if (use_video && close_window) {
            video_renderer_destroy();
            video_renderer_init(render_logger, server_name.c_str(), videoflip, video_parser.c_str(),
                                video_decoder.c_str(), video_converter.c_str(), videosink.c_str(), &fullscreen,
                                &video_sync);
            video_renderer_start();
        }
        if (relaunch_video) {
            unsigned short port = raop_get_port(raop);
            raop_start(raop, &port);
            raop_set_port(raop, port);
            goto reconnect;
        } else {
            LOGI("Re-launching RAOP server...");
            stop_raop_server();
            stop_dnssd();
            goto restart;
        }
    } else {
        LOGI("Stopping...");
        stop_raop_server();
        stop_dnssd();
    }
    cleanup:
    if (use_audio) {
        audio_renderer_destroy();
    }
    if (use_video)  {
        video_renderer_destroy();
    }
    logger_destroy(render_logger);
    render_logger = NULL;
    if(audio_dumpfile) {
        fclose(audio_dumpfile);
    }
    if (video_dumpfile) {
        fwrite(mark, 1, sizeof(mark), video_dumpfile);
        fclose(video_dumpfile);
    }
    if (coverart_filename.length()) {
	remove (coverart_filename.c_str());
    }
}
