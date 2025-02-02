
project(spectate)

SET(SOURCES
        spectate.cpp)

dfhack_plugin(${PROJECT_NAME} ${SOURCES})
