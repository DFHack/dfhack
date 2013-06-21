PROJECT (rendermax)
Find_Package(SDL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
# A list of source files
SET(PROJECT_SRCS
    rendermax.cpp
)
# A list of headers
SET(PROJECT_HDRS
    renderer_opengl.hpp
)
SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})
include_directories(${SDL_INCLUDE_DIR})

#linux
IF(UNIX)
    add_definitions(-DLINUX_BUILD)
    SET(PROJECT_LIBS
        # add any extra linux libs here
		${SDL_LIBRARY}
		opengl
        ${PROJECT_LIBS}
    )
# windows
ELSE(UNIX)
    SET(PROJECT_LIBS
        # add any extra windows libs here
		${SDL_LIBRARY}
		opengl
        ${PROJECT_LIBS}
        $(NOINHERIT)
    )
ENDIF(UNIX)
# this makes sure all the stuff is put in proper places and linked to dfhack
DFHACK_PLUGIN(rendermax ${PROJECT_SRCS} LINK_LIBRARIES ${PROJECT_LIBS})
