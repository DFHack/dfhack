if(NOT DEFINED DFHACK_CONSISTENCY)
    MESSAGE(FATAL_ERROR "Please build the whole thing, not parts. You can turn parts on/off using options.")
ENDIF()

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
    add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

# deep magic. make sure it runs to be able to do debug runs for MSVC projects out of the box
IF(MSVC)
    ADD_CUSTOM_TARGET( memxml-for-playground
    COMMAND ${CMAKE_COMMAND} -E copy ${dfhack_SOURCE_DIR}/Memory.xml ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying Memory.xml to ${CMAKE_CURRENT_BINARY_DIR}"
    )
    set(LOCAL_DEPNAME memxml-for-playground)
ENDIF()

# a creature mood dump hack. has hardcoded offsets
DFHACK_TOOL(dfmoodump moodump.cpp)

# bauxite - turn all mechanisms into bauxite mechanisms
# Author: Alex Legg
# FIXME: turned off. there is no reliable Items module.
#DFHACK_TOOL(dfbauxite dfbauxite.cpp)

# digger - designate for digging by tile class
# Author: mizipzor
DFHACK_TOOL(dfdigger digger.cpp)

# digger2 - designate for digging from a text file
# Author: rOut
DFHACK_TOOL(dfdigger2 digger2.cpp)

# itemdesignator - change some item designations (dump, forbid, on-fire) for all
#                  items of a given type and material
# Author: belal
# FIXME: turned off. there is no reliable Items module.
#DFHACK_TOOL(dfitemdesignator itemdesignator.cpp)

# catsplosion - Accelerates pregnancy
# Author: Zhentar
# FIXME: no longer works due to changes in DF
#DFHACK_TOOL(dfcatsplosion catsplosion.cpp)

# findnameindexes
# Author: belal
#DFHACK_TOOL(dffindnameindexes findnameindexes.cpp)

# renamer - change the custom names and professions of creatures, sends keys to
#           df directly
# Author: belal
#DFHACK_TOOL(dfrenamer renamer.cpp)

# copypaste
# Author: belal
# copies the current buildings in a df map, and then designates the area to be dug
# mainly a proof of concept for my gui application dfCopyPaste
DFHACK_TOOL(dfcopypaste copypaste.cpp)

# paths
# Author: belal
# dumps the current path to the DF exe, as well as the relative paths to the 
# current tileset and color files
DFHACK_TOOL(dfpaths paths.cpp)

# printtiletypes
# Author: zilpin
# Prints CSV dump of all tile type information.
# No DF process needed.  Intended only for debugging and information purposes.
DFHACK_TOOL(dfprinttiletypes printtiletypes.cpp)

# hellhole
# Author: zilpin
# Creates a bottomless hole to hell.
# Experimental version hard-codes values.
# Will have many options in the future.
DFHACK_TOOL(dfhellhole hellhole.cpp)

# this needs the C bindings
IF(BUILD_DFHACK_C_BINDINGS)
    # for trying out some 'stuff'
    DFHACK_TOOL(dftest test.cpp)
ENDIF(BUILD_DFHACK_C_BINDINGS)
