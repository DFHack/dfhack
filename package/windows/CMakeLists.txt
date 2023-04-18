project(package_windows)

if(WIN32)
    include_directories(${STEAMAPI_SOURCE_DIR})
    link_libraries(${STEAMAPI_LIBRARY})
    add_executable(launchdf WIN32 launchdf.cpp)
    install(TARGETS launchdf DESTINATION ${DFHACK_DATA_DESTINATION})
    install(FILES ${STEAMAPI_SHARED_LIBRARY} DESTINATION ${DFHACK_DATA_DESTINATION})
endif()
