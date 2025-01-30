if(UNIX)
    if(NOT APPLE)
        # Linux: Check for unresolved symbols at link time
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-z,defs")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-z,defs")
    endif()
endif()

macro(car var)
    set(${var} ${ARGV1})
endmacro()

macro(cdr var junk)
    set(${var} ${ARGN})
endmacro()

macro(list_contains var value)
    set(${var})
    foreach(value2 ${ARGN})
        if(${value} STREQUAL ${value2})
            set(${var} TRUE)
        endif()
    endforeach()
endmacro()

macro(parse_arguments prefix arg_names option_names)
    set(DEFAULT_ARGS)
    foreach(arg_name ${arg_names})
        set(${prefix}_${arg_name})
    endforeach()

    foreach(option ${option_names})
        set(${prefix}_${option} FALSE)
    endforeach()

    set(current_arg_name DEFAULT_ARGS)
    set(current_arg_list)
    foreach(arg ${ARGN})
        list_contains(is_arg_name ${arg} ${arg_names})
        if(is_arg_name)
            set(${prefix}_${current_arg_name} ${current_arg_list})
            set(current_arg_name ${arg})
            set(current_arg_list)
        else()
            list_contains(is_option ${arg} ${option_names})
            if(is_option)
                set(${prefix}_${arg} TRUE)
            else()
                set(current_arg_list ${current_arg_list} ${arg})
            endif()
        endif()
    endforeach()
    set(${prefix}_${current_arg_name} ${current_arg_list})
endmacro()

macro(dfhack_plugin)
    parse_arguments(PLUGIN
        "LINK_LIBRARIES;DEPENDS;PROTOBUFS;COMPILE_FLAGS;COMPILE_FLAGS_GCC;COMPILE_FLAGS_MSVC"
        "SOME_OPT"
        ${ARGN}
    )
    car(PLUGIN_NAME ${PLUGIN_DEFAULT_ARGS})
    cdr(PLUGIN_SOURCES ${PLUGIN_DEFAULT_ARGS})

    set(PLUGIN_PROTOS)
    foreach(pbuf ${PLUGIN_PROTOBUFS})
        list(APPEND PLUGIN_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/${pbuf}.proto)
    endforeach()

    list(LENGTH PLUGIN_PROTOS NUM_PROTO)
    if(NUM_PROTO)
        string(REPLACE ".proto" ".pb.cc" PLUGIN_PROTO_SRCS "${PLUGIN_PROTOS}")
        string(REPLACE ".proto" ".pb.h" PLUGIN_PROTO_HDRS "${PLUGIN_PROTOS}")
        string(REPLACE "/proto/" "/proto/tmp/" PLUGIN_PROTO_TMP_FILES "${PLUGIN_PROTO_SRCS};${PLUGIN_PROTO_HDRS}")
        set_source_files_properties(${PLUGIN_PROTO_SRCS} ${PLUGIN_PROTO_HDRS} PROPERTIES GENERATED TRUE)

        # Force a re-gen if any *.pb.* files are missing
        # (only runs when cmake is run, but better than nothing)
        foreach(file IN LISTS PLUGIN_PROTO_SRCS PLUGIN_PROTO_HDRS)
            if(NOT EXISTS ${file})
                # MESSAGE("Resetting generate_proto_${PLUGIN_NAME} because '${file}' is missing")
                file(REMOVE ${PLUGIN_PROTO_TMP_FILES})
                break()
            endif()
        endforeach()

        add_custom_command(
            OUTPUT ${PLUGIN_PROTO_TMP_FILES}
            COMMAND protoc-bin -I=${CMAKE_CURRENT_SOURCE_DIR}/proto/
                --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto/tmp/
                ${PLUGIN_PROTOS}
            COMMAND ${PERL_EXECUTABLE} ${dfhack_SOURCE_DIR}/depends/copy-if-different.pl
                ${PLUGIN_PROTO_TMP_FILES}
                ${CMAKE_CURRENT_SOURCE_DIR}/proto/
            COMMENT "Generating plugin ${PLUGIN_NAME} protobufs"
            DEPENDS protoc-bin ${PLUGIN_PROTOS}
        )

        if(UNIX)
            set_source_files_properties(${PLUGIN_PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-misleading-indentation")
        endif()

        add_custom_target(generate_proto_${PLUGIN_NAME} DEPENDS ${PLUGIN_PROTO_TMP_FILES})

        # Merge headers into sources
        set_source_files_properties( ${PLUGIN_PROTO_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE )
        list(APPEND PLUGIN_SOURCES ${PLUGIN_PROTO_HDRS})
        list(APPEND PLUGIN_SOURCES ${PLUGIN_PROTO_SRCS})
        list(APPEND PLUGIN_SOURCES ${PLUGIN_PROTOS})

        if(UNIX)
            set(PLUGIN_COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS} -include Export.h")
        else()
            set(PLUGIN_COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS} /FI\"Export.h\"")
        endif()
    endif()

    if(BUILD_LIBRARY AND BUILD_PLUGINS)
        add_library(${PLUGIN_NAME} MODULE ${PLUGIN_SOURCES})
        ide_folder(${PLUGIN_NAME} "Plugins")

        if(NUM_PROTO)
            add_dependencies(${PLUGIN_NAME} generate_proto_${PLUGIN_NAME})
            target_include_directories(${PLUGIN_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/proto")
            target_link_libraries(${PLUGIN_NAME} protobuf-lite)
        endif()
        target_link_libraries(${PLUGIN_NAME} dfhack dfhack-version ${PLUGIN_LINK_LIBRARIES})

        if(UNIX)
            set(PLUGIN_COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS} ${PLUGIN_COMPILE_FLAGS_GCC}")
        else()
            set(PLUGIN_COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS} ${PLUGIN_COMPILE_FLAGS_MSVC}")
        endif()
        set_target_properties(${PLUGIN_NAME} PROPERTIES COMPILE_FLAGS "${PLUGIN_COMPILE_FLAGS}")

        if(APPLE)
            set_target_properties(${PLUGIN_NAME} PROPERTIES SUFFIX .plug.dylib PREFIX "")
        elseif(UNIX)
            set_target_properties(${PLUGIN_NAME} PROPERTIES SUFFIX .plug.so PREFIX "")
        else()
            set_target_properties(${PLUGIN_NAME} PROPERTIES SUFFIX .plug.dll)
        endif()

        install(TARGETS ${PLUGIN_NAME}
            LIBRARY DESTINATION ${DFHACK_PLUGIN_DESTINATION}
            RUNTIME DESTINATION ${DFHACK_PLUGIN_DESTINATION})
    endif()
endmacro()
