#list depends here.
add_subdirectory(lua)
add_subdirectory(md5)
add_subdirectory(protobuf)

# Don't build tinyxml if it's being externally linked against.
if(NOT TinyXML_FOUND)
    add_subdirectory(tinyxml)
endif()

add_subdirectory(tthread)
OPTION(JSONCPP_WITH_TESTS "Compile and (for jsoncpp_check) run JsonCpp test executables" OFF)
OPTION(JSONCPP_WITH_POST_BUILD_UNITTEST "Automatically run unit-tests as a post build step" OFF)
add_subdirectory(jsoncpp-sub EXCLUDE_FROM_ALL)
# build clsocket static and only as a dependency. Setting those options here overrides its own default settings.
OPTION(CLSOCKET_SHARED "Build clsocket lib as shared." OFF)
OPTION(CLSOCKET_DEP_ONLY "Build for use inside other CMake projects as dependency." ON)
add_subdirectory(clsocket)
IDE_FOLDER(clsocket "Depends")

# These options are overrides that are applied to the options and parameters used in the curl CMakeLists file.
# They are copied from this file directly as originally defined, then the values are modified to our perferred ones.
# Generally we are attempting to make a static lib, with all fancy functionality disabled. We just want basic REST
# client functionality for now, with no frills.
option(BUILD_CURL_EXE "Set to ON to build curl executable." OFF) # default ON, but we only want to build the lib not the exe
option(HTTP_ONLY "disables all protocols except HTTP (This overrides all CURL_DISABLE_* options)" ON) # default OFF
option(CMAKE_USE_LIBSSH2 "Use libSSH2" OFF) # default ON
option(ENABLE_UNIX_SOCKETS "Define if you want Unix domain sockets support" OFF) # default ON
option(BUILD_TESTING "Disable curl build testing" OFF) # default ON, set by CTest
set(CURL_CA_BUNDLE "none" CACHE STRING "Path to the CA bundle. Set 'none' to disable or 'auto' for auto-detection. Defaults to 'auto'." FORCE) # default "auto"
set(CURL_CA_FALLBACK OFF CACHE BOOL "Set ON to use built-in CA store of TLS backend. Defaults to OFF" FORCE) # default OFF
set(CURL_CA_PATH "none" CACHE STRING "Location of default CA path. Set 'none' to disable or 'auto' for auto-detection. Defaults to 'auto'." FORCE) # default "auto"
option(CURL_DISABLE_CRYPTO_AUTH "to disable cryptographic authentication" ON) # default OFF
option(ENABLE_IPV6 "Define if you want to enable IPv6 support" OFF) # default OFF
option(CURL_ZLIB "Set to ON to enable building curl with zlib support." OFF) # default ON

add_subdirectory(curl)