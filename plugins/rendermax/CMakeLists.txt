project(rendermax)

# A list of source files
set(PROJECT_SRCS
    rendermax.cpp
    renderer_light.cpp
)
# A list of headers
set(PROJECT_HDRS
    renderer_opengl.hpp
    renderer_light.hpp
)
set_source_files_properties(${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
list(APPEND PROJECT_SRCS ${PROJECT_HDRS})

# this makes sure all the stuff is put in proper places and linked to dfhack
dfhack_plugin(rendermax ${PROJECT_SRCS} LINK_LIBRARIES lua)
install(FILES rendermax.lua
    DESTINATION ${DFHACK_DATA_DESTINATION}/raw)
