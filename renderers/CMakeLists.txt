cmake_minimum_required(VERSION 3.4.1)
  
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g")

if (APPLE )
  set( ENV{PKG_CONFIG_PATH} "/Library/FrameWorks/GStreamer.framework/Libraries/pkgconfig" ) # GStreamer.framework, preferred
  set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig" ) # standard location, and Brew
  set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/local/lib/pkgconfig/" ) # MacPorts
  message( "PKG_CONFIG_PATH (Apple, renderers) = " $ENV{PKG_CONFIG_PATH} )
  find_program( PKG_CONFIG_EXECUTABLE  pkg-config  PATHS /Library/FrameWorks/GStreamer.framework/Commands )
endif()

find_package( PkgConfig REQUIRED )
pkg_check_modules(GST REQUIRED    gstreamer-1.0>=1.4
                                  gstreamer-sdp-1.0>=1.4
                                  gstreamer-video-1.0>=1.4
                                  gstreamer-app-1.0>=1.4
)

add_library( renderers
             STATIC
             audio_renderer_gstreamer.c
	     video_renderer_gstreamer.c )

target_link_libraries ( renderers PUBLIC airplay )

# hacks to fix cmake confusion due to links in path with macOS FrameWorks

if( GST_INCLUDE_DIRS MATCHES "/Library/FrameWorks/GStreamer.framework/include" )
  set( GST_INCLUDE_DIRS "/Library/FrameWorks/GStreamer.framework/Headers")
  message( STATUS  "GST_INCLUDE_DIRS" ${GST_INCLUDE_DIRS} )
endif()
target_include_directories ( renderers PUBLIC ${GST_INCLUDE_DIRS} )

if( GST_LIBRARY_DIRS MATCHES "/Library/FrameWorks/GStreamer.framework/lib" )
  set( GST_LIBRARY_DIRS "/Library/FrameWorks/GStreamer.framework/Libraries")
  message( STATUS  "GST_LIBRARY_DIRS" ${GST_LIBRARY_DIRS} )
  target_link_libraries( renderers PUBLIC ${GST_LIBRARIES} )
  if( CMAKE_VERSION VERSION_LESS "3.13" )
    message( FATAL_ERROR "This macOS build needs cmake >= 3.13" )
  endif()
  target_link_directories ( renderers  PUBLIC ${GST_LIBRARY_DIRS} )  
elseif( CMAKE_VERSION VERSION_LESS "3.12" )
  target_link_libraries ( renderers  PUBLIC ${GST_LIBRARIES} )  
else()		  
  target_link_libraries( renderers PUBLIC ${GST_LINK_LIBRARIES} )
endif()


