# UxPlay 1.67：适用于Linux、macOS和Unix的隔空播放-镜像和隔空投放-音频服务器(现在也可在Windows上运行)。

### 现在已迁移到网站[https：//github.com/FDH2/UxPlay](https://github.com/FDH2/UxPlay),请在此投issues。
### 请积极改进翻译质量,尤其是针对专有名词的翻译
   * _**v1.67中的新功能**：支持一次性Apple风格的“pin”代码客户端身份验证(“客户端-服务器
     pairing") when the option "-pin" is used._
   
## 亮点：

   * 使用GPLv3许可证,开源。
   * 最初仅支持隔空播放-镜像协议,现在已添加对iOS/iPadOS的仅音频 (ALAC)传输支持  **不支持隔空投放2视频流协议,以后也不会支持。**
   * 运行macOS的设备(自2011年后的使用intel或M1,M2芯片的mac)既可以充当客户端,也可以运行uxplay来成为服务端。在另一台主机上运行Uxplay来扩展mac的显示屏。
   * 支持较旧的iOS客户端(如运行iOS 9.3.5以上的32位的iPad,iPod Touch第五代,iPhone 4S 和更新的64位设备),也支持模拟隔空投放的客户端,如AirMyPC。
   * 使用GStreamer插件进行音频和视频渲染(提供了大量的选项以便你完全自定义传输内容).
   * 支持使用防火墙的主机
   * Raspberry Pi有无硬件视频解码皆可(使用博通GPU进行硬件解码)。_在Raspberry Pi型号3B+、4B和5上测试。_
   * 支持在Microsoft Windows上运行(在unix-like MSYS2 环境下用 MinGW-64编译).

## 打包状态(Linux和\*BSD发行版)

[![当前打包状态](https://repology.org/badge/vertical-allrepos/uxplay.svg)](https：//repology.org/project/uxplay/versions)。

* 安装
在基于Debian的Linux系统上: `$ sudo apt install uxplay`
在FreeBSD上: $ `sudo pkg install uxplay`
也可通过AUR在基于Arch的系统上使用。
从版本1.66起,uxplay现在也在Fedora 38打包成RPM格式: $ sudo dnf install uxplay

* 对于其他尚未打包UxPlay的使用RPM打包格式的发行版,现在提供了RPM“specfile”**uxplay.spec**,[releases](https://github.com/FDH2/UxPlay/releases)(参见其“Assets”),也可以在UxPlay源代码根目录中找到。
请参阅[构建可安装的RPM包](#building-an-installable-rpm-package)一节打包。

安装后：

* (在Linux和\*BSD上)：如果在托管UxPlay的服务器上有防火墙,确保mDNS/DNS-SD查询的默认网络端口(UDP 5353)已打开(请参见[疑难解答](#troubleshooting));还要为Uxplay打开三个UDP和三个TCP端口,并使用“uxplay -p<n>“选项指定端口。(参见“`man uxplay`“或“``uxplay -h``“)

* 即使你安装了你的发行版的预编译的uxplay二进制包,你也可能需要阅读下面的说明用于[运行UxPlay](#running-uxplay),以查看您还应该安装哪个发行版的**GStreamer插件包**。

* 仅音频模式(Apple Music等)使用选项“`uxplay -async`”来获得最佳音频质量,但会有iOS带来的约2秒延迟。

* `~/.uxplayrc`是储存uxplay缺省选项的位置(参见“`man uxplay`“或“`uxplay -h`“查看编写格式和其他读取的位置)

* 在Raspberry Pi上：如果您使用Ubuntu 22.10或更早版本,GStreamer必须打[补丁](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches)以使用Broadcom GPU硬件视频解码
对于运行Raspberry Pi OS (Bullseye)且不想打补丁的用户,请使用选项“`uxplay -bt709`“

要从源代码编译最新的UxPlay,请参阅[获取UxPlay](#getting-uxplay)一节。

# UxPlay的详细描述
这个项目是一个GPL v3开源unix 隔空投放2镜像服务器,适用于Linux,macOS和BSD。它最初是由[antimof](http://github.com/antimof/Uxplay)基于[RPiPlay](https://github.com/FD-/RPiPlay)(它又是基于Open-MAX的源代码)的源代码改写,这反过来又可以追溯到[AirplayServer](https://github.com/KqsMea8/隔空投放Server),[shairplay](https://github.com/juhovh/shairplay)和[playfair](https://github.com/EstebanKubata/playfair)。
(The antimof网站不再参与开发,但定期从[UxPlay站点](https://github.com/FDH2/UxPlay)拉取并发布releases。

UxPlay已被测试于大量的系统, 包括 (包含的版本): Debian (10 "Buster", 11 "Bullseye", 12 "Bookworm"),‘Ubuntu(20.04 LTS,22.04 LTS,23.04(也是Ubuntu衍生发行版Linux Mint,Pop！OS)、Red Hat和其衍生发行版(Fedora 38,Rocky Linux 9.2),openSUSE Leap 15.5,Mageia 9,OpenMandriva“ROME”,PCLinuxOS,Arch Linux,Manjaro,并且应该在任何Linux系统上运行。还在macOS Catalina和Ventura(Intel)和Sonoma(M2),FreeBSD 14.0,Windows 10和11(64位)上进行了测试。

在Raspberry Pi 4型号B上,它在Raspberry Pi OS(Bullseye和Bookworm)(32位和64位)上进行了测试,
Ubuntu 22.04 LTS和23.04,Manjaro RPi4 23.02,以及openSUSE 15.5上的(无硬件视频解码)。
还测试了Raspberry Pi 3 Model B+和新的Model 5。

它的主要用途是像AppleTV一样对iOS/iPadOS/macOS客户端进行屏幕镜像(带音频)(iPhone、iPod Touch、iPad、Mac电脑)在服务端显示屏上运行Linux、macOS或其他unix(现在还有Microsoft Windows)的主机。UxPlay支持苹果的Airplay2协议,采用“传统协议”,但缺少一些功能。
(关于苹果Airplay2协议的公开信息可以在[在这里](https://openairplay.github.io/airplay-spec/),[这里](https://github.com/SteeBono/airplayreceiver/wiki/airplay2-Protocol)和[在这里](https://emanuelecozzi.net/docs/airplay2);也请见[pyatv](https://pyatv.dev/documentation/protocols),可能有新协议的文档。)虽然不能保证未来iOS版本将继续支持“传统协议”,但目前iOS 17继续支持。

UxPlay服务器及其客户端必须位于同一局域网上,**Bonjour/Zeroconf与mDNS/DNS-SD服务器**也在其上运行
(只有DNS-SD“服务发现”服务是严格必要的,它不是必要的
本地网络也可以是基于“.local”mDNS的类型)。
在Linux和BSD Unix服务器上,这通常由[Avahi](https://www.avahi.org)提供,
通过avahi-daemon服务,并且包含在大多数Linux发行版中(这
服务也可以由macOS、iOS或Windows服务器提供)。

连接到UxPlay服务器,
iOS/MacOS客户端可以在**隔空投放镜像**模式下启动(
有损压缩的AAC音频,同时镜像客户端屏幕,
或者在替代的**隔空投放音频**模式下,
Apple Lossless(ALAC)音频,无需屏幕镜像。 在**音频**模式下,
在所述Uxplay终端中显示元数据;
如果使用UxPlay选项``-ca<name>``,
还输出附带封面艺术
到一个按顺序更新的文件,并可以查看
一个(重新加载)图形查看器的选择。
在活动连接期间,在_**Mirror**_和_**Audio**_模式之间切换是
可能：在_**Mirror**_mode下,停止镜像(或关闭镜像窗口)并启动_**Audio**_mode连接,
通过启动_**Mirror**_mode连接切换回来;封面艺术显示在您离开/重新进入_**Audio**_mode时停止/重新启动。_

* **苹果视频DRM
(如在客户端上的“Apple TV”的视频)无法通过UxPlay解密,并且
无法使用UxPlay的隔空投放镜像模式观看Apple TV应用程序(只有未受保护的音频将以AAC格式流式传输),
但来自无DRM应用程序(如“YouTube应用程序”)的视频和音频内容都将通过UxPlay以镜像模式进行流式传输。**

* **由于UxPlay不支持非镜像隔空投放2视频流(其中
客户端控制隔空投放服务器上的Web服务器,
内容以避免其被客户端解码和重新编码),
在YouTube等应用程序中使用隔空投放视频图标
将只发送音频(无损的ALAC格式),而不发送附带的视频。**

### 使用硬件加速的h264视频解码的可能性,如果可用的话。

UxPlay使用[GStreamer](https://gstreamer.freedesktop.org)“插件”进行渲染
音频和视频。  这意味着视频和音频是“开箱即用”的,
使用插件的选择。  隔空投放以h264格式流式传输视频：gstreamer解码
是插件不可知的,并使用加速GPU硬件h264解码器,如果可用;
如果不是,则使用软件解码。

* **VAAPI for Intel and AMD integrated graphics, NVIDIA with "Nouveau" open-source driver**

   使用Intel或AMD GPU,使用开源VAAPI gstreamer进行硬件解码
   Plugin更好。   NVIDIA显卡的开源“新”驱动程序包括
   还原则上支持：[见此](https://nouveau.freedesktop.org/VideoAcceleration.html),但这需要
   VAAPI将补充从专有NVIDIA驱动程序中提取的固件。

* **带有专有驱动程序的NVIDIA**

   `nvh264dec`插件
   (包含在gstreamer 1.0-plugins-bad中,自GStreamer-1.18.0起)
   可用于加速NVIDIA GPU上的视频解码,
   NVIDIA的CUDA驱动程序`libcuda.so`已安装。 适用于GStreamer-1.16.3
   或者更早的时候,这个插件被称为`nvdec`,
   必须[由用户创建](https://github.com/FDH2/UxPlay/wiki/NVIDIA-nvdec-and-nvenc-plugins)。

*  **Video4Linux2支持Raspberry Pi Broadcom 2835 GPU(Pi 4B及更早版本)**

    Raspberry Pi (RPi) computers (tested on Pi 4 Model B) can now run UxPlay using software video decoding,
    but hardware-accelerated decoding by firmware in the Pi's 
    GPU is prefered.  UxPlay使用GStreamer-1.22 Video 4Linux 2(v4 l2)插件访问此功能;
    the plugin from older GStreamer < 1.22 needs a backport patch (already partially applied in
    Raspberry Pi OS (Bullseye), available for 1.18.4 and later
    in the [UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches)). 也
    requires the out-of-mainline Linux kernel module  bcm2835-codec  maintained by Raspberry Pi,
    so far only included in Raspberry Pi OS, and two other  distributions (Ubuntu, Manjaro) available
    with Raspberry Pi Imager.  _注意：最新的Raspberry Pi Model 5不提供
    GPU H264 解码,因为CPU足够强大到可以应付软解码。_


### Note to packagers:

UxPlay的GPL v3许可证没有添加
“GPL例外”明确允许它在链接到OpenSSL版本时以编译形式分发
**在v.3.0.0之前**(旧版本的OpenSSL具有与GPL不兼容的许可条款,除非
OpenSSL可以被看作是一个“系统库”,它在 *BSD中)。  许多Linux发行版对待OpenSSL
作为“系统库”,但有些(如Debian)不这样做：在这种情况下,问题是通过链接解决的
使用OpenSSL-3.0.0或更高版本。

# 获取UxPlay

下载并解压缩[UxPlay-master.zip](https://github.com/FDH2/UxPlay/archive/refs/heads/master.zip),
或者(如果安装了git)：“git clone https://github.com/FDH2/UxPlay“。   你
还可以下载列出的最新或更早版本
在[释放](https://github.com/FDH2/UxPlay/releases)。

* 最近的一个UxPlay也可以在原来的[antiof网站](https://github.com/antimof/UxPlay)上找到;
该原始项目是不活动的,但通常与
[活跃的UxPlay github站点](https://github.com/FDH2/UxPlay)(感谢antiof！)。

## 在Linux(或\*BSD)上构建UxPlay：

### 基于Debian的系统：

(这些说明适用于非基于Debian的Linux或 *BSD;对于macOS,
见下面的具体说明)。 有关以下方面的帮助,请参见下面[的故障排除](#troubleshooting)
任何困难。

你需要一个带有标准开发库的C/C++编译器(例如g++)
安装.  基于Debian的系统提供了一个“build-essential”包,
在编译软件。   您还需要pkg-config：如果找不到
通过“`which pkg-config`“,安装pkg-config或其类似替代品
pkgconf.   同时确保安装了cmake>=3.5：
“`sudo apt-get install cmake`“(添加build-essential和`pkg-config`
(or pkgconf),如果需要的话。

确保您的发行版提供OpenSSL 1.1.1或更高版本,
libplist 2.0或更高版本。 (This是指基于Debian 10“Buster”的系统(例如Ubuntu 18.04)或更新版本;
在Debian 10系统上,“libplist”是旧版本,您需要“libplist 3”。 如果它
不,您可能需要从
源代码(参见本自述文件末尾的说明)。  如果你有一个非标准的OpenSSL
安装时,您可能需要设置环境变量OPENSSL_ROOT_SSL
(例如： ,“`export OPENSSL_ROOT_DIR=/usr/local/lib64`”(如果是安装的话)。

* 大多数用户将使用其发行版提供的GStreamer,但少数(特别是用户
Raspberry Pi OS Lite Legacy(Buster)在Raspberry Pi型号4B上,
不受支持的遗留操作系统与其他应用程序的兼容性)应该从源代码构建一个较新的Gstreamer
遵循[这些指示](https://github.com/FDH2/UxPlay/wiki/Building-latest-GStreamer-from-source-on-distributions-with-older-GStreamer-(e.g.-Raspberry-Pi-OS-))。 . **这么做
在构建UxPlay之前。


在终端窗口中,将目录更改为
下载的源代码(“UxPlay-\*",“\*”=“master”或
zipfile下载,“UxPlay”用于“git clone”下载),然后按照以下说明操作：

**注意：**默认情况下,UxPlay将针对
它是建立在计算机上的;当情况并非如此时,例如当你在包装时,
对于发行版,请使用cmake选项`-DNO_MARCH_NATIVE=ON`。

如果您在Linux或 *BSD上使用X11 Windows,并希望通过按键切换进入/退出全屏模式
(F11或Alt_L+Enter)
UxPlay需要依赖于X11来构建。  从UxPlay-1.59开始,这将由
**如果**安装并检测到X11开发库,则为默认值。   安装这些与
“`sudo apt-get install libx11-dev`“。    If GStreamer < 1.20 is detected, a fix needed by
屏幕共享应用程序(例如, Zoom)也将被制作。

* 如果存在X11开发库,但您
希望构建*不*依赖于X11的UxPlay,请使用
cmake选项`-DNO_X11_DEPS=ON`。

1. `sudo apt-get install libssl-dev libplist-dev`“.
    (_unless you need to build OpenSSL and libplist from source_).
2.  `sudo apt-get install libavahi-compat-libdnssd-dev`
3.  `sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev`. (\*_如果您从源代码构建Gstreamer_,则跳过)
4.  `cmake .` (_对于更简洁的构建(如果您修改源代码,这很有用),请替换此
    by_ "``mkdir build; cd build; cmake ..``“：_然后您可以删除
    `build` directory if needed, without affecting the source._)   同时添加任何cmake“`-D`“选项
    here as needed (e.g, `-DNO_X11_DEPS=ON` or ``-DNO_MARCH_NATIVE=ON``).
5. `make`
6. `sudo make install`(你可以在以后使用sudo make uninstall卸载``
    in the same directory in which this was run).

这会将可执行文件“`uxplay`“安装到`/usr/local/bin`中(并安装一个手册页,
例如`/usr/local/share/man/man1`和README
文件到类似`/usr/local/share/doc/uxplay`的地方)。  (If“man uxplay”失败,检查是否设置了$MANPATH：
如果是这样,则需要将手册页的路径(通常是/usr/local/share/man/)添加到$MANPATH中。)
在构建后的build目录中也可以找到uxplay可执行文件
如果您希望在安装之前进行测试(在这种情况下
必须首先安装GStreamer插件)。



### 在非Debian Linux和\*BSD上构建
**对于那些使用基于RPM的发行版的用户,还可以使用RPM规范文件uxplay.spec：请参见
[构建一个可安装的rpm包](#building-an-installable-rpm-package)。

* **Red Hat,或者像CentOS这样的克隆(现在继续称为Rocky Linux或Alma Linux)：**
(sudo dnf install,or sudo yum install)openssl-devel libplist-devel avahi-compat-libdns_sd-devel
gstreamer 1-devel gstreamer 1-plugins-base-devel(+libX 11-devel for fullscreen X11)_(其中一些
可能在“CodeReady”附加存储库中,被克隆称为“PowerTools”)_

* **Mageia、PCLinuxOS、OpenMandriva：**
与Red Hat相同,除了名称更改：(Mageia)“gstreamer1.0-devel”,“gstreamer-plugins-base1.0-devel”;
(OpenMandriva)“libopenssl-devel”,“gstreamer-devel”,“libgst-plugins-base1.0-devel”.  PCLinuxOS：与Mageia相同,
但使用Synaptic(或APT)作为其包管理器。

 * **openSUSE：**
(sudo zypper install)libopenssl-3-devel(以前称为
 libopenssl-devel)libplist-2_0-devel(以前的libplist-devel)
avahi-compat-mDNSResponder-devel gstreamer-devel
gstreamer-plugins-base-devel(+ libX11-devel for fullscreen X11).

* **Arch Linux**(_在AUR中也可作为软件包提供_)：
(sudo pacman -Syu)openssl libplist avahi gst-plugins-base.

* **FreeBSD:** (sudo pkg install) libplist gstreamer1.
还必须安装avahi-libdns或mDNSResponder以提供dns_sd库。
OpenSSL已作为系统库安装。

#### 构建可安装的RPM包
第一次使用RPM构建器的人应该首先安装rpm-build和rpmdevtools包,
然后用“rpmdev-setuptree“创建rpmbuild树。`rpmdev-setuptree`  然后下载并
将uxplay.spec复制到~/rpmbuild/SPECS中。  在该目录中,运行“`rpmdev-spectool -g -R  uxplay.spec`“下载相应的
将源文件`uxplay-*.tar.gz`导入~/rpmbuild/SOURCES(“rpmdev-spectool”也可以称为“spectool”);然后
运行“`rpmbuild -ba uxplay.spec`“(您需要安装
任何所需的依赖性(这一报告)。  这应该会在`~/rpmbuild/RPMS`目录中创建uxplay RPM包。
(**uxplay.spec**在Fedora 38,Rocky Linux 9.2,openSUSE Leap 15.5,Mageia 9,OpenMandriva,PCLinuxOS上测试;
它可以很容易地修改,以包括其他基于RPM的发行版的依赖项列表。

## 运行UxPlay

### 安装插件(基于Debian的Linux系统)(_如果您从源代码构建了完整的GStreamer,则跳过_)

接下来安装`sudo apt-get install gstreamer1.0-<plugin>`所需的GStreamer插件。
所需的值为： 

1. “**插件库**“
2. “**libav**“(声音),
3. “**plugins-good**“(用于v4 l2硬件h264解码)
4. “**plugins-bad**“(用于h264解码)。

Plugins that may also be needed include "**gl** " for OpenGL support (which may be useful, and should
由NVIDIA GPU与h264解码一起使用),而“**x**“表示
X11支持,尽管这些可能已经安装;“**vaapi**“
英特尔硬件加速的h264视频解码需要
或AMD显卡(但不适用于使用专有驱动程序的NVIDIA)。 如果声音是
不工作,“**alsa**“",“**pulseaudio**“,或“**pipewire**“插件可能需要
安装,这取决于您的音频设置。

* 同时安装“**gstreamer1.0-tools**“以获取gst-inspect-1.0工具来检查GStreamer安装。


### Installing plugins (Non-Debian-based Linux or \*BSD) (_skip if you built a complete GStreamer from source_)

在某些情况下,由于专利问题,
在镜像模式下解码AAC音频所需的libav插件功能**avdec_aac**在官方发行版中没有提供：
从这些发行版的社区仓库中获取。

* **Red Hat,或者像CentOS这样的克隆(现在继续称为Rocky Linux或Alma Linux)：**
安装gstreamer 1-libav gstreamer 1-plugins-bad-free(+gstreamer 1-vaapi
英特尔/AMD显卡)。  在最近的Fedora中,gstreamer 1-libav被重命名为gstreamer 1-plugin-libav。
**要获取avdec_aac,请从[rpmfusion.org](https://rpmfusion.org)安装软件包**：(从rpmfusion获取ffmpeg-libs;
在RHEL或克隆上,但不是最近的Fedora,也可以从那里获得gstreamer 1-libav)。

* **Mageia、PCLinuxOS、OpenMandriva：**
安装gstreamer1.0-libav gstreamer1.0-plugins-bad(+ gstreamer1.0-vaapi
英特尔/AMD显卡)。 **在Mageia上,要获取avdec_aac,请从“污染”存储库安装ffmpeg**,
(它还提供了一个更完整的gstreamer1.0-plugins-bad)。

* **openSUSE：**
安装gstreamer-plugins-libav gstreamer-plugins-bad(+ gstreamer-plugins-vaapi
英特尔/AMD显卡)。  **要获取avdec_aac,请安装openSUSE的libav\* 包
来自[Packman](https://ftp.gwdg.de/pub/linux/misc/packman/suse/)“Essentials”;建议：在添加
Packman存储库,请使用YaST软件管理中的选项切换
多媒体的所有系统包给Packman)。

* **Arch Linux**
安装gst-plugins-good gst-plugins-bad gst-libav(+ gstreamer-vaapi
英特尔/AMD显卡)。

 * **FreeBSD:** Install gstreamer1-libav, gstreamer1-plugins, gstreamer1-plugins-*
(I * =核心、良好、不良、x、gtk、gl、vulkan、脉冲、v4l2、.), (+ gstreamer1-vaapi for Intel/AMD graphics).


### 启动和运行UxPlay 

自UxPlay-1.64起,UxPlay可以使用从配置文件读取的选项启动,这将是第一个找到的配置文件。
(1)一个文件,其路径由环境变量`$UXPLAYRC`给出,(2)~/.uxplayrc在用户的主页中
目录(“~“),(3)``~/.config/uxplayrc.  格式是每行一个选项,省略了
命令行选项。   配置文件中以`"#"`开头的行被视为注释并被忽略。

**在终端窗口中运行uxplay**。 在某些系统上,您可以切换到全屏模式和退出全屏模式
用F11或(按住左Alt键)+Enter键。  使用Ctrl-C(或关闭窗口)
完成后就终止它 如果UxPlay服务器未被
iOS客户端的下拉“屏幕镜像”面板,检查您的DNS-SD
服务器(通常是avahi-daemon)正在运行：在终端窗口中执行此操作
使用`systemctl status avahi-daemon`。
如果这显示avahi-daemon没有运行,请控制它
使用`sudo systemctl[启动、停止、启用、禁用]  avahi-daemon`(在非systemd系统上,例如\*BSD,
使用sudo service avahi-daemon[status,start,stop,restart,.]). 如果UxPlay是
看到,但客户端无法连接
如果选择此选项,则服务器上可能存在防火墙,
UxPlay接收客户端连接请求,除非某些网络端口
打开：如果防火墙处于活动状态,还应打开UDP端口5353(用于mDNS查询)
需要Avahi。 请参见下面的[故障排除](#troubleshooting),
帮助解决这个或其他问题。

* 从v1.67开始,UxPlay选项“`-pin`“允许客户端与UxPlay服务器“配对”
当他们第一次连接到它的时候,
显示在UxPlay终端上的4位PIN码。   (This是可选的,但如果客户端是
公司拥有和管理的设备,具有MDM移动终端管理功能。)   配对只发生一次,目前只有
记录在客户端中,并持续存在,除非
UxPlay公钥(存储在$HOME/.uxplay.pem中,如果使用了option`-key,则存储在<filename>`其他位置)被移动或删除,
从而生成新密钥。  (非Apple客户端可能不会实现持久性功能。

* 默认情况下,UxPlay被锁定为
直到客户端断开连接;从UxPlay-1.58开始,选项`-nohold`修改了这一点
行为,以便当新客户端请求连接时,它会删除当前客户端并接管。   UxPlay 1.66介绍
一种机制(`-restrict`、``-allow<id>``、```-block<id>```),用于控制允许哪些客户端连接,
“deviceID”(在Apple设备中似乎是不可变的)。

* 在镜像模式下,GStreamer可以选择**两种**方法来播放视频及其附带的音频：在UxPlay-1.64之前,
视频流和音频流在到达后尽快播放(GStreamer“_sync=false_”方法),
一个GStreamer内部时钟,用来保持它们的同步。    **Starting with UxPlay-1.64, the other method
(GStreamer的“_sync=true_”模式)使用客户端发送的音频和视频流中的时间戳,这是新的默认值。
在低解码功率的UxPlay主机(如Raspberry Pi 3型号)上,这将丢弃无法及时解码的视频帧
播放音频,使视频不稳定,但仍然同步。

不丢弃后期视频帧的旧方法
在更强大的系统上运行良好,并且仍然可以通过UxPlay选项“`-vsync no`“使用;这种方法适用于
到“直播”,并可能更好地使用时,UxPlay作为第二个显示器的Mac电脑,例如,而新的默认
基于时间戳的方法最适合观看视频,以保持嘴唇运动和声音同步。   (如果不使用时间戳,
如果解码速度不够快,视频最终会落后于音频：硬件加速视频解码有助于防止这种情况
以前没有使用时间戳时)。



* 在仅音频模式下,GStreamer“sync=false”模式(不使用时间戳)仍然是默认模式,但如果您想保留音频,
要在服务器上播放与客户端上显示的视频同步的视频,请使用基于时间戳的选项。`-async`  (An例子可能是
如果你想在客户端上跟随Apple Music的歌词,同时在UxPlay服务器上聆听卓越的声音)。   这
延迟客户端上的视频以匹配服务器上的音频,因此导致
在客户端上发起的暂停或音轨改变对服务器播放的音频生效之前的轻微延迟。 

The -vsync and -async options
还允许可选的正(或负)音频延迟调整(以毫秒为单位)进行微调：`-vsync 20.5`
音频相对于视频延迟0.0205秒;负值使其提前。)

* 你可能会发现视频是改善了设置-fps 60,允许一些视频播放60帧
每秒 (You可以通过使用-vs fpsDisplaysink和/或-FPSdata来查看实际流的帧速率。)
使用此选项时,应使用默认的基于时间戳的同步选项`-vsync`。

* 自UxPlay-1.54起,您可以在仅音频(ALAC)模式下显示Apple Music等来源的附带“封面艺术”：
在后台运行“`uxplay -ca  &`“,然后运行一个具有自动重装功能：的图像查看器：示例
是“feh”：运行“``feh -R1<name>``“
在前台;终止feh,然后使用“`ctrl-C fg ctrl-C`“进行Uxplay。

默认情况下,GStreamer使用一种算法来搜索要使用的最佳“videosink”(GStreamer对显示图像的图形驱动程序的术语)。
你可以使用uxplay选项`-vs来<videosink>`覆盖它。 哪些视频链接可用取决于您的操作系统,
图形硬件：使用“`gst-inspect-1.0| grep接收器| grep -e video -e Video -e image`“查看可用内容。   在Linux/\*BSD上的一些可能性是：

* **glimagesink**(OpenGL),**waylandsink**

* **xvimagesink**,**ximagesink**(X11)

* **kmssink**、**fbdevsink**(不带X11的控制台图形)

* **vaapisink**(用于Intel/AMD硬件加速显卡);对于NVIDIA硬件显卡(使用CUDA),使用**glimagesink**组合
  使用“`-vd nvh264dec`“(或“nvh 264 sldec”,这是一个新的变体,在GStreamer-1.24中将变为“nvh 264 dec”)。

GStreamer also searches for the best "audiosink"; override its choice  with `-as <audiosink>`. Linux上的选择包括
pulsesink、alsasink、pipewiresink、oss 4sink;查看gst-inspect-1.0提供的功能`|grep接收器| grep -e audio -e Audio`。

**一个常见的问题涉及GStreamer
尝试使用错误配置或缺少加速硬件h264
视频解码(例如, VAAPI)。
尝试“`uxplay -avdec`“来强制软件视频解码;如果这有效,你可以
然后尝试修复加速硬件视频解码,如果你需要它,或者只是卸载GStreamer vaapi插件。**

有关更多运行时选项,请参见[用法](#usage)。


### **Raspberry Pi的特殊说明(在R Pi 4型号B 8GB和R Pi 3型号B+上测试)**：

* 如果您使用纯软件(h264)视频解码UxPlay选项`-avdec`,它现在可以正常工作
比以前更好,新的默认的基于时间戳的同步,以保持音频和视频同步。

* 为了获得最佳性能,**型号5之前**的Raspberry Pi需要使用GStreamer Video4linux2插件
它的Broadcom GPU硬件用于解码h264视频。   这需要bcm2835_codec内核模块
它由Raspberry Pi在Linux内核之外维护,
[Raspberry Pi内核树](https://github.com/raspberrypi/linux),
已知提供R Pi的发行版仅包括Raspberry Pi OS、Ubuntu和Manjaro(全部可用
来自Raspberry Pi和他们的Raspberry Pi Imager)。  其他发行版一般不
提供它：**没有这个内核模块,UxPlay无法使用GPU中的解码固件。**

* 在运行不受支持的“Legacy”R Pi OS(Buster)的Raspberry Pi型号4B上,请注意,
这是一个非常旧的GStreamer-1.14.4**,无法修补以访问Broadcom GPU**。  如果你需要在无人支持的情况下
“Legacy”OS(“Lite”版本),但要使用UxPlay与硬件视频解码,您需要
首先从源代码构建一个完整的更新GStreamer
使用[这些指令](https://github.com/FDH2/UxPlay/wiki/Building-latest-GStreamer-from-source-on-distributions-with-older-GStreamer-(e.g.-Raspberry-Pi-OS-))。
在构建UxPlay之前。 请注意,运行Legacy OS的3B+ Pi型号可以通过GStreamer-1.14的omx插件访问GPU(使用选项“`-vd omxh264dec`“),但
这个插件在型号4B的固件上被破坏了,并且在R Pi OS(Bullseye)中删除了omx支持。


要使用GPU,请使用raspi-config“Performance Options”(在Raspberry Pi OS上,请在其他操作系统上使用类似工具)。
分配)为GPU分配足够的内存(在R. Pi 3型号B+,建议最大(256MB)。
即使使用GPU视频解码,一些帧可能会被较低功耗的3B+丢弃,以保持音频和视频同步
使用时间戳。

* 最新的GStreamer-1.22版本中的插件工作良好,但旧版本的GStreamer则不能
除非使用GStreamer-1.22的backports进行修补,否则无法正常工作。  Raspberry Pi OS(Bullseye)现在有一个
工作后门。 对于更完整的后端或其他发行版,GStreamer Video4Linux2插件的补丁
可以[在UxPlay Wiki中找到说明](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches)。

R Pi的基本uxplay选项是`[uxplay-vs<videosink>]`。 的
choice = glimagesink有时很有用。 
在没有X11的系统上(如R Pi OS Lite),使用帧缓冲区视频,使用= kmssink。
在Wayland视频合成器中,使用= waylandsink。  使用Video4Linux2时
插件访问硬件视频解码,选项`-v4l2`可能是有用的。
**您可能会发现,只有“`uxplay`“,(_without_ the-v4 l2 option,它让GStreamer
尝试自己找到最佳视频解决方案)
提供最佳结果(前一`-rpi*`选项相当于`-v4l2`或-v4 l2加上以下选项之一
“-vs”选项,在UxPlay v1.67中被删除,因为它们不适用于R Pi模型5。

* 如果您使用的是Raspberry Pi OS(靶心)和未修补的GStreamer-1.18.4中的Video 4Linux 2,
在UxPlay-1.56或更高版本中需要`-bt709`选项。
也不要使用选项`-v4l2`,因为
如果客户端屏幕旋转,则会导致崩溃。  (当最新的GStreamer-1.18.4
已应用UxPlay Wiki中的修补程序。)

* 提示：要使用ssh在远程主机(如Raspberry Pi)上启动UxPlay：

```
   ssh user@remote_host
   export DISPLAY=:0
   nohup uxplay [options] > FILE &
```
  声音和视频将在远程主机上播放;如果ssh会话是
  关门了  终端输出保存到FILE(可以是/dev/null以丢弃它)

## 在macOS上构建UxPlay：**(Intel X86_64和“Apple Silicon”M1/M2 Mac)**

_注意：macOS 12 Monterey中包含本机隔空投放服务器功能,但仅限于最新硬件。
UxPlay可以在无法运行Monterey的旧版macOS系统上运行,或者可以运行Monterey但不能运行隔空投放。

这些针对macOS的说明假定已安装Xcode命令行开发人员工具(如果已安装Xcode)。
安装后,打开终端,输入“sudo xcode-select --install”并接受条件)。

还假设安装了CMake >= 3.13：
这可以通过包管理器[MacPorts](http://www.macports.org)(`sudo port install cmake`)来完成,
[Homebrew](http://brew.sh)(`brew install cmake`),或从
[https：//cmake.org/download/](https://cmake.org/download/).    Also install `git` if you will use it to fetch UxPlay.

接下来安装libplist和openssl-3.x。  请注意,这些库的静态版本将
在macOS构建中使用,因此如果您愿意,可以在构建uxplay后对其进行重新配置。

*  如果你使用Homebrew：`brew install libplist openssl@3`

* 如果使用MacPorts：`sudo port install libplist-devel openssl3`

否则,从源代码构建libplist和openssl：请参阅本README末尾的说明;
需要开发工具(autoconf,automake,libtool等) 待安装。


接下来获取最新的macOS版本GStreamer-1.0。

**使用“官方”GStreamer(建议MacPorts和Homebrew用户使用)**：安装
macOS的GStreamer版本
来自[https：//gstreamer.freedesktop.org/download/](https://gstreamer.freedesktop.org/download/)。
(This release包含自己的pkg-config,
所以你不需要安装一个)。 安装gstreamer-1.0和gstreamer-1.0-devel包。  下载后,按Shift键点击它们
安装(它们安装到/Library/FrameWorks/GStreamer.framework)。   Homebrew或MacPorts用户**不**应安装(或应卸载)
如果他们使用的是“官方”发行版,则使用他们的包管理器提供的GStreamer。

* 从GStreamer v1.22开始,“Official”(gstreamer.freedesktop.org)macOS二进制文件需要包装器“gst_macos_main”
围绕实际的主程序(uxplay)。 这应该在UxPlay编译过程中应用,
初始UxPlay终端消息应当确认其正在被使用。 (UxPlay也可以使用“官方”GStreamer v.1.20.7构建
二进制文件,无需包装器即可工作。)

**使用Homebrew的GStreamer**：pkg-config is needed：(“brew install pkg-config gstreamer”)。
这导致大量额外的软件包被Homebrew作为依赖项安装。
[Homebrew gstreamer安装](https://formulae.brew.sh/formula/gstreamer#default)最近已被
重新加工成一个名为`gstreamer`单一“公式”,它现在不需要GST_PLUGIN_PATH被
设置在环境中。  Homebrew将gstreamer安装到`(HOMEBREW)/lib/gstreamer-1.0`,其中(HOMEBREW)/* 是
`/opt/homebrew/*` on Apple Silicon Macs, and ``/usr/local/*``(在Intel Mac上);不要将任何
额外的非Homebrew插件(你自己构建的),而不是设置GST_PLUGIN_PATH指向
他们的位置(Homebrew不提供完整的GStreamer,但似乎有UxPlay所需的一切)。


**使用从MacPorts安装的GStreamer**：**不**建议这样做,因为目前MacPorts GStreamer
是旧的(v1.16.2),未维护,并构建为使用X11：

 * 而是[自己构建gstreamer](https://github.com/FDH2/UxPlay/wiki/Building-GStreamer-from-Source-on-macOS-with-MacPorts)
如果你使用MacPorts并且不想使用“官方”Gstreamer二进制文件。

_(如果你真的想使用MacPorts GStreamer-1.16.2,
install pkgconf(“sudo port install pkgconf”),然后
“sudo port install gstreamer1-gst-plugins-base gstreamer1-gst-plugins-good gstreamer1-gst-plugins-bad gstreamer1-gst-libav”.
对于macOS上的X11支持,使用特殊的cmake选项`-DUSE_X11=ON`编译UxPlay,然后运行
它来自一个带有-vs ximagesink的XQuartz终端;较老的非视网膜Mac需要较低的分辨率
使用X11时：`uxplay -s 800x600`.)_



安装GStreamer后,构建并安装uxplay：打开一个终端,切换到UxPlay源目录
(“UxPlay-master”用于zipfile下载,“UxPlay”用于“git clone”下载)并使用
“cmake。 ; make ; sudo make install“(与Linux相同)。

   * 在检查GStreamer警告时运行UxPlay(在运行UxPlay之前使用“export GST_DEBUG=2”执行此操作),
     that with the default (since UxPlay 1.64) use of timestamps for video synchonization, many video frames are being dropped
     (only on macOS), perhaps due to another error (about videometa) that shows up in the GStreamer warnings.   **建议：
     use the new UxPlay "no timestamp" option "`-vsync no`"** (you can add a line "vsync no" in the uxplayrc configuration file).

   * 在安装了GStreamer的macOS上,唯一可用的videosink似乎是glimagesink(由
     autovideosink) and osxvideosink.   窗口标题不显示隔空投放服务器名称,但该窗口对
     screen-sharing apps (e.g., 缩放)。 唯一可用的audiosink似乎是osxaudiosink。

   * 无论是否选中,始终使用选项-nc。
     This is a workaround for a problem with GStreamer videosinks on macOS:
     if the GStreamer pipeline is destroyed while the mirror window is still open,  a segfault occurs.
   
   * 对于glimagesink,分辨率设置“-s wxh”不影响
     the (small) initial OpenGL mirror window size, but the window can be expanded using the mouse or trackpad.
     In contrast, a window created with "-vs osxvideosink" is initially big, but has the wrong aspect ratio (stretched image);
     in this case the aspect ratio changes when the window width is changed by dragging its side;
     the option "-vs osxvideosink force-aspect-ratio=true" can be used to make the window have the
     correct aspect ratio when it first opens.

## 在Microsoft Windows上构建UxPlay,使用MSYS 2和MinGW-64编译器。

* 在Windows 10和11上测试,64位。

1. 下载并安装**Bonjour SDK for Windows v3.0**。  无需注册即可下载SDK
   [softpedia.com](https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml)或从Apple官方网站获取
   网站[https：//developer.apple.com/download](https://developer.apple.com/download/all/?q=Bonjour%20SDK%20for%20Windows)(苹果公司制造
   你注册为开发人员,从他们的网站访问它)。  这应该将Bonjour SDK安装为`C:\Program Files\Bonjour SDK`。

2. (This适用于64位Windows;适用于32位Windows的版本应该是可能的,但尚未经过测试。) 的
   将使用类Unix的MSYS2构建环境：从官方下载并安装MSYS2
   网址：[https：//www.msys2.org/](https://www.msys2.org)。  接受默认安装位置`C:\mysys64`。

3. [MSYS2软件包](https://packages.msys2.org/package/)安装时使用
   Arch Linux使用的“pacman”包管理器的变体。  打开“MSYS2 MINGW 64”终端
   从Windows“开始”菜单的MSYS2选项卡中,
   使用“pacman -Syu”安装MSYS 2。  然后安装**MinGW-64**编译器,   **CMake**
      
   ```
   pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc
   ```

   带有所有必需依赖项的编译器将安装在msys64目录中,
   默认路径`C:/msys64/mingw64`。  这里我们将简单地从命令行构建UxPlay
   在MSYS2环境中(使用“`ninja`“代替“``make``“,   构建系统)。

4. 从github下载最新的UxPlay(要使用`git`,请使用pacman -S git安装,
   然后“`git clone https://github.com/FDH2/UxPlay`“),然后安装UxPlay依赖项(openssl已经
   安装MSYS2)：

   `pacman -S mingw-w64-x86_64-libplist mingw-w64-x86_64-gstreamer mingw-w64-x86_64-gst-plugins-base`

    Note that libplist will be linked statically to the uxplay executable.
    If you are trying a different Windows build system, MSVC versions of GStreamer
    for Windows are available from the [official GStreamer site](https://gstreamer.freedesktop.org/download/),
    but only the MinGW 64-bit build on MSYS2 has been tested.
    
5.  cd到UxPlay源目录,然后“`mkdir build`“和“cd build“。  构建过程假设
    the Bonjour SDK is installed at `C:\Program Files\Bonjour SDK`.   如果是其他地方,设置环境
    variable BONJOUR_SDK_HOME to point to its location.   然后使用

     `cmake ..`

     `ninja`

6.  假设这两个都没有错误,您将在****
    current ("build")  directory. 中提供的“sudo make install”和“sudo make uninstall”功能
    other builds are not available on Windows; instead, the MSYS2 environment has 
    `/mingw64/...` 可用,您可以安装uxplay.exe可执行文件
    in `C:/msys64/mingw64/bin` (plus manpage and documentation in ``C:/msys64/mingw64/share/...``) 与

    `cmake --install . --prefix /mingw64`
    
    To be able to view the manpage, you need to install the manpage viewer with "`pacman -S man`".

要运行**uxplay.exe,**您需要安装一些gstreamer插件包
使用`pacman -S mingw-w 64-x86_64-gst-<plugin>`,其中所需的值由以下公式``<plugin>``给出：

   1. **利巴夫**
   2. **插件-好**
   3. **插件坏**
  
列出了您可能使用的其他可能的MSYS2 gstreamer插件包
在[MSYS2包](https://packages.msys2.org/package/)中。
   
您还需要向uxplay可执行文件uxplay.exe授予通过Windows访问数据的权限
防火墙  当您第一次运行uxplay时,系统可能会自动为您提供选择,或者您可能需要这样做
使用Windows设置->更新和安全->Windows安全->防火墙和网络保护->允许应用程序
通过防火墙。 如果您的病毒防护将uxplay.exe标记为“可疑”(但没有真正的恶意软件签名)
你可能需要破例一次
 
现在通过运行“`uxplay`“(在MSYS2终端窗口中)进行测试。  如果你
需要指定audiosink,在Windows上有两个主要选择：旧的DirectSound
插件“`-as directsoundsink`“,以及更现代的Windows Audio Session API(wasapi)
插件“`-as wasapisink`“,它支持[其他选项](https://gstreamer.freedesktop.org/documentation/wasapi/wasapisink.html),例如

```
uxplay -as 'wasapisink device=\"<guid>\"' 
```
其中,通过可用的音频设备的类型指定可用的音频设备,可以使用
“`gst-device-monitor-1.0 Audio`“：``<guid>``有一个表单
比如"0.0.0.0000000"。{98e35b2b-8eba-412e-b840-fd2c2492cf44\}`。 如果未指定“`device`“,
使用默认音频设备。

如果您希望使用`-vs<videosink>`选项指定videosink,可以选择以下选项
`d3d11videosink`,d3dvideosink,`glimagesink`,
`gtksink`。   使用Direct 3D 11.0或更高版本,您可以使用Alt-Enter组合键和
选项`-vs "d3d11videosink fullscreen-toggle-mode=alt-enter"`。  为方便起见,如果只使用-vs d3 d11 videosink(单独使用),则会添加此选项。
(You可能希望将“vs d3 d11 videosink“(无首字母“`-`“)添加到UxPlay启动选项文件中;参见“man uxplay”或“uxplay -h”。

可执行文件uxplay.exe也可以在没有MSYS2环境的情况下运行,
Windows终端,使用`C:\msys64\mingw64\bin\uxplay`。

# 使用

选项：

* 这些也可以写在UxPlay启动文件中(每行一个选项,没有初始“`-`“字符)(由
环境变量`$UXPLAYRC`或``~/.uxplayrc``或```~/.config/uxplayrc```);行开始
带“`#`“的字符被视为注释,并被忽略。  命令行选项将取代启动文件中的选项。

**-n server_name**(默认值：UxPlay); server_name@_hostname_将显示提供
   隔空投放服务到您的iPad,iPhone等,其中_hostname_是运行uxplay的服务器的名称。
   这也将是镜像显示(X11)窗口上方显示的名称。

**-nh**不要在隔空投放服务器名称的末尾追加“@_hostname_”。

**-pin[nnnn]**：(自v1.67起)当新客户端首次连接时,使用Apple风格的(一次性)“pin”身份验证：
   显示在终端上,并且客户端屏幕显示要输入的登录提示。    When "-pin" is used by itself, a new random
   为每个认证选择PIN码;如果“-pinnnnn”(例如, “-pin 3939”),这将设置一个不变的固定代码。
   为了在UxPlay重新启动后保留客户端身份验证,在第一次使用“-pin”时,服务器公钥将写入文件(默认值：$HOME/.uxplay.pem;可以
   用选项键改变),`<filename>`
   并在随后的UxPlay初创公司中重新阅读。  只要不删除或移动此文件,
   客户端将不必在初始认证之后重新认证。   _(如果您希望在UxPlay启动文件中添加“pin”条目,
   UxPlay服务器使用此协议)。

**-vsync[x]**(在镜像模式下：)此选项(**现在是默认值**)使用时间戳来同步服务器上的音频和视频,
   可选音频延迟(十进制)毫秒   (_x_ =“20.5”)   表示0.0205秒延迟：正值或
   允许小于一秒的负延迟)。   它需要在低功耗系统上使用,例如没有硬件的Raspberry Pi
   视频解码

**-vsync no**(在镜像模式下：)这将关闭基于时间戳的音频-视频同步,恢复之前的默认行为。
UxPlay-1.64. 标准的桌面系统似乎在不使用时间戳的情况下也能很好地工作：这种模式适用于“实时流”,
使用UxPlay作为Mac电脑的第二个显示器,或监控网络摄像头;有了它,没有视频帧被丢弃。

**- c[x]**(在仅音频(ALAC)模式下：)此选项使用时间戳将服务器上的音频与客户端上的视频同步,
   可选音频延迟(十进制)毫秒   (_x_ =“20.5”)   表示0.0205秒延迟：正值或
   允许小于一秒的负延迟)。   因为客户端添加了一个视频
   delay为了解决延迟问题,服务器在-dec模式下会添加一个等效的音频延迟,这意味着
   音频更改(如暂停或音轨更改)将不会生效
   立即  原则上,通过使用`-al`音频延迟设置来更改延迟(默认为0.25秒),可以缓解此问题(_A)
   服务器向客户端报告,但目前改变这一点似乎没有任何效果。

**- 不是**。   这仍然是仅音频模式下的默认行为,但此选项可能作为命令行选项用于关闭
在“uxplayrc”配置文件中设置`-async`选项。

**-s wxh**(例如-s 1920 x1080,这是默认值)设置显示分辨率(宽度和高度,
   以像素为单位)。   (This可以是
   向隔空投放客户端发出的请求,并且可能不会
   这是你最终的决定)。   w和h是带4的整数
   数字或更少。   请注意,**高度**像素大小是控制
   一个由客户端用于确定流格式;宽度为
   根据图像的形状(肖像或风景)动态调整
   格式,取决于如何持有iPad,例如)。

**-s wxh@r**  As above, but also informs the 隔空投放  client about the screen
   显示器的刷新率。 默认值为r=60(60 Hz); r必须为整数
   低于256。

**-o**打开显示窗口的“overscanned”选项。    This
   通过使用所需的某些像素来降低图像分辨率
   通过选项-s wxh(或其默认值1920 x1080)添加空
   未使用像素的边界帧(在全屏中会丢失
   过扫描的显示,并且不被GStreamer显示)。
   建议：**不要使用此选项,**除非有一些特殊的
   有理由使用它。

**-fs**使用全屏模式,但仅适用于X11、Wayland或VAAPI。

**-p**允许您选择UxPlay使用的网络端口(这些端口需要
   如果服务器在防火墙后面,则打开)。   本身,-p集
   传统端口TCP 7100、7000、7001、UDP 6000、6001、7011。   -p n(例如-p
   35000)设置TCP和UDP端口n、n+1、n+2。  -p n1,n2,n3(逗号分隔
   值)分别设置每个端口; -p n1,n2设置端口n1,n2,n2+1。  -p tcp n
   或-p udp n仅设置TCP或UDP端口。  端口必须在范围内
   [1024-65535]。

如果未使用-p选项,则会动态(随机)选择端口,
如果防火墙正在运行,则这将不起作用。

**-avdec**强制使用软件h264解码,使用Gstreamer元素avdec_h264(libav h264解码器)。 这
   选项应该阻止autovideosink选择硬件加速的videosink插件,如vaapisink。
   
**-vp _parser_**choses GStreamer pipeline的h264 parser元素,默认为h264 parse。 使用
   引用“...“允许添加选项。
   
**-vd _decoder_**选择GStreamer管道的h264解码器元素,而不是默认值
   “decodebin”为您选择它。  软件解码由avdec_h264完成;各种硬件解码器
   包括：vaapih264dec、nvdec、nvh264dec、v4l2h264dec(这些要求适当的硬件
   可用)。  使用引号“...“允许解码器名称中包含一些参数。

**-vc _converter_**选择GStreamer管道的视频转换器元素,而不是默认的
   值“videoconvert”。  当使用GPU的Video 4Linux 2硬件解码时,`-vc  v4l2convert`也将使用
   GPU视频转换  使用引号“...“允许将某些参数包含在转换器名称中。
   
**-vs _videosink_**选择GStreamer videosink,而不是默认值
   “autovideosink”为您选择它。  一些videosink选择是：ximagesink,xvimagesink,
   vaapisink(用于英特尔显卡)、gtksink、glimagesink、waylandsink、osxvideosink(用于macOS)、kmsink(用于
   没有X11的系统,如Raspberry Pi OS lite)或
   fpsDisplaysink(以fps为单位显示流帧速率)。   使用引号
   “...“允许将某些参数包含在videosink名称中。
   例如,vaapisink插件支持**全屏**模式,
   使用-vs“vaapisink fullscreen=true”获得;这也适用于waylandsink。
   这些选项的语法是特定于一个给定的插件,和videosink的一些选择
   你的系统可能不工作。

**-vs 0**禁止显示流式视频,但播放流式音频。   (The客户端屏幕
   仍以每秒1帧的降低速率镜像,但不渲染或显示。)  这
   功能(以AAC音频格式流式传输音频)现在可能不需要了,因为UxPlay现在可以
   在隔空投放非镜像模式下流式传输无视频的优质Apple Lossless音频。

**-v4l2**Video settings for hardware h264 video decoding in the GPU by Video4Linux2.  相当于
   `-vd v4l2h264dec -vc v4l2convert`。

**- bt 709**解决旧版Video 4Linux 2插件无法识别Apple的
   使用数字电视的bt709色彩标准的一种不常见的(但被允许的)“全范围色彩”变体。
   GStreamer-1.20.4不再需要它,并从它进行反向移植。

**-rpi**等效于“-v4 l2“(不适用于Raspberry Pi型号5,并在UxPlay 1.67中删除)

**-rpigl**相当于“-rpi -vs glimagesink”。 (自UxPlay 1.67起移除)

**-rpifb**等效于“-rpi -vs kmssink”(自UxPlay 1.67起删除)

**-rpiwl**相当于“-rpi -vs waylandsink”。 (自UxPlay 1.67起移除)

**-as _audiosink_**选择GStreamer audiosink,而不是让
   autoaudiosink为您挑选。  一些audiosink选择是：脉冲,alsasink,pipewiresink,
   osssink、oss4sink、jackaudiosink、osxaudiosink(用于macOS)、wasapisink、directsoundsink(用于Windows)。  使用引号
   “...“可能允许audiosink名称中包含某些参数。
   (Some audiosink的选择可能无法在您的系统上工作。   

**-as 0**  (or just **-a**) suppresses playing of streamed audio, but displays streamed video.

**-al _x_**指定向客户端报告的音频延迟时间(以仅音频(ALAC)表示,以十进制秒为单位)。  值
   在[0.0到10.0]秒的范围内,将被转换为微秒的整数。  默认
   是0.25秒(250000 usec)。   _(但是,客户端似乎忽略了此报告的延迟,因此此选项似乎不起作用。)_

**-ca _filename_**提供用于输出“封面”的文件(其中_filename_可以包含完整路径)
   (from Apple Music等) 在纯音频的ALAC模式下。   此文件将被最新的封面覆盖,
   就到了。   如果不使用此选项,则会丢弃封面(jpeg格式)。    Use with a image viewer that reloads the image
   如果它发生变化,或定期(例如, 每秒一次)。    To achieve this,  run "`uxplay -ca [path/to/]filename &`" in the background,
   然后在前台运行图像查看器。   例如,使用`feh`作为查看器：运行“feh -R1[path/to/] filename“(在
   在其中将UxPlay置于背景中的同一终端窗口)。   要退出,请使用`ctrl-C fg ctrl-C`终止
   图像查看器,将uxplay带入前台,并终止它。

**-reset n**设置客户端响应ntp请求的连续超时失败的限制为_n_
   从服务器(这些是每3秒发送一次,以检查客户端是否仍然存在,并与它同步)。   后
   _n_ failures,则客户端将被假定为离线,并且连接将被重置以允许新的
   连接.   _n_的默认值为5;值_n_ = 0表示超时“无限制”。

**-nc**保持以前的UxPlay < 1.45行为,当客户端****
   发出“停止镜像”信号 _此选项当前在macOS中默认使用,
   由于GStreamer在macOS中创建的窗口无法正确终止(它会导致segfault)
   如果GStreamer管道关闭时它仍然打开。_

**-nohold**在新客户端尝试连接时断开当前连接。  如果没有这个选项,
   当前客户端保持UxPlay的独占所有权直到其断开连接。

**-**Restrict限制允许连接到`-allow<deviceID>`指定的客户端。  deviceID具有
    form of a MAC address which is displayed by UxPlay when the client attempts to connect, and appears to be immutable.   它
    has the format `XX:XX:XX:XX:XX:XX`, X = 0-9,A-F, and is possibly the "true" hardware
    MAC address of the device.  请注意,iOS客户端通常会暴露不同的随机“私人Wi_Fi地址”(“假”MAC地址),
    different networks (for privacy reasons, to prevent tracking), which may change, and do not correpond to the deviceID.

**-restrict no**删除限制(默认)。  这是一个有用的命令行参数覆盖限制集
   在启动文件中。
   
**-allow _id_**在客户端限制时将deviceID = _id_添加到允许的客户端列表
   被强制执行。  通常这是uxplayrc启动文件中的一个条目。

**-block _id_**始终阻止deviceID = _id_的客户端,即使客户端限制未
   普遍强制执行。 通常这是uxplayrc启动文件中的一个条目。

**-FPSdata**打开监控有关视频流性能的定期报告
   由客户端发送。  这些将显示在终端窗口中,如果
   选项已使用。   客户端以1秒的间隔更新数据。

**-fps n**设置隔空投放的最大帧速率(每秒帧数)
   客户端流视频; n必须是小于256的整数。
   (The客户端可以选择以任何帧速率提供视频
   默认值为30 fps)。  设置为60 fps可能会改善视频质量
   但不建议在Raspberry Pi上使用。   低于30 fps的设置可能有助于
   如果您同时运行多个uxplay实例,请减少延迟。
   _此设置仅供参考,
   客户端设备,因此设置一个高值将不会强制高帧速率。_
   (You可以使用“-vs fpsDisplaysink”来测试帧率
   接收,或使用选项-FPSdata显示视频流性能数据
   在视频流期间由客户端连续发送)。

**-f {H| V| I}**实现“videoflip”图像变换：H =水平翻转
   (左右翻转或镜像); V =垂直翻转; I =
   180度旋转或反转(H与V的组合)。

**-r {R| L}** 90度右(顺时针)或左(逆时针)
   旋转;这些图像变换在任何**-f**变换之后执行。

**-m[mac]**更改UxPlay使用的MAC地址(设备ID)(默认为使用
   主机网卡报告的真实硬件MAC地址)。
   (不同的服务器名称、MAC
   地址和网络端口是每个运行的uxplay所必需的,
   尝试在同一台计算机上运行多个uxplay实例。
   如果没有给出[mac](格式为xx：xx：xx：xx：xx,6个十六进制八位字节),
   生成MAC地址。
   如果UxPlay无法找到网卡的真实MAC地址,(更多
   具体地,由检测到的第一活动网络接口使用的MAC地址)
   即使未指定选项**-m,也**将使用随机MAC地址。
   (Note每次启动UxPlay时随机MAC地址将不同)。

**-key[_filename_]**：默认情况下,服务器私钥存储在文件$HOME/. uxplay. pem中。 使用
   “-key _filename_”选项来更改此位置。   此选项应在UxPlay启动文件中设置
   作为一行“`key filename`“(无首字母“-”),其中``文件名``是完整路径。   文件名可能包含在
   在引号中(“......“`),(如果文件名中有空格,则必须是)。

**-dacp[_filename_]**：将当前客户端DACP-ID和Active-Remote密钥导出到文件：默认值为$HOME/. uxplay. dacp。
   (可选地,可以更改为_filename_)。 可用于远程控制应用程序。 文件是临时的：仅存在
   当客户端连接时。

**-vdmp**将h264视频转储到文件videodump.h264。  -vdmp n转储不超过n个NAL单元,
   videodump.x.h264; x= 1,2,... 每次SPS/PPS NAL单元到达时增加。   要更改名称
   _videodump_,使用-vdmp[n]_filename_。

**- admp将音频转储到文件audiodump. x. aac(AAC-ELD格式音频)、audiodump. x. alac(ALAC格式音频)或audiodump. x. aud**
   (其他格式音频),其中x = 1,2,3. 每次音频格式更改时都会增加。 -admp _n_限制
   数据包转储到一个文件到_n_或更少。    要更改名称_audiodump_,请使用-admp [n]_filename_。   _请注意(与转储视频不同)
   转储的音频目前只对调试有用,因为它没有容器化以使其可与标准音频播放器一起播放。

**-d**  Enable debug output.   注意：这不会显示GStreamer错误或调试消息。   查看GStreamer错误
    and warning messages, set the environment variable GST_DEBUG with "export GST_DEBUG=2" before running uxplay.
    To see GStreamer information messages, set GST_DEBUG=4; for DEBUG messages, GST_DEBUG=5; increase this to see even
    more of the GStreamer inner workings.

# 故障排除

注意：`uxplay`是从终端命令行运行的,信息性消息会写入终端。

### 0.  编译UxPlay的问题。
    
一个用户(在Ubuntu上)发现编译失败,并显示有关链接到“usr/local/lib/libcrypto.a”和“zlib”的消息。
这是因为(除了标准的libssl-dev的ubuntu安装),用户不知道第二次安装
/usr/local中的libcrypto。
解决方案：当存在多个OpenSSL安装时,将环境变量OPEN_SSL_ROOT_OTT设置为指向正确的安装;
在64位Ubuntu上,这是通过
在运行cmake之前运行`export OPENSSL_ROOT_DIR=/usr/lib/X86_64-linux-gnu/`。

### 1. **Avahi/DNS_SD Bonjour/Zeroconf问题**

DNS_SD Service-Discovery(“Bonjour”或“Zeroconf”)服务是UxPlay工作所必需的。  在Linux上,它通常由Avahi提供,要解决这个问题,您可以
应该使用`avahi-browse`工具。  (You可能需要安装一个单独的软件包,名称为avahi-utils。)

在Linux上,确保安装了Avahi,
并在运行uxplay的系统上启动avahi-daemon服务(您的发行版将记录如何执行此操作,例如：
`sudo systemctl<cmd> avahi-daemon`或``sudo service avahi-daemon<cmd>``,
``<cmd>``启用、禁用、启动、停止、状态之一。
您可能需要编辑avahi-daemon.conf文件(通常位于/etc/avahi/中,使用“`sudo find /etc -name avahi-daemon.conf`“找到它)：
确保“disable-publishing”**不是**一个选中的选项)。
某些系统可能会使用mdnsd守护程序作为替代方案来提供DNS-SD服务。
(FreeBSD提供了两种选择,但只有Avahi进行了测试;请参阅[这里](https://gist.github.com/reidransom/6033227)。

* **uxplay启动,但在“Initialized server socket(s)”出现(_在客户端上没有显示服务器名称_)后停止或停止**。

如果UxPlay停止并显示“No DNS-SD Server found”消息,这意味着您的网络**没有正在运行的Bonjour/zeroconf DNS-SD服务器。**
在v1.60之前,如果DNS-SD服务注册失败,UxPlay通常会默默地停止,但现在会停止,并返回一条错误消息。
DNSServiceRegister函数：kDNSServiceErr_Unknown如果没有找到DNS-SD服务器：
其他mDNS错误代码在FFFE FF 00(-65792)到FFFE FFFF(-65537)的范围内,并在
dnssd. h文件。  一个较旧的版本(avahi使用的版本)可以[在这里](https://github.com/lathiat/avahi/blob/master/avahi-compat-libdns_sd/dns_sd.h)找到。
在以后的版本中定义了一些额外的错误代码
来自[Apple](https://opensource.apple.com/source/mDNSResponder/mDNSResponder-544/mDNSShared/dns_sd.h.auto.html)。


If UxPlay stalls _without an error message_ and _without the server name showing on the client_, **this is a network problem**  (if your UxPlay version
比1.60旧,这也是没有找到DNS-SD服务器时的行为。

从客户端检查此类网络问题的一个有用工具是(免费)Discovery DNS-SD
浏览器[可在苹果应用程序商店](https://apps.apple.com/us/developer/lily-ballard/id305441020)的iOS(工程iPadOS太)和macOS。

* 一些使用双频(2.4GHz/5GHz)路由器的用户报告说,使用5GHz频段的客户端(有时)“看不到
UxPlay”(即, 没有得到他们的mDNS查询响应),但2.4GHz频段工作。 其他使用Bonjour/mDNS的项目也有类似的报告;
这个问题似乎是路由器特有的,可能与“自动”而不是固定信道选择有关(5GHz有更多
通道之间切换)或通道宽度选择;一种推测是,由于mDNS使用UDP协议(其中
如果在查询期间发生信道切换,则mDNS查询可能会丢失。

如果你的路由器有这个问题,一个报告的“修复”是(至少在5GHz上)使用固定信道和/或固定(非动态)信道宽度。

*  **Avahi最初可以工作,但新客户端看不到UxPlay,或者最初看到它的客户端在断开连接后停止这样做**。

这通常是因为Avahi只使用“DNS”网络接口,并且不接收来自未使用的新客户端的mDNS查询。
当UxPlay启动时,

要检查这一点,启动uxplay后,**在**服务器上的另一个终端窗口中使用avahi-browse -a -t实用程序
验证UxPlay Aircraft和隔空投放服务是否已正确注册(只有Aircraft服务
在UxPlay使用的“传统”隔空投放镜像模式中使用,但隔空投放服务用于初始联系)。

avahi-browse返回的结果应显示以下条目：
Uxplay like


```
+   eno1 IPv6 UxPlay                                        隔空投放 Remote Video local
+   eno1 IPv4 UxPlay                                        隔空投放 Remote Video local
+     lo IPv4 UxPlay                                        隔空投放 Remote Video local
+   eno1 IPv6 863EA27598FE@UxPlay                           AirTunes Remote Audio local
+   eno1 IPv4 863EA27598FE@UxPlay                           AirTunes Remote Audio local
+     lo IPv4 863EA27598FE@UxPlay                           AirTunes Remote Audio local

```
如果只显示了防火墙(“lo”)条目,则UxPlay主机上的防火墙
可能会阻止完整的DNS-SD服务,您需要为mDNS请求打开默认的UDP端口5353,
因为基于环回的DNS-SD服务是不可靠的。

如果UxPlay服务如上所述由avahi-browse列出,但未被客户端看到,
该问题很可能是本地网络的问题。


### 2. uxplay启动,但在“Initialized server socket(s)”出现后停止,*服务器名称显示在客户端上*(但当选择UxPlay服务器时,客户端无法连接)。

这表明*DNS-SD*服务正在工作,客户端听到UxPlay可用,但UxPlay服务器没有收到来自客户端的响应。
这通常是因为
服务器上的防火墙正在阻止来自客户端的连接请求。
(One坚持防火墙已关闭的用户原来有两个活动防火墙(*firewalld*和*ufw*)
_both_ running on the server！)  如果可能,关闭防火墙
看看是不是问题所在,或者获取三个连续的网络端口,
从端口n开始,所有三个端口都在1024-65535范围内,为tcp和udp打开,并使用“uxplay -pn”
(or打开UDP 7011,6001,6000 TCP 7100,7000,7001并使用"uxplay-p")。

如果你真的确定没有防火墙,你可能需要用netstat这样的工具来调查你的网络传输,但几乎总是这样。
是防火墙的问题

### 3. 建立客户端-服务器连接后出现的问题：

如果您没有看到消息raop_mirror_starting mirroring,那么在客户端-服务器协商完成之前就出现了问题。
对于此类问题,使用“uxplay -d“(调试日志选项)查看发生了什么：它将显示连接过程在
故障发生。    You can compare your debug  output to
从[UxPlay Wiki](https://github.com/FDH2/UxPlay/wiki)中的UxPlay成功开始。

**如果UxPlay报告镜像已启动,但没有视频或音频,则问题可能来自
GStreamer插件在您的系统上不起作用(默认情况下,
GStreamer使用“autovideosink”和“autoaudiosink”算法
猜猜你的系统上使用的“最佳”插件是什么)。
一个不同的原因没有音频发生时,一个用户与防火墙只打开两个udp网络
端口：需要**三个**(第三个接收音频数据)。

**Raspberry Pi**设备(_Pi 4B+和更早版本：这不适用于Pi 5,它不提供硬件h264解码,也不提供
如果GStreamer v1.20.x或更早版本中的Video4Linux2插件具有以下功能,
已被修补(请参阅UxPlay[Wiki](https://github.com/FDH2/UxPlay/wiki/Gstreamer-Video4Linux2-plugin-patches)的补丁)。
这在GStreamer-1.22中得到了修复,并通过在Raspberry Pi OS(Bullseye)等发行版中的后端补丁得到了修复：使用选项`-bt709`
使用Raspberry Pi OS的GStreamer-1.18.4
这还需要标准Linux内核中没有的bcm 2835-codec内核模块(它在Raspberry Pi OS,Ubuntu和Manjaro中可用)。

* **如果此内核模块在您的Raspberry Pi操作系统中不可用,或者如果GStreamer < 1.22未打补丁,请使用选项`-avdec`
用于软件H264解码。**

有时“autovideosink”可能会选择OpenGL渲染器“glimagesink”,
可能无法在您的系统上正常工作。   尝试选项“-vs ximagesink”或
“-vs xvimagesink”查看是否使用其中一个解决了问题。

其他报告的问题与GStreamer VAAPI插件有关
(for硬件加速的英特尔显卡,但不是NVIDIA显卡)。
使用选项“-avdec”
强制软件H264视频解码：这将阻止autovideosink选择vaapisink videosink。
或者,找出
gstreamer1.0-vaapi插件已安装,如果已安装,请卸载它。
(If这不能解决问题,您可以重新安装它。

There are some reports of other GStreamer problems with hardware-accelerated Intel HD graphics.  一个用户
(on Debian)通过“sudo apt install intel-media-va-driver-non-free”解决了这个问题。  这是第8代(或更高版本)驱动程序
“*-lake”英特尔芯片,那似乎与VAAPI加速显卡有关。

如果您有英特尔高清显卡,并且安装了vaapi插件,但-vs vaapisink不起作用,请检查vaapi是否未在GStreamer安装中列入“黑名单”：运行gst-inspect-1.0 vaapi,如果报告0个功能,则需要在运行uxplay之前导出GST_VAAPI_ALL_DRIVERS=1,或在默认环境中设置此参数。

You can try to fix audio or video problems by using the "`-as <audiosink>`" or "``-vs <videosink>``"  options to choose the GStreamer audiosink or videosink , rather than
让GStreamer为您选择一个。 (See在[启动和运行UxPlay中](#starting-and-running-uxplay)选择`<audiosink>`或``<videosink>``。 

The "OpenGL renderer" window created on Linux by "-vs glimagesink" sometimes does not close properly when its "close" button is clicked.
(this是GStreamer的问题)。  您可能需要使用Ctrl-C终止uxplay以关闭“僵尸”OpenGl窗口。   如果类似的问题发生在
客户端发送“停止镜像”信号,尝试不关闭选项“-nc”,使视频窗口打开。

### 4. GStreamer问题(缺少插件等)：

* clearing the user's GStreamer cache with  `rm -rf ~/.cache/gstreamer-1.0/*`  may be the solution to problems
 其中gst-inspect-1.0不显示您认为已安装的插件。   下次将重新生成缓存
GStreamer已启动。  **这是解决由缓存损坏引起的令人困惑的问题的方法,应该首先尝试。**

如果UxPlay无法启动,并显示一条消息,指出未找到所需的GStreamer插件(例如“libav”),请首先使用GStreamer工具进行检查
gst-inspect-1.0查看GStreamer知道哪些可用。  (You可能需要安装一些额外的GStreamer“工具”包来获取gst-inspect-1.0)。
例如,  一个libav问题,检查“`gst-inspect-1.0 libav`“。  如果它不显示为GStreamer可用,但您的包管理器
显示安装的相关软件包(如一个用户发现的),请尝试完全删除并重新安装该软件包。
该用户发现,对于不断重复出现的“**Required gstreamer plugin 'libav' not found**“消息的解决方案是清除该用户的gstreamer
缓存
 
如果它无法启动,并出现类似“`no element "avdec_aac"`“的错误,
因为即使安装了gstreamer-libav。 它是不完整的,因为一些插件功能丢失：“`gst-inspect-1.0|  grep avdec_aac`“将
显示avdec_aac是否可用。   与其他GStreamer插件不同,libav插件是FFmpeg编解码器的前端,提供avdec_*。

* 一些发行版(RedHat,SUSE等)提供了不完整的FFmpeg版本,因为使用的编解码器存在专利问题。
某些插件。 在这些情况下,将有一些“额外的包”提供商,如[RPM融合](https://rpmfusion.org)(RedHat),
[packman](http://packman.links2linux.org/)(SUSE),您可以在其中获取完整的软件包(您的
发行版通常会提供这方面的说明,Mageia将它们放在一个可选的“污染”存储库中)。 包装
需要的可能是“ffmpeg\*”或“libav\*”包：GStreamer libav插件包本身不包含任何编解码器,它只是提供了一种方法,
GStreamer使用ffmpeg/libav编解码器库,必须单独安装。  出于类似的原因,发行版可能会发送不完整的软件包
GStreamer“plugins-bad”. 使用用户在Fedora上认为他们已经从rpmfusion安装,但系统没有服从：_“添加--allowerasing到
dnf命令在重新启动”_“之后修复了它。

* 从UxPlay-1.65.3版本开始,如果avdec_aac丢失,UxPlay将继续运行,但在镜像模式下没有音频。

要排除GStreamer故障,请执行“export GST_DEBUG=2”
在终端中设置GStreamer调试级环境变量
在这里你将运行uxplay,这样你就可以看到警告和错误消息;
参见[GStreamer调试工具](https://gstreamer.freedesktop.org/documentation/tutorials/basic/debugging-tools.html)
如何看到更多的内部发生的事情
GStreamer。   运行“gst-inspect-1.0”查看哪些GStreamer插件
安装在您的系统上。

可能需要安装一些额外的GStreamer包用于特殊插件(或重新安装：用户使用Wayland显示系统作为X11的替代品
报告说,重新安装Lubuntu 18.4后,UxPlay将无法工作,直到安装gstreamer1.0-x,大概是Wayland的X11兼容模式)。
不同的发行版可能会以不同的方式将GStreamer 1.x分解为多个包;上面在构建说明中列出的包应该包含
其他必需的GStreamer包作为依赖项,但不会安装所有可能的插件。

GStreamer视频管道,`uxplay -d`的初始输出所示,
具有默认形式

```
appsrc name=video_source ! queue ! h264parse ! decodebin ! videoconvert ! autovideosink name=video_sink sync=false
```

流水线是完全可配置的：默认元素“h264parse”、“decodebin”、“videoconvert”和“autovideosink”可以分别通过使用uxplay替换。
选项`-vp`、-vd、`-vc`和-vs,如果需要
修改它(条目可以在引号中给出“...“包括备选方案)。

### 5.  镜像屏幕冻结(网络问题)：

如果来自客户端的TCP视频流停止到达服务器,可能是因为网络问题(UDP音频流可能继续到达),就会发生这种情况。   在3秒时
间隔,UxPlay通过向客户端发送NTP时间信号的请求来检查客户端是否仍然连接。   如果在0.3秒内未收到来自客户端的回复,
时间窗口,注册“NTP超时”。    If a certain number (currently 5) of consecutive ntp timeouts occur, UxPlay assumes that the client is "dead", and resets the connection,
变得可用于连接到新的客户端或重新连接到先前的客户端。   有时,连接可能会在达到超时限制之前恢复,如果
默认限制不适合您的网络,可以使用选项“-reset _n_"进行修改,其中_n_是所需的超时限制值(_n_ = 0表示“无限制”)。    If the connection
在ntp超时后开始恢复,超时前损坏的视频数据包可能会触发“connection reset by peer”错误,这也会导致UxPlay重置
连接.

* When the connection is reset, the "frozen" mirror screen of the previous connection is left in place, but does **not** block 
新的连接,并将接管一个新的客户端连接时,它是。

### 6. 协议问题(对客户端发送的加密音频和视频流进行解密)。

协议失败可能会触发无休止的错误消息流,这意味着
音频解密密钥(也用于视频解密)
未从客户端发送的数据中正确提取。

在UxPlay-1.65中,该协议被修改,因为发现可以避免客户端-服务器“配对”步骤(导致更快的
连接设置,没有5秒延迟),通过禁用“功能”代码UxPlay中的“支持传统配对”(位27)
DNS-SD服务发现  大多数客户端在配对时将不会尝试设置“共享密钥”,这是AppleTV用于同时
处理多个客户端(UxPlay一次只支持一个客户端)。
**此更改现在经过了充分的测试,但如果它导致任何协议失败,则可以通过取消注释先前的“FEATURES_1”设置将UxPlay恢复到先前的行为
(and注释掉新的),然后重建UxPlay。** (当通过使用UxPlay 1.67中引入的“-pin”选项运行UxPlay来激活新的Apple式一次性“pin”身份验证时,“配对”将重新启用。


iOS 9.3或更高版本的客户端不应发生协议故障。  如果客户端
使用与基于Windows的
隔空投放客户端模拟器_AirMyPC_,协议可以切换到旧版本
通过设置`OLD_PROTOCOL_CLIENT_USER_AGENT_LIST`
在`UxPlay/lib/global.h`中。
UxPlay在连接时报告客户端的“User Agent”字符串。 如果
某些其他客户端也无法解密所有音频和视频,请尝试添加
其“User Agent”字符串代替条目“AirMyPC/2.0;xxx”中的“xxx”
在global.h和重建uxplay.

请注意,对于DNS-SD服务发现,Uxplay声明自己是AppleTV 3,2(32位设备),
sourceVersion 220.68;这也可以在global. h中更改。
UxPlay也可以工作,如果它宣布自己是一个AppleTV 6,2与
sourceVersion 380.20.1(2017年推出的AppleTV 4K第1代,运行
tvOS 12.2.1),所以UxPlay声称是什么版本似乎并不重要。


# Changelog
1.67 2023-11-30   添加对带有选项“-pin”的客户端的Apple式一次性pin身份验证的支持：
                  (uses SRP6a authentication protocol and public key persistence).   错误信息检测
		  通过有线以太网请求高分辨率时,(当前)不支持的H265视频。
		  删除了rpi* 选项(对于新的Raspberry Pi型号5无效,可以替换
		  通过其他选项的组合)。   添加可选参数“mac”到“-m”选项,以
		  指定替换MAC地址/设备ID。将llhttp更新为v.9.1.3。  添加-dacp选项
		  用于导出当前客户端DACP信息(用于远程)。

1.66 2023-09-05 2023-09-05 2023-09-05   修复IPV6支持。  添加选项以将客户端限制为允许的设备ID列表中的客户端,
                  or to block connections from clients on a list of blocked deviceIDs.   修复#207从
		  @thiccaxe(客户端从睡眠中唤醒后,vsync模式下的屏幕延迟)。
		  
1.65.3 2023-07-23添加RPM规范文件;如果需要gstreamer libav功能“avdec_aac”,
                  missing: (this occurs in RPM-based distributions that ship an incomplete FFmpeg for Patent
                  or License reasons, and rely on users installing an externally-supplied complete FFmpeg).
		  如果avdec_aac丢失,则音频模式隔空投放现在可以在没有音频的情况下工作。

1.65 2023-06-03 2023-06-03 2023-06-03   删除连接协议中的pair_setup部分,以实现与客户端的更快连接
                  (thanks to @shuax #176 for this discovery); to revert, uncomment a line in lib/dnssdint.h.
                  Disconnect from audio device when connection closes, to not block its use by other apps if 
                  uxplay is running but not connected.  修复AirMyPC客户端(自1.60以来已损坏),因此其
                  older non-NTP timestamp protocol works with -vsync.   更正配置解析
		  文件条目都在引号里。

1.64 2023-04-23 - 24 -23 - 24 - 25 - 24 - 25-   基于时间戳的音频和视频同步现在是镜像模式中的默认设置。
                  (Use "-vsync no" to restore previous behavior.)    A configuration file can now be used
                  for startup options.   还有一些内部清理和一个小的bug修复,修复#192。

1.63 2023-02-12 2023-02-12 2023-02-12   重新设计的音频-视频同步,具有新选项-vsync(用于镜像模式)和
                  -async (for Audio-Only mode, to sync with client video).   选项-vsync使软件
                  h264 decoding of streamed videos with option -avdec viable on some recent Raspberry Pi models.
                  Internal change: all times are now processed in nanoseconds units.   删除-ao选项
                  introduced in 1.62.

1.62 2023-01-18 2023-01-18 2023-01-18   添加了仅音频模式时间偏移-ao x,以允许用户同步ALAC
                  audio playing on the server with video, song lyrics, etc. playing on the client.
                  x = 5.0 appears to be optimal in many cases.  质量修复：批量清理
                  changes, timestamps, some bugfixes.

1.61 2022-12-30   删除-t选项(Avahi问题的解决方法,通过打开网络正确解决
                  port UDP 5353 in firewall).  从CMAKE_CFLAGS中删除-g调试标志。 推迟(而不是
                  of prepend) build environment CFLAGS to CMAKE_CFLAGS.  重构部分uxplay.cpp

1.60 2022-12-15 - 01 - 02   添加了在DNSServiceRegister失败时退出并显示错误消息(而不仅仅是停止)。
                  Test for Client's attempt to using unsupported 隔空投放 2 "REMOTE CONTROL" protocol
                  (with no timing channel), and exit if this occurs.   返工的元数据处理
                  to correctly parse DMAP header (previous version worked with DMAP messages currently
                  received, but was not correct).

1.59 2022-12-12 2022-12-12 2022 -12-12   删除“ZOOMFIX”编译选项,并使编译与X11依赖的
                  default if X11 development libraries are detected (this now also provides 
                  fullscreen mode with a F11 or Alt+Enter key toggle); ZOOMFIX is now automatically 
                  applied for GStreamer < 1.20. 新的cmake选项-DNO_X11_DEPS编译uxplay而不
                  X11 dependence. 重新设计了内部元数据处理。 使用“-vs 0”修复segfault。

1.58 2022-10-29 - 2022-10-29   添加选项“-nohold”,当新客户端连接时,该选项将删除现有连接。
                  Update llhttp to v8.1.0.

1.57 2022-10-09 - 2022-10-09   次要修复：(修复AUR上的“停止镜像”核心转储,当编译时发生
                  AUR CFLAGS -DFORTIFY_SOURCE); graceful exit when required plugins are missing;
                  improved support for builds on Windows.  在GStreamer中包含音频样本
                  audio pipeline.

1.56 2022-09-01 2022-09-01   添加了对在Windows上构建和运行UxPlay-1.56的支持(无更改
		  到Unix(Linux,*BSD,macOS)代码库。
					
1.56 2022-07-30 - 2022 - 2023 - 2022 - 2023   从-rpi、-rpiwl、-rpifb中删除-bt 709,因为GStreamer现在已修复。

1.55 2022-07-04 2022-07-04   从-v4 l2中删除bt 709修复并创建新的-bt 709选项(以前的
                  "-v4l2" is now "-v4l2 -bt709").  这使得当前所需的-bt 709
                  option to be used on its own on RPi without -v4l2 (sometimes this give better results).

1.54添加对仅音频(ALAC)模式下的“封面艺术”显示的支持。   Add support for "Cover Art" display in Audio-only (ALAC) mode. 还原了一个变化
                  that caused  VAAPI to crash with AMD POLARIS graphics cards.   内部小改动,
                  plist code and uxplay option parsing.

1.53 2022-06-13 2022-06-13 2022-06-13   音频同步代码的内部更改,修订文档,
                  Minor bugfix (fix assertion crash when resent audio packets are empty).

1.52 2022-05-05 2022-05-05 2022-05-05   清理了初始音频同步代码,并重新格式化
                  streaming debug output (readable aligned timestamps with
                  decimal points in seconds).   消除内存泄漏
		  (由Valgrind发现)。   支持显示ALAC
		  (仅音频)元数据(配乐艺术家姓名、标题等)
		  在UXPlay终端中。

1.51 2022-04-24 2022-04-24   修改了Video 4Linux 2支持的选项(新选项-v4 l2)和短选项-rpi、-rpifb、-rpiwl作为
                  synonyms for -v4l2,  -v4l2 -vs kmssink, and -v4l2 -vs waylandsink.  恢复了从1.48突破的变化
		  客户端发送“停止镜像”后重新连接。

1.50 2022-04-22 2022-04-22   添加了-fs全屏选项(仅适用于Wayland或VAAPI插件),将-rpi更改为用于framebuffer(“lite”)RPi
                  systems and added -rpigl (OpenGL) and -rpiwl (Wayland) options for RPi Desktop systems.
                  Also modified  timestamps from "DTS" to "PTS" for latency improvement, plus internal cleanups.
		  
1.49 2022-03-28 2022-03-28 2022-03-28   增加了将视频和/或音频转储到文件、调试等的选项。h264 PPS/SPS NALU显示为-d。
                  Fixed video-not-working for M1 Mac clients.

1.48 2022-03-11 2022-03-11   使GStreamer视频管道完全可配置,用于硬件h264解码。  支持Raspberry Pi。

1.47 2022-02-05 2022 -02-05   添加了-FPSdata选项,用于显示(在终端中)客户端发送的有关视频流的定期报告
                  performance.  从客户端接收的视频数据包处理的内部清理。   添加-reset n选项
                  to reset the connection after n ntp timeouts (also reset after "connection reset by peer" error in video stream).

1.46 2022-01-20 2022-01-20   恢复1.44之前的行为(1.44可能已经破坏了硬件加速)：再次在视频管道中使用decodebin;
                  introduce new option "-avdec" to force software h264 decoding by libav h264, if needed (to prevent selection of 
                  vaapisink by autovideosink).  将llhttp更新为v6.0.6。  UxPlay现在报告自己为AppleTV3,2。  将连接限制
                  to one client at a time (second client must now wait for first client to disconnect).

1.45新行为：当客户端请求“停止镜像”时关闭视频窗口。   New behavior: close video window when client requests "stop mirroring".  (新增“不关闭”选项“-nc”
                  for users who wish to retain previous behavior that does not close the video window).

1.44 2021-12-13 - 01 - 02 - 01 - 01   对于AirMyPC客户端,使用ecdh_secret忽略aeskey的哈希;在内部重新排列此哈希的位置
                  done. 在-d调试模式下完整报告客户端和服务器之间的所有初始通信。 在GStreamer中替换decodebin
                  video pipeline by h264-specific elements.

1.43 2021-12-07 2021-12-07   各种内部变化,如测试成功解密,统一处理
                  of informational/debug messages, etc., 更新README。

1.42 2021-11-20 2021-11-20 2021-11-20   修复MAC检测,以便与现代Linux接口命名实践、MacOS和 *BSD配合使用。

1.41 2021-11-11 2021-11-11    进一步清理多种音频格式支持(内部更改,
                   separated RAOP and GStreamer audio/video startup)

1.40 2021-11-09 -09 - 01 - 01 - 01    在ALAC支持中排除segfault,manpage位置修复,在调试模式下显示请求Plists。

1.39 2021-11-06 2021-11-06 2021-11-06    增加了对Apple Lossless(ALAC)音频流的支持。

1.38 2021-10- 11 - 01     添加-as _audiosink_ option以允许用户选择GStreamer audiosink。

1.37 2021-09-29 2021-09-29    将“@hostname”附加到隔空投放服务器名称,其中“hostname”是
                   server running uxplay (reworked change in 1.36).

1.36 2021-09-29 2021-09-29    实施建议(由@mrbesen和@PetrusZ)以使用机器的主机名
                   runing uxplay as the default server name 

1.35.1 2021-09-28添加了-vs 0选项,用于流式传输音频,但不显示视频。

1.35 2021-09-10 2021-09-10   现在使用GLib MainLoop,并基于macOS构建(在Intel Mac 10.15上测试)。
                   New option  -t _timeout_ for relaunching server if no connections were active in
                   previous _timeout_ seconds (to renew Bonjour registration).
                   
1.341 2021-09-04   修复：渲染记录器未被stop_server()销毁

1.34 2021-08-27 2021-08-27 2021-08-27   修复“ZOOMFIX”：X11窗口名称修复只在
                   first time the GStreamer window was created by uxplay, and
		   如果在GStreamer窗口之后重新启动服务器,则不会
		   已经关闭,但uxplay仍在运行。   修正于1.34节

### 从源代码构建OpenSSL >= 1.1.1。

如果您需要这样做,请注意,您可以使用较新的版本(OpenSSL-3.0.1已知可以工作)。
你需要标准的开发工具集(autoconf,automake,libtool)。
下载源代码,
[https：//www.openssl.org/source/](https://www.openssl.org/source/).
安装下载的
openssl,方法是在Downloads目录中打开一个终端,然后解压缩源代码发行版：
(“tar -xvzf openssl-3.0.1.tar.gz; cd openssl-3.0.1”)。 然后构建/安装
“./ config ; make ; sudo make install_dev”。  这通常会安装所需的库`libcrypto.*`,
在/usr/local/lib或/usr/local/lib中。

_(对于MacOS上的构建版本,忽略以下内容：)_
在某些系统上,如
Debian或Ubuntu,您可能还需要添加缺少的条目`/usr/local/lib64```
在/etc/ld.so.conf中(或将包含“/usr/local/lib 64/libcrypto.so“的文件放在/etc/ld.so.conf.d中)
然后运行“sudo ldconfig”。 

### Building libplist >= 2.0.0 from source.

_(注意：在Debian 9“Stretch”或Ubuntu 16.04 LTS版本上,您可以通过安装libplist-dev来避免此步骤
和libplist3从Debian 10或Ubuntu 18.04.)_
除了常用的构建工具(autoconf、automake、libtool),
可能还需要安装一些libpython\*-dev包。  下载最新源码
使用git从[https：//github.com/libimobiledevice/libplist](https://github.com/libimobiledevice/libplist),或者
从Releases部分获取源代码(使用\*.tar.bz2版本,而**不是**\*.zip或\*.tar.gz版本)：
下载[libplist-2.3.0](https://github.com/libimobiledevice/libplist/releases/download/2.3.0/libplist-2.3.0.tar.bz2),
然后解包(“tar -xvjf libplist-2.3.0.tar.bz2 ; cd libplist-2.3.0”),并构建/安装它：
(“./ configure ; make ; sudo make install”)。   这可能会安装libplist-2.0。\* 在/usr/local/lib中。
新的libplist-2.3.0版本应该与
UxPlay;[libplist-2.2.0](https://github.com/libimobiledevice/libplist/releases/download/2.2.0/libplist-2.2.0.tar.bz2)
如果有任何问题,也可以使用。

_(Ignore the following for builds on MacOS:)_  On some systems like
Debian或Ubuntu,您可能还需要添加缺少的条目`/usr/local/lib```
在/etc/ld.so.conf中(或将包含“/usr/local/lib/libplist-2.0.so“的文件放在/etc/ld.so.conf.d中)
然后运行“sudo ldconfig”。 




# Disclaimer

这个资源库中的所有资源都是使用互联网上免费提供的信息编写的。 代码和相关资源用于
仅用于教育目的。 用户有责任确保遵守所有当地法律。

该项目使用第三方GPL库来处理F隔空投放。 该图书馆的法律地位尚不清楚。 如果你是一个代表,
苹果公司对该库的合法性及其在本项目中的使用有任何异议,请联系开发者并采取相应的步骤
会被带走

考虑到大量的第三方隔空投放接收器(大多数是闭源的)可供购买,我们的理解是,
实现相同的功能也不会侵犯苹果的任何权利。



# UxPlay作者

_[改编自fdraschbacher关于RPiPlay前身的笔记]_

这个存储库中的代码是随着时间的推移从各种来源积累起来的。 这里
尝试列出各种作者和他们创建的组件：

UxPlay最初是由RPiPlay**的antiof**创建的,通过替换其Raspberry-Pi改编的OpenMAX视频
和带有GStreamer渲染的音频渲染系统,
桌面Linux系统;在`renderers/`中的代码上的antimof工作后来被反向移植到RPiPlay,antimof项目进入休眠状态,但后来
在当前的[GitHub网站](http://github.com/FDH2/UxPlay)上复活,以服务于更广泛的用户社区。

UxPlay中通过继承RPiPlay而包含的代码的先前作者包括：

* **EstebanKubata**：创建了一个名为[PlayFair的](https://github.com/EstebanKubata/playfair)F隔空投放库。 位于`lib/playfair`文件夹中。 许可证：GNU GPL
* **Juho Vähä-Herttua**和贡献者：创建了一个名为[Sh隔空投放的](https://github.com/juhovh/sh隔空投放)隔空投放音频服务器,包括对基于PlayFair的F隔空投放的支持。 大部分代码   在`lib/`中,最初源于这个项目。 许可证：GNU LGPLv2.1+
* **dsafa22**：创建了一个隔空投放 2镜像服务器[隔空投放Server](https://github.com/dsafa22/隔空投放Server)(现在似乎已经消失了),基于Sh隔空投放的Android。   代码
  保存[在这里](https://github.com/jiangban/隔空投放Server),并[看到这里](https://github.com/FDH2/UxPlay/wiki/隔空投放2)的描述
  隔空投放Server作者对隔空投放 2镜像协议的分析,该协议使RPiPlay成为可能。 所有
  `lib/`中关于镜像的代码是dsafa 22的工作。 许可证：GNU LGPLv2.1+
* **Florian Draschbacher**(FD-)和贡献者：将dsafa 22的Android项目改编为Raspberry Pi,进行了广泛的清理,调试和改进。  的
   Project[RPiPlay](https://github.com/FD-/RPiPlay)基本上是dsafa22代码到Raspberry Pi的一个端口,利用OpenMAX和OpenSSL在Pi上实现更好的性能。 GPL v3许可证。
   FD-写了一个关于[隔空投放协议版本](http://github.com/FD-/RPiPlay#隔空投放-protocol-versions)历史的有趣说明,
   可以在RPiPlay github仓库中找到。


独立于UxPlay,但由其使用并与其捆绑：

* **Fedor Indutny**(Node.js,前身为Joyent,Inc)和贡献者：创建了一个名为[llhttp的](https://github.com/nodejs/llhttp)http解析库。 位于`lib/llhttp/`。 许可证：MIT


