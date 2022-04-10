
if("${CMAKE_GENERATOR}" STREQUAL Ninja)
    if("${CMAKE_VERSION}" VERSION_LESS 3.9)
        message(WARNING "You are using an old version of CMake (${CMAKE_VERSION}) with Ninja. This may result in ninja errors - see docs/Compile.rst for more details. Upgrading your CMake version is recommended.")
    endif()
endif()

if(NOT("${CMAKE_VERSION}" VERSION_LESS 3.12))
    # make ZLIB_ROOT work in CMake >= 3.12
    # https://cmake.org/cmake/help/git-stage/policy/CMP0074.html
    cmake_policy(SET CMP0074 NEW)
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

    # MSVC panics if an object file contains more than 65,279 sections. this
    # happens quite frequently with code that uses templates, such as vectors.
    add_definitions("/bigobj")
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

# generates compile_commands.json, used for autocompletion by some editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set up versioning.
set(DF_VERSION "0.47.05")
set(DFHACK_RELEASE "r4")
set(DFHACK_PRERELEASE FALSE)

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

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
if(UNIX)
    ## flags for GCC
    # default to hidden symbols
    # build 32bit
    # ensure compatibility with older CPUs
    # enable C++11 features
    add_definitions(-DLINUX_BUILD)
    add_definitions(-D_GLIBCXX_USE_C99)
    set(GCC_COMMON_FLAGS "-fvisibility=hidden -mtune=generic -Wall -Werror")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g")
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
elseif(MSVC)
    # for msvc, tell it to always use 8-byte pointers to member functions to avoid confusion
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /vmg /vmm /MP")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Od")
    string(REPLACE "/O2" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()

if(APPLE)
    add_definitions(-D_DARWIN)
    set(CMAKE_MACOSX_RPATH 1)
elseif(UNIX)
    add_definitions(-D_LINUX)
elseif(WIN32)
    add_definitions(-DWIN32)
endif()
