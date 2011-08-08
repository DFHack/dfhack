PROJECT ( lua C )
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF(WIN32)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE )
ELSE()
    ADD_DEFINITIONS ( -DLUA_USE_POSIX -DLUA_USE_DLOPEN )
    SET ( LIBS m dl )
ENDIF()

# Build Libraries
SET (SRC_LIBLUA
src/lapi.c
src/lcode.c
src/ldebug.c
src/ldo.c
src/ldump.c
src/lfunc.c
src/lgc.c
src/llex.c
src/lmem.c
src/lobject.c
src/lopcodes.c
src/lparser.c
src/lstate.c
src/lstring.c
src/ltable.c
src/ltm.c
src/lundump.c
src/lvm.c
src/lzio.c
src/lauxlib.c
src/lbaselib.c
src/ldblib.c
src/liolib.c
src/lmathlib.c
src/loslib.c
src/ltablib.c
src/lstrlib.c
src/loadlib.c
src/linit.c
)

ADD_LIBRARY ( lua ${SRC_LIBLUA} )
TARGET_LINK_LIBRARIES ( lua ${LIBS})

#SET ( SRC_LUA src/lua.c )
#SET ( SRC_LUAC src/luac.c src/print.c )

#ADD_EXECUTABLE ( lua ${SRC_LUA} ${LUA_RC_FILE})
#ADD_EXECUTABLE ( luac ${SRC_LUAC} ${LUAC_RC_FILE})
#TARGET_LINK_LIBRARIES ( lua liblua )
#TARGET_LINK_LIBRARIES ( luac liblua_static )

