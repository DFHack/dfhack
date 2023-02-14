project(buildingplan)

set(COMMON_HDRS
    buildingplan.h
    itemfilter.h
    plannedbuilding.h
)
set_source_files_properties(${COMMON_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

dfhack_plugin(buildingplan
    buildingplan.cpp buildingplan_cycle.cpp itemfilter.cpp plannedbuilding.cpp
    ${COMMON_HDRS}
    LINK_LIBRARIES lua)
