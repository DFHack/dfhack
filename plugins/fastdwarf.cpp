#include "Debug.h"
#include "PluginManager.h"

#include "modules/Maps.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/unit_relationship_type.h"

using std::string;
using std::vector;
using namespace DFHack;

DFHACK_PLUGIN("fastdwarf");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);
using df::global::debug_turbospeed;  // soft dependency, so not REQUIRE_GLOBAL

namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(fastdwarf, control, DebugCategory::LINFO);
    // for logging during the update cycle
    DBG_DECLARE(fastdwarf, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_FAST = 0,
    CONFIG_TELE = 1,
};

static command_result do_command(color_ostream &out, vector<string> & parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> & commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        "fastdwarf",
        "Citizens walk fast and and finish jobs instantly.",
        do_command));

    return CR_OK;
}

static void set_state(color_ostream &out, int fast, int tele) {
    DEBUG(control,out).print("setting state: fast=%d, tele=%d\n", fast, tele);
    is_enabled = fast || tele;
    config.set_int(CONFIG_FAST, fast);
    config.set_int(CONFIG_TELE, tele);

    if (debug_turbospeed)
        *debug_turbospeed = fast == 2;
    else if (fast == 2)
        out.printerr("Speed level 2 not available.\n");
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot enable %s without a loaded map.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        set_state(out, enable ? 1 : 0, 0);
    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(control,out).print("shutting down %s\n", plugin_name);

    // make sure the debug flag doesn't get left on
    if (debug_turbospeed)
        *debug_turbospeed = false;

    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        set_state(out, false, false);
    } else {
        is_enabled = config.get_int(CONFIG_FAST) || config.get_int(CONFIG_TELE);
        DEBUG(control,out).print("loading persisted state: fast=%d, tele=%d\n",
            config.get_int(CONFIG_FAST),
            config.get_int(CONFIG_TELE));
    }

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

static void do_tele(color_ostream &out, df::unit * unit) {
    // skip dwarves that are dragging creatures or being dragged
    if ((unit->relationship_ids[df::unit_relationship_type::Draggee] != -1) ||
        (unit->relationship_ids[df::unit_relationship_type::Dragger] != -1))
        return;

    // skip dwarves that are following other units
    if (unit->following != 0)
        return;

    // skip unconscious units
    if (unit->counters.unconscious > 0)
        return;

    // don't do anything if the dwarf isn't going anywhere
    if (!unit->pos.isValid() || !unit->path.dest.isValid() || unit->pos == unit->path.dest)
        return;

    // skip units with no current job so they can still meander and fulfill needs
    if (!unit->job.current_job)
        return;

    // don't do anything if the destination is in a different pathability group or is unrevealed
    if (!Maps::canWalkBetween(unit->pos, unit->path.dest))
        return;
    if (!Maps::isTileVisible(unit->path.dest))
        return;

    DEBUG(cycle,out).print("teleporting unit %d\n", unit->id);

    if (!Units::teleport(unit, unit->path.dest))
        return;

    unit->path.path.clear();
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    // fast mode 2 is handled by DF itself
    bool is_fast = config.get_int(CONFIG_FAST) == 1;
    bool is_tele = config.get_int(CONFIG_TELE) == 1;

    std::vector<df::unit *> citizens;
    Units::getCitizens(citizens);
    for (auto & unit : citizens) {
        if (is_tele)
            do_tele(out, unit);

        if (is_fast) {
            DEBUG(cycle,out).print("fastifying unit %d\n", unit->id);
            Units::setGroupActionTimers(out, unit, 1, df::unit_action_type_group::All);
        }
    }

    return CR_OK;
}

/////////////////////////////////////////////////////
// CLI logic
//

static command_result do_command(color_ostream &out, vector <string> & parameters)
{
    if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot run %s without a loaded map.\n", plugin_name);
        return CR_FAILURE;
    }

    const size_t num_params = parameters.size();

    if (num_params > 2 || (num_params > 1 && parameters[0] == "help"))
        return CR_WRONG_USAGE;

    if (num_params == 0 || parameters[0] == "status") {
        out.print("Current state: fast = %d, teleport = %d.\n",
            config.get_int(CONFIG_FAST),
            config.get_int(CONFIG_TELE));
        return CR_OK;
    }

    int fast = string_to_int(parameters[0]);
    int tele = num_params >= 2 ? string_to_int(parameters[1]) : 0;

    if (fast < 0 || fast > 2) {
        out.printerr("Invalid value for fast: '%s'", parameters[0].c_str());
        return CR_WRONG_USAGE;
    }
    if (tele < 0 || tele > 1) {
        out.printerr("Invalid value for tele: '%s'", parameters[1].c_str());
        return CR_WRONG_USAGE;
    }

    set_state(out, fast, tele);

    return CR_OK;
}
