
# UxPlay 1.42:  AirPlay/AirPlay-Mirror server for Linux, macOS, and Unix.

This project is a GPLv3  unix AirPlay server which  now also works on macOS.
Its main use is to act like an AppleTV for screen-mirroring (with audio) of iOS/macOS clients
(iPads, iPhones, MacBooks) in a window on the server display (with the possibility of
sharing that window on screen-sharing applications such as Zoom)
on a host running Linux, macOS, or other unix, using Apple's AirPlay Mirror protocol first available in iOS 5.
As well as screen mirroring, when not mirroring the screen it can also play Apple Lossless (ALAC) audio streamed from the client
using the audio-only Airplay protocol.
(Details of what is known about the AirPlay2 protocol can be found [here](https://github.com/SteeBono/airplayreceiver/wiki/AirPlay2-Protocol) and
[here](https://emanuelecozzi.net/docs/airplay2)).

The UxPlay server and its client must be on the same local area network,
on which a **Bonjour/Zeroconf mDNS/DNS-SD server**  is also running
(only DNS-SD "Service Discovery" service is necessary, it is not necessary that the local network also be of the ".local" mDNS-based type). 
On Linux and BSD Unix servers, this is usually provided by [Avahi](https://www.avahi.org),
through the avahi-daemon service, and is included in  most Linux distributions (this
service can also be provided by macOS, iOS or Windows servers).

_**New:** UxPlay > 1.38  now also supports the Airplay audio-only protocol as well as AirPlay Mirror protocol, and (when the client screen is not being mirrored)
can play Apple Lossless (ALAC)
44100/16/2 audio streamed from the client in  2-channel stereo without video (the accompanying cover-art and metadata is received by the server,
but not displayed). The initial connection to the client can be in AirPlay audio mode, or an initial Airplay Mirror connection can be switched to
Airplay audio by closing the Mirror window and reconnecting in audio-only mode (this changes back to AAC audio if screen mirroring is (re)started)._

UxPlay is based on https://github.com/FD-/RPiPlay, with GStreamer integration from
https://github.com/antimof/UxPlay.
(UxPlay only uses GStreamer, and  does not contain the alternative Raspberry-Pi-specific
audio and video renderers also found in RPiPlay.)
Tested on Ubuntu 20.04, Linux Mint 20.2, OpenSUSE 15.3, macOS 10.15, FreeBSD 13.0.

Features:
1. Based on Gstreamer.
2. Video and audio are supported out of the box.
3. Gstreamer decoding is plugin agnostic. Uses accelerated decoders if
available. VAAPI is preferable, (but don't use VAAPI with nVidia).
4. Automatic screen orientation.

**Note for packagers:  UxPlay can be built to use the recently-released OpenSSL- 3.x shared "libcrypto" library, which has
the Apache license which is generally viewed as compatible with GPL v3 (see
[this announcement](https://www.openssl.org/blog/blog/2021/09/07/OpenSSL3.Final/) and 
[this](https://www.openssl.org/blog/blog/2017/03/22/license/)).
The older license of OpenSSL-1.1.1 is viewed by some distributions (e.g. Debian) as incompatible with GPLv3 code
unless the authors have granted a permissive "exception" to allow this.  The historical origins of the UxPlay code
make it impossible to obtain such a  permission from all previous authors.
However there are  other distributions which have viewed  OpenSSL with the older license
as a  "System library" which would make it GPL-permissible to distribute binary compiled packages of GPLv3
applications that are linked to it, without an explicit GPL exception having been granted by the application's authors.**

# Getting UxPlay:

Either download and unzip [UxPlay-master.zip](https://github.com/FDH2/UxPlay/archive/refs/heads/master.zip), 
or (if git is installed): "git clone https://github.com/FDH2/UxPlay".   

*This is a pull request on the
original site https://github.com/antimof/UxPlay ; it may or may not ever
get committed into the codebase  on the antimof site, as that
project appears to no longer be active.
If the pull request ever gets committed, replace "FDH2" by "antimof" in the above.*

# Building this version (Linux and  \*BSD):

(Instructions for Debian/Ubuntu; adapt these for other Linuxes; for macOS, see below).

You need a C/C++ compiler (e.g. g++) with the standard development libraries installed.

Make sure that cmake>=3.4.1 and pkg-config are also installed: "sudo apt-get install cmake pkg-config".
In a terminal window, change directories to the source directory of the
downloaded source code ("UxPlay-master" for zipfile downloads, "UxPlay" for "git clone" downloads), then do

1. `sudo apt-get install libssl-dev libplist-dev libavahi-compat-libdnssd-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-libav gstreamer1.0-plugins-bad libavahi-compat-libdnssd-dev`
2. `sudo apt-get install gstreamer1.0-vaapi` (For Intel graphics, but not nVidia graphics)
3. `sudo apt-get install libx11-dev`  (for the "ZOOMFIX" X11_display name fix for screen-sharing with e.g.,  ZOOM)
4. `cmake .` (or "`cmake -DZOOMFIX=ON .`" to get a screen-sharing fix to
make X11 mirror display windows visible to screen-sharing applications such as
Zoom, see below).
5. `make`
6. `sudo make install`    (you can afterwards uninstall with `sudo make uninstall` in the same directory in which this was run)

_Note that older cmake does not require the final "`.`" to specify the current directory as the source directory,
but newer cmake requires it. If you intend to modify the code, use a separate "build" directory: replace "cmake  [ ] ." by "mkdir build ; cd build ; cmake [ ] .."; you can then get back to the clean source (with any changes you made) with "cd .. ; rm -rf build"._

The above script installs the executable file "`uxplay`" to `/usr/local/bin`, (and installs a manpage to `/usr/local/man/man1` and README
files to `/usr/local/share/doc/uxplay`).
It can also be found in the build directory after the build
processs. Run uxplay in a terminal window.


**Note libplist-dev (which must be for libplist version 2.0 or greater) and (for ZOOMFIX) libx11-dev are new dependencies.  Older distributions
may only supply libplist 1.x, which is too old.  [Installing libplist-dev (with libplist3) from ubuntu 18.04 solves this problem on ubuntu 16.04.]**

_If you  cannot find a libplist-2.x package that installs on your older distribution, and you don't wish to  upgrade, see the instructions below
(in the macOS section) on building libplist from source (you need autoconf, automake, libtool, and may need to also install some libpython*-dev package)
but (unlike the statically-linked macOS case) do not uninstall the libplist library after building uxplay; it  must remain installed.
It is in /usr/local/lib. If uxplay fails to find libplist when you run it, this is probably because /usr/local/lib is not in the library path (by default,
this is the case in ubuntu). To fix this, run  "sudo ldconfig" (you might also need to create a file /etc/ld.so.conf.d/libplist.conf
containing  the text "/usr/local/lib" before running ldconfig) to permanently
add /usr/local/lib  to the library path._

**Red Hat, Fedora, CentOS:** 
(sudo yum install) openssl-devel libplist-devel avahi-compat-libdns_sd-devel (+libX11-devel for ZOOMFIX).   The required
GStreamer packages are:
gstreamer1-devel gstreamer1-plugins-base-devel gstreamer1-libav gstreamer1-plugins-bad-free ( + gstreamer1-vaapi for intel graphics).

**OpenSUSE:**
(sudo zypper install) libopenssl-devel libplist-devel
avahi-compat-mDNSResponder-devel (+ libX11-devel for ZOOMFIX).  The required
GStreamer packages are:
gstreamer-devel gstreamer-plugins-base-devel gstreamer-plugins-libav gstreamer-plugins-bad (+ gstreamer-plugins-vaapi for Intel graphics).

**FreeBSD:** (sudo pkg install) libplist gstreamer1, gstreamer1-libav, gstreamer1-plugins, gstreamer1-plugins-*
(\* = core, good,  bad, x, gtk, gl, vulkan, pulse ...), (+ gstreamer1-vaapi for Intel graphics).
OpenSSL is already installed.   "ZOOMFIX" is untested; don't try to use it unless you need it.

# Building this version (macOS):

**(Only tested on Intel X86_64 Macs)**

_Note: A native AirPlay Server feature is  included in upcoming macOS 12 Monterey,
but UxPlay can run  on older macOS systems that will not be able to run Monterey._

These instructions for macOS asssume that the Xcode command-line developer tools are installed (if Xcode is installed, open the Terminal, type "sudo xcode-select --install" and accept the conditions).

It is also assumed that CMake >= 3.13 is installed:
this can be done with package managers [MacPorts](http://www.macports.org),
[Fink](http://finkproject.org) or [Homebrew](http://brew.sh), or by a download from
[https://cmake.org/download/](https://cmake.org/download/).


First get the latest macOS release of GStreamer-1.0
from [https://gstreamer.freedesktop.org/download/](https://gstreamer.freedesktop.org/download/).
Install both the macOS runtime and development installer packages. Assuming that the latest release is 1.18.5
they are ```gstreamer-1.0-1.18.5-x86_64.pkg``` and ```gstreamer-1.0-devel-1.18.5-x86_64.pkg```.
Click on them to install (they install to
/Library/FrameWorks/GStreamer.framework).
It is recommended you use GStreamer.framework rather than install Gstreamer with Homebrew or MacPorts (see later).

Next install OpenSSL-1.1.1 and libplist:  these can be built from source (see below) but it's easier to get them using
MacPorts "sudo port install openssl libplist-devel" or Homebrew "brew install openssl libplist".   Only the
static forms of the two libraries will used for the macOS build, so they do not need to remain installed after you have built uxplay:
if you don't have MacPorts or Homebrew installed, you can just install
one of these package-managers before building  uxplay, and uninstall it afterwards if you don't want to keep it.
Unfortunately, Fink's openssl11-dev package currently doesn't supply the static (libcrypto.a) form of the
needed library libcrypto, and its libplist1 package is too old.

Finally, build and install uxplay (without ZOOMFIX): open a terminal and change into the UxPlay source directory
("UxPlay-master" for zipfile downloads, "UxPlay" for "git clone" downloads) and build/install with
"cmake . ; make ; sudo make install " (same as for Linux).  

The macOS build uses OpenGL, not X11, to create the mirror display window.   This has some "quirks":
the window title is "OpenGL renderer" instead of the Airplay server name, but it is visible to
screen-sharing apps (e.g., Zoom).   The option -t _timeout_
cannot be used because if the GStreamer pipeline is destroyed while the OpenGL window is still open,
and uxplay is left running, a segfault occurs (this is an issue with the glimagesink GStreamer OpenGL
plugin, not UxPlay).
Also, the resolution settings "-s wxh" do not affect
the (small) initial mirror window size, but the window can be expanded using the mouse or trackpad.


***Building OpenSSL-1.1.1 and libplist from source on macOS***

If you have have the standard GNU toolset (autoconf, automake, libtool, etc.) installed,
you can also  download and compile the source code for these libraries from
[https://www.openssl.org/source/](https://www.openssl.org/source/), 
[https://github.com/libimobiledevice/libplist](https://github.com/libimobiledevice/libplist).
Install the downloaded
openssl-1.1.1 by opening a terminal in your Downloads directory, and  unpacking the source distribution openssl-1.1.1x.tar.gz (where "x" is a "patch" label,
currently given by  "x" = "l"):
("tar -xvzf openssl-1.1.1x.tar.gz ; cd openssl-1.1.1x"). Then build/install with
"./config ; make ; sudo make install_dev" and clean up after building uxplay  with "sudo make uninstall" in the same directory.
Similarly, for libplist, download the source as a zipfile from github as
[libplist-master.zip](https://github.com/libimobiledevice/libplist/archive/refs/heads/master.zip), then
unpack ("unzip libplist-master.zip ; cd libplist-master"), build/install
("./autogen.sh ; make ; sudo make install") and clean up after uxplay is built  with "sudo make uninstall"  in the same directory.  


***Other ways (Homebrew, MacPorts) to install GStreamer on macOS (not recommended):***

First make sure that pkgconfig is installed  (Homebrew: "brew install pkgconfig" ; MacPorts: "sudo port install pkgconfig" ).  

(a) with Homebrew: "brew install gst-plugins-good gst-plugins-bad gst-libav".   This appears to be functionally equivalent
to using GStreamer.framework, but causes a large number of extra packages to be installed by Homebrew as dependencies.

(b) with MacPorts: "sudo port install gstreamer1-gst-plugins-good gstreamer1-gst-plugins-bad gstreamer1-gst-libav".
The MacPorts GStreamer is built to use X11, so must be run from an XQuartz terminal, can use ZOOMFIX, and needs
option "-vs ximagesink".  On an older unibody MacBook Pro, the default resolution  wxh = 1920x1080 was too large  for
the non-retina display, but using option "-s 800x600" worked; However, the GStreamer pipeline is fragile against attempts to change
the X11 window size, or to rotations that switch a connected client between portrait and landscape mode while uxplay is running. 
Using the MacPorts X11 GStreamer is only viable if the image size is left unchanged from the initial "-s wxh" setting 
(also use the iPad/iPhone setting that locks the screen orientation against switching  between portrait and landscape mode
as the device is rotated).

# **Troubleshooting:**

If uxplay starts, but stalls after "Initialized server socket(s)" appears,
it is probably because a firewall is blocking
access to the server on which it is running.  If possible, either turn off the firewall
to see if that is the problem, or get three consecutive network ports,
starting at port n, all three in the range 1024-65535, opened  for both tcp and udp, and use "uxplay -p n"
(or open UDP 6000, 6001, 6011 TCP 7000,7001,7100 and use "uxplay -p").

Stalling after "Initialize server socket(s)", with the server showing as available on the client iPad/iPhone,
is almost certainly a firewall problem: one user was unaware that
_two_ firewalls (ufw and firewalld) were both active  on their system.

Stalling this way, but _without_ the server showing as available on the client,
probably means that your network **does not have a running Bonjour/zeroconf DNS-SD server.**
On Linux, make sure Avahi is installed,
and start the avahi-daemon service (your distribution will document how to do  this).
Some  systems  may instead use the mdnsd daemon as an alternative to provide DNS-SD service.
(FreeBSD offers both alternatives, but only Avahi was tested: one of the steps needed for
getting Avahi running on this system is to edit /usr/local/etc/avahi/avahi-daemon.conf to
uncomment a line for airplay support.)

For other problems after a connection is made, use "uxplay -d " (debug log option)  to see what is happening.
**Such problems are usually due to a GStreamer plugin that doesn't work on your system**: (by default,
GStreamer uses an algorithm to guess what is the "best"
plugin to use on your system).
If you use an
nVidia graphics card, make sure that the gstreamer1.0-vaapi
plugin for Intel graphics is *NOT* installed (**uninstall it** if it is installed!).
(You can test for this  by explicitly choosing the GStreamer videosink with option
"-vs ximagesink" or "-vs xvimagesink", to see if this fixes the problem, or "-vs vaapisink" to see if this
reproduces the problem.)

You can try to fix audio problems by using the "-as _audiosink_"  option to choose the GStreamer audiosink , rather than
have autoaudiosink pick one for you.    The command "gst_inspect-1.0 | grep Sink | grep Audio" " will show you which audiosinks are 
available on your system.  (Replace  "Audio" by "Video" to see videosinks).   Some possible audiosinks are pulsesink, alsasink, osssink, oss4sink,
and osxaudiosink (macOS).  
 
If you ran cmake with "-DZOOMFIX=ON", check if the problem is still there without ZOOMFIX.
ZOOMFIX is only applied to the default videosink choice ("autovideosink") and the two X11 videosinks
"ximagesink" and "xvimagesink".   ZOOMFIX is only designed for these last two; if
autovideosink chooses a different videosink, ZOOMFIX is now ignored. 
If you are using the X11 windowing system (standard on Linux), and have trouble with screen-sharing on Zoom, use
ZOOMFIX and "-vs xvimagesink" (or "-vs ximagesink" if the previous choice doesn't work). 

As other  videosink choices are not affected by ZOOMFIX, they  may or may not be visible to screen-sharing apps.
Cairo-based windows created on Linux with "-vs gtksink" are visible to screen-sharing aps without ZOOMFIX; windows on macOS created by
"-vs glimagesink" (default choice) and "-vs osximagesink" are also visible.

The "OpenGL renderer" window created on Linux by "-vs glimagesink" sometimes does not close properly when its "close" button is clicked.
(this is a GStreamer issue).  You may need to terminate uxplay with Ctrl-C to close a "zombie" OpenGl window.

__GStreamer issues:__  To troubleshoot GStreamer execute  "export GST_DEBUG=2"
to set the GStreamer debug-level environment-variable in the terminal
where you will run uxplay, so that you see warning and error messages;
(replace "2" by "4" to see much (much) more of what is happening inside
GStreamer).   Run "gst-inspect-1.0" to see which GStreamer plugins are
installed on your system.

#  **Usage:**

Options:

**-n server_name** (Default: UxPlay);  server_name@_hostname_ will be the name that appears offering
   AirPlay services to your iPad, iPhone etc, where _hostname_ is the name of the server running uxplay. 
   This will also now be the name shown above the mirror display (X11)  window.

**-s wxh** (e.g. -s 1920x1080 , which is the default ) sets the display resolution (width and height,
   in pixels).   (This may be a
   request made to the AirPlay client, and perhaps will not
   be the final resolution you get.)   w and h are whole numbers with four
   digits or less.   Note that the **height** pixel size is the controlling
   one used by the client for determining the streaming format; the width is
   dynamically adjusted to the shape of the image (portrait or landscape
   format, depending on how an iPad is held, for example). 

**-s wxh@r**  As above, but also informs the AirPlay  client about the screen
   refresh rate of the display. Default is r=60 (60 Hz); r is a whole number
   with three digits or less.   Values greater than 255 are invalid.
   
**-fps n** sets a maximum frame rate (in frames per second) for the AirPlay
   client to stream video; n must be a whole number with 3 digits or less.
   (The client may choose to serve video at any frame rate lower
   than this;  default is 30 fps.)  A setting
   below 30 fps might be useful to reduce latency if you are running more than
   one instance of uxplay at the same time.   Values greater than 255 are
   ignored.  This setting is only an advisory to the client device, so setting
   a high value will not force a high framerate.
   (You can test using "-vs fpsdisplaysink" to see what framerate is being
   received.)
   
**-o** turns on an "overscanned" option for the display window.    This
   reduces the image resolution by using some of the pixels requested
   by  option -s wxh (or their default values 1920x1080) by adding an empty
   boundary frame of unused pixels (which would be lost in a full-screen
   display that overscans, and is not displayed by gstreamer).
   Recommendation: **don't use this option** unless there is some special
   reason to use it.

**-p**  allows you to select the network ports used by UxPlay (these need
   to be opened if the server is behind a firewall).   By itself, -p sets
   "legacy" ports TCP 7100, 7000, 7001, UDP 6000, 6001, 7011.   -p n (e.g. -p
   35000)  sets TCP and UDP ports n, n+1, n+2.  -p n1,n2,n3 (comma-separated
   values) sets each port separately; -p n1,n2 sets ports n1,n2,n2+1.  -p tcp n
   or -p udp n sets just the TCP or UDP ports.  Ports must be in the range
   [1024-65535].

If the -p option is not used, the ports are chosen dynamically (randomly),
which will not work if a firewall is running.

**-m**  generates a random MAC address to use instead of the true hardware MAC
   number of the computer's network card.   (Different server_name,  MAC
   addresses,  and network ports are needed for each running uxplay  if you
   attempt to  run two instances of uxplay on the same computer.)
   If UxPlay fails to find the true MAC address of a  network card, (more
   specifically, the MAC address used by the first active network interface detected)
   a random MAC address will be used even if option **-m** was not specifed.
   (Note that a random MAC address will be different each time UxPlay is started).


Also: image transforms that had been added to RPiPlay have been ported to UxPlay:

**-f {H|V|I}**  implements "videoflip" image transforms: H = horizontal flip
(right-left flip, or mirror image); V = vertical flip ;  I =
180 degree rotation or inversion (which is the combination of H with V).

**-r {R|L}**  90 degree Right (clockwise) or Left (counter-clockwise)
   rotations; these are carried out after any **-f** transforms.

**-vs _videosink_** chooses the GStreamer videosink, instead of letting
   autovideosink pick it for you.  Some videosink choices are:  ximagesink, xvimagesink,
   vaapisink (for intel graphics), gtksink, glimagesink, waylandsink, osximagesink (for macOS),  or
   fpsdisplaysink (which shows the streaming framerate in fps).   Using quotes
   "..." might allow some parameters to be included with the videosink name. 
   (Some choices of videosink might not work on your system.)   

**-vs 0** suppresses display of streamed video, but plays  streamed audio.   (The client's screen
   is still mirrored at a reduced rate of 1 frame per second,  but is not rendered or displayed.)  This
   feature (which streams audio in AAC audio format) is now probably unneeded, as UxPlay can now 
   stream superior-quality Apple Lossless audio without video in Airplay non-mirror mode.

**-as _audiosink_** chooses the GStreamer audiosink, instead of letting
   autoaudiosink pick it for you.  Some audiosink choices are:  pulsesink, alsasink, 
   osssink, oss4sink, and osxaudiosink (for macOS).  Using quotes
   "..." might allow some parameters to be included with the audiosink name. 
   (Some choices of audiosink might not work on your system.)   

**-as 0**  (or just **-a**) suppresses playing of streamed audio, but displays streamed video.

**-t _timeout_**  will cause the server to relaunch (without stopping uxplay) if no connections
   have been present during the previous _timeout_ seconds.  (You may wish to use this  because the Server may not be
   visible to  new Clients that were inactive when the Server was launched, and an idle Bonjour
   registration also eventually becomes unavailable for new connections.)  The timer only starts once a
   Client has first made a mirror connection and then has disconnected with "Stop Mirrroring".
   _This option should **not** be
   used if the display window is an OpenGL window on macOS, as such an  OpenGL window created
   by GStreamer does not terminate correctly (it causes a segfault)
   if it is still open when the  GStreamer pipeline is closed._


# ChangeLog
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

# Improvements since the original UxPlay by antimof:

1. Updates of the RAOP (AirPlay protocol)  collection of codes  maintained
at  https://github.com/FD-/RPiPlay.git so it is current as of 2021-08-01,
adding all changes since the original release of UxPlay by antimof.
This involved crypto updates, replacement
of the included plist library by the system-installed version, and  a change
over to a library llhttp for http parsing. 

2. Added the -s, -o -p, -m, -r,  -f, -fps  -vs -as  and -t  options.

3. If "`cmake -DZOOMFIX=ON .`"  is run before compiling,
the mirrored window is now visible to screen-sharing applications such as
Zoom. To compile with ZOOMFIX=ON, the X11 development libraries must be installed.
(ZOOMFIX will not be needed once the upcoming  gstreamer-1.20 is available,
since starting with that release, the GStreamer X11 mirror window will be natively
visible for screen-sharing.) Thanks to David Ventura
https://github.com/DavidVentura/UxPlay for the fix
and also for getting it into  gstreamer-1.20.
[If uxplay was compiled after
cmake was run without -DZOOMFIX=ON, and your gstreamer version is older than
1.20, you can still manually make the X11 window visible to screen-sharing apps with the X11 utility
xdotool, if it is installed, with: ``` xdotool selectwindow set_window --name <name>```
(where ```<name>``` is your choice of name), and then select the uxplay window
by clicking on it with the mouse.]

4. The AirPlay server now terminates correctly when the gstreamer display window is
closed, and is relaunched with the same settings to wait for a new connection.
The program uxplay terminates when Ctrl-C is typed in the terminal window. The **-t _timeout_**
option relaunches the server after _timeout_ seconds  of inactivity to allow new connections to be made.

5.   In principle, multiple instances of uxplay can be run simultaneously
using the **-m** (generate random MAC address) option to give each a
different ("local" as opposed to "universal")  MAC address.
If the **-p [n]** option is used, they also need separate network port choices.
(However, there may be a large latency, and running two instances of uxplay
simultaneously on the same computer may not be very useful; using the **-fps** option
to force streaming framerates below 30fps could be helpful.)

6.  Without the **-p** [n] option,  uxplay makes a random dynamic assignment of
network ports. This will not work if most ports are closed by a firewall.
With e.g., **-p 45000**   you should open both TCP and UDP on
ports 45000, 45001, 45002.   Minimum allowed port is 1024, maximum is 65535.
The option "**-p**" with no argument uses a "legacy" set of ports TCP 7100,
7000, 7001, and UDP  7011, 6000, 6001.  Finer control is also
possible: "**-p udp n1,n2,n3 -p tcp n4,n5,n6**" sets all six ports individually.

7.  The default resolution setting is 1920x1080 width x height pixels.
To change this, use "**-s wxh**"  where w and h are positive  decimals
with 4 or less digits.   It seems that the width and height may be negotiated
with the AirPlay client, so this may not be the actual screen geometry that
displays.

8. The title on the GStreamer display window is now is the AirPlay server name.
(This works for X11 windows created
by gstreamer videosinks ximagesink, xvimagesink, but not OpenGL windows created by glimagesink.)

9. The avahi_compat "nag" warning on startup is suppressed, by placing
"AVAHI_COMPAT_NOWARN=1" into the runtime environment when uxplay starts.
(This uses a call to putenv() in a form that is believed to be safe against
memory leaks, at least in modern Linux; if for any reason you don't want
this fix, comment out the line in CMakeLists.txt that activates it when uxplay
is compiled.) On macOS, Avahi is not used.

10. UxPlay now builds on macOS.

11. The hostname of the server running uxplay is now appended to the AirPlay server name,
    which is now displayed as _name_@hostname, where _name_ is "UxPlay", (or whatever is set with the **-n** option).

12. Added support for audio-only streaming with original (non-Mirror) AirPlay protocol, with Apple Lossless (ALAC) audio.

# Disclaimer

All the resources in this repository are written using only freely available information from the internet. The code and related resources are meant for educational purposes only. It is the responsibility of the user to make sure all local laws are adhered to.

This project makes use of a third-party GPL library for handling FairPlay. The legal status of that library is unclear. Should you be a representative of Apple and have any objections against the legality of the library and its use in this project, please contact me and I'll take the appropriate steps.

Given the large number of third-party AirPlay receivers (mostly closed-source) available for purchase, it is my understanding that an open source implementation of the same functionality wouldn't violate any of Apple's rights either.


# RPiPlay Authors (Notes by Florian Draschbacher, RPiPlay creator.)

(From the  https://github.com/FD-/RPiPlay.git repository.)

The code in this repository accumulated from various sources over time. Here
is my (__fdrachbacher__) attempt at listing the various authors and the components they created:

* **dsafa22**: Created an [AirPlay 2 mirroring server](https://github.com/dsafa22/AirplayServer)(seems gone now) for Android based on ShairPlay. This project is basically a port of dsafa22's code to the Raspberry Pi, utilizing OpenMAX and OpenSSL for better performance on the Pi. All code in `lib/` concerning mirroring is dsafa22's work. License: GNU LGPLv2.1+
* **Juho Vähä-Herttua** and contributors: Created an AirPlay audio server called [ShairPlay](https://github.com/juhovh/shairplay), including support for Fairplay based on PlayFair. Most of the code in `lib/` originally stems from this project. License: GNU LGPLv2.1+
* **EstebanKubata**: Created a FairPlay library called [PlayFair](https://github.com/EstebanKubata/playfair). Located in the `lib/playfair` folder. License: GNU GPL
* **Joyent, Inc and contributors**: Created an http library called [llhttp](https://github.com/nodejs/llhttp). Located at `lib/llhttp/`. License: MIT
* **Team XBMC**: Managed to show a black background for OpenMAX video rendering. This code is used in the video renderer. License: GNU GPL
* **Alex Izvorski and contributors**: Wrote [h264bitstream](https://github.com/aizvorski/h264bitstream), a library for manipulation h264 streams. Used for reducing delay in the Raspberry Pi video pipeline. Located in the `renderers/h264-bitstream` folder. License: GNU LGPLv2.1

# AirPlay protocol versions

For multiple reasons, it's very difficult to clearly define the protocol names and versions of the components that make up the AirPlay streaming system. In fact, it seems like the AirPlay version number used for marketing differs from that used in the actual implementation. In order to tidy up this whole mess a bit, I did a little research that I'd like to summarize here:


The very origin of the AirPlay protocol suite was launched as AirTunes sometime around 2004. It allowed to stream audio from iTunes to an AirPort Express station. Internally, the name of the protocol that was used was RAOP, or Remote Audio Output Protocol. It seems already back then, the protocol involved AES encryption. A public key was needed for encrypting the audio sent to an AirPort Express, and the private key was needed for receiving the protocol (ie used in the AirPort Express to decrypt the stream). Already in 2004, the public key was reverse-engineered, so that [third-party sender applications](http://nanocr.eu/2004/08/11/reversing-airtunes/) were developed.


Some time [around 2008](https://weblog.rogueamoeba.com/2008/01/10/a-tour-of-airfoil-3/), the protocol was revised and named AirTunes 2. It seems the changes primarily concerned timing. By 2009, the new protocol was [reverse-engineered and documented](https://git.zx2c4.com/Airtunes2/about/).


When the Apple TV 2nd generation was introduced in 2010, it received support for the AirTunes protocol. However, because this device allowed playback of visual content, the protocol was extended and renamed AirPlay. It was now possible to stream photo slideshows and videos. Shortly after the release of the Apple TV 2nd generation, AirPlay support for iOS was included in the iOS 4.2 update. It seems like at that point, the audio stream was still actually using the same AirTunes 2 protocol as described above. The video and photo streams were added as a whole new protocol based on HTTP, pretty much independent from the audio stream. Soon, the first curious developers began to [investigate how it worked](https://web.archive.org/web/20101211213705/http://www.tuaw.com/2010/12/08/dear-aunt-tuaw-can-i-airplay-to-my-mac/). Their conclusion was that visual content is streamed unencrypted.


In April 2011, a talented hacker [extracted the AirPlay private key](http://www.macrumors.com/2011/04/11/apple-airplay-private-key-exposed-opening-door-to-airport-express-emulators/) from an AirPort Express. This meant that finally, third-party developers were able to also build AirPlay receiver (server) programs.


For iOS 5, released in 2011, Apple added a new protocol to the AirPlay suite: AirPlay mirroring. [Initial investigators](https://www.aorensoftware.com/blog/2011/08/20/exploring-airplay-mirroring-internals/) found this new protocol used encryption in order to protect the transferred video data.


By 2012, most of AirPlay's protocols had been reverse-engineered and [documented](https://nto.github.io/AirPlay.html). At this point, audio still used the AirTunes 2 protocol from around 2008, video, photos and mirroring still used their respective protocols in an unmodified form, so you could still speak of AirPlay 1 (building upon AirTunes 2). The Airplay server running on the Apple TV reported as version 130. The setup of AirPlay mirroring used the xml format, in particular a stream.xml file.
Additionally, it seems like the actual audio data is using the ALAC codec for audio-only (AirTunes 2) streaming and AAC for mirror audio. At least these different formats were used in [later iOS versions](https://github.com/espes/Slave-in-the-Magic-Mirror/issues/12#issuecomment-372380451).


Sometime before iOS 9, the protocol for mirroring was slightly modified: Instead of the "stream.xml" API endpoint, the same information could also be querried in binary plist form, just by changing the API endpoint to "stream", without any extension. I wasn't able to figure out which of these was actually used by what specific client / server versions.


For iOS 9, Apple made [considerable changes](https://9to5mac.com/2015/09/11/apple-ios-9-airplay-improvements-screen-mirroring/) to the AirPlay protocol in 2015, including audio and mirroring. Apparently, the audio protocol was only slightly modified, and a [minor change](https://github.com/juhovh/shairplay/issues/43) restored compatibility. For mirroring, an [additional pairing phase](https://github.com/juhovh/shairplay/issues/43#issuecomment-142115959) was added to the connection establishment procedure, consisting of pair-setup and pair-verify calls. Seemingly, these were added in order to simplify usage with devices that are connected frequently. Pair-setup is used only the first time an iOS device connects to an AirPlay receiver. The generated cryptographic binding can be used for pair-verify in later sessions. Additionally, the stream / stream.xml endpoint was replaced with the info endpoint (only available as binary plist AFAICT).
As of iOS 12, the protocol introduced with iOS 9 was still supported with only slight modifications, albeit as a legacy mode. While iOS 9 used two SETUP calls (one for general connection and mirroring video, and one for audio), iOS 12 legacy mode uses 3 SETUP calls (one for general connection (timing and events), one for mirroring video, one for audio).


The release of tvOS 10.2 broke many third-party AirPlay sender (client) programs in 2017. The reason was that it was now mandatory to perform device verification via a pin in order to stream content to an Apple TV. The functionality had been in the protocol before, but was not mandatory. Some discussion about the new scheme can be found [here](https://github.com/postlund/pyatv/issues/79). A full specification of the pairing and authentication protocol was made available on [GitHub](https://htmlpreview.github.io/?https://github.com/philippe44/RAOP-Player/blob/master/doc/auth_protocol.html). At that point, tvOS 10.2 reported as AirTunes/320.20.


In tvOS 11, the reported server version was [increased to 350.92.4](https://github.com/ejurgensen/forked-daapd/issues/377#issuecomment-309213273).


iOS 11.4 added AirPlay 2 in 2018. Although extensively covered by the media, it's not entirely clear what changes specifically Apple has made protocol-wise.


From captures of the traffic between an iOS device running iOS 12.2 and an AppleTV running tvOS 12.2.1, one can see that the communication on the main mirroring HTTP connection is encrypted after the initial handshake.
This could theoretically be part of the new AirPlay 2 protocol. The AppleTV running tvOS 12.2.1 identifies as AirTunes/380.20.1.
When connecting from the same iOS device to an AppleTV 3rd generation (reporting as AirTunes/220.68), the communication is still visible in plain. From the log messages that the iOS device produces when connected to an AppleTV 3rd generation, it becomes apparent that the iOS device is treating this plain protocol as the legacy protocol (as originally introduced with iOS 9). Further research showed that at the moment, all available third-party AirPlay mirroring receivers (servers) are using this legacy protocol, including the open source implementation of dsafa22, which is the base for RPiPlay. Given Apple considers this a legacy protocol, it can be expected to be removed entirely in the future. This means that all third-party AirPlay receivers will have to be updated to the new (fully encrypted) protocol at some point.

More specifically, the encryption starts after the pair-verify handshake completed, so the fp-setup handshake is already happening encrypted. Judging from the encryption scheme for AirPlay video (aka HLS Relay), likely two AES GCM 128 ciphers are used on the socket communication (one for sending, one for receiving). However, I have no idea how the keys are derived from the handshake data.
