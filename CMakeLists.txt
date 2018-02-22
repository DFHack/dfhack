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
  if (${GCC_VERSION_OUT} VERSION_LESS "4.8")
    message(SEND_ERROR "${COMPILER_PATH} version ${GCC_VERSION_OUT} cannot be used - use GCC 4.8 or later")
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
  if((NOT MSVC) OR (NOT MSVC_VERSION STREQUAL 1900))
    message(SEND_ERROR "MSVC 2015 is required")
  endif()
endif()

if(MSVC)
# disable C4819 code-page warning
add_definitions( "/wd4819" )

# Disable use of POSIX name warnings
add_definitions ( "/D_CRT_NONSTDC_NO_WARNINGS")

# supress C4503 - VC++ dislikes if a name is too long. If you get
# weird and mysterious linking errors, you can disable this, but you'll have to
# deal with a LOT of compiler noise over it
# see https://msdn.microsoft.com/en-us/library/074af4b6.aspx
add_definitions( "/wd4503")
endif()

# Automatically detect architecture based on Visual Studio generator
IF(MSVC AND NOT DEFINED DFHACK_BUILD_ARCH)
    IF(${CMAKE_GENERATOR} MATCHES "Win64")
        SET(DFHACK_BUILD_ARCH "64")
    ELSE()
        SET(DFHACK_BUILD_ARCH "32")
    ENDIF()
ELSE()
    SET(DFHACK_BUILD_ARCH "32" CACHE STRING "Architecture to build ('32' or '64')")
ENDIF()

IF("${DFHACK_BUILD_ARCH}" STREQUAL "32")
  SET(DFHACK_BUILD_32 1)
  SET(DFHACK_BUILD_64 0)
  set(DFHACK_SETARCH "i386")
ELSEIF("${DFHACK_BUILD_ARCH}" STREQUAL "64")
  SET(DFHACK_BUILD_32 0)
  SET(DFHACK_BUILD_64 1)
  set(DFHACK_SETARCH "x86_64")
  ADD_DEFINITIONS(-DDFHACK64)
ELSE()
  MESSAGE(SEND_ERROR "Invalid build architecture (should be 32/64): ${DFHACK_BUILD_ARCH}")
ENDIF()

IF(CMAKE_CROSSCOMPILING)
  SET(DFHACK_NATIVE_BUILD_DIR "DFHACK_NATIVE_BUILD_DIR-NOTFOUND" CACHE FILEPATH "Path to a native build directory")
  INCLUDE("${DFHACK_NATIVE_BUILD_DIR}/ImportExecutables.cmake")
ENDIF()

find_package(Perl REQUIRED)

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

# generates compile_commands.json, used for autocompletion by some editors
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# mixing the build system with the source code is ugly and stupid. enforce the opposite :)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# make sure all the necessary submodules have been set up
if (NOT EXISTS ${dfhack_SOURCE_DIR}/library/xml/codegen.pl OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/clsocket/CMakeLists.txt)
    message(SEND_ERROR "One or more required submodules could not be found! Run 'git submodule update --init' from the root DFHack directory. (See the section 'Getting the Code' in docs/Compile.rst)")
endif()

# set up versioning.
set(DF_VERSION "0.44.05")
set(DFHACK_RELEASE "r2")
set(DFHACK_PRERELEASE FALSE)

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

SET(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
IF(UNIX)
  ## flags for GCC
  # default to hidden symbols
  # build 32bit
  # ensure compatibility with older CPUs
  # enable C++11 features
  add_definitions(-DLINUX_BUILD)
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -Wall -Wno-unused-variable")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -mtune=generic -std=c++0x")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -mtune=generic")
  IF(DFHACK_BUILD_64)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -mno-avx")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -mno-avx")
  ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -march=i686")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i686")
  ENDIF()
  STRING(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
ELSEIF(MSVC)
  # for msvc, tell it to always use 8-byte pointers to member functions to avoid confusion
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /vmg /vmm /MP")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Od")
  STRING(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
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

#### download depends ####

include(CMake/DownloadFile.cmake)

if(WIN32)
  # Download zlib on Windows
  set(ZLIB_DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/depends/zlib/lib/win${DFHACK_BUILD_ARCH})
  if(${DFHACK_BUILD_ARCH} STREQUAL "64")
    download_file("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/win64-zlib.lib"
      ${ZLIB_DOWNLOAD_DIR}/zlib.lib
      "a3b2fc6b68efafa89b0882e354fc8418")
  else()
    download_file("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/win32-zlib.lib"
      ${ZLIB_DOWNLOAD_DIR}/zlib.lib
      "f4ebaa21d9de28566e88b1edfcdff901")
  endif()

  # Move zlib to the build folder so possible 32 and 64-bit builds
  # in the same source tree don't conflict
  file(COPY ${CMAKE_SOURCE_DIR}/depends/zlib
      DESTINATION ${CMAKE_BINARY_DIR}/depends/)
  file(COPY ${ZLIB_DOWNLOAD_DIR}/zlib.lib
      DESTINATION ${CMAKE_BINARY_DIR}/depends/zlib/lib/)


  # Do the same for SDLreal.dll
  # (DFHack doesn't require this at build time, so no need to move it to the build folder)
  set(SDLREAL_DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/package/windows/win${DFHACK_BUILD_ARCH})
  if(${DFHACK_BUILD_ARCH} STREQUAL "64")
    download_file("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/win64-SDL.dll"
      ${SDLREAL_DOWNLOAD_DIR}/SDLreal.dll
      "1ae242c4b94cb03756a1288122a66faf")
  else()
    download_file("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/win32-SDL.dll"
      ${SDLREAL_DOWNLOAD_DIR}/SDLreal.dll
      "5a09604daca6b2b5ce049d79af935d6a")
  endif()
endif()

if(APPLE)
  # libstdc++ (GCC 4.8.5 for OS X 10.6)
  # fixes crash-on-unwind bug in DF's libstdc++
  set(LIBSTDCXX_DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/package/darwin/osx${DFHACK_BUILD_ARCH})
  if(${DFHACK_BUILD_ARCH} STREQUAL "64")
    download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/osx64-libstdcxx.6.dylib.gz"
      "gz"
      ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib.gz
      "cf26ed588be8e83c8e3a49919793b416"
      ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib
      "16dc6dbd4ecde7f9b95bb6dc91f07404")
  else()
    download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/osx32-libstdcxx.6.dylib.gz"
      "gz"
      ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib.gz
      "40f3d83871b114f0279240626311621b"
      ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib
      "c3f5678b8204917e03870834902c3e8b")
  endif()
endif()

#### expose depends ####

if(UNIX)
    # Rescan for pthread and zlib if the build arch changed
    if(NOT "${DFHACK_BUILD_ARCH}" STREQUAL "${DFHACK_BUILD_ARCH_PREV}")
        unset(ZLIB_LIBRARY CACHE)
        unset(CMAKE_HAVE_PTHREAD_H CACHE)
    endif()
endif()

# find and make available libz
if(NOT UNIX) # Windows
    # zlib is in here so 32-bit and 64-bit builds in the same source tree are possible
    set(ZLIB_ROOT ${CMAKE_BINARY_DIR}/depends/zlib/)
else()
    if(NOT APPLE AND DFHACK_BUILD_32)
        # 32-bit Linux
        set(ZLIB_ROOT /usr/lib/i386-linux-gnu)
    endif()
endif()

find_package(ZLIB REQUIRED)
include_directories(depends/protobuf)
include_directories(depends/lua/include)
include_directories(depends/md5)
include_directories(depends/jsoncpp)

# Support linking against external tinyxml
# If we find an external tinyxml, set the DFHACK_TINYXML variable to "tinyxml"
# Otherwise, set it to "dfhack-tinyxml"
option(EXTERNAL_TINYXML "Choose to link against external TinyXML" OFF)
if(EXTERNAL_TINYXML)
    find_package(TinyXML REQUIRED)
    if(NOT TinyXML_FOUND)
        message(SEND_ERROR "Could not find an external TinyXML, consider setting EXTERNAL_TINYXML to OFF.")
    endif()
    SET(DFHACK_TINYXML "tinyxml")
else()
    include_directories(depends/tinyxml)
    SET(DFHACK_TINYXML "dfhack-tinyxml")
endif()

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

file(WRITE "${CMAKE_BINARY_DIR}/dfhack_setarch.txt" ${DFHACK_SETARCH})
install(FILES "${CMAKE_BINARY_DIR}/dfhack_setarch.txt" DESTINATION "${DFHACK_DATA_DESTINATION}")

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
    SET(CPACK_GENERATOR "TBZ2")
ELSEIF(WIN32)
    SET(CPACK_GENERATOR "ZIP")
ENDIF()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
IF(APPLE)
    set(DFHACK_PACKAGE_PLATFORM_NAME OSX)
ELSE()
    set(DFHACK_PACKAGE_PLATFORM_NAME ${CMAKE_SYSTEM_NAME})
ENDIF()
set(CPACK_PACKAGE_FILE_NAME "dfhack-${DFHACK_VERSION}-${DFHACK_PACKAGE_PLATFORM_NAME}-${DFHACK_BUILD_ARCH}${DFHACK_PACKAGE_SUFFIX}")
INCLUDE(CPack)

#INCLUDE(FindSphinx.cmake)

# Store old build arch
SET(DFHACK_BUILD_ARCH_PREV "${DFHACK_BUILD_ARCH}" CACHE STRING "Previous build architecture" FORCE)
