# main project file. use it from a build sub-folder, see COMPILE for details
PROJECT (dfhack) 
cmake_minimum_required(VERSION 2.6)

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

include_directories (${CMAKE_SOURCE_DIR}/library/)

add_subdirectory (library)
add_subdirectory (tools)