#FIXME: inherit all macros and stuff from the dfhack SDK
IF(UNIX)
    add_definitions(-DLINUX_BUILD)
    SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
    SET(CMAKE_CXX_FLAGS "-fvisibility=hidden -m32")
    SET(CMAKE_C_FLAGS "-fvisibility=hidden -m32")
ENDIF()

include_directories("${dfhack_SOURCE_DIR}/library/include")

MACRO(DFHACK_PLUGIN PLUGIN_NAME PLUGIN_SOURCES)
    ADD_LIBRARY(${PLUGIN_NAME} MODULE ${PLUGIN_SOURCES})
    TARGET_LINK_LIBRARIES(${PLUGIN_NAME} dfhack)
    IF(UNIX)
        SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES SUFFIX .plug.so PREFIX "")
    ELSE()
        SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES SUFFIX .plug.dll)
    ENDIF()
ENDMACRO()

DFHACK_PLUGIN(reveal reveal.cpp)
DFHACK_PLUGIN(kittens kittens.cpp)
DFHACK_PLUGIN(prospector prospector.cpp)
