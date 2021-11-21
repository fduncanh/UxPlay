cmake_minimum_required( VERSION 3.4.3 )

project( uxplay )

message( STATUS "Project name: " ${PROJECT_NAME} )

include(GNUInstallDirs)

set ( CMAKE_CXX_STANDARD 11 )

if ( ZOOMFIX )
add_definitions( -DX_DISPLAY_FIX )
find_package( X11 REQUIRED )
link_libraries( ${X11_LIBRARIES} )
include_directories( ${X11_INCLUDE_DIR} )
endif ( ZOOMFIX )

if( UNIX AND NOT APPLE )
 add_definitions( -DSUPPRESS_AVAHI_COMPAT_WARNING )
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(DNSSD REQUIRED avahi-compat-libdns_sd)
pkg_check_modules(PLIST REQUIRED libplist-2.0)
if(NOT PLIST_FOUND)
  pkg_check_modules(PLIST REQUIRED libplist)
endif()

link_directories(${PLIST_LIBRARY_DIRS})

add_subdirectory( lib/llhttp )
add_subdirectory( lib/playfair )
add_subdirectory( lib )
add_subdirectory( renderers )

add_executable( uxplay uxplay.cpp )

if( UNIX AND NOT APPLE )
  include_directories( uxplay ${GST_INCLUDE_DIRS} )
else()
  include_directories( uxplay
                  /Library/FrameWorks/GStreamer.framework/Headers/
                  /usr/local/include
		  /usr/local/include/glib-2.0
		  /usr/local/lib/glib-2.0/include
                  /opt/local/include
		  /opt/local/include/glib-2.0
		  /opt/local/lib/glib-2.0/include
                  )
endif()

target_link_libraries( uxplay
                   renderers
                   airplay
                   ${PLIST_LIBRARIES}
		   )

install( TARGETS  uxplay RUNTIME DESTINATION bin )
install( FILES uxplay.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 )
install( FILES README.md README.txt README.html LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR} ) 

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
