project(channel-safely)
# might just be clion, but
include_directories(${dfhack_SOURCE_DIR}/library/include)
#MESSAGE(${dfhack_SOURCE_DIR}/library/include)
#MESSAGE(/data/develop/projects/dwarf-fortress/dfhack/library/include)
SET(HEADERS channel-jobs.h channel-groups.h channel-manager.h inlines.h)
SET(SOURCES channel-jobs.cpp channel-groups.cpp channel-manager.cpp channel-safely-plugin.cpp)
dfhack_plugin(channel-safely ${HEADERS} ${SOURCES} LINK_LIBRARIES lua)
