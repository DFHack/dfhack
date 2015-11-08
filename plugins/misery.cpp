#include "PluginManager.h"
#include "Export.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/unit_thought.h"
#include "df/unit_thought_type.h"

#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace DFHack;
/*
misery
======
When enabled, every new negative dwarven thought will be multiplied by a factor (2 by default).

Usage:

:misery enable n:  enable misery with optional magnitude n. If specified, n must be positive.
:misery n:         same as "misery enable n"
:misery enable:    same as "misery enable 2"
:misery disable:   stop adding new negative thoughts. This will not remove existing
                   duplicated thoughts. Equivalent to "misery 1"
:misery clear:     remove fake thoughts added in this session of DF. Saving makes them
                   permanent! Does not change factor.
*/

DFHACK_PLUGIN("misery");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);

static int factor = 1;
static map<int, int> processedThoughtCountTable;

//keep track of fake thoughts so you can remove them if requested
static vector<std::pair<int,int> > fakeThoughts;
static int count;
const int maxCount = 1000;

command_result misery(color_ostream& out, vector<string>& parameters);

DFhackCExport command_result plugin_shutdown(color_ostream& out) {
    factor = 1;
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream& out) {
    static bool wasLoaded = false;
    if ( factor == 1 || !world || !world->map.block_index ) {
        if ( wasLoaded ) {
            //we just unloaded the game: clear all data
            factor = 1;
            is_enabled = false;
            processedThoughtCountTable.clear();
            fakeThoughts.clear();
            wasLoaded = false;
        }
        return CR_OK;
    }

    if ( !wasLoaded ) {
        wasLoaded = true;
    }

    if ( count < maxCount ) {
        count++;
        return CR_OK;
    }
    count = 0;

    int32_t race_id = ui->race_id;
    int32_t civ_id = ui->civ_id;
    for ( size_t a = 0; a < world->units.all.size(); a++ ) {
        df::unit* unit = world->units.all[a]; //TODO: consider units.active
        //living, native units only
        if ( unit->race != race_id || unit->civ_id != civ_id )
            continue;
        if ( unit->flags1.bits.dead )
            continue;

        int processedThoughtCount;
        map<int,int>::iterator i = processedThoughtCountTable.find(unit->id);
        if ( i == processedThoughtCountTable.end() ) {
            processedThoughtCount = unit->status.recent_events.size();
            processedThoughtCountTable[unit->id] = processedThoughtCount;
        } else {
            processedThoughtCount = (*i).second;
        }

        if ( processedThoughtCount == unit->status.recent_events.size() ) {
            continue;
        } else if ( processedThoughtCount > unit->status.recent_events.size() ) {
            processedThoughtCount = unit->status.recent_events.size();
        }

        //don't reprocess any old thoughts
        vector<df::unit_thought*> newThoughts;
        for ( size_t b = processedThoughtCount; b < unit->status.recent_events.size(); b++ ) {
            df::unit_thought* oldThought = unit->status.recent_events[b];
            const char* bob = ENUM_ATTR(unit_thought_type, value, oldThought->type);
            if ( bob[0] != '-' ) {
                //out.print("unit %4d: old thought value = %s\n", unit->id, bob);
                continue;
            }
            /*out.print("unit %4d: Duplicating thought type %d (%s), value %s, age %d, subtype %d, severity %d\n",
                unit->id,
                oldThought->type.value,
                ENUM_ATTR(unit_thought_type, caption, (oldThought->type)),
                //df::enum_traits<df::unit_thought_type>::attr_table[oldThought->type].caption
                bob,
                oldThought->age,
                oldThought->subtype,
                oldThought->severity
            );*/
            //add duplicate thoughts to the temp list
            for ( size_t c = 0; c < factor; c++ ) {
                df::unit_thought* thought = new df::unit_thought;
                thought->type     = unit->status.recent_events[b]->type;
                thought->age      = unit->status.recent_events[b]->age;
                thought->subtype  = unit->status.recent_events[b]->subtype;
                thought->severity = unit->status.recent_events[b]->severity;
                newThoughts.push_back(thought);
            }
        }
        for ( size_t b = 0; b < newThoughts.size(); b++ ) {
            fakeThoughts.push_back(std::pair<int, int>(a, unit->status.recent_events.size()));
            unit->status.recent_events.push_back(newThoughts[b]);
        }
        processedThoughtCountTable[unit->id] = unit->status.recent_events.size();
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
        factor = enable ? 2 : 1;
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
        factor = 1;
        is_enabled = false;
        return CR_OK;
    } else if ( parameters[0] == "enable" ) {
        is_enabled = true;
        factor = 2;
        if ( parameters.size() == 2 ) {
            int a = atoi(parameters[1].c_str());
            if ( a <= 1 ) {
                out.printerr("Second argument must be a positive integer.\n");
                return CR_WRONG_USAGE;
            }
            factor = a;
        }
    } else if ( parameters[0] == "clear" ) {
        for ( size_t a = 0; a < fakeThoughts.size(); a++ ) {
            int dorfIndex = fakeThoughts[a].first;
            int thoughtIndex = fakeThoughts[a].second;
            world->units.all[dorfIndex]->status.recent_events[thoughtIndex]->age = 1000000;
        }
        fakeThoughts.clear();
    } else {
        int a = atoi(parameters[0].c_str());
        if ( a < 1 ) {
            return CR_WRONG_USAGE;
        }
        factor = a;
        is_enabled = factor > 1;
    }

    return CR_OK;
}

