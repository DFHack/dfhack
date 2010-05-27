# don't use this file directly. use the one in the root folder of the project

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

# a reveal clone
ADD_EXECUTABLE(dfreveal reveal.cpp)
TARGET_LINK_LIBRARIES(dfreveal dfhack)

# prospector - produces a list of available materials and their quantities
ADD_EXECUTABLE(dfprospector prospector.cpp)
TARGET_LINK_LIBRARIES(dfprospector dfhack)

# vdig - dig the vein under the cursor
ADD_EXECUTABLE(dfvdig vdig.cpp)
TARGET_LINK_LIBRARIES(dfvdig dfhack)

# cleanmap - removes mud, snow, blood and similar stuff from a map. farmers beware
ADD_EXECUTABLE(dfcleanmap cleanmap.cpp)
TARGET_LINK_LIBRARIES(dfcleanmap dfhack)

# unstuck - make DF run if something goes wrong with the 'normal' memory access method
ADD_EXECUTABLE(dfunstuck unstuck.cpp)
TARGET_LINK_LIBRARIES(dfunstuck dfhack)

# probe - map tile probe
ADD_EXECUTABLE(dfprobe probe.cpp)
TARGET_LINK_LIBRARIES(dfprobe dfhack)

# attachtest - 100x attach/detach, suspend/resume
ADD_EXECUTABLE(dfattachtest attachtest.cpp)
TARGET_LINK_LIBRARIES(dfattachtest dfhack)

# a benchmark program, reads the map 1000x
ADD_EXECUTABLE(dfexpbench expbench.cpp)
TARGET_LINK_LIBRARIES(dfexpbench dfhack)

# suspendtest - test if suspend works. df should stop responding when suspended
#               by dfhack
ADD_EXECUTABLE(dfsuspend suspendtest.cpp)
TARGET_LINK_LIBRARIES(dfsuspend dfhack)

# flows - check flows impact on fps
ADD_EXECUTABLE(dfflows flows.cpp)
TARGET_LINK_LIBRARIES(dfflows dfhack)

# liquids manipulation tool
# Original author: Aleric
ADD_EXECUTABLE(dfliquids liquids.cpp)
TARGET_LINK_LIBRARIES(dfliquids dfhack)

IF(UNIX)
install(TARGETS
dfreveal
dfprospector
dfvdig
dfcleanmap
dfunstuck
dfprobe
dfattachtest
dfexpbench
dfsuspend
dfflows
dfliquids
RUNTIME DESTINATION bin
)
ENDIF(UNIX)
