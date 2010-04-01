# don't use this file directly. use the one in the root folder of the project

SET(PROJECT_HDRS
DFCommonInternal.h
DFError.h
DFHackAPI.h
DFMemInfo.h
DFMemInfoManager.h
DFProcessEnumerator.h
DFProcess.h
DFTileTypes.h
DFTypes.h
DFVector.h
DFWindow.h
integers.h

md5/md5.h
md5/md5wrapper.h

tinyxml/tinystr.h
tinyxml/tinyxml.h

argstream/argstream.h

../shmserver/shms.h
)

SET(PROJECT_SRCS
DFMemInfo.cpp
DFMemInfoManager.cpp
DFHackAPI.cpp
DFTileTypes.cpp
DFVector.cpp
md5/md5.cpp
md5/md5wrapper.cpp
tinyxml/tinystr.cpp
tinyxml/tinyxml.cpp
tinyxml/tinyxmlerror.cpp
tinyxml/tinyxmlparser.cpp
)

SET(PROJECT_HDRS_LINUX
)

SET(PROJECT_HDRS_WINDOWS
stdint_win.h
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


CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/library/config.h.cmake ${CMAKE_SOURCE_DIR}/library/config.h )



IF(UNIX)
  add_definitions(-DLINUX_BUILD)
  add_definitions(-DUSE_CONFIG_H)
  find_library(X11_LIBRARY X11)
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -pedantic")
  SET(CMAKE_CXX_FLAGS "-fvisibility=hidden")
    
  SET(PROJECT_LIBS ${X11_LIBRARY} rt)
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

# SWIG stuff
FIND_PACKAGE(SWIG)

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