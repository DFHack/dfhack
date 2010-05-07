# don't use this file directly. use the one in the root folder of the project

SET(PROJECT_HDRS
include/DFCommonInternal.h
include/DFError.h
include/DFHackAPI.h
include/DFMemInfo.h
include/DFMemInfoManager.h
include/DFProcessEnumerator.h
include/DFProcess.h
include/DFTileTypes.h
include/DFTypes.h
include/DFVector.h
include/DFWindow.h
include/DFHackAPI_C.h
include/integers.h
shm/shms.h
)

SET(PROJECT_SRCS
DFMemInfo.cpp
DFMemInfoManager.cpp
DFHackAPI.cpp
APIPrivate.cpp
DFTileTypes.cpp
DFVector.cpp
DFHackAPI_C.cpp

depends/md5/md5.cpp
depends/md5/md5wrapper.cpp

depends/tinyxml/tinystr.cpp
depends/tinyxml/tinyxml.cpp
depends/tinyxml/tinyxmlerror.cpp
depends/tinyxml/tinyxmlparser.cpp

modules/Creatures.cpp
modules/Gui.cpp
modules/Maps.cpp
modules/Materials.cpp
modules/Position.cpp
modules/Translation.cpp
modules/Vegetation.cpp
modules/Buildings.cpp
modules/Constructions.cpp

modules/Position_C.cpp
modules/Gui_C.cpp
modules/Materials_C.cpp
modules/Buildings_C.cpp
modules/Constructions_C.cpp
modules/Maps_C.cpp
modules/Vegetation_C.cpp
)

SET(PROJECT_HDRS_LINUX
)

SET(PROJECT_HDRS_WINDOWS
include/stdint_win.h
)

SET(PROJECT_SRCS_LINUX
DFProcess-linux.cpp
DFProcess-linux-SHM.cpp
DFProcess-linux-wine.cpp
DFWindow-linux.cpp
DFProcessEnumerator-linux.cpp
)

SET(PROJECT_SRCS_WINDOWS
DFProcess-windows.cpp
DFProcess-windows-SHM.cpp
DFWindow-windows.cpp
DFProcessEnumerator-windows.cpp
)

IF(UNIX)
    LIST(APPEND PROJECT_HDRS ${PROJECT_HDRS_LINUX})
    LIST(APPEND PROJECT_SRCS ${PROJECT_SRCS_LINUX})
ELSE(UNIX)
    LIST(APPEND PROJECT_HDRS ${PROJECT_HDRS_WINDOWS})
    LIST(APPEND PROJECT_SRCS ${PROJECT_SRCS_WINDOWS})
ENDIF(UNIX)

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE )

LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

SET( MEMXML_DATA_PATH . CACHE PATH "search path for Memory.xml")
# OPTION( VARIABLE "Description" Initial state)
#OPTION( WITH_FOO "Enable FOO support" ON )
#OPTION( WITH_BAR "Enable BAR component" OFF )
    
# Are we 64bit? (Damn you, ptrace()!)
IF( CMAKE_SIZEOF_VOID_P MATCHES 4 )
    SET( HAVE_64_BIT 0 )
ELSE( CMAKE_SIZEOF_VOID_P MATCHES 4 )
    SET( HAVE_64_BIT 1 )
ENDIF( CMAKE_SIZEOF_VOID_P MATCHES 4 )

CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/dfhack/config.h.cmake ${CMAKE_SOURCE_DIR}/dfhack/include/config.h )

ADD_DEFINITIONS(-DBUILD_DFHACK_LIB)

IF(UNIX)
  add_definitions(-DLINUX_BUILD)
  add_definitions(-DUSE_CONFIG_H)
  find_library(X11_LIBRARY X11)
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
  SET(CMAKE_CXX_FLAGS "-fvisibility=hidden")
    
  SET(PROJECT_LIBS ${X11_LIBRARY} rt ) #dfhack-md5 dfhack-tixml
ELSE(UNIX)
  SET(PROJECT_LIBS psapi)
ENDIF(UNIX)

ADD_LIBRARY(dfhack SHARED ${PROJECT_SRCS})

SET_TARGET_PROPERTIES(dfhack  PROPERTIES DEBUG_POSTFIX "-debug" )

TARGET_LINK_LIBRARIES(dfhack ${PROJECT_LIBS})

IF(UNIX)
  install(TARGETS dfhack LIBRARY DESTINATION lib)
  install(FILES ${CMAKE_SOURCE_DIR}/output/Memory.xml DESTINATION share/dfhack)
ENDIF(UNIX)

# SWIG stuff is dead
# FIND_PACKAGE(SWIG)

#IF(SWIG_FOUND)
#    INCLUDE(${SWIG_USE_FILE})
#    FIND_PACKAGE(PythonLibs)
#    IF(PYTHONLIBS_FOUND)
#        INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
#        SET(CMAKE_SWIG_FLAGS "-c++")
#        SET_SOURCE_FILES_PROPERTIES(pydfhack.i PROPERTIES CPLUSPLUS ON)
#        SET(CMAKE_DFHACK_SWIG_OUTDIR ${dfhack_SOURCE_DIR}/output CACHE PATH "Directory where Java wrapped libraries will be saved.")
#        # SET_SOURCE_FILES_PROPERTIES(pydfhack.i PROPERTIES SWIG_FLAGS "-includeall")
#        SWIG_ADD_MODULE(pydfhack python pydfhack.i)
#        SWIG_LINK_LIBRARIES(pydfhack ${PYTHON_LIBRARIES} dfhack)
#    ENDIF(PYTHONLIBS_FOUND)
#ENDIF(SWIG_FOUND)