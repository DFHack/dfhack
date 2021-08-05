install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/quickfort/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/quickfort")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blueprints/
        DESTINATION blueprints
        FILES_MATCHING PATTERN "*"
                       PATTERN blueprints/library/test EXCLUDE)

if(BUILD_TESTS)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blueprints/library/test/
            DESTINATION blueprints/library/test
    )
endif()

