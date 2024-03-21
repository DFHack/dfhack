# main project file. use it from a build sub-folder, see COMPILE for details

## some generic CMake magic
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0074 NEW)
project(dfhack)

# set up versioning.
set(DF_VERSION "50.12")
set(DFHACK_RELEASE "r2")
set(DFHACK_PRERELEASE FALSE)

set(DFHACK_VERSION "${DF_VERSION}-${DFHACK_RELEASE}")
set(DFHACK_ABI_VERSION 2)
set(DFHACK_BUILD_ID "" CACHE STRING "Build ID (should be specified on command line)")

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
option(BUILD_DOCS_NO_HTML "Don't build the HTML docs, only the in-game docs." OFF)
option(REMOVE_SYMBOLS_FROM_DF_STUBS "Remove debug symbols from DF stubs. (Reduces libdfhack size to about half but removes a few useful symbols)" ON)

macro(CHECK_GCC compiler_path)
    execute_process(COMMAND ${compiler_path} -dumpversion OUTPUT_VARIABLE GCC_VERSION_OUT)
    string(STRIP "${GCC_VERSION_OUT}" GCC_VERSION_OUT)
    if(${GCC_VERSION_OUT} VERSION_LESS "10")
        message(SEND_ERROR "${compiler_path} version ${GCC_VERSION_OUT} cannot be used - use GCC 10 or later")
        # TODO: this may need to be removed when DF linux actually comes out
        # TODO: and we can test
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
    if(NOT MSVC)
        message(SEND_ERROR "No MSVC found! MSVC 2022 version 1930 to 1939 is required.")
    elseif((MSVC_VERSION LESS 1930) OR (MSVC_VERSION GREATER 1939))
        message(SEND_ERROR "MSVC 2022 version 1930 to 1939 is required, Version Found: ${MSVC_VERSION}")
    endif()
endif()

# Ask for C++-20 standard from compilers
set(CMAKE_CXX_STANDARD 20)
# Require the standard support from compilers.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Use only standard c++ to keep code portable
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # increase warning level and treat warnings as errors
    add_compile_options("/WX")
    add_compile_options("/W3")

    # disable C4819 code-page warning
    add_compile_options("/wd4819")

    # disable use of POSIX name warnings
    add_definitions("/D_CRT_NONSTDC_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS")

    # supress C4503 - VC++ dislikes if a name is too long. If you get
    # weird and mysterious linking errors, you can disable this, but you'll have to
    # deal with a LOT of compiler noise over it
    # see https://msdn.microsoft.com/en-us/library/074af4b6.aspx
    add_compile_options("/wd4503")

    # suppress C4267 - VC++ complains whenever we implicitly convert an integer to
    # a smaller type, and most of the time this is just conversion from 64 to 32 bits
    # for things like vector sizes, which are never that big anyway.
    add_compile_options("/wd4267")

    # MSVC panics if an object file contains more than 65,279 sections. this
    # happens quite frequently with code that uses templates, such as vectors.
    add_compile_options("/bigobj")
endif()

# Automatically detect architecture based on Visual Studio generator
if(MSVC AND NOT DEFINED DFHACK_BUILD_ARCH)
    if ((${CMAKE_GENERATOR} MATCHES "Win32") OR (${CMAKE_GENERATOR} MATCHES "x86"))
        message(SEND_ERROR "DF v50 does not support 32-bit")
    else()
        set(DFHACK_BUILD_ARCH "64")
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
    message(SEND_ERROR "Invalid build architecture (should be 32 or 64): ${DFHACK_BUILD_ARCH}")
endif()

if(CMAKE_CROSSCOMPILING)
    set(DFHACK_NATIVE_BUILD_DIR "DFHACK_NATIVE_BUILD_DIR-NOTFOUND" CACHE FILEPATH "Path to a native build directory")
    include("${DFHACK_NATIVE_BUILD_DIR}/ImportExecutables.cmake")
endif()

find_package(Perl REQUIRED)

# set up folder structures for IDE solutions
# checking for msvc express is meaningless now, all available editions of msvc support folder groupings
option(CMAKE_USE_FOLDERS "Enable folder grouping of projects in IDEs." ON)

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
if("${dfhack_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# make sure all the necessary submodules have been set up
if(NOT EXISTS ${dfhack_SOURCE_DIR}/library/xml/codegen.pl
    OR NOT EXISTS ${dfhack_SOURCE_DIR}/scripts/CMakeLists.txt
    OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/clsocket/CMakeLists.txt
    OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/jsoncpp-sub/CMakeLists.txt
    OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/libexpat/expat/CMakeLists.txt
    OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/libzip/CMakeLists.txt
    OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/xlsxio/CMakeLists.txt
    OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/googletest/CMakeLists.txt
    OR NOT EXISTS ${dfhack_SOURCE_DIR}/depends/luacov/src
)
    message(SEND_ERROR "One or more required submodules could not be found! Run 'git submodule update --init' from the root DFHack directory. (See the section 'Getting the Code' in docs/dev/compile/Compile.rst)")
endif()

# dfhack data goes here:
set(DFHACK_DATA_DESTINATION hack)

## where to install things (after the build is done, classic 'make install' or package structure)
# the dfhack libraries will be installed here:
if(UNIX)
    # put the lib into DF/hack
    set(DFHACK_LIBRARY_DESTINATION ${DFHACK_DATA_DESTINATION})
else()
    # windows is crap, therefore we can't do nice things with it. leave the libs on a nasty pile...
    set(DFHACK_LIBRARY_DESTINATION .)
endif()

# external tools will be installed here:
set(DFHACK_BINARY_DESTINATION .)
# plugin libs go here:
set(DFHACK_PLUGIN_DESTINATION ${DFHACK_DATA_DESTINATION}/plugins)
# dfhack lua files go here:
set(DFHACK_LUA_DESTINATION ${DFHACK_DATA_DESTINATION}/lua)

# user documentation goes here:
set(DFHACK_USERDOC_DESTINATION ${DFHACK_DATA_DESTINATION})

# some options for the user/developer to play with
option(BUILD_LIBRARY "Build the DFHack library." ON)
option(BUILD_PLUGINS "Build the DFHack plugins." ON)
option(INSTALL_SCRIPTS "Install DFHack scripts." ON)
option(INSTALL_DATA_FILES "Install DFHack platform independent files." ON)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
if(UNIX)
    ## flags for GCC
    # default to hidden symbols
    # ensure compatibility with older CPUs
    add_definitions(-DLINUX_BUILD)
    set(GCC_COMMON_FLAGS "-fvisibility=hidden -mtune=generic -Wall -Werror -Wl,--disable-new-dtags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COMMON_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COMMON_FLAGS}")
    if(DFHACK_BUILD_64)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -mno-avx")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -mno-avx")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -march=i686")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i686")
    endif()
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_INSTALL_RPATH ${DFHACK_LIBRARY_DESTINATION})
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

#### dependencies ####

# fix for pyenv: default to `python3` before `python3.x`
set(Python_FIND_UNVERSIONED_NAMES FIRST)

include(CMake/DownloadFile.cmake)

if(WIN32)
    set(ZLIB_FILE zlib.lib)
    set(ZLIB_PATH ${dfhack_SOURCE_DIR}/depends/zlib/)
    set(ZLIB_MD5 a3b2fc6b68efafa89b0882e354fc8418)
    download_file("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/win64-${ZLIB_FILE}"
        ${ZLIB_PATH}lib/${ZLIB_FILE}
        ${ZLIB_MD5})
    set(ZLIB_ROOT ${ZLIB_PATH})
else()
    # Rescan for pthread and zlib if the build arch changed
    if(NOT "${DFHACK_BUILD_ARCH}" STREQUAL "${DFHACK_BUILD_ARCH_PREV}")
        unset(ZLIB_LIBRARY CACHE)
        unset(CMAKE_HAVE_PTHREAD_H CACHE)
    endif()

    if(NOT APPLE AND DFHACK_BUILD_32)
        set(ZLIB_ROOT /usr/lib/i386-linux-gnu)
    endif()
endif()
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

if(BUILD_LIBRARY)
    # Download SDL release and extract into depends in the build dir
    # all we need are the header files (including generated headers), so the same release package
    # will work for all platforms
    # (the above statement is untested for OSX)
    set(SDL_VERSION 2.26.2)
    set(SDL_ZIP_MD5 574daf26d48de753d0b1e19823c9d8bb)
    set(SDL_ZIP_FILE SDL2-devel-${SDL_VERSION}-VC.zip)
    set(SDL_ZIP_PATH ${dfhack_SOURCE_DIR}/depends/SDL2/)
    download_file("https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/${SDL_ZIP_FILE}"
        ${SDL_ZIP_PATH}${SDL_ZIP_FILE}
        ${SDL_ZIP_MD5})
    file(ARCHIVE_EXTRACT INPUT ${SDL_ZIP_PATH}${SDL_ZIP_FILE}
        DESTINATION ${SDL_ZIP_PATH})
    include_directories(${SDL_ZIP_PATH}/SDL2-${SDL_VERSION}/include)
endif()

if(APPLE)
    # libstdc++ (GCC 4.8.5 for OS X 10.6)
    # fixes crash-on-unwind bug in DF's libstdc++
    set(LIBSTDCXX_DOWNLOAD_DIR ${dfhack_SOURCE_DIR}/package/darwin/osx${DFHACK_BUILD_ARCH})

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

include_directories(depends/lodepng)
include_directories(depends/clsocket/src)
include_directories(depends/xlsxio/include)

if(BUILD_LIBRARY)
    add_subdirectory(depends)
endif()

# Testing with CTest
macro(dfhack_test name files)
if(BUILD_LIBRARY AND UNIX AND NOT APPLE) # remove this once our MSVC build env has been updated
    add_executable(${name} ${files})
    target_include_directories(${name} PUBLIC depends/googletest/googletest/include)
    target_link_libraries(${name} dfhack gtest)
    add_test(NAME ${name} COMMAND ${name})
endif()
endmacro()
include(CTest)

find_package(Git REQUIRED)
if(NOT GIT_FOUND)
    message(SEND_ERROR "could not find git")
endif()

# build the lib itself
add_subdirectory(library)
if(BUILD_LIBRARY)
    file(WRITE ${CMAKE_BINARY_DIR}/dfhack_setarch.txt ${DFHACK_SETARCH})
    install(FILES ${CMAKE_BINARY_DIR}/dfhack_setarch.txt DESTINATION ${DFHACK_DATA_DESTINATION})
endif()

# build the plugins
add_subdirectory(plugins)

if(INSTALL_DATA_FILES)
    add_subdirectory(data)
    install(FILES LICENSE.rst DESTINATION ${DFHACK_USERDOC_DESTINATION})
    install(FILES docs/changelog-placeholder.txt DESTINATION ${DFHACK_USERDOC_DESTINATION} RENAME changelog.txt)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/depends/luacov/src/luacov/ DESTINATION ${DFHACK_DATA_DESTINATION}/lua/luacov)
endif()

if(INSTALL_SCRIPTS)
    add_subdirectory(scripts)
endif()

if(BUILD_DOCS)
    find_package(Python3)
    find_package(Sphinx)

    if(NOT SPHINX_FOUND)
        message(SEND_ERROR "Sphinx not found but BUILD_DOCS enabled")
    endif()

    file(GLOB SPHINX_GLOB_DEPS
        LIST_DIRECTORIES false
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/images/*.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/styles/*"
        "${CMAKE_CURRENT_SOURCE_DIR}/data/init/*init"
    )
    file(GLOB_RECURSE SPHINX_GLOB_RECURSE_DEPS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.rst"
        "${CMAKE_CURRENT_SOURCE_DIR}/changelog.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/*py"
    )
    list(FILTER SPHINX_GLOB_RECURSE_DEPS
        EXCLUDE REGEX "docs/changelogs"
    )
    list(FILTER SPHINX_GLOB_RECURSE_DEPS
        EXCLUDE REGEX "docs/html"
    )
    list(FILTER SPHINX_GLOB_RECURSE_DEPS
        EXCLUDE REGEX "docs/tags"
    )
    list(FILTER SPHINX_GLOB_RECURSE_DEPS
        EXCLUDE REGEX "docs/text"
    )
    list(FILTER SPHINX_GLOB_RECURSE_DEPS
        EXCLUDE REGEX "docs/tools"
    )
    set(SPHINX_DEPS ${SPHINX_GLOB_DEPS} ${SPHINX_GLOB_RECURSE_DEPS} ${SPHINX_SCRIPT_DEPS}
        "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/conf.py"
    )

    if(BUILD_DOCS_NO_HTML)
        set(SPHINX_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/docs/text/index.txt")
        set(SPHINX_BUILD_TARGETS text)
    else()
        set(SPHINX_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/docs/html/.buildinfo")
        set(SPHINX_BUILD_TARGETS html text)
    endif()

    set_property(
        DIRECTORY PROPERTY ADDITIONAL_CLEAN_FILES TRUE
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/changelogs"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/html"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/pdf"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/pseudoxml"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/tags"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/text"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/tools"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/xml"
        "${CMAKE_BINARY_DIR}/docs/html"
        "${CMAKE_BINARY_DIR}/docs/pdf"
        "${CMAKE_BINARY_DIR}/docs/pseudoxml"
        "${CMAKE_BINARY_DIR}/docs/text"
        "${CMAKE_BINARY_DIR}/docs/xml"
    )

    add_custom_command(OUTPUT ${SPHINX_OUTPUT}
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/docs/build.py"
        ${SPHINX_BUILD_TARGETS} --sphinx="${SPHINX_EXECUTABLE}" -- -q -W
        DEPENDS ${SPHINX_DEPS}
        COMMENT "Building documentation with Sphinx"
    )

    add_custom_target(dfhack_docs ALL
        DEPENDS ${SPHINX_OUTPUT}
    )
    # Sphinx doesn't touch this file if it didn't make changes,
    # which makes CMake think it didn't complete
    add_custom_command(TARGET dfhack_docs POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E touch ${SPHINX_OUTPUT})

    if(NOT BUILD_DOCS_NO_HTML)
        install(DIRECTORY ${dfhack_SOURCE_DIR}/docs/html/
            DESTINATION ${DFHACK_USERDOC_DESTINATION}/docs
            FILES_MATCHING PATTERN "*"
                           PATTERN html/_sources EXCLUDE)
    endif()
    install(DIRECTORY ${dfhack_SOURCE_DIR}/docs/text/
        DESTINATION ${DFHACK_USERDOC_DESTINATION}/docs)
    install(FILES docs/changelogs/news.rst docs/changelogs/news-dev.rst DESTINATION ${DFHACK_USERDOC_DESTINATION})
    install(FILES "README.html" DESTINATION "${DFHACK_DATA_DESTINATION}")
endif()

option(BUILD_TESTS "Include tests (currently just installs Lua tests into the scripts folder)" OFF)
if(BUILD_TESTS)
    if(EXISTS "${dfhack_SOURCE_DIR}/test/scripts")
        message(SEND_ERROR "test/scripts must not exist in the dfhack repo since it would conflict with the tests installed from the scripts repo.")
    endif()
    install(DIRECTORY ${dfhack_SOURCE_DIR}/test
        DESTINATION ${DFHACK_DATA_DESTINATION}/scripts)
    install(FILES ci/test.lua DESTINATION ${DFHACK_DATA_DESTINATION}/scripts)
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
set(CPACK_PACKAGE_FILE_NAME "dfhack-${DFHACK_VERSION}-${DFHACK_BUILD_ID_PACKAGE}${DFHACK_PACKAGE_PLATFORM_NAME}-${DFHACK_BUILD_ARCH}bit${DFHACK_PACKAGE_SUFFIX}")
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
if(BUILD_LIBRARY AND BUILD_SIZECHECK)
    add_subdirectory(depends/sizecheck)
    add_dependencies(dfhack sizecheck)
endif()

add_subdirectory(package)
