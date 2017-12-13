PROJECT (labormanager)
# A list of source files
SET(PROJECT_SRCS
    labormanager.cpp
    joblabormapper.cpp
)
# A list of headers
SET(PROJECT_HDRS
    labormanager.h
    joblabormapper.h
)
SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

#linux
IF(UNIX)
    add_definitions(-DLINUX_BUILD)
    SET(PROJECT_LIBS
        # add any extra linux libs here
        ${PROJECT_LIBS}
    )
# windows
ELSE(UNIX)
    SET(PROJECT_LIBS
        # add any extra linux libs here
        ${PROJECT_LIBS}
        $(NOINHERIT)
    )
ENDIF(UNIX)
# this makes sure all the stuff is put in proper places and linked to dfhack

DFHACK_PLUGIN(labormanager ${PROJECT_SRCS} LINK_LIBRARIES ${PROJECT_LIBS})
