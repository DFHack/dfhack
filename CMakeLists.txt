# main project file. use it from a build sub-folder, see COMPILE for details

## some generic CMake magic
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
project(dfhack)

if("${CMAKE_GENERATOR}" STREQUAL Ninja)
    if("${CMAKE_VERSION}" VERSION_LESS 3.9)
        message(WARNING "You are using an old version of CMake (${CMAKE_VERSION}) with Ninja. This may result in ninja errors - see docs/Compile.rst for more details. Upgrading your CMake version is recommended.")
    endif()
endif()

# Set up build types
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo" CACHE STRING "List of supported configuration types" FORCE)
else(CMAKE_CONFIGURATION_TYPES)
    set(DFHACK_TYPE_HELP "Choose the type of build, options are: Release and RelWithDebInfo")
    # Prevent cmake C module attempts to overwrite our help string
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "${DFHACK_TYPE_HELP}")
    else(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "${DFHACK_TYPE_HELP}")
    endif(NOT CMAKE_BUILD_TYPE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Release;RelWithDebInfo")
endif(CMAKE_CONFIGURATION_TYPES)

option(BUILD_DOCS "Choose whether to build the documentation (requires python and Sphinx)." OFF)
option(REMOVE_SYMBOLS_FROM_DF_STUBS "Remove debug symbols from DF stubs. (Reduces libdfhack size to about half but removes a few useful symbols)" ON)

macro(CHECK_GCC compiler_path)
    execute_process(COMMAND ${compiler_path} -dumpversion OUTPUT_VARIABLE GCC_VERSION_OUT)
    string(STRIP "${GCC_VERSION_OUT}" GCC_VERSION_OUT)
    if(${GCC_VERSION_OUT} VERSION_LESS "4.8")
        message(SEND_ERROR "${compiler_path} version ${GCC_VERSION_OUT} cannot be used - use GCC 4.8 or later")
    elseif(${GCC_VERSION_OUT} VERSION_GREATER "4.9.9")
        # GCC 5 changes ABI name mangling to enable C++11 changes.
        # This must be disabled to enable linking against DF.
        # http://developerblog.redhat.com/2015/02/05/gcc5-and-the-c11-abi/
        add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
    endif()
endmacro()

if(UNIX)
    if(CMAKE_COMPILER_IS_GNUCC)
        check_gcc(${CMAKE_C_COMPILER})
    else()
        message(SEND_ERROR "C compiler is not GCC")
    endif()
    if(CMAKE_COMPILER_IS_GNUCXX)
        check_gcc(${CMAKE_CXX_COMPILER})
    else()
        message(SEND_ERROR "C++ compiler is not GCC")
    endif()
endif()

if(WIN32)
    if((NOT MSVC) OR (MSVC_VERSION LESS 1900) OR (MSVC_VERSION GREATER 1919))
        message(SEND_ERROR "MSVC 2015 or 2017 is required")
    endif()
endif()

# Ask for C++11 standard from compilers
set(CMAKE_CXX_STANDARD 11)
# Require the standard support from compilers.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Use only standard c++ to keep code portable
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # disable C4819 code-page warning
    add_definitions("/wd4819")

    # disable use of POSIX name warnings
    add_definitions("/D_CRT_NONSTDC_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS")

    # supress C4503 - VC++ dislikes if a name is too long. If you get
    # weird and mysterious linking errors, you can disable this, but you'll have to
    # deal with a LOT of compiler noise over it
    # see https://msdn.microsoft.com/en-us/library/074af4b6.aspx
    add_definitions("/wd4503")

    # suppress C4267 - VC++ complains whenever we implicitly convert an integer to
    # a smaller type, and most of the time this is just conversion from 64 to 32 bits
    # for things like vector sizes, which are never that big anyway.
    add_definitions("/wd4267")
endif()

# Automatically detect architecture based on Visual Studio generator
if(MSVC AND NOT DEFINED DFHACK_BUILD_ARCH)
    if(${CMAKE_GENERATOR} MATCHES "Win64")
        set(DFHACK_BUILD_ARCH "64")
    else()
        set(DFHACK_BUILD_ARCH "32")
    endif()
else()
    set(DFHACK_BUILD_ARCH "64" CACHE STRING "Architecture to build ('32' or '64')")
endif()

if("${DFHACK_BUILD_ARCH}" STREQUAL "32")
    set(DFHACK_BUILD_32 1)
    set(DFHACK_BUILD_64 0)
    set(DFHACK_SETARCH "i386")
elseif("${DFHACK_BUILD_ARCH}" STREQUAL "64")
    set(DFHACK_BUILD_32 0)
    set(DFHACK_BUILD_64 1)
    set(DFHACK_SETARCH "x86_64")
    add_definitions(-DDFHACK64)
else()
    message(SEND_ERROR "Invalid build architecture (should be 32/64): ${DFHACK_BUILD_ARCH}")
endif()

if(CMAKE_CROSSCOMPILING)
    set(DFHACK_NATIVE_BUILD_DIR "DFHACK_NATIVE_BUILD_DIR-NOTFOUND" CACHE FILEPATH "Path to a native build directory")
    include("${DFHACK_NATIVE_BUILD_DIR}/ImportExecutables.cmake")
endif()

find_package(Perl REQUIRED)

# set up folder structures for IDE solutions
# MSVC Express won't load solutions that use this. It also doesn't include MFC supported
# Check for MFC!
find_package(MFC QUIET)
if(MFC_FOUND OR (NOT MSVC))
    option(CMAKE_USE_FOLDERS "Enable folder grouping of projects in IDEs." ON)
else()
    option(CMAKE_USE_FOLDERS "Enable folder grouping of projects in IDEs." OFF)
endif()

if(CMAKE_USE_FOLDERS)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
else()
    set_property(GLOBAL PROPERTY USE_FOLDERS OFF)
endif()

# macro for setting up IDE folders without nasty if()s everywhere
macro(IDE_FOLDER target folder)
    if(CMAKE_USE_FOLDERS)
        set_property(TARGET ${target} PROPERTY FOLDER ${folder})
    endif()
endmacro()

set(CMAKE_MODULE_PATH
    ${dfhack_SOURCE_DIR}/CMake/Modules
    ${CMAKE_MODULE_PATH}
)

# generates compile_commands.json, used for autocompletion by some editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <cstdlib>
#include <cuchar>
int main(void) {
    char32_t in = 0;
    char out[MB_CUR_MAX];
    std::mbstate_t state{};
    std::c32rtomb(out, in, &state);
    return 0;
}" HAVE_CUCHAR2)
if(HAVE_CUCHAR2)
    add_definitions("-DHAVE_CUCHAR")
endif()

# mixing the build system with the source code is ugly and stupid. enforce the opposite :)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# make sure all the necessary submodules have been set up
if(NOT EXISTS ${dfhack_SOURCE_DIR}/library/xml/codegen.pl OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/clsocket/CMakeLists.txt)
    message(SEND_ERROR "One or more required submodules could not be found! Run 'git submodule update --init' from the root DFHack directory. (See the section 'Getting the Code' in docs/Compile.rst)")
endif()

# set up versioning.
set(DF_VERSION "0.47.03")
set(DFHACK_RELEASE "beta1")
set(DFHACK_PRERELEASE TRUE)

set(DFHACK_VERSION "${DF_VERSION}-${DFHACK_RELEASE}")

set(DFHACK_ABI_VERSION 1)

set(DFHACK_BUILD_ID "" CACHE STRING "Build ID (should be specified on command line)")

## where to install things (after the build is done, classic 'make install' or package structure)
# the dfhack libraries will be installed here:
if(UNIX)
    # put the lib into DF/hack
    set(DFHACK_LIBRARY_DESTINATION hack)
    set(DFHACK_EGGY_DESTINATION libs)
else()
    # windows is crap, therefore we can't do nice things with it. leave the libs on a nasty pile...
    set(DFHACK_LIBRARY_DESTINATION .)
    set(DFHACK_EGGY_DESTINATION .)
endif()

# external tools will be installed here:
set(DFHACK_BINARY_DESTINATION .)
# dfhack data goes here:
set(DFHACK_DATA_DESTINATION hack)
# plugin libs go here:
set(DFHACK_PLUGIN_DESTINATION hack/plugins)
# dfhack header files go here:
set(DFHACK_INCLUDES_DESTINATION hack/include)
# dfhack lua files go here:
set(DFHACK_LUA_DESTINATION hack/lua)
# the windows .lib file goes here:
set(DFHACK_DEVLIB_DESTINATION hack)

# user documentation goes here:
set(DFHACK_USERDOC_DESTINATION hack)
# developer documentation goes here:
set(DFHACK_DEVDOC_DESTINATION hack)

# some options for the user/developer to play with
option(BUILD_LIBRARY "Build the library that goes into DF." ON)
option(BUILD_PLUGINS "Build the plugins." ON)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
if(UNIX)
    ## flags for GCC
    # default to hidden symbols
    # build 32bit
    # ensure compatibility with older CPUs
    # enable C++11 features
    add_definitions(-DLINUX_BUILD)
    add_definitions(-D_GLIBCXX_USE_C99)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -Wall -Wno-unused-variable")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -mtune=generic")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -mtune=generic")
    if(DFHACK_BUILD_64)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -mno-avx")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -mno-avx")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -march=i686")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i686")
    endif()
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
elseif(MSVC)
    # for msvc, tell it to always use 8-byte pointers to member functions to avoid confusion
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /vmg /vmm /MP")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Od")
    string(REPLACE "/O2" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()

# use shared libraries for protobuf
add_definitions(-DPROTOBUF_USE_DLLS)
add_definitions(-DLUA_BUILD_AS_DLL)

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
        download_file("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/win64-zlib.lib"
            ${ZLIB_DOWNLOAD_DIR}/zlib.lib
            "a3b2fc6b68efafa89b0882e354fc8418")
    else()
        download_file("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/win32-zlib.lib"
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
        download_file("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/win64-SDL.dll"
            ${SDLREAL_DOWNLOAD_DIR}/SDLreal.dll
            "1ae242c4b94cb03756a1288122a66faf")
    else()
        download_file("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/win32-SDL.dll"
            ${SDLREAL_DOWNLOAD_DIR}/SDLreal.dll
            "5a09604daca6b2b5ce049d79af935d6a")
    endif()
endif()

if(APPLE)
    # libstdc++ (GCC 4.8.5 for OS X 10.6)
    # fixes crash-on-unwind bug in DF's libstdc++
    set(LIBSTDCXX_DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/package/darwin/osx${DFHACK_BUILD_ARCH})

    if(${GCC_VERSION_OUT} VERSION_LESS "4.9")
        set(LIBSTDCXX_GCC_VER "48")
    else()
        set(LIBSTDCXX_GCC_VER "7")
        set(LIBSTDCXX_DOWNLOAD_DIR "${LIBSTDCXX_DOWNLOAD_DIR}-gcc7")
    endif()

    if(${DFHACK_BUILD_ARCH} STREQUAL "64")
        if(${LIBSTDCXX_GCC_VER} STREQUAL "48")
            download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/osx64-gcc48-libstdcxx.6.dylib.gz"
                "gz"
                ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib.gz
                "cf26ed588be8e83c8e3a49919793b416"
                ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib
                "16dc6dbd4ecde7f9b95bb6dc91f07404")
        else()
            # GCC 7
            download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/osx64-gcc7-libstdcxx.6.dylib.gz"
                "gz"
                ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib.gz
                "81314b7846f9e8806409bef2160c76e6"
                ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib
                "93b6cf4b01e9a9084a508fd6a4a88992")
        endif()

    else() # 32-bit

        if(${LIBSTDCXX_GCC_VER} STREQUAL "48")
            download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/osx32-gcc48-libstdcxx.6.dylib.gz"
                "gz"
                ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib.gz
                "40f3d83871b114f0279240626311621b"
                ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib
                "c3f5678b8204917e03870834902c3e8b")
        else()
            # GCC 7
            download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/osx32-gcc7-libstdcxx.6.dylib.gz"
                "gz"
                ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib.gz
                "dbd213171f66edb90d204d525f10c969"
                ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib
                "b14c857e7e485a097c70a9ccd3132da7")
        endif()
    endif()

    if(NOT EXTERNAL_LIBSTDCXX)
        install(PROGRAMS ${LIBSTDCXX_DOWNLOAD_DIR}/libstdc++.6.dylib
            DESTINATION ./hack/)
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

# Support linking against external tinyxml
# If we find an external tinyxml, set the DFHACK_TINYXML variable to "tinyxml"
# Otherwise, set it to "dfhack-tinyxml"
option(EXTERNAL_TINYXML "Choose to link against external TinyXML" OFF)
if(EXTERNAL_TINYXML)
    find_package(TinyXML REQUIRED)
    if(NOT TinyXML_FOUND)
        message(SEND_ERROR "Could not find an external TinyXML, consider setting EXTERNAL_TINYXML to OFF.")
    endif()
    set(DFHACK_TINYXML "tinyxml")
else()
    include_directories(depends/tinyxml)
    set(DFHACK_TINYXML "dfhack-tinyxml")
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
if(BUILD_LIBRARY)
    add_subdirectory(library)
    install(FILES LICENSE.rst docs/changelog.txt DESTINATION ${DFHACK_USERDOC_DESTINATION})
endif()

file(WRITE "${CMAKE_BINARY_DIR}/dfhack_setarch.txt" ${DFHACK_SETARCH})
install(FILES "${CMAKE_BINARY_DIR}/dfhack_setarch.txt" DESTINATION "${DFHACK_DATA_DESTINATION}")

install(DIRECTORY dfhack-config/ DESTINATION dfhack-config/default)
install(DIRECTORY test DESTINATION "${DFHACK_DATA_DESTINATION}")

# build the plugins
if(BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

add_subdirectory(scripts)

find_package(Sphinx QUIET)
if(BUILD_DOCS)
    if(NOT SPHINX_FOUND)
        message(SEND_ERROR "Sphinx not found but BUILD_DOCS enabled")
    endif()

    file(GLOB SPHINX_DEPS
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/*.rst"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/changelog.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/gen_changelog.py"
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
        DESTINATION ${DFHACK_USERDOC_DESTINATION}/docs)
    install(FILES docs/_auto/news.rst docs/_auto/news-dev.rst DESTINATION ${DFHACK_USERDOC_DESTINATION})
    install(FILES "README.html" DESTINATION "${DFHACK_DATA_DESTINATION}")
endif()

# Packaging with CPack!
set(DFHACK_PACKAGE_SUFFIX "")
if(UNIX)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    string(STRIP ${GCC_VERSION} GCC_VERSION)
    set(DFHACK_PACKAGE_SUFFIX "-gcc-${GCC_VERSION}")
    set(CPACK_GENERATOR "TBZ2")
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")
endif()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
if(APPLE)
    set(DFHACK_PACKAGE_PLATFORM_NAME OSX)
else()
    set(DFHACK_PACKAGE_PLATFORM_NAME ${CMAKE_SYSTEM_NAME})
endif()
# set on command line
if(DFHACK_BUILD_ID STREQUAL "")
    set(DFHACK_BUILD_ID_PACKAGE "")
else()
    set(DFHACK_BUILD_ID_PACKAGE "${DFHACK_BUILD_ID}-")
endif()
set(CPACK_PACKAGE_FILE_NAME "dfhack-${DFHACK_VERSION}-${DFHACK_BUILD_ID_PACKAGE}${DFHACK_PACKAGE_PLATFORM_NAME}-${DFHACK_BUILD_ARCH}${DFHACK_PACKAGE_SUFFIX}")
include(CPack)

option(DFHACK_INCLUDE_CORE "Download and include Dwarf Fortress core files in DFHack. Useful for local testing, but should not be used in releases." OFF)
if(DFHACK_INCLUDE_CORE)
    string(REPLACE "." "_" DF_CORE_FILENAME "${DF_VERSION}")
    string(REGEX REPLACE "^0_" "df_" DF_CORE_FILENAME "${DF_CORE_FILENAME}")
    if(UNIX)
        if(APPLE)
            string(APPEND DF_CORE_FILENAME "_osx")
        else()
            string(APPEND DF_CORE_FILENAME "_linux")
        endif()
        if(DFHACK_BUILD_32)
            string(APPEND DF_CORE_FILENAME "32")
        endif()
        string(APPEND DF_CORE_FILENAME ".tar.bz2")
    else()
        string(APPEND DF_CORE_FILENAME "_win")
        if(DFHACK_BUILD_32)
            string(APPEND DF_CORE_FILENAME "32")
        endif()
        string(APPEND DF_CORE_FILENAME ".zip")
    endif()
    set(DF_CORE_URL "http://bay12games.com/dwarves/${DF_CORE_FILENAME}")
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/${DF_CORE_FILENAME}")
        file(DOWNLOAD "${DF_CORE_URL}" "${CMAKE_BINARY_DIR}/${DF_CORE_FILENAME}" SHOW_PROGRESS)
    endif()
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/df-core")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/df-core")
    if(UNIX)
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xjf "../${DF_CORE_FILENAME}" --strip-components=1
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/df-core")
    else()
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "../${DF_CORE_FILENAME}" --format=zip
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/df-core")
        file(REMOVE "${CMAKE_BINARY_DIR}/df-core/SDL.dll")
    endif()
    install(DIRECTORY "${CMAKE_BINARY_DIR}/df-core/"
        DESTINATION .)
endif()

# Store old build arch
set(DFHACK_BUILD_ARCH_PREV "${DFHACK_BUILD_ARCH}" CACHE STRING "Previous build architecture" FORCE)

option(BUILD_SIZECHECK "Build the sizecheck library, for research" OFF)
if(BUILD_SIZECHECK)
    add_subdirectory(depends/sizecheck)
    add_dependencies(dfhack sizecheck)
endif()
