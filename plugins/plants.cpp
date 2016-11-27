#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>
#include <string>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "TileTypes.h"
#include "modules/MapCache.h"

#include "df/plant.h"
#include "df/world.h"

using std::vector;
using std::string;
using namespace DFHack;

DFHACK_PLUGIN("plants");
REQUIRE_GLOBAL(world);

const uint32_t sapling_to_tree_threshold = 120 * 28 * 12 * 3 - 1; // 3 years minus 1 - let the game handle the actual growing-up

/* Immolate/Extirpate no longer work in 0.40
enum do_what
{
    do_immolate,
    do_extirpate
};

static bool getoptions( vector <string> & parameters, bool & shrubs, bool & trees, bool & help)
{
    for(size_t i = 0;i < parameters.size();i++)
    {
        if(parameters[i] == "shrubs")
        {
            shrubs = true;
        }
        else if(parameters[i] == "trees")
        {
            trees = true;
        }
        else if(parameters[i] == "all")
        {
            trees = true;
            shrubs = true;
        }
        else if(parameters[i] == "help" || parameters[i] == "?")
        {
            help = true;
        }
        else
        {
            return false;
        }
    }
    return true;
}

//
// Book of Immolations, chapter 1, verse 35:
// Armok emerged from the hellish depths and beheld the sunny realms for the first time.
// And he cursed the plants and trees for their bloodless wood, turning them into ash and smoldering ruin.
// Armok was pleased and great temples were built by the dwarves, for they shared his hatred for trees and plants.
//

static command_result immolations (color_ostream &out, do_what what, bool shrubs, bool trees)
{
    CoreSuspender suspend;
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    uint32_t x_max, y_max, z_max;
    Maps::getSize(x_max, y_max, z_max);
    MapExtras::MapCache map;
    if(shrubs || trees)
    {
        int destroyed = 0;
        for(size_t i = 0 ; i < world->plants.all.size(); i++)
        {
            df::plant *p = world->plants.all[i];
            if(shrubs && p->flags.bits.is_shrub || trees && !p->flags.bits.is_shrub)
            {
                if (what == do_immolate)
                    p->damage_flags.bits.is_burning = true;
                p->hitpoints = 0;
                destroyed ++;
            }
        }
        out.print("Praise Armok! %i plants destroyed.\n", destroyed);
    }
    else
    {
        int32_t x,y,z;
        if(Gui::getCursorCoords(x,y,z))
        {
            bool didit = false;
            for(size_t i = 0; i < world->plants.all.size(); i++)
            {
                df::plant *tree = world->plants.all[i];
                if(tree->pos.x == x && tree->pos.y == y && tree->pos.z == z)
                {
                    if(what == do_immolate)
                        tree->damage_flags.bits.is_burning = true;
                    tree->hitpoints = 0;
                    didit = true;
                    break;
                }
            }
            if(didit)
                out.print("Praise Armok! Selected plant destroyed.\n");
            else
               out.printerr("No plant found at specified location!\n");
        }
        else
        {
            out.printerr("No mass destruction and no cursor...\n" );
        }
    }
    return CR_OK;
}

command_result df_immolate (color_ostream &out, vector <string> & parameters, do_what what)
{
    bool shrubs = false, trees = false, help = false;
    if (getoptions(parameters, shrubs, trees, help) && !help)
    {
        return immolations(out, what, shrubs, trees);
    }

    string mode;
    if (what == do_immolate)
        mode = "Set plants on fire";
    else
        mode = "Kill plants";

    if (!help)
        out.printerr("Invalid parameter!\n");

    out << "Usage:\n" <<
        mode << " (under cursor, 'shrubs', 'trees' or 'all').\n"
        "Without any options, this command acts on the plant under the cursor.\n"
        "Options:\n"
        "shrubs   - affect all shrubs\n"
        "trees    - affect all trees\n"
        "all      - affect all plants\n";

    return CR_OK;
}
*/
command_result df_grow (color_ostream &out, vector <string> & parameters)
{
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out.print("Usage:\n"
                "This command turns all living saplings on the map into full-grown trees.\n"
                "With active cursor, work on the targetted one only.\n");
            return CR_OK;
        }
    }

    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    MapExtras::MapCache map;
    int32_t x,y,z;
    int grown = 0;
    if(Gui::getCursorCoords(x,y,z))
    {
        for(size_t i = 0; i < world->plants.all.size(); i++)
        {
            df::plant * tree = world->plants.all[i];
            if(tree->pos.x == x && tree->pos.y == y && tree->pos.z == z)
            {
                if(tileShape(map.tiletypeAt(DFCoord(x,y,z))) == tiletype_shape::SAPLING &&
                    tileSpecial(map.tiletypeAt(DFCoord(x,y,z))) != tiletype_special::DEAD)
                {
                    tree->grow_counter = sapling_to_tree_threshold;
                    grown++;
                }
                break;
            }
        }
    }
    else
    {
        for(size_t i = 0 ; i < world->plants.all.size(); i++)
        {
            df::plant *p = world->plants.all[i];
            df::tiletype ttype = map.tiletypeAt(df::coord(p->pos.x,p->pos.y,p->pos.z));
            if(!p->flags.bits.is_shrub && tileShape(ttype) == tiletype_shape::SAPLING && tileSpecial(ttype) != tiletype_special::DEAD)
            {
                p->grow_counter = sapling_to_tree_threshold;
                grown++;
            }
        }
    }
    if (grown)
        out.print("%i plants grown.\n", grown);
    else
        out.printerr("No plant(s) found!\n");

    return CR_OK;
}

command_result df_createplant (color_ostream &out, vector <string> & parameters)
{
    if ((parameters.size() != 1) || (parameters[0] == "help" || parameters[0] == "?"))
    {
        out.print("Usage:\n"
            "Create a new plant at the cursor.\n"
            "Specify the type of plant to create by its raw ID (e.g. TOWER_CAP or MUSHROOM_HELMET_PLUMP).\n"
            "Only shrubs and saplings can be placed, and they must be located on a dirt or grass floor.\n");
        return CR_OK;
    }

    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int32_t x,y,z;
    if(!Gui::getCursorCoords(x,y,z))
    {
        out.printerr("No cursor detected - please place the cursor over the location in which you wish to create a new plant.\n");
        return CR_FAILURE;
    }
    df::map_block *map = Maps::getTileBlock(x, y, z);
    df::map_block_column *col = Maps::getBlockColumn((x / 48) * 3, (y / 48) * 3);
    if (!map || !col)
    {
        out.printerr("Invalid location selected!\n");
        return CR_FAILURE;
    }
    int tx = x & 15, ty = y & 15;
    int mat = tileMaterial(map->tiletype[tx][ty]);
    if ((tileShape(map->tiletype[tx][ty]) != tiletype_shape::FLOOR) || ((mat != tiletype_material::SOIL) && (mat != tiletype_material::GRASS_DARK) && (mat != tiletype_material::GRASS_LIGHT)))
    {
        out.printerr("Plants can only be placed on dirt or grass floors!\n");
        return CR_FAILURE;
    }

    int plant_id = -1;
    df::plant_raw *plant_raw = NULL;
    for (size_t i = 0; i < world->raws.plants.all.size(); i++)
    {
        plant_raw = world->raws.plants.all[i];
        if (plant_raw->id == parameters[0])
        {
            plant_id = i;
            break;
        }
    }
    if (plant_id == -1)
    {
        out.printerr("Invalid plant ID specified!\n");
        return CR_FAILURE;
    }
    if (plant_raw->flags.is_set(plant_raw_flags::GRASS))
    {
        out.printerr("You cannot plant grass using this command.\n");
        return CR_FAILURE;
    }

    df::plant *plant = df::allocate<df::plant>();
    if (plant_raw->flags.is_set(plant_raw_flags::TREE))
        plant->hitpoints = 400000;
    else
    {
        plant->hitpoints = 100000;
        plant->flags.bits.is_shrub = 1;
    }
    // for now, always set "watery" for WET-permitted plants, even if they're spawned away from water
    // the proper method would be to actually look for nearby water features, but it's not clear exactly how that works
    if (plant_raw->flags.is_set(plant_raw_flags::WET))
        plant->flags.bits.watery = 1;
    plant->material = plant_id;
    plant->pos.x = x;
    plant->pos.y = y;
    plant->pos.z = z;
    plant->update_order = rand() % 10;

    world->plants.all.push_back(plant);
    switch (plant->flags.whole & 3)
    {
    case 0: world->plants.tree_dry.push_back(plant); break;
    case 1: world->plants.tree_wet.push_back(plant); break;
    case 2: world->plants.shrub_dry.push_back(plant); break;
    case 3: world->plants.shrub_wet.push_back(plant); break;
    }
    col->plants.push_back(plant);
    if (plant->flags.bits.is_shrub)
        map->tiletype[tx][ty] = tiletype::Shrub;
    else
        map->tiletype[tx][ty] = tiletype::Sapling;

    return CR_OK;
}

command_result df_plant (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() >= 1)
    {
        if (parameters[0] == "grow") {
            parameters.erase(parameters.begin());
            return df_grow(out, parameters);
        } else
/*
        if (parameters[0] == "immolate") {
            parameters.erase(parameters.begin());
            return df_immolate(out, parameters, do_immolate);
        } else
        if (parameters[0] == "extirpate") {
            parameters.erase(parameters.begin());
            return df_immolate(out, parameters, do_extirpate);
        } else
*/
        if (parameters[0] == "create") {
            parameters.erase(parameters.begin());
            return df_createplant(out, parameters);
        }
    }
    return CR_WRONG_USAGE;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("plant", "Plant creation and removal.", df_plant, false,
        "Command to create, grow or remove plants on the map. For more details, check the subcommand help :\n"
        "plant grow help      - Grows saplings into trees.\n"
//        "plant immolate help  - Set plants on fire.\n"
//        "plant extirpate help - Kill plants.\n"
        "plant create help    - Create a new plant.\n"));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
