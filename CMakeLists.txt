# main project file. use it from a build sub-folder, see COMPILE for details

## some generic CMake magic
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
PROJECT (dfhack)

SET(CMAKE_MODULE_PATH
${dfhack_SOURCE_DIR}/CMake/Modules
${CMAKE_MODULE_PATH}
)

if("${dfhack_SOURCE_DIR}" STREQUAL "${dfhack_BINARY_DIR}")
   message(FATAL_ERROR "In-source builds are not allowed.")
endif()

SET(DFHACK_CONSISTENCY 1)

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "5")
set(CPACK_PACKAGE_VERSION_PATCH "11")
set(DFHACK_REVISION "1")

set(DFHACK_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION ${DFHACK_VERSION})

set(CPACK_PACKAGE_NAME "dfhack")
SET(CPACK_PACKAGE_VENDOR "dethware.org")
SET(CPACK_PACKAGE_CONTACT "peterix@dethware.org")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Memory hacks for Dwarf Fortress")
SET(CPACK_PACKAGE_DESCRIPTION
"DFHack is a Dwarf Fortress memory access library and a set of basic
tools using this library. The library is a work in progress, so things
might change as more tools are written for it.
.
It is an attempt to unite the various ways tools access DF memory and
allow for easier development of new tools.")


## setting the build type
IF(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

## put everything in one big ugly directory to make MSVC and KDevelop debuggers happy
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${dfhack_BINARY_DIR}/bin")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${dfhack_BINARY_DIR}/bin")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${dfhack_BINARY_DIR}/bin")

IF(WIN32)
    set (DFHACK_INST_DEFAULT "portable")
ELSE()
    set (DFHACK_INST_DEFAULT "linux")
ENDIF()
SET( DFHACK_INSTALL ${DFHACK_INST_DEFAULT} CACHE STRING
"Choose the install type:
'portable' for a portable zip or tar.gz package (windows default)
'linux' for generic packaging and system installs on linux (linux default)
'ubuntu-10.10' for ubuntu maverick package.")
SET( MEMXML_DATA_PATH . CACHE PATH
"Path to a valid Memory.xml file.
This is baked into the library, so when you package DFHack for linux, set it to the right path.")

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
    IF(UNIX)
        SET(CMAKE_INSTALL_RPATH "$ORIGIN")
    ENDIF()
    # documentation goes here:
    SET(DFHACK_USERDOC_DESTINATION .)
    SET(DFHACK_DEVDOC_DESTINATION dev)
    SET(DFHACK_DOXYGEN_DESTINATION dev/doxygen)
ENDIF()

# generic linux package in a .tar.gz
IF(${DFHACK_INSTALL} STREQUAL "linux")
    if(WIN32)
        MESSAGE(FATAL_ERROR "WTF are you doing?")
    endif()
    # set RPATH to always point at the dfhack lib using relative path.
    SET(CMAKE_INSTALL_RPATH "$ORIGIN/../lib/")
    # the dfhack libraries will be installed here:
    SET(DFHACK_LIBRARY_DESTINATION lib)
    # the dfhack tools will be installed here:
    SET(DFHACK_BINARY_DESTINATION bin)
    # Memory.xml goes here:
    SET(DFHACK_DATA_DESTINATION share/dfhack)
    # Includes go here:
    SET(DFHACK_INCLUDES_DESTINATION include)
    # documentation goes here:
    SET(DFHACK_USERDOC_DESTINATION share/dfhack/doc)
    SET(DFHACK_DEVDOC_DESTINATION share/dfhack/doc)
    SET(DFHACK_DOXYGEN_DESTINATION share/dfhack/doc/doxygen)
ENDIF()

IF(${DFHACK_INSTALL} STREQUAL "ubuntu-10.10")
    if(WIN32)
        MESSAGE(FATAL_ERROR "WTF are you doing?")
    endif()
    # set RPATH to always point at the dfhack lib using relative path.
    SET(CMAKE_INSTALL_RPATH "$ORIGIN/../lib/")
    # the dfhack libraries will be installed here:
    SET(DFHACK_LIBRARY_DESTINATION usr/lib)
    # the dfhack tools will be installed here:
    SET(DFHACK_BINARY_DESTINATION usr/bin)
    # Memory.xml goes here:
    SET(DFHACK_DATA_DESTINATION usr/share/dfhack)
    # Includes go here:
    SET(DFHACK_INCLUDES_DESTINATION usr/include)
    # documentation goes here:
    SET(DFHACK_USERDOC_DESTINATION usr/share/dfhack/doc)
    SET(DFHACK_DEVDOC_DESTINATION usr/share/dfhack/doc)
    SET(DFHACK_DOXYGEN_DESTINATION usr/share/dfhack/doc/doxygen)
    INSTALL(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/package/ubuntu-10.10/99-dfhack.conf"
            DESTINATION etc/sysctl.d)
ENDIF()



## some options for the user/developer to play with
OPTION(BUILD_DFHACK_LIBRARY "Build the library. Needed for all the tools." ON)
OPTION(BUILD_DFHACK_C_BINDINGS "Build the C portion of the library." ON)
OPTION(BUILD_DFHACK_PYTHON_BINDINGS "Build/install the python wrapper." ON)
IF(WIN32)
    OPTION(BUILD_DFHACK_DEVEL "Build the developer stuff." OFF)
ENDIF()
IF(UNIX)
    OPTION(BUILD_DFHACK_DEVEL "Build the developer stuff." ON)
ENDIF()
OPTION(BUILD_DFHACK_DOXYGEN "Create doxygen documentation for developers" ON)
OPTION(BUILD_DFHACK_SUPPORTED "Build the supported tools." ON)
OPTION(BUILD_DFHACK_EXAMPLES "Build example tools" OFF)
OPTION(BUILD_DFHACK_PLAYGROUND "Build tools from the playground folder" OFF)

include_directories (${dfhack_SOURCE_DIR}/library/include/)
include_directories (${dfhack_SOURCE_DIR}/library/shm/)
include_directories (${dfhack_SOURCE_DIR}/library/depends/argstream/)

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

IF(BUILD_DFHACK_LIBRARY)

    add_subdirectory (library)

    IF(BUILD_DFHACK_PYTHON_BINDINGS)
        MESSAGE("TODO: write CMakeLists.txt for the python things.")
    ENDIF()
    IF(BUILD_DFHACK_SUPPORTED)
        add_subdirectory (tools/supported)
    ENDIF()

    IF(BUILD_DFHACK_EXAMPLES)
        add_subdirectory (tools/examples)
    ENDIF()

    IF(BUILD_DFHACK_PLAYGROUND)
        add_subdirectory (tools/playground)
    ENDIF()

    ## install the default documentation files
    install(FILES LICENSE Readme.html DESTINATION ${DFHACK_USERDOC_DESTINATION})
endif()

IF(BUILD_DFHACK_DOXYGEN AND BUILD_DFHACK_DEVEL)
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

#-------------------------------------------------------------------------------
# Figure out debian architecture
#-------------------------------------------------------------------------------

FUNCTION(GET_DEBIAN_ARCHITECTURE arch)
    SET(dpkgarch)

    FIND_PROGRAM(DPKG_CMD dpkg)
    IF(NOT DPKG_CMD)
        MESSAGE(STATUS "Can not find dpkg in your path, default to i386.")
        SET(${arch} i386 PARENT_SCOPE)
    ENDIF()
    EXECUTE_PROCESS(COMMAND "${DPKG_CMD}" --print-architecture
                    OUTPUT_VARIABLE dpkgarch
                    OUTPUT_STRIP_TRAILING_WHITESPACE )
    SET(${arch} ${dpkgarch} PARENT_SCOPE)
ENDFUNCTION()

IF(${DFHACK_INSTALL} STREQUAL "ubuntu-10.10")
    SET(CPACK_GENERATOR "DEB")

    #wtf, wtf, wtf. force them to be empty
    set(CMAKE_INSTALL_PREFIX "" FORCE)
    set(CPACK_INSTALL_PREFIX "")
    SET(CPACK_PACKAGING_INSTALL_PREFIX "")
    set(CPACK_SET_DESTDIR true)

    SET(CPACK_DEBIAN_PACKAGE_SECTION "Games") # yep. magma.
    SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional") # very.
    SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # find deps automatically! hopefully... maybe...
    SET(INSTSCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/package/ubuntu-10.10")
    SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${INSTSCRIPT}/postinst;${INSTSCRIPT}/preinst;${INSTSCRIPT}/postrm")

    GET_DEBIAN_ARCHITECTURE(PKG_ARCHITECTURE)

    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${DFHACK_VERSION}-${DFHACK_REVISION}_${PKG_ARCHITECTURE}")
    INCLUDE(CPack)
ENDIF()
