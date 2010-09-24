# main project file. use it from a build sub-folder, see COMPILE for details
PROJECT (dfhack)
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules)
SET ( DFHACK_VERSION "0.4.1.0-dev" )

# Set this to project source dir. When dfhack is used
# as a submodule, CMAKE_SOURCE_DIR is not pointing to
# the root where this particular CMakeLists.txt file sits.
SET(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIR})

# disable warning, autosearch
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(SEND_ERROR "In-source builds are not allowed.")
endif("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

IF(NOT DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF(NOT DEFINED CMAKE_BUILD_TYPE)

SET( LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output CACHE PATH "Output directory for the dfhack library" )
SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output CACHE PATH "Output directory for the dfhack tools" )
SET( DATA_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output CACHE PATH "Output directory for the dfhack data (offset files)" )

OPTION(BUILD_DFHACK_DOCUMENTATION "Create doxygen documentation for developers" OFF)
OPTION(BUILD_DFHACK_EXAMPLES "Build example tools" OFF)
OPTION(BUILD_DFHACK_PLAYGROUND "Build tools from the playground folder" OFF)
OPTION(BUILD_DFHACK_C_BINDINGS "Build the C portion of the library" ON)
OPTION(BUILD_OFFSET_EDITOR "Build the Offset GUI editor (not ready for use)." OFF)
OPTION(BUILD_DFHACK_SUPPORTED "Build the supported toold." ON)
OPTION(BUILD_DFHACK_SHM "Build the SHM." OFF)

include_directories (${CMAKE_SOURCE_DIR}/library/include/)
include_directories (${CMAKE_SOURCE_DIR}/library/shm/)
include_directories (${CMAKE_SOURCE_DIR}/library/depends/md5/)
include_directories (${CMAKE_SOURCE_DIR}/library/depends/tinyxml/)
include_directories (${CMAKE_SOURCE_DIR}/library/depends/argstream/)

add_subdirectory (library)

IF(BUILD_DFHACK_SUPPORTED)
    add_subdirectory (tools/supported)
ENDIF(BUILD_DFHACK_SUPPORTED)

IF(BUILD_OFFSET_EDITOR)
    add_subdirectory (offsetedit)
ENDIF(BUILD_OFFSET_EDITOR)

IF(BUILD_DFHACK_SHM)
    add_subdirectory (library/shm)
ENDIF(BUILD_DFHACK_SHM)

IF(BUILD_DFHACK_EXAMPLES)
    add_subdirectory (tools/examples)
ENDIF(BUILD_DFHACK_EXAMPLES)

IF(BUILD_DFHACK_PLAYGROUND)
    add_subdirectory (tools/playground)
ENDIF(BUILD_DFHACK_PLAYGROUND)

IF(BUILD_DFHACK_DOCUMENTATION)
    add_subdirectory (doc)
ENDIF(BUILD_DFHACK_DOCUMENTATION)
