if(NOT EXISTS ${dfhack_SOURCE_DIR}/.git/index OR NOT EXISTS ${dfhack_SOURCE_DIR}/.git/modules/library/xml/index)
    MESSAGE(FATAL_ERROR "Could not find git index file(s)")
endif()

set(git_describe_tmp_h ${dfhack_SOURCE_DIR}/library/include/git-describe.tmp.h)
set(git_describe_h ${dfhack_SOURCE_DIR}/library/include/git-describe.h)

if(EXISTS ${git_describe_tmp_h} AND
        NOT(${dfhack_SOURCE_DIR}/.git/index IS_NEWER_THAN ${git_describe_tmp_h}) AND
        NOT(${dfhack_SOURCE_DIR}/.git/modules/library/xml/index IS_NEWER_THAN ${git_describe_tmp_h}))
    return()
endif()

execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=8 --long
    WORKING_DIRECTORY "${dfhack_SOURCE_DIR}"
    OUTPUT_VARIABLE DFHACK_GIT_DESCRIPTION)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY "${dfhack_SOURCE_DIR}"
    OUTPUT_VARIABLE DFHACK_GIT_COMMIT)
execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=8 --exact-match
    WORKING_DIRECTORY "${dfhack_SOURCE_DIR}"
    RESULT_VARIABLE DFHACK_GIT_TAGGED_RESULT
    OUTPUT_QUIET ERROR_QUIET)

# Check library/xml for matching commits
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD:library/xml
    WORKING_DIRECTORY "${dfhack_SOURCE_DIR}"
    OUTPUT_VARIABLE DFHACK_GIT_XML_EXPECTED_COMMIT)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY "${dfhack_SOURCE_DIR}/library/xml"
    OUTPUT_VARIABLE DFHACK_GIT_XML_COMMIT)

file(WRITE ${git_describe_tmp_h} "")

macro(git_describe_definition var)
    string(STRIP "${${var}}" GIT_DEF_VALUE)
    file(APPEND ${git_describe_tmp_h} "#define ${var} \"${GIT_DEF_VALUE}\"\n")
endmacro()

git_describe_definition(DFHACK_GIT_DESCRIPTION)
git_describe_definition(DFHACK_GIT_COMMIT)
git_describe_definition(DFHACK_GIT_XML_EXPECTED_COMMIT)
git_describe_definition(DFHACK_GIT_XML_COMMIT)
if(${DFHACK_GIT_TAGGED_RESULT} EQUAL 0)
    file(APPEND ${git_describe_tmp_h} "#define DFHACK_GIT_TAGGED\n")
endif()
if(${DFHACK_GIT_XML_COMMIT} STREQUAL ${DFHACK_GIT_XML_EXPECTED_COMMIT})
    file(APPEND ${git_describe_tmp_h} "#define DFHACK_GIT_XML_MATCH\n")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${git_describe_tmp_h} ${git_describe_h})
