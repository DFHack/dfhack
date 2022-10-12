
project(spectate)

SET(SOURCES
        spectate.cpp
        pause.cpp)

dfhack_plugin(${PROJECT_NAME} ${SOURCES} LINK_LIBRARIES lua)
