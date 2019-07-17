project(remotefortressreader)
# A list of source files
set(PROJECT_SRCS
    remotefortressreader.cpp
    adventure_control.cpp
    building_reader.cpp
    item_reader.cpp
)
# A list of headers
set(PROJECT_HDRS
    adventure_control.h
    building_reader.h
    item_reader.h
    df_version_int.h
)
# proto files to include.
set(PROJECT_PROTO
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/RemoteFortressReader.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/AdventureControl.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/ItemdefInstrument.pb.cc
)

set_source_files_properties(${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${PROJECT_PROTO} PROPERTIES GENERATED TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
list(APPEND PROJECT_SRCS ${PROJECT_HDRS};${PROJECT_PROTO})

if(UNIX AND NOT APPLE)
    set(PROJECT_LIBS ${PROJECT_LIBS} SDL)
endif()

# this makes sure all the stuff is put in proper places and linked to dfhack
dfhack_plugin(RemoteFortressReader ${PROJECT_SRCS} LINK_LIBRARIES protobuf-lite ${PROJECT_LIBS} COMPILE_FLAGS_MSVC "/FI\"Export.h\"" COMPILE_FLAGS_GCC "-include Export.h -Wno-misleading-indentation" )
