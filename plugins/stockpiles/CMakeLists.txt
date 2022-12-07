project(stockpiles)

# add *our* headers here.
set(PROJECT_HDRS
    StockpileUtils.h
    OrganicMatLookup.h
    StockpileSerializer.h
)

set(PROJECT_SRCS
    OrganicMatLookup.cpp
    StockpileSerializer.cpp
    stockpiles.cpp
)

set(PROJECT_PROTOS
    stockpiles
)

set_source_files_properties(${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)
list(APPEND PROJECT_SRCS ${PROJECT_HDRS})

dfhack_plugin(stockpiles ${PROJECT_SRCS} ${PROJECT_HDRS} PROTOBUFS ${PROJECT_PROTOS} LINK_LIBRARIES protobuf-lite lua)
