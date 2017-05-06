// (un)designate matching plants for gathering/cutting

#include <set>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"
#include "TileTypes.h"

#include "df/map_block.h"
#include "df/plant.h"
#include "df/plant_raw.h"
#include "df/tile_dig_designation.h"
#include "df/world.h"

#include "modules/Designations.h"
#include "modules/Maps.h"

using std::string;
using std::vector;
using std::set;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("getplants");
REQUIRE_GLOBAL(world);

command_result df_getplants (color_ostream &out, vector <string> & parameters)
{
    string plantMatStr = "";
    set<int> plantIDs;
    set<string> plantNames;
    bool deselect = false, exclude = false, treesonly = false, shrubsonly = false, all = false;

    int count = 0;
    for (size_t i = 0; i < parameters.size(); i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
            return CR_WRONG_USAGE;
        else if(parameters[i] == "-t")
            treesonly = true;
        else if(parameters[i] == "-s")
            shrubsonly = true;
        else if(parameters[i] == "-c")
            deselect = true;
        else if(parameters[i] == "-x")
            exclude = true;
        else if(parameters[i] == "-a")
            all = true;
        else
            plantNames.insert(parameters[i]);
    }
    if (treesonly && shrubsonly)
    {
        out.printerr("Cannot specify both -t and -s at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (all && exclude)
    {
        out.printerr("Cannot specify both -a and -x at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (all && plantNames.size())
    {
        out.printerr("Cannot specify -a along with plant IDs!\n");
        return CR_WRONG_USAGE;
    }

    CoreSuspender suspend;

    for (size_t i = 0; i < world->raws.plants.all.size(); i++)
    {
        df::plant_raw *plant = world->raws.plants.all[i];
        if (all)
            plantIDs.insert(i);
        else if (plantNames.find(plant->id) != plantNames.end())
        {
            plantNames.erase(plant->id);
            plantIDs.insert(i);
        }
    }
    if (plantNames.size() > 0)
    {
        out.printerr("Invalid plant ID(s):");
        for (set<string>::const_iterator it = plantNames.begin(); it != plantNames.end(); it++)
            out.printerr(" %s", it->c_str());
        out.printerr("\n");
        return CR_FAILURE;
    }

    if (plantIDs.size() == 0)
    {
        out.print("Valid plant IDs:\n");
        for (size_t i = 0; i < world->raws.plants.all.size(); i++)
        {
            df::plant_raw *plant = world->raws.plants.all[i];
            if (plant->flags.is_set(plant_raw_flags::GRASS))
                continue;
            out.print("* (%s) %s - %s\n", plant->flags.is_set(plant_raw_flags::TREE) ? "tree" : "shrub", plant->id.c_str(), plant->name.c_str());
        }
        return CR_OK;
    }

    count = 0;
    for (size_t i = 0; i < world->plants.all.size(); i++)
    {
        const df::plant *plant = world->plants.all[i];
        df::map_block *cur = Maps::getTileBlock(plant->pos);
        bool dirty = false;

        int x = plant->pos.x % 16;
        int y = plant->pos.y % 16;
        if (plantIDs.find(plant->material) != plantIDs.end())
        {
            if (exclude)
                continue;
        }
        else
        {
            if (!exclude)
                continue;
        }
        df::tiletype_shape shape = tileShape(cur->tiletype[x][y]);
        df::tiletype_material material = tileMaterial(cur->tiletype[x][y]);
        df::tiletype_special special = tileSpecial(cur->tiletype[x][y]);
        if (plant->flags.bits.is_shrub && (treesonly || !(shape == tiletype_shape::SHRUB && special != tiletype_special::DEAD)))
            continue;
        if (!plant->flags.bits.is_shrub && (shrubsonly || !(material == tiletype_material::TREE)))
            continue;
        if (cur->designation[x][y].bits.hidden)
            continue;
        if (deselect && Designations::unmarkPlant(plant))
        {
            ++count;
        }
        if (!deselect && Designations::markPlant(plant))
        {
            ++count;
        }
    }
    if (count)
        out.print("Updated %d plant designations.\n", count);
    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "getplants", "Cut down trees or gather shrubs by ID",
        df_getplants, false,
        "  Specify the types of trees to cut down and/or shrubs to gather by their\n"
        "  plant IDs, separated by spaces.\n"
        "Options:\n"
        "  -t - Select trees only (exclude shrubs)\n"
        "  -s - Select shrubs only (exclude trees)\n"
        "  -c - Clear designations instead of setting them\n"
        "  -x - Apply selected action to all plants except those specified\n"
        "  -a - Select every type of plant (obeys -t/-s)\n"
        "Specifying both -t and -s will have no effect.\n"
        "If no plant IDs are specified, all valid plant IDs will be listed.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
