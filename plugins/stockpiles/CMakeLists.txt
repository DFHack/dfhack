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
    stockpiles.proto
)

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

DFHACK_PLUGIN(stockpiles ${PROJECT_SRCS} ${PROJECT_HDRS} PROTOBUFS ${PROJECT_PROTOS} LINK_LIBRARIES protobuf-lite lua)
