#include "Debug.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <cstdint>

#include "modules/Units.h"
#include "modules/Buildings.h"
#include "modules/Persistence.h"
#include "modules/EventManager.h"
#include "modules/World.h"
#include "modules/Translation.h"

#include "df/world.h"
#include "df/unit.h"
#include "df/building.h"
#include "df/building_civzonest.h"

using namespace DFHack;
using namespace df::enums;


// <BOILERPLATE>
DFHACK_PLUGIN("preserve-tombs");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);


static const std::string CONFIG_KEY = std::string(plugin_name) + "/config";
static PersistentDataItem config;

static int32_t cycle_timestamp;
static constexpr int32_t cycle_freq = 100;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
};

static std::unordered_map<int32_t, int32_t> tomb_assignments;

namespace DFHack {
    DBG_DECLARE(preservetombs, config, DebugCategory::LINFO);
    DBG_DECLARE(preservetombs, cycle, DebugCategory::LINFO);
    DBG_DECLARE(preservetombs, event, DebugCategory::LINFO);
}


static int get_config_val(PersistentDataItem &c, int index) {
    if (!c.isValid())
        return -1;
    return c.ival(index);
}
static bool get_config_bool(PersistentDataItem &c, int index) {
    return get_config_val(c, index) == 1;
}
static void set_config_val(PersistentDataItem &c, int index, int value) {
    if (c.isValid())
        c.ival(index) = value;
}
static void set_config_bool(PersistentDataItem &c, int index, bool value) {
    set_config_val(c, index, value ? 1 : 0);
}

static bool assign_to_tomb(int32_t unit_id, int32_t building_id);
static void update_tomb_assignments(color_ostream& out);
void onUnitDeath(color_ostream& out, void* ptr);
static command_result do_command(color_ostream& out, std::vector<std::string>& params);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        plugin_name,
        "Preserve tomb assignments when assigned units die.",
        do_command));
    return CR_OK;
}

static command_result do_command(color_ostream& out, std::vector<std::string>& params) {
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot use %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }
    if (params.size() == 0 || params[0] == "status") {
        out.print("%s is currently %s\n", plugin_name, is_enabled ? "enabled" : "disabled");
        if (is_enabled) {
            out.print("tracked tomb assignments:\n");
            std::for_each(tomb_assignments.begin(), tomb_assignments.end(), [&out](const auto& p){
                auto& [unit_id, building_id] = p;
                auto* unit = df::unit::find(unit_id);
                std::string name = unit ? Translation::TranslateName(&unit->name) : "UNKNOWN UNIT" ;
                out.print("%s (id %d) -> building %d\n", name.c_str(), unit_id, building_id);
            });
        }
        return CR_OK;
    }
    if (params[0] == "now") {
        if (!is_enabled) {
            out.printerr("Cannot update %s when not enabled", plugin_name);
            return CR_FAILURE;
        }
        CoreSuspender suspend;
        update_tomb_assignments(out);
        out.print("Updated tomb assignments\n");
        return CR_OK;
    }
    return CR_WRONG_USAGE;
}

// event listener
EventManager::EventHandler assign_tomb_handler(onUnitDeath, 0);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot enable %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(config,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
        if (enable) {
            EventManager::registerListener(EventManager::EventType::UNIT_DEATH, assign_tomb_handler, plugin_self);
            update_tomb_assignments(out);
        }
        else {
            tomb_assignments.clear();
            EventManager::unregisterAll(plugin_self);
        }
    } else {
        DEBUG(config,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(config,out).print("shutting down %s\n", plugin_name);

// PluginManager handles unregistering our handler from EventManager,
// so we don't have to do that here
    return CR_OK;
}

DFhackCExport command_result plugin_load_data (color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(config,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
        set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
    }

    is_enabled = get_config_bool(config, CONFIG_IS_ENABLED);
    DEBUG(config,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        tomb_assignments.clear();
        if (is_enabled) {
            DEBUG(config,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
        EventManager::unregisterAll(plugin_self);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (is_enabled && world->frame_counter - cycle_timestamp >= cycle_freq)
        update_tomb_assignments(out);
    return CR_OK;
}
// </BOILERPLATE>




// On unit death - check if we assigned them a tomb
//
//
void onUnitDeath(color_ostream& out, void* ptr) {
    // input is void* that contains the unit id
    int32_t unit_id = reinterpret_cast<std::intptr_t>(ptr);

    // check if unit was assigned a tomb in life
    auto it = tomb_assignments.find(unit_id);
    if (it == tomb_assignments.end()) return;

    // assign that unit to their previously assigned tomb in life
    int32_t building_id = it->second;
    if (!assign_to_tomb(unit_id, building_id)) {
        WARN(event, out).print("Unit %d died - but failed to assign them back to their tomb %d\n", unit_id, building_id);
        return;
    }
    // success, print status update and remove assignment from our memo-list
    INFO(event, out).print("Unit %d died - assigning them back to their tomb\n", unit_id);
    tomb_assignments.erase(it);

}


// Update tomb assignments
//
//
static void update_tomb_assignments(color_ostream &out) {
    cycle_timestamp = world->frame_counter;
    // check tomb civzones for assigned units
    for (auto* bld : world->buildings.other.ZONE_TOMB) {

        auto* tomb = virtual_cast<df::building_civzonest>(bld);
        if (!tomb || !tomb->flags.bits.exists) continue;
        if (!tomb->assigned_unit) continue;
        if (Units::isDead(tomb->assigned_unit)) continue; // we only care about living units

        auto it = tomb_assignments.find(tomb->assigned_unit_id);

        if (it == tomb_assignments.end()) {
            tomb_assignments.emplace(tomb->assigned_unit_id, tomb->id);
            DEBUG(cycle, out).print("%s new tomb assignment, unit %d to tomb %d\n",
                                    plugin_name, tomb->assigned_unit_id, tomb->id);
        }

        else if (it->second != tomb->id) {
            DEBUG(cycle, out).print("%s tomb assignment to %d changed, (old: %d, new: %d)\n",
                                    plugin_name, tomb->assigned_unit_id, it->second, tomb->id);
            it->second = tomb->id;
        }

    }

    // now check our civzones for unassignment / deleted zone
    std::erase_if(tomb_assignments,[&](const auto& p){
        auto &[unit_id, building_id] = p;

        const int tomb_idx = binsearch_index(world->buildings.other.ZONE_TOMB, building_id);
        if (tomb_idx == -1) {
            DEBUG(cycle, out).print("%s tomb missing: %d - removing\n", plugin_name, building_id);
            return true;
        }
        const auto tomb = virtual_cast<df::building_civzonest>(world->buildings.other.ZONE_TOMB[tomb_idx]);
        if (!tomb || !tomb->flags.bits.exists) {
            DEBUG(cycle, out).print("%s tomb missing: %d - removing\n", plugin_name, building_id);
            return true;
        }
        if (tomb->assigned_unit_id != unit_id) {
            DEBUG(cycle, out).print("%s unit %d unassigned from tomb %d - removing\n", plugin_name, unit_id, building_id);
            return true;
        }

        return false;
    });

}


// ASSIGN UNIT TO TOMB
//
//
static bool assign_to_tomb(int32_t unit_id, int32_t building_id) {

    df::unit* unit = df::unit::find(unit_id);

    if (!unit || !Units::isDead(unit)) return false;

    const int tomb_idx = binsearch_index(world->buildings.other.ZONE_TOMB, building_id);
    if (tomb_idx == -1) return false;

    df::building_civzonest* tomb = virtual_cast<df::building_civzonest>(world->buildings.other.ZONE_TOMB[tomb_idx]);
    if (!tomb || tomb->assigned_unit) return false;

    Buildings::setOwner(tomb, unit);
    return true;
}
