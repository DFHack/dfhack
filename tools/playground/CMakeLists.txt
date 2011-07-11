if(NOT DEFINED DFHACK_CONSISTENCY)
    MESSAGE(FATAL_ERROR "Please build the whole thing, not parts. You can turn parts on/off using options.")
ENDIF()

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
    add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

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
DFHACK_TOOL(dfitemdesignator itemdesignator.cpp)

# catsplosion - Accelerates pregnancy
# Author: Zhentar
# FIXME: no longer works due to changes in DF
#DFHACK_TOOL(dfcatsplosion catsplosion.cpp)

# findnameindexes
# Author: belal
DFHACK_TOOL(dffindnameindexes findnameindexes.cpp)

# try things
DFHACK_TOOL(dftry dftry.cpp)

# renamer - change the custom names and professions of creatures, sends keys to
#           df directly
# Author: belal
# DFHACK_TOOL(dfrenamer renamer.cpp)

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

# dfcreaturemanager
# Author: raoulxq
# - Display creatures (overview & detail)
# - Modify skills and labors of creatures
# - Kill creatures
# - Etc. 
DFHACK_TOOL(dfcreaturemanager creaturemanager.cpp)

# digpattern
# Author: raoulxq
# Dig a specific pattern (in this case 3x3 bedrooms, modify as you like)
DFHACK_TOOL(dfdigpattern digpattern.cpp)

DFHACK_TOOL(dffixbug-3708 fix-3708.cpp)

# blockflags
# Author: matthew cline
# Invert/toggle all block flags, to see what they do.
DFHACK_TOOL(dfblockflags blockflags.cpp)

# this needs the C bindings
IF(BUILD_DFHACK_C_BINDINGS)
    # The C bindings won't be real C bindings until this compiles.
    #DFHACK_TOOL(dftest test.c)
ENDIF(BUILD_DFHACK_C_BINDINGS)
