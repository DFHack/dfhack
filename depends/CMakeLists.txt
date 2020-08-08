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

set(XLSXIO_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/xlsxio)
ExternalProject_Add(xlsxio_project
    PREFIX      xlsxio
    SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/xlsxio
    INSTALL_DIR ${XLSXIO_INSTALL_DIR}
    BUILD_BYPRODUCTS ${XLSXIO_INSTALL_DIR}/lib/libxlsxio_read.a
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${XLSXIO_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC=ON -DBUILD_SHARED=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_TOOLS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DOCUMENTATION=OFF -DWITH_LIBZIP=ON -DZLIB_DIR=${ZLIB_DIR}
)
    #-DLIBZIP_DIR=
    #-DEXPAT_DIR=
