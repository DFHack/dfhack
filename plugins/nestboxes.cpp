#include "Debug.h"
#include "PluginManager.h"

#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Persistence.h"
#include "modules/World.h"

#include "df/buildingitemst.h"
#include "df/building_nest_boxst.h"
#include "df/item.h"
#include "df/item_eggst.h"
#include "df/unit.h"
#include "df/world.h"

using std::string;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("nestboxes");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(nestboxes, control, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(nestboxes, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
};

static const int32_t CYCLE_TICKS = 7; // need to react quickly when eggs are laid/unforbidden
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_cycle(out);
    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(control,out).print("shutting down %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
    }

    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(control,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

static void do_cycle(color_ostream &out) {
    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    for (df::building_nest_boxst *nb : world->buildings.other.NEST_BOX) {
        for (auto &contained_item : nb->contained_items) {
            if (contained_item->use_mode == df::building_item_role_type::PERM)
                continue;
            if (auto *item = virtual_cast<df::item_eggst>(contained_item->item)) {
                bool fertile = item->egg_flags.bits.fertile;
                if (item->flags.bits.forbid == fertile)
                    continue;
                item->flags.bits.forbid = fertile;
                if (fertile && item->flags.bits.in_job) {
                    // cancel any job involving the egg
                    df::specific_ref *sref = Items::getSpecificRef(
                            item, df::specific_ref_type::JOB);
                    if (sref && sref->data.job)
                        Job::removeJob(sref->data.job);
                }
                out.print("nestboxes: %d eggs %s\n", item->getStackSize(), fertile ? "forbidden" : "unforbidden");
            }
        }
    }
}
