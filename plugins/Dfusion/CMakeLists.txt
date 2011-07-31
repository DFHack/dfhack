find_package(Lua51 QUIET)

if(LUA51_FOUND)
	include_directories(${LUA_INCLUDE_DIR} include)
	include_directories("${dfhack_SOURCE_DIR}/library/depends/tthread")
	FILE(GLOB DFUSION_CPPS src/*.c*)
	set(
		DFUSION_CPPS_ALL
		dfusion.cpp
		${DFUSION_CPPS}
	)
	DFHACK_PLUGIN(dfusion  ${DFUSION_CPPS_ALL})
	target_link_libraries(dfusion ${LUA_LIBRARIES})
	install(DIRECTORY luafiles/ DESTINATION ${DFHACK_DATA_DESTINATION}/dfusion)
else(LUA51_FOUND)
	MESSAGE(STATUS "Required libraries (lua51) not found - dfusion plugin can't be built.")
endif(LUA51_FOUND)
