# don't use this file directly. use the one in the root folder of the project

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

# a benchmark program, reads the map 1000x
ADD_EXECUTABLE(expbench expbench.cpp)
TARGET_LINK_LIBRARIES(expbench dfhack)

# a reveal clone
ADD_EXECUTABLE(reveal reveal.cpp)
TARGET_LINK_LIBRARIES(reveal dfhack)

# prospector - produces a list of available materials and their quantities
ADD_EXECUTABLE(prospector prospector.cpp)
TARGET_LINK_LIBRARIES(prospector dfhack)

# cleanmap - removes mud, snow, blood and similar stuff from a map. farmers beware
ADD_EXECUTABLE(cleanmap cleanmap.cpp)
TARGET_LINK_LIBRARIES(cleanmap dfhack)

# creaturedump - basic creature dump - a test of the creature related exports
ADD_EXECUTABLE(creaturedump creaturedump.cpp)
TARGET_LINK_LIBRARIES(creaturedump dfhack)

# attachtest - 100x attach/detach, 100x reads, 100x writes
ADD_EXECUTABLE(attachtest attachtest.cpp)
TARGET_LINK_LIBRARIES(attachtest dfhack)