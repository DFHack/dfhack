execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --long
    WORKING_DIRECTORY "${dfhack_SOURCE_DIR}"
    OUTPUT_VARIABLE DFHACK_GIT_DESCRIPTION)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY "${dfhack_SOURCE_DIR}"
    OUTPUT_VARIABLE DFHACK_GIT_COMMIT)
string(STRIP ${DFHACK_GIT_DESCRIPTION} DFHACK_GIT_DESCRIPTION)
file(WRITE ${dfhack_SOURCE_DIR}/library/include/git-describe.tmp.h
    "#define DFHACK_GIT_DESCRIPTION \"${DFHACK_GIT_DESCRIPTION}\"\n")
string(STRIP ${DFHACK_GIT_COMMIT} DFHACK_GIT_COMMIT)
file(APPEND ${dfhack_SOURCE_DIR}/library/include/git-describe.tmp.h
    "#define DFHACK_GIT_COMMIT \"${DFHACK_GIT_COMMIT}\"")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${dfhack_SOURCE_DIR}/library/include/git-describe.tmp.h
    ${dfhack_SOURCE_DIR}/library/include/git-describe.h)
