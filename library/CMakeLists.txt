project(dfapi)
cmake_minimum_required(VERSION 3.21)

# prevent CMake warnings about INTERFACE_LINK_LIBRARIES vs LINK_INTERFACE_LIBRARIES
cmake_policy(SET CMP0022 NEW)

# build options
if(UNIX)
    option(CONSOLE_NO_CATCH "Make the console not catch 'CTRL+C' events for easier debugging." OFF)
endif()

option(DFHACK_SDL_CONSOLE "Use experimental SDL console" ON)

# Generation
set(CODEGEN_OUT ${dfapi_SOURCE_DIR}/include/df/codegen.out.xml)

file(GLOB GENERATE_INPUT_SCRIPTS ${dfapi_SOURCE_DIR}/xml/*.pm ${dfapi_SOURCE_DIR}/xml/*.xslt)
file(GLOB GENERATE_INPUT_XMLS ${dfapi_SOURCE_DIR}/xml/df.*.xml)

execute_process(COMMAND ${PERL_EXECUTABLE} xml/list.pl xml ${dfapi_SOURCE_DIR}/include/df ";"
    WORKING_DIRECTORY ${dfapi_SOURCE_DIR}
    OUTPUT_VARIABLE GENERATED_HDRS)

set_source_files_properties(${GENERATED_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE GENERATED TRUE)

add_custom_command(
    OUTPUT ${CODEGEN_OUT}
    BYPRODUCTS ${GENERATED_HDRS}
    COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/xml/codegen.pl
        ${CMAKE_CURRENT_SOURCE_DIR}/xml
        ${CMAKE_CURRENT_SOURCE_DIR}/include/df
    MAIN_DEPENDENCY ${dfapi_SOURCE_DIR}/xml/codegen.pl
    COMMENT "Generating codegen.out.xml and df/headers"
    DEPENDS ${GENERATE_INPUT_XMLS} ${GENERATE_INPUT_SCRIPTS}
)

if(NOT("${CMAKE_GENERATOR}" STREQUAL Ninja))
    # use BYPRODUCTS instead under Ninja to avoid rebuilds
    list(APPEND CODEGEN_OUT ${GENERATED_HDRS})
endif()

add_custom_target(generate_headers DEPENDS ${CODEGEN_OUT})

add_subdirectory(xml)

if(BUILD_LIBRARY)

set(MAIN_HEADERS
    include/Internal.h
    include/DFHackVersion.h
    include/BitArray.h
    include/ColorText.h
    include/Console.h
    include/Core.h
    include/CoreDefs.h
    include/DataDefs.h
    include/DataFuncs.h
    include/DataIdentity.h
    include/Debug.h
    include/DebugManager.h
    include/Error.h
    include/Export.h
    include/Hooks.h
    include/LuaTools.h
    include/LuaWrapper.h
    include/MemAccess.h
    include/Memory.h
    include/MiscUtils.h
    include/Module.h
    include/MemAccess.h
    include/ModuleFactory.h
    include/PluginLua.h
    include/PluginManager.h
    include/PluginStatics.h
    include/RemoteClient.h
    include/RemoteServer.h
    include/RemoteTools.h
    include/Signal.hpp
    include/TileTypes.h
    include/Types.h
    include/VersionInfo.h
    include/VersionInfoFactory.h
    include/VTableInterpose.h
)

set(MAIN_HEADERS_WINDOWS
    include/wdirent.h
)

set(MAIN_SOURCES
    Core.cpp
    ColorText.cpp
    CompilerWorkAround.cpp
    DataDefs.cpp
    DataIdentity.cpp
    Debug.cpp
    Error.cpp
    VTableInterpose.cpp
    LuaWrapper.cpp
    LuaTypes.cpp
    LuaTools.cpp
    LuaApi.cpp
    DataStatics.cpp
    DataStaticsCtor.cpp
    MiscUtils.cpp
    Types.cpp
    PluginManager.cpp
    PluginStatics.cpp
    PlugLoad.cpp
    TileTypes.cpp
    VersionInfoFactory.cpp
    RemoteClient.cpp
    RemoteServer.cpp
    RemoteTools.cpp
)

file(GLOB_RECURSE TEST_SOURCES
    LIST_DIRECTORIES false
    *test.cpp)

if (DFHACK_SDL_CONSOLE)
    list(APPEND TEST_SOURCES SDLConsole_impl.cpp)
endif()
dfhack_test(dfhack-test "${TEST_SOURCES}")

if(WIN32)
        set(CONSOLE_SOURCES Console-windows.cpp)
else()
        set(CONSOLE_SOURCES Console-posix.cpp)
endif()
list(APPEND CONSOLE_SOURCES Console.cpp)
if (DFHACK_SDL_CONSOLE)
    list(APPEND CONSOLE_SOURCES Console-sdl.cpp SDLConsole_impl.cpp)
endif()

set(DFCLIENT_CONSOLE_SOURCES Console.cpp)

set(MAIN_SOURCES_WINDOWS
    ${CONSOLE_SOURCES}
    Hooks.cpp
    Process-windows.cpp
)

if(WIN32)
    source_group("Main\\Headers" FILES ${MAIN_HEADERS} ${MAIN_HEADERS_WINDOWS})
    source_group("Main\\Sources" FILES ${MAIN_SOURCES} ${MAIN_SOURCES_WINDOWS})
endif()

set(MAIN_SOURCES_LINUX
    ${CONSOLE_SOURCES}
    Process-linux.cpp
)

set(MAIN_SOURCES_DARWIN
    ${CONSOLE_SOURCES}
    PlugLoad-posix.cpp
    Process-darwin.cpp
)

set(MODULE_HEADERS
    include/modules/Buildings.h
    include/modules/Burrows.h
    include/modules/Constructions.h
    include/modules/DFSDL.h
    include/modules/DFSteam.h
    include/modules/Designations.h
    include/modules/EventManager.h
    include/modules/Filesystem.h
    include/modules/Graphic.h
    include/modules/Gui.h
    include/modules/GuiHooks.h
    include/modules/Items.h
    include/modules/Job.h
    include/modules/Kitchen.h
    include/modules/MapCache.h
    include/modules/Maps.h
    include/modules/Materials.h
    include/modules/Military.h
    include/modules/Once.h
    include/modules/Persistence.h
    include/modules/Random.h
    include/modules/References.h
    include/modules/Renderer.h
    include/modules/Screen.h
    include/modules/Textures.h
    include/modules/Translation.h
    include/modules/Units.h
    include/modules/World.h
)

set(MODULE_SOURCES
    modules/Buildings.cpp
    modules/Burrows.cpp
    modules/Constructions.cpp
    modules/DFSDL.cpp
    modules/DFSteam.cpp
    modules/Designations.cpp
    modules/EventManager.cpp
    modules/Filesystem.cpp
    modules/Graphic.cpp
    modules/Gui.cpp
    modules/Items.cpp
    modules/Job.cpp
    modules/Kitchen.cpp
    modules/MapCache.cpp
    modules/Maps.cpp
    modules/Materials.cpp
    modules/Military.cpp
    modules/Once.cpp
    modules/Persistence.cpp
    modules/Random.cpp
    modules/References.cpp
    modules/Renderer.cpp
    modules/Screen.cpp
    modules/Textures.cpp
    modules/Translation.cpp
    modules/Units.cpp
    modules/World.cpp
)

set(STATIC_FIELDS_FILES)
foreach(GROUP other a b c d e f g h i j k l m n o p q r s t u v w x y z)
    set(STATIC_FIELDS_FILENAME ${dfhack_SOURCE_DIR}/library/DataStaticsFields/${GROUP}.cpp)
    if(${GROUP} STREQUAL "other")
        set(STATIC_FIELDS_INC_FILENAME "df/static.fields.inc")
    else()
        set(STATIC_FIELDS_INC_FILENAME "df/static.fields-${GROUP}.inc")
    endif()
    file(WRITE ${STATIC_FIELDS_FILENAME}.tmp
        "#include \"DataStaticsFields.inc\"\n"
        "#include \"${STATIC_FIELDS_INC_FILENAME}\"\n"
    )
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${STATIC_FIELDS_FILENAME}.tmp ${STATIC_FIELDS_FILENAME})
    file(REMOVE ${STATIC_FIELDS_FILENAME}.tmp)
    list(APPEND STATIC_FIELDS_FILES ${STATIC_FIELDS_FILENAME})
endforeach()
list(APPEND MAIN_SOURCES ${STATIC_FIELDS_FILES})

if(WIN32)
    source_group("Modules\\Headers" FILES ${MODULE_HEADERS})
    source_group("Modules\\Sources" FILES ${MODULE_SOURCES})
    source_group("Generated" FILES ${GENERATED_HDRS})
endif()

set(PROJECT_HEADERS)
list(APPEND PROJECT_HEADERS ${MAIN_HEADERS})
list(APPEND PROJECT_HEADERS ${MODULE_HEADERS})
set(PROJECT_SOURCES)
list(APPEND PROJECT_SOURCES ${MAIN_SOURCES})
list(APPEND PROJECT_SOURCES ${MODULE_SOURCES})

if(UNIX)
    if(APPLE)
        list(APPEND PROJECT_SOURCES ${MAIN_SOURCES_DARWIN})
    else()
        list(APPEND PROJECT_SOURCES ${MAIN_SOURCES_LINUX})
    endif()
elseif(WIN32)
    list(APPEND PROJECT_SOURCES ${MAIN_SOURCES_WINDOWS})
    list(APPEND PROJECT_HEADERS ${MAIN_HEADERS_WINDOWS})
endif()

# Protobuf
file(GLOB PROJECT_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

string(REPLACE ".proto" ".pb.cc" PROJECT_PROTO_SRCS "${PROJECT_PROTOS}")
string(REPLACE ".proto" ".pb.h" PROJECT_PROTO_HDRS "${PROJECT_PROTOS}")
string(REPLACE "/proto/" "/proto/tmp/" PROJECT_PROTO_TMP_FILES "${PROJECT_PROTO_SRCS};${PROJECT_PROTO_HDRS}")
set_source_files_properties(${PROJECT_PROTO_SRCS} ${PROJECT_PROTO_HDRS}
    PROPERTIES GENERATED TRUE)

# Force a re-gen if any *.pb.* files are missing
# (only runs when cmake is run, but better than nothing)
foreach(file IN LISTS PROJECT_PROTO_SRCS PROJECT_PROTO_HDRS)
    if(NOT EXISTS ${file})
        # message("Resetting generate_proto_core because '${file}' is missing")
        file(REMOVE ${PROJECT_PROTO_TMP_FILES})
        break()
    endif()
endforeach()

list(APPEND PROJECT_HEADERS ${PROJECT_PROTO_HDRS})
list(APPEND PROJECT_SOURCES ${PROJECT_PROTO_SRCS})

add_custom_command(
    OUTPUT ${PROJECT_PROTO_TMP_FILES}
    COMMAND protoc-bin -I=${CMAKE_CURRENT_SOURCE_DIR}/proto/
        --cpp_out=dllexport_decl=DFHACK_EXPORT:${CMAKE_CURRENT_SOURCE_DIR}/proto/tmp/
        ${PROJECT_PROTOS}
    COMMAND ${PERL_EXECUTABLE} ${dfhack_SOURCE_DIR}/depends/copy-if-different.pl
        ${PROJECT_PROTO_TMP_FILES}
        ${CMAKE_CURRENT_SOURCE_DIR}/proto/
    COMMENT "Generating core protobufs"
    DEPENDS protoc-bin ${PROJECT_PROTOS}
)

if(UNIX)
    set_source_files_properties(${PROJECT_PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-misleading-indentation")
endif()

add_custom_target(generate_proto_core DEPENDS ${PROJECT_PROTO_TMP_FILES})

# Merge headers into sources
set_source_files_properties( ${PROJECT_HEADERS} PROPERTIES HEADER_FILE_ONLY TRUE )
list(APPEND PROJECT_SOURCES ${PROJECT_HEADERS})
list(APPEND PROJECT_SOURCES ${GENERATED_HDRS})

if(REMOVE_SYMBOLS_FROM_DF_STUBS)
    if(UNIX)
        # Don't produce debug info for generated stubs
        set_source_files_properties(DataStatics.cpp DataStaticsCtor.cpp ${STATIC_FIELDS_FILES}
            PROPERTIES COMPILE_FLAGS "-g0 -O1")
    else(WIN32)
        set_source_files_properties(DataStatics.cpp DataStaticsCtor.cpp ${STATIC_FIELDS_FILES}
            PROPERTIES COMPILE_FLAGS "/O1 /bigobj")
    endif()
else()
    if(WIN32)
        set_source_files_properties(DataStatics.cpp DataStaticsCtor.cpp ${STATIC_FIELDS_FILES}
            PROPERTIES COMPILE_FLAGS "/Od /bigobj")
    endif()
endif()

# Compilation

add_definitions(-DBUILD_DFHACK_LIB)

if(UNIX)
    if(CONSOLE_NO_CATCH)
        add_definitions(-DCONSOLE_NO_CATCH)
    endif()
endif()

if(APPLE)
    set(PROJECT_LIBS dl dfhack-md5 ${DFHACK_TINYXML})
elseif(UNIX)
    set(PROJECT_LIBS rt dl dfhack-md5 ${DFHACK_TINYXML})
else(WIN32)
    # FIXME: do we really need psapi?
    set(PROJECT_LIBS psapi dbghelp dfhack-md5 ${DFHACK_TINYXML})
endif()

set(VERSION_SRCS DFHackVersion.cpp)
set(VERSION_HDRS include/git-describe.h)

set_source_files_properties(${VERSION_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND VERSION_SRCS ${VERSION_HDRS})

add_library(dfhack-version STATIC ${VERSION_SRCS})
set_property(TARGET dfhack-version APPEND PROPERTY COMPILE_DEFINITIONS
    DFHACK_VERSION="${DFHACK_VERSION}"
    DF_VERSION="${DF_VERSION}"
    DFHACK_RELEASE="${DFHACK_RELEASE}"
    DFHACK_ABI_VERSION=${DFHACK_ABI_VERSION}
    DFHACK_RUN_URL="${DFHACK_RUN_URL}"
)
target_include_directories(dfhack-version PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(DFHACK_PRERELEASE)
    set_property(TARGET dfhack-version APPEND PROPERTY COMPILE_DEFINITIONS
        DFHACK_PRERELEASE=1
    )
endif()

configure_file(git-describe.cmake.in ${CMAKE_CURRENT_SOURCE_DIR}/git-describe.cmake @ONLY)
if(EXISTS ${dfhack_SOURCE_DIR}/.git/index AND EXISTS ${dfhack_SOURCE_DIR}/.git/modules/library/xml/index)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/git-describe.h
    COMMAND ${CMAKE_COMMAND}
        -D dfhack_SOURCE_DIR:STRING=${dfhack_SOURCE_DIR}
        -D GIT_EXECUTABLE:STRING=${GIT_EXECUTABLE}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/git-describe.cmake
    COMMENT "Obtaining git commit information"
    DEPENDS ${dfhack_SOURCE_DIR}/.git/index
        ${dfhack_SOURCE_DIR}/.git/modules/library/xml/index
        ${CMAKE_CURRENT_SOURCE_DIR}/git-describe.cmake
        include/git-describe.h.in
    )
endif()

add_library(dfhack SHARED ${PROJECT_SOURCES})
target_include_directories(dfhack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/proto)

get_target_property(xlsxio_INCLUDES xlsxio_read_STATIC INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(dfhack PRIVATE ${xlsxio_INCLUDES} ${SDL2_INCLUDE_DIRS})
add_dependencies(dfhack generate_proto_core)
add_dependencies(dfhack generate_headers)

add_library(dfhack-client SHARED RemoteClient.cpp ColorText.cpp MiscUtils.cpp Error.cpp ${PROJECT_PROTO_SRCS} ${DFCLIENT_CONSOLE_SOURCES})
target_include_directories(dfhack-client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/proto)
target_compile_definitions(dfhack-client PUBLIC BUILD_DFHACK_CLIENT)

add_dependencies(dfhack-client dfhack)

add_executable(dfhack-run dfhack-run.cpp)
target_include_directories(dfhack-run PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/proto)

add_executable(binpatch binpatch.cpp)
target_link_libraries(binpatch dfhack-md5)

target_include_directories(dfhack-test PRIVATE ${SDL2_INCLUDE_DIRS})

if(WIN32)
    set_target_properties(dfhack PROPERTIES OUTPUT_NAME "dfhooks_dfhack" )
    set_target_properties(dfhack PROPERTIES COMPILE_FLAGS "/FI\"Export.h\"" )
    set_target_properties(dfhack-client PROPERTIES COMPILE_FLAGS "/FI\"Export.h\"" )
else()
    set_target_properties(dfhack PROPERTIES COMPILE_FLAGS "-include Export.h" )
    set_target_properties(dfhack-client PROPERTIES COMPILE_FLAGS "-include Export.h" )
    add_library(dfhooks_dfhack SHARED Hooks.cpp)
    target_link_libraries(dfhooks_dfhack dfhack)
endif()

# effectively disables debug builds...
set_target_properties(dfhack PROPERTIES DEBUG_POSTFIX "-debug" )

if(APPLE)
    set(DF_SDL_LIBRARY ${CMAKE_INSTALL_PREFIX}/libs/SDL.framework/Versions/A/SDL)
    if(NOT EXISTS ${DF_SDL_LIBRARY})
        message(SEND_ERROR "SDL framework not found. Make sure CMAKE_INSTALL_PREFIX is specified and correct.")
    endif()
    set(SDL_LIBRARY ${CMAKE_BINARY_DIR}/SDL)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DF_SDL_LIBRARY} ${SDL_LIBRARY})
    set(CXX_LIBRARY ${CMAKE_INSTALL_PREFIX}/libs/libstdc++.6.dylib)
    set(ZIP_LIBRARY /usr/lib/libz.dylib)
    target_link_libraries(dfhack ${SDL_LIBRARY})
    target_link_libraries(dfhack ${CXX_LIBRARY})
    if(EXISTS ${ZIP_LIBRARY})
        # doesn't exist on macOS 11, but DFHack seems to find the right library there
        target_link_libraries(dfhack ${ZIP_LIBRARY})
    endif()
    target_link_libraries(dfhack ncurses)
    set_target_properties(dfhack PROPERTIES VERSION 1.0.0)
    set_target_properties(dfhack PROPERTIES SOVERSION 1.0.0)
endif()

target_link_libraries(dfhack protobuf-lite clsocket lua jsoncpp_static dfhack-version ${PROJECT_LIBS})
set_target_properties(dfhack PROPERTIES INTERFACE_LINK_LIBRARIES "")

target_link_libraries(dfhack-client protobuf-lite clsocket jsoncpp_static)
if(WIN32)
    target_link_libraries(dfhack-client dbghelp)
endif()
target_link_libraries(dfhack-run dfhack-client)

if(APPLE)
    add_custom_command(TARGET dfhack-run COMMAND ${dfhack_SOURCE_DIR}/package/darwin/fix-libs.sh WORKING_DIRECTORY ../ COMMENT "Fixing library dependencies...")
endif()

if(UNIX)
    if(APPLE)
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/darwin/dfhack
            DESTINATION .)
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/darwin/dfhack-run
            DESTINATION .)
    else()
        # On linux, copy our version of the df launch script which sets LD_PRELOAD
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/linux/dfhack
            DESTINATION .)
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/linux/dfhack-run
            DESTINATION .)
    endif()
    install(TARGETS dfhooks_dfhack
        LIBRARY DESTINATION .
        RUNTIME DESTINATION .)
endif()

# install the main lib
install(TARGETS dfhack
    LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
    RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})

install(TARGETS dfhack-run dfhack-client binpatch
    LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
    RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})

endif(BUILD_LIBRARY)

# install the offset file
if(INSTALL_DATA_FILES)
    install(FILES xml/symbols.xml
        DESTINATION ${DFHACK_DATA_DESTINATION})

    install(DIRECTORY lua/
        DESTINATION ${DFHACK_LUA_DESTINATION}
        FILES_MATCHING PATTERN "*.lua")

    install(DIRECTORY ${dfhack_SOURCE_DIR}/patches
        DESTINATION ${DFHACK_DATA_DESTINATION}
        FILES_MATCHING PATTERN "*.dif")
endif()
