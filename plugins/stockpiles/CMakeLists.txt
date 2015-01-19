PROJECT(stockpiles)

# add *our* headers here.
SET(PROJECT_HDRS
    StockpileUtils.h
    OrganicMatLookup.h
    StockpileSerializer.h
)

SET(PROJECT_SRCS
    OrganicMatLookup.cpp
    StockpileSerializer.cpp
    stockpiles.cpp
)

SET(PROJECT_PROTOS
${CMAKE_CURRENT_SOURCE_DIR}/proto/stockpiles.proto
)

#Create new lists of what sources and headers protoc will output after we invoke it
STRING(REPLACE ".proto" ".pb.cc;" PROJECT_PROTO_SRCS ${PROJECT_PROTOS})
STRING(REPLACE ".proto" ".pb.h;" PROJECT_PROTO_HDRS ${PROJECT_PROTOS})

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_PROTO_HDRS} PROPERTIES GENERATED TRUE)
SET_SOURCE_FILES_PROPERTIES( ${PROJECT_PROTO_SRCS} PROPERTIES GENERATED TRUE)

LIST(APPEND PROJECT_HDRS ${PROJECT_PROTO_HDRS})
LIST(APPEND PROJECT_SRCS ${PROJECT_PROTO_SRCS})

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

#Generate sources from our proto files and store them in the source tree
ADD_CUSTOM_COMMAND(
OUTPUT ${PROJECT_PROTO_SRCS} ${PROJECT_PROTO_HDRS}
COMMAND protoc-bin -I=${CMAKE_CURRENT_SOURCE_DIR}/proto/ --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto/ ${PROJECT_PROTOS}
DEPENDS protoc-bin ${PROJECT_PROTOS}
)

IF(WIN32)
    DFHACK_PLUGIN(stockpiles ${PROJECT_SRCS} ${PROJECT_HDRS} LINK_LIBRARIES protobuf-lite lua)
ELSE()
    DFHACK_PLUGIN(stockpiles ${PROJECT_SRCS} ${PROJECT_HDRS} LINK_LIBRARIES protobuf-lite lua)
ENDIF()
