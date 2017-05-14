# Allow build system to turn off downloading of libruby.so.
OPTION(DOWNLOAD_RUBY "Download prebuilt libruby.so for ruby plugin." ON)

IF (DOWNLOAD_RUBY)

    IF (APPLE)
        SET(RUBYLIB ${CMAKE_CURRENT_SOURCE_DIR}/osx${DFHACK_BUILD_ARCH}/libruby.dylib)
        SET(RUBYLIB_INSTALL_NAME "libruby.dylib")
        IF(${DFHACK_BUILD_ARCH} STREQUAL 64)
            # MESSAGE("No ruby lib for 64-bit OS X yet")
        ELSE()
            DOWNLOAD_FILE_UNZIP("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/osx32-libruby187.dylib.gz"
                "gz"
                ${RUBYLIB}.gz
                "e9bc4263557e652121b055a46abb4f97"
                ${RUBYLIB}
                "3ee5356759f764a440be5b5b44649826")
        ENDIF()
    ELSEIF(UNIX)
        SET(RUBYLIB ${CMAKE_CURRENT_SOURCE_DIR}/linux${DFHACK_BUILD_ARCH}/libruby.so)
        SET(RUBYLIB_INSTALL_NAME "libruby.so")
        IF(${DFHACK_BUILD_ARCH} STREQUAL 64)
            DOWNLOAD_FILE_UNZIP("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/linux64-libruby187.so.gz"
                "gz"
                ${RUBYLIB}.gz
                "8eb757bb9ada08608914d8ca8906c427"
                ${RUBYLIB}
                "e8c36a06f031cfbf02def28169bb5f1f")
        ELSE()
            DOWNLOAD_FILE_UNZIP("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/linux32-libruby187.so.gz"
                "gz"
                ${RUBYLIB}.gz
                "2d06f5069ff07ea934ecd40db55a4ac5"
                ${RUBYLIB}
                "b00d8d7086cb39f6fde793f9d89cb2d7")
        ENDIF()
    ELSE()
        SET(RUBYLIB ${CMAKE_CURRENT_SOURCE_DIR}/win${DFHACK_BUILD_ARCH}/libruby.dll)
        SET(RUBYLIB_INSTALL_NAME "libruby.dll")
        IF(${DFHACK_BUILD_ARCH} STREQUAL 64)
            DOWNLOAD_FILE_UNZIP("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/win64-libruby200.dll.gz"
                "gz"
                ${RUBYLIB}.gz
                "81db54a8b8b3090c94c6ae2147d30b8f"
                ${RUBYLIB}
                "8a8564418aebddef3dfee1e96690e713")
        ELSE()
            DOWNLOAD_FILE_UNZIP("https://github.com/DFHack/dfhack-bin/releases/download/0.43.05/win32-libruby187.dll.gz"
                "gz"
                ${RUBYLIB}.gz
                "ffc0f1b5b33748e2a36128e90c97f6b2"
                ${RUBYLIB}
                "482c1c418f4ee1a5f04203eee1cda0ef")
        ENDIF()
    ENDIF()

ENDIF()

IF (APPLE OR UNIX)
    SET(RUBYAUTOGEN ruby-autogen-gcc.rb)
ELSE (APPLE OR UNIX)
    SET(RUBYAUTOGEN ruby-autogen-win.rb)
ENDIF (APPLE OR UNIX)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${RUBYAUTOGEN}
    COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_BINARY_DIR}/${RUBYAUTOGEN} ${CMAKE_SYSTEM_NAME} ${DFHACK_BUILD_ARCH}
    # cmake quirk: depending on codegen.out.xml or generate_headers only is not enough, needs both
    # test by manually patching any library/xml/moo.xml, run make ruby-autogen-rb -j2, and check build/plugins/ruby/ruby-autogen.rb for patched xml data
    DEPENDS generate_headers ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl
    COMMENT ${RUBYAUTOGEN}
)
ADD_CUSTOM_TARGET(ruby-autogen-rb DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${RUBYAUTOGEN})

INCLUDE_DIRECTORIES("${dfhack_SOURCE_DIR}/depends/tthread")

DFHACK_PLUGIN(ruby ruby.cpp LINK_LIBRARIES dfhack-tinythread)
ADD_DEPENDENCIES(ruby ruby-autogen-rb)

IF(EXISTS ${RUBYLIB})
    INSTALL(FILES ${RUBYLIB} DESTINATION ${DFHACK_LIBRARY_DESTINATION} RENAME ${RUBYLIB_INSTALL_NAME})
ELSE()
    # Only fire this warning if DOWNLOAD_RUBY was set.
    IF(NOT(APPLE AND ${DFHACK_BUILD_ARCH} STREQUAL 64) AND DOWNLOAD_RUBY)
        MESSAGE(WARNING "Ruby library not found at ${RUBYLIB} - will not be installed")
    ENDIF()
ENDIF()

INSTALL(DIRECTORY .
        DESTINATION hack/ruby
        FILES_MATCHING PATTERN "*.rb")

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${RUBYAUTOGEN} DESTINATION hack/ruby)
