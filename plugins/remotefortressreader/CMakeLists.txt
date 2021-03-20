project(remotefortressreader)
# A list of source files
set(PROJECT_SRCS
    remotefortressreader.cpp
    adventure_control.cpp
    building_reader.cpp
    dwarf_control.cpp
    item_reader.cpp
)
# A list of headers
set(PROJECT_HDRS
    adventure_control.h
    building_reader.h
    dwarf_control.h
    item_reader.h
    df_version_int.h
)
# proto files to include.
set(PROJECT_PROTO
    RemoteFortressReader
    AdventureControl
    ItemdefInstrument
    DwarfControl
    ui_sidebar_mode
)

set(PLUGIN_PROTOS)
foreach(pbuf ${PROJECT_PROTO})
    list(APPEND PLUGIN_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/../proto/${pbuf}.proto)
endforeach()

string(REPLACE ".proto" ".pb.cc" PLUGIN_PROTO_SRCS "${PLUGIN_PROTOS}")
string(REPLACE ".proto" ".pb.h" PLUGIN_PROTO_HDRS "${PLUGIN_PROTOS}")
set_source_files_properties(${PLUGIN_PROTO_SRCS} ${PLUGIN_PROTO_HDRS} PROPERTIES GENERATED TRUE)

set_source_files_properties( ${PROJECT_HDRS} ${PLUGIN_PROTO_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
list(APPEND PROJECT_SRCS ${PROJECT_HDRS} ${PLUGIN_PROTOS} ${PLUGIN_PROTO_SRCS} ${PLUGIN_PROTO_HDRS})

if(UNIX AND NOT APPLE)
    set(PROJECT_LIBS ${PROJECT_LIBS} SDL)
endif()

# this makes sure all the stuff is put in proper places and linked to dfhack
dfhack_plugin(RemoteFortressReader ${PROJECT_SRCS} LINK_LIBRARIES protobuf-lite ${PROJECT_LIBS} COMPILE_FLAGS_MSVC "/FI\"Export.h\"" COMPILE_FLAGS_GCC "-include Export.h -Wno-misleading-indentation" )
