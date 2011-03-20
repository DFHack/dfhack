if(NOT DEFINED DFHACK_CONSISTENCY)
    MESSAGE(FATAL_ERROR "Please build the whole thing, not parts. You can turn parts on/off using options.")
ENDIF()

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
    add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

IF(MSVC)
    ADD_CUSTOM_TARGET( memxml-for-examples
    COMMAND ${CMAKE_COMMAND} -E copy ${dfhack_SOURCE_DIR}/Memory.xml ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying Memory.xml to ${CMAKE_CURRENT_BINARY_DIR}"
    )
    set(LOCAL_DEPNAME memxml-for-examples)
ENDIF()

# buildingsdump - dump buildings and their raw data filtered by type
DFHACK_TOOL(dfbuildingsdump buildingsdump.cpp)

# constructiondump - dump constructions!
DFHACK_TOOL(dfconstructiondump construction_dump.cpp)

# creaturedump - basic creature dump - a test of the creature related exports
DFHACK_TOOL(dfcreaturedump creaturedump.cpp)

# materialtest - just list the first material of each type
DFHACK_TOOL(dfmaterialtest materialtest.cpp)

# itemdump - dump the item under the cursor
DFHACK_TOOL(dfitemdump dfitemdump.cpp)

# hotkeynotedump - dumps the hotkeys and notes for the loaded map
# Author: belal
DFHACK_TOOL(dfhotkeynotedump hotkeynotedump.cpp)

# settlementdump - dumps the settlements on the loaded map
# Author: belal
# DFHACK_TOOL(dfsettlementdump settlementdump.cpp)

# treedump - dump them trees!
DFHACK_TOOL(dftreedump treedump.cpp)

# spatterdump - dump spatter 'veins'
DFHACK_TOOL(dfspatterdump spatterdump.cpp)

# processenum - demonstrates the use of ProcessEnumerator
DFHACK_TOOL(dfprocessenum processenum.cpp)
