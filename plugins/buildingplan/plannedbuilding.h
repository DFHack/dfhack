#pragma once

#include "buildingplan.h"
#include "defaultitemfilters.h"
#include "itemfilter.h"

#include "Core.h"

#include "modules/Persistence.h"

#include "df/building.h"
#include "df/job_item_vector_id.h"

class PlannedBuilding {
public:
    const df::building::key_field_type id;

    // job_item idx -> list of vectors the task is linked to
    const std::vector<std::vector<df::job_item_vector_id>> vector_ids;

    const HeatSafety heat_safety;

    const std::vector<ItemFilter> item_filters;

    const std::set<std::string> specials;

    PlannedBuilding(DFHack::color_ostream &out, df::building *bld, HeatSafety heat, const DefaultItemFilters &item_filters);
    PlannedBuilding(DFHack::color_ostream &out, DFHack::PersistentDataItem &bld_config);

    void remove(DFHack::color_ostream &out);

    // Ensure the building still exists and is in a valid state. It can disappear
    // for lots of reasons, such as running the game with the buildingplan plugin
    // disabled, manually removing the building, modifying it via the API, etc.
    df::building * getBuildingIfValidOrRemoveIfNot(DFHack::color_ostream &out, bool skip_remove = false);

private:
    DFHack::PersistentDataItem bld_config;
};
