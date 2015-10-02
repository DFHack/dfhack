include(../plugins/Plugins.cmake)

MACRO(DFHACK_SCRIPTS)
  PARSE_ARGUMENTS(SCRIPT
    "SUBDIRECTORY"
    "SOME_OPT"
    ${ARGN}
  )
  CAR(SCRIPT_SUBDIRECTORY ${SCRIPT_SUBDIRECTORY})
  install(FILES ${SCRIPT_DEFAULT_ARGS}
    DESTINATION ${DFHACK_DATA_DESTINATION}/scripts/${SCRIPT_SUBDIRECTORY})
ENDMACRO()

MACRO(DFHACK_3RDPARTY_SCRIPT_REPO repo_path)
  if(NOT EXISTS ${dfhack_SOURCE_DIR}/scripts/3rdparty/${repo_path}/CMakeLists.txt)
    MESSAGE(SEND_ERROR "Script submodule scripts/3rdparty/${repo_path} does not exist - run `git submodule update --init`.")
  endif()
  add_subdirectory(3rdparty/${repo_path})
ENDMACRO()
