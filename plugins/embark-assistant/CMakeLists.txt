PROJECT (embark-assistant)
# A list of source files
SET(PROJECT_SRCS
    embark-assistant.cpp
    finder_ui.cpp
    help_ui.cpp
    matcher.cpp
    overlay.cpp
    screen.cpp
    survey.cpp
)
# A list of headers
SET(PROJECT_HDRS
    defs.h
    embark-assistant.h
    finder_ui.h
    help_ui.h
    matcher.h
    overlay.h
    screen.h
    survey.h
)
SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

DFHACK_PLUGIN(embark-assistant ${PROJECT_SRCS})
