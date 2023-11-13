project(package_windows)

option(BUILD_DFLAUNCH "Whether to build the Steam launcher exectuable (requires Steam SDK)." OFF)

if(WIN32 AND BUILD_DFLAUNCH)
    # builder must manually download Steam SDK
    set (STEAMAPI_DIR ${dfhack_SOURCE_DIR}/depends/steam)
    set (STEAMAPI_VER 156)
    set (STEAMAPI_ZIP_EXPECTED_HASH af5a579990dbe5ae4c1b0689260d001b)
    set (STEAMSDK_ZIP ${STEAMAPI_DIR}/steamworks_sdk_${STEAMAPI_VER}.zip)

    set (STEAM_SDK_HASH "NOT FOUND")
    file(MD5 ${STEAMSDK_ZIP} STEAM_SDK_HASH)
    if (NOT (${STEAM_SDK_HASH} STREQUAL ${STEAMAPI_ZIP_EXPECTED_HASH}))
        message(FATAL_ERROR "You need the Steamworks SDK at ${STEAMSDK_ZIP} to build launchdf.exe. Please disable the BUILD_DFLAUNCH CMake option or download the Steam SDK from: https://partner.steamgames.com/downloads/steamworks_sdk_${STEAMAPI_VER}.zip")
    endif()
    if (${STEAMSDK_ZIP} IS_NEWER_THAN ${STEAMAPI_DIR}/sdk)
        file(ARCHIVE_EXTRACT
            INPUT ${STEAMSDK_ZIP}
            DESTINATION ${STEAMAPI_DIR})
    endif()

    set(STEAMAPI_LIBRARY "${STEAMAPI_DIR}/sdk/redistributable_bin/win64/steam_api64.lib")
    set(STEAMAPI_SOURCE_DIR "${STEAMAPI_DIR}/sdk/public/steam")
    set(STEAMAPI_SHARED_LIBRARY "${STEAMAPI_DIR}/sdk/redistributable_bin/win64/steam_api64.dll")

    include_directories(${STEAMAPI_SOURCE_DIR})
    link_libraries(${STEAMAPI_LIBRARY})
    add_executable(launchdf WIN32 launchdf.cpp)
    install(TARGETS launchdf DESTINATION ${DFHACK_DATA_DESTINATION})
    install(FILES ${STEAMAPI_SHARED_LIBRARY} DESTINATION ${DFHACK_DATA_DESTINATION})
endif()
