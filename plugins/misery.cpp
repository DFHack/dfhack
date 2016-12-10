#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Units.h"

#include "df/emotion_type.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/unit_personality.h"
#include "df/unit_soul.h"
#include "df/unit_thought_type.h"
#include "df/world.h"

using namespace std;
using namespace DFHack;

DFHACK_PLUGIN("misery");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(cur_year);
REQUIRE_GLOBAL(cur_year_tick);

typedef df::unit_personality::T_emotions Emotion;

static int factor = 1;
static int tick = 0;
const int INTERVAL = 1000;

command_result misery(color_ostream& out, vector<string>& parameters);
void add_misery(df::unit *unit);
void clear_misery(df::unit *unit);

const int FAKE_EMOTION_FLAG = (1 << 30);
const int STRENGTH_MULTIPLIER = 100;

bool is_valid_unit (df::unit *unit) {
    if (!Units::isOwnRace(unit) || !Units::isOwnCiv(unit))
        return false;
    if (Units::isDead(unit))
        return false;
    return true;
}

inline bool is_fake_emotion (Emotion *e) {
    return e->flags.whole & FAKE_EMOTION_FLAG;
}

void add_misery (df::unit *unit) {
    // Add a fake miserable thought
    // Remove any fake ones that already exist
    if (!unit || !unit->status.current_soul)
        return;
    clear_misery(unit);
    auto &emotions = unit->status.current_soul->personality.emotions;
    Emotion *e = new Emotion;
    e->type = df::emotion_type::MISERY;
    e->thought = df::unit_thought_type::SoapyBath;
    e->flags.whole |= FAKE_EMOTION_FLAG;
    emotions.push_back(e);

    for (Emotion *e : emotions) {
        if (is_fake_emotion(e)) {
            e->year = *cur_year;
            e->year_tick = *cur_year_tick;
            e->strength = STRENGTH_MULTIPLIER * factor;
            e->severity = STRENGTH_MULTIPLIER * factor;
        }
    }
}

void clear_misery (df::unit *unit) {
    if (!unit || !unit->status.current_soul)
        return;
    auto &emotions = unit->status.current_soul->personality.emotions;
    auto it = remove_if(emotions.begin(), emotions.end(), [](Emotion *e) {
        if (is_fake_emotion(e)) {
            delete e;
            return true;
        }
        return false;
    });
    emotions.erase(it, emotions.end());
}

DFhackCExport command_result plugin_shutdown(color_ostream& out) {
    factor = 0;
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream& out) {
    static bool wasLoaded = false;
    if ( factor == 0 || !world || !world->map.block_index ) {
        if ( wasLoaded ) {
            //we just unloaded the game: clear all data
            factor = 0;
            is_enabled = false;
            wasLoaded = false;
        }
        return CR_OK;
    }

    if ( !wasLoaded ) {
        wasLoaded = true;
    }

    if ( tick < INTERVAL ) {
        tick++;
        return CR_OK;
    }
    tick = 0;

    //TODO: consider units.active
    for (df::unit *unit : world->units.all) {
        if (is_valid_unit(unit)) {
            add_misery(unit);
        }
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream& out, vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("misery", "increase the intensity of negative dwarven thoughts",
        &misery, false,
        "misery: When enabled, every new negative dwarven thought will be multiplied by a factor (2 by default).\n"
        "Usage:\n"
        "  misery enable n\n"
        "    enable misery with optional magnitude n. If specified, n must be positive.\n"
        "  misery n\n"
        "    same as \"misery enable n\"\n"
        "  misery enable\n"
        "    same as \"misery enable 2\"\n"
        "  misery disable\n"
        "    stop adding new negative thoughts. This will not remove existing duplicated thoughts. Equivalent to \"misery 1\"\n"
        "  misery clear\n"
        "    remove fake thoughts added in this session of DF. Saving makes them permanent! Does not change factor.\n\n"
        ));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (enable != is_enabled)
    {
        is_enabled = enable;
        factor = enable ? 1 : 0;
        tick = INTERVAL;
    }

    return CR_OK;
}

command_result misery(color_ostream &out, vector<string>& parameters) {
    if ( !world || !world->map.block_index ) {
        out.printerr("misery can only be enabled in fortress mode with a fully-loaded game.\n");
        return CR_FAILURE;
    }

    if ( parameters.size() < 1 || parameters.size() > 2 ) {
        return CR_WRONG_USAGE;
    }

    if ( parameters[0] == "disable" ) {
        if ( parameters.size() > 1 ) {
            return CR_WRONG_USAGE;
        }
        factor = 0;
        is_enabled = false;
        return CR_OK;
    } else if ( parameters[0] == "enable" ) {
        is_enabled = true;
        factor = 1;
        if ( parameters.size() == 2 ) {
            int a = atoi(parameters[1].c_str());
            if ( a < 1 ) {
                out.printerr("Second argument must be a positive integer.\n");
                return CR_WRONG_USAGE;
            }
            factor = a;
        }
        tick = INTERVAL;
    } else if ( parameters[0] == "clear" ) {
        for (df::unit *unit : world->units.all) {
            if (is_valid_unit(unit)) {
                clear_misery(unit);
            }
        }
    } else {
        int a = atoi(parameters[0].c_str());
        if ( a < 0 ) {
            return CR_WRONG_USAGE;
        }
        factor = a;
        is_enabled = factor > 0;
    }

    return CR_OK;
}

