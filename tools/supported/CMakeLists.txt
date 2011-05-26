if(NOT DEFINED DFHACK_CONSISTENCY)
    MESSAGE(FATAL_ERROR "Please build the whole thing, not parts. You can turn parts on/off using options.")
ENDIF()

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
    add_definitions(-DLINUX_BUILD)
ENDIF()

# burn trees and shrubs to ashes
DFHACK_TOOL(dfimmolate immolate.cpp)

# grow saplings into trees instantly
DFHACK_TOOL(dfgrow grow.cpp)

# a reveal clone
DFHACK_TOOL(dfreveal reveal.cpp)

# re-hide borked reveals
DFHACK_TOOL(dfunreveal unreveal.cpp)

# designate all visible floor tiles as lair, freezing items in place
DFHACK_TOOL(dflair lair.cpp)

# force pause!
DFHACK_TOOL(dfpause forcepause.cpp)

# prospector - produces a list of available materials and their quantities
DFHACK_TOOL(dfprospector prospector.cpp)
IF(WIN32)
    INSTALL(PROGRAMS dfprospector-all.bat DESTINATION ${DFHACK_BINARY_DESTINATION})
ENDIF()

# vdig - dig the vein under the cursor
DFHACK_TOOL(dfvdig vdig.cpp)
IF(WIN32)
    INSTALL(PROGRAMS dfXvdig.bat DESTINATION ${DFHACK_BINARY_DESTINATION})
ENDIF()

# cleanmap - removes mud, snow, blood and similar stuff from a map. farmers beware
DFHACK_TOOL(dfcleanmap cleanmap.cpp)

# cleanowned - confiscate items owned by dwarves so they can be atomsmashed/reused/etc.
DFHACK_TOOL(dfcleanowned cleanowned.cpp)
IF(WIN32)
    INSTALL(PROGRAMS dfremovelitter.bat DESTINATION ${DFHACK_BINARY_DESTINATION})
    INSTALL(PROGRAMS dfconfiscate.bat DESTINATION ${DFHACK_BINARY_DESTINATION})
ENDIF()

# unstuck - make DF run if something goes wrong with the 'normal' memory access method
DFHACK_TOOL(dfunstuck unstuck.cpp)

# probe - map tile probe
DFHACK_TOOL(dfprobe probe.cpp)

# attachtest - 100x attach/detach, suspend/resume
DFHACK_TOOL(dfattachtest attachtest.cpp)

# a benchmark program, reads the map 1000x
DFHACK_TOOL(dfexpbench expbench.cpp)

# suspendtest - test if suspend works. df should stop responding when suspended
#               by dfhack
DFHACK_TOOL(dfsuspend suspendtest.cpp)

# flows - check flows impact on fps
DFHACK_TOOL(dfflows flows.cpp)

# liquids manipulation tool
# Original author: Aleric
DFHACK_TOOL(dfliquids liquids.cpp)

# Solves the problem of unusable items after reclaim by clearing the 'in_job' bit of all items.
# Original author: Quietust
DFHACK_TOOL(dfcleartask cleartask.cpp)

# position - check the DF window and cursor parameters
DFHACK_TOOL(dfposition position.cpp)

# mode - a utility to change the current game and control modes
DFHACK_TOOL(dfmode mode.cpp)

# just dump offsets of the current version
DFHACK_TOOL(dfdoffsets dumpoffsets.cpp)

# change the weather
DFHACK_TOOL(dfweather weather.cpp)

# refill adamantine veins
DFHACK_TOOL(dftubefill dftubefill.cpp)

# deramp
# Author: zilpin
# seeks entire map for 'remove ramp' designation, makes a floor, removes designation.
# intended use is to simulate old 'channel' functionality.
DFHACK_TOOL(dfderamp deramp.cpp)

# incrementalsearch - a bit like cheat engine, only DF-specific, very basic
#DFHACK_TOOL(dfautosearch autosearch.cpp)
DFHACK_TOOL(dfincremental incrementalsearch.cpp)

# auto dump. dumps all items marked for dumping at the cursor position without the need for dwarf labor.
# ... or just kills the items? :P
DFHACK_TOOL(dfautodump autodump.cpp)
IF(WIN32)
    INSTALL(PROGRAMS dfautodestroy.bat DESTINATION ${DFHACK_BINARY_DESTINATION})
ENDIF()

# tile types. tile shape, material, and special information manipulation tool
DFHACK_TOOL(dftiletypes tiletypes.cpp)

# veinlook - look at the map... sort of, kind of
DFHACK_CURSES_TOOL(dfveinlook veinlook.cpp)

# dfstatus - watches DF and prints stuff in a window
DFHACK_CURSES_TOOL(dfstatus dfstatus.cpp)
