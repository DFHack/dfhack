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
   message(FATAL_ERROR "In-source builds are not allowed.")
endif()

set(DEVEL_RELEASE "-dev")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "5")
set(CPACK_PACKAGE_VERSION_PATCH "8")
set(DFHACK_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}${DEVEL_RELEASE}")
set(CPACK_PACKAGE_NAME "dfhack")

## setting the build type
IF(NOT DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

## put everything in one big ugly directory to make MSVC and KDevelop debuggers happy
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

SET(DFHACK_INSTALL "portable" CACHE STRING "Choose the install type: 'portable' for a portable zip or tar.gz package, 'linux' for packaging and system installs on linux, 'none' for no install target (default).")
SET( MEMXML_DATA_PATH . CACHE PATH "Path to a valid Memory.xml file. This is baked into the library, so when you package DFHack for linux, set it to the right path.")

IF(${DFHACK_INSTALL} STREQUAL "portable")
    # the dfhack libraries will be installed here:
    SET(DFHACK_LIBRARY_DESTINATION .)
    # the dfhack tools will be installed here:
    SET(DFHACK_BINARY_DESTINATION .)
    # Memory.xml goes here:
    SET(DFHACK_DATA_DESTINATION .)
    # Includes go here:
    SET(DFHACK_INCLUDES_DESTINATION dev/include)
    # the Windows .lib files go here:
    IF(WIN32)
        SET(DFHACK_DEVLIB_DESTINATION dev)
        SET(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    ENDIF()
ENDIF()

IF(${DFHACK_INSTALL} STREQUAL "linux")
    # the dfhack libraries will be installed here:
    SET(DFHACK_LIBRARY_DESTINATION lib)
    # the dfhack tools will be installed here:
    SET(DFHACK_BINARY_DESTINATION bin)
    # Memory.xml goes here:
    SET(DFHACK_DATA_DESTINATION share/dfhack)
    # Includes go here:
    SET(DFHACK_INCLUDES_DESTINATION include)
ENDIF()

## some options for the user/developer to play with
OPTION(BUILD_DFHACK_SUPPORTED "Build the supported tools." ON)
OPTION(BUILD_DFHACK_C_BINDINGS "Build the C portion of the library" ON)
OPTION(BUILD_DFHACK_DOXYGEN "Create doxygen documentation for developers" ON)
OPTION(BUILD_DFHACK_EXAMPLES "Build example tools" OFF)
OPTION(BUILD_DFHACK_PLAYGROUND "Build tools from the playground folder" OFF)

include_directories (${CMAKE_SOURCE_DIR}/library/include/)
include_directories (${CMAKE_SOURCE_DIR}/library/shm/)
include_directories (${CMAKE_SOURCE_DIR}/library/depends/argstream/)

add_subdirectory (library)

# macro to save on typing in the tool subdirs
# builds a tool, links it to the dfhack lib and makes sure the dependency
# LOCAL_DEPNAME is built first, in case there is one
MACRO(DFHACK_TOOL TOOL_NAME TOOL_SOURCES)
    ADD_EXECUTABLE(${TOOL_NAME} ${TOOL_SOURCES})
    TARGET_LINK_LIBRARIES(${TOOL_NAME} dfhack)
    if(DEFINED LOCAL_DEPNAME)
        ADD_DEPENDENCIES(${TOOL_NAME} ${LOCAL_DEPNAME})
    endif()
    install(TARGETS
            ${TOOL_NAME}
            RUNTIME DESTINATION ${DFHACK_BINARY_DESTINATION})
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

IF(${DFHACK_INSTALL} STREQUAL "portable")
    IF(UNIX)
        SET(CPACK_GENERATOR "TGZ")
    ENDIF()
    IF(WIN32)
        SET(CPACK_GENERATOR "ZIP")
        # this includes the MSVC C++ DLLs in the package. Doesn't work with Express versions in general.
        INCLUDE(InstallRequiredSystemLibraries)
    ENDIF()
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${DFHACK_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    INCLUDE(CPack)
ENDIF()