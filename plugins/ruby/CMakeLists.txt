OPTION(DL_RUBY "download libruby from the internet" OFF)
IF (NOT APPLE)
	IF (UNIX)
		IF (DL_RUBY)
			FILE(DOWNLOAD http://cloud.github.com/downloads/jjyg/dfhack/libruby187.tar.gz ${CMAKE_CURRENT_SOURCE_DIR}/libruby187.tar.gz
				EXPECTED_MD5 eb2adea59911f68e6066966c1352f291)
		ENDIF(DL_RUBY)
		EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/libruby187.tar.gz
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		FILE(RENAME ${CMAKE_CURRENT_BINARY_DIR}/libruby1.8.so.1.8.7 ${CMAKE_CURRENT_BINARY_DIR}/libruby.so)
		SET(RUBYLIB ${CMAKE_CURRENT_BINARY_DIR}/libruby.so)
	ELSE (UNIX)
		IF (DL_RUBY)
			FILE(DOWNLOAD http://cloud.github.com/downloads/jjyg/dfhack/msvcrtruby187.tar.gz ${CMAKE_CURRENT_SOURCE_DIR}/msvcrtruby187.tar.gz
				EXPECTED_MD5 9f4a1659ac3a5308f32d3a1937bbeeae)
		ENDIF(DL_RUBY)
		EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/msvcrtruby187.tar.gz
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		FILE(RENAME ${CMAKE_CURRENT_BINARY_DIR}/msvcrt-ruby18.dll ${CMAKE_CURRENT_BINARY_DIR}/libruby.dll)
		SET(RUBYLIB ${CMAKE_CURRENT_BINARY_DIR}/libruby.dll)
	ENDIF(UNIX)
ENDIF(NOT APPLE)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.rb
    COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.rb
    # cmake quirk: depending on codegen.out.xml or generate_headers only is not enough, needs both
    # test by manually patching any library/xml/moo.xml, run make ruby-autogen-rb -j2, and check build/plugins/ruby/ruby-autogen.rb for patched xml data
    DEPENDS generate_headers ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl
    COMMENT ruby-autogen.rb
)
ADD_CUSTOM_TARGET(ruby-autogen-rb DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.rb)

INCLUDE_DIRECTORIES("${dfhack_SOURCE_DIR}/depends/tthread")

DFHACK_PLUGIN(ruby ruby.cpp LINK_LIBRARIES dfhack-tinythread)
ADD_DEPENDENCIES(ruby ruby-autogen-rb)

INSTALL(FILES ${RUBYLIB} DESTINATION ${DFHACK_LIBRARY_DESTINATION})

INSTALL(DIRECTORY .
        DESTINATION hack/ruby
        FILES_MATCHING PATTERN "*.rb")

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.rb DESTINATION hack/ruby)
