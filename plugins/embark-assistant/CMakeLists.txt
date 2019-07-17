project(embark-assistant)
# A list of source files
set(PROJECT_SRCS
    biome_type.cpp
    embark-assistant.cpp
    finder_ui.cpp
    help_ui.cpp
    matcher.cpp
    overlay.cpp
    screen.cpp
    survey.cpp
)
# A list of headers
set(PROJECT_HDRS
    biome_type.h
    defs.h
    embark-assistant.h
    finder_ui.h
    help_ui.h
    matcher.h
    overlay.h
    screen.h
    survey.h
)
set_source_files_properties(${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
list(APPEND PROJECT_SRCS ${PROJECT_HDRS})

dfhack_plugin(embark-assistant ${PROJECT_SRCS})
