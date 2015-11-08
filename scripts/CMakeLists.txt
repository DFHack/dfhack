include(Scripts.cmake)
DFHACK_3RDPARTY_SCRIPT_REPO(dscorbett)
DFHACK_3RDPARTY_SCRIPT_REPO(kane-t)
DFHACK_3RDPARTY_SCRIPT_REPO(lethosor)
DFHACK_3RDPARTY_SCRIPT_REPO(maienm)
DFHACK_3RDPARTY_SCRIPT_REPO(maxthyme)
# DFHACK_3RDPARTY_SCRIPT_REPO(roses)

install(DIRECTORY ${dfhack_SOURCE_DIR}/scripts
        DESTINATION ${DFHACK_DATA_DESTINATION}
        FILES_MATCHING PATTERN "*.lua"
                       PATTERN "*.rb"
                       PATTERN "3rdparty" EXCLUDE
        )
