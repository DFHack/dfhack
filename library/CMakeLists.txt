PROJECT (dfapi)
cmake_minimum_required(VERSION 2.8)

## build options
OPTION(BUILD_DEVEL "Install/package files required for development (For SDK)." OFF)
OPTION(BUILD_DOXYGEN "Create/install/package doxygen documentation for DFHack (For SDK)." OFF)

include_directories (include)
include_directories (depends/md5)
include_directories (depends/libnoise)
include_directories (depends/tinyxml)
include_directories (private)

SET(PROJECT_HDRS_INTERNAL
    private/ContextShared.h
    private/Internal.h
    private/wdirent.h
)

SET(PROJECT_HDRS
include/DFHack.h
include/dfhack/Error.h
include/dfhack/Export.h
include/dfhack/Virtual.h
include/dfhack/MiscUtils.h
include/dfhack/Module.h
include/dfhack/Pragma.h
include/dfhack/Console.h
include/dfhack/Process.h
include/dfhack/TileTypes.h
include/dfhack/Types.h
include/dfhack/Vector.h
include/dfhack/VersionInfoFactory.h
include/dfhack/VersionInfo.h
include/dfhack/extra/MapExtras.h
include/dfhack/extra/termutil.h
include/dfhack/extra/stopwatch.h
include/dfhack/extra/stdiostream.h
include/dfhack/modules/Buildings.h
include/dfhack/modules/Constructions.h
include/dfhack/modules/Creatures.h
include/dfhack/modules/Engravings.h
include/dfhack/modules/Gui.h
include/dfhack/modules/Items.h
include/dfhack/modules/Maps.h
include/dfhack/modules/Materials.h
include/dfhack/modules/Translation.h
include/dfhack/modules/Vegetation.h
include/dfhack/modules/World.h
)

SET(PROJECT_SRCS
Core.cpp
PluginManager.cpp
VersionInfo.cpp
VersionInfoFactory.cpp
TileTypes.cpp
Virtual.cpp

depends/md5/md5.cpp
depends/md5/md5wrapper.cpp

depends/tinyxml/tinystr.cpp
depends/tinyxml/tinyxml.cpp
depends/tinyxml/tinyxmlerror.cpp
depends/tinyxml/tinyxmlparser.cpp

modules/Buildings.cpp
modules/Constructions.cpp
modules/Creatures.cpp
modules/Engravings.cpp
modules/Gui.cpp
modules/Items.cpp
modules/Maps.cpp
modules/Materials.cpp
modules/Translation.cpp
modules/Vegetation.cpp
modules/Vermin.cpp
modules/World.cpp
)

SET(PROJECT_HDRS_LINUX
)

SET(PROJECT_HDRS_WINDOWS
)

SET(PROJECT_SRCS_LINUX
FakeSDL-linux.cpp
Console-linux.cpp
Process-linux.cpp
)

SET(PROJECT_SRCS_WINDOWS
FakeSDL-windows.cpp
Console-windows.cpp
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

ADD_DEFINITIONS(-DBUILD_DFHACK_LIB)

IF(UNIX)
  add_definitions(-DLINUX_BUILD)
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
  SET(CMAKE_CXX_FLAGS "-fvisibility=hidden -m32")
  SET(CMAKE_C_FLAGS "-fvisibility=hidden -m32")

  SET(PROJECT_LIBS rt )
ELSE()
  IF(MSVC)
    SET(PROJECT_LIBS psapi ${dfhack_SOURCE_DIR}/library/depends/ntdll/ntdll.lib)
  ELSE()
    SET(PROJECT_LIBS psapi ntdll)
  ENDIF()
ENDIF()

ADD_LIBRARY(dfhack SHARED ${PROJECT_SRCS})

IF(WIN32)
    SET_TARGET_PROPERTIES(dfhack PROPERTIES OUTPUT_NAME "SDL" )
ENDIF()

SET_TARGET_PROPERTIES(dfhack  PROPERTIES DEBUG_POSTFIX "-debug" )

TARGET_LINK_LIBRARIES(dfhack ${PROJECT_LIBS})

ADD_CUSTOM_TARGET( prepare
DEPENDS ${dfhack_SOURCE_DIR}/Memory.xml
COMMAND ${CMAKE_COMMAND} -E make_directory ${DFHACK_OUTPUT_DIR}
COMMAND ${CMAKE_COMMAND} -E make_directory ${DFHACK_PLUGIN_OUTPUT_DIR}
COMMAND ${CMAKE_COMMAND} -E copy ${dfhack_SOURCE_DIR}/Memory.xml ${DFHACK_OUTPUT_DIR})
ADD_DEPENDENCIES(dfhack prepare)

# Copy our version of the df launch script which sets LD_PRELOAD
IF(UNIX)
    ADD_CUSTOM_TARGET( prepare_UNIX
    DEPENDS ${dfhack_SOURCE_DIR}/package/linux/dfhack
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DFHACK_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${dfhack_SOURCE_DIR}/package/linux/dfhack ${DFHACK_OUTPUT_DIR})
    ADD_DEPENDENCIES(dfhack prepare_UNIX)
    install(PROGRAMS ${dfhack_SOURCE_DIR}/package/linux/dfhack
            DESTINATION ${DFHACK_LIBRARY_DESTINATION}) #linux: share/dfhack

ENDIF()
 
install(TARGETS dfhack
        LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
        RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION}) #linux: lib
install(FILES ${dfhack_SOURCE_DIR}/Memory.xml
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
