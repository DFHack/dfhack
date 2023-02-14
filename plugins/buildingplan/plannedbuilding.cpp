#include "plannedbuilding.h"
#include "buildingplan.h"

#include "Debug.h"

#include "modules/World.h"

namespace DFHack {
    DBG_EXTERN(buildingplan, status);
    DBG_EXTERN(buildingplan, cycle);
}

using namespace DFHack;

PlannedBuilding::PlannedBuilding(color_ostream &out, df::building *building)
        : id(building->id) {
    DEBUG(status,out).print("creating persistent data for building %d\n", id);
    bld_config = World::AddPersistentData(BLD_CONFIG_KEY);
    set_config_val(bld_config, BLD_CONFIG_ID, id);
}

PlannedBuilding::PlannedBuilding(PersistentDataItem &bld_config)
    : id(get_config_val(bld_config, BLD_CONFIG_ID)), bld_config(bld_config) { }

// Ensure the building still exists and is in a valid state. It can disappear
// for lots of reasons, such as running the game with the buildingplan plugin
// disabled, manually removing the building, modifying it via the API, etc.
df::building * PlannedBuilding::getBuildingIfValidOrRemoveIfNot(color_ostream &out) {
    auto bld = df::building::find(id);
    bool valid = bld && bld->getBuildStage() == 0;
    if (!valid) {
        remove(out);
        return NULL;
    }
    return bld;
}
