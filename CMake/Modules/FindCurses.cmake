IF(Curses_FOUND)
  SET(Curses_FIND_QUIETLY TRUE)
ENDIF()

FIND_PATH(Curses_INCLUDE_PATH
  NAMES ncurses.h curses.h
  PATH_SUFFIXES ncurses
  PATHS /usr/include /usr/local/include /usr/pkg/include
)

FIND_LIBRARY(Curses_LIBRARY
  NAMES ncursesw
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

IF (Curses_INCLUDE_PATH AND Curses_LIBRARY)
  SET(Curses_FOUND TRUE)
ENDIF()

MARK_AS_ADVANCED(
  Curses_INCLUDE_PATH
  Curses_LIBRARY
)
