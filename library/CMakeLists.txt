PROJECT (dfapi)
cmake_minimum_required(VERSION 2.8)

## build options
OPTION(BUILD_DEVEL "Install/package files required for development (For SDK)." OFF)
OPTION(BUILD_DOXYGEN "Create/install/package doxygen documentation for DFHack (For SDK)." OFF)
IF(UNIX)
    OPTION(CONSOLE_NO_CATCH "Make the console not catch 'CTRL+C' events for easier debugging." OFF)
    OPTION(MALLOC_FILL "Make malloc calls fill the allocated memory with 0xCC values." OFF)
ENDIF()

include_directories (include)
include_directories (depends/md5)
include_directories (depends/tinyxml)
include_directories (depends/tthread)
add_subdirectory (depends/protobuf)

SET(PERL_EXECUTABLE "perl" CACHE FILEPATH "This is the perl executable to run in the codegen step. Tweak it if you need to run a specific one.")

execute_process(COMMAND ${PERL_EXECUTABLE} xml/list.pl xml include/df ";"
                WORKING_DIRECTORY ${dfapi_SOURCE_DIR}
                OUTPUT_VARIABLE GENERATED_HDRS)

SET(PROJECT_HDRS_INTERNAL
)

SET(PROJECT_HDRS
include/Internal.h
include/DFHack.h
include/Console.h
include/Core.h
include/DataDefs.h
include/Error.h
include/Export.h
include/Hooks.h
include/MiscUtils.h
include/Module.h
include/Pragma.h
include/MemAccess.h
include/SDL_events.h
include/SDL_keyboard.h
include/SDL_keysym.h
include/TileTypes.h
include/Types.h
include/VersionInfo.h
include/VersionInfoFactory.h
include/Virtual.h
include/modules/Buildings.h
include/modules/Constructions.h
include/modules/Units.h
include/modules/Engravings.h
include/modules/Gui.h
include/modules/Items.h
include/modules/Job.h
include/modules/kitchen.h
include/modules/Maps.h
include/modules/MapCache.h
include/modules/Materials.h
include/modules/Notes.h
include/modules/Translation.h
include/modules/Vegetation.h
include/modules/Vermin.h
include/modules/World.h
include/modules/Graphic.h
)

SET(PROJECT_SRCS
Core.cpp
DataDefs.cpp
DataStatics.cpp
DataStaticsCtor.cpp
MiscUtils.cpp
PluginManager.cpp
TileTypes.cpp
VersionInfoFactory.cpp
Virtual.cpp

depends/md5/md5.cpp
depends/md5/md5wrapper.cpp

depends/tinyxml/tinystr.cpp
depends/tinyxml/tinyxml.cpp
depends/tinyxml/tinyxmlerror.cpp
depends/tinyxml/tinyxmlparser.cpp

depends/tthread/tinythread.cpp

modules/Buildings.cpp
modules/Constructions.cpp
modules/Units.cpp
modules/Engravings.cpp
modules/Gui.cpp
modules/Items.cpp
modules/Job.cpp
modules/kitchen.cpp
modules/Maps.cpp
modules/Materials.cpp
modules/Notes.cpp
modules/Translation.cpp
modules/Vegetation.cpp
modules/Vermin.cpp
modules/World.cpp
modules/Graphic.cpp
)

SET(PROJECT_HDRS_LINUX
)

SET(PROJECT_HDRS_WINDOWS
include/wdirent.h
)

SET(PROJECT_SRCS_LINUX
Console-linux.cpp
Hooks-linux.cpp
Process-linux.cpp
)

SET(PROJECT_SRCS_WINDOWS
Console-windows.cpp
Hooks-windows.cpp
Process-windows.cpp
)

IF(UNIX)
    LIST(APPEND PROJECT_HDRS ${PROJECT_HDRS_LINUX})
    LIST(APPEND PROJECT_SRCS ${PROJECT_SRCS_LINUX})
ELSE()
    LIST(APPEND PROJECT_HDRS ${PROJECT_HDRS_WINDOWS})
    LIST(APPEND PROJECT_SRCS ${PROJECT_SRCS_WINDOWS})
ENDIF()

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE )

LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

# Generation

SET_SOURCE_FILES_PROPERTIES(${GENERATED_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE GENERATED TRUE)

FILE(GLOB GENERATE_INPUT_SCRIPTS ${dfapi_SOURCE_DIR}/xml/*.pm ${dfapi_SOURCE_DIR}/xml/*.xslt)
FILE(GLOB GENERATE_INPUT_XMLS ${dfapi_SOURCE_DIR}/xml/*.xml)

ADD_CUSTOM_COMMAND(
    OUTPUT ${dfapi_SOURCE_DIR}/include/df/static.inc
    COMMAND ${PERL_EXECUTABLE} xml/codegen.pl xml include/df
    WORKING_DIRECTORY ${dfapi_SOURCE_DIR}
    MAIN_DEPENDENCY ${dfapi_SOURCE_DIR}/xml/codegen.pl
    DEPENDS ${GENERATE_INPUT_XMLS} ${GENERATE_INPUT_SCRIPTS}
)

ADD_CUSTOM_TARGET(generate_headers DEPENDS ${dfapi_SOURCE_DIR}/include/df/static.inc)

# Compilation

ADD_DEFINITIONS(-DBUILD_DFHACK_LIB)

IF(UNIX)
    IF(CONSOLE_NO_CATCH)
        ADD_DEFINITIONS(-DCONSOLE_NO_CATCH)
    ENDIF()
    IF(MALLOC_FILL)
        ADD_DEFINITIONS(-DMALLOC_FILL)
    ENDIF()
ENDIF()

IF(UNIX)
  SET(PROJECT_LIBS rt dl)
ELSE(WIN32)
  #FIXME: do we really need this?
  SET(PROJECT_LIBS psapi ${dfhack_SOURCE_DIR}/library/depends/ntdll/ntdll.lib)
ENDIF()

ADD_LIBRARY(dfhack SHARED ${PROJECT_SRCS})
ADD_DEPENDENCIES(dfhack generate_headers)

IF(WIN32)
    SET_TARGET_PROPERTIES(dfhack PROPERTIES OUTPUT_NAME "SDL" )
ENDIF()

#effectively disables debug builds...
SET_TARGET_PROPERTIES(dfhack  PROPERTIES DEBUG_POSTFIX "-debug" )

TARGET_LINK_LIBRARIES(dfhack ${PROJECT_LIBS})

IF(UNIX)
    # On linux, copy our version of the df launch script which sets LD_PRELOAD
    install(PROGRAMS ${dfhack_SOURCE_DIR}/package/linux/dfhack
            DESTINATION .)
ELSE()
    # On windows, copy the renamed SDL so DF can still run.
    install(PROGRAMS ${dfhack_SOURCE_DIR}/package/windows/SDLreal.dll
            DESTINATION ${DFHACK_LIBRARY_DESTINATION})
ENDIF()

#install the main lib
install(TARGETS dfhack
        LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
        RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})
#install the offset file
install(FILES xml/symbols.xml
        DESTINATION ${DFHACK_DATA_DESTINATION}) #linux: share/dfhack

if(BUILD_DEVEL)
    if(WIN32)
        install(TARGETS dfhack
                ARCHIVE DESTINATION ${DFHACK_DEVLIB_DESTINATION})
    endif()
    # note the ending '/'. This means *contents* of the directory are installed
    # without the '/', the directory itself is installed
    install(DIRECTORY include/
            DESTINATION ${DFHACK_INCLUDES_DESTINATION}
            FILES_MATCHING PATTERN "*.h" ) #linux: include
    # Building the docs
    IF(BUILD_DOXYGEN)
        add_subdirectory (doc)
    ENDIF()
endif()
