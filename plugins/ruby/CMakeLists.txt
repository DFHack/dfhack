find_package(Ruby)
if(RUBY_FOUND)
    ADD_CUSTOM_COMMAND(
        OUTPUT ruby-autogen.rb
        COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_SOURCE_DIR}/ruby-autogen.rb
        DEPENDS ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml codegen.pl
    )
    ADD_CUSTOM_TARGET(ruby-autogen-rb ALL DEPENDS ruby-autogen.rb)
    include_directories("${dfhack_SOURCE_DIR}/depends/tthread" ${RUBY_INCLUDE_PATH})
    DFHACK_PLUGIN(ruby ruby.cpp LINK_LIBRARIES dfhack-tinythread)
    target_link_libraries(ruby ${RUBY_LIBRARY})
    install(FILES ruby.rb ruby-autogen.rb DESTINATION ${DFHACK_LIBRARY_DESTINATION})
else(RUBY_FOUND)
    MESSAGE(STATUS "Required library (ruby) not found - ruby plugin can't be built.")
endif(RUBY_FOUND)
