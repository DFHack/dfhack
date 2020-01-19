# list depends here.
add_subdirectory(lua)
add_subdirectory(md5)
add_subdirectory(protobuf)

# Don't build tinyxml if it's being externally linked against.
if(NOT TinyXML_FOUND)
    add_subdirectory(tinyxml)
endif()

add_subdirectory(tthread)
option(JSONCPP_WITH_TESTS "Compile and (for jsoncpp_check) run JsonCpp test executables" OFF)
option(JSONCPP_WITH_POST_BUILD_UNITTEST "Automatically run unit-tests as a post build step" OFF)
add_subdirectory(jsoncpp-sub EXCLUDE_FROM_ALL)
# build clsocket static and only as a dependency. Setting those options here overrides its own default settings.
option(CLSOCKET_SHARED "Build clsocket lib as shared." OFF)
option(CLSOCKET_DEP_ONLY "Build for use inside other CMake projects as dependency." ON)
add_subdirectory(clsocket)
ide_folder(clsocket "Depends")
