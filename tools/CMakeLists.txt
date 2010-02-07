# don't use this file directly. use the one in the root folder of the project

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

# a benchmark program, reads the map 1000x
ADD_EXECUTABLE(dfexpbench expbench.cpp)
TARGET_LINK_LIBRARIES(dfexpbench dfhack)

# a reveal clone
ADD_EXECUTABLE(dfreveal reveal.cpp)
TARGET_LINK_LIBRARIES(dfreveal dfhack)

# prospector - produces a list of available materials and their quantities
ADD_EXECUTABLE(dfprospector prospector.cpp)
TARGET_LINK_LIBRARIES(dfprospector dfhack)

# cleanmap - removes mud, snow, blood and similar stuff from a map. farmers beware
ADD_EXECUTABLE(dfcleanmap cleanmap.cpp)
TARGET_LINK_LIBRARIES(dfcleanmap dfhack)

# creaturedump - basic creature dump - a test of the creature related exports
ADD_EXECUTABLE(dfcreaturedump creaturedump.cpp)
TARGET_LINK_LIBRARIES(dfcreaturedump dfhack)

# buildingsdump - dump buildings and their raw data filtered by type
ADD_EXECUTABLE(dfbuildingsdump buildingsdump.cpp)
TARGET_LINK_LIBRARIES(dfbuildingsdump dfhack)


# attachtest - 100x attach/detach, 100x reads, 100x writes
ADD_EXECUTABLE(dfattachtest attachtest.cpp)
TARGET_LINK_LIBRARIES(dfattachtest dfhack)

# materialtest - just list the first material of each type
ADD_EXECUTABLE(dfmaterialtest materialtest.cpp)
TARGET_LINK_LIBRARIES(dfmaterialtest dfhack)

# materialtest - just list the first material of each type
ADD_EXECUTABLE(dfposition position.cpp)
TARGET_LINK_LIBRARIES(dfposition dfhack)

#currently only stable on linux
IF(UNIX)
    # incremental - incremental memory search tool, a foreshadowing of the future direction of dfhack
    ADD_EXECUTABLE(dfincremental incrementalsearch.cpp)
    TARGET_LINK_LIBRARIES(dfincremental dfhack)
ENDIF(UNIX)

# suspendtest - test if suspend works. df should stop responding when suspended by dfhack
ADD_EXECUTABLE(dfsuspend suspendtest.cpp)
TARGET_LINK_LIBRARIES(dfsuspend dfhack)

# itemdump - dump the item under the cursor
ADD_EXECUTABLE(dfitemdump dfitemdump.cpp)
TARGET_LINK_LIBRARIES(dfitemdump dfhack)

# bauxite - turn all mechanisms into bauxite mechanisms
# Author: Alex Legg
ADD_EXECUTABLE(dfbauxite dfbauxite.cpp)
TARGET_LINK_LIBRARIES(dfbauxite dfhack)

# itemdesignator - change some item designations (dump, forbid, on-fire) for all items of a given type and material
ADD_EXECUTABLE(dfitemdesignator itemdesignator.cpp)
TARGET_LINK_LIBRARIES(dfitemdesignator dfhack)

# customCtreatureNameProf - change the custom names and professions of creatures, sends keys to df directly
ADD_EXECUTABLE(dfcustomCreatureNameProf customCreatureNameProf.cpp)
TARGET_LINK_LIBRARIES(dfcustomCreatureNameProf dfhack)

IF(UNIX)
install(TARGETS
dfexpbench
dfreveal
dfreveal
dfprospector
dfcleanmap
dfcreaturedump
dfattachtest
dfmaterialtest
dfbuildingsdump
dfposition
dfincremental
dfitemdump
dfsuspend
RUNTIME DESTINATION bin
)
ENDIF(UNIX)