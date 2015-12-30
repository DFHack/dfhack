# main project file. use it from a build sub-folder, see COMPILE for details

# prevent CMake warnings about INTERFACE_LINK_LIBRARIES vs LINK_INTERFACE_LIBRARIES
IF(CMAKE_VERSION VERSION_GREATER "2.8.12")
    CMAKE_POLICY(SET CMP0022 OLD)
ENDIF()

# Set up build types
if(CMAKE_CONFIGURATION_TYPES)
  SET(CMAKE_CONFIGURATION_TYPES Release RelWithDebInfo)
  SET(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "List of supported configuration types" FORCE)
else(CMAKE_CONFIGURATION_TYPES)
  if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Release RelWithDebInfo.")
  endif (NOT CMAKE_BUILD_TYPE)
endif(CMAKE_CONFIGURATION_TYPES)

OPTION(BUILD_DOCS "Choose whether to build the documentation (requires python and Sphinx)." OFF)

## some generic CMake magic
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(dfhack)

macro(CHECK_GCC COMPILER_PATH)
  execute_process(COMMAND ${COMPILER_PATH} -dumpversion OUTPUT_VARIABLE GCC_VERSION_OUT)
  string(STRIP "${GCC_VERSION_OUT}" GCC_VERSION_OUT)
  if (${GCC_VERSION_OUT} VERSION_LESS "4.5")
    message(SEND_ERROR "${COMPILER_PATH} version ${GCC_VERSION_OUT} cannot be used - use GCC 4.5 or later")
  elseif (${GCC_VERSION_OUT} VERSION_GREATER "4.9.9")
    # GCC 5 changes ABI name mangling to enable C++11 changes.
    # This must be disabled to enable linking against DF.
    # http://developerblog.redhat.com/2015/02/05/gcc5-and-the-c11-abi/
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
  endif()
endmacro()

if(UNIX)
  if(CMAKE_COMPILER_IS_GNUCC)
    CHECK_GCC(${CMAKE_C_COMPILER})
  else()
    message(SEND_ERROR "C compiler is not GCC")
  endif()
  if(CMAKE_COMPILER_IS_GNUCXX)
    CHECK_GCC(${CMAKE_CXX_COMPILER})
  else()
    message(SEND_ERROR "C++ compiler is not GCC")
  endif()
endif()

if(WIN32)
  if((NOT MSVC) OR (NOT MSVC_VERSION STREQUAL 1600))
    message(SEND_ERROR "MSVC 2010 is required")
  endif()
endif()

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
    message(SEND_ERROR "One or more required submodules could not be found! Run 'git submodule update --init' from the root DFHack directory. (See the section 'Getting the Code' in Compile.rst or Compile.html)")
endif()

# set up versioning.
set(DF_VERSION "0.42.04")
SET(DFHACK_RELEASE "alpha1")
SET(DFHACK_PRERELEASE TRUE)

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

IF(UNIX)
  ## flags for GCC
  # default to hidden symbols
  # build 32bit
  # ensure compatibility with older CPUs
  # enable C++11 features
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
    set(CMAKE_MACOSX_RPATH 1)
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
include_directories(depends/jsoncpp)
include_directories(depends/tinyxml)
include_directories(depends/tthread)
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(depends/clsocket/src)
add_subdirectory(depends)

find_package(Git REQUIRED)
if(NOT GIT_FOUND)
    message(SEND_ERROR "could not find git")
endif()

# build the lib itself
IF(BUILD_LIBRARY)
    add_subdirectory (library)
    install(FILES LICENSE.rst NEWS.rst DESTINATION ${DFHACK_USERDOC_DESTINATION})
endif()

install(DIRECTORY dfhack-config/ DESTINATION dfhack-config/default)

#build the plugins
IF(BUILD_PLUGINS)
    add_subdirectory (plugins)
endif()

add_subdirectory(scripts)

find_package(Sphinx QUIET)
if (BUILD_DOCS)
  if (NOT SPHINX_FOUND)
    message(SEND_ERROR "Sphinx not found but BUILD_DOCS enabled")
  endif()

  file(GLOB SPHINX_DEPS
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/*.rst"
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/images/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/styles/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/conf.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/about.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/*/about.txt"
  )
  file(GLOB_RECURSE SPHINX_SCRIPT_DEPS
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.lua"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.rb"
  )
  set(SPHINX_DEPS ${SPHINX_DEPS} ${SPHINX_SCRIPT_DEPS}
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.rst"
    "${CMAKE_CURRENT_SOURCE_DIR}/NEWS.rst"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
  )

  set(SPHINX_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/docs/html/.buildinfo")
  set_source_files_properties(${SPHINX_OUTPUT} PROPERTIES GENERATED TRUE)
  add_custom_command(OUTPUT ${SPHINX_OUTPUT}
    COMMAND ${SPHINX_EXECUTABLE}
      -a -E -q -b html
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/html"
      -w "${CMAKE_CURRENT_SOURCE_DIR}/docs/_sphinx-warnings.txt"
      -j 2
    DEPENDS ${SPHINX_DEPS}
    COMMENT "Building HTML documentation with Sphinx"
  )

  add_custom_target(dfhack_docs ALL
    DEPENDS ${SPHINX_OUTPUT}
  )
  # Sphinx doesn't touch this file if it didn't make changes,
  # which makes CMake think it didn't complete
  add_custom_command(TARGET dfhack_docs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E touch ${SPHINX_OUTPUT})

  install(DIRECTORY ${dfhack_SOURCE_DIR}/docs/html/
      DESTINATION ${DFHACK_USERDOC_DESTINATION}/docs
      )
  install(FILES "README.html" DESTINATION "${DFHACK_DATA_DESTINATION}")
endif()

# Packaging with CPack!
SET(DFHACK_PACKAGE_SUFFIX "")
IF(UNIX)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    string(STRIP ${GCC_VERSION} GCC_VERSION)
    SET(DFHACK_PACKAGE_SUFFIX "-gcc-${GCC_VERSION}")
    if(APPLE)
        SET(CPACK_GENERATOR "ZIP;TBZ2")
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
set(CPACK_PACKAGE_FILE_NAME "dfhack-${DFHACK_VERSION}-${DFHACK_PACKAGE_PLATFORM_NAME}${DFHACK_PACKAGE_SUFFIX}")
INCLUDE(CPack)

#INCLUDE(FindSphinx.cmake)
