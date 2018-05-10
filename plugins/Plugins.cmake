IF(UNIX)
  add_definitions(-DLINUX_BUILD)
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
  SET(CMAKE_CXX_FLAGS "-fvisibility=hidden -std=c++0x")
  SET(CMAKE_C_FLAGS "-fvisibility=hidden")
  IF(DFHACK_BUILD_64)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -mno-avx")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -mno-avx")
    IF(NOT APPLE)
      # Linux: Check for unresolved symbols at link time
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-z,defs")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-z,defs")
    ENDIF()
  ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
  ENDIF()
ENDIF()

include_directories("${dfhack_SOURCE_DIR}/library/include")
include_directories("${dfhack_SOURCE_DIR}/library/proto")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/proto")
include_directories("${dfhack_SOURCE_DIR}/library/depends/xgetopt")

MACRO(CAR var)
  SET(${var} ${ARGV1})
ENDMACRO()

MACRO(CDR var junk)
  SET(${var} ${ARGN})
ENDMACRO()

MACRO(LIST_CONTAINS var value)
  SET(${var})
  FOREACH (value2 ${ARGN})
    IF (${value} STREQUAL ${value2})
      SET(${var} TRUE)
    ENDIF()
  ENDFOREACH()
ENDMACRO()

MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})
    SET(${prefix}_${arg_name})
  ENDFOREACH()

  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH()

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})
    LIST_CONTAINS(is_arg_name ${arg} ${arg_names})
    IF (is_arg_name)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE()
      LIST_CONTAINS(is_option ${arg} ${option_names})
      IF(is_option)
        SET(${prefix}_${arg} TRUE)
      ELSE()
        SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF()
    ENDIF()
  ENDFOREACH()
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO()

MACRO(DFHACK_PLUGIN)
  PARSE_ARGUMENTS(PLUGIN
    "LINK_LIBRARIES;DEPENDS;PROTOBUFS;COMPILE_FLAGS;COMPILE_FLAGS_GCC;COMPILE_FLAGS_MSVC"
    "SOME_OPT"
    ${ARGN}
    )
  CAR(PLUGIN_NAME ${PLUGIN_DEFAULT_ARGS})
  CDR(PLUGIN_SOURCES ${PLUGIN_DEFAULT_ARGS})

  SET(PLUGIN_PROTOS)
  FOREACH(pbuf ${PLUGIN_PROTOBUFS})
    LIST(APPEND PLUGIN_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/${pbuf}.proto)
  ENDFOREACH()

  LIST(LENGTH PLUGIN_PROTOS NUM_PROTO)
  IF(NUM_PROTO)
    STRING(REPLACE ".proto" ".pb.cc" PLUGIN_PROTO_SRCS "${PLUGIN_PROTOS}")
    STRING(REPLACE ".proto" ".pb.h" PLUGIN_PROTO_HDRS "${PLUGIN_PROTOS}")
    STRING(REPLACE "/proto/" "/proto/tmp/" PLUGIN_PROTO_TMP_FILES "${PLUGIN_PROTO_SRCS};${PLUGIN_PROTO_HDRS}")
    SET_SOURCE_FILES_PROPERTIES(${PLUGIN_PROTO_SRCS} ${PLUGIN_PROTO_HDRS} PROPERTIES GENERATED TRUE)

    # Force a re-gen if any *.pb.* files are missing
    # (only runs when cmake is run, but better than nothing)
    FOREACH(file IN LISTS PLUGIN_PROTO_SRCS PLUGIN_PROTO_HDRS)
      IF(NOT EXISTS ${file})
        # MESSAGE("Resetting generate_proto_${PLUGIN_NAME} because '${file}' is missing")
        FILE(REMOVE ${PLUGIN_PROTO_TMP_FILES})
        BREAK()
      ENDIF()
    ENDFOREACH()

    ADD_CUSTOM_COMMAND(
        OUTPUT ${PLUGIN_PROTO_TMP_FILES}
        COMMAND protoc-bin -I=${CMAKE_CURRENT_SOURCE_DIR}/proto/
                --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto/tmp/
                ${PLUGIN_PROTOS}
        COMMAND ${PERL_EXECUTABLE} ${CMAKE_SOURCE_DIR}/depends/copy-if-different.pl
                ${PLUGIN_PROTO_TMP_FILES}
                ${CMAKE_CURRENT_SOURCE_DIR}/proto/
        COMMENT "Generating plugin ${PLUGIN_NAME} protobufs"
        DEPENDS protoc-bin ${PLUGIN_PROTOS}
    )

    IF(UNIX)
      SET_SOURCE_FILES_PROPERTIES(${PLUGIN_PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-misleading-indentation")
    ENDIF()

    ADD_CUSTOM_TARGET(generate_proto_${PLUGIN_NAME} DEPENDS ${PLUGIN_PROTO_TMP_FILES})

    # Merge headers into sources
    SET_SOURCE_FILES_PROPERTIES( ${PLUGIN_PROTO_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE )
    LIST(APPEND PLUGIN_SOURCES ${PLUGIN_PROTO_HDRS})
    LIST(APPEND PLUGIN_SOURCES ${PLUGIN_PROTO_SRCS})

    IF(UNIX)
      SET(PLUGIN_COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS} -include Export.h")
    ELSE()
      SET(PLUGIN_COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS} /FI\"Export.h\"")
    ENDIF()
  ENDIF()

  ADD_LIBRARY(${PLUGIN_NAME} MODULE ${PLUGIN_SOURCES})
  IDE_FOLDER(${PLUGIN_NAME} "Plugins")

  IF(NUM_PROTO)
    ADD_DEPENDENCIES(${PLUGIN_NAME} generate_proto_${PLUGIN_NAME})
    TARGET_LINK_LIBRARIES(${PLUGIN_NAME} dfhack protobuf-lite dfhack-version ${PLUGIN_LINK_LIBRARIES})
  ELSE()
    TARGET_LINK_LIBRARIES(${PLUGIN_NAME} dfhack dfhack-version ${PLUGIN_LINK_LIBRARIES})
  ENDIF()

  ADD_DEPENDENCIES(${PLUGIN_NAME} dfhack-version)

  # Make sure the source is generated before the executable builds.
  ADD_DEPENDENCIES(${PLUGIN_NAME} generate_proto)

  IF(UNIX)
    SET(PLUGIN_COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS} ${PLUGIN_COMPILE_FLAGS_GCC}")
  ELSE()
    SET(PLUGIN_COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS} ${PLUGIN_COMPILE_FLAGS_MSVC}")
  ENDIF()
  SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS}")

  IF(APPLE)
    SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES SUFFIX .plug.dylib PREFIX "")
  ELSEIF(UNIX)
    SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES SUFFIX .plug.so PREFIX "")
  ELSE()
    SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES SUFFIX .plug.dll)
  ENDIF()

  install(TARGETS ${PLUGIN_NAME}
          LIBRARY DESTINATION ${DFHACK_PLUGIN_DESTINATION}
          RUNTIME DESTINATION ${DFHACK_PLUGIN_DESTINATION})
ENDMACRO(DFHACK_PLUGIN)
