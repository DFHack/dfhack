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

# assemble environment args to pass on to dependency projects
get_cmake_property(vars CACHE_VARIABLES)
foreach(var ${vars})
  if(var MATCHES "^CMAKE_" AND NOT var MATCHES "^CMAKE_CACHE" AND NOT var MATCHES "^CMAKE_HOME" AND NOT var MATCHES "^CMAKE_PROJECT")
    message("${var} = [${${var}}]")
    list(APPEND CL_ARGS "-D${var}=${${var}}")
  endif()
endforeach()
message("final CL_ARGS = ${CL_ARGS}")

set(LIBEXPAT_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libexpat)
ExternalProject_Add(libexpat_project
    PREFIX      libexpat
    SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/libexpat/expat
    INSTALL_DIR ${LIBEXPAT_INSTALL_DIR}
    BUILD_BYPRODUCTS ${LIBEXPAT_INSTALL_DIR}/lib/libexpat.a
    CMAKE_ARGS  ${CL_ARGS} -DCMAKE_INSTALL_PREFIX=${LIBEXPAT_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release -DEXPAT_BUILD_EXAMPLES=OFF -DEXPAT_BUILD_TESTS=OFF -DEXPAT_BUILD_TOOLS=OFF -DEXPAT_SHARED_LIBS=OFF -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
)

set(LIBZIP_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libzip)
ExternalProject_Add(libzip_project
    PREFIX      libzip
    SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/libzip
    INSTALL_DIR ${LIBZIP_INSTALL_DIR}
    BUILD_BYPRODUCTS ${LIBZIP_INSTALL_DIR}/lib/libzip.a
    CMAKE_ARGS  ${CL_ARGS} -DCMAKE_INSTALL_PREFIX=${LIBZIP_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release -DBUILD_DOC=OFF -DBUILD_EXAMPLES=OFF -DBUILD_REGRESS=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_TOOLS=OFF -DENABLE_BZIP2=OFF -DENABLE_COMMONCRYPTO=OFF -DENABLE_GNUTLS=OFF -DENABLE_LZMA=OFF -DENABLE_MBEDTLS=OFF -DENABLE_OPENSSL=OFF -DENABLE_WINDOWS_CRYPTO=OFF -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
)

set(XLSXIO_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/xlsxio)
ExternalProject_Add(xlsxio_project
    PREFIX      xlsxio
    SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/xlsxio
    INSTALL_DIR ${XLSXIO_INSTALL_DIR}
    BUILD_BYPRODUCTS ${XLSXIO_INSTALL_DIR}/lib/libxlsxio_read.a
    CMAKE_ARGS  ${CL_ARGS} -DCMAKE_INSTALL_PREFIX=${XLSXIO_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC=ON -DBUILD_SHARED=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_TOOLS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DOCUMENTATION=OFF -DWITH_LIBZIP=ON -DZLIB_DIR=${ZLIB_DIR} -DLIBZIP_DIR=${LIBZIP_INSTALL_DIR} -DEXPAT_DIR=${LIBEXPAT_INSTALL_DIR} -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
)
add_dependencies(xlsxio_project libexpat_project)
add_dependencies(xlsxio_project libzip_project)
