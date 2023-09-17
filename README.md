# UxPlay 1.67:  AirPlay-Mirror and AirPlay-Audio server for Linux, macOS, and Unix (now also runs on Windows).

### Now developed at the GitHub site [https://github.com/FDH2/UxPlay](https://github.com/FDH2/UxPlay) (where all user issues should be posted).

## Highlights:

   * GPLv3, open source.
   * Originally supported only AirPlay Mirror protocol, now has added support
     for AirPlay Audio-only (Apple Lossless ALAC) streaming 
     from current iOS/iPadOS clients.  **There is no support for Airplay2 video-streaming protocol, and none is planned.**
   * macOS computers (2011 or later, both Intel and "Apple Silicon" M1/M2
     systems) can act either as AirPlay clients, or
     as the server running UxPlay. Using AirPlay, UxPlay can
     emulate a second display for macOS clients.
   * Support for older iOS clients (such as 32-bit iPad 2nd gen., iPod Touch 5th gen. and
     iPhone 4S, when upgraded to iOS 9.3.5, or later 64-bit devices), plus a
     Windows AirPlay-client emulator, AirMyPC.
   * Uses GStreamer plugins for audio and video rendering (with options
     to select different hardware-appropriate output "videosinks" and
     "audiosinks", and a fully-user-configurable video streaming pipeline).
   * Support for server behind a firewall.
   * Raspberry Pi support  **both with and without hardware  video decoding** by the Broadcom GPU.  _Tested on Raspberry Pi 4 Model B and  Pi 3 model B+._
   * Support for running on Microsoft Windows (builds with the MinGW-64 compiler in the
     unix-like MSYS2 environment).

## Packaging status (Linux and \*BSD distributions)

[![Current Packaging status](https://repology.org/badge/vertical-allrepos/uxplay.svg)](https://repology.org/project/uxplay/versions).

* Install uxplay on Debian-based Linux systems with "`sudo apt install uxplay`"; on FreeBSD with "``sudo pkg install uxplay``".  Also
available on Arch-based systems through AUR.   It may soon be available on some RPM-based distributions (upcoming Fedora 39?: "``sudo dnf install uxplay``").

* **NEW**: for RPM-based distributions which have not packaged UxPlay, a RPM "specfile" **uxplay.spec** is now provided with recent
[UxPlay releases](https://github.com/FDH2/UxPlay/releases) >= 1.65.3 (see their  "Assets"), and can also be found in the UxPlay source top directory.
This can be used to build an installable RPM package. First-time RPM builders should first install the rpm-build and rpmdevtools packages,
then create the rpmbuild tree with "`rpmdev-setuptree`".  Then download and
copy uxplay.spec into ``~/rpmbuild/SPECS``.  In that directory, run "`rpmdev-spectool -g -R  uxplay.spec`" to download the corresponding
source file `uxplay-*.tar.gz` into ``~/rpmbuild/SOURCES`` ("rpmdev-spectool" may also be just called "spectool"); then
run "```rpmbuild -ba uxplay.spec```" (you will need to install
any required dependencies this reports).  This should create the uxplay RPM package in a subdirectory of `~/rpmbuild/RPMS`.
(**uxplay.spec** is tested on Fedora 38, Rocky Linux 9.2, openSUSE Leap 15.5, Mageia 9, OpenMandriva, PCLinuxOS;
it can be easily modified to include dependency lists for other RPM-based distributions.)

* On Linux and  \*BSD the mDNS/DNS-SD (Bonjour/ZeroConf) local network services needed by UxPlay are usually provided by Avahi: **if
there is a firewall on the server that will host UxPlay, make sure the default network port for mDNS queries (UDP 5353) is open**. (Uxplay
can work without this port by using only the host's loopback interface, but its visibility to clients will be
degraded.)  See the [Troubleshooting](#troubleshooting) section below for more details. (With a firewall, you also need to open
ports for UxPlay, and use the `-p <n>` option; see `man uxplay` or ``uxplay -h``.)

* Even if you install your distribution's pre-compiled uxplay binary package, you may need to read the instructions below
for [running UxPlay](#running-uxplay) to see which of your distribution's **GStreamer plugin packages** you should also install.

* For hardware-accelerated video decoding on Raspberry Pi, Ubuntu < 23.04 needs GStreamer 
to be [patched](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches) (recommended but optional
for Raspberry Pi OS (Bullseye), no longer needed for Manjaro >= 23.02).
_(Only these three distributions supply a kernel module maintained by Raspberry Pi outside the mainline Linux kernel
that accesses the firmware decoder in the Broadcom GPU)_.

* To (easily) compile the latest UxPlay from source, see the section [Getting UxPlay](#getting-uxplay).

# Detailed description of UxPlay
This project is a GPLv3 open source unix AirPlay2 Mirror server for Linux, macOS, and \*BSD.
It was initially developed by
[antimof](http://github.com/antimof/Uxplay) using code 
from OpenMAX-based [RPiPlay](https://github.com/FD-/RPiPlay), which in turn derives from
[AirplayServer](https://github.com/KqsMea8/AirplayServer),
[shairplay](https://github.com/juhovh/shairplay), and [playfair](https://github.com/EstebanKubata/playfair).
(The antimof site is no longer involved in
development, but periodically posts updates pulled from the new
main [UxPlay site](https://github.com/FDH2/UxPlay)). 

UxPlay is tested on a number of systems, including (among others) Debian (10 "Buster", 11 "Bullseye", 12 "Bookworm"),
Ubuntu (20.04 LTS, 22.04 LTS, 23.04 (also Ubuntu derivatives Linux Mint, Pop!\_OS), Red Hat and clones (Fedora 38,
Rocky Linux 9.2), openSUSE Leap 15.5, Mageia 9, OpenMandriva "ROME", PCLinuxOS, Arch Linux, Manjaro, and should run on any Linux system.
Also tested on macOS 13.3 (Intel and M2), FreeBSD 13.2, Windows 10 and 11 (64 bit).

On Raspberry Pi 4 model B, it is tested on Raspberry Pi OS (Bullseye) (32- and 64-bit), Ubuntu 22.04 LTS and 23.04, Manjaro RPi4 23.02,
and (without hardware video decoding) on openSUSE 15.5. Also tested on Raspberry Pi 3 model B+.

Its main use is to act like an AppleTV for screen-mirroring (with audio) of iOS/iPadOS/macOS clients
(iPhone, iPod Touch, iPad, Mac computers) on the server display
of a host running Linux, macOS, or other unix (and now also Microsoft Windows).  UxPlay supports
Apple's AirPlay2 protocol using "Legacy Protocol", but some features are missing.
(Details of what is publicly known about Apple's AirPlay 2 protocol can be found
[here](https://openairplay.github.io/airplay-spec/),
[here](https://github.com/SteeBono/airplayreceiver/wiki/AirPlay2-Protocol) and
[here](https://emanuelecozzi.net/docs/airplay2); see also [pyatv](https://pyatv.dev/documentation/protocols) which could be
a resource for adding modern protocols.)  While there is no guarantee that future
iOS releases will keep supporting "Legacy Protocol",  iOS 17 continues support.

The UxPlay server and its client must be on the same local area network,
on which a **Bonjour/Zeroconf mDNS/DNS-SD server**  is also running
(only DNS-SD "Service Discovery" service is strictly necessary, it is not necessary
that the local network also be of the ".local" mDNS-based type). 
On Linux and BSD Unix servers, this is usually provided by [Avahi](https://www.avahi.org),
through the avahi-daemon service, and is included in  most Linux distributions (this
service can also be provided by macOS, iOS or Windows servers).

Connections to the UxPlay server by
iOS/MacOS  clients can be initiated both in **AirPlay Mirror** mode (which streams
lossily-compressed AAC audio while mirroring the client screen,
or in the alternative **AirPlay Audio** mode which streams
Apple Lossless (ALAC) audio without screen mirroring. In **Audio** mode,
metadata is displayed in the uxplay terminal;
if UxPlay option ``-ca <name>`` is used,
the accompanying cover art is also output 
to a periodically-updated file `<name>`, and can be viewed with
a (reloading) graphics viewer of your choice.
_Switching between_ **Mirror** _and_ **Audio** _modes  during an active  connection is
possible: in_ **Mirror** _mode, stop mirroring (or close the mirror window) and start an_ **Audio** _mode connection,
switch back by initiating a_ **Mirror** _mode connection; cover-art display stops/restarts as you leave/re-enter_ **Audio** _mode._

* **Note that Apple video-DRM
(as found in "Apple TV app" content on the client) cannot be decrypted by UxPlay, and
the Apple TV app cannot be watched using UxPlay's AirPlay Mirror mode (only the unprotected audio will be streamed, in AAC format),
but both video and audio content from  DRM-free apps like "YouTube app" will be streamed  by UxPlay in Mirror mode.**

* **As UxPlay does not support non-Mirror AirPlay2 video streaming (where the
client controls a web server on the AirPlay server that directly receives
content to avoid it being decoded and re-encoded by the client),
using the icon for AirPlay video in apps such as the YouTube app
will only send audio (in lossless ALAC format) without the accompanying video.**

### Possibility for using hardware-accelerated h264 video-decoding, if available.

UxPlay uses [GStreamer](https://gstreamer.freedesktop.org) "plugins" for rendering
audio and video.  This means that video and audio are supported "out of the box",
using a choice of plugins.  AirPlay streams video in h264 format: gstreamer decoding
is plugin agnostic, and uses accelerated GPU hardware h264 decoders if available;
if not, software decoding is used. 

* **VAAPI for Intel and AMD integrated graphics, NVIDIA with "Nouveau" open-source driver**

   With an Intel or AMD GPU,  hardware decoding with the open-source VAAPI gstreamer
   plugin is preferable.   The open-source "Nouveau" drivers for NVIDIA graphics are
   also in principle supported:
   see [here](https://nouveau.freedesktop.org/VideoAcceleration.html), but this requires
   VAAPI to be supplemented with firmware extracted from the proprietary NVIDIA drivers.

* **NVIDIA with proprietary drivers**

   The `nvh264dec` plugin 
   (included in gstreamer1.0-plugins-bad since GStreamer-1.18.0)
   can be used for accelerated video decoding on the NVIDIA GPU after
   NVIDIA's CUDA driver `libcuda.so` is installed. For GStreamer-1.16.3
   or earlier, the plugin is called `nvdec`, and
   must be [built by the user](https://github.com/FDH2/UxPlay/wiki/NVIDIA-nvdec-and-nvenc-plugins).

*  **Video4Linux2 support for the Raspberry Pi Broadcom 2835 GPU**

    Raspberry Pi (RPi) computers (tested on Pi 4 Model B) can now run UxPlay using software video decoding,
    but hardware-accelerated decoding by firmware in the Pi's 
    GPU is prefered.  UxPlay accesses this using the GStreamer-1.22 Video4Linux2 (v4l2) plugin;
    the plugin from older GStreamer needs a patch to backport fixes from v1.22 (already applied in
    Raspberry Pi OS (Bullseye), and available for 1.18.4 and later
    in the [UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches)). Also
    requires the out-of-mainline Linux kernel module  bcm2835-codec  maintained by Raspberry Pi,
    so far only included in Raspberry Pi OS, and two other  distributions (Ubuntu, Manjaro) available
    with Raspberry Pi Imager.


### Note to packagers:

UxPlay's GPLv3 license does not have an added
"GPL exception" explicitly allowing it to be distributed in compiled form when linked to OpenSSL versions
**prior to v. 3.0.0** (older versions of OpenSSL have a license clause incompatible with the GPL unless
OpenSSL can be regarded as a "System Library", which it is in *BSD).  Many Linux distributions treat OpenSSL
as a "System Library", but some (e.g. Debian) do not: in this case, the issue is solved by linking
with OpenSSL-3.0.0  or later.

# Getting UxPlay

Either download and unzip [UxPlay-master.zip](https://github.com/FDH2/UxPlay/archive/refs/heads/master.zip), 
or (if git is installed): "git clone https://github.com/FDH2/UxPlay".   You
can also download a recent or earlier version listed
in [Releases](https://github.com/FDH2/UxPlay/releases).

* A recent UxPlay  can also be found on the original [antimof site](https://github.com/antimof/UxPlay);
that original project is inactive, but is usually kept current or almost-current with the 
[active UxPlay github site](https://github.com/FDH2/UxPlay) (thank you antimof!).

## Building UxPlay on  Linux (or \*BSD):

### Debian-based systems:

(Adapt these instructions for non-Debian-based Linuxes or *BSD; for macOS,
see specific instruction below). See [Troubleshooting](#troubleshooting) below for help with
any difficulties.

You need a C/C++ compiler (e.g. g++) with the standard development libraries
installed.  Debian-based systems provide a package "build-essential" for use
in compiling software.   You also need pkg-config: if it is not found
by "`which pkg-config`",  install pkg-config or its  work-alike replacement
pkgconf.   Also make sure that cmake>=3.5 is installed:
"`sudo apt-get install cmake`" (add ``build-essential`` and `pkg-config`
(or ``pkgconf``)  to this if needed).

Make sure that your distribution provides OpenSSL 1.1.1 or later, and
libplist 2.0 or later. (This means  Debian 10 "Buster" based systems (e.g, Ubuntu 18.04) or newer;
on Debian 10 systems "libplist" is an older version, you need "libplist3".) If it does
not, you may need to build and install these from
source (see instructions at the end of this README).  If you have a non-standard OpenSSL
installation, you may need to set the environment variable OPENSSL_ROOT_DIR
(_e.g._ , "`export OPENSSL_ROOT_DIR=/usr/local/lib64`" if that is where it is installed).

* Most users will use the GStreamer supplied by their distribution, but a few (in particular users
of Raspberry Pi OS Lite Legacy (Buster) on a Raspberry Pi model 4B who wish to stay on that
unsupported Legacy OS for compatibility with other apps) should instead build a newer Gstreamer from source
following  [these instructions](https://github.com/FDH2/UxPlay/wiki/Building-latest-GStreamer-from-source-on-distributions-with-older-GStreamer-(e.g.-Raspberry-Pi-OS-).) . **Do this
_before_ building UxPlay**.



In a terminal window, change directories to the source directory of the
downloaded source code ("UxPlay-\*", "\*" = "master" or the release tag for
zipfile downloads, "UxPlay" for "git clone" downloads), then follow the instructions below:

**Note:** By default UxPlay will be built with optimization for the
computer it is built on; when this is not the case, as when you are packaging
for a distribution, use the cmake option `-DNO_MARCH_NATIVE=ON`.

If you use X11 Windows on Linux or *BSD, and wish to toggle in/out of fullscreen mode with a keypress
(F11 or Alt_L+Enter)
UxPlay needs to be built with a dependence on X11.  Starting with UxPlay-1.59, this will be done by
default **IF** the X11 development libraries are installed and detected.   Install these with
"`sudo apt-get install libx11-dev`".    If GStreamer < 1.20 is detected, a fix needed by 
screen-sharing apps (_e.g._, Zoom) will also be made.

* If X11 development libraries are present, but you
wish to build UxPlay *without* any X11 dependence, use
the cmake option `-DNO_X11_DEPS=ON`.

1. `sudo apt-get install libssl-dev libplist-dev`".
    (_unless you need to build OpenSSL and libplist from source_).
2.  `sudo apt-get install libavahi-compat-libdnssd-dev`
3.  `sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev`. (\*_Skip if you built Gstreamer from source_)
4.  `cmake .` (_For a cleaner build, which is useful if you modify the source, replace this 
    by_ "``mkdir build; cd build; cmake ..``": _you can then delete the contents of the
    `build` directory if needed, without affecting the source._)   Also add any cmake "`-D`" options
    here as needed (e.g, `-DNO_X11_DEPS=ON` or ``-DNO_MARCH_NATIVE=ON``).
5. `make`
6. `sudo make install` (you can afterwards uninstall with ``sudo make uninstall``
    in the same directory in which this was run).

This installs the executable file "`uxplay`" to `/usr/local/bin`, (and installs a manpage to
somewhere standard like `/usr/local/share/man/man1` and README
files to somewhere like `/usr/local/share/doc/uxplay`).  (If "man uxplay" fails, check if $MANPATH is set:
if so, the path to the  manpage (usually /usr/local/share/man/) needs to be added to $MANPATH .)
The uxplay executable  can also be found in the build directory after the build
process, if you wish to test before installing (in which case
the GStreamer plugins must first be installed).



### Building on  non-Debian Linux and \*BSD
**For those with RPM-based distributions, a RPM spec file uxplay.spec is also available for 
building a rpm package with rpmbuild**. See ["Packaging Status"](#packaging-status-linux-and-bsd-distributions) section above.


* **Red Hat, or clones like CentOS (now continued as Rocky Linux or Alma Linux):** 
(sudo dnf install, or sudo yum install) openssl-devel libplist-devel avahi-compat-libdns_sd-devel
gstreamer1-devel gstreamer1-plugins-base-devel (+libX11-devel for fullscreen X11) _(some of these 
may be in the "CodeReady" add-on repository, called "PowerTools" by clones)_ 

* **Mageia, PCLinuxOS, OpenMandriva:**
Same as Red Hat, except for name changes: (Mageia) "gstreamer1.0-devel", "gstreamer-plugins-base1.0-devel";
(OpenMandriva) "libopenssl-devel", "gstreamer-devel", "libgst-plugins-base1.0-devel".  PCLinuxOS: same as Mageia, 
but uses synaptic (or apt) as its package manager.

 * **openSUSE:**
(sudo zypper install) libopenssl-3-devel (formerly
 libopenssl-devel) libplist-2_0-devel (formerly libplist-devel)
avahi-compat-mDNSResponder-devel gstreamer-devel 
gstreamer-plugins-base-devel (+ libX11-devel for fullscreen X11).

* **Arch Linux** (_Also available as a package in AUR_):
(sudo pacman -Syu) openssl libplist avahi  gst-plugins-base. 

* **FreeBSD:** (sudo pkg install) libplist gstreamer1.
Either avahi-libdns or mDNSResponder must also be installed to provide the dns_sd library.
OpenSSL is already installed as a System Library.


## Running UxPlay

### Installing plugins (Debian-based Linux systems) (_skip if you built a complete GStreamer from source_)

Next install the GStreamer plugins that are needed with `sudo apt-get install gstreamer1.0-<plugin>`.
Values of `<plugin>` required are: 

1. "**plugins-base**" 
2. "**libav**" (for sound),
3. "**plugins-good**" (for v4l2 hardware h264 decoding)
4. "**plugins-bad**" (for h264 decoding).   

Plugins that may also be needed include "**gl**" for OpenGL support (which may be useful, and should
be used with h264 decoding by the NVIDIA GPU), and "**x**" for
X11 support, although these may  already be installed; "**vaapi**"
is needed for hardware-accelerated h264 video decoding by Intel
or AMD  graphics (but not for use with NVIDIA using proprietary drivers). If sound is 
not working, "**alsa**"", "**pulseaudio**", or "**pipewire**" plugins may need to be
installed, depending on how your audio is set up.

* Also install "**gstreamer1.0-tools**" to get the utility gst-inspect-1.0 for examining the GStreamer installation.  


### Installing plugins (Non-Debian-based Linux or \*BSD) (_skip if you built a complete GStreamer from source_)

In some cases, because of patent issues,
the libav plugin feature **avdec_aac** needed for decoding AAC audio in mirror mode is not provided in the official distribution:
get it from community repositories for those distributions.

* **Red Hat, or clones like CentOS (now continued as Rocky Linux or Alma Linux):**
Install gstreamer1-libav gstreamer1-plugins-bad-free (+ gstreamer1-vaapi
for Intel/AMD graphics).  In recent Fedora, gstreamer1-libav is renamed gstreamer1-plugin-libav.
**To get avdec_aac, install packages from [rpmfusion.org](https://rpmfusion.org)**:  (get ffmpeg-libs from rpmfusion;
on RHEL or clones, but not recent Fedora, also get gstreamer1-libav from there).

* **Mageia, PCLinuxOS, OpenMandriva:**
Install gstreamer1.0-libav gstreamer1.0-plugins-bad (+ gstreamer1.0-vaapi
for Intel/AMD graphics). **On Mageia, to get avdec_aac, install ffmpeg from the "tainted" repository**,
(which also provides a more complete gstreamer1.0-plugins-bad).

* **openSUSE:**
Install gstreamer-plugins-libav gstreamer-plugins-bad (+ gstreamer-plugins-vaapi
for Intel/AMD graphics).  **To get avdec_aac, install libav\* packages for openSUSE
from [Packman](https://ftp.gwdg.de/pub/linux/misc/packman/suse/) "Essentials"**; recommendation: after adding the
Packman repository, use the option in YaST Software management to switch
all system packages for multimedia to Packman).

* **Arch Linux**
Install gst-plugins-good gst-plugins-bad gst-libav (+ gstreamer-vaapi
for Intel/AMD graphics). 

 * **FreeBSD:** Install gstreamer1-libav, gstreamer1-plugins, gstreamer1-plugins-*
(\* = core, good,  bad, x, gtk, gl, vulkan, pulse, v4l2,  ...), (+ gstreamer1-vaapi for Intel/AMD graphics).


### Starting and running UxPlay 

Since UxPlay-1.64, UxPlay can be started with options read from a configuration file, which will be the first found of
(1) a file with a path given by environment variable `$UXPLAYRC`, (2) ``~/.uxplayrc`` in the user's home
directory ("~"), (3) ``~/.config/uxplayrc``.  The format is one option per line, omitting the initial ``"-"`` of
the command-line option.   Lines in the configuration file beginning with `"#"` are treated as comments and ignored.

**Run uxplay in a terminal window**. On some systems, you can toggle into and out of fullscreen mode
with F11 or (held-down left Alt)+Enter keys.  Use Ctrl-C (or close the window)
to terminate it when done. If the UxPlay server is not seen by the
iOS client's drop-down "Screen Mirroring" panel, check that your DNS-SD
server (usually avahi-daemon) is running: do this in a terminal window
with ```systemctl status avahi-daemon```.
If this shows the avahi-daemon is not running, control it
with ```sudo systemctl [start,stop,enable,disable] avahi-daemon``` (on non-systemd systems, such as  \*BSD,
use ``sudo service avahi-daemon [status, start, stop, restart, ...]``). If UxPlay is
seen, but the client fails to connect
when it is selected, there may be a firewall on the server that prevents
UxPlay from receiving client connection requests unless some network ports
are opened: **if a firewall is active, also open UDP port 5353 (for mDNS queries)
needed by Avahi**. See [Troubleshooting](#troubleshooting) below for
help with this or other problems.

* By default, UxPlay is locked to
its current client until that client drops the connection; since UxPlay-1.58, the option `-nohold` modifies this
behavior so that when a new client requests a connection, it removes the current client and takes over.   UxPlay 1.66 introduces
a mechanism ( `-restrict`, ``-allow <id>``, ```-block <id>```) to control which clients are allowed to connect, using their
"deviceID" (which appears to be immutable).

* In Mirror mode, GStreamer has a choice of **two** methods to play video with its accompanying audio: prior to UxPlay-1.64,
the video and audio streams were both played as soon as possible after they arrived (the GStreamer "_sync=false_" method), with
a GStreamer internal clock used to try to keep them synchronized.    **Starting with UxPlay-1.64, the other method
(GStreamer's "_sync=true_" mode), which uses timestamps in the audio and video streams sent by the client, is the new default**.
On low-decoding-power UxPlay hosts (such as Raspberry Pi 3 models) this will drop video frames that cannot be decoded in time
to play with the audio, making the video jerky, but still synchronized.

The older method which does not drop late video frames
worked well on more powerful systems, and is still available with the  UxPlay option "`-vsync no`"; this method  is adapted
to "live streaming", and may be better when using UxPlay as a second monitor for a Mac computer, for example, while the new default
timestamp-based method is best for watching a video, to keep lip movements and voices synchronized.   (Without use of timestamps,
video will eventually lag behind audio if it cannot be decoded fast enough: hardware-accelerated video-decoding helped to prevent this
previously when timestamps were not being used.)



* In Audio-only mode the GStreamer "sync=false" mode (not using timestamps) is still the default, but if you want to keep the audio
playing on the server synchronized with the video showing on the client, use the `-async` timestamp-based option.  (An example might be
if you want to follow the Apple Music lyrics on the client while listening to superior sound on the UxPlay server).   This
delays the video on the client to match audio on the server, so leads to
a slight delay before a pause or track-change initiated on the client takes effect on the audio played by the server. 

The -vsync and -async options
also allow an optional positive (or negative) audio-delay adjustment in _milliseconds_ for fine-tuning : `-vsync 20.5`
delays audio relative to video by 0.0205 secs; a negative value advances it.)

* you may find video is improved by the setting -fps 60 that allows some video to be played at 60 frames
per second. (You can see what framerate is actually streaming by using -vs fpsdisplaysink, and/or -FPSdata.)
When using this, you should use the default timestamp-based synchronization option `-vsync`.

* Since UxPlay-1.54, you can display the accompanying "Cover Art" from sources like Apple Music in Audio-Only (ALAC) mode:
run "`uxplay -ca <name> &`" in the background, then run a image viewer with an autoreload feature: an example
is "feh": run "``feh -R 1 <name>``"
in the foreground; terminate feh and then Uxplay with "`ctrl-C fg ctrl-C`".

By default, GStreamer uses an algorithm to search for the best "videosink" (GStreamer's term for a graphics driver to display images) to use.
You can overide this with the uxplay option `-vs <videosink>`. Which videosinks are available depends on your operating system and
graphics hardware:  use "`gst-inspect-1.0 | grep sink | grep -e video -e Video -e image`" to see what is available.   Some possibilites on Linux/\*BSD are:

* **glimagesink** (OpenGL), **waylandsink**

* **xvimagesink**, **ximagesink** (X11)

* **kmssink**, **fbdevsink** (console graphics without X11)

* **vaapisink** (for Intel/AMD hardware-accelerated graphics); for NVIDIA hardware graphics (with CUDA) use **glimagesink** combined
  with "`-vd nvh264dec`" (or  "nvh264sldec", a new variant  which will become "nvh264dec" in GStreamer-1.24). 

GStreamer also searches for the best "audiosink"; override its choice  with `-as <audiosink>`. Choices on Linux include
pulsesink, alsasink, pipewiresink, oss4sink; see what is available with `gst-inspect-1.0 | grep sink | grep -e audio -e Audio`.

**One common problem involves GStreamer
attempting to use incorrectly-configured or absent accelerated hardware h264
video decoding (e.g., VAAPI).
Try "`uxplay -avdec`" to force software video decoding; if this works you can
then try to fix accelerated hardware video decoding if you need it, or just uninstall the GStreamer vaapi plugin.**

See [Usage](#usage) for more run-time options.


### **Special instructions for Raspberry Pi (tested on R Pi 4 model B 8GB and  R Pi 3 model B+)**:

* If you use the software-only (h264) video-decoding UxPlay option `-avdec`, it now works
better than earlier, with the new default timestamp-based synchronization to keep audio and video synchronized.

* For best performance, the Raspberry Pi needs the GStreamer Video4linux2 plugin  to use
its Broadcom GPU hardware for decoding h264 video.   This needs the bcm2835_codec kernel module
which is maintained outside the mainline Linux kernel by Raspberry Pi in the 
the [Raspberry Pi kernel tree](https://github.com/raspberrypi/linux), and the
only distributions for R Pi that are known to supply it include Raspberry Pi OS, Ubuntu, and Manjaro (all available
from Raspberry Pi with their Raspberry Pi Imager).  Other distributions generally do not
provide it: **without this kernel module, UxPlay cannot use the decoding firmware in the GPU.**

* On a  Raspberry Pi model 4B running the unsupported "Legacy" R Pi OS (Buster), note that 
this comes with a very old GStreamer-1.14.4 **that cannot be patched to access the  Broadcom GPU**.  If you need to stay on the unsupported
"Legacy" OS ("Lite" version), but want to use UxPlay with hardware video decoding,  you need
to first build a complete newer GStreamer from source
using [these instructions](https://github.com/FDH2/UxPlay/wiki/Building-latest-GStreamer-from-source-on-distributions-with-older-GStreamer-(e.g.-Raspberry-Pi-OS-).)
before building UxPlay. Note that a model 3B+ Pi running the Legacy OS can access the GPU with GStreamer-1.14's omx plugin (use option "`-vd omxh264dec`"), but
this plugin is broken on model 4B's firmware, and omx support was removed in  R Pi OS (Bullseye).


For use of the GPU, use raspi-config "Performance Options" (on Raspberry Pi OS, use a similar tool on other
distributions) to allocate sufficient memory for the GPU (on  R. Pi 3 model B+, the maximum (256MB) is suggested).
Even with GPU video decoding, some frames may be dropped by the lower-power 3 B+  to keep audio and video synchronized
using timestamps.

* The plugin in the latest GStreamer-1.22 release works well, but older releases of GStreamer will not
work unless patched with backports from GStreamer-1.22.  Raspberry Pi OS (Bullseye) now has a
working backport. For a fuller backport, or for other distributions, patches for the GStreamer Video4Linux2 plugin
are [available with instructions in the UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches).

The basic uxplay options for R Pi are ```uxplay [-vs <videosink>]```. The
choice `<videosink>` = ``glimagesink`` is sometimes useful. 
On a system without X11 (like R Pi OS Lite) with framebuffer video, use `<videosink>` = ``kmssink``.
With the  Wayland video compositor,  use `<videosink>` = ``waylandsink``.  When using the Video4Linux2
plugin to access hardware video decoding, an option `-v4l2` may be useful: for convenience, this also comes
combined with various videosink options as  `-rpi`, ``-rpigl`` ``-rpifb``, ```-rpiwl```, respectively
provided for X11, X11 with OpenGL, framebuffer, and Wayland systems.
**You may find that just "`uxplay`", (_without_ ``-v4l2`` or ```-rpi*``` options, which lets GStreamer
try to find the best video solution by itself)
provides the best results** (the `-rpi*` options may be removed in a future release of UxPlay.)

* If you are using Raspberry Pi OS (Bullseye) with Video4Linux2 from unpatched GStreamer-1.18.4, you
need the `-bt709` option with UxPlay-1.56 or later.
Also don't use options `-v4l2` and ``-rpi*`` with it, as they
cause a crash if the client screen is rotated.  (These issues  do not occur when the latest GStreamer-1.18.4
patch from the UxPlay Wiki has been applied.)

* Tip: to start UxPlay on a remote host (such as a Raspberry Pi) using ssh:

```
   ssh user@remote_host
   export DISPLAY=:0
   nohup uxplay [options] > FILE &
```
  Sound and video will play on the remote host; "nohup" will keep uxplay running if the ssh session is
  closed.  Terminal output is saved to FILE (which can be /dev/null to discard it)

## Building UxPlay on macOS:  **(Intel X86_64 and "Apple Silicon" M1/M2 Macs)**

_Note: A native AirPlay Server feature is included in  macOS 12 Monterey, but is restricted to recent hardware.
UxPlay can run  on older macOS systems that will not be able to run Monterey, or can run Monterey  but not AirPlay._

These instructions for macOS  assume that the Xcode command-line developer tools are installed (if Xcode is
installed, open the Terminal, type "sudo xcode-select --install" and accept the conditions).

It is also assumed that CMake >= 3.13 is installed:
this can be done with package managers [MacPorts](http://www.macports.org) (`sudo port install cmake`),
[Homebrew](http://brew.sh) (`brew install cmake`), or by a download from
[https://cmake.org/download/](https://cmake.org/download/).    Also install `git` if you will use it to fetch UxPlay.

Next install libplist and openssl-3.x.  Note that static versions of these libraries will be
used in the macOS builds, so they can be uninstalled after building uxplay, if you wish.

*  If you use Homebrew: `brew install libplist openssl@3`

* if you use MacPorts: `sudo port install libplist-devel openssl3`

Otherwise, build libplist and openssl from source: see instructions near the  end of this README;
requires development tools (autoconf, automake, libtool, _etc._) to be installed.


Next get the latest macOS release of GStreamer-1.0.

**Using "Official" GStreamer (Recommended for both MacPorts and Homebrew users)**: install
the GStreamer release for macOS 
from [https://gstreamer.freedesktop.org/download/](https://gstreamer.freedesktop.org/download/).
(This release contains its own pkg-config,
so you don't have to install one.) Install both the gstreamer-1.0 and gstreamer-1.0-devel packages.  After downloading, Shift-Click on them
to install (they install to /Library/FrameWorks/GStreamer.framework).   Homebrew or MacPorts users should **not** install (or should uninstall)
the GStreamer supplied by their package manager, if they use the "official" release.

* **ADDED 2023-01-25: in the current 1.22.x releases  something in the GStreamer macOS binaries appears to not be
working (UxPlay starts receiving the AirPlay stream, but the video window does not open)**.  If
you have this problem, use the older GStreamer-1.20.7 binary packages until a fix is found.  _You could instead
compile the "official" GStreamer-1.22.x release from source: GStreamer-1.22.0 has been successfully
built this way on a system using MacPorts: see_ [the UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki/Building-GStreamer-from-Source-on-macOS-with-MacPorts).

**Using Homebrew's GStreamer**:  pkg-config is needed:  ("brew install pkg-config gstreamer").
This causes a large number of extra packages to be installed by Homebrew as dependencies.
The [Homebrew gstreamer installation](https://formulae.brew.sh/formula/gstreamer#default) has recently been
reworked into a single "formula" named `gstreamer`, which now works without needing  GST_PLUGIN_PATH to be
set in the enviroment.  Homebrew installs gstreamer to `(HOMEBREW)/lib/gstreamer-1.0` where ``(HOMEBREW)/*`` is
`/opt/homebrew/*` on Apple Silicon Macs, and ``/usr/local/*`` on Intel Macs; do not put any
extra non-Homebrew plugins (that you build yourself) there, and instead set GST_PLUGIN_PATH to point to
their location (Homebrew does not supply a complete GStreamer, but seems to have everything needed for UxPlay).


Finally, build and install uxplay: open a terminal and change into the UxPlay source directory
("UxPlay-master" for zipfile downloads, "UxPlay" for "git clone" downloads) and build/install with
"cmake . ; make ; sudo make install " (same as for Linux).

   * Running UxPlay while checking for GStreamer warnings (do this with "export GST_DEBUG=2" before runnng UxPlay) reveals
     that with the default (since UxPlay 1.64) use of timestamps for video synchonization, many video frames are being dropped
     (only on macOS), perhaps due to another error (about videometa) that shows up in the GStreamer warnings.   **Recommendation:
     use the new UxPlay "no timestamp" option "`-vsync no`"** (you can add a line "vsync no" in the uxplayrc configuration file).

   * On macOS with this installation of GStreamer, the only videosinks available seem to be glimagesink (default choice made by
     autovideosink) and osxvideosink.   The window title does not show the Airplay server name, but the window is visible to
     screen-sharing apps (e.g., Zoom). The only available audiosink seems to be osxaudiosink.

   * The option -nc is always used, whether or not it is selected.
     This is a workaround for a problem with GStreamer videosinks on macOS:
     if the GStreamer pipeline is destroyed while the mirror window is still open,  a segfault occurs.
   
   * In the case of glimagesink, the resolution settings "-s wxh" do not affect
     the (small) initial OpenGL mirror window size, but the window can be expanded using the mouse or trackpad.
     In contrast, a window created with "-vs osxvideosink" is initially big, but has the wrong aspect ratio (stretched image);
     in this case the aspect ratio changes when the window width is changed by dragging its side;
     the option "-vs osxvideosink force-aspect-ratio=true" can be used to make the window have the
     correct aspect ratio when it first opens.


***Using GStreamer installed from MacPorts (not recommended):***

To install: "sudo port install pkgconf"; "sudo port install gstreamer1-gst-plugins-base gstreamer1-gst-plugins-good gstreamer1-gst-plugins-bad gstreamer1-gst-libav".
**The MacPorts GStreamer is old (v1.16.2) and built to use X11**: use the special CMake option `-DUSE_X11=ON`
when building UxPlay. Then uxplay must be run from an XQuartz terminal, and needs
option "-vs ximagesink".  On a unibody (non-retina)  MacBook Pro, the default resolution  wxh = 1920x1080
was too large, but using option "-s 800x600" worked.  The MacPorts GStreamer pipeline
seems fragile against attempts to change the X11 window size, or to rotations that switch a connected client between portrait and landscape mode while uxplay is running. Using the MacPorts X11 GStreamer seems only possible if the image size is left unchanged from the initial "-s wxh" setting (also use the iPad/iPhone setting that locks the screen orientation against switching  between portrait and landscape mode as the device is rotated).

## Building UxPlay on Microsoft Windows, using MSYS2 with the MinGW-64 compiler.

* tested on Windows 10 and 11, 64-bit.

1. Download and install  **Bonjour SDK for Windows v3.0**.  You can download the SDK without any registration
   at [softpedia.com](https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml), or get it from the official Apple
   site [https://developer.apple.com/download](https://developer.apple.com/download/all/?q=Bonjour%20SDK%20for%20Windows) (Apple makes
   you register as a developer to access it from their site).  This should install the Bonjour SDK as `C:\Program Files\Bonjour SDK`.

2. (This is for 64-bit Windows; a build for 32-bit Windows should be possible, but is not tested.) The
   unix-like MSYS2 build environment will be used: download and install MSYS2 from the official
   site [https://www.msys2.org/](https://www.msys2.org).  Accept the default installation location `C:\mysys64`.

3. [MSYS2 packages](https://packages.msys2.org/package/) are installed with a
   variant of the "pacman" package manager used by Arch Linux.  Open a "MSYS2 MINGW64" terminal
   from the MSYS2 tab in the Windows Start menu, and update the new
   MSYS2 installation with "pacman -Syu".  Then install the  **MinGW-64** compiler  and   **cmake**
      
   ```
   pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc
   ```

   The compiler with all required dependencies will be installed in the msys64 directory, with
   default path `C:/msys64/mingw64`.  Here we will simply build UxPlay  from the command line
   in the MSYS2 environment (this uses "`ninja`" in place of "``make``" for   the build system).

4. Download the latest UxPlay from github **(to use `git`, install it with ``pacman -S git``, 
   then "`git clone https://github.com/FDH2/UxPlay`")**, then install UxPlay dependencies (openssl is already
   installed with MSYS2):

   `pacman -S mingw-w64-x86_64-libplist mingw-w64-x86_64-gstreamer mingw-w64-x86_64-gst-plugins-base`

    Note that libplist will be linked statically to the uxplay executable.
    If you are trying a different Windows build system, MSVC versions of GStreamer
    for Windows are available from the [official GStreamer site](https://gstreamer.freedesktop.org/download/),
    but only the MinGW 64-bit build on MSYS2 has been tested.
    
5.  cd to the UxPlay source directory, then "`mkdir build`" and  "``cd build``".  The build process assumes that
    the Bonjour SDK is installed at `C:\Program Files\Bonjour SDK`.   If it is somewhere else, set the enviroment
    variable BONJOUR_SDK_HOME to point to its location.   Then build UxPlay with

     `cmake ..`

     `ninja`

6.  Assuming no error in either of these, you will have built the uxplay executable **uxplay.exe** in the
    current ("build")  directory. The "sudo make install" and "sudo make uninstall" features offered in the
    other builds are not available on Windows; instead, the MSYS2 environment has 
    `/mingw64/...` available, and you can install the uxplay.exe executable 
    in `C:/msys64/mingw64/bin` (plus manpage and documentation in ``C:/msys64/mingw64/share/...``) with

    `cmake --install . --prefix /mingw64`
    
    To be able to view the manpage, you need to install the manpage viewer with "`pacman -S man`".

To run **uxplay.exe** you need to install some gstreamer plugin packages
with `pacman -S mingw-w64-x86_64-gst-<plugin>`, where the required ones  have ``<plugin>`` given by

   1. **libav**
   2. **plugins-good**
   3. **plugins-bad**
  
Other possible MSYS2 gstreamer plugin packages you might use are listed
in [MSYS2 packages](https://packages.msys2.org/package/).
   
You also will need to grant permission to the uxplay executable uxplay.exe to access data through the Windows 
firewall.  You may automatically be offered the choice to do this when you first run uxplay, or you may need to do it 
using **Windows Settings->Update and Security->Windows Security->Firewall & network protection -> allow an app
through firewall**. If your virus protection flags uxplay.exe as "suspicious" (but without a true malware signature)
you may need to give it an exception.
 
Now test by running "`uxplay`" (in a MSYS2 terminal window).  If you
need to specify the audiosink, there are two main choices on Windows: the older DirectSound
plugin "`-as directsoundsink`", and the more modern Windows Audio Session API  (wasapi)
plugin "`-as wasapisink`", which supports [additional options](https://gstreamer.freedesktop.org/documentation/wasapi/wasapisink.html) such as

```
uxplay -as 'wasapisink device=\"<guid>\"' 
```
where `<guid>` specifies an available audio device by its GUID, which can be found using
"`gst-device-monitor-1.0 Audio`": ``<guid>`` has a form
like ```\{0.0.0.00000000\}.\{98e35b2b-8eba-412e-b840-fd2c2492cf44\}```. If "`device`" is not specified, the
default audio device is used.

If you wish to specify the videosink using the `-vs <videosink>` option, some choices for `<videosink>` are
`d3d11videosink`, ``d3dvideosink``, ```glimagesink```,
`gtksink`.   With Direct3D 11.0 or greater, you can get the ability to toggle into and out of fullscreen mode using the Alt-Enter key combination with
option `-vs "d3d11videosink fullscreen-toggle-mode=alt-enter"`.  For convenience, this option will be added if just ``-vs d3d11videosink`` (by itself) is used.
(You may wish to add "``vs d3d11videosink``" (no initial "`-`") to the UxPlay startup options file; see "man uxplay" or "uxplay -h".)

The executable uxplay.exe can also be run without the MSYS2 environment, in
the Windows Terminal, with `C:\msys64\mingw64\bin\uxplay`.

# Usage

Options:

* These can also be written (one option per line, without the initial "`-`" character) in the UxPlay startup file (either given by
environment variable `$UXPLAYRC`, or ``~/.uxplayrc`` or ```~/.config/uxplayrc```); lines begining
with "`#`" are treated as comments, and ignored.  Command line options supersede  options in the startup file.

**-n server_name** (Default: UxPlay);  server_name@_hostname_ will be the name that appears offering
   AirPlay services to your iPad, iPhone etc, where _hostname_ is the name of the server running uxplay. 
   This will also now be the name shown above the mirror display (X11)  window.

**-nh** Do not append "@_hostname_" at the end of the AirPlay server name.

**-pair** set "Features" bit 27 to reactivate client-pairing support using "legacy pairing protocol" (the default
   since v1.65 is that bit 27 is not set, and legacy client-pairing setup is skipped).

**-vsync [x]**  (In Mirror mode:) this option (**now the default**) uses timestamps to synchronize audio with video on the server,
   with an optional audio delay in (decimal) milliseconds   (_x_ = "20.5"   means 0.0205 seconds delay: positive or
   negative delays less than a second are allowed.)   It is needed on low-power systems such as Raspberry Pi without hardware
   video decoding.

**-vsync no** (In Mirror mode:) this switches off timestamp-based audio-video synchronization, restoring the default behavior prior to
UxPlay-1.64. Standard  desktop systems seem to work well without use of timestamps: this mode is  appropriate for "live streaming" such as
using UxPlay as a second monitor for a mac computer, or monitoring a webcam; with it, no video frames are dropped.

**-async [x]** (In Audio-Only (ALAC) mode:) this option uses timestamps to synchronize audio on the server with video on the client,
   with an optional audio delay in (decimal) milliseconds   (_x_ = "20.5"   means 0.0205 seconds delay: positive or
   negative delays less than a second are allowed.)   Because the client adds a video 
   delay to account for latency, the server in -async mode  adds an equivalent audio delay, which means that
   audio changes such as a pause or a track-change will not take effect
   immediately.  _This might in principle be mitigated by using the `-al` audio latency setting to change the latency (default 0.25 secs)
   that the server reports to the client, but at present changing this does not seem to have any effect_.

**-async no**.   This is the still the default behavior in Audio-only mode, but this option may be useful as a command-line option to switch off a
`-async` option set in a "uxplayrc" configuration file.

**-s wxh** (e.g. -s 1920x1080 , which is the default ) sets the display resolution (width and height,
   in pixels).   (This may be a
   request made to the AirPlay client, and perhaps will not
   be the final resolution you get.)   w and h are whole numbers with four
   digits or less.   Note that the **height** pixel size is the controlling
   one used by the client for determining the streaming format; the width is
   dynamically adjusted to the shape of the image (portrait or landscape
   format, depending on how an iPad is held, for example). 

**-s wxh@r**  As above, but also informs the AirPlay  client about the screen
   refresh rate of the display. Default is r=60 (60 Hz); r must be a whole number
   less than 256.

**-o** turns on an "overscanned" option for the display window.    This
   reduces the image resolution by using some of the pixels requested
   by  option -s wxh (or their default values 1920x1080) by adding an empty
   boundary frame of unused pixels (which would be lost in a full-screen
   display that overscans, and is not displayed by gstreamer).
   Recommendation: **don't use this option** unless there is some special
   reason to use it.

**-fs** uses fullscreen mode, but only works with X11, Wayland or VAAPI.

**-p**  allows you to select the network ports used by UxPlay (these need
   to be opened if the server is behind a firewall).   By itself, -p sets
   "legacy" ports TCP 7100, 7000, 7001, UDP 6000, 6001, 7011.   -p n (e.g. -p
   35000)  sets TCP and UDP ports n, n+1, n+2.  -p n1,n2,n3 (comma-separated
   values) sets each port separately; -p n1,n2 sets ports n1,n2,n2+1.  -p tcp n
   or -p udp n sets just the TCP or UDP ports.  Ports must be in the range
   [1024-65535].

If the -p option is not used, the ports are chosen dynamically (randomly),
which will not work if a firewall is running.

**-avdec** forces use of software h264 decoding using Gstreamer element avdec_h264 (libav h264 decoder). This
   option should prevent autovideosink choosing a hardware-accelerated videosink plugin such as vaapisink.
   
**-vp _parser_** choses the GStreamer pipeline's h264 parser element, default is h264parse. Using
   quotes "..." allows options to be added.
   
**-vd _decoder_** chooses the GStreamer pipeline's h264 decoder element, instead of the default value
   "decodebin" which chooses it for you.  Software decoding is done by avdec_h264; various hardware decoders
   include: vaapih264dec, nvdec, nvh264dec, v4l2h264dec (these require that the appropriate hardware is
   available).  Using quotes "..." allows some parameters to be included with the decoder name.

**-vc _converter_** chooses the GStreamer pipeline's videoconverter element, instead of the default
   value "videoconvert".  When using Video4Linux2 hardware-decoding by a GPU,`-vc  v4l2convert` will also use
   the GPU for video conversion.  Using quotes "..." allows some parameters to be included with the converter name.
   
**-vs _videosink_** chooses the GStreamer videosink, instead of the default value
   "autovideosink" which chooses it for you.  Some videosink choices are:  ximagesink, xvimagesink,
   vaapisink (for intel graphics), gtksink, glimagesink, waylandsink, osxvideosink (for macOS),  kmssink (for
   systems without X11, like Raspberry Pi OS lite) or
   fpsdisplaysink (which shows the streaming framerate in fps).   Using quotes
   "..." allows some parameters to be included with the videosink name. 
   For example, **fullscreen** mode is supported by the vaapisink plugin, and is
   obtained using ``-vs "vaapisink fullscreen=true"``; this also works with ``waylandsink``.
   The syntax of such options is specific to a given plugin, and some choices of videosink
   might not work on your system.

**-vs 0** suppresses display of streamed video, but plays  streamed audio.   (The client's screen
   is still mirrored at a reduced rate of 1 frame per second,  but is not rendered or displayed.)  This
   feature (which streams audio in AAC audio format) is now probably unneeded, as UxPlay can now 
   stream superior-quality Apple Lossless audio without video in Airplay non-mirror mode.

**-v4l2** Video settings for hardware h264 video decoding in the GPU by Video4Linux2.  Equivalent to
   `-vd v4l2h264dec -vc v4l2convert`.

**-bt709**  A workaround for the failure of the older  Video4Linux2 plugin to recognize Apple's
   use of an uncommon (but permitted) "full-range color" variant of the bt709 color standard for digital TV.
   This is no longer needed by GStreamer-1.20.4 and backports from it.

**-rpi**  Equivalent to  "-v4l2 ".   Use for "Desktop" Raspberry Pi systems with X11.

**-rpigl**  Equivalent to  "-rpi -vs glimagesink".  Sometimes better for "Desktop" Raspberry Pi systems with X11.

**-rpifb** Equivalent to "-rpi -vs kmssink" (use for Raspberry Pi systems
   using the framebuffer, like RPi OS Bullseye Lite).

**-rpiwl** Equivalent to "-rpi -vs waylandsink", for Raspberry
   Pi "Desktop" systems using the Wayland video compositor (use for
   Ubuntu 21.10 for Raspberry Pi 4B).

**-as _audiosink_** chooses the GStreamer audiosink, instead of letting
   autoaudiosink pick it for you.  Some audiosink choices are:  pulsesink, alsasink, pipewiresink, 
   osssink, oss4sink, jackaudiosink, osxaudiosink (for macOS), wasapisink, directsoundsink (for Windows).  Using quotes
   "..." might allow some parameters to be included with the audiosink name. 
   (Some choices of audiosink might not work on your system.)   

**-as 0**  (or just **-a**) suppresses playing of streamed audio, but displays streamed video.

**-al _x_** specifies an audio latency _x_ in (decimal) seconds in Audio-only (ALAC), that is reported to the client.  Values
   in the range [0.0, 10.0] seconds are allowed, and will be converted to a whole number of microseconds.  Default
   is 0.25 sec (250000 usec).   _(However, the client appears to ignore this reported latency, so this option seems non-functional.)_

**-ca _filename_** provides a file (where _filename_ can include a full path) used for output of "cover art"
   (from Apple Music, _etc._,) in audio-only ALAC mode.   This file is overwritten with the latest cover art as
   it arrives.   Cover art (jpeg format) is discarded if this option is not used.    Use with a image viewer that reloads the image
   if it changes, or regularly (_e.g._ once per second.).    To achieve this,  run "`uxplay -ca [path/to/]filename &`" in the background,
   then run the the image viewer in the foreground.   Example, using `feh` as the viewer: run "``feh -R 1 [path/to/]filename``" (in
   the same terminal window in which uxplay was put into the background).   To quit, use ```ctrl-C fg ctrl-C``` to terminate
   the image viewer, bring ``uxplay`` into the foreground, and terminate it too.

**-reset n** sets a limit of _n_ consecutive timeout failures of the client to respond to ntp requests
   from the server (these are sent every 3 seconds to check if the client is still present, and synchronize with it).   After
   _n_ failures, the client will be presumed to be offline, and the connection will be reset to allow a new
   connection.   The default value of _n_ is 5; the value _n_ = 0 means "no limit" on timeouts.

**-nc** maintains previous UxPlay < 1.45 behavior that does **not close** the video window when the the client
   sends the "Stop Mirroring" signal. _This option is currently used by default in macOS,
   as the  window created in macOS by GStreamer does not terminate correctly (it causes a segfault)
   if it is still open when the GStreamer pipeline is closed._

**-nohold**  Drops the current connection when a new client attempts to connect.  Without this option,
   the current client maintains exclusive ownership of UxPlay until it disconnects.

**-restrict** Restrict clients allowed to connect to those specified by `-allow <deviceID>`.  The deviceID has the
    form of a MAC address which is displayed by UxPlay when the client attempts to connect, and appears to be immutable.   It
    has the format `XX:XX:XX:XX:XX:XX`, X = 0-9,A-F, and is possibly the "true" hardware
    MAC address of the device.  Note that iOS clients generally expose different random "private Wi_Fi addresses" ("fake" MAC addresses) to 
    different networks (for privacy reasons, to prevent tracking), which may change, and do not correpond to the deviceID.

**-restrict no**  Remove restrictions (default).  This is useful as a command-line argument to overide restrictions set
   in the Startup file.
   
**-allow _id_** Adds the deviceID = _id_ to the list of allowed clients when client restrictions
   are being enforced.  Usually this will be an entry in the uxplayrc startup file.

**-block _id_** Always block clients with deviceID = _id_, even when client restrictions are not
   being enforced generally. Usually this will be an entry in the uxplayrc startup file.

**-FPSdata** Turns on monitoring of regular reports about video streaming performance
   that are sent by the client.  These will be displayed in the terminal window if this
   option is used.   The data is updated by the client at 1 second intervals.

**-fps n** sets a maximum frame rate (in frames per second) for the AirPlay
   client to stream video; n must be a whole number less than 256.
   (The client may choose to serve video at any frame rate lower
   than this;  default is 30 fps.)  A setting of 60 fps may give you improved video
   but is not recommended on Raspberry Pi.   A setting below 30 fps might be useful to
   reduce latency if you are running more than one instance of uxplay at the same time.
   _This setting is only an advisory to
   the client device, so setting a high value will not force a high framerate._
   (You can test using "-vs fpsdisplaysink" to see what framerate is being
   received, or use the option -FPSdata which displays video-stream performance data
   continuously sent by the client during video-streaming.)

**-f {H|V|I}**  implements "videoflip" image transforms: H = horizontal flip
   (right-left flip, or mirror image); V = vertical flip ;  I =
   180 degree rotation or inversion (which is the combination of H with V).

**-r {R|L}**  90 degree Right (clockwise) or Left (counter-clockwise)
   rotations; these image transforms are carried out after any **-f** transforms.

**-m**  generates a random MAC address to use instead of the true hardware MAC
   number of the computer's network card.   (Different server_name,  MAC
   addresses,  and network ports are needed for each running uxplay  if you
   attempt to  run two instances of uxplay on the same computer.)
   If UxPlay fails to find the true MAC address of a  network card, (more
   specifically, the MAC address used by the first active network interface detected)
   a random MAC address will be used even if option **-m** was not specified.
   (Note that a random MAC address will be different each time UxPlay is started).

**-t _timeout_** [This option was removed in UxPlay v.1.61.]

**-vdmp** Dumps h264 video to file videodump.h264.  -vdmp n dumps not more than n NAL units to
   videodump.x.h264; x= 1,2,... increases each time a SPS/PPS NAL unit arrives.   To change the name
   _videodump_,  use -vdmp [n] _filename_.

**-admp** Dumps  audio to file audiodump.x.aac (AAC-ELD format audio), audiodump.x.alac (ALAC format audio) or audiodump.x.aud
   (other-format audio), where x = 1,2,3... increases each time the audio format changes. -admp _n_ restricts the number of
   packets dumped to a file to _n_ or less.    To change the name _audiodump_, use -admp [n] _filename_.   _Note that (unlike dumped video)
   the dumped audio is currently only useful for debugging, as it is not containerized to make it playable with standard audio players._ 

**-d**  Enable debug output.   Note:  this does not show GStreamer error or debug messages.   To see GStreamer error
    and warning messages, set the environment variable GST_DEBUG with "export GST_DEBUG=2" before running uxplay.
    To see GStreamer information messages, set GST_DEBUG=4; for DEBUG messages, GST_DEBUG=5; increase this to see even
    more of the GStreamer inner workings.

# Troubleshooting

Note: ```uxplay```  is run from a terminal command line, and informational messages are written to the terminal.

### 0.  Problems in compiling UxPlay.
    
One user (on Ubuntu) found compilation failed with messages about linking to "usr/local/lib/libcrypto.a"  and "zlib".
This was because (in addition to the standard ubuntu installation of libssl-dev), the user was unaware that a second installation
with libcrypto in /usr/local was present.
Solution: when more than one installation of OpenSSL is present, set the environment variable OPEN_SSL_ROOT_DIR to point to the correct one;
on 64-bit Ubuntu, this is done by
running `export OPENSSL_ROOT_DIR=/usr/lib/X86_64-linux-gnu/` before running cmake.

### 1. **Avahi/DNS_SD Bonjour/Zeroconf issues**

The DNS_SD Service-Discovery ("Bonjour" or "Zeroconf") service is required for UxPlay to work.  On Linux, it will be usually provided by Avahi, and to troubleshoot this, you 
should use the tool `avahi-browse`.  (You may need to install a separate package with a name like ``avahi-utils`` to get this.)

On Linux, make sure Avahi is installed,
and start the avahi-daemon service on the system running uxplay (your distribution will document how to do this, for example:
`sudo systemctl <cmd> avahi-daemon` or ``sudo service avahi-daemon <cmd>``, with
``<cmd>`` one of enable, disable, start, stop, status.
You might need to edit the avahi-daemon.conf file (it is typically in /etc/avahi/, find it with "`sudo find /etc -name avahi-daemon.conf`"):
make sure that "disable-publishing" is **not** a selected option).
Some  systems  may instead use the mdnsd daemon as an alternative to provide DNS-SD service.
(FreeBSD offers both alternatives, but only Avahi was tested; see [here](https://gist.github.com/reidransom/6033227).)

* **uxplay starts, but either stalls or stops after "Initialized server socket(s)" appears (_without the server name showing on the client_)**.

If UxPlay stops with the "No DNS-SD Server found" message, this  means that your network **does not have a running Bonjour/zeroconf DNS-SD server.**
Before v1.60, UxPlay used to stall silently if DNS-SD service registration failed, but now stops with an error message returned by the
DNSServiceRegister function: kDNSServiceErr_Unknown if no DNS-SD server was found:
other mDNS error codes are in the range FFFE FF00 (-65792) to FFFE FFFF (-65537), and are listed in the
dnssd.h file.  An older version of this (the one used by avahi) is found [here](https://github.com/lathiat/avahi/blob/master/avahi-compat-libdns_sd/dns_sd.h).
A few additional error codes are defined in a later version
from [Apple](https://opensource.apple.com/source/mDNSResponder/mDNSResponder-544/mDNSShared/dns_sd.h.auto.html).   


If UxPlay stalls _without an error message_ and _without the server name showing on the client_, this is either pre-UxPlay-1.60
behavior when no DNS-SD server was found, or a network problem.

*  **Avahi works at first, but new clients do not see UxPlay, or clients that initially saw it stop doing so after they disconnect**.

This is usually because Avahi is only using the "loopback" network interface, and is not receiving mDNS queries from new clients that were not
listening when UxPlay started.

To check this, after starting uxplay, use the utility ``avahi-browse -a -t`` **in a different terminal window** on the server to
verify that the UxPlay AirTunes and AirPlay services are correctly registered (only the AirTunes service is
used in the "Legacy" AirPlay Mirror mode  used by UxPlay, but the AirPlay service is used for the initial contact).

The results returned by avahi-browse should show entries for
uxplay like


```
+   eno1 IPv6 UxPlay                                        AirPlay Remote Video local
+   eno1 IPv4 UxPlay                                        AirPlay Remote Video local
+     lo IPv4 UxPlay                                        AirPlay Remote Video local
+   eno1 IPv6 863EA27598FE@UxPlay                           AirTunes Remote Audio local
+   eno1 IPv4 863EA27598FE@UxPlay                           AirTunes Remote Audio local
+     lo IPv4 863EA27598FE@UxPlay                           AirTunes Remote Audio local

```
If only the loopback ("lo") entries are shown, a firewall on the UxPlay host
is probably blocking full DNS-SD service, and  you need to open the default UDP port 5353 for mDNS requests,
as loopback-based DNS-SD service is unreliable.

If the UxPlay services are listed by avahi-browse as above, but are not seen by the client,
the problem is likely to be a problem with the local network.


### 2. uxplay starts, but stalls after "Initialized server socket(s)" appears, *with the server name showing on the client* (but the client fails to connect when the UxPlay server is selected).

This shows that a *DNS-SD* service  is working, clients hear UxPlay is available, but the UxPlay server is not receiving the response from the client.
This is usually because 
a firewall on the server is blocking the connection request from the client.
(One user who insisted that the firewall had been turned off turned out to have had _two_ active firewalls (*firewalld* and *ufw*)
_both_ running on the server!)  If possible, either turn off the firewall
to see if that is the problem, or get three consecutive network ports,
starting at port n, all three in the range 1024-65535, opened  for both tcp and udp, and use "uxplay -p n"
(or open UDP 7011,6001,6000 TCP 7100,7000,7001 and use "uxplay -p").

If you are _really_ sure there is no firewall, you may need to investigate your network transmissions with a tool like netstat, but almost always this
is a firewall issue.

### 3. Problems _after_ the client-server connection has been made:

If you do _not_ see the message ``raop_rtp_mirror starting mirroring``, something went wrong before the client-server negotiations were finished.
For such  problems, use "uxplay -d " (debug log option)  to see what is happening: it will show how far the connection process gets before
the failure occurs.    You can compare your debug  output to
that from a successful start of UxPlay in the [UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki).

**If UxPlay reports that mirroring started, but you get no video or audio, the problem is probably  from  a
GStreamer plugin that doesn't work on your system** (by default,
GStreamer uses the "autovideosink"  and  "autoaudiosink" algorithms 
to guess what are  the "best" plugins to use on your system).
A different reason for no audio occurred when  a user with a firewall only opened two udp network
ports: **three** are required (the third one receives the audio data).

**Raspberry Pi** devices work best with hardware GPU h264 video decoding if the Video4Linux2 plugin in  GStreamer v1.20.x or earlier has
been patched (see the UxPlay [Wiki](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches) for patches).
This is fixed in  GStreamer-1.22, and by backport patches from this in distributions such as Raspberry Pi OS (Bullseye): **use option `-bt709`
with the GStreamer-1.18.4 from Raspberry Pi OS**.
This also needs the bcm2835-codec kernel module that is not in the standard Linux kernel (it is available in Raspberry Pi OS, Ubuntu and Manjaro).

* **If this kernel module is not available in your Raspberry Pi operating system, or if GStreamer < 1.22 is not patched, use option `-avdec`
for software h264-decoding.**

Sometimes "autovideosink" may select the OpenGL renderer "glimagesink" which
may not work correctly on your system.   Try the options "-vs ximagesink" or
"-vs xvimagesink" to see if using one of these fixes the problem.

Other reported problems are connected to the GStreamer VAAPI plugin
(for hardware-accelerated Intel graphics, but not NVIDIA graphics).
Use the option "-avdec"
to force software h264 video decoding: this should prevent autovideosink from selecting the vaapisink videosink.
Alternatively, find out if the
gstreamer1.0-vaapi plugin is installed, and if  so, uninstall it.
(If this does not fix the problem, you can reinstall it.) 

There are some reports of other GStreamer problems with hardware-accelerated Intel HD graphics.  One user
(on Debian) solved this with "sudo apt install intel-media-va-driver-non-free".  This is a driver for 8'th (or later) generation
"*-lake" Intel chips, that seems to be related to VAAPI accelerated graphics.

If you _do_ have Intel HD graphics, and have installed the vaapi plugin, but ``-vs vaapisink`` does not work, check that vaapi is not "blacklisted" in your GStreamer installation: run ``gst-inspect-1.0 vaapi``, if this reports ``0 features``, you need to ``export GST_VAAPI_ALL_DRIVERS=1`` before running uxplay, or set this in the default environment. 

You can try to fix audio or video problems by using the "`-as <audiosink>`" or "``-vs <videosink>``"  options to choose the GStreamer audiosink or videosink , rather than
letting GStreamer choose one for you. (See above, in [Starting and running UxPlay](#starting-and-running-uxplay) for choices of  `<audiosink>` or ``<videosink>``.) 

The "OpenGL renderer" window created on Linux by "-vs glimagesink" sometimes does not close properly when its "close" button is clicked.
(this is a GStreamer issue).  You may need to terminate uxplay with Ctrl-C to close a "zombie" OpenGl window.   If similar problems happen when 
the client sends the "Stop Mirroring" signal, try the no-close option "-nc" that leaves the video window open.

### 4. GStreamer issues (missing plugins, etc.): 

* clearing the user's GStreamer cache with  `rm -rf ~/.cache/gstreamer-1.0/*`  may be the solution to problems
 where gst-inspect-1.0 does not show a plugin that you believe is installed.   The cache will be regenerated next time
GStreamer is started.  **This is the solution to puzzling problems that turn out to come from corruption of  the cache, and should be tried first.**

If UxPlay fails to start, with a message that  a required GStreamer plugin (such as "libav") was not found, first check with the GStreamer tool
gst-inspect-1.0 to see what GStreamer knows is available.  (You may need to install some additional GStreamer "tools" package to get gst-inspect-1.0).
For, _e.g._  a libav problem, check with "`gst-inspect-1.0 libav`".  If it is not shown as available to GStreamer, but your package manager 
shows the relevant package as installed (as one user found), try entirely removing and reinstalling the package.
That user found that a solution to a "**Required gstreamer plugin 'libav' not found**" message that kept recurring was to clear the user's gstreamer 
cache.
 
If it fails to start with an error like '`no element "avdec_aac"`' this is 
because even though gstreamer-libav is installed. it is incomplete because some plugin features are missing: "`gst-inspect-1.0 | grep avdec_aac`" will 
show if avdec_aac is available.   Unlike other GStreamer plugins, the libav plugin is a front end to FFmpeg codecs which provide avdec_*.
Some distributions (RedHat, SUSE, etc) provide incomplete versions of FFmpeg because of patent issues with codecs used by
certain plugins. In those cases there will be some "extra  package" provider  like [RPM fusion](https://rpmfusion.org) (RedHat),
[packman](http://packman.links2linux.org/) (SUSE) where you  can get complete packages (your 
distribution will usually provide  instructions for this, Mageia puts them in an optional "tainted" repo). The packages 
needed may be "ffmpeg\*" or "libav\*" packages: the GStreamer libav plugin package does not contain any codecs itself, it just provides a way 
for GStreamer to use ffmpeg/libav codec libraries which must be installed separately.  For similar reasons, distributions may ship incomplete packages 
of GStreamer "plugins-bad".

* starting with release UxPlay-1.65.3, UxPlay will continue to function, but without audio in mirror mode, if avdec_aac is missing.

To troubleshoot GStreamer execute  "export GST_DEBUG=2"
to set the GStreamer debug-level environment-variable in the terminal
where you will run uxplay, so that you see warning and error messages;
see [GStreamer debugging tools](https://gstreamer.freedesktop.org/documentation/tutorials/basic/debugging-tools.html)
for how to see much more of what is happening inside
GStreamer.   Run "gst-inspect-1.0" to see which GStreamer plugins are
installed on your system.

Some extra GStreamer packages for special plugins may need to be installed (or reinstalled: a user using a Wayland display system as an alternative to X11
reported that after reinstalling Lubuntu 18.4, UxPlay would not  work until gstreamer1.0-x was installed, presumably for Wayland's X11-compatibility mode).
Different distributions may break up GStreamer 1.x into packages in different ways; the packages listed above in the build instructions should bring in 
other required GStreamer packages as dependencies, but will not install all possible plugins.

The GStreamer video pipeline, which is shown in the initial output from `uxplay -d`,
has the default form

```
appsrc name=video_source ! queue ! h264parse ! decodebin ! videoconvert ! autovideosink name=video_sink sync=false
```

The pipeline is fully configurable: default  elements "h264parse", "decodebin", "videoconvert", and "autovideosink" can respectively be replaced by using  uxplay
options `-vp`, ``-vd``, ```-vc```, and ````-vs````, if there is any need to
modify it (entries can be given in quotes "..." to include options).

### 5.  Mirror screen freezes (a network problem):

This can happen if the  TCP video stream from the client stops arriving at the server, probably because of network problems  (the UDP audio stream may continue to arrive).   At 3-second 
intervals, UxPlay checks that the client is still connected by sending it a request for a NTP time signal.   If a reply is not received from the client within a 0.3 sec 
time-window, an "ntp timeout" is registered.    If a certain number (currently 5) of consecutive ntp timeouts occur, UxPlay assumes that the client is "dead", and resets the connection, 
becoming available for connection to a new client, or reconnection to the previous one.   Sometimes the connection may recover before the timeout limit is reached, and if the
default limit is not right  for your network,  it can be modified using the option "-reset _n_", where _n_ is the desired timeout-limit value (_n_ = 0 means "no limit").    If the connection 
starts to recover after ntp timeouts, a corrupt video packet from before the timeout may trigger a "connection reset by peer"  error, which also causes UxPlay to reset the
connection. 

* When the connection is reset, the "frozen" mirror screen of the previous connection is left in place, but does **not** block 
new connections, and will be taken over by a new client connection when it is made.

### 6. Protocol issues (with decryption of the encrypted audio and video streams sent by the client).

A protocol failure may trigger an unending stream of error messages, and means that the
audio decryption key (also used in video decryption) 
was not correctly extracted from data sent by the  client.

The default protocol was modifed in UxPlay-1.65 after it was discovered that the client-server "pairing" step could be avoided (leading to a much quicker
connection setup, without a 5 second delay) by disabling "Supports Legacy Pairing" (bit 27) in the "features" code UxPlay advertises 
on DNS-SD Service Discovery.  Most clients will then not attempt the setup of a  "shared secret key" when pairing, which is used by AppleTV for simultaneous
handling of multiple clients (UxPlay only supports one client at a time).
**This change is now well-tested, but in case it causes any protocol failures, UxPlay can be reverted to the previous behavior by using the option `-pair`
(option added in UxPlay 1.67).**


Protocol failure should not happen for iOS 9.3 or later clients.  However, if a client 
uses the same  older version of the protocol that is used by the Windows-based
AirPlay client emulator _AirMyPC_, the protocol can be switched to the older version
by the setting ```OLD_PROTOCOL_CLIENT_USER_AGENT_LIST``` 
in `UxPlay/lib/global.h`.
UxPlay reports the  client's "User Agent" string when it connects. If 
some other client also fails to decrypt all audio and video, try adding 
its "User Agent" string in place of "xxx" in the  entry "AirMyPC/2.0;xxx" 
in global.h and rebuild uxplay.

Note that for DNS-SD Service Discovery, Uxplay declares itself to be an AppleTV3,2 (a 32 bit device) with a 
sourceVersion 220.68; this can also be changed in global.h.
UxPlay also works if it declares itself as an AppleTV6,2 with
sourceVersion 380.20.1 (an AppleTV 4K 1st gen, introduced 2017, running
tvOS 12.2.1), so it does not seem to matter what UxPlay claims to be.


# Changelog

1.67 2023-09-17   Make Airplay server "Features" settings accessible for modification in uxplay.cpp; use this
                  to add option "-pair" to restore pre-1.65 client pairing protocol.

1.66 2023-09-05   Fix IPV6 support.  Add option to restrict clients to those on a list of allowed deviceIDs,
                  or to block connections from clients on a list of blocked deviceIDs.   Fix for #207 from
		  @thiccaxe (screen lag in vsync mode after client wakes from sleep). 
		  
1.65.3 2023-07-23 Add RPM spec file; add warning if required gstreamer libav feature "avdec_aac" is 
                  missing: (this occurs in RPM-based distributions that ship an incomplete FFmpeg for Patent
                  or License reasons, and rely on users installing an externally-supplied complete FFmpeg).
		  Mirror-mode airplay will now work without audio if avdec_aac is missing.

1.65 2023-06-03   Eliminate pair_setup part of connection protocol to allow faster connections with clients
                  (thanks to @shuax #176 for this discovery); to revert, uncomment a line in lib/dnssdint.h.
                  Disconnect from audio device when connection closes, to not block its use by other apps if 
                  uxplay is running but not connected.  Fix for AirMyPC client (broken since 1.60), so its 
                  older non-NTP timestamp protocol works with -vsync.   Corrected parsing of configuration
		  file entries that were in quotes.

1.64 2023-04-23   Timestamp-based synchronization of audio and video is now the default in Mirror mode.
                  (Use "-vsync no" to restore previous behavior.)    A configuration file can now be used
                  for startup options.   Also some internal cleanups and a minor bugfix that fixes #192.

1.63 2023-02-12   Reworked audio-video synchronization, with new options -vsync (for Mirror mode) and
                  -async (for Audio-Only mode, to sync with client video).   Option  -vsync makes software
                  h264 decoding of streamed videos with option -avdec viable on some recent Raspberry Pi models.
                  Internal change: all times are now processed in nanoseconds units.   Removed -ao option
                  introduced in 1.62.

1.62 2023-01-18   Added Audio-only mode time offset -ao x to allow user synchronization of ALAC
                  audio playing on the server with video, song lyrics, etc. playing on the client.
                  x = 5.0 appears to be optimal in many cases.  Quality fixes: cleanup in volume 
                  changes, timestamps, some bugfixes.

1.61 2022-12-30   Removed -t option (workaround for an Avahi issue, correctly solved by opening network
                  port UDP 5353 in firewall).  Remove -g debug flag from CMAKE_CFLAGS. Postpend (instead 
                  of prepend) build environment CFLAGS to CMAKE_CFLAGS.  Refactor parts of uxplay.cpp

1.60 2022-12-15   Added exit with error message if DNSServiceRegister fails (instead of just stalling).
                  Test for Client's attempt to using unsupported AirPlay 2 "REMOTE CONTROL" protocol
                  (with no timing channel), and exit if this occurs.   Reworked metadata processing 
                  to correctly parse DMAP header (previous version worked with DMAP messages currently
                  received, but was not correct).

1.59 2022-12-12   remove "ZOOMFIX" compile option and make compilation with X11-dependence the
                  default if X11 development libraries are detected (this now also provides 
                  fullscreen mode with a F11 or Alt+Enter key toggle); ZOOMFIX is now automatically 
                  applied for GStreamer < 1.20. New cmake option -DNO_X11_DEPS compiles uxplay without
                  X11 dependence. Reworked internal metadata handling. Fix segfault with "-vs 0".

1.58 2022-10-29   Add option "-nohold" that will drop existing connections when a new client connects.
                  Update llhttp to v8.1.0.

1.57 2022-10-09   Minor fixes: (fix coredump on AUR on "stop mirroring", occurs when compiled with
                  AUR CFLAGS -DFORTIFY_SOURCE); graceful exit when required plugins are missing;
                  improved support for builds on Windows.  Include audioresample in GStreamer
                  audio pipeline.

1.56 2022-09-01   Added support for building and running UxPlay-1.56 on Windows (no changes
		  to Unix (Linux, *BSD, macOS) codebase.)
					
1.56 2022-07-30   Remove -bt709 from -rpi, -rpiwl, -rpifb as GStreamer is now fixed.

1.55 2022-07-04   Remove the bt709 fix from -v4l2 and  create a new -bt709 option (previous
                  "-v4l2" is now "-v4l2 -bt709").  This allows the currently-required -bt709 
                  option to be used on its own on RPi without -v4l2 (sometimes this give better results). 

1.54 2022-06-25   Add support for "Cover Art" display in Audio-only (ALAC) mode. Reverted a change 
                  that caused  VAAPI to crash with AMD POLARIS graphics cards.   Minor internal changes to
                  plist code and uxplay option parsing.

1.53 2022-06-13   Internal changes to audio sync code, revised documentation, 
                  Minor bugfix (fix assertion crash when resent audio packets are empty).

1.52 2022-05-05   Cleaned up initial audio sync code, and reformatted
                  streaming debug output (readable aligned timestamps with
                  decimal points in seconds).   Eliminate memory leaks
		  (found by valgrind).   Support for display of ALAC
		  (audio-only) metadata (soundtrack artist names, titles etc.)
		  in the uxplay terminal.

1.51 2022-04-24   Reworked options forVideo4Linux2 support (new option -v4l2) and short options -rpi, -rpifb, -rpiwl as
                  synonyms for -v4l2,  -v4l2 -vs kmssink, and -v4l2 -vs waylandsink.  Reverted a change from 1.48 that broke
		  reconnection after "Stop Mirroring" is sent by client.

1.50 2022-04-22   Added -fs fullscreen option (for Wayland or VAAPI plugins only), Changed -rpi to be for framebuffer ("lite") RPi
                  systems and added -rpigl (OpenGL) and -rpiwl (Wayland) options for RPi Desktop systems.
                  Also modified  timestamps from "DTS" to "PTS" for latency improvement, plus internal cleanups.
		  
1.49 2022-03-28   Addded options for dumping video and/or audio to file, for debugging, etc.  h264  PPS/SPS NALU's are shown with -d.
                  Fixed video-not-working for M1 Mac clients.

1.48 2022-03-11   Made the GStreamer video pipeline fully configurable, for use with hardware h264 decoding.  Support for Raspberry Pi.

1.47 2022-02-05   Added -FPSdata option to display (in the terminal) regular reports sent by the client about video streaming 
                  performance.  Internal cleanups of processing of video packets received from the client.   Added -reset n option 
                  to reset the connection after n ntp timeouts (also reset after "connection reset by peer" error in video stream).

1.46 2022-01-20   Restore pre-1.44 behavior (1.44 may have broken hardware acceleration): once again use decodebin in the video pipeline; 
                  introduce new option "-avdec" to force software h264 decoding by libav h264, if needed (to prevent selection of 
                  vaapisink by autovideosink).  Update llhttp to v6.0.6.  UxPlay now reports itself as AppleTV3,2.  Restrict connections 
                  to one client at a time (second client must now wait for first client to disconnect). 

1.45 2022-01-10   New behavior: close video window when client requests "stop mirroring".  (A new "no close" option "-nc" is added
                  for users who wish to retain previous behavior that does not close the video window).

1.44 2021-12-13   Omit hash of aeskey with ecdh_secret for an AirMyPC client; make an internal rearrangement of where this hash is 
                  done. Fully report all initial communications between client and server in -d debug mode. Replace decodebin in GStreamer
                  video pipeline by h264-specific elements.

1.43 2021-12-07   Various internal changes, such as tests for successful decryption, uniform treatment 
                  of informational/debug messages, etc., updated README.

1.42 2021-11-20   Fix MAC detection to work with modern Linux interface naming practices, MacOS and *BSD.

1.41 2021-11-11    Further cleanups of multiple audio format support (internal changes, 
                   separated RAOP and GStreamer audio/video startup)

1.40 2021-11-09    Cleanup segfault in ALAC support, manpage location fix, show request Plists in debug mode.

1.39 2021-11-06    Added support for Apple Lossless (ALAC) audio streams.

1.38 2021-10-8     Add -as _audiosink_ option to allow user to choose the  GStreamer audiosink.

1.37 2021-09-29    Append "@hostname" to AirPlay Server name, where "hostname" is the name of the
                   server running uxplay (reworked change in 1.36).

1.36 2021-09-29    Implemented suggestion (by @mrbesen and @PetrusZ) to use hostname of machine
                   runing uxplay as the default server name 

1.35.1 2021-09-28  Added the -vs 0 option for streaming audio, but not displaying video.

1.35  2021-09-10   now uses a GLib MainLoop, and builds on macOS (tested on Intel Mac, 10.15 ).
                   New option  -t _timeout_ for relaunching server if no connections were active in
                   previous _timeout_ seconds (to renew Bonjour registration).
                   
1.341 2021-09-04   fixed: render logger was not being destroyed by stop_server()

1.34  2021-08-27   Fixed "ZOOMFIX": the X11 window name fix was only being made the
                   first time the GStreamer window was created by uxplay, and
		   not if the server was relaunched after the GStreamer window
		   was closed, with uxplay still running.   Corrected in v. 1.34

### Building OpenSSL >= 1.1.1 from source.

If you need to do this, note that you may be able to use a newer version (OpenSSL-3.0.1 is known to work).
You will need the standard development toolset (autoconf, automake, libtool).
Download the source code from
[https://www.openssl.org/source/](https://www.openssl.org/source/).
Install the downloaded
openssl by opening a terminal in your Downloads directory, and unpacking the source distribution:
("tar -xvzf openssl-3.0.1.tar.gz ; cd openssl-3.0.1"). Then build/install with
"./config ; make ; sudo make install_dev".  This will typically install the needed library ```libcrypto.*```,
either in /usr/local/lib or /usr/local/lib64.

_(Ignore the following for builds on MacOS:)_
On some systems like
Debian or Ubuntu, you may also need to add a missing  entry ```/usr/local/lib64```
in /etc/ld.so.conf (or place a file containing "/usr/local/lib64/libcrypto.so" in /etc/ld.so.conf.d)
and then run "sudo ldconfig". 

### Building libplist >= 2.0.0 from source.

_(Note: on Debian 9 "Stretch" or Ubuntu 16.04 LTS editions, you can avoid this step by installing libplist-dev
and libplist3 from Debian 10 or Ubuntu 18.04.)_
As well as the usual build tools (autoconf, automake, libtool), you
may need to also install some libpython\*-dev package.  Download the latest source
with git from [https://github.com/libimobiledevice/libplist](https://github.com/libimobiledevice/libplist), or 
get the source from the Releases section (use the \*.tar.bz2 release, **not** the \*.zip or \*.tar.gz versions): 
download [libplist-2.3.0](https://github.com/libimobiledevice/libplist/releases/download/2.3.0/libplist-2.3.0.tar.bz2),
then unpack it ("tar -xvjf libplist-2.3.0.tar.bz2 ; cd libplist-2.3.0"), and build/install it:
("./configure ;  make ; sudo make install").   This will probably install libplist-2.0.\* in /usr/local/lib.
The new libplist-2.3.0 release should be compatible with 
UxPlay; [libplist-2.2.0](https://github.com/libimobiledevice/libplist/releases/download/2.2.0/libplist-2.2.0.tar.bz2) is 
also available if there are any issues.  

_(Ignore the following for builds on MacOS:)_  On some systems like
Debian or Ubuntu, you may also need to add a missing  entry ```/usr/local/lib```
in /etc/ld.so.conf (or place a file containing "/usr/local/lib/libplist-2.0.so" in /etc/ld.so.conf.d)
and then run "sudo ldconfig". 




# Disclaimer

All the resources in this repository are written using only freely available information from the internet. The code and related resources are meant for 
educational purposes only. It is the responsibility of the user to make sure all local laws are adhered to.

This project makes use of a third-party GPL library for handling FairPlay. The legal status of that library is unclear. Should you be a representative of 
Apple and have any objections against the legality of the library and its use in this project, please contact the developers and the appropriate steps 
will be taken.

Given the large number of third-party AirPlay receivers (mostly closed-source) available for purchase, it is our understanding that an open source 
implementation of the same functionality wouldn't violate any of Apple's rights either.



# UxPlay authors

_[adapted from fdraschbacher's notes on  RPiPlay antecedents]_

The code in this repository accumulated from various sources over time. Here
is an attempt at listing the various authors and the components they created:

UxPlay was initially created by **antimof** from RPiPlay, by replacing its Raspberry-Pi-adapted OpenMAX  video 
and audio rendering system with GStreamer rendering for
desktop Linux systems; the antimof work on code in `renderers/` was later backported to RPiPlay, and the antimof project became dormant, but was later 
revived at the [current GitHub site](http://github.com/FDH2/UxPlay)  to serve a wider community of users.

The previous authors of code included in UxPlay by inheritance from RPiPlay include:

* **EstebanKubata**: Created a FairPlay library called [PlayFair](https://github.com/EstebanKubata/playfair). Located in the `lib/playfair` folder. License: GNU GPL
* **Juho Vähä-Herttua** and contributors: Created an AirPlay audio server called [ShairPlay](https://github.com/juhovh/shairplay), including support for Fairplay based on PlayFair. Most of the code   in `lib/` originally stems from this project. License: GNU LGPLv2.1+
* **dsafa22**: Created an AirPlay 2 mirroring server [AirplayServer](https://github.com/dsafa22/AirplayServer) (seems gone now), for Android based on ShairPlay.   Code is 
  preserved [here](https://github.com/jiangban/AirplayServer), and [see here](https://github.com/FDH2/UxPlay/wiki/AirPlay2) for  the description 
  of the analysis of the AirPlay 2 mirror protocol that made RPiPlay possible, by the AirplayServer author. All 
  code in `lib/` concerning mirroring is dsafa22's work. License: GNU LGPLv2.1+
* **Florian Draschbacher** (FD-) and contributors: adapted dsafa22's Android project for the Raspberry Pi, with extensive cleanups, debugging and improvements.  The
   project [RPiPlay](https://github.com/FD-/RPiPlay) is basically a port of dsafa22's code to the Raspberry Pi, utilizing OpenMAX and OpenSSL for better performance on the Pi. License GPL v3.
   FD- has written an interesting note on the history of [Airplay protocol versions](http://github.com/FD-/RPiPlay#airplay-protocol-versions),
   available at the RPiPlay github repository.


Independent of UxPlay, but used by it and bundled with it:

* **Fedor Indutny** (of Node.js, and formerly Joyent, Inc) and contributors: Created an http parsing library called [llhttp](https://github.com/nodejs/llhttp). Located at `lib/llhttp/`. License: MIT


