OPTION(DL_RUBY "download libruby from the internet" ON)
IF (DL_RUBY AND NOT APPLE)
    IF (UNIX)
        FILE(DOWNLOAD http://cloud.github.com/downloads/jjyg/dfhack/libruby187.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/libruby187.tar.gz
            EXPECTED_MD5 eb2adea59911f68e6066966c1352f291)
        EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E tar xzf libruby187.tar.gz
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        FILE(RENAME ${CMAKE_CURRENT_BINARY_DIR}/libruby1.8.so.1.8.7 ${CMAKE_CURRENT_BINARY_DIR}/libruby.so)
        SET(RUBYLIB ${CMAKE_CURRENT_BINARY_DIR}/libruby.so)
    ELSE (UNIX)
        FILE(DOWNLOAD http://cloud.github.com/downloads/jjyg/dfhack/msvcrtruby187.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/msvcrtruby187.tar.gz
            EXPECTED_MD5 9f4a1659ac3a5308f32d3a1937bbeeae)
        EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E tar xzf msvcrtruby187.tar.gz
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        FILE(RENAME ${CMAKE_CURRENT_BINARY_DIR}/msvcrt-ruby18.dll ${CMAKE_CURRENT_BINARY_DIR}/libruby.dll)
        SET(RUBYLIB ${CMAKE_CURRENT_BINARY_DIR}/libruby.dll)
    ENDIF(UNIX)
ENDIF(DL_RUBY AND NOT APPLE)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.rb
    COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.rb
    DEPENDS ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl
    COMMENT ruby-autogen.rb
)
ADD_CUSTOM_TARGET(ruby-autogen-rb DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.rb)

INCLUDE_DIRECTORIES("${dfhack_SOURCE_DIR}/depends/tthread")

DFHACK_PLUGIN(ruby ruby.cpp LINK_LIBRARIES dfhack-tinythread)
ADD_DEPENDENCIES(ruby ruby-autogen-rb)

INSTALL(FILES ${RUBYLIB} DESTINATION ${DFHACK_LIBRARY_DESTINATION})

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.rb DESTINATION hack/ruby)

INSTALL(DIRECTORY .
        DESTINATION hack/ruby
        FILES_MATCHING PATTERN "*.rb")
