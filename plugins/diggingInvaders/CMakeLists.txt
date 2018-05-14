PROJECT (diggingInvaders)
# A list of source files
SET(PROJECT_SRCS
    diggingInvaders.cpp
    edgeCost.cpp
    assignJob.cpp
)
# A list of headers
SET(PROJECT_HDRS
    edgeCost.h
    assignJob.h
)
SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

DFHACK_PLUGIN(diggingInvaders ${PROJECT_SRCS})
