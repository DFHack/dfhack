project(buildingplan)

set(COMMON_HDRS
    buildingplan.h
    buildingplan-planner.h
    buildingplan-rooms.h
)
set_source_files_properties(${COMMON_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

dfhack_plugin(buildingplan buildingplan.cpp buildingplan-planner.cpp
    buildingplan-rooms.cpp ${COMMON_HDRS} LINK_LIBRARIES lua)
