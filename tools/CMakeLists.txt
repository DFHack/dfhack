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

# cleanmap - removes mud, snow, blood and similar stuff from a map. farmers beware
ADD_EXECUTABLE(dfcleanmap cleanmap.cpp)
TARGET_LINK_LIBRARIES(dfcleanmap dfhack)

# incrementalsearch - a bit like cheat engine, only DF-specific and very basic
ADD_EXECUTABLE(dfincremental incrementalsearch.cpp)
TARGET_LINK_LIBRARIES(dfincremental dfhack)

# bauxite - turn all mechanisms into bauxite mechanisms
# Author: Alex Legg
ADD_EXECUTABLE(dfbauxite dfbauxite.cpp)
TARGET_LINK_LIBRARIES(dfbauxite dfhack)

# digger - designate for digging by tile class
# Author: mizipzor
ADD_EXECUTABLE(dfdigger digger.cpp)
TARGET_LINK_LIBRARIES(dfdigger dfhack)

# itemdesignator - change some item designations (dump, forbid, on-fire) for all items of a given type and material
ADD_EXECUTABLE(dfitemdesignator itemdesignator.cpp)
TARGET_LINK_LIBRARIES(dfitemdesignator dfhack)


IF(UNIX)
install(TARGETS
dfreveal
dfprospector
dfcleanmap
dfincremental
dfbauxite
dfitemdesignator
RUNTIME DESTINATION bin
)
ENDIF(UNIX)