# main project file. use it from a build sub-folder, see COMPILE for details

# Set up build types
if(CMAKE_CONFIGURATION_TYPES)
  SET(CMAKE_CONFIGURATION_TYPES Release RelWithDebInfo)
  SET(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "List of supported configuration types" FORCE)
else(CMAKE_CONFIGURATION_TYPES)
  if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Release RelWithDebInfo.")
  endif (NOT CMAKE_BUILD_TYPE)
endif(CMAKE_CONFIGURATION_TYPES)

## some generic CMake magic
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(dfhack)

SET(CMAKE_MODULE_PATH
${dfhack_SOURCE_DIR}/CMake/Modules
${CMAKE_MODULE_PATH}
)

# mixing the build system with the source code is ugly and stupid. enforce the opposite :)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# set up versioning.
set(DF_VERSION_MAJOR "0")
set(DF_VERSION_MINOR "34")
set(DF_VERSION_PATCH "02")
set(DF_VERSION "${DF_VERSION_MAJOR}.${DF_VERSION_MINOR}.${DF_VERSION_PATCH}")

set(DFHACK_RELEASE "1-dev")

set(DFHACK_VERSION "${DF_VERSION_MAJOR}.${DF_VERSION_MINOR}.${DF_VERSION_PATCH}-r${DFHACK_RELEASE}")
add_definitions(-DDFHACK_VERSION="${DFHACK_VERSION}")

## where to install things (after the build is done, classic 'make install' or package structure)
# the dfhack libraries will be installed here:
IF(UNIX)
    # put the lib into DF/hack
    SET(DFHACK_LIBRARY_DESTINATION hack)
ELSE()
    # windows is crap, therefore we can't do nice things with it. leave the libs on a nasty pile...
    SET(DFHACK_LIBRARY_DESTINATION .)
ENDIF()
# external tools will be installed here:
SET(DFHACK_BINARY_DESTINATION .)
# dfhack data goes here:
SET(DFHACK_DATA_DESTINATION hack)
# plugin libs go here:
SET(DFHACK_PLUGIN_DESTINATION hack/plugins)
# dfhack header files go here:
SET(DFHACK_INCLUDES_DESTINATION hack/include)
# the windows .lib file goes here:
SET(DFHACK_DEVLIB_DESTINATION hack)

# user documentation goes here:
SET(DFHACK_USERDOC_DESTINATION hack)
# developer documentation goes here:
SET(DFHACK_DEVDOC_DESTINATION hack)

## some options for the user/developer to play with
OPTION(BUILD_LIBRARY "Build the library that goes into DF." ON)
OPTION(BUILD_PLUGINS "Build the plugins." ON)

## flags for GCC
# default to hidden symbols
# build 32bit
# ensure compatibility with older CPUs
# enable C++11 features
IF(UNIX)
  add_definitions(-DLINUX_BUILD)
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
  SET(CMAKE_CXX_FLAGS "-fvisibility=hidden -m32 -march=i686 -mtune=generic -std=c++0x")
  SET(CMAKE_C_FLAGS "-fvisibility=hidden -m32 -march=i686 -mtune=generic")
ENDIF()

#add depends to include path
INCLUDE_DIRECTORIES ( library/depends )

# build the static lua for dfusion
INCLUDE_DIRECTORIES ( lua/include )
add_subdirectory (lua)

# build the lib itself
IF(BUILD_LIBRARY)
    add_subdirectory (library)
    ## install the default documentation files
    install(FILES LICENSE Readme.html Compile.html DESTINATION ${DFHACK_USERDOC_DESTINATION})
endif()

#build the plugins
IF(BUILD_PLUGINS)
    add_subdirectory (plugins)
endif()

# Packaging with CPack!
IF(UNIX)
    SET(CPACK_GENERATOR "TGZ")
ELSEIF(WIN32)
    SET(CPACK_GENERATOR "ZIP")
ENDIF()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_FILE_NAME "dfhack-${DFHACK_VERSION}-${CMAKE_SYSTEM_NAME}")
INCLUDE(CPack)
