## Building UxPlay on Windows with MinGW-w64:

### Download and install dependencies:
Download MSYS2 from official site: https://www.msys2.org/

Download **Bonjour SDK for Windows v3.0** from official Apple site: https://developer.apple.com/download/all/?q=Bonjour%20SDK%20for%20Windows 

### Install dependencies in MSYS2:
Open **MSYS2 MSYS** and install required packages:
```
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-libplist \
    mingw-w64-x86_64-gstreamer \
    mingw-w64-x86_64-gst-libav \
    mingw-w64-x86_64-gst-plugins-bad \
    mingw-w64-x86_64-gst-plugins-bad-libs \
    mingw-w64-x86_64-gst-rtsp-server \
    mingw-w64-x86_64-gst-plugins-base \
    mingw-w64-x86_64-gst-plugins-good \
    mingw-w64-x86_64-gst-plugins-ugly \
    git
```
#### Build and install kdewin:
```
export PATH="/mingw64/bin/:$PATH"
git clone https://github.com/KDE/kdewin.git --depth=1
cd kdewin; mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/mingw64/ -DBUILD_QT_LIB=OFF -DBUILD_BASE_LIB_WITH_QT=OFF -DSTATIC_LIBRARY=ON ..
ninja
ninja install
```
Add permanently mingw64 in MSYS2 PATH:
```
echo 'export PATH="/mingw64/bin/:$PATH"' >> ~/.bashrc
```

Now you can add **MinGW-w64** compiler installed from MSYS2 to your IDE. Compiler with all required dependencies located in msys64 directory, by default this path `C:/msys64/mingw64`. Also you can build UxPlay using MSYS2 environment.

### Known issues:
* No sound