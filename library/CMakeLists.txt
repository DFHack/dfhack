PROJECT (dfapi)
cmake_minimum_required(VERSION 2.8)

# prevent CMake warnings about INTERFACE_LINK_LIBRARIES vs LINK_INTERFACE_LIBRARIES
IF(CMAKE_VERSION VERSION_GREATER "2.8.12")
    CMAKE_POLICY(SET CMP0022 OLD)
ENDIF()

## build options
OPTION(BUILD_DEVEL "Install/package files required for development (For SDK)." OFF)
OPTION(BUILD_DOXYGEN "Create/install/package doxygen documentation for DFHack (For SDK)." OFF)
IF(UNIX)
    OPTION(CONSOLE_NO_CATCH "Make the console not catch 'CTRL+C' events for easier debugging." OFF)
ENDIF()

include_directories (proto)
include_directories (include)

SET(PERL_EXECUTABLE "perl" CACHE FILEPATH "This is the perl executable to run in the codegen step. Tweak it if you need to run a specific one.")

execute_process(COMMAND ${PERL_EXECUTABLE} xml/list.pl xml ${dfapi_SOURCE_DIR}/include/df ";"
                WORKING_DIRECTORY ${dfapi_SOURCE_DIR}
                OUTPUT_VARIABLE GENERATED_HDRS)

SET_SOURCE_FILES_PROPERTIES(${GENERATED_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE GENERATED TRUE)

SET(MAIN_HEADERS
include/Internal.h
include/DFHack.h
include/DFHackVersion.h
include/Console.h
include/Core.h
include/ColorText.h
include/DataDefs.h
include/DataIdentity.h
include/VTableInterpose.h
include/LuaWrapper.h
include/LuaTools.h
include/Error.h
include/Export.h
include/Hooks.h
include/MiscUtils.h
include/Module.h
include/Pragma.h
include/MemAccess.h
include/TileTypes.h
include/Types.h
include/VersionInfo.h
include/VersionInfoFactory.h
include/RemoteClient.h
include/RemoteServer.h
include/RemoteTools.h
)

SET(MAIN_HEADERS_WINDOWS
include/wdirent.h
)

SET(MAIN_SOURCES
Core.cpp
ColorText.cpp
DataDefs.cpp
VTableInterpose.cpp
LuaWrapper.cpp
LuaTypes.cpp
LuaTools.cpp
LuaApi.cpp
DataStatics.cpp
DataStaticsCtor.cpp
DataStaticsFields.cpp
MiscUtils.cpp
Types.cpp
PluginManager.cpp
TileTypes.cpp
VersionInfoFactory.cpp
RemoteClient.cpp
RemoteServer.cpp
RemoteTools.cpp
)

SET(MAIN_SOURCES_WINDOWS
Console-windows.cpp
Hooks-windows.cpp
PlugLoad-windows.cpp
Process-windows.cpp
)

IF(WIN32)
    SOURCE_GROUP("Main\\Headers" FILES ${MAIN_HEADERS} ${MAIN_HEADERS_WINDOWS})
    SOURCE_GROUP("Main\\Sources" FILES ${MAIN_SOURCES} ${MAIN_SOURCES_WINDOWS})
ENDIF()

SET(MAIN_SOURCES_LINUX
Console-posix.cpp
Hooks-linux.cpp
PlugLoad-posix.cpp
Process-linux.cpp
)

SET(MAIN_SOURCES_DARWIN
Console-posix.cpp
Hooks-darwin.cpp
PlugLoad-posix.cpp
Process-darwin.cpp
)

SET(MAIN_SOURCES_LINUX_EGGY
Console-linux.cpp
Hooks-egg.cpp
PlugLoad-linux.cpp
Process-linux.cpp
)

SET(MODULE_HEADERS
include/modules/Buildings.h
include/modules/Burrows.h
include/modules/Constructions.h
include/modules/Units.h
include/modules/Engravings.h
include/modules/EventManager.h
include/modules/Gui.h
include/modules/GuiHooks.h
include/modules/Items.h
include/modules/Job.h
include/modules/kitchen.h
include/modules/Maps.h
include/modules/MapCache.h
include/modules/Materials.h
include/modules/Notes.h
include/modules/Persistent.h
include/modules/Random.h
include/modules/Renderer.h
include/modules/Screen.h
include/modules/Translation.h
include/modules/Vermin.h
include/modules/World.h
include/modules/Graphic.h
include/modules/Once.h
include/modules/Filesystem.h
)

SET( MODULE_SOURCES
modules/Buildings.cpp
modules/Burrows.cpp
modules/Constructions.cpp
modules/Units.cpp
modules/Engravings.cpp
modules/EventManager.cpp
modules/Gui.cpp
modules/Items.cpp
modules/Job.cpp
modules/kitchen.cpp
modules/MapCache.cpp
modules/Maps.cpp
modules/Materials.cpp
modules/Notes.cpp
modules/Persistent.cpp
modules/Random.cpp
modules/Renderer.cpp
modules/Screen.cpp
modules/Translation.cpp
modules/Vermin.cpp
modules/World.cpp
modules/Graphic.cpp
modules/Windows.cpp
modules/Once.cpp
modules/Filesystem.cpp
)

SET(STATIC_FIELDS_FILES)
FOREACH(GROUP other a b c d e f g h i j k l m n o p q r s t u v w x y z)
    SET(STATIC_FIELDS_FILENAME ${dfhack_SOURCE_DIR}/library/DataStaticsFields/${GROUP}.cpp)
    IF(${GROUP} STREQUAL "other")
        SET(STATIC_FIELDS_INC_FILENAME "df/static.fields.inc")
    ELSE()
        SET(STATIC_FIELDS_INC_FILENAME "df/static.fields-${GROUP}.inc")
    ENDIF()
    FILE(WRITE ${STATIC_FIELDS_FILENAME}.tmp
        "#define STATIC_FIELDS_GROUP\n"
        "#include \"../DataStaticsFields.cpp\"\n"
        "#include \"${STATIC_FIELDS_INC_FILENAME}\"\n"
    )
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${STATIC_FIELDS_FILENAME}.tmp ${STATIC_FIELDS_FILENAME})
    FILE(REMOVE ${STATIC_FIELDS_FILENAME}.tmp)
    LIST(APPEND STATIC_FIELDS_FILES ${STATIC_FIELDS_FILENAME})
ENDFOREACH()
LIST(APPEND MAIN_SOURCES ${STATIC_FIELDS_FILES})

IF(WIN32)
    SOURCE_GROUP("Modules\\Headers" FILES ${MODULE_HEADERS})
    SOURCE_GROUP("Modules\\Sources" FILES ${MODULE_SOURCES})
    SOURCE_GROUP("Generated" FILES ${GENERATED_HDRS})
ENDIF()

SET(PROJECT_HEADERS)
LIST(APPEND PROJECT_HEADERS ${MAIN_HEADERS})
LIST(APPEND PROJECT_HEADERS ${MODULE_HEADERS})
SET(PROJECT_SOURCES)
LIST(APPEND PROJECT_SOURCES ${MAIN_SOURCES})
LIST(APPEND PROJECT_SOURCES ${MODULE_SOURCES})

IF(UNIX)
    OPTION(BUILD_EGGY "Make DFHack strangely egg-shaped." OFF)
    IF(BUILD_EGGY)
        LIST(APPEND PROJECT_SOURCES ${MAIN_SOURCES_LINUX_EGGY})
    ELSEIF(APPLE)
        LIST(APPEND PROJECT_SOURCES ${MAIN_SOURCES_DARWIN})
    ELSE()
        LIST(APPEND PROJECT_SOURCES ${MAIN_SOURCES_LINUX})
    ENDIF()
ELSEIF(WIN32)
    LIST(APPEND PROJECT_SOURCES ${MAIN_SOURCES_WINDOWS})
    LIST(APPEND PROJECT_HEADERS ${MAIN_HEADERS_WINDOWS})
ENDIF()

# Protobuf
FILE(GLOB PROJECT_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

STRING(REPLACE ".proto" ".pb.cc" PROJECT_PROTO_SRCS "${PROJECT_PROTOS}")
STRING(REPLACE ".proto" ".pb.h" PROJECT_PROTO_HDRS "${PROJECT_PROTOS}")

LIST(APPEND PROJECT_HEADERS ${PROJECT_PROTO_HDRS})
LIST(APPEND PROJECT_SOURCES ${PROJECT_PROTO_SRCS})

ADD_CUSTOM_COMMAND(
    OUTPUT ${PROJECT_PROTO_SRCS} ${PROJECT_PROTO_HDRS}
    COMMAND protoc-bin -I=${CMAKE_CURRENT_SOURCE_DIR}/proto/
            --cpp_out=dllexport_decl=DFHACK_EXPORT:${CMAKE_CURRENT_SOURCE_DIR}/proto/
            ${PROJECT_PROTOS}
    DEPENDS protoc-bin ${PROJECT_PROTOS}
)

# Merge headers into sources
SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HEADERS} PROPERTIES HEADER_FILE_ONLY TRUE )
LIST(APPEND PROJECT_SOURCES ${PROJECT_HEADERS})

# Generation
LIST(APPEND PROJECT_SOURCES ${GENERATED_HDRS})

FILE(GLOB GENERATE_INPUT_SCRIPTS ${dfapi_SOURCE_DIR}/xml/*.pm ${dfapi_SOURCE_DIR}/xml/*.xslt)
FILE(GLOB GENERATE_INPUT_XMLS ${dfapi_SOURCE_DIR}/xml/*.xml)

ADD_CUSTOM_COMMAND(
    OUTPUT ${dfapi_SOURCE_DIR}/include/df/codegen.out.xml
    COMMAND ${PERL_EXECUTABLE} xml/codegen.pl xml include/df
    WORKING_DIRECTORY ${dfapi_SOURCE_DIR}
    MAIN_DEPENDENCY ${dfapi_SOURCE_DIR}/xml/codegen.pl
    DEPENDS ${GENERATE_INPUT_XMLS} ${GENERATE_INPUT_SCRIPTS}
)

ADD_CUSTOM_TARGET(generate_headers DEPENDS ${dfapi_SOURCE_DIR}/include/df/codegen.out.xml)

IF(UNIX)
  # Don't produce debug info for generated stubs
  SET_SOURCE_FILES_PROPERTIES(DataStatics.cpp DataStaticsCtor.cpp DataStaticsFields.cpp ${STATIC_FIELDS_FILES}
                              PROPERTIES COMPILE_FLAGS "-g0 -O1")
ELSE(WIN32)
  SET_SOURCE_FILES_PROPERTIES(DataStatics.cpp DataStaticsCtor.cpp DataStaticsFields.cpp ${STATIC_FIELDS_FILES}
                              PROPERTIES COMPILE_FLAGS "/O1 /bigobj")
ENDIF()

# Compilation

ADD_DEFINITIONS(-DBUILD_DFHACK_LIB)

IF(UNIX)
    IF(CONSOLE_NO_CATCH)
        ADD_DEFINITIONS(-DCONSOLE_NO_CATCH)
    ENDIF()
ENDIF()

IF(APPLE)
  SET(PROJECT_LIBS dl dfhack-md5 dfhack-tinyxml dfhack-tinythread)
ELSEIF(UNIX)
  SET(PROJECT_LIBS rt dl dfhack-md5 dfhack-tinyxml dfhack-tinythread)
ELSE(WIN32)
  #FIXME: do we really need psapi?
  SET(PROJECT_LIBS psapi dfhack-md5 dfhack-tinyxml dfhack-tinythread)
ENDIF()

ADD_LIBRARY(dfhack-version STATIC DFHackVersion.cpp)
SET_PROPERTY(TARGET dfhack-version APPEND PROPERTY COMPILE_DEFINITIONS
    DFHACK_VERSION="${DFHACK_VERSION}"
    DF_VERSION="${DF_VERSION}"
    DFHACK_RELEASE="${DFHACK_RELEASE}"
)
IF(DFHACK_PRERELEASE)
    SET_PROPERTY(TARGET dfhack-version APPEND PROPERTY COMPILE_DEFINITIONS
        DFHACK_PRERELEASE=1
    )
ENDIF()

ADD_CUSTOM_TARGET(git-describe
    COMMAND ${CMAKE_COMMAND}
        -D dfhack_SOURCE_DIR:STRING=${dfhack_SOURCE_DIR}
        -D GIT_EXECUTABLE:STRING=${GIT_EXECUTABLE}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/git-describe.cmake
    COMMENT "Obtaining git commit information"
)
ADD_DEPENDENCIES(dfhack-version git-describe)

ADD_LIBRARY(dfhack SHARED ${PROJECT_SOURCES})
ADD_DEPENDENCIES(dfhack generate_headers)
ADD_DEPENDENCIES(dfhack jsoncpp)

ADD_LIBRARY(dfhack-client SHARED RemoteClient.cpp ColorText.cpp MiscUtils.cpp ${PROJECT_PROTO_SRCS})
ADD_DEPENDENCIES(dfhack-client dfhack)

ADD_EXECUTABLE(dfhack-run dfhack-run.cpp)

ADD_EXECUTABLE(binpatch binpatch.cpp)
TARGET_LINK_LIBRARIES(binpatch dfhack-md5)

IF(BUILD_EGGY)
    SET_TARGET_PROPERTIES(dfhack PROPERTIES OUTPUT_NAME "egg" )
else()
    IF(WIN32)
        SET_TARGET_PROPERTIES(dfhack PROPERTIES OUTPUT_NAME "SDL" )
    ENDIF()
endif()

IF(WIN32)
    SET_TARGET_PROPERTIES(dfhack PROPERTIES COMPILE_FLAGS "/FI\"Export.h\"" )
    SET_TARGET_PROPERTIES(dfhack-client PROPERTIES COMPILE_FLAGS "/FI\"Export.h\"" )
ELSE()
    SET_TARGET_PROPERTIES(dfhack PROPERTIES COMPILE_FLAGS "-include Export.h" )
    SET_TARGET_PROPERTIES(dfhack-client PROPERTIES COMPILE_FLAGS "-include Export.h" )
ENDIF()

#effectively disables debug builds...
SET_TARGET_PROPERTIES(dfhack  PROPERTIES DEBUG_POSTFIX "-debug" )

IF(APPLE)
    SET(DF_SDL_LIBRARY ${CMAKE_INSTALL_PREFIX}/libs/SDL.framework/Versions/A/SDL)
    IF(NOT EXISTS ${DF_SDL_LIBRARY})
        MESSAGE(SEND_ERROR "SDL framework not found. Make sure CMAKE_INSTALL_PREFIX is specified and correct.")
    ENDIF()
    SET(SDL_LIBRARY ${CMAKE_BINARY_DIR}/SDL)
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DF_SDL_LIBRARY} ${SDL_LIBRARY})
    SET(CXX_LIBRARY ${CMAKE_INSTALL_PREFIX}/libs/libstdc++.6.dylib)
    SET(ZIP_LIBRARY /usr/lib/libz.dylib)
    TARGET_LINK_LIBRARIES(dfhack ${SDL_LIBRARY})
    TARGET_LINK_LIBRARIES(dfhack ${CXX_LIBRARY})
    TARGET_LINK_LIBRARIES(dfhack ${ZIP_LIBRARY})
    SET_TARGET_PROPERTIES(dfhack PROPERTIES VERSION 1.0.0)
    SET_TARGET_PROPERTIES(dfhack PROPERTIES SOVERSION 1.0.0)
ENDIF()

TARGET_LINK_LIBRARIES(dfhack protobuf-lite clsocket lua dfhack-version jsoncpp ${PROJECT_LIBS})
SET_TARGET_PROPERTIES(dfhack PROPERTIES LINK_INTERFACE_LIBRARIES "")

TARGET_LINK_LIBRARIES(dfhack-client protobuf-lite clsocket)
TARGET_LINK_LIBRARIES(dfhack-run dfhack-client)

if(APPLE)
    add_custom_command(TARGET dfhack-run COMMAND ${dfhack_SOURCE_DIR}/package/darwin/fix-libs.sh WORKING_DIRECTORY ../ COMMENT "Fixing library dependencies...")
endif()

IF(UNIX)
    if (APPLE)
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/darwin/dfhack
                DESTINATION .)
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/darwin/dfhack-run
                DESTINATION .)
        OPTION(INSTALL_NEW_LIBSTDCXX "Install a version of libstdc++ from GCC 4.5.4 to fix various crashes" ON)
        IF(INSTALL_NEW_LIBSTDCXX)
            execute_process(COMMAND bunzip2 --keep --force ${dfhack_SOURCE_DIR}/package/darwin/libstdc++.6.dylib.bz2)
            install(PROGRAMS ${dfhack_SOURCE_DIR}/package/darwin/libstdc++.6.dylib
                DESTINATION ./hack/)
        ENDIF(INSTALL_NEW_LIBSTDCXX)
    else()
        # On linux, copy our version of the df launch script which sets LD_PRELOAD
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/linux/dfhack
            DESTINATION .)
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/linux/dfhack-run
            DESTINATION .)
    endif()
ELSE()
    if(NOT BUILD_EGGY)
        # On windows, copy the renamed SDL so DF can still run.
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/windows/SDLreal.dll
                DESTINATION ${DFHACK_LIBRARY_DESTINATION})
    endif()
ENDIF()

#install the main lib
if(NOT BUILD_EGGY)
    install(TARGETS dfhack
            LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
            RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})
else()
    install(TARGETS dfhack
            LIBRARY DESTINATION ${DFHACK_EGGY_DESTINATION}
            RUNTIME DESTINATION ${DFHACK_EGGY_DESTINATION})
endif()

#install the offset file
install(FILES xml/symbols.xml
        DESTINATION ${DFHACK_DATA_DESTINATION}) #linux: share/dfhack
#install the example autoexec file
install(FILES ../dfhack.init-example
        DESTINATION ${DFHACK_BINARY_DESTINATION})

install(TARGETS dfhack-run dfhack-client binpatch
        LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
        RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})

install(DIRECTORY lua/
        DESTINATION ${DFHACK_LUA_DESTINATION}
        FILES_MATCHING PATTERN "*.lua")

#install(DIRECTORY ${dfhack_SOURCE_DIR}/scripts
#        DESTINATION ${DFHACK_DATA_DESTINATION}
#        FILES_MATCHING PATTERN "*.lua"
#                       PATTERN "*.rb"
#                       PATTERN "3rdparty" EXCLUDE
#        )

install(DIRECTORY ${dfhack_SOURCE_DIR}/patches
        DESTINATION ${DFHACK_DATA_DESTINATION}
        FILES_MATCHING PATTERN "*.dif")

# Unused for so long that it's not even relevant now...
if(BUILD_DEVEL)
    if(WIN32)
        install(TARGETS dfhack
                ARCHIVE DESTINATION ${DFHACK_DEVLIB_DESTINATION})
    endif()
    # note the ending '/'. This means *contents* of the directory are installed
    # without the '/', the directory itself is installed
    install(DIRECTORY include/
            DESTINATION ${DFHACK_INCLUDES_DESTINATION}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.inc" ) #linux: include
    # Building the docs
    IF(BUILD_DOXYGEN)
        add_subdirectory (doc)
    ENDIF()
endif()
