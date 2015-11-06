PROJECT(protobuf)

#Protocol buffers use C++0x hash maps, so we need to look for those. This is a rewrite of stl_hash.m4 in CMake.
IF(CMAKE_COMPILER_IS_GNUCC)
    EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION)
    STRING(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
    LIST(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
    LIST(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)
    #IF(GCC_MAJOR LESS 4 OR (GCC_MAJOR EQUAL 4 AND GCC_MINOR LESS 2))
        #GCC is too old
    #    SET(STL_HASH_OLD_GCC 1)
    #ENDIF()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    SET(HAVE_HASH_MAP 0)
    SET(HASH_MAP_CLASS unordered_map)

    #Check for all of the possible combinations of unordered_map and hash_map

    FOREACH(header tr1/unordered_map unordered_map)
        FOREACH(namespace std::tr1 std )
            IF(HAVE_HASH_MAP EQUAL 0 AND NOT STL_HASH_OLD_GCC)
                CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/testHashMap.cpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/testHashMap.cpp")
                TRY_RUN(HASH_MAP_RUN_RESULT HASH_MAP_COMPILE_RESULT ${PROJECT_BINARY_DIR}/CMakeTmp "${CMAKE_CURRENT_SOURCE_DIR}/testHashMap.cpp")
                IF (HASH_MAP_COMPILE_RESULT AND HASH_MAP_RUN_RESULT EQUAL 1)
                    SET(HASH_MAP_H <${header}>)
                    STRING(REPLACE "map" "set" HASH_SET_H ${HASH_MAP_H})
                    SET(HASH_NAMESPACE ${namespace})
                    SET(HASH_MAP_CLASS unordered_map)
                    SET(HASH_SET_CLASS unordered_set)
                    SET(HAVE_HASH_MAP 1)
                    SET(HAVE_HASH_SET 1)
                ENDIF()
            ENDIF()
        ENDFOREACH(namespace)
    ENDFOREACH(header)
    IF (HAVE_HASH_MAP EQUAL 0)
        SET(HASH_MAP_CLASS hash_map)
        FOREACH(header ext/hash_map hash_map)
            FOREACH(namespace __gnu_cxx "" std stdext)
                IF (HAVE_HASH_MAP EQUAL 0)
                    CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/testHashMap.cpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/testHashMap.cpp")
                    TRY_COMPILE(HASH_MAP_COMPILE_RESULT ${PROJECT_BINARY_DIR}/CMakeTmp "${CMAKE_CURRENT_SOURCE_DIR}/testHashMap.cpp")
                    IF (HASH_MAP_COMPILE_RESULT)
                        SET(HASH_MAP_H <${header}>)
                        STRING(REPLACE "map" "set" HASH_SET_H ${HASH_MAP_H})
                        SET(HASH_NAMESPACE ${namespace})
                        SET(HASH_MAP_CLASS hash_map)
                        SET(HASH_SET_CLASS hash_set)
                        SET(HAVE_HASH_MAP 1)
                        SET(HAVE_HASH_SET 1)
                    ENDIF()
                ENDIF()
            ENDFOREACH()
        ENDFOREACH()
    ENDIF()

    IF (HAVE_HASH_MAP EQUAL 0)
        MESSAGE(SEND_ERROR "Could not find a working hash map implementation. Please install GCC >= 4.4, and all necessary 32-bit C++ development libraries.")
    ENDIF()

    FIND_PACKAGE(Threads)

ELSE()
    SET(HASH_MAP_H <hash_map>)
    SET(HASH_NAMESPACE std)
    SET(HASH_SET_H <hash_set>)
    SET(HAVE_HASH_MAP 1)
    SET(HAVE_HASH_SET 1)
    SET(HASH_MAP_CLASS hash_map)
    SET(HASH_SET_CLASS hash_set)
ENDIF()

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")

SET(LIBPROTOBUF_LITE_HDRS
google/protobuf/io/coded_stream.h
google/protobuf/io/coded_stream_inl.h
google/protobuf/stubs/common.h
${CMAKE_CURRENT_BINARY_DIR}/config.h
google/protobuf/extension_set.h
google/protobuf/generated_message_util.h
google/protobuf/stubs/hash.h
google/protobuf/stubs/map-util.h
google/protobuf/message_lite.h
google/protobuf/stubs/once.h
google/protobuf/repeated_field.h
google/protobuf/stubs/stl_util-inl.h
google/protobuf/wire_format_lite.h
google/protobuf/wire_format_lite_inl.h
google/protobuf/io/zero_copy_stream.h
google/protobuf/io/zero_copy_stream_impl_lite.h
google/protobuf/io/gzip_stream.h
google/protobuf/io/zero_copy_stream_impl.h
)

SET(LIBPROTOBUF_FULL_HDRS
google/protobuf/descriptor.h
google/protobuf/descriptor.pb.h
google/protobuf/descriptor_database.h
google/protobuf/dynamic_message.h
google/protobuf/generated_message_reflection.h
google/protobuf/compiler/importer.h
google/protobuf/message.h
google/protobuf/compiler/parser.h
google/protobuf/io/printer.h
google/protobuf/reflection_ops.h
google/protobuf/service.h
google/protobuf/stubs/strutil.h
google/protobuf/stubs/substitute.h
google/protobuf/text_format.h
google/protobuf/io/tokenizer.h
google/protobuf/unknown_field_set.h
google/protobuf/wire_format.h
)

LIST(APPEND LIBPROTOBUF_FULL_HDRS ${LIBPROTOBUF_LITE_HDRS})

SET(LIBPROTOBUF_LITE_SRCS
google/protobuf/io/coded_stream.cc
google/protobuf/stubs/common.cc
google/protobuf/extension_set.cc
google/protobuf/generated_message_util.cc
google/protobuf/message_lite.cc
google/protobuf/stubs/once.cc
google/protobuf/repeated_field.cc
google/protobuf/wire_format_lite.cc
google/protobuf/io/zero_copy_stream.cc
google/protobuf/io/zero_copy_stream_impl_lite.cc
google/protobuf/io/gzip_stream.cc
google/protobuf/io/zero_copy_stream_impl.cc
)

SET(LIBPROTOBUF_FULL_SRCS
google/protobuf/descriptor.cc
google/protobuf/descriptor.pb.cc
google/protobuf/descriptor_database.cc
google/protobuf/dynamic_message.cc
google/protobuf/extension_set_heavy.cc
google/protobuf/generated_message_reflection.cc
google/protobuf/compiler/importer.cc
google/protobuf/io/gzip_stream.cc
google/protobuf/message.cc
google/protobuf/compiler/parser.cc
google/protobuf/io/printer.cc
google/protobuf/reflection_ops.cc
google/protobuf/service.cc
google/protobuf/stubs/structurally_valid.cc
google/protobuf/stubs/strutil.cc
google/protobuf/stubs/substitute.cc
google/protobuf/text_format.cc
google/protobuf/io/tokenizer.cc
google/protobuf/unknown_field_set.cc
google/protobuf/wire_format.cc
google/protobuf/io/zero_copy_stream_impl.cc
)

SET(LIBPROTOC_HDRS
google/protobuf/compiler/code_generator.h
google/protobuf/compiler/command_line_interface.h
google/protobuf/compiler/cpp/cpp_enum.h
google/protobuf/compiler/cpp/cpp_enum_field.h
google/protobuf/compiler/cpp/cpp_extension.h
google/protobuf/compiler/cpp/cpp_field.h
google/protobuf/compiler/cpp/cpp_file.h
google/protobuf/compiler/cpp/cpp_generator.h
google/protobuf/compiler/cpp/cpp_helpers.h
google/protobuf/compiler/cpp/cpp_message.h
google/protobuf/compiler/cpp/cpp_message_field.h
google/protobuf/compiler/cpp/cpp_primitive_field.h
google/protobuf/compiler/cpp/cpp_service.h
google/protobuf/compiler/cpp/cpp_string_field.h
google/protobuf/compiler/plugin.h
google/protobuf/compiler/plugin.pb.h
google/protobuf/compiler/subprocess.h
google/protobuf/compiler/zip_writer.h
)

SET(LIBPROTOC_SRCS
google/protobuf/compiler/code_generator.cc
google/protobuf/compiler/command_line_interface.cc
google/protobuf/compiler/cpp/cpp_enum.cc
google/protobuf/compiler/cpp/cpp_enum_field.cc
google/protobuf/compiler/cpp/cpp_extension.cc
google/protobuf/compiler/cpp/cpp_field.cc
google/protobuf/compiler/cpp/cpp_file.cc
google/protobuf/compiler/cpp/cpp_generator.cc
google/protobuf/compiler/cpp/cpp_helpers.cc
google/protobuf/compiler/cpp/cpp_message.cc
google/protobuf/compiler/cpp/cpp_message_field.cc
google/protobuf/compiler/cpp/cpp_primitive_field.cc
google/protobuf/compiler/cpp/cpp_service.cc
google/protobuf/compiler/cpp/cpp_string_field.cc
google/protobuf/compiler/plugin.cc
google/protobuf/compiler/plugin.pb.cc
google/protobuf/compiler/subprocess.cc
google/protobuf/compiler/zip_writer.cc
)

LIST(APPEND LIBPROTOBUF_FULL_SRCS ${LIBPROTOBUF_LITE_SRCS})

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wno-sign-compare")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result")
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET(PROTOBUF_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Protobuf shared libraries

ADD_LIBRARY(protobuf SHARED ${LIBPROTOBUF_FULL_SRCS} ${LIBPROTOBUF_FULL_HDRS})
IDE_FOLDER(protobuf "Depends")
ADD_LIBRARY(protobuf-lite SHARED ${LIBPROTOBUF_LITE_SRCS} ${LIBPROTOBUF_LITE_HDRS})
IDE_FOLDER(protobuf-lite "Depends")

SET_TARGET_PROPERTIES(protobuf PROPERTIES COMPILE_DEFINITIONS LIBPROTOBUF_EXPORTS)
SET_TARGET_PROPERTIES(protobuf-lite PROPERTIES COMPILE_DEFINITIONS LIBPROTOBUF_EXPORTS)

TARGET_LINK_LIBRARIES(protobuf ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES})
TARGET_LINK_LIBRARIES(protobuf-lite ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES})

install(TARGETS protobuf-lite
        LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
        RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})

# Protobuf compiler shared library

ADD_LIBRARY(protoc SHARED ${LIBPROTOC_SRCS} ${LIBPROTOC_HDRS})
IDE_FOLDER(protoc "Depends")

SET_TARGET_PROPERTIES(protoc PROPERTIES COMPILE_DEFINITIONS LIBPROTOC_EXPORTS)
TARGET_LINK_LIBRARIES(protoc protobuf)

# Protobuf compiler executable

ADD_EXECUTABLE(protoc-bin google/protobuf/compiler/main.cc google/protobuf/compiler/command_line_interface.h google/protobuf/compiler/cpp/cpp_generator.h)
IDE_FOLDER(protoc-bin "Depends")

SET_TARGET_PROPERTIES(protoc-bin PROPERTIES OUTPUT_NAME protoc)
TARGET_LINK_LIBRARIES(protoc-bin protoc)
