include_directories(include)
include_directories("${dfhack_SOURCE_DIR}/library/depends/tthread")
FILE(GLOB DFUSION_CPPS src/*.c*)
set(
    DFUSION_CPPS_ALL
    dfusion.cpp
    ${DFUSION_CPPS}
)
DFHACK_PLUGIN(dfusion  ${DFUSION_CPPS_ALL} LINK_LIBRARIES lua)
# installs into DF root
install(DIRECTORY luafiles/ DESTINATION dfusion)
