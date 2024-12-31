project(remotefortressreader)
# A list of source files
set(PROJECT_SRCS
    remotefortressreader.cpp
    adventure_control.cpp
    building_reader.cpp
    dwarf_control.cpp
    item_reader.cpp
)
# A list of headers
set(PROJECT_HDRS
    adventure_control.h
    building_reader.h
    dwarf_control.h
    item_reader.h
    df_version_int.h
)
# proto files to include.
set(PROJECT_PROTO
    RemoteFortressReader
    AdventureControl
    ItemdefInstrument
    DwarfControl
    ui_sidebar_mode
)

if(UNIX AND NOT APPLE)
    set(PROJECT_LIBS ${PROJECT_LIBS})
endif()

# this makes sure all the stuff is put in proper places and linked to dfhack
dfhack_plugin(RemoteFortressReader ${PROJECT_SRCS} LINK_LIBRARIES ${PROJECT_LIBS} PROTOBUFS ${PROJECT_PROTO})
target_include_directories(RemoteFortressReader PRIVATE ${SDL2_INCLUDE_DIRS})
