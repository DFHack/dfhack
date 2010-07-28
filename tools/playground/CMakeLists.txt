# don't use this file directly. use the one in the root folder of the project

# only build this stuff when BUILD_DFHACK_PLAYGROUND is set to ON
IF (BUILD_DFHACK_PLAYGROUND)

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

# a creature mood dump hack. has hardcoded offsets
ADD_EXECUTABLE(dfmoodump moodump.cpp)
TARGET_LINK_LIBRARIES(dfmoodump dfhack)

# for trying out some 'stuff'
ADD_EXECUTABLE(dftest test.cpp)
TARGET_LINK_LIBRARIES(dftest dfhack)

# just dump offsets of the current version
ADD_EXECUTABLE(dfdoffsets dumpoffsets.cpp)
TARGET_LINK_LIBRARIES(dfdoffsets dfhack)

# bauxite - turn all mechanisms into bauxite mechanisms
# Author: Alex Legg
#ADD_EXECUTABLE(dfbauxite dfbauxite.cpp)
#TARGET_LINK_LIBRARIES(dfbauxite dfhack)

# digger - designate for digging by tile class
# Author: mizipzor
ADD_EXECUTABLE(dfdigger digger.cpp)
TARGET_LINK_LIBRARIES(dfdigger dfhack)

# digger2 - designate for digging from a text file
# Author: rOut
ADD_EXECUTABLE(dfdigger2 digger2.cpp)
TARGET_LINK_LIBRARIES(dfdigger2 dfhack)

# itemdesignator - change some item designations (dump, forbid, on-fire) for all
#                  items of a given type and material
# Author: belal
#ADD_EXECUTABLE(dfitemdesignator itemdesignator.cpp)
#TARGET_LINK_LIBRARIES(dfitemdesignator dfhack)

# incrementalsearch - a bit like cheat engine, only DF-specific, very basic
# and Linux-only
IF(UNIX)
    ADD_EXECUTABLE(dfincremental incrementalsearch.cpp)
    TARGET_LINK_LIBRARIES(dfincremental dfhack)
ENDIF(UNIX)

# catsplosion - Accelerates pregnancy
# Author: Zhentar
ADD_EXECUTABLE(dfcatsplosion catsplosion.cpp)
TARGET_LINK_LIBRARIES(dfcatsplosion dfhack)

# findnameindexes
# Author: belal
#ADD_EXECUTABLE(dffindnameindexes findnameindexes.cpp)
#TARGET_LINK_LIBRARIES(dffindnameindexes dfhack)

# renamer - change the custom names and professions of creatures, sends keys to
#           df directly
# Author: belal
#ADD_EXECUTABLE(dfrenamer renamer.cpp)
#TARGET_LINK_LIBRARIES(dfrenamer dfhack)

install(TARGETS
dfmoodump
dftest
dfdoffsets
dfdigger
dfdigger2
dfcatsplosion
RUNTIME DESTINATION bin
)
IF(UNIX)
    install(TARGETS
    dfincremental
    RUNTIME DESTINATION bin
    )
ENDIF(UNIX)
ENDIF (BUILD_DFHACK_PLAYGROUND)