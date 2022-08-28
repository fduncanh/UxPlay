cmake_minimum_required(VERSION 3.4.1)
include_directories( playfair llhttp )

# Common Linux cflags
if ( (UNIX OR MINGW) AND NOT APPLE )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g" )
endif()


# Common x86/x86_64 cflags
if( NOT NO_MARCH_NATIVE AND CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)" )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -march=native" )
  message( STATUS "Using CFLAGS with  -march=native" )
  message( STATUS "*** ONLY USE THIS WHEN COMPILING ON THE MACHINE THAT WILL RUN UXPLAY" )
  message( STATUS "  run \"cmake -DNO_MARCH_NATIVE=ON\" to switch off this compiler option" )
else()
  message( STATUS "Not using -march=native" )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2" )
endif()

INCLUDE (CheckIncludeFiles)
# for BSD Unix (e.g. FreeBSD)
CHECK_INCLUDE_FILES ("sys/endian.h" BSD )
if ( BSD )
  add_definitions( -DSYS_ENDIAN_H )
endif ( BSD )

if( APPLE )
  set( ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig" ) # standard location, and Brew
  set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/local/lib/pkgconfig/" ) # MacPorts
  set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/opt/openssl@3/lib/pkgconfig" ) # Brew openssl
  message( "PKG_CONFIG_PATH (Apple, lib) = " $ENV{PKG_CONFIG_PATH} )
  find_program( PKG_CONFIG_EXECUTABLE  pkg-config  PATHS /Library/FrameWorks/GStreamer.framework/Commands )
  message( "PKG_CONFIG_EXECUTABLE " ${PKG_CONFIG_EXECUTABLE} )
endif()
find_package(PkgConfig REQUIRED)

aux_source_directory(. play_src)
set(DIR_SRCS ${play_src})

add_library( airplay  STATIC
             ${DIR_SRCS}
           )

if ( APPLE )
  target_link_libraries( airplay 
                         pthread
                         playfair
                         llhttp )
elseif( MINGW )
  target_link_libraries( airplay PUBLIC 
                         pthread
                         playfair
                         llhttp
                         wsock32
                         ws2_32 )
else()
  target_link_libraries( airplay PUBLIC
          pthread
          playfair
          llhttp )
endif()

if( MINGW )
  pkg_check_modules( KDEWIN REQUIRED kdewin>=0.6.3 )
  target_include_directories( airplay PRIVATE ${KDEWIN_INCLUDE_DIRS} )
endif()

# libplist
if( (UNIX OR MINGW) AND NOT APPLE )
  pkg_search_module(PLIST libplist>=2.0)
  if(NOT PLIST_FOUND)
    pkg_search_module(PLIST REQUIRED libplist-2.0)
  endif()
  find_library( LIBPLIST ${PLIST_LIBRARIES} PATH ${PLIST_LIBDIR} )
  target_link_libraries ( airplay PUBLIC ${LIBPLIST} )
elseif( APPLE )
   # use static linking
   pkg_search_module( PLIST REQUIRED libplist-2.0 )
   find_library( LIBPLIST libplist-2.0.a REQUIRED )
   message( STATUS "LIBPLIST " ${LIBPLIST} )
   target_link_libraries ( airplay  ${LIBPLIST} )
endif()
target_include_directories( airplay PRIVATE ${PLIST_INCLUDE_DIRS} )

#libcrypto
if( (UNIX OR MINGW) AND NOT APPLE )
  find_package(OpenSSL 1.1.1 REQUIRED)
  target_compile_definitions( airplay PUBLIC OPENSSL_API_COMPAT=0x10101000L )
  target_link_libraries( airplay PUBLIC OpenSSL::Crypto )
elseif( APPLE )
  # use static linking
  # can either compile Openssl 1.1.1 or 3.0.0  from source (install_dev  to /usr/local) or use Macports or Brew
  # MacPorts needs zlib with it, Brew has a strange "keg-only" installation in usr/local/opt/openssl@3
  pkg_check_modules( OPENSSL REQUIRED openssl>=1.1.1)
  message( "OPENSSL_LIBRARY_DIRS " ${OPENSSL_LIBRARY_DIRS} )
  message( "OPENSSL_INCLUDE_DIRS " ${OPENSSL_INCLUDE_DIRS} )
  find_library( LIBCRYPTO libcrypto.a PATHS ${OPENSSL_LIBRARY_DIRS} REQUIRED )
  message( "LIBCRYPTO "  ${LIBCRYPTO}  )
  target_link_libraries( airplay ${LIBCRYPTO} )
  if( LIBCRYPTO MATCHES "/opt/local/lib/libcrypto.a" ) #MacPorts openssl
    find_library( LIBZ libz.a)  # needed by MacPorts openssl
    message("(MacPorts) LIBZ= " ${LIBZ} )
    target_link_libraries( airplay  ${LIBZ} )
  endif()
  target_include_directories( airplay PRIVATE ${OPENSSL_INCLUDE_DIRS} )
endif()

#dns_sd 
if ( (UNIX OR MINGW) AND NOT APPLE )
  pkg_search_module(AVAHI_DNSSD avahi-compat-libdns_sd)
  if (AVAHI_DNSSD_FOUND)
    target_include_directories( airplay PRIVATE ${AVAHI_DNSSD_INCLUDE_DIRS} )
    find_library( DNSSD ${AVAHI_DNSSD_LIBRARIES} PATH ${AVAHI_DNSSD_LIBDIR})
  else()  # UxPlay can also build if mDNSResponder or another implementation of dns_sd is present instead of Avahi
    if ( MINGW )
      set(DNSSD "C:/Program Files/Bonjour SDK/Lib/x64/dnssd.lib")
      set(DNSSD_INCLUDE_DIR "C:/Program Files/Bonjour SDK/Include")
    else()
      find_library( DNSSD dns_sd )
      if( NOT DNSSD )
        message( FATAL_ERROR "libdns_sd missing; can be provided by avahi_compat-libdns_sd or mDNSResponder." )
      else()
        message( STATUS "dns_sd:  found" ${DNSSD} )
      endif()
    endif()

    find_path(DNSSD_INCLUDE_DIR dns_sd.h HINTS ${CMAKE_INSTALL_INCLUDEDIR} )
    if ( NOT DNSSD_INCLUDE_DIR )
      message( FATAL_ERROR " dns_sd.h not found ")
    else()
      message( STATUS "found dns_sd.h in " ${DNSSD_INCLUDE_DIR} )
    endif()
    target_include_directories( airplay PRIVATE ${DNSSD_INCLUDE_DIR} )
  endif()
  target_link_libraries(airplay PUBLIC ${DNSSD} )
endif()
