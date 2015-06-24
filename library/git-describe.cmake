execute_process(COMMAND git describe --tags
    WORKING_DIRECTORY "${dfhack_SOURCE_DIR}"
    OUTPUT_VARIABLE DFHACK_GIT_DESCRIPTION)
string(STRIP ${DFHACK_GIT_DESCRIPTION} DFHACK_GIT_DESCRIPTION)
file(WRITE ${dfhack_SOURCE_DIR}/library/include/git-describe.tmp.h
    "#define DFHACK_GIT_DESCRIPTION \"${DFHACK_GIT_DESCRIPTION}\"")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${dfhack_SOURCE_DIR}/library/include/git-describe.tmp.h
    ${dfhack_SOURCE_DIR}/library/include/git-describe.h)
