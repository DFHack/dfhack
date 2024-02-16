project(tweak)
set(PROJECT_SRCS
    tweak.cpp
)

dfhack_plugin(tweak ${PROJECT_SRCS} LINK_LIBRARIES lua)
