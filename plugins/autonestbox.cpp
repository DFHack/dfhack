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

static bool isEmptyPasture(df::building *building) {
    if (!Buildings::isPenPasture(building))
        return false;
    df::building_civzonest *civ = (df::building_civzonest *)building;
    return (civ->assigned_units.size() == 0);
}

static bool isFreeNestboxAtPos(int32_t x, int32_t y, int32_t z) {
    for (auto building : world->buildings.all) {
        if (building->getType() == df::building_type::NestBox
                && building->x1 == x
                && building->y1 == y
                && building->z  == z) {
            df::building_nest_boxst *nestbox = (df::building_nest_boxst *)building;
            if (nestbox->claimed_by == -1 && nestbox->contained_items.size() == 1) {
                return true;
            }
        }
    }
    return false;
}

static df::building* findFreeNestboxZone() {
    for (auto building : world->buildings.all) {
        if (isEmptyPasture(building) &&
                Buildings::isActive(building) &&
                isFreeNestboxAtPos(building->x1, building->y1, building->z)) {
            return building;
        }
    }
    return NULL;
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

static bool isFreeEgglayer(df::unit *unit)
{
    return Units::isActive(unit) && !Units::isUndead(unit)
        && Units::isFemale(unit)
        && Units::isTame(unit)
        && Units::isOwnCiv(unit)
        && Units::isEggLayer(unit)
        && !isAssigned(unit)
        && !Units::isGrazer(unit) // exclude grazing birds because they're messy
        && !Units::isMerchant(unit) // don't steal merchant mounts
        && !Units::isForest(unit);  // don't steal birds from traders, they hate that
}

static df::unit * findFreeEgglayer() {
    for (auto unit : world->units.all) {
        if (isFreeEgglayer(unit))
            return unit;
    }
    return NULL;
}

static df::general_ref_building_civzone_assignedst * createCivzoneRef() {
    static bool vt_initialized = false;

    // after having run successfully for the first time it's safe to simply create the object
    if (vt_initialized) {
        return (df::general_ref_building_civzone_assignedst *)
            df::general_ref_building_civzone_assignedst::_identity.instantiate();
    }

    // being called for the first time, need to initialize the vtable
    for (auto creature : world->units.all) {
        for (auto ref : creature->general_refs) {
            if (ref->getType() == df::general_ref_type::BUILDING_CIVZONE_ASSIGNED) {
                if (strict_virtual_cast<df::general_ref_building_civzone_assignedst>(ref)) {
                    vt_initialized = true;
                    // !! calling new() doesn't work, need _identity.instantiate() instead !!
                    return (df::general_ref_building_civzone_assignedst *)
                        df::general_ref_building_civzone_assignedst::_identity.instantiate();
                }
            }
        }
    }
    return NULL;
}

static bool assignUnitToZone(color_ostream &out, df::unit *unit, df::building *building) {
    // try to get a fresh civzone ref
    df::general_ref_building_civzone_assignedst *ref = createCivzoneRef();
    if (!ref) {
        ERR(cycle,out).print("Could not find a clonable activity zone reference!"
            " You need to manually pen/pasture/pit at least one creature"
            " before autonestbox can function.\n");
        return false;
    }

    ref->building_id = building->id;
    unit->general_refs.push_back(ref);

    df::building_civzonest *civz = (df::building_civzonest *)building;
    civz->assigned_units.push_back(unit->id);

    INFO(cycle,out).print("Unit %d (%s) assigned to nestbox zone %d (%s)\n",
        unit->id, Units::getRaceName(unit).c_str(),
        building->id, building->name.c_str());

    return true;
}

static size_t countFreeEgglayers() {
    size_t count = 0;
    for (auto unit : world->units.all) {
        if (isFreeEgglayer(unit))
            ++count;
    }
    return count;
}

static size_t assign_nestboxes(color_ostream &out) {
    size_t processed = 0;
    df::building *free_building = NULL;
    df::unit *free_unit = NULL;
    do {
        free_building = findFreeNestboxZone();
        free_unit = findFreeEgglayer();
        if (free_building && free_unit) {
            if (!assignUnitToZone(out, free_unit, free_building)) {
                DEBUG(cycle,out).print("Failed to assign unit to building.\n");
                return processed;
            }
            DEBUG(cycle,out).print("assigned unit %d to zone %d\n",
                                   free_unit->id, free_building->id);
            ++processed;
        }
    } while (free_unit && free_building);

    if (free_unit && !free_building) {
        static size_t old_count = 0;
        size_t freeEgglayers = countFreeEgglayers();
        // avoid spamming the same message
        if (old_count != freeEgglayers)
            did_complain = false;
        old_count = freeEgglayers;
        if (!did_complain) {
            std::stringstream ss;
            ss << freeEgglayers;
            string announce = "Not enough free nestbox zones found! You need " + ss.str() + " more.";
            Gui::showAnnouncement(announce, 6, true);
            out << announce << std::endl;
            did_complain = true;
        }
    }
    return processed;
}

static void autonestbox_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running autonestbox cycle\n");

    size_t processed = assign_nestboxes(out);
    if (processed > 0) {
        std::stringstream ss;
        ss << processed << " nestboxes were assigned.";
        string announce = ss.str();
        DEBUG(cycle,out).print("%s\n", announce.c_str());
        Gui::showAnnouncement(announce, 2, false);
        out << announce << std::endl;
        // can complain again
        // (might lead to spamming the same message twice, but catches the case
        // where for example 2 new egglayers hatched right after 2 zones were created and assigned)
        did_complain = false;
    }
}
