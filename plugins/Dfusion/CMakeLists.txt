#FIXME: inherit all macros and stuff from the dfhack SDK
find_package(Lua51 QUIET)
if(LUA51_FOUND)
	include_directories(${LUA_INCLUDE_DIR})
	
	DFHACK_PLUGIN(dfusion dfusion.cpp)
	target_link_libraries(dfusion ${LUA_LIBRARIES})
else(LUA51_FOUND)
	MESSAGE(STATUS "Required libraries (lua51) not found - dfusion plugin can't be built.")
endif(LUA51_FOUND)
