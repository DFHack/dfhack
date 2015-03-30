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

if(MSVC)
# disable C4819 code-page warning
add_definitions( "/wd4819" )
endif()

# set up folder structures for IDE solutions
# MSVC Express won't load solutions that use this. It also doesn't include MFC supported
# Check for MFC!
find_package(MFC QUIET)
if(MFC_FOUND OR (NOT MSVC))
    OPTION(CMAKE_USE_FOLDERS "Enable folder grouping of projects in IDEs." ON)
else()
    OPTION(CMAKE_USE_FOLDERS "Enable folder grouping of projects in IDEs." OFF)
endif()

if(CMAKE_USE_FOLDERS)
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
else()
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS OFF)
endif()

# macro for setting up IDE folders without nasty IF()s everywhere
MACRO(IDE_FOLDER target folder)
    if(CMAKE_USE_FOLDERS)
        SET_PROPERTY(TARGET ${target} PROPERTY FOLDER ${folder})
    endif()
ENDMACRO()

SET(CMAKE_MODULE_PATH
${dfhack_SOURCE_DIR}/CMake/Modules
${CMAKE_MODULE_PATH}
)

# mixing the build system with the source code is ugly and stupid. enforce the opposite :)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# make sure all the necessary submodules have been set up
if (NOT EXISTS ${dfhack_SOURCE_DIR}/library/xml/codegen.pl OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/clsocket/CMakeLists.txt)
    message(FATAL_ERROR "Required submodules could not be found! First run 'git submodule init' and 'git submodule update' from the root DFHack directory. (See the section 'Getting the Code' in Compile.html)")
endif()

# set up versioning.
set(DF_VERSION "0.40.24")
SET(DFHACK_RELEASE "r3")

set(DFHACK_VERSION "${DF_VERSION}-${DFHACK_RELEASE}")

## where to install things (after the build is done, classic 'make install' or package structure)
# the dfhack libraries will be installed here:
IF(UNIX)
    # put the lib into DF/hack
    SET(DFHACK_LIBRARY_DESTINATION hack)
    SET(DFHACK_EGGY_DESTINATION libs)
ELSE()
    # windows is crap, therefore we can't do nice things with it. leave the libs on a nasty pile...
    SET(DFHACK_LIBRARY_DESTINATION .)
    SET(DFHACK_EGGY_DESTINATION .)
ENDIF()
# external tools will be installed here:
SET(DFHACK_BINARY_DESTINATION .)
# dfhack data goes here:
SET(DFHACK_DATA_DESTINATION hack)
# plugin libs go here:
SET(DFHACK_PLUGIN_DESTINATION hack/plugins)
# dfhack header files go here:
SET(DFHACK_INCLUDES_DESTINATION hack/include)
# dfhack lua files go here:
SET(DFHACK_LUA_DESTINATION hack/lua)
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
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -Wall -Wno-unused-variable")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -m32 -march=i686 -mtune=generic -std=c++0x")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -m32 -march=i686 -mtune=generic")
ELSEIF(MSVC)
  # for msvc, tell it to always use 8-byte pointers to member functions to avoid confusion
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /vmg /vmm /MP")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Od")
ENDIF()

# use shared libraries for protobuf
ADD_DEFINITIONS(-DPROTOBUF_USE_DLLS)
ADD_DEFINITIONS(-DLUA_BUILD_AS_DLL)

if(APPLE)
    add_definitions(-D_DARWIN)
elseif(UNIX)
    add_definitions(-D_LINUX)
elseif(WIN32)
    add_definitions(-DWIN32)
endif()

#### expose depends ####

# find and make available libz
if(NOT UNIX)
    SET(ZLIB_ROOT depends/zlib/)
endif()
find_package(ZLIB REQUIRED)
include_directories(depends/protobuf)
include_directories(depends/lua/include)
include_directories(depends/md5)
include_directories(depends/tinyxml)
include_directories(depends/tthread)
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(depends/clsocket/src)
add_subdirectory(depends)


#find_package(Docutils)

#set (RST_FILES
#"Readme"
#"Compile"
#"LUA Api"
#"Contributors"
#)

#set (RST_PROCESSED_FILES "")
#IF(RST2HTML_EXECUTABLE)
#    foreach(F ${RST_FILES})
#        add_custom_command(
#            OUTPUT "${dfhack_BINARY_DIR}/${F}.html"
#            COMMAND ${RST2HTML_EXECUTABLE} "${dfhack_SOURCE_DIR}/${F}.rst" "${dfhack_BINARY_DIR}/${F}.html"
#            COMMENT "Translating ${F} to html"
#            DEPENDS "${dfhack_SOURCE_DIR}/${F}.rst")
#        list (APPEND RST_PROCESSED_FILES "${dfhack_BINARY_DIR}/${F}.html")
#    endforeach()
#    add_custom_target(HTML_DOCS ALL DEPENDS ${RST_PROCESSED_FILES})
#ENDIF()

# build the lib itself
IF(BUILD_LIBRARY)
    add_subdirectory (library)
    ## install the default documentation files
    install(FILES LICENSE NEWS "Lua API.html" Readme.html Compile.html Contributors.html DESTINATION ${DFHACK_USERDOC_DESTINATION})
    install(DIRECTORY images DESTINATION ${DFHACK_USERDOC_DESTINATION})
endif()

#build the plugins
IF(BUILD_PLUGINS)
    add_subdirectory (plugins)
endif()

# Packaging with CPack!
IF(UNIX)
    if(APPLE)
        SET(CPACK_GENERATOR "ZIP")
    else()
        SET(CPACK_GENERATOR "TBZ2")
    endif()
ELSEIF(WIN32)
    SET(CPACK_GENERATOR "ZIP")
ENDIF()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
IF(APPLE)
    set(DFHACK_PACKAGE_PLATFORM_NAME OSX)
ELSE()
    set(DFHACK_PACKAGE_PLATFORM_NAME ${CMAKE_SYSTEM_NAME})
ENDIF()
set(CPACK_PACKAGE_FILE_NAME "dfhack-${DFHACK_VERSION}-${DFHACK_PACKAGE_PLATFORM_NAME}")
INCLUDE(CPack)
