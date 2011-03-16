# main project file. use it from a build sub-folder, see COMPILE for details

## some generic CMake magic
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
PROJECT (dfhack)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules)
# Set this to project source dir. When dfhack is used
# as a submodule, CMAKE_SOURCE_DIR is not pointing to
# the root where this particular CMakeLists.txt file sits.
SET(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIR})

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(SEND_ERROR "In-source builds are not allowed.")
endif()

## version - only set this for actual releases
SET (DFHACK_VERSION "dev" CACHE STRING "Version number of dfhack. Set to 'dev' by default. Only set for releases." )

## setting the build type
IF(NOT DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

## put everything in one big ugly directory to make MSVC and KDevelop happy
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

OPTION(BUILD_DFHACK_DOXYGEN "Create doxygen documentation for developers" ON)
OPTION(BUILD_DFHACK_EXAMPLES "Build example tools" OFF)
OPTION(BUILD_DFHACK_PLAYGROUND "Build tools from the playground folder" OFF)
OPTION(BUILD_DFHACK_C_BINDINGS "Build the C portion of the library" ON)
OPTION(BUILD_DFHACK_SUPPORTED "Build the supported toold." ON)

include_directories (${CMAKE_SOURCE_DIR}/library/include/)
include_directories (${CMAKE_SOURCE_DIR}/library/shm/)
include_directories (${CMAKE_SOURCE_DIR}/library/depends/argstream/)

add_subdirectory (library)

# macro to save on typing in the tool subdirs
MACRO(DFHACK_TOOL TOOL_NAME TOOL_SOURCES)
    ADD_EXECUTABLE(${TOOL_NAME} ${TOOL_SOURCES})
    TARGET_LINK_LIBRARIES(${TOOL_NAME} dfhack)
    if(DEFINED LOCAL_DEPNAME)
        ADD_DEPENDENCIES(${TOOL_NAME} ${LOCAL_DEPNAME})
    endif()
    install(TARGETS
            ${TOOL_NAME}
            RUNTIME DESTINATION bin)
ENDMACRO()

IF(BUILD_DFHACK_SUPPORTED)
    add_subdirectory (tools/supported)
ENDIF()

IF(BUILD_DFHACK_EXAMPLES)
    add_subdirectory (tools/examples)
ENDIF()

IF(BUILD_DFHACK_PLAYGROUND)
    add_subdirectory (tools/playground)
ENDIF()

IF(BUILD_DFHACK_DOXYGEN)
    add_subdirectory (doc)
ENDIF()
