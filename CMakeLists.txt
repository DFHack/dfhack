# main project file. use it from a build sub-folder, see COMPILE for details
PROJECT (dfhack)
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules)

# disable warning, autosearch
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
   message(SEND_ERROR "In-source builds are not allowed.")
endif("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")

IF(NOT DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF(NOT DEFINED CMAKE_BUILD_TYPE)

SET(SO_MAJOR_VERSION "0")
SET(SO_MINOR_VERSION "0")
SET(SO_BUILD_VERSION "1")
SET(SO_VERSION "${SO_MAJOR_VERSION}.${SO_MINOR_VERSION}.${SO_BUILD_VERSION}")


SET( LIBRARY_OUTPUT_PATH ${dfhack_SOURCE_DIR}/output CACHE PATH "Output directory for the dfhack library" )
SET( EXECUTABLE_OUTPUT_PATH ${dfhack_SOURCE_DIR}/output CACHE PATH "Output directory for the dfhack tools" )
SET( CMAKE_SWIG_OUTDIR ${dfhack_SOURCE_DIR}/output CACHE PATH "Global output directory for swig generated language wrappers" )

include_directories (${CMAKE_SOURCE_DIR}/dfhack/include/)
include_directories (${CMAKE_SOURCE_DIR}/dfhack/shm/)
include_directories (${CMAKE_SOURCE_DIR}/dfhack/depends/md5/)
include_directories (${CMAKE_SOURCE_DIR}/dfhack/depends/tinyxml/)
include_directories (${CMAKE_SOURCE_DIR}/dfhack/depends/argstream/)

add_subdirectory (dfhack)
#add_subdirectory (dfhack/python)
add_subdirectory (dfhack/shm)
add_subdirectory (tools/examples)
add_subdirectory (tools/playground)
add_subdirectory (tools/supported)
