#list depends here.
add_subdirectory(lua)
add_subdirectory(md5)
add_subdirectory(protobuf)

# Don't build tinyxml if it's being externally linked against.
if(NOT TinyXML_FOUND)
    add_subdirectory(tinyxml)
endif()

add_subdirectory(tthread)
add_subdirectory(jsoncpp)
# build clsocket static and only as a dependency. Setting those options here overrides its own default settings.
OPTION(CLSOCKET_SHARED "Build clsocket lib as shared." OFF)
OPTION(CLSOCKET_DEP_ONLY "Build for use inside other CMake projects as dependency." ON)
add_subdirectory(clsocket)
IDE_FOLDER(clsocket "Depends")
