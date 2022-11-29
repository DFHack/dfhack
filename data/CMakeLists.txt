install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/init/
        DESTINATION "${DFHACK_DATA_DESTINATION}/init")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/base_command_counts.json
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/base_command_counts.json")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/quickfort/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/quickfort")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/orders/
        DESTINATION dfhack-config/orders/library)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples/
        DESTINATION "${DFHACK_DATA_DESTINATION}/examples")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/professions/
        DESTINATION dfhack-config/professions/library)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blueprints/
        DESTINATION blueprints
        FILES_MATCHING PATTERN "*"
                       PATTERN blueprints/library/test EXCLUDE)

if(BUILD_SCRIPT_TESTS)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blueprints/library/test/
            DESTINATION blueprints/library/test
    )
endif()
