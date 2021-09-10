This project is an early stage prototype of unix AirPlay server.
Work is based on https://github.com/FD-/RPiPlay.
Tested on Ubuntu 19.10 desktop.
5G Wifi connection is the must.

Features:
1. Based on Gstreamer.
2. Video and audio are supported out of the box.
3. Gstreamer decoding is plugin agnostic. Uses accelerated decoders if
available. VAAPI is preferable. (but don't use VAAPI with nVidia)
4. Automatic screen orientation.

Getting it: (after sudo apt-get-install git cmake):

git clone https://github.com/FDH2/UxPlay.git   

This is a pull request on the
original site https://github.com/antimof/UxPlay.git ; it may or may not ever
get committed into the codebase  on the original antimof site, as the antimof
project may no longer be active.
If it has been  committed, replace "FDH2" by "antimof" in the above.

**Building this version** (Instructions for Ubuntu; adapt these for other
  Linuxes).
  
In a terminal window, change directories to the UxPlay directory of the
downloaded source code, then do


1. sudo apt-get install libssl-dev libplist-dev libavahi-compat-libdnssd-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-libav gstreamer1.0-plugins-bad 
2. sudo apt-get install gstreamer1.0-vaapi (For Intel graphics, but not nVidia graphics)
3. sudo apt-get install libx11-dev  (for the X_display name fix for screen-sharing with e.g.,  ZOOM)
4. mkdir build
5. cd build
6. cmake ..      (or "cmake -DZOOMFIX=ON .." to get a screen-sharing fix to
make the mirror display window visible to screen-sharing applications such as
Zoom, see below).
7. cd ..
8. make
9. sudo make install

Run uxplay in a terminal window.

**Note libplist-dev and (for ZOOMFIX) libx11-dev are new dependencies.**

**Red Hat, Fedora, CentOS:** 
(sudo yum install) openssl-devel libplist-devel avahi-compat-libdns_sd-devel (+libX11-devel for ZOOMFIX).   The required
GStreamer packages are:
gstreamer1-devel gstreamer1-plugins-base-devel gstreamer1-libav gstreamer1-plugins-bad-free ( + gstreamer1-vaapi for intel graphics).

**OpenSUSE:**
(sudo zypper install) libopenssl-devel libplist-devel
avahi-compat-mDNSResponder-devel (+ libX11-devel for ZOOMFIX).  The required
GStreamer packages are:
gstreamer-devel gstreamer-plugins-base-devel gstreamer-plugins-libav gstreamer-plugins-bad (+ gstreamer-plugins-vaapi for Intel graphics).

# **Troubleshooting:**

If uxplay starts, but stalls after "Initialized server socket(s)" appears,
it is probably because a firewall is blocking
access to the server on which it is running.  If possible, either turn off the firewall
to see if that is the problem, or get three consecutive network ports,
starting at port n, all three in the range 1024-65535, opened  for both tcp and udp, and use "uxplay -p n"
(or open UDP 6000, 6001, 6011 TCP 7000,7001,7100 and use "uxplay -p").

Try "uxplay -d " (debug log option)  to see what is happening. If you use an
nVidia graphics card, make sure that the gstreamer1.0-vaapi
plugin for Intel graphics is *NOT* installed (de-install it!).

If your mirror window has no title showing, the "ZOOMFIX" will not work.
The window is created by GStreamer, using a  videosink that the default "autovideosink" 
has chosen for you. Maybe an unusual videosink was chosen.   Fix: use the -vs option to make your own choice of videosink;
"-vs xvimagesink" or "-vs ximagesink" will create window titles on displays managed by X11.


#  **Usage:**

Options:
**-n server_name **;  server_name will be the name that appears offering
AirPlay services to your iPad, iPhone etc.
**NEW**: this will also now be the name shown above the mirror display  window, 

**-s wxh** (e.g. -s 1920x1080 , which is the default ) sets the display resolution (width and height,
   in pixels).   (This may be a
   request made to the AirPlay client, and perhaps will not
   be the final resolution you get.)   w and h are whole numbers with four
   digits or less.   Note that the **height** pixel size is the controlling
   one used by the client for determining the streaming format; the width is
   dynamically adjusted to the shape of the image (portait or landscape
   format, depending on how an iPad is held, for example). 

**-s wxh@r**  As above, but also informs the AirPlay  client about the screen
   refresh rate of the display. Default is r=60 (60 Hz); r is a whole number
   with three digits or less.   Values greater that 255 are ignored.
   
**-fps n ** sets a maximum frame rate (in frames per second) for the AirPlay
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

**-a** disable audio, leaving only the video playing.

Also: image transforms that had been added to RPiPlay have been ported to UxPlay:

**-f {H|V|I}**  implements "videoflip" image transforms: H = horizontal flip
(right-left flip, or mirror image); V = vertical flip ;  I =
180 degree rotation or inversion (which is the combination of H with V).

**-r {R|L}**  90 degree Right (clockwise) or Left (counter-clockwise)
   rotations; these are carried out after any **-f** transforms.

**-vs videosink** chooses the GStreamer videosink, instead of letting
   autovideosink pick it for you. For example, xvimagesink, vaapisink, or
   fpsdisplaysink (which shows the streaming framerate in fps).   Using quotes
   "..." might allow some parameters to be included with the videosink name. 
   (Some choices of videosink might not work on your system.)

# ChangeLog
1.341 2021-09-04   fixed: render_logger was not being destroyed by stop_server()
1.34  2021-08-27   Fixed "ZOOMFIX": the X11 window name fix was only being made the
                   first time the GStreamer window was created by uxplay, and
		   not if the server was relaunched after the GStreamer window
		   was closed, with uxplay still running.   Corrected in v. 1.34

# New features available: (v 1.32 2021-08-20)

1. Updates of the RAOP (AirPlay protocol)  collection of codes  maintained
at  https://github.com/FD-/RPiPlay.git so it is current as of 2021-08-01,
adding all changes since the original release of UxPlay by antimof.
This involved crypto updates, replacement
of the included plist library by the system-installed version, and  a change
over to a library llhttp for http parsing. 

2. Added the -s, -o -p, -m, -r,  -f,  -fps, and -vs  options.

3. If "cmake -DZOOMFIX=ON .."  is run before compiling,
the mirrored window is now visible to screen-sharing applications such as
Zoom. To compile with ZOOMFIX=ON, the X11 development libraries must be installed.
(ZOOMFIX will not be needed once the upcoming  gstreamer-1.20 is available,
since starting with that release, the GStreamer mirror window will be natively
visible for screen-sharing.) Thanks to David Ventura
https://github.com/DavidVentura/UxPlay for the fix
and also for getting it into  gstreamer-1.20.
[If uxplay was compiled after
cmake was run without -DZOOMFIX=ON, and your gstreamer version is older than
1.20, you can still manually make the window visible to screen-sharing apps with the X11 utility
xdotool, if it is installed, with: 
```
 xdotool selectwindow set_window --name <name>
```
, where ```<name>``` is your choice of name, and then select the uxplay window
by clicking on it with the mouse.]

4. The AirPlay server now terminates correctly when the gstreamer display window is
closed, and is relaunched with the same settings to wait for a new connection.
The program uxplay terminates when Ctrl-C is typed in the terminal window. 

5.   In principle, multiple instances of uxplay can be run simultaneously
using the **-m** (generate random MAC address) option to give each a
different ("local" as opposed to "universal")  MAC address.
If the **-p [n]** option is used, they also need separate network port choices.
(However, there may be a large latency, and running two instances of uxplay
simultaneously on the same computer may not be very useful; using -fps option
to force streaming framerates below 30fps could be useful.)

6.  Without the **-p** [n] option,  uxplay makes a random dynamic assignment of
network ports. This will not work if most ports are closed by a firewall.
With e.g., **-p 45000**   you should open both TCP and UDP on
ports 45000, 45001, 45002.   Minimum allowed port is 1024, maximum is 65535.
The option "**-p**" with no argument uses a "legacy" set of ports TCP 7100,
7000, 7001, and UDP  7011, 6000, 6001.

7.  The default resolution setting is 1920x1080 width x height pixels.
To change this, use "**-s wxh**"  where w and h are positive  decimals
with 4 or less digits.   It seems that the width and height may be negotiated
with the AirPlay client, so this may not be the actual screen geometry that
displays.

8. The title on the GStreamer display window is now is the AirPlay server name
(default "UxPlay", but can be changed with option **-n**), rather than the program
name "uxplay" (note the difference in capitalization).

9. The avahi_compat "nag" warning on startup is suppressed, by placing
"AVAHI_COMPAT_NOWARN=1" into the runtime environment when uxplay starts.
(This uses a call to putenv() in a form that is believed to be safe against
memory leaks, at least in modern Linux; if for any reason you don't want
this fix, comment out the line in CMakeLists.txt that activates it when uxplay
is compiled.)

10.  Allow choice (with -vs option) of the videosink that ends the GStreamer pipline. 

# Disclaimer

All the resources in this repository are written using only freely available information from the internet. The code and related resources are meant for educational purposes only. It is the responsibility of the user to make sure all local laws are adhered to.

This project makes use of a third-party GPL library for handling FairPlay. The legal status of that library is unclear. Should you be a representative of Apple and have any objections against the legality of the library and its use in this project, please contact me and I'll take the appropriate steps.

Given the large number of third-party AirPlay receivers (mostly closed-source) available for purchase, it is my understanding that an open source implementation of the same functionality wouldn't violate any of Apple's rights either.


# RPiPlay Authors (Notes by fdraschbacher, RPiPlay creator.)

(From the  https://github.com/FD-/RPiPlay.git repository.)

The code in this repository accumulated from various sources over time. Here
is my (__fdrachbacher__) attempt at listing the various authors and the components they created:

* **dsafa22**: Created an [AirPlay 2 mirroring server](https://github.com/dsafa22/AirplayServer)(seems gone now) for Android based on ShairPlay. This project is basically a port of dsafa22's code to the Raspberry Pi, utilizing OpenMAX and OpenSSL for better performance on the Pi. All code in `lib/` concerning mirroring is dsafa22's work. License: GNU LGPLv2.1+
* **Juho Vähä-Herttua** and contributors: Created an AirPlay audio server called [ShairPlay](https://github.com/juhovh/shairplay), including support for Fairplay based on PlayFair. Most of the code in `lib/` originally stems from this project. License: GNU LGPLv2.1+
* **EstebanKubata**: Created a FairPlay library called [PlayFair](https://github.com/EstebanKubata/playfair). Located in the `lib/playfair` folder. License: GNU GPL
* **Joyent, Inc and contributors**: Created an http library called [llhttp](https://github.com/nodejs/llhttp). Located at `lib/llhttp/`. License: MIT
* **Team XBMC**: Managed to show a black background for OpenMAX video rendering. This code is used in the video renderer. License: GNU GPL
* **Alex Izvorski and contributors**: Wrote [h264bitstream](https://github.com/aizvorski/h264bitstream), a library for manipulation h264 streams. Used for reducing delay in the Raspberry Pi video pipeline. Located in the `renderers/h264-bitstream` folder. License: GNU LGPLv2.1

# RPiPlay Changelog

### Version 1.2

* Blank screen after connection stopped

### Version 1.1

* Now audio and video work on Raspberry Pi Zero. I don't know what exactly did the trick, but static compilation seems to have helped.
* Smoother video due to clock syncing
* Correct lip-sync due to clock syncing
* Lower latency due to injecting max_dec_frame_buffering into SPS NAL 
* Disabled debug logging by default
* Added command line flag for debug logging
* Added command line flag for unsynchronized low-latency mode
* Bug fixes


# AirPlay protocol versions

For multiple reasons, it's very difficult to clearly define the protocol names and versions of the components that make up the AirPlay streaming system. In fact, it seems like the AirPlay version number used for marketing differs from that used in the actual implementation. In order to tidy up this whole mess a bit, I did a little research that I'd like to summarize here:


The very origin of the AirPlay protocol suite was launched as AirTunes sometime around 2004. It allowed to stream audio from iTunes to an AirPort Express station. Internally, the name of the protocol that was used was RAOP, or Remote Audio Output Protocol. It seems already back then, the protocol involved AES encryption. A public key was needed for encrypting the audio sent to an AirPort Express, and the private key was needed for receiving the protocol (ie used in the AirPort Express to decrypt the stream). Already in 2004, the public key was reverse-engineered, so that [third-party sender applications](http://nanocr.eu/2004/08/11/reversing-airtunes/) were developed.


Some time [around 2008](https://weblog.rogueamoeba.com/2008/01/10/a-tour-of-airfoil-3/), the protocol was revised and named AirTunes 2. It seems the changes primarily concerned timing. By 2009, the new protocol was [reverse-engineered and documented](https://git.zx2c4.com/Airtunes2/about/).


When the Apple TV 2nd generation was introduced in 2010, it received support for the AirTunes protocol. However, because this device allowed playback of visual content, the protocol was extended and renamed AirPlay. It was now possible to stream photo slideshows and videos. Shortly after the release of the Apple TV 2nd generation, AirPlay support for iOS was included in the iOS 4.2 update. It seems like at that point, the audio stream was still actually using the same AirTunes 2 protocol as described above. The video and photo streams were added as a whole new protocol based on HTTP, pretty much independent from the audio stream. Soon, the first curious developers began to [investigate how it worked](https://web.archive.org/web/20101211213705/http://www.tuaw.com/2010/12/08/dear-aunt-tuaw-can-i-airplay-to-my-mac/). Their conclusion was that visual content is streamed unencrypted.


In April 2011, a talented hacker [extracted the AirPlay private key](http://www.macrumors.com/2011/04/11/apple-airplay-private-key-exposed-opening-door-to-airport-express-emulators/) from an AirPort Express. This meant that finally, third-party developers were able to also build AirPlay reveiver (server) programs.


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
