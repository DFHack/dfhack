# don't use this file directly. use the one in the root folder of the project

# only build this stuff when BUILD_DFHACK_EXAMPLES is set to ON
IF (BUILD_DFHACK_EXAMPLES)

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

# buildingsdump - dump buildings and their raw data filtered by type
ADD_EXECUTABLE(dfbuildingsdump buildingsdump.cpp)
TARGET_LINK_LIBRARIES(dfbuildingsdump dfhack)

# constructiondump - dump constructions!
ADD_EXECUTABLE(dfconstructiondump construction_dump.cpp)
TARGET_LINK_LIBRARIES(dfconstructiondump dfhack)

# creaturedump - basic creature dump - a test of the creature related exports
ADD_EXECUTABLE(dfcreaturedump creaturedump.cpp)
TARGET_LINK_LIBRARIES(dfcreaturedump dfhack)

# materialtest - just list the first material of each type
ADD_EXECUTABLE(dfmaterialtest materialtest.cpp)
TARGET_LINK_LIBRARIES(dfmaterialtest dfhack)

# position - check the DF window and cursor parameters
ADD_EXECUTABLE(dfposition position.cpp)
TARGET_LINK_LIBRARIES(dfposition dfhack)

# itemdump - dump the item under the cursor
ADD_EXECUTABLE(dfitemdump dfitemdump.cpp)
TARGET_LINK_LIBRARIES(dfitemdump dfhack)

# hotkeynotedump - dumps the hotkeys and notes for the loaded map
# Author: belal
ADD_EXECUTABLE(dfhotkeynotedump hotkeynotedump.cpp)
TARGET_LINK_LIBRARIES(dfhotkeynotedump dfhack)

# settlementdump - dumps the settlements on the loaded map
# Author: belal
# ADD_EXECUTABLE(dfsettlementdump settlementdump.cpp)
# TARGET_LINK_LIBRARIES(dfsettlementdump dfhack)

# treedump - dump them trees!
ADD_EXECUTABLE(dftreedump treedump.cpp)
TARGET_LINK_LIBRARIES(dftreedump dfhack)

# spatterdump - dump spatter 'veins'
ADD_EXECUTABLE(dfspatterdump spatterdump.cpp)
TARGET_LINK_LIBRARIES(dfspatterdump dfhack)

# processenum - demonstrates the use of ProcessEnumerator
ADD_EXECUTABLE(dfprocessenum processenum.cpp)
TARGET_LINK_LIBRARIES(dfprocessenum dfhack)

install(TARGETS
dfbuildingsdump
dfconstructiondump
dfcreaturedump
dfmaterialtest
dfposition
dfitemdump
dfhotkeynotedump
dftreedump
dfspatterdump
dfprocessenum
RUNTIME DESTINATION bin
)
ENDIF (BUILD_DFHACK_EXAMPLES)