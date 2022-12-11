project(protobuf)

set(HASH_MAP_H <unordered_map>)
set(HASH_SET_H <unordered_map>)
set(HASH_NAMESPACE std)
set(HASH_MAP_CLASS unordered_map)
set(HASH_SET_CLASS unordered_set)
set(HAVE_HASH_MAP 1)
set(HAVE_HASH_SET 1)

if(HAVE_HASH_MAP EQUAL 0)
    message(SEND_ERROR "Could not find a working hash map implementation. Please install GCC >= 4.4, and all necessary 32-bit C++ development libraries.")
endif()

if(UNIX)
    find_package(Threads REQUIRED)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")

set(LIBPROTOBUF_LITE_HDRS
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

set(LIBPROTOBUF_FULL_HDRS
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

list(APPEND LIBPROTOBUF_FULL_HDRS ${LIBPROTOBUF_LITE_HDRS})

set(LIBPROTOBUF_LITE_SRCS
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

set(LIBPROTOBUF_FULL_SRCS
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

set(LIBPROTOC_HDRS
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

set(LIBPROTOC_SRCS
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

list(APPEND LIBPROTOBUF_FULL_SRCS ${LIBPROTOBUF_LITE_SRCS})

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result -Wno-unused-local-typedefs -Wno-misleading-indentation -Wno-class-memaccess -Wno-sign-compare")
elseif(MSVC)
    # Disable warnings for integer conversion to smaller type
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4273")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(PROTOBUF_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Protobuf shared libraries

add_library(protobuf-lite SHARED ${LIBPROTOBUF_LITE_SRCS} ${LIBPROTOBUF_LITE_HDRS})
ide_folder(protobuf-lite "Depends")

set_target_properties(protobuf-lite PROPERTIES COMPILE_DEFINITIONS LIBPROTOBUF_EXPORTS)

target_link_libraries(protobuf-lite ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES})

install(TARGETS protobuf-lite
    LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
    RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})

if(NOT CMAKE_CROSSCOMPILING)
    add_library(protobuf SHARED ${LIBPROTOBUF_FULL_SRCS} ${LIBPROTOBUF_FULL_HDRS})
    ide_folder(protobuf "Depends")
    set_target_properties(protobuf PROPERTIES COMPILE_DEFINITIONS LIBPROTOBUF_EXPORTS)
    target_link_libraries(protobuf ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES})

    # Protobuf compiler shared library

    add_library(protoc SHARED ${LIBPROTOC_SRCS} ${LIBPROTOC_HDRS})
    ide_folder(protoc "Depends")

    set_target_properties(protoc PROPERTIES COMPILE_DEFINITIONS LIBPROTOC_EXPORTS)
    target_link_libraries(protoc protobuf)

    # Protobuf compiler executable

    add_executable(protoc-bin google/protobuf/compiler/main.cc google/protobuf/compiler/command_line_interface.h google/protobuf/compiler/cpp/cpp_generator.h)
    ide_folder(protoc-bin "Depends")

    set_target_properties(protoc-bin PROPERTIES OUTPUT_NAME protoc)
    target_link_libraries(protoc-bin protoc)

    export(TARGETS protoc-bin FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake )
endif()
