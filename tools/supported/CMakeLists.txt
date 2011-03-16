# don't use this file directly. use the one in the root folder of the project

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
add_definitions(-DLINUX_BUILD)
ENDIF()

IF(MSVC)
    ADD_CUSTOM_TARGET( memxml-for-supported
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/Memory.xml ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying Memory.xml to ${CMAKE_CURRENT_BINARY_DIR}")
    set(LOCAL_DEPNAME memxml-for-supported)
ENDIF()

# a reveal clone
DFHACK_TOOL(dfreveal reveal.cpp)

# force pause!
DFHACK_TOOL(dfpause forcepause.cpp)

# prospector - produces a list of available materials and their quantities
DFHACK_TOOL(dfprospector prospector.cpp)

# vdig - dig the vein under the cursor
DFHACK_TOOL(dfvdig vdig.cpp)

# cleanmap - removes mud, snow, blood and similar stuff from a map. farmers beware
DFHACK_TOOL(dfcleanmap cleanmap.cpp)

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
#DFHACK_TOOL(dfcleartask cleartask.cpp)

# position - check the DF window and cursor parameters
DFHACK_TOOL(dfposition position.cpp)

# mode - a utility to change the current game and control modes
DFHACK_TOOL(dfmode mode.cpp)

# just dump offsets of the current version
DFHACK_TOOL(dfdoffsets dumpoffsets.cpp)

# change the weather
DFHACK_TOOL(dfweather weather.cpp)

# incrementalsearch - a bit like cheat engine, only DF-specific, very basic
#DFHACK_TOOL(dfautosearch autosearch.cpp)
DFHACK_TOOL(dfincremental incrementalsearch.cpp)


IF(UNIX)
    SET(VEINLOOK_BUILT "NO")
    SET(CURSES_NEED_WIDE "YES")
    SET(CURSES_NEED_NCURSES "NO")
    find_package(Curses QUIET)
    IF(CURSES_FOUND)
        if(CURSES_HAVE_NCURSESW_NCURSES_H)
            SET(NCURSES_H "ncursesw/ncurses.h")
        elseif(CURSES_HAVE_NCURSESW_CURSES_H)
            SET(NCURSES_H "ncursesw/curses.h")
        elseif(CURSES_HAVE_NCURSESW_H)
            SET(NCURSES_H "ncursesw.h")
        elseif(CURSES_HAVE_CURSESW_H)
            SET(NCURSES_H "cursesw.h")
        endif(CURSES_HAVE_NCURSESW_NCURSES_H)
        IF(NCURSES_H)
            # OPTION( VARIABLE "Description" Initial state)
            #OPTION( WITH_FOO "Enable FOO support" ON )
            #OPTION( WITH_BAR "Enable BAR component" OFF )
            #SET( BAZ 18 )
            CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/fake-curses.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/fake-curses.h )
            # veinlook - look at the map... sort of
            ADD_EXECUTABLE(dfveinlook veinlook.cpp)
            INCLUDE_DIRECTORIES(${CURSES_INCLUDE_DIR})
            TARGET_LINK_LIBRARIES(dfveinlook dfhack ${CURSES_LIBRARIES})
            install(TARGETS
            dfveinlook
            RUNTIME DESTINATION bin
            )
        ENDIF(NCURSES_H)
    ELSE(CURSES_FOUND)
        MESSAGE(STATUS "Wide-character ncurses library not found - veinlook can't be built")
    ENDIF(CURSES_FOUND)
ENDIF(UNIX)
