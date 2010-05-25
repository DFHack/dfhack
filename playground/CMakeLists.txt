# don't use this file directly. use the one in the root folder of the project

# this is required to ensure we use the right configuration for the system.
IF(UNIX)
add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

# for trying out some 'stuff'
ADD_EXECUTABLE(dftest test.cpp)
TARGET_LINK_LIBRARIES(dftest dfhack)

