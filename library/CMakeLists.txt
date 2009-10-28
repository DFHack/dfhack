# don't use this file directly. use the one in the root folder of the project

SET(PROJECT_HDRS
DFCommon.h
DFDataModel.h
DFHackAPI.h
DFHackAPIc.h
DFMemAccess.h
DFMemInfo.h
DFProcessManager.h
DFTileTypes.h
DFTypes.h
DFVector.h
integers.h
stdint_win.h
LinuxMemAccess-memfiles.h
LinuxMemAccess-ptrace.h
WindowsMemAccess.h

md5/md5.h
md5/md5wrapper.h

tinyxml/tinystr.h
tinyxml/tinyxml.h
)

SET(PROJECT_SRCS
DFDataModel.cpp
DFMemInfo.cpp
DFProcess.cpp
DFProcessManager.cpp
DFHackAPI.cpp
DFHackAPIc.cpp
DFTileTypes.cpp
md5/md5.cpp
md5/md5wrapper.cpp
tinyxml/tinystr.cpp
tinyxml/tinyxml.cpp
tinyxml/tinyxmlerror.cpp
tinyxml/tinyxmlparser.cpp
)

SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE )

LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

IF(UNIX)
  add_definitions(-DLINUX_BUILD)
ELSE(UNIX)
  SET(PROJECT_LIBS psapi)
ENDIF(UNIX)

ADD_LIBRARY(dfhack SHARED ${PROJECT_SRCS})

SET_TARGET_PROPERTIES( dfhack  PROPERTIES DEBUG_POSTFIX "-debug" )

TARGET_LINK_LIBRARIES(dfhack ${PROJECT_LIBS})