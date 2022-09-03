# UxPlay 1.56:  AirPlay-Mirror and AirPlay-Audio server for Linux, macOS, and Unix (now also runs on Windows).

### Now developed at the GitHub site [https://github.com/FDH2/UxPlay](https://github.com/FDH2/UxPlay) (where all user issues should be posted).


Highlights:

   * GPLv3, open source.
   * Originally supported only AirPlay Mirror protocol, now has added support
     for AirPlay Audio-only (Apple Lossless ALAC) streaming 
     from current iOS/iPadOS 15.6 clients.  **There is no support for Airplay2 video-streaming protocol, and none is planned.**
   * macOS computers (2011 or later, both Intel and "Apple Silicon" M1/M2
     systems) can act either as AirPlay clients, or
     as the server running UxPlay. Using AirPlay, UxPlay can
     emulate a second display for macOS clients.
   * Support for older iOS clients (such as 32-bit iPad 2nd gen. and
     iPhone 4S, when upgraded to iOS 9.3.5 or later), plus a
     Windows AirPlay-client emulator, AirMyPC.
   * Uses GStreamer plugins for audio and video rendering (with options
     to select different hardware-appropriate output "videosinks" and
     "audiosinks", and a fully-user-configurable video streaming pipeline).
   * Support for server behind a firewall.
   * **New**: Support for Raspberry Pi, with hardware video acceleration using
     Video4Linux2 (vl42), which supports both 32- and 64-bit systems, unlike deprecated
     OpenMAX (omx), which is being dropped by RPi distributions in favor of v4l2. (For GStreamer < 1.22,
     a [patch](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches)
     to the GStreamer Video4Linux2 plugin, available in the
     [UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki), is required, unless
     your distribution has made a backport of changes from the
     development branch.)
     See [success reports](https://github.com/FDH2/UxPlay/wiki/UxPlay-on-Raspberry-Pi:-success-reports:).

   * **New**: Support for running on Microsoft Windows (so far only tested on current Windows 10 64 bit,
     using MinGW-64 compiler in MSYS2 environment).

This project is a GPLv3 open source unix AirPlay2 Mirror server for Linux, macOS, and \*BSD.
It was initially developed by
[antimof](http://github.com/antimof/Uxplay) using code 
from OpenMAX-based [RPiPlay](https://github.com/FD-/RPiPlay), which in turn derives from
[AirplayServer](https://github.com/KqsMea8/AirplayServer),
[shairplay](https://github.com/juhovh/shairplay), and [playfair](https://github.com/EstebanKubata/playfair).
(The antimof site is no longer involved in
development, but periodically posts updates pulled from the new
main [UxPlay site](https://github.com/FDH2/UxPlay)). 

UxPlay is tested on a number of systems, including (among others) Debian 10.11 "Buster" and  11.2 "Bullseye",
Ubuntu 20.04 and 22.04, Linux Mint 20.3, Pop!\_OS 22.04 (NVIDIA edition), Rocky Linux 8.6 (a CentOS successor),
OpenSUSE 15.4, Arch Linux 5.16.8, macOS 12.3 (Intel and M1), FreeBSD 13.1.
On Raspberry Pi, it is tested on Raspberry Pi OS (Bullseye) (32- and 64-bit), Ubuntu 22.04, and Manjaro RPi4 22.04.

Its main use is to act like an AppleTV for screen-mirroring (with audio) of iOS/iPadOS/macOS clients
(iPhones, iPads, MacBooks) in a window
on the server display (with the possibility of
sharing that window on screen-sharing applications such as Zoom)
on a host running Linux, macOS, or other unix.  UxPlay supports Apple's AirPlay2
protocol using "Legacy Pairing", but some features are missing.
(Details of what is publically known about Apple's AirPlay 2 protocol can be found
[here](https://openairplay.github.io/airplay-spec/),
[here](https://github.com/SteeBono/airplayreceiver/wiki/AirPlay2-Protocol) and
[here](https://emanuelecozzi.net/docs/airplay2)).

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
a (reloading) graphics viewer of your choice such as `feh`:
run "`uxplay -ca <name> &`" in the background, then run "``feh -R 1 <name>``"
in the foreground; terminate with "`ctrl-C fg ctrl-C`".
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
using the icon for AirPlay video in apps such as the You Tube app
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
   NVIDIA's CUDA driver `libcuda.so` is installed.
   (This plugin should be used with options
   `uxplay -vd nvh264dec -vs glimagesink`.)     For GStreamer-1.16.3
   or earlier, replace `nvh264dec` by the older plugin `nvdec`, which
   must be built by the user:
   See [these instructions](https://github.com/FDH2/UxPlay/wiki/NVIDIA-nvdec-and-nvenc-plugins).

*  **Video4Linux2 support for the Raspberry Pi Broadcom GPU**

    Raspberry Pi (RPi) computers can run UxPlay with software decoding
    of h264 video but this usually has unacceptable latency, and hardware-accelerated
    GPU decoding should be used.  UxPlay accesses the GPU using the GStreamer
    plugin for Video4Linux2 (v4l2), which replaces unmaintained  32-bit-only  OpenMax used by
    RPiPlay. Fixes to the v4l2 plugin that allow it to
    work with UxPlay on RPi are now in the GStreamer development branch, and will appear
    in the upcoming GStreamer-1.22 release.
    A (partial) backport (as `gstreamer1.0-plugins-good-1.18.4-2+~rpt1`)
    has already appeared in RPi OS updates. Until the full update
    appears, or for other distributions, you can find
    [patching instructions for GStreamer](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches)
    in the [UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki) for GStreamer  1.18.4 and later.

### Note to packagers:

UxPlay's GPLv3 license does not have an added
"exception" explicitly allowing it to be distributed in compiled form when linked to OpenSSL versions
**prior to v. 3.0.0** (older versions of OpenSSL have a license clause incompatible with the GPL unless
OpenSSL can be regarded as a "System Library", which it is in *BSD).  Many Linux distributions treat OpenSSL
as a "System Library", but some (e.g. Debian) do not: in this case, the issue is solved by linking with OpenSSL-3.0.0  or later.

# Getting UxPlay:

* Your distribution may already provide a pre-built uxplay package.  It will be included in the next Debian release "Bookworm"
(currently in "testing" phase) and Ubuntu-22.04 already provides a uxplay-1.46 package based on this. Arch-based distributions also
have AUR self-building packages for both the latest UxPlay release and the current GitHub version.
To build the latest version yourself, follow the instructions below.

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
pkgconf.   Also make sure that cmake>=3.4.1 is installed:
"`sudo apt-get install cmake`" (add ``build-essential`` and `pkg-config`
(or ``pkgconf``)  to this if needed).

Make sure that your distribution provides OpenSSL 1.1.1 or later, and
libplist 2.0 or later. (This means Debian 10 "Buster", Ubuntu 18.04 or
later.) If it does not, you may need to build and install these from
source (see instructions at the end of this README).  If you have a non-standard OpenSSL
installation, you may need to set the environment variable OPENSSL_ROOT_DIR
(_e.g._ , "`export OPENSSL_ROOT_DIR=/usr/local/lib64`" if that is where it is installed).

In a terminal window, change directories to the source directory of the
downloaded source code ("UxPlay-\*", "\*" = "master" or the release tag for
zipfile downloads, "UxPlay" for "git clone" downloads), then follow the instructions below:

**Note:** By default UxPlay will be built with optimization for the
computer it is built on; when this is not the case, as when you are packaging
for a distribution, use the cmake option `-DNO_MARCH_NATIVE=ON`.

If you use Gstreamer older than 1.20, and wish to share the UxPlay screen using screen-sharing apps such
as Zoom, you should use the cmake option "` -DZOOMFIX=ON`" in step 3.  This requires
the X11 development libraries to be installed: on Debian-based systems do this with "`sudo apt-get install libx11-dev`" .
"ZOOMFIX" is not needed on macOS, or if you are using non-X11 windows 
(such as OpenGL) on Linux. See [ZOOMFIX compile-time option](#zoomfix-compile-time-option) below for more information,
and alternatives to "ZOOMFIX"

1. `sudo apt-get install libssl-dev libplist-dev`".
    (unless you need to build OpenSSL and libplist from source).
2.  `sudo apt-get install libavahi-compat-libdnssd-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev`. 
3.  `cmake .` (For a cleaner build, which is useful if you modify the source, replace this 
    by "``mkdir build; cd build; cmake ..``": you can then delete the
    `build` directory if needed, without affecting the source.)   Also add any cmake "`-D`" options
    here as needed (e.g, ZOOMFIX=ON or NO_MARCH_NATIVE=ON).
4. `make`
5. `sudo make install` (you can afterwards uninstall with ``sudo make uninstall``
    in the same directory in which this was run).

The above script installs the executable file "`uxplay`" to `/usr/local/bin`, (and installs a manpage to
somewhere like `/usr/local/share/man/man1` and README
files to somewhere like `/usr/local/share/doc/uxplay`).
The uxplay executable  can also be found in the build directory after the build
process, if you wish to test before installing (in which case
the GStreamer plugins must already be installed)

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
or AMD  graphics (but not for use with NVIDIA using proprietary drivers).
Also install "**tools**" to get the utility gst-inspect-1.0 for
examining the GStreamer installation.  If sound is not working, "**alsa**"", "**pulseaudio**",
or "**pipewire**" plugins may need to be installed, depending on how your audio is set up.

**Finally, run uxplay in a terminal window**.  Use Ctrl-C (or close the window) to terminate it when done. If it is not seen by the
iOS client's drop-down "Screen Mirroring" panel, check that your DNS-SD
server (usually avahi-daemon) is running: do this in a terminal window
with ```systemctl status avahi-daemon```.
If this shows the avahi-daemon is not running, control it
with ```sudo systemctl [start,stop,enable,disable] avahi-daemon``` (or
avahi-daemon.service). If UxPlay is seen, but the client fails to connect
when it is selected, there may be a firewall on the server that  prevents
UxPlay from receiving client connection requests unless some network ports
are opened. See [Troubleshooting](#troubleshooting) below for
help with this or other problems.

**One common problem involves GStreamer
attempting to use incorrectly-configured or absent accelerated hardware h264
video decoding (e.g., VAAPI).
Try "`uxplay -avdec`" to force software video decoding; if this works you can
then try to fix accelerated hardware video decoding if you need it, or just uninstall the GStreamer VAAPI plugin. If
your system uses the Wayland compositor for graphics, use "`uxplay -vs waylandsink`".**
See [Usage](#usage) for more run-time options.

### **Special instructions for Raspberry Pi (only tested on model 4B)**:

* For good performance, the Raspberry Pi needs the GStreamer Video4linux2 plugin  to use its Broadcom GPU hardware for decoding h264 video. 
You can also test UxPlay with software-only video decoding using option `-avdec`.


* The upcoming GStreamer-1.22 release will work well, but older releases of GStreamer will not work unless  patched with backports of the
improvements from GStreamer-1.22.   Patches for GStreamer-1.18.4 and later
are [available with instructions in the UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches).

The basic uxplay options for R Pi are ```uxplay -v4l2 [-vs <videosink>]```. The
choice `<videosink>` = ``glimagesink`` is sometimes useful. 
On a system without X11 (like R Pi OS Lite) with framebuffer video, use `<videosink>` = ``kmssink``.
With the  Wayland video compositor (as in  recent Ubuntu for R Pi) use `<videosink>` = ``waylandsink``.  For convenience,
these options are also available combined in options `-rpi`, ``-rpigl``
``-rpifb``, ```-rpiwl```, respectively provided for X11, X11 with OpenGL, framebuffer, and Wayland systems.
You may find the simple "uxplay", (which lets GStreamer try to find the best video solution by itself)
provides the best results.

* **If you are not using the latest patches from the wiki, you will also need to add the `-bt709` option**: previously the GStreamer v4l2 plugin could
not recognise Apple's color format (an unusual "full-range" variant of the bt709 HDTV standard), which -bt709 fixes.


* Tip: to start UxPlay on a remote host (such as a Raspberry Pi) using ssh:

```
   ssh user@remote_host
   export DISPLAY=:0
   nohup uxplay [options] > FILE &
```
  Sound and video will play on the remote host; "nohup" will keep uplay running if the ssh session is closed.  
  Terminal output is saved to FILE (which can be /dev/null to discard it).

### Non-Debian-based Linux or \*BSD

* **Red Hat, Fedora, CentOS (now continued as Rocky Linux or Alma Linux):** 
(sudo yum install) openssl-devel libplist-devel avahi-compat-libdns_sd-devel (some from the "PowerTools" add-on repository)
(+libX11-devel for ZOOMFIX). The required GStreamer packages (some from [rpmfusion.org](https://rpmfusion.org)) are:
gstreamer1-devel gstreamer1-plugins-base-devel gstreamer1-libav gstreamer1-plugins-bad-free (+ gstreamer1-vaapi for intel graphics). 

 * **OpenSUSE:**
(sudo zypper install) libopenssl-devel libplist-devel
avahi-compat-mDNSResponder-devel (+ libX11-devel for ZOOMFIX).  The required
GStreamer packages are: gstreamer-devel gstreamer-plugins-base-devel gstreamer-plugins-libav gstreamer-plugins-bad (+ gstreamer-plugins-vaapi for Intel graphics); in some cases, 
you may need to use gstreamer packages for OpenSUSE from [Packman](https://ftp.gwdg.de/pub/linux/misc/packman/suse/) "Essentials".


* **Arch Linux**
(sudo pacman -Syu) openssl libplist avahi  gst-plugins-base gst-plugins-good gst-plugins-bad gst-libav (+ gstreamer-vaapi
for Intel graphics). (**Also available as a package in AUR**).

 * **FreeBSD:** (sudo pkg install) libplist gstreamer1, gstreamer1-libav, gstreamer1-plugins, gstreamer1-plugins-*
(\* = core, good,  bad, x, gtk, gl, vulkan, pulse ...), (+ gstreamer1-vaapi for Intel graphics).
Either avahi-libdns or mDNSResponder must also be installed to provide the dns_sd library.
OpenSSL is already installed as a System Library.


## Building UxPlay on macOS:  **(Intel X86_64 and "Apple Silicon" M1/M2 Macs)**

_Note: A native AirPlay Server feature is included in  macOS 12 Monterey, but is restricted to recent hardware.
UxPlay can run  on older macOS systems that will not be able to run Monterey, or can run Monterey  but not AirPlay._

These instructions for macOS  assume that the Xcode command-line developer tools are installed (if Xcode is
installed, open the Terminal, type "sudo xcode-select --install" and accept the conditions).

It is also assumed that CMake >= 3.13 is installed:
this can be done with package managers [MacPorts](http://www.macports.org),
[Fink](http://finkproject.org) or [Homebrew](http://brew.sh), or by a download from
[https://cmake.org/download/](https://cmake.org/download/).

First install OpenSSL and libplist: static versions of these libaries will be used, so they can be uninstalled after UxPlay is built.
These are available in MacPorts and Homebrew, or they  can easily be built from source (see instructions at the end of this README; this
requires development tools autoconf, automake, libtool, which can be installed using MacPorts, HomeBrew, or Fink).


Next get the latest macOS release of GStreamer-1.0.

* recommended: install the "official" GStreamer release for macOS 
from [https://gstreamer.freedesktop.org/download/](https://gstreamer.freedesktop.org/download/).  The alternative is to install it from Homebrew
(MacPorts also supplies it, but compiled to use X11).

**For the "official" release**: install both the macOS runtime and development installer packages. Assuming that the latest release is 1.20.3.
install `gstreamer-1.0-1.20.3-universal.pkg` and ``gstreamer-1.0-devel-1.20.3-universal.pkg``.  (If
you have an Intel-architecture Mac, and  have problems with the "universal" packages, you can also
use `gstreamer-1.0-1.18.6-x86_64.pkg` and ``gstreamer-1.0-devel-1.18.6-x86_64.pkg``.)   Click on them to
install (they install to /Library/FrameWorks/GStreamer.framework).



**For Homebrew**:  pkgconfig is needed  ("brew install pkgconfig").
Then
"brew install gst-plugins-base gst-plugins-good gst-plugins-bad gst-libav".   This appears to be functionally equivalent
to using GStreamer.framework, but causes a large number of extra packages to be installed by Homebrew as dependencies.
**You may need to set the environment variable GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0 to point to the Homebrew GStreamer installation.**




Finally, build and install uxplay (without ZOOMFIX): open a terminal and change into the UxPlay source directory
("UxPlay-master" for zipfile downloads, "UxPlay" for "git clone" downloads) and build/install with
"cmake . ; make ; sudo make install " (same as for Linux).  

   * On macOS with this installation of GStreamer, the only videosinks available seem to be glimagesink (default choice made by
     autovideosink) and osxvideosink.   The window title does not show the Airplay server name, but the window is visible to
     screen-sharing apps (e.g., Zoom). The only available audiosink seems to be osxaudiosink.

   * The option -t _timeout_ is currently suppressed, and the option -nc is always used, whether or not it is selected.
     This is a workaround for a problem with GStreamer videosinks on macOS:
     if the GStreamer pipeline is destroyed while the mirror window is still open,  a segfault occurs.
   
   * In the case of glimagesink, the resolution settings "-s wxh" do not affect
     the (small) initial OpenGL mirror window size, but the window can be expanded using the mouse or trackpad.
     In contrast, a window created with "-vs osxvideosink" is initially big, but has the wrong aspect ratio (stretched image);
     in this case the aspect ratio changes when the window width is changed by dragging its side.


***Using GStreamer installed from MacPorts (not recommended):***

To install: "sudo port install pkgconfig"; "sudo port install gstreamer1-gst-plugins-base gstreamer1-gst-plugins-good gstreamer1-gst-plugins-bad gstreamer1-gst-libav".
**The MacPorts GStreamer is built to use X11**, so uxplay must be run from an XQuartz terminal, can use ZOOMFIX, and needs
option "-vs ximagesink".  On an unibody (non-retina)  MacBook Pro, the default resolution  wxh = 1920x1080 was too large,
but using option "-s 800x600" worked.  The MacPorts GStreamer pipeline seems fragile against attempts to change
the X11 window size, or to rotations that switch a connected client between portrait and landscape mode while uxplay is running. 
Using the MacPorts X11 GStreamer seems only possible if the image size is left unchanged from the initial "-s wxh" setting 
(also use the iPad/iPhone setting that locks the screen orientation against switching  between portrait and landscape mode
as the device is rotated).

## Building UxPlay on Windows (tested on Windows 10 64bit, using MSYS2 and MinGW-64 compiler)


1. Download and install  **Bonjour SDK for Windows v3.0** from the official Apple site
   [https://developer.apple.com/download](https://developer.apple.com/download/all/?q=Bonjour%20SDK%20for%20Windows)

2. (This is for the MSYS2 build enviroment; other build environments may also work, but are not yet tested):
   download and install MSYS2 from the official site https://www.msys2.org/

3. For building on Windows 64 bit, install the  **MinGW-64** compiler and cmake ([MSYS2 packages](https://packages.msys2.org/package/) are
   installed with a  variant of the "pacman" package manager adapted from Arch Linux).   After installation,  you can add this
   compiler  to your IDE. The compiler with all required dependencies is located in the msys64 directory, with
   default path `C:/msys64/mingw64`. Alternatively,  you can build UxPlay from the command line in the MSYS2 environment
   (this uses "`ninja`" in place of "``make``" for   the build system).

   To install and build from the command line, open a MSYS2 MinGW x64 terminal from the MSYS2 64 bit tab in the Windows Start menu, then run 

   `pacman -S mingw-w64-x86_64-cmake`
   
   `pacman -S mingw-w64-x86_64-gcc`

   `echo 'export PATH="/mingw64/bin/:$PATH"' >> ~/.bashrc`

    Now close the MSYS2 terminal window, and reopen a new one from the Start menu, to use the new PATH.
    
4.  Download latest UxPlay from github **(to use `git`, install it with ``pacman -S git``, 
    then "`git clone https://github.com/FDH2/UxPlay`")**, then install UxPlay dependencies:


    `pacman -S mingw-w64-x86_64-libplist mingw-w64-x86_64-openssl`
    
    `pacman -S mingw-w64-x86_64-gstreamer  mingw-w64-x86_64-gst-plugins-base`

    Note that libplist will be linked statically to the uxplay executable.
    It should also be possible to install gstreamer for Windows from the [offical GStreamer site](https://gstreamer.freedesktop.org/download/),
    especially if you are trying a different Windows build system.
5.  cd to the UxPlay source directory, then "`mkdir build`" and  "``cd build``", followed by

     `cmake ..`

     `ninja`

6.  Assuming no error in either of these, you will have built the uxplay executable **uxplay.exe** in the current ("build") 
    directory. The "sudo make install" and "sudo make uninstall" features offered in the other builds are not available 
    on Windows; instead, the MSYS2 environment has 
    `/usr/local/...` available, and you can install the uxplay.exe executable 
    in `/usr/local/bin` (plus manpage and documentation in ``/usr/local/share``) with

    `cmake --install . --prefix /usr/local`
    
    To be able to view the manpage, you need to install the manpage viewer with "`pacman -S man`", then give it the location of the uxplay manpage:
    
    `echo 'export "MANPATH=$MANPATH:/usr/local/share/man"' >> ~/.bashrc `
    
    (followed by "`source ~./bashrc`").

To run **uxplay.exe** you need to install gstreamer plugins with `pacman -S mingw-w64-x86_64-gst-<plugin>`, where ``<plugin>`` is

   1. **libav**
   2. **plugins-good**
   3. **plugins-bad**
  
Other possible MSYS2 gstreamer plugin packages you might use are listed in [MSYS2 packages](https://packages.msys2.org/package/).
   
You also will need to grant permission to the uxplay executable uxplay.exe to access data through the Windows firewall.  You may automatically 
be offered the choice to do this when you first run uxplay, or you may need to do it 
using **Windows Settings->Update and Security->Windows Security->Firewall & network protection -> allow an app through firewall**. If
your virus protection flags uxplay.exe as "suspicious" (but without a true malware signature) you may need to give it an exception.
 
Now test by running "`uxplay`" (in a MSYS2 terminal window).  If you
need to specify the audiosink, there are two main choices on Windows: the older DirectSound plugin "`-as directsoundsink`",
and the more modern Windows Audio Session API  (wasapi) plugin "`-as wasapisink`", which supports options such as

```
uxplay -as 'wasapisink low_latency=true device=\"<guid>\"' 
```
where `<guid>` specifies an available audio device by its GUID, which can be found using
"`gst-device-monitor-1.0 Audio`": ``<guid>`` has a form like ```\{0.0.0.00000000\}.\{98e35b2b-8eba-412e-b840-fd2c2492cf44\}```.


# Usage

Options:

**-n server_name** (Default: UxPlay);  server_name@_hostname_ will be the name that appears offering
   AirPlay services to your iPad, iPhone etc, where _hostname_ is the name of the server running uxplay. 
   This will also now be the name shown above the mirror display (X11)  window.

**-nh** Do not append "@_hostname_" at the end of the AirPlay server name.

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

**-fs** uses fullscreen mode, but only works with Wayland or VAAPI plugins.

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
   
**-vd _decoder_** chooses the GStreamer pipeline's h264 decoder element, instead of letting
   decodebin pick it for you.  Software decoding is done by avdec_h264; various hardware decoders
   include: vaapih264dec, nvdec, nvh264dec, v4l2h264dec (these require that the appropriate hardware is
   available).  Using quotes "..." allows some parameters to be included with the decoder name.

**-vc _converter_** chooses the GStreamer pipeline's videoconverter element, instead of the default
   value "videoconvert".  When using Video4Linux2 hardware-decoding by a GPU,`-vc  v4l2convert` will also use
   the GPU for video conversion.  Using quotes "..." allows some parameters to be included with the converter name.
   
**-vs _videosink_** chooses the GStreamer videosink, instead of letting
   autovideosink pick it for you.  Some videosink choices are:  ximagesink, xvimagesink,
   vaapisink (for intel graphics), gtksink, glimagesink, waylandsink, osximagesink (for macOS),  kmssink (for systems without X11, like Raspberry Pi OS lite) or
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

**-rpigl**  Equivalent to  "-v4l2 -vs glimagesink".  Sometimes better for "Desktop" Raspberry Pi systems with X11.

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

**-ca _filename_** provides a file (where _filename_ can include a full path) used for output of "cover art"
   (from Apple Music, _etc._,) in audio-only ALAC mode.   This file is overwritten with the latest cover art as
   it arrives.   Cover art (jpeg format) is discarded if this option is not used.    Use with a image viewer that reloads the image
   if it changes, or regularly (_e.g._ once per second.).    To achieve this,  run "`uxplay -ca [path/to/]filename &`" in the background,
   then run the the image viewer in the foreground.   Example, using `feh` as the viewer: run "``feh -R 1 [path/to/]filename``" (in
   the same terminal window in which uxplay was put into the background).   To quit, use ```ctrl-C fg ctrl-C``` to terminate
   the image viewer, bring ``uxplay`` into the foreground, and terminate it too.

**-reset n** sets a limit of n consecutive timeout failures of the client to respond to ntp requests
   from the server (these are sent every 3 seconds to check if the client is still present).   After
   n failures, the client will be presumed to be offline, and the connection will be reset to allow a new
   connection.   The default value of n is 5; the value n = 0 means "no limit" on timeouts.

**-nc** maintains previous UxPlay < 1.45 behavior that does **not close** the video window when the the client
   sends the "Stop Mirroring" signal. _This option is currently used by default in macOS,
   as the  window created in macOS by GStreamer does not terminate correctly (it causes a segfault)
   if it is still open when the GStreamer pipeline is closed._

**-FPSdata** Turns on monitoring of regular reports about video streaming performance
   that are sent by the client.  These will be displayed in the terminal window if this
   option is used.   The data is updated by the client at 1 second intervals.

**-fps n** sets a maximum frame rate (in frames per second) for the AirPlay
   client to stream video; n must be a whole number less than 256.
   (The client may choose to serve video at any frame rate lower
   than this;  default is 30 fps.)  A setting below 30 fps might be useful to
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
   a random MAC address will be used even if option **-m** was not specifed.
   (Note that a random MAC address will be different each time UxPlay is started).

**-t _timeout_**  will cause the server to relaunch (without stopping uxplay) if no connections
   have been present during the previous _timeout_ seconds.  You may wish to use this if the Server
   is not visible to new Clients that were inactive when the Server was launched, and an idle Bonjour
   registration  eventually becomes unavailable for new connections (this is a workaround for what
   may be due to a problem with your DNS-SD or Avahi setup).   _This option is currently disabled in
   macOS, for the same reason that requires the -nc option._

**-vdmp** Dumps h264 video to file videodump.h264.  -vdmp n dumps not more than n NAL units to
   videodump.x.h264; x= 1,2,... increases each time a SPS/PPS NAL unit arrives.   To change the name
   _videodump_,  use -vdmp [n] _filename_.

**-admp** Dumps  audio to file audiodump.x.aac (AAC-ELD format audio), audiodump.x.alac (ALAC format audio) or audiodump.x.aud
   (other-format audio), where x = 1,2,3... increases each time the audio format changes. -admp _n_ restricts the number of
   packets dumped to a file to _n_ or less.    To change the name _audiodump_, use -admp [n] _filename_.

**-d**  Enable debug output.   Note:  this does not show GStreamer error or debug messages.   To see GStreamer error
    and warning messages, set the environment variable GST_DEBUG with "export GST_DEBUG=2" before running uxplay.
    To see GStreamer debug messages, set GST_DEBUG=4; increase this to see even more of the GStreamer inner workings.

# Troubleshooting

Note: ```uxplay```  is run from a terminal command line, and informational messages are written to the terminal.

### 0.  Problems in compiling UxPlay.
    
One user (on Ubuntu) found compilation failed with messages about linking to "usr/local/lib/libcrypto.a"  and "zlib".
This was because (in addition to the standard ubuntu installation of libssl-dev), the user was unaware that a second installation
with libcrypto in /usr/local was present.
Solution: when more than one installation of OpenSSL is present, set the environment variable OPEN_SSL_ROOT_DIR to point to the correct one;
on 64-bit Ubuntu, this is done by
running `export OPENSSL_ROOT_DIR=/usr/lib/X86_64-linux-gnu/` before running cmake.

### 1. uxplay starts, but stalls after "Initialized server socket(s)" appears, *without any server name showing on the client*.

Stalling this way, with _no_  server name showing  _on  the client_ as available,
probably means that your network **does not have a running Bonjour/zeroconf DNS-SD server.**
On Linux, make sure Avahi is installed,
and start the avahi-daemon service on the system running uxplay (your distribution will document how to do  this).
Some  systems  may instead use the mdnsd daemon as an alternative to provide DNS-SD service.
_(FreeBSD offers both alternatives, but only Avahi was tested: one of the steps needed for
getting Avahi running on a FreeBSD system is to edit ```/usr/local/etc/avahi/avahi-daemon.conf```  to
uncomment a line for airplay support._)

After starting uxplay, use the utility ```avahi-browse -a -t``` in a different terminal window on the server to
verify that the UxPlay AirTunes and AirPlay services are correctly registered (only the AirTunes service is
used in the "Legacy" AirPlay Mirror mode  used by UxPlay).    If the UxPlay service is listed by avahi-browse, but is not seen by the client,
the problem is likely to be a problem with the local network.

### 2. uxplay starts, but stalls after "Initialized server socket(s)" appears, *with the server name showing on the client* (but the client fails to connect when the UxPlay server is selected).

This shows that a *DNS-SD* service  is working, but a firewall on the server is probably blocking the connection request from the client.
(One user who insisted that the firewall had been turned off turned out to have had _two_ active firewalls (*firewalld* and *ufw*)
_both_ running on the server!)  If possible, either turn off the firewall
to see if that is the problem, or get three consecutive network ports,
starting at port n, all three in the range 1024-65535, opened  for both tcp and udp, and use "uxplay -p n"
(or open UDP 6000, 6001, 6011 TCP 7000,7001,7100 and use "uxplay -p").

### 3. Problems _after_ the client-server connection has been made:

If you do _not_ see the message ``raop_rtp_mirror starting mirroring``, something went wrong before the client-server negotiations were finished.
For such  problems, use "uxplay -d " (debug log option)  to see what is happening: it will show how far the connection process gets before
the failure occurs.    You can compare your debug  output to
that from a successful start of UxPlay in the [UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki).

**If UxPlay reports that mirroring started, but you get no video or audio, the problem is probably  from  a
GStreamer plugin that doesn't work on your system** (by default,
GStreamer uses the "autovideosink"  and  "autoaudiosink" algorithms 
to guess what are  the "best" plugins to use on your system).

**Raspberry Pi** devices (-rpi option) only work with hardware GPU decoding if the Video4Linux2 plugin in  GStreamer v1.20.x or earlier has been patched
(see the UxPlay [Wiki](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches) for patches).
This may be fixed in the future when GStreamer-1.22 is released, or by backport patches in distributions such as Raspberry Pi OS (Bullseye).

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

You can try to fix audio problems by using the "-as _audiosink_"  option to choose the GStreamer audiosink , rather than
have autoaudiosink pick one for you.    The command "gst-inspect-1.0 | grep Sink | grep Audio" " will show you which audiosinks are 
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
(this is a GStreamer issue).  You may need to terminate uxplay with Ctrl-C to close a "zombie" OpenGl window.   If similar problems happen when 
the client sends the "Stop Mirroring" signal, try the no-close option "-nc" that leaves the video window open.

### 4. GStreamer issues (missing plugins, etc.): 

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

### 5.  Mirror screen freezes:

This can happen if the  TCP video stream from the client stops arriving at the server, probably because of network problems  (the UDP audio stream may continue to arrive).   At 3-second 
intervals, UxPlay checks that the client is still connected by sending it a request for a NTP time signal.   If a reply is not received from the client within a 0.3 sec 
time-window, an "ntp timeout" is registered.    If a certain number (currently 5) of consecutive ntp timeouts occur, UxPlay assumes that the client is "dead", and resets the connection, 
becoming available for connection to a new client, or reconnection to the previous one.   Sometimes the connection may recover before the timeout limit is reached, and if the
default limit is not right  for your network,  it can be modified using the option "-reset _n_", where _n_ is the desired timeout-limit value (_n_ = 0 means "no limit").    If the connection 
starts to recover after ntp timeouts, a corrupt video packet from before the timeout may trigger a "connection reset by peer"  error, which also causes UxPlay to reset the
connection. When the connection is reset, the "frozen" mirror screen of the previous connection is left in place, and will be taken over by a new client connection when it is made.

### 6. Protocol issues, such as failure to decrypt ALL video and audio streams from old or non-Apple clients:

A protocol failure may trigger an unending stream of error messages, and means that the
audio decryption key (also used in video decryption) 
was not correctly extracted from data sent by the  client.
This should not happen for iOS 9.3 or later clients.  However, if a client 
uses the same  older version of the protocol that is used by the Windows-based
AirPlay client emulator _AirMyPC_, the protocol can be switched to the older version
by the setting ```OLD_PROTOCOL_CLIENT_USER_AGENT_LIST``` 
in `UxPlay/lib/global.h`.
UxPlay reports the  client's "User Agent" string when it connects. If 
some other client also fails to decrypt all audio and video, try adding 
its "User Agent" string in place of "xxx" in the  entry "AirMyPC/2.0;xxx" 
in global.h and rebuild uxplay.

Note that Uxplay declares itself to be an AppleTV3,2 with a 
sourceVersion 220.68; this can also be changed in global.h. It had been thought
that it was necessary for UxPlay to claim to be an older 32 bit
AppleTV model that cannot run modern 64bit tvOS, in order for the client
to use a "legacy" protocol for pairing with the server.
However, UxPlay still works if it declares itself as an AppleTV6,2 with
sourceVersion 380.20.1 (an AppleTV 4K 1st gen, introduced 2017, running
tvOS 12.2.1); it seems that the use of "legacy" protocol just requires bit 27 (listed as
"SupportsLegacyPairing") of the
"features" plist code (reported to the client by the AirPlay server) to be set.
The "features" code and other settings are set in `UxPlay/lib/dnssdint.h`.

# Changelog
1.56 2022-09-01   Added support for building and running UxPlay-1.56 on Windows (github source only, no changes
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

### ZOOMFIX compile-time option

In GStreamer-1.18.6 and earlier, if UxPlay is using an X11 window for screen mirroring, this window is not visible
to screen-sharing apps like ZOOM.    OpenGL-based windows (use `-vs glimagesink` or ``-vs gtksink``, _etc._) do not have this problem.

A workaround is to  manually make the X11 window visible to screen-sharing apps with the X11 utility
xdotool, if it is installed, with: ``` xdotool selectwindow set_window --name <name>```
(where ```<name>``` is your choice of name), and then select the uxplay window
by clicking on it with the mouse.

However, if "`cmake -DZOOMFIX=ON .`"  is run before compiling,
the mirrored window is visible to screen-sharing applications, without this procedure.
To compile with ZOOMFIX=ON, the X11 development libraries must be installed. (Without ZOOMFIX,
UxPlay has no dependence on X11).

**ZOOMFIX is not needed in GStreamer-1.20 or later.**
Thanks to David Ventura
https://github.com/DavidVentura/UxPlay for the fix and also for getting a fix into  gstreamer-1.20.


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
from [https://github.com/libimobiledevice/libplist](https://github.com/libimobiledevice/libplist): get
[libplist-master.zip](https://github.com/libimobiledevice/libplist/archive/refs/heads/master.zip), then
("unzip libplist-master.zip ; cd libplist-master"), build/install
("./autogen.sh ; make ; sudo make install").   This will probably install libplist-2.0.* in /usr/local/lib.

_(Ignore the following for builds on MacOS:)_  On some systems like
Debian or Ubuntu, you may also need to add a missing  entry ```/usr/local/lib```
in /etc/ld.so.conf (or place a file containing "/usr/local/lib/libplist-2.0.so" in /etc/ld.so.conf.d)
and then run "sudo ldconfig". 




# Disclaimer

All the resources in this repository are written using only freely available information from the internet. The code and related resources are meant for educational purposes only. It is the responsibility of the user to make sure all local laws are adhered to.

This project makes use of a third-party GPL library for handling FairPlay. The legal status of that library is unclear. Should you be a representative of Apple and have any objections against the legality of the library and its use in this project, please contact me and I'll take the appropriate steps.

Given the large number of third-party AirPlay receivers (mostly closed-source) available for purchase, it is my understanding that an open source implementation of the same functionality wouldn't violate any of Apple's rights either.



# UxPlay authors

_[adapted from fdraschbacher's notes on  RPiPlay antecedents]_

The code in this repository accumulated from various sources over time. Here
is an attempt at listing the various authors and the components they created:

UxPlay was initially created by **antimof** from RPiPlay, by replacing its Raspberry-Pi-adapted OpenMAX  video 
and audio rendering system with GStreamer rendering for
desktop Linux systems (antimof's work on code in `renderers/` was later backported to RPiPlay).

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


