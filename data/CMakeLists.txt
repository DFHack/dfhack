install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dfhack-config/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/dfhack-config-defaults")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/init/
        DESTINATION "${DFHACK_DATA_DESTINATION}/init")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/base_command_counts.json
        DESTINATION "${DFHACK_DATA_DESTINATION}/data")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/orders/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/orders")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stockpiles/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/stockpiles")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/art/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/art")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/professions/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/professions")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blueprints/
        DESTINATION "${DFHACK_DATA_DESTINATION}/data/blueprints"
        FILES_MATCHING PATTERN "*"
                       PATTERN blueprints/test EXCLUDE)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/patches/
    DESTINATION ${DFHACK_DATA_DESTINATION}/patches
)

if(BUILD_TESTS)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blueprints/test/
            DESTINATION "${DFHACK_DATA_DESTINATION}/data/blueprints/test")
endif()
