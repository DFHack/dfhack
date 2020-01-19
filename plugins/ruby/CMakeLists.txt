# Allow build system to turn off downloading of libruby.so.
option(DOWNLOAD_RUBY "Download prebuilt libruby.so for ruby plugin." ON)

if(DOWNLOAD_RUBY)

    if(APPLE)
        set(RUBYLIB ${CMAKE_CURRENT_SOURCE_DIR}/osx${DFHACK_BUILD_ARCH}/libruby.dylib)
        set(RUBYLIB_INSTALL_NAME "libruby.dylib")
        if(${DFHACK_BUILD_ARCH} STREQUAL 64)
            # message("No ruby lib for 64-bit OS X yet")
        else()
            download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/osx32-libruby187.dylib.gz"
                "gz"
                ${RUBYLIB}.gz
                "e9bc4263557e652121b055a46abb4f97"
                ${RUBYLIB}
                "3ee5356759f764a440be5b5b44649826")
        endif()
    elseif(UNIX)
        set(RUBYLIB ${CMAKE_CURRENT_SOURCE_DIR}/linux${DFHACK_BUILD_ARCH}/libruby.so)
        set(RUBYLIB_INSTALL_NAME "libruby.so")
        if(${DFHACK_BUILD_ARCH} STREQUAL 64)
            download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/linux64-libruby187.so.gz"
                "gz"
                ${RUBYLIB}.gz
                "8eb757bb9ada08608914d8ca8906c427"
                ${RUBYLIB}
                "e8c36a06f031cfbf02def28169bb5f1f")
        else()
            download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/linux32-libruby187.so.gz"
                "gz"
                ${RUBYLIB}.gz
                "2d06f5069ff07ea934ecd40db55a4ac5"
                ${RUBYLIB}
                "b00d8d7086cb39f6fde793f9d89cb2d7")
        endif()
    else()
        set(RUBYLIB ${CMAKE_CURRENT_SOURCE_DIR}/win${DFHACK_BUILD_ARCH}/libruby.dll)
        set(RUBYLIB_INSTALL_NAME "libruby.dll")
        if(${DFHACK_BUILD_ARCH} STREQUAL 64)
            download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/win64-libruby200.dll.gz"
                "gz"
                ${RUBYLIB}.gz
                "81db54a8b8b3090c94c6ae2147d30b8f"
                ${RUBYLIB}
                "8a8564418aebddef3dfee1e96690e713")
        else()
            download_file_unzip("https://github.com/DFHack/dfhack-bin/releases/download/0.44.09/win32-libruby187.dll.gz"
                "gz"
                ${RUBYLIB}.gz
                "ffc0f1b5b33748e2a36128e90c97f6b2"
                ${RUBYLIB}
                "482c1c418f4ee1a5f04203eee1cda0ef")
        endif()
    endif()

endif()

if(APPLE OR UNIX)
    set(RUBYAUTOGEN ruby-autogen-gcc.rb)
else(APPLE OR UNIX)
    set(RUBYAUTOGEN ruby-autogen-win.rb)
endif(APPLE OR UNIX)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${RUBYAUTOGEN}
    COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_BINARY_DIR}/${RUBYAUTOGEN} ${CMAKE_SYSTEM_NAME} ${DFHACK_BUILD_ARCH}
    # cmake quirk: depending on codegen.out.xml or generate_headers only is not enough, needs both
    # test by manually patching any library/xml/moo.xml, run make ruby-autogen-rb -j2, and check build/plugins/ruby/ruby-autogen.rb for patched xml data
    DEPENDS generate_headers ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl
    COMMENT ${RUBYAUTOGEN}
)
add_custom_target(ruby-autogen-rb DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${RUBYAUTOGEN})

include_directories("${dfhack_SOURCE_DIR}/depends/tthread")

dfhack_plugin(ruby ruby.cpp LINK_LIBRARIES dfhack-tinythread)
add_dependencies(ruby ruby-autogen-rb)

if(EXISTS ${RUBYLIB})
    install(FILES ${RUBYLIB} DESTINATION ${DFHACK_LIBRARY_DESTINATION} RENAME ${RUBYLIB_INSTALL_NAME})
else()
    # Only fire this warning if DOWNLOAD_RUBY was set.
    if(NOT(APPLE AND ${DFHACK_BUILD_ARCH} STREQUAL 64) AND DOWNLOAD_RUBY)
        message(WARNING "Ruby library not found at ${RUBYLIB} - will not be installed")
    endif()
endif()

install(DIRECTORY .
    DESTINATION hack/ruby
    FILES_MATCHING PATTERN "*.rb")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${RUBYAUTOGEN} DESTINATION hack/ruby)
