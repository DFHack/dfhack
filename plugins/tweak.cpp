// A container for random minor tweaks that don't fit anywhere else,
// in order to avoid creating lots of plugins and polluting the namespace.

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"

#include "DataDefs.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/squad.h"
#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/assumed_identity.h"
#include "df/language_name.h"
#include "df/death_info.h"
#include "df/criminal_case.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::ui;
using df::global::world;

using namespace DFHack::Gui;

static command_result tweak(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("tweak");

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand(
        "tweak", "Various tweaks for minor bugs.", tweak, false,
        "  tweak clear-missing\n"
        "    Remove the missing status from the selected unit.\n"
        "  tweak lair\n"
        "    Mark the map as monster lair, preventing item scatter on reclaim.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

static command_result lair(color_ostream &out, std::vector<std::string> & params);

static command_result tweak(color_ostream &out, vector <string> &parameters)
{
    CoreSuspender suspend;

    if (parameters.empty())
        return CR_WRONG_USAGE;

    string cmd = parameters[0];

    if (cmd == "clear-missing")
    {
        df::unit *unit = getSelectedUnit(out);
        if (!unit)
            return CR_FAILURE;

        auto death = df::death_info::find(unit->counters.death_id);

        if (death)
        {
            death->flags.bits.discovered = true;

            auto crime = df::criminal_case::find(death->crime_id);
            if (crime)
                crime->flags.bits.discovered = true;
        }
    }
    else if(cmd == "lair")
    {
        return lair(out,parameters);
    }
    else return CR_WRONG_USAGE;

    return CR_OK;
}

#include "modules/Maps.h"

command_result lair(color_ostream &out, std::vector<std::string> & params)
{
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    uint32_t x_max,y_max,z_max;
    Maps::getSize(x_max,y_max,z_max);
    for (size_t i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        DFHack::occupancies40d & occupancies = block->occupancy;
        // for each tile in block
        for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++)
        {
            // set to revealed
            occupancies[x][y].bits.monster_lair = true;
        }
    }
    out.print("Map monsterized.\n");
    return CR_OK;
}