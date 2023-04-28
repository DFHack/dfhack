project(package_windows)

if(WIN32)
    if (BUILD_DFLAUNCH)
        if ((DEFINED ENV{steam_username}) AND (DEFINED ENV{steam_password}))
            # download Steam SDK
            set (STEAMAPI_DIR ${dfhack_SOURCE_DIR}/depends/steam)
            file(DOWNLOAD "https://partner.steamgames.com/downloads/steamworks_sdk_156.zip"
                    ${STEAMAPI_DIR}/steamworks_sdk_156.zip
                    EXPECTED_HASH MD5=af5a579990dbe5ae4c1b0689260d001b
                    USERPWD $ENV{steam_username}:$ENV{steam_password}
                    STATUS STEAM_SDK_DOWNLOAD_STATUS
                    SHOW_PROGRESS
            )
            list(GET STEAM_SDK_DOWNLOAD_STATUS 0 STEAM_SDK_DL_STATUS_CODE)
            list(GET STEAM_SDK_DOWNLOAD_STATUS 1 STEAM_SDK_DL_ERROR_MESSAGE)
            if (NOT (${STEAM_SDK_DL_STATUS_CODE} EQUAL 0))
                message(FATAL_ERROR "Steam SDK download: " ${STEAM_SDK_DL_ERROR_MESSAGE})
            else ()
                message(STATUS "Steam SDK download: " ${STEAM_SDK_DL_ERROR_MESSAGE})
                file(ARCHIVE_EXTRACT
                        INPUT ${STEAMAPI_DIR}/steamworks_sdk_156.zip
                        DESTINATION ${STEAMAPI_DIR})
                set(STEAMAPI_LIBRARY "${STEAMAPI_DIR}/sdk/redistributable_bin/win64/steam_api64.lib")
                set(STEAMAPI_SOURCE_DIR "${STEAMAPI_DIR}/sdk/public/steam")
                set(STEAMAPI_SHARED_LIBRARY "${STEAMAPI_DIR}/sdk/redistributable_bin/win64/steam_api64.dll")
            endif()
        else()
            message(SEND_ERROR "Need to set steam_username and steam_password in environment to download Steamworks SDK")
        endif()

        include_directories(${STEAMAPI_SOURCE_DIR})
        link_libraries(${STEAMAPI_LIBRARY})
        add_executable(launchdf WIN32 launchdf.cpp)
        install(TARGETS launchdf DESTINATION ${DFHACK_DATA_DESTINATION})
        install(FILES ${STEAMAPI_SHARED_LIBRARY} DESTINATION ${DFHACK_DATA_DESTINATION})
    endif()
endif()
