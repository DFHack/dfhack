# - Find GDK-PixBuf
# Find the GDK-PixBuf include directories and libraries
#
# This module defines
#   GDKPIXBUF_FOUND       - true if the following are found
#   GDKPIXBUF_INCLUDE_DIR - GDK-PixBuf include directory
#   GDKPIXBUF_LIBRARY     - GDK-PixBuf library location

find_package( PkgConfig )
if( PKG_CONFIG_FOUND )
    # If pkg-config finds gdk-pixbuf-2.0, this will set:
    #   PC_GDKPIXBUF_FOUND (to TRUE)
    #   PC_GDKPIXBUF_INCLUDEDIR
    #   PC_GDKPIXBUF_INCLUDE_DIRS
    #   PC_GDKPIXBUF_LIBDIR
    #   PC_GDKPIXBUF_LIBRARY_DIRS
    # These variables are then used as hints to find_path()
    # and find_library()
    pkg_search_module( PC_GDKPIXBUF gdk-pixbuf-2.0 )
endif()

find_path( GDKPIXBUF_INCLUDE_DIR gdk-pixbuf/gdk-pixbuf.h
    HINTS
        # Hints provided by pkg-config
        ${PC_GDKPIXBUF_INCLUDEDIR}
        ${PC_GDKPIXBUF_INCLUDEDIR}/*
        ${PC_GDKPIXBUF_INCLUDE_DIRS}
    PATHS
        # Standard include directories
        /usr/include/
        /sw/include/
        /usr/local/include/
        # Search all subdirs of the above
        /usr/include/*
        /sw/include/*
        /usr/local/include/*
    PATH_SUFFIXES
        # Subdirectory hints
        gdk-pixbuf-2.0
        gtk-2.0
)

find_library( GDKPIXBUF_LIBRARY gdk_pixbuf-2.0
    HINTS
        # Hints provided by pkg-config
        ${PC_GDKPIXBUF_LIBDIR}
        ${PC_GDKPIXBUF_LIBRARY_DIRS}
)

include( FindPackageHandleStandardArgs )
# Sets GDKPIXBUF_FOUND to true if GDKPIXBUF_INCLUDE_DIR and
# GDKPIXBUF_LIBRARY are both set
find_package_handle_standard_args( GDKPixBuf DEFAULT_MSG
    GDKPIXBUF_LIBRARY
    GDKPIXBUF_INCLUDE_DIR
)
if( GDKPIXBUF_FOUND )
    message( STATUS "\tInclude directory: ${GDKPIXBUF_INCLUDE_DIR}" )
endif()

mark_as_advanced( GDKPIXBUF_INCLUDE_DIR GDKPIXBUF_LIBRARY )
