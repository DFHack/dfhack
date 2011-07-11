# main project file. use it from a build sub-folder, see COMPILE for details

## some generic CMake magic
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(dfhack)

SET(CMAKE_MODULE_PATH
${dfhack_SOURCE_DIR}/CMake/Modules
${CMAKE_MODULE_PATH}
)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "In-source builds are not allowed.")
endif()

set(DFHACK_VERSION_MAJOR "0")
set(DFHACK_VERSION_MINOR "31")
set(DFHACK_VERSION_PATCH "25")
set(DFHACK_REVISION "1")

set(DFHACK_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

## setting the build type
IF(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Release RelWithDebInfo.")
ENDIF()

SET(DFHACK_OUTPUT_DIR "${dfhack_BINARY_DIR}/bin" CACHE STRING "Where should be the produced libraries and binaries stored.")
SET(DFHACK_PLUGIN_OUTPUT_DIR "${DFHACK_OUTPUT_DIR}/plugins")

## where to put things during the build (force MSVC to behave)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DFHACK_OUTPUT_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${DFHACK_OUTPUT_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${DFHACK_OUTPUT_DIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${DFHACK_OUTPUT_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${DFHACK_OUTPUT_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${DFHACK_OUTPUT_DIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DFHACK_OUTPUT_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DFHACK_OUTPUT_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DFHACK_OUTPUT_DIR})

## where to install things (after the build is done, classic 'make install' or package structure)
# the dfhack libraries will be installed here:
SET(DFHACK_LIBRARY_DESTINATION .)
# the dfhack tools will be installed here:
SET(DFHACK_BINARY_DESTINATION .)
# Memory.xml goes here:
SET(DFHACK_DATA_DESTINATION .)
# Plugins go here
SET(DFHACK_PLUGIN_DESTINATION plugins)
# Includes go here:
SET(DFHACK_INCLUDES_DESTINATION dev/include)
# the Windows .lib files go here:
IF(WIN32)
    SET(DFHACK_DEVLIB_DESTINATION dev)
    SET(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
ENDIF()
IF(UNIX)
    SET(CMAKE_INSTALL_RPATH "$ORIGIN")
ENDIF()
# documentation goes here:
SET(DFHACK_USERDOC_DESTINATION .)
SET(DFHACK_DEVDOC_DESTINATION dev)
SET(DFHACK_DOXYGEN_DESTINATION dev/doxygen)


## some options for the user/developer to play with
OPTION(BUILD_LIBRARY "Build the library that goes into DF." ON)
OPTION(BUILD_PLUGINS "Build the library that goes into DF." ON)

IF(BUILD_LIBRARY)
    add_subdirectory (library)
    ## install the default documentation files
    install(FILES LICENSE Readme.html DESTINATION ${DFHACK_USERDOC_DESTINATION})
endif()

IF(BUILD_PLUGINS)
    add_subdirectory (plugins)
endif()

# Packaging with CPack!
IF(UNIX)
    SET(CPACK_GENERATOR "TGZ")
ENDIF()
IF(WIN32)
    SET(CPACK_GENERATOR "ZIP")
    # this includes the MSVC C++ DLLs in the package. Doesn't work with Express versions in general.
    # we should be fine with the stuff DF already has packaged... so, commenting out
    # INCLUDE(InstallRequiredSystemLibraries)
ENDIF()
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${DFHACK_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
INCLUDE(CPack)


