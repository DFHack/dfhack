PROJECT(mapexport)
SET(MAPEXPORT_SOURCE_DIR ${dfhack_SOURCE_DIR}/plugins/mapexport)

include_directories (
    ${CMAKE_CURRENT_BINARY_DIR}
    ${dfhack_SOURCE_DIR}/library/depends/protobuf/
)

#Generated protobuf files and the headers they will require
SET(PROJECT_HDRS
${dfhack_SOURCE_DIR}/library/depends/protobuf/google/protobuf/stubs/once.h
${dfhack_SOURCE_DIR}/library/depends/protobuf/google/protobuf/stubs/common.h
${dfhack_SOURCE_DIR}/library/depends/protobuf/google/protobuf/io/coded_stream.h
${dfhack_SOURCE_DIR}/library/depends/protobuf/google/protobuf/wire_format_lite_inl.h
${dfhack_SOURCE_DIR}/library/depends/protobuf/google/protobuf/generated_message_util.h
${dfhack_SOURCE_DIR}/library/depends/protobuf/google/protobuf/repeated_field.h
${dfhack_SOURCE_DIR}/library/depends/protobuf/google/protobuf/extension_set.h
proto/Tile.pb.h
)

SET(PROJECT_SRCS
mapexport.cpp
proto/Tile.pb.cc
)

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

ADD_CUSTOM_COMMAND(
OUTPUT proto/Tile.pb.cc proto/Tile.pb.h
COMMAND protoc -I=${CMAKE_CURRENT_SOURCE_DIR}/proto --cpp_out=proto ${CMAKE_CURRENT_SOURCE_DIR}/proto/Tile.proto
DEPENDS protoc ${CMAKE_CURRENT_SOURCE_DIR}/proto/Tile.proto
)

DFHACK_PLUGIN(mapexport ${PROJECT_SRCS} ${PROJECT_HDRS} LINK_LIBRARIES libprotobuf-lite)
