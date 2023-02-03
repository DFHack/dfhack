project(autolabor)
# A list of source files
set(COMMON_SRCS
)
# A list of headers
set(COMMON_HDRS laborstatemap.h
)
set_source_files_properties(${COMMON_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
list(APPEND COMMON_SRCS ${COMMON_HDRS})

#dfhack_plugin(labormanager labormanager.cpp joblabormapper.cpp ${COMMON_SRCS})

dfhack_plugin(autolabor autolabor.cpp ${COMMON_SRCS} LINK_LIBRARIES lua)
