install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dfhack-config/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/dfhack-config-defaults")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/init/
        DESTINATION "${DFHACK_DATA_DESTINATION}/init")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/base_command_counts.json
        DESTINATION "${DFHACK_DATA_DESTINATION}/data")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/quickfort/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/quickfort")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/orders/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/orders")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/art/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/art")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/professions/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/professions")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blueprints/
        DESTINATION blueprints
        FILES_MATCHING PATTERN "*"
                       PATTERN blueprints/library/test EXCLUDE)

if(BUILD_TESTS)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blueprints/library/test/
            DESTINATION blueprints/library/test
    )
endif()
