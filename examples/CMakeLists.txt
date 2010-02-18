# don't use this file directly. use the one in the root folder of the project

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

# attachtest - 100x attach/detach, suspend/resume
ADD_EXECUTABLE(dfattachtest attachtest.cpp)
TARGET_LINK_LIBRARIES(dfattachtest dfhack)

# a benchmark program, reads the map 1000x
ADD_EXECUTABLE(dfexpbench expbench.cpp)
TARGET_LINK_LIBRARIES(dfexpbench dfhack)

# creaturedump - basic creature dump - a test of the creature related exports
ADD_EXECUTABLE(dfcreaturedump creaturedump.cpp)
TARGET_LINK_LIBRARIES(dfcreaturedump dfhack)

# buildingsdump - dump buildings and their raw data filtered by type
ADD_EXECUTABLE(dfbuildingsdump buildingsdump.cpp)
TARGET_LINK_LIBRARIES(dfbuildingsdump dfhack)

# materialtest - just list the first material of each type
ADD_EXECUTABLE(dfmaterialtest materialtest.cpp)
TARGET_LINK_LIBRARIES(dfmaterialtest dfhack)

# position - check the DF window and cursor parameters
ADD_EXECUTABLE(dfposition position.cpp)
TARGET_LINK_LIBRARIES(dfposition dfhack)

# suspendtest - test if suspend works. df should stop responding when suspended by dfhack
ADD_EXECUTABLE(dfsuspend suspendtest.cpp)
TARGET_LINK_LIBRARIES(dfsuspend dfhack)

# itemdump - dump the item under the cursor
ADD_EXECUTABLE(dfitemdump dfitemdump.cpp)
TARGET_LINK_LIBRARIES(dfitemdump dfhack)

# hotkeynotedump - dumps the hotkeys and notes for the loaded map
# Author: belal
ADD_EXECUTABLE(dfhotkeynotedump hotkeynotedump.cpp)
TARGET_LINK_LIBRARIES(dfhotkeynotedump dfhack)

IF(UNIX)
    # veinlook - look at the map... sort of
    ADD_EXECUTABLE(dfveinlook veinlook.cpp)
    TARGET_LINK_LIBRARIES(dfveinlook dfhack ncurses)
ENDIF(UNIX)

# renamer - change the custom names and professions of creatures, sends keys to df directly
ADD_EXECUTABLE(dfrenamer renamer.cpp)
TARGET_LINK_LIBRARIES(dfrenamer dfhack)

IF(UNIX)
install(TARGETS
dfattachtest
dfbuildingsdump
dfcreaturedump
dfitemdump
dfdigger
dfexpbench
dfmaterialtest
dfposition
dfrenamer
dfsuspend
dfveinlook
RUNTIME DESTINATION bin
)
ENDIF(UNIX)
