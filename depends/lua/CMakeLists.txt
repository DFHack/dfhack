project(lua CXX)
cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DLUA_USE_APICHECK")

# Make bit32 library available (for things like bit32.extract())
add_definitions(-DLUA_COMPAT_BITLIB)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE /wd4334)
else()
    add_definitions(-DLUA_USE_POSIX -DLUA_USE_DLOPEN)
    set(LIBS m dl)
endif()

if(UNIX)
    add_definitions(-DLINUX_BUILD)
    if(DFHACK_BUILD_64)
        set(CMAKE_C_FLAGS "-m64 -mno-avx")
    else()
        set(CMAKE_C_FLAGS "-m32")
    endif()
endif()

set(HDR_LIBLUA
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
set_source_files_properties(${HDR_LIBLUA} PROPERTIES HEADER_FILE_ONLY TRUE)
include_directories(include)

# Build Libraries
set(SRC_LIBLUA
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
    src/lutf8lib.c
    src/lvm.c
    src/lzio.c
)
# compile with C++ compiler
set_source_files_properties(${SRC_LIBLUA} PROPERTIES LANGUAGE CXX)
# append headers to sources to make them show up in MSVC GUI
list(APPEND SRC_LIBLUA ${HDR_LIBLUA})

add_library(lua SHARED ${SRC_LIBLUA})
target_link_libraries(lua ${LIBS})

if(MSVC)
    # need no space to prevent /FI from being stripped: https://github.com/DFHack/dfhack/issues/1455
    target_compile_options(lua PRIVATE "/FIdfhack_llimits.h")
else()
    target_compile_options(lua PRIVATE -include dfhack_llimits.h)
    set_source_files_properties(src/lstring.c PROPERTIES COMPILE_FLAGS "-Wno-stringop-overflow")
endif()

install(TARGETS lua
    LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
    RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})

ide_folder(lua "Depends")
