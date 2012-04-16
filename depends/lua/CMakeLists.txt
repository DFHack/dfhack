PROJECT ( lua CXX )
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DLUA_USE_APICHECK")

IF(WIN32)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE )
ELSE()
    ADD_DEFINITIONS ( -DLUA_USE_POSIX -DLUA_USE_DLOPEN )
    SET ( LIBS m dl )
ENDIF()

IF(UNIX)
  add_definitions(-DLINUX_BUILD)
  SET(CMAKE_C_FLAGS "-m32")
ENDIF()

SET (HDR_LIBLUA
include/lapi.h
include/lauxlib.h
include/lcode.h
include/lctype.h
include/ldebug.h
include/ldo.h
include/lfunc.h
include/lgc.h
include/llex.h
include/llimits.h
include/lmem.h
include/lobject.h
include/lopcodes.h
include/lparser.h
include/lstate.h
include/lstring.h
include/ltable.h
include/ltm.h
include/lua.h
include/luaconf.h
include/lualib.h
include/lundump.h
include/lvm.h
include/lzio.h
)
SET_SOURCE_FILES_PROPERTIES(${HDR_LIBLUA} PROPERTIES HEADER_FILE_ONLY TRUE)
include_directories(include)

# Build Libraries
SET (SRC_LIBLUA
src/lapi.c
src/lauxlib.c
src/lbaselib.c
src/lbitlib.c
src/lcode.c
src/lcorolib.c
src/lctype.c
src/ldblib.c
src/ldebug.c
src/ldo.c
src/ldump.c
src/lfunc.c
src/lgc.c
src/linit.c
src/liolib.c
src/llex.c
src/lmathlib.c
src/lmem.c
src/loadlib.c
src/lobject.c
src/lopcodes.c
src/loslib.c
src/lparser.c
src/lstate.c
src/lstring.c
src/lstrlib.c
src/ltable.c
src/ltablib.c
src/ltm.c
src/lundump.c
src/lvm.c
src/lzio.c
)
# compile with C++ compiler
set_source_files_properties(${SRC_LIBLUA} PROPERTIES LANGUAGE CXX)
# append headers to sources to make them show up in MSVC GUI
LIST(APPEND SRC_LIBLUA ${HDR_LIBLUA})

ADD_LIBRARY ( lua SHARED ${SRC_LIBLUA} )
TARGET_LINK_LIBRARIES ( lua ${LIBS})

install(TARGETS lua
        LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
        RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})

IDE_FOLDER(lua "Depends")

#SET ( SRC_LUA src/lua.c )
#SET ( SRC_LUAC src/luac.c src/print.c )

#ADD_EXECUTABLE ( lua ${SRC_LUA} ${LUA_RC_FILE})
#ADD_EXECUTABLE ( luac ${SRC_LUAC} ${LUAC_RC_FILE})
#TARGET_LINK_LIBRARIES ( lua liblua )
#TARGET_LINK_LIBRARIES ( luac liblua_static )

