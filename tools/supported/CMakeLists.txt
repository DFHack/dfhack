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

# vdig - dig the vein under the cursor
ADD_EXECUTABLE(dfvdig vdig.cpp)
TARGET_LINK_LIBRARIES(dfvdig dfhack)

# cleanmap - removes mud, snow, blood and similar stuff from a map. farmers beware
ADD_EXECUTABLE(dfcleanmap cleanmap.cpp)
TARGET_LINK_LIBRARIES(dfcleanmap dfhack)

# unstuck - make DF run if something goes wrong with the 'normal' memory access method
ADD_EXECUTABLE(dfunstuck unstuck.cpp)
TARGET_LINK_LIBRARIES(dfunstuck dfhack)

# probe - map tile probe
ADD_EXECUTABLE(dfprobe probe.cpp)
TARGET_LINK_LIBRARIES(dfprobe dfhack)

# attachtest - 100x attach/detach, suspend/resume
ADD_EXECUTABLE(dfattachtest attachtest.cpp)
TARGET_LINK_LIBRARIES(dfattachtest dfhack)

# a benchmark program, reads the map 1000x
ADD_EXECUTABLE(dfexpbench expbench.cpp)
TARGET_LINK_LIBRARIES(dfexpbench dfhack)

# suspendtest - test if suspend works. df should stop responding when suspended
#               by dfhack
ADD_EXECUTABLE(dfsuspend suspendtest.cpp)
TARGET_LINK_LIBRARIES(dfsuspend dfhack)

# flows - check flows impact on fps
ADD_EXECUTABLE(dfflows flows.cpp)
TARGET_LINK_LIBRARIES(dfflows dfhack)

# liquids manipulation tool
# Original author: Aleric
ADD_EXECUTABLE(dfliquids liquids.cpp)
TARGET_LINK_LIBRARIES(dfliquids dfhack)

# position - check the DF window and cursor parameters
ADD_EXECUTABLE(dfposition position.cpp)
TARGET_LINK_LIBRARIES(dfposition dfhack)

# just dump offsets of the current version
ADD_EXECUTABLE(dfdoffsets dumpoffsets.cpp)
TARGET_LINK_LIBRARIES(dfdoffsets dfhack)

IF(UNIX)
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


install(TARGETS
dfreveal
dfprospector
dfposition
dfvdig
dfcleanmap
dfunstuck
dfprobe
dfdoffsets
dfattachtest
dfexpbench
dfsuspend
dfflows
dfliquids
RUNTIME DESTINATION bin
)
IF(UNIX)
    install(TARGETS
    dfveinlook
    RUNTIME DESTINATION bin
    )
ENDIF(UNIX)
