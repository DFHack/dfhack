if(NOT DEFINED DFHACK_CONSISTENCY)
    MESSAGE(FATAL_ERROR "Please build the whole thing, not parts. You can turn parts on/off using options.")
ENDIF()

PROJECT (dfhack-library)
cmake_minimum_required(VERSION 2.8)

include_directories (include)
include_directories (depends/md5)
include_directories (depends/tinyxml)
include_directories (depends/argstream)
include_directories (private)

SET(PROJECT_HDRS_INTERNAL
    private/ContextShared.h
    private/Internal.h
    private/SHMProcess.h
    private/LinuxProcess.h
    private/ProcessFactory.h
    private/MicrosoftSTL.h
)

SET(PROJECT_HDRS
include/DFHack.h
include/dfhack/DFContext.h
include/dfhack/DFContextManager.h
include/dfhack/DFError.h
include/dfhack/DFExport.h
include/dfhack/DFIntegers.h
include/dfhack/DFMiscUtils.h
include/dfhack/DFModule.h
include/dfhack/DFPragma.h
include/dfhack/DFProcess.h
include/dfhack/DFProcessEnumerator.h
include/dfhack/DFTileTypes.h
include/dfhack/DFTypes.h
include/dfhack/DFVector.h
include/dfhack/VersionInfoFactory.h
include/dfhack/VersionInfo.h
include/dfhack/extra/MapExtras.h
include/dfhack/extra/termutil.h
include/dfhack/extra/stopwatch.h
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
include/dfhack/modules/WindowIO.h
include/dfhack/modules/World.h
)

SET(PROJECT_C_HDRS
include/DFHack_C.h
include/dfhack-c/DFProcess_C.h
include/dfhack-c/DFTypes_C.h
include/dfhack-c/DFTileTypes_C.h
include/dfhack-c/DFContext_C.h
include/dfhack-c/modules/Buildings_C.h
include/dfhack-c/modules/Constructions_C.h
include/dfhack-c/modules/Creatures_C.h
include/dfhack-c/modules/Gui_C.h
include/dfhack-c/modules/Items_C.h
include/dfhack-c/modules/Maps_C.h
include/dfhack-c/modules/Materials_C.h
include/dfhack-c/modules/Translation_C.h
include/dfhack-c/modules/Vegetation_C.h
include/dfhack-c/modules/WindowIO_C.h
include/dfhack-c/modules/World_C.h
)

SET(PROJECT_SRCS
VersionInfo.cpp
VersionInfoFactory.cpp
DFContextManager.cpp
DFContext.cpp
DFTileTypes.cpp
DFProcessEnumerator.cpp
ContextShared.cpp
DFProcess-SHM.cpp
MicrosoftSTL.cpp

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
modules/World.cpp
)

SET(PROJECT_C_SRCS
DFContext_C.cpp
DFProcess_C.cpp
DFTypes_C.cpp
DFTileTypes_C.cpp

modules/Buildings_C.cpp
modules/Constructions_C.cpp
modules/Creatures_C.cpp
modules/Gui_C.cpp
modules/Items_C.cpp
modules/Maps_C.cpp
modules/Materials_C.cpp
modules/Translation_C.cpp
modules/Vegetation_C.cpp
modules/WindowIO_C.cpp
modules/World_C.cpp
)

SET(PROJECT_HDRS_LINUX
)

SET(PROJECT_HDRS_WINDOWS
include/dfhack/DFstdint_win.h
)

SET(PROJECT_SRCS_LINUX
DFProcess-linux.cpp
DFProcess-linux-base.cpp
DFProcess-linux-SHM.cpp
DFProcess-linux-wine.cpp
modules/WindowIO-linux.cpp
)

SET(PROJECT_SRCS_WINDOWS
DFProcess-windows.cpp
DFProcess-windows-SHM.cpp
modules/WindowIO-windows.cpp
)

IF(UNIX)
    LIST(APPEND PROJECT_HDRS ${PROJECT_HDRS_LINUX})
    LIST(APPEND PROJECT_SRCS ${PROJECT_SRCS_LINUX})
ELSE()
    LIST(APPEND PROJECT_HDRS ${PROJECT_HDRS_WINDOWS})
    LIST(APPEND PROJECT_SRCS ${PROJECT_SRCS_WINDOWS})
ENDIF()

IF(BUILD_DFHACK_C_BINDINGS)
    LIST(APPEND PROJECT_HDRS ${PROJECT_C_HDRS})
    LIST(APPEND PROJECT_SRCS ${PROJECT_C_SRCS})
ENDIF()

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE )

LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

# Are we 64bit? (Damn you, ptrace()!)
IF( CMAKE_SIZEOF_VOID_P MATCHES 8 )
    ADD_DEFINITIONS(-DHAVE_64_BIT)
ENDIF()

ADD_DEFINITIONS(-DMEMXML_DATA_PATH="${MEMXML_DATA_PATH}")
ADD_DEFINITIONS(-DBUILD_DFHACK_LIB)

IF(UNIX)
  add_definitions(-DLINUX_BUILD)
  find_library(X11_LIBRARY X11)
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
  SET(CMAKE_CXX_FLAGS "-fvisibility=hidden")

  SET(PROJECT_LIBS ${X11_LIBRARY} rt )
ELSE()
  IF(MSVC)
    SET(PROJECT_LIBS psapi ${dfhack_SOURCE_DIR}/library/depends/ntdll/ntdll.lib)
  ELSE()
    SET(PROJECT_LIBS psapi ntdll)
  ENDIF()
ENDIF()

ADD_LIBRARY(dfhack SHARED ${PROJECT_SRCS})

SET_TARGET_PROPERTIES(dfhack  PROPERTIES DEBUG_POSTFIX "-debug" )

TARGET_LINK_LIBRARIES(dfhack ${PROJECT_LIBS})

ADD_CUSTOM_TARGET( memxmlcopy
DEPENDS ${dfhack_SOURCE_DIR}/Memory.xml
COMMAND ${CMAKE_COMMAND} -E make_directory ${dfhack_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/
COMMAND ${CMAKE_COMMAND} -E copy ${dfhack_SOURCE_DIR}/Memory.xml ${dfhack_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/)
ADD_DEPENDENCIES(dfhack memxmlcopy)

install(TARGETS dfhack
        LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
        RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION}) #linux: lib
install(FILES ${dfhack_SOURCE_DIR}/Memory.xml
        DESTINATION ${DFHACK_DATA_DESTINATION}) #linux: share/dfhack

if(BUILD_DFHACK_DEVEL)
    if(WIN32)
        install(TARGETS dfhack
                ARCHIVE DESTINATION ${DFHACK_DEVLIB_DESTINATION})
    endif()
    # note the ending '/'. This means *constents* of the directory are installed
    # without the '/', the directory itself is installed
    install(DIRECTORY include/
            DESTINATION ${DFHACK_INCLUDES_DESTINATION}
            FILES_MATCHING PATTERN "*.h" ) #linux: include
endif()
