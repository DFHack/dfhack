include_directories(include)
include_directories("${dfhack_SOURCE_DIR}/library/depends/tthread")
FILE(GLOB DFUSION_CPPS src/*.c*)
set(
    DFUSION_CPPS_ALL
    dfusion.cpp
    ${DFUSION_CPPS}
)
FILE(GLOB DFUSION_HS include/*)
SET_SOURCE_FILES_PROPERTIES( ${DFUSION_HS} PROPERTIES HEADER_FILE_ONLY TRUE )
DFHACK_PLUGIN(dfusion  ${DFUSION_CPPS_ALL} ${DFUSION_HS} LINK_LIBRARIES lua dfhack-tinythread)

# installs into DF root
install(DIRECTORY luafiles/ DESTINATION dfusion)