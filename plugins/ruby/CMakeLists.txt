find_package(Ruby)
if(RUBY_FOUND)
    ADD_CUSTOM_COMMAND(
        OUTPUT ruby-autogen.cpp
        COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.cpp
        DEPENDS ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml codegen.pl
    )
    ADD_EXECUTABLE(ruby-autogen ruby-autogen.cpp)
    ADD_CUSTOM_COMMAND(
        OUTPUT ruby-autogen.offsets
        COMMAND ruby-autogen ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.offsets
        DEPENDS ruby-autogen
    )
    ADD_CUSTOM_COMMAND(
        OUTPUT ruby-autogen.rb
        COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/codegen.pl ${dfhack_SOURCE_DIR}/library/include/df/codegen.out.xml ${CMAKE_CURRENT_SOURCE_DIR}/ruby-autogen.rb ${CMAKE_CURRENT_BINARY_DIR}/ruby-autogen.offsets
        DEPENDS ruby-autogen.offsets
    )
    ADD_CUSTOM_TARGET(ruby-autogen-rb ALL DEPENDS ruby-autogen.rb)
    include_directories("${dfhack_SOURCE_DIR}/depends/tthread" ${RUBY_INCLUDE_PATH})
    DFHACK_PLUGIN(ruby ruby.cpp LINK_LIBRARIES dfhack-tinythread)
    target_link_libraries(ruby ${RUBY_LIBRARY})
    install(FILES ruby.rb ruby-autogen.rb DESTINATION ${DFHACK_LIBRARY_DESTINATION})
else(RUBY_FOUND)
    MESSAGE(STATUS "Required library (ruby) not found - ruby plugin can't be built.")
endif(RUBY_FOUND)
