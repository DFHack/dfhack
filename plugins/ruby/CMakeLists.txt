OPTION(DL_RUBY "download libruby from the internet" ON)
IF (DL_RUBY)
    IF (UNIX)
        FILE(DOWNLOAD http://cloud.github.com/downloads/jjyg/dfhack/libruby-1.9.1.so.zip
            ${dfhack_SOURCE_DIR}/libruby-1.9.1.so.zip
            EXPECTED_MD5 f38c4d3effc547ccc60ac4620f40bd14)
        execute_process(COMMAND unzip ${dfhack_SOURCE_DIR}/libruby-1.9.1.so.zip
            WORKING_DIRECTORY ${dfhack_SOURCE_DIR})
        SET(RUBYLIB ${dfhack_SOURCE_DIR}/libruby-1.9.1.so.1.9.1)
    ELSE (UNIX)
        FILE(DOWNLOAD http://cloud.github.com/downloads/jjyg/dfhack/msvcrt-ruby191.zip
            ${dfhack_SOURCE_DIR}/msvcrt-ruby191.zip
            EXPECTED_MD5 ed90e11e150b4b00588b66807a6276e7)
        execute_process(COMMAND unzip ${dfhack_SOURCE_DIR}/msvcrt-ruby191.zip
            WORKING_DIRECTORY ${dfhack_SOURCE_DIR})
        SET(RUBYLIB ${dfhack_SOURCE_DIR}/msvcrt-ruby191.dll)
    ENDIF(UNIX)
ENDIF(DL_RUBY)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ruby-autogen.rb
    COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_SOURCE_DIR}/ruby-autogen.rb
    DEPENDS ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl
    COMMENT ruby-autogen.rb
)
ADD_CUSTOM_TARGET(ruby-autogen-rb DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ruby-autogen.rb)

include_directories("${dfhack_SOURCE_DIR}/depends/tthread")

DFHACK_PLUGIN(ruby ruby.cpp LINK_LIBRARIES dfhack-tinythread)
ADD_DEPENDENCIES(ruby ruby-autogen-rb)

install(FILES ruby.rb ruby-autogen.rb ${RUBYLIB} DESTINATION ${DFHACK_LIBRARY_DESTINATION})
