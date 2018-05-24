PROJECT (labormanager)
# A list of source files
SET(PROJECT_SRCS
    labormanager.cpp
    joblabormapper.cpp
)
# A list of headers
SET(PROJECT_HDRS
    labormanager.h
    joblabormapper.h
)
SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

DFHACK_PLUGIN(labormanager ${PROJECT_SRCS})
