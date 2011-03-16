# repurposed from libnoise: http://github.com/qknight/libnoise/tree/master/doc/
# following code and comments is by the original author, with some changes by
# me (peterix)
# ------------------------------------------------------------------------------
#
# many thanks go to Philippe Poilbarbe for writing the code this file is based on
#  http://www.cmake.org/pipermail/cmake/2006-August/010794.html
#
# much later i also found this:
#  http://tobias.rautenkranz.ch/cmake/doxygen/
# but it is hard to understand...

FIND_PACKAGE(Doxygen QUIET)

IF(DOXYGEN_FOUND)
  SET(DOXYGEN_LANGUAGE "English" CACHE STRING "Language used by doxygen")
  MARK_AS_ADVANCED(DOXYGEN_LANGUAGE)

  # you could also set the version with this, see Doxygen.in
  # there you will find a line like this: 
  #      PROJECT_NUMBER         = @DFHACK_VERSION@
  # @DFHACK_VERSION@ is then replaced by our global DFHACK_VERSION
  #
  # for instance you could uncomment the next 3 lines and change the version for testing
  # SET(DFHACK_VERSION
  #     "1.2.3-foo500"
  # )

  # doxygen can reference external images with IMAGE_PATH, this is how we set it dynamically
  SET( CMAKE_DOXYGEN_IMAGE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/img"
  )

  # doxygen searches for source code (defined in FILE_PATTERNS, for example: *.cpp *.h)
  # with DOXYGEN_SOURCE_DIR we fill a list of directories and later we write it into
  # the Doxyfile with a REGEX REPLACE (see below)
  SET( DOXYGEN_SOURCE_DIR
 #   "${CMAKE_SOURCE_DIR}/library"
    "${CMAKE_SOURCE_DIR}/doc/index.dxgen"
    "${CMAKE_SOURCE_DIR}/library/include"
    "${CMAKE_SOURCE_DIR}/library/include/dfhack"
    "${CMAKE_SOURCE_DIR}/library/include/dfhack/modules"
    "${CMAKE_SOURCE_DIR}/library/include/dfhack-c"
    "${CMAKE_SOURCE_DIR}/library/include/dfhack-c/modules"
#    "${CMAKE_SOURCE_DIR}/library/modules"
#    "${CMAKE_SOURCE_DIR}/library/shm"
#    "${CMAKE_SOURCE_DIR}/library/private"
  )

  STRING(REGEX REPLACE ";" " " CMAKE_DOXYGEN_INPUT_LIST "${DOXYGEN_SOURCE_DIR}")

  CONFIGURE_FILE(Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  SET(HTML_TARGET "html" )

  ADD_CUSTOM_TARGET(${HTML_TARGET} ALL
        /usr/bin/doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # INSTALL( DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html/" DESTINATION "/usr/share/doc/dfhack-${DFHACK_VERSION}" )

ELSE(DOXYGEN_FOUND)
  MESSAGE (WARNING "Doxygen binary couldn't be found. Can't build development documentation.'")
ENDIF(DOXYGEN_FOUND)