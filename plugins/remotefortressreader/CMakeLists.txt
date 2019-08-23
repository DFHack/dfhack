PROJECT (remotefortressreader)
# A list of source files
SET(PROJECT_SRCS
    remotefortressreader.cpp
    adventure_control.cpp
    building_reader.cpp
    dwarf_control.cpp
    item_reader.cpp
)
# A list of headers
SET(PROJECT_HDRS
    adventure_control.h
    building_reader.h
    dwarf_control.h
    item_reader.h
    df_version_int.h
)
#proto files to include.
SET(PROJECT_PROTO
    RemoteFortressReader
    AdventureControl
    ItemdefInstrument
    DwarfControl
    ui_sidebar_mode
)

SET(PLUGIN_PROTOS)
FOREACH(pbuf ${PROJECT_PROTO})
    LIST(APPEND PLUGIN_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/../proto/${pbuf}.proto)
ENDFOREACH()

STRING(REPLACE ".proto" ".pb.cc" PLUGIN_PROTO_SRCS "${PLUGIN_PROTOS}")
STRING(REPLACE ".proto" ".pb.h" PLUGIN_PROTO_HDRS "${PLUGIN_PROTOS}")
SET_SOURCE_FILES_PROPERTIES(${PLUGIN_PROTO_SRCS} ${PLUGIN_PROTO_HDRS} PROPERTIES GENERATED TRUE)

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} ${PLUGIN_PROTO_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS} ${PLUGIN_PROTOS} ${PLUGIN_PROTO_SRCS} ${PLUGIN_PROTO_HDRS})

IF(UNIX AND NOT APPLE)
    SET(PROJECT_LIBS ${PROJECT_LIBS} SDL)
ENDIF()

# this makes sure all the stuff is put in proper places and linked to dfhack
DFHACK_PLUGIN(RemoteFortressReader ${PROJECT_SRCS} LINK_LIBRARIES protobuf-lite ${PROJECT_LIBS} COMPILE_FLAGS_MSVC "/FI\"Export.h\"" COMPILE_FLAGS_GCC "-include Export.h -Wno-misleading-indentation" )
