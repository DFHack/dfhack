#pragma once

#include "Core.h"

#include "modules/Persistence.h"

#include "df/building.h"

class PlannedBuilding {
public:
    const df::building::key_field_type id;

    PlannedBuilding(DFHack::color_ostream &out, df::building *building);
    PlannedBuilding(DFHack::PersistentDataItem &bld_config);

    void remove(DFHack::color_ostream &out);

    // Ensure the building still exists and is in a valid state. It can disappear
    // for lots of reasons, such as running the game with the buildingplan plugin
    // disabled, manually removing the building, modifying it via the API, etc.
    df::building * getBuildingIfValidOrRemoveIfNot(DFHack::color_ostream &out);

private:
    DFHack::PersistentDataItem bld_config;
};
