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

option(BUILD_CURL_EXE "Set to ON to build curl executable." OFF)
option(HTTP_ONLY "disables all protocols except HTTP (This overrides all CURL_DISABLE_* options)" ON)
option(CMAKE_USE_LIBSSH2 "Use libSSH2" OFF)
option(ENABLE_UNIX_SOCKETS "Define if you want Unix domain sockets support" OFF)
option(BUILD_TESTING "Disable curl build testing" OFF)
add_subdirectory(curl)
