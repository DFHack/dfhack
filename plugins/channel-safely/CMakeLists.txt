project(channel-safely)

SET(SOURCES
        channel-jobs.cpp
        channel-groups.cpp
        channel-manager.cpp
        channel-safely-plugin.cpp)

dfhack_plugin(${PROJECT_NAME} ${SOURCES} LINK_LIBRARIES lua)
