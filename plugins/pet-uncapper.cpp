#include "Debug.h"
#include "PluginManager.h"

#include "modules/Maps.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/unit.h"
#include "df/world.h"

#include <random>

using namespace DFHack;
using std::map;
using std::string;
using std::vector;

DFHACK_PLUGIN("pet-uncapper");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(petuncapper, control, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(petuncapper, cycle, DebugCategory::LINFO);
}

static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_FREQ = 1,
    CONFIG_POP_CAP = 2,
    CONFIG_PREG_TIME = 3,
};

bool impregnate(df::unit* female, df::unit* male) {
    if (!female || !male)
        return false;
    if (female->pregnancy_genes)
        return false;

    df::unit_genes* preg = new df::unit_genes;
    *preg = male->appearance.genes;
    female->pregnancy_genes = preg;
    female->pregnancy_timer = config.get_int(CONFIG_PREG_TIME);
    female->pregnancy_caste = male->caste;
    return true;
}

void impregnateMany(color_ostream &out, bool verbose = false) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    map<int32_t, vector<df::unit *>> males;
    map<int32_t, vector<df::unit *>> females;
    map<int32_t, int32_t> popcount;

    std::random_device seed;
    std::mt19937 gen{seed()};

    const int popcap = config.get_int(CONFIG_POP_CAP);
    int pregnancies = 0;

    for (auto unit : world->units.active) {
        // not restricted to fort pets since merchant/wild animals can participate
        if (!Units::isActive(unit) || unit->flags1.bits.active_invader || unit->flags2.bits.underworld || unit->flags2.bits.visitor_uninvited || unit->flags2.bits.visitor)
            continue;
        popcount[unit->race]++;
        if (unit->pregnancy_genes) {
            // already pregnant -- if remaining time is less than the current setting, speed it up
            if ((int)unit->pregnancy_timer > config.get_int(CONFIG_PREG_TIME))
                unit->pregnancy_timer = config.get_int(CONFIG_PREG_TIME);
            // for player convenience and population stability, count the fetus toward the population cap
            popcount[unit->race]++;
            continue;
        }
        if (unit->flags1.bits.caged)
            continue;
        // must have PET or PET_EXOTIC
        if (!Units::isTamable(unit))
            continue;
        // check for adulthood
        if (Units::isBaby(unit) || Units::isChild(unit))
            continue;
        if (Units::isMale(unit))
            males[unit->race].push_back(unit);
        else if (Units::isFemale(unit))
            females[unit->race].push_back(unit);
    }

    for (auto [race, femalesList] : females) {
        if (!males.contains(race))
            continue;

        for (auto female : femalesList) {
            if (popcap > 0 && popcount[race] >= popcap)
                break;

            vector<df::unit *> compatibles;
            for (auto male : males[race]) {
                if (Maps::canWalkBetween(female->pos, male->pos) )
                    compatibles.push_back(male);
            }
            if (compatibles.empty())
                continue;

            std::uniform_int_distribution<> dist{0, (int)compatibles.size() - 1};
            if (impregnate(female, compatibles[dist(gen)])) {
                pregnancies++;
                popcount[race]++;
            }
        }
    }

    if (pregnancies || verbose) {
        INFO(cycle, out).print("%d pet pregnanc%s initiated\n",
            pregnancies, pregnancies == 1 ? "y" : "ies");
    }
}

command_result do_command(color_ostream &out, vector<string> & parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (parameters.size() == 0 || parameters[0] == "status") {
        out.print("%s is %s\n\n", plugin_name, is_enabled ? "enabled" : "not enabled");
        out.print("population cap per species: %d\n", config.get_int(CONFIG_POP_CAP));
        out.print("updating pregnancies every %d ticks\n", config.get_int(CONFIG_FREQ));
        out.print("pregancies last %d ticks\n", config.get_int(CONFIG_PREG_TIME));
    } else if (parameters[0] == "now") {
        impregnateMany(out, true);
    } else {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;
        string command = parameters[0];
        int val = std::max(0, string_to_int(parameters[1]));
        if (command == "cap")
            config.set_int(CONFIG_POP_CAP, val);
        else if (command == "every")
            config.set_int(CONFIG_FREQ, val);
        else if (command == "pregtime")
            config.set_int(CONFIG_PREG_TIME, val);
        else
            return CR_WRONG_USAGE;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "pet-uncapper",
        "Modify the pet population cap.",
        do_command));
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
            impregnateMany(out);
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
        config.set_int(CONFIG_FREQ, 10000);
        config.set_int(CONFIG_POP_CAP, 100);
        config.set_int(CONFIG_PREG_TIME, 200000);
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
    if (world->frame_counter - cycle_timestamp >= config.get_int(CONFIG_FREQ))
        impregnateMany(out);
    return CR_OK;
}
