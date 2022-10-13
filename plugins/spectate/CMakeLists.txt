
project(spectate)

set_source_files_properties(pause.h PROPERTIES HEADER_FILE_ONLY TRUE)

SET(SOURCES
        spectate.cpp
        pause.h
        pause.cpp)

dfhack_plugin(${PROJECT_NAME} ${SOURCES} LINK_LIBRARIES lua)
