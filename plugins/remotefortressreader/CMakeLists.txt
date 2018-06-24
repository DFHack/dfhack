PROJECT (remotefortressreader)
# A list of source files
SET(PROJECT_SRCS
    remotefortressreader.cpp
    adventure_control.cpp
    building_reader.cpp
 	dwarf_control.cpp
    item_reader.cpp
)
# A list of headers
SET(PROJECT_HDRS
    adventure_control.h
    building_reader.h
  	dwarf_control.h
    item_reader.h
    df_version_int.h
)
#proto files to include.
SET(PROJECT_PROTO
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/RemoteFortressReader.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/AdventureControl.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/ItemdefInstrument.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/DwarfControl.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/ui_sidebar_mode.pb.cc
)

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)
SET_SOURCE_FILES_PROPERTIES( ${PROJECT_PROTO} PROPERTIES GENERATED TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS};${PROJECT_PROTO})

IF(UNIX AND NOT APPLE)
    SET(PROJECT_LIBS ${PROJECT_LIBS} SDL)
ENDIF()

# this makes sure all the stuff is put in proper places and linked to dfhack
DFHACK_PLUGIN(RemoteFortressReader ${PROJECT_SRCS} LINK_LIBRARIES protobuf-lite ${PROJECT_LIBS} COMPILE_FLAGS_MSVC "/FI\"Export.h\"" COMPILE_FLAGS_GCC "-include Export.h -Wno-misleading-indentation" )
