project(skeleton)
# A list of source files
set(PROJECT_SRCS
    skeleton.cpp
)
# A list of headers
set(PROJECT_HDRS
    skeleton.h
)
set_source_files_properties(${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
list(APPEND PROJECT_SRCS ${PROJECT_HDRS})

# option to use a thread for no particular reason
option(SKELETON_THREAD "Use threads in the skeleton plugin." ON)
if(UNIX)
    if(APPLE)
        set(PROJECT_LIBS
            # add any extra mac libraries here
            ${PROJECT_LIBS}
        )
    else()
        set(PROJECT_LIBS
            # add any extra linux libraries here
            ${PROJECT_LIBS}
        )
    endif()
else()
    set(PROJECT_LIBS
        # add any extra windows libraries here
        ${PROJECT_LIBS}
    )
endif()
# this makes sure all the stuff is put in proper places and linked to dfhack
dfhack_plugin(skeleton ${PROJECT_SRCS} LINK_LIBRARIES ${PROJECT_LIBS})
