project(package_windows)

if(WIN32)
    add_executable(launchdf WIN32 launchdf.c)
    install(TARGETS launchdf
        DESTINATION ${DFHACK_DATA_DESTINATION})
endif()
