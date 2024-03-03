// - full automation of handling mini-pastures over nestboxes:
//   go through all pens, check if they are empty and placed over a nestbox
//   find female tame egg-layer who is not assigned to another pen and assign it to nestbox pasture
//   maybe check for minimum age? it's not that useful to fill nestboxes with freshly hatched birds
//   state and sleep setting is saved the first time autonestbox is started (to avoid writing stuff if the plugin is never used)

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/building_cagest.h"
#include "df/building_civzonest.h"
#include "df/building_nest_boxst.h"
#include "df/general_ref_building_civzone_assignedst.h"
#include "df/world.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("autonestbox");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(cur_season);
REQUIRE_GLOBAL(world);

namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(autonestbox, control, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(autonestbox, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;
enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
};

static bool did_complain = false; // avoids message spam
static const int32_t CYCLE_TICKS = 6067;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result df_autonestbox(color_ostream &out, vector<string> &parameters);
static void autonestbox_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        plugin_name,
        "Auto-assign egg-laying female pets to nestbox zones.",
        df_autonestbox));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::IsSiteLoaded()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
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

    // we have to copy our enabled flag into the global plugin variable, but
    // all the other state we can directly read/modify from the persistent
    // data structure.
    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");
    did_complain = false;
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
    if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        autonestbox_cycle(out);
    return CR_OK;
}

/////////////////////////////////////////////////////
// configuration interface
//

struct autonestbox_options {
    // whether to display help
    bool help = false;

    // whether to run a cycle right now
    bool now = false;

    static struct_identity _identity;
};
static const struct_field_info autonestbox_options_fields[] = {
    { struct_field_info::PRIMITIVE, "help",  offsetof(autonestbox_options, help),  &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "now",   offsetof(autonestbox_options, now),   &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::END }
};
struct_identity autonestbox_options::_identity(sizeof(autonestbox_options), &df::allocator_fn<autonestbox_options>, NULL, "autonestbox_options", NULL, autonestbox_options_fields);

static bool get_options(color_ostream &out,
                        autonestbox_options &opts,
                        const vector<string> &parameters)
{
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, parameters.size() + 2) ||
        !Lua::PushModulePublic(
            out, L, "plugins.autonestbox", "parse_commandline")) {
        out.printerr("Failed to load autonestbox Lua code\n");
        return false;
    }

    Lua::Push(L, &opts);
    for (const string &param : parameters)
        Lua::Push(L, param);

    if (!Lua::SafeCall(out, L, parameters.size() + 1, 0))
        return false;

    return true;
}

static command_result df_autonestbox(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isMapLoaded() || !World::IsSiteLoaded()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    autonestbox_options opts;
    if (!get_options(out, opts, parameters) || opts.help)
        return CR_WRONG_USAGE;

    if (opts.now) {
        did_complain = false;
        autonestbox_cycle(out);
    }
    else {
        out << "autonestbox is " << (is_enabled ? "" : "not ") << "running" << std::endl;
    }
    return CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

static bool isEmptyPasture(df::building_civzonest *zone) {
    if (!Buildings::isPenPasture(zone))
        return false;
    return (zone->assigned_units.size() == 0);
}

static bool isInBuiltCage(df::unit *unit) {
    for (auto building : world->buildings.all) {
        if (building->getType() == df::building_type::Cage) {
            df::building_cagest* cage = (df::building_cagest *)building;
            for (auto unitid : cage->assigned_units) {
                if (unitid == unit->id)
                    return true;
            }
        }
    }
    return false;
}

// check if assigned to pen, pit, (built) cage or chain
// note: BUILDING_CAGED is not set for animals (maybe it's used for dwarves who get caged as sentence)
// animals in cages (no matter if built or on stockpile) get the ref CONTAINED_IN_ITEM instead
// removing them from cages on stockpiles is no problem even without clearing the ref
// and usually it will be desired behavior to do so.
static bool isAssigned(df::unit *unit) {
    for (auto ref : unit->general_refs) {
        auto rtype = ref->getType();
        if(rtype == df::general_ref_type::BUILDING_CIVZONE_ASSIGNED
                || rtype == df::general_ref_type::BUILDING_CAGED
                || rtype == df::general_ref_type::BUILDING_CHAIN
                || (rtype == df::general_ref_type::CONTAINED_IN_ITEM && isInBuiltCage(unit))) {
            return true;
        }
    }
    return false;
}

static bool unlikely_to_revert_to_wild(df::unit *unit) {
    if (Units::isDomesticated(unit))
        return true;
    return Units::isTame(unit) && Units::isMarkedForTraining(unit);
}

static bool isFreeEgglayer(df::unit *unit) {
    return Units::isActive(unit)
        && !Units::isUndead(unit)
        && Units::isFemale(unit)
        && unlikely_to_revert_to_wild(unit)
        && Units::isOwnCiv(unit)
        && Units::isEggLayer(unit)
        && !isAssigned(unit)
        && !Units::isGrazer(unit) // exclude grazing birds because they're messy
        && !Units::isMerchant(unit) // don't steal merchant mounts
        && !Units::isForest(unit);  // don't steal birds from traders, they hate that
}

static df::general_ref_building_civzone_assignedst * createCivzoneRef() {
    return (df::general_ref_building_civzone_assignedst *)
        df::general_ref_building_civzone_assignedst::_identity.instantiate();
}

static bool assignUnitToZone(color_ostream &out, df::unit *unit, df::building_civzonest *zone) {
    auto ref = createCivzoneRef();
    if (!ref) {
        ERR(cycle,out).print("Could not create activity zone reference!\n");
        return false;
    }

    ref->building_id = zone->id;
    unit->general_refs.push_back(ref);
    zone->assigned_units.push_back(unit->id);

    INFO(cycle,out).print("Unit %d (%s) assigned to nestbox zone %d (%s)\n",
        unit->id, Units::getRaceName(unit).c_str(),
        zone->id, zone->name.c_str());

    return true;
}

// also assigns units to zone if they have already claimed the nestbox
// and are not assigned elsewhere; returns number assigned
static size_t getFreeNestboxZones(color_ostream &out, vector<df::building_civzonest *> &free_zones) {
    size_t assigned = 0;
    for (auto zone : world->buildings.other.ZONE_PEN) {
        if (!isEmptyPasture(zone) || !Buildings::isActive(zone))
            continue;

        // nestbox must be in upper left corner
        // this could be made more flexible
        df::coord pos(zone->x1, zone->y1, zone->z);
        auto bld = Buildings::findAtTile(pos);
        if (!bld || bld->getType() != df::building_type::NestBox)
            continue;

        df::building_nest_boxst *nestbox = virtual_cast<df::building_nest_boxst>(bld);
        if (!nestbox)
            continue;

        if (nestbox->claimed_by >= 0) {
            auto unit = df::unit::find(nestbox->claimed_by);
            if (isFreeEgglayer(unit)) {
                // if the nestbox is claimed by a free egg layer, attempt to assign that unit to the zone
                if (assignUnitToZone(out, unit, zone))
                    ++assigned;
            }
        } else if (nestbox->contained_items.size() == 1) {
            free_zones.push_back(zone);
        }
    }
    return assigned;
}

static vector<df::unit *> getFreeEggLayers(color_ostream &out) {
    vector<df::unit *> ret;
    for (auto unit : world->units.active) {
        if (isFreeEgglayer(unit))
            ret.push_back(unit);
    }
    return ret;
}

// rate limit to one message per season
// assumes this gets run at least once a season, which is true when enabled
// running manually with the "now" param also resets did_complain, so we
// won't miss the case where the player happens to run "autonestbox now"
// once a year in Spring
static void rate_limit_complaining() {
    static df::season old_season = df::season::None;
    df::season this_season = *cur_season;
    if (old_season != this_season)
        did_complain = false;
    old_season = this_season;
}

static size_t assign_nestboxes(color_ostream &out) {
    rate_limit_complaining();

    vector<df::building_civzonest *> free_zones;
    size_t assigned = getFreeNestboxZones(out, free_zones);
    vector<df::unit *> free_units = getFreeEggLayers(out);

    const size_t max_idx = std::min(free_zones.size(), free_units.size());
    for (size_t idx = 0; idx < max_idx; ++idx) {
        if (!assignUnitToZone(out, free_units[idx], free_zones[idx])) {
            DEBUG(cycle,out).print("Failed to assign unit to building.\n");
            return assigned;
        }
        DEBUG(cycle,out).print("assigned unit %d to zone %d\n",
                                free_units[idx]->id, free_zones[idx]->id);
        ++assigned;
    }

    if (free_zones.size() < free_units.size() && !did_complain) {
        size_t num_needed = free_units.size() - free_zones.size();
        std::stringstream ss;
        ss << "Not enough free nestbox zones! You need to make " << num_needed << " more 1x1 pastures and build nestboxes in them.";
        string announce = ss.str();
        out << announce << std::endl;
        Gui::showAnnouncement("[DFHack autonestbox] " + announce, COLOR_BROWN, true);
        did_complain = true;
    }
    return assigned;
}

static void autonestbox_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running autonestbox cycle\n");

    size_t assigned = assign_nestboxes(out);
    if (assigned > 0) {
        std::stringstream ss;
        ss << assigned << " nestbox" << (assigned == 1 ? " was" : "es were") << " assigned to roaming egg layers.";
        string announce = ss.str();
        out << announce << std::endl;
        Gui::showAnnouncement("[DFHack autonestbox] " + announce, COLOR_GREEN, false);
        // can complain again
        did_complain = false;
    }
}
