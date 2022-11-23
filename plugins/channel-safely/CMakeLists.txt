project(channel-safely)

include_directories(include)
SET(SOURCES
        channel-groups.cpp
        channel-manager.cpp
        channel-safely-plugin.cpp)

dfhack_plugin(${PROJECT_NAME} ${SOURCES} LINK_LIBRARIES lua)
