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
#include "modules/Vegetation.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "TileTypes.h"
#include "modules/MapCache.h"

using std::vector;
using std::string;
using namespace DFHack;
using df::global::world;

command_result df_grow (color_ostream &out, vector <string> & parameters);
command_result df_immolate (color_ostream &out, vector <string> & parameters);
command_result df_extirpate (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("plants");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("grow", "Grows saplings into trees (with active cursor, only the targetted one).", df_grow));
    commands.push_back(PluginCommand("immolate", "Set plants on fire (under cursor, 'shrubs', 'trees' or 'all').", df_immolate));
    commands.push_back(PluginCommand("extirpate", "Kill plants (same mechanics as immolate).", df_extirpate));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

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

/**
 * Book of Immolations, chapter 1, verse 35:
 * Armok emerged from the hellish depths and beheld the sunny realms for the first time.
 * And he cursed the plants and trees for their bloodless wood, turning them into ash and smoldering ruin.
 * Armok was pleased and great temples were built by the dwarves, for they shared his hatred for trees and plants.
 */
static command_result immolations (color_ostream &out, do_what what, bool shrubs, bool trees, bool help)
{
    static const char * what1 = "destroys";
    static const char * what2 = "burns";
    if(help)
    {
        out.print("Without any options, this command %s a plant under the cursor.\n"
        "Options:\n"
        "shrubs   - affect all shrubs\n"
        "trees    - affect all trees\n"
        "all      - affect all plants\n",
        what == do_immolate? what2 : what1
        );
        return CR_OK;
    }
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
                    p->is_burning = true;
                p->hitpoints = 0;
                destroyed ++;
            }
        }
        out.print("Praise Armok!\n");
    }
    else
    {
        int32_t x,y,z;
        if(Gui::getCursorCoords(x,y,z))
        {
            auto block = Maps::getTileBlock(x,y,z);
            vector<df::plant *> *alltrees = block ? &block->plants : NULL;
            if(alltrees)
            {
                bool didit = false;
                for(size_t i = 0 ; i < alltrees->size(); i++)
                {
                    df::plant * tree = alltrees->at(i);
                    if(tree->pos.x == x && tree->pos.y == y && tree->pos.z == z)
                    {
                        if(what == do_immolate)
                            tree->is_burning = true;
                        tree->hitpoints = 0;
                        didit = true;
                        break;
                    }
                }
                /*
                if(!didit)
                {
                    cout << "----==== There's NOTHING there! ====----" << endl;
                }
                */
            }
        }
        else
        {
            out.printerr("No mass destruction and no cursor...\n" );
        }
    }
    return CR_OK;
}

command_result df_immolate (color_ostream &out, vector <string> & parameters)
{
    bool shrubs = false, trees = false, help = false;
    if(getoptions(parameters,shrubs,trees,help))
    {
        return immolations(out,do_immolate,shrubs,trees, help);
    }
    else
    {
        out.printerr("Invalid parameter!\n");
        return CR_FAILURE;
    }
}

command_result df_extirpate (color_ostream &out, vector <string> & parameters)
{
    bool shrubs = false, trees = false, help = false;
    if(getoptions(parameters,shrubs,trees, help))
    {
        return immolations(out,do_extirpate,shrubs,trees, help);
    }
    else
    {
        out.printerr("Invalid parameter!\n");
        return CR_FAILURE;
    }
}

command_result df_grow (color_ostream &out, vector <string> & parameters)
{
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out.print("This command turns all living saplings into full-grown trees.\n");
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
    if(Gui::getCursorCoords(x,y,z))
    {
        auto block = Maps::getTileBlock(x,y,z);
        vector<df::plant *> *alltrees = block ? &block->plants : NULL;
        if(alltrees)
        {
            for(size_t i = 0 ; i < alltrees->size(); i++)
            {
                df::plant * tree = alltrees->at(i);
                if(tree->pos.x == x && tree->pos.y == y && tree->pos.z == z)
                {
                    if(tileShape(map.tiletypeAt(DFCoord(x,y,z))) == tiletype_shape::SAPLING &&
                        tileSpecial(map.tiletypeAt(DFCoord(x,y,z))) != tiletype_special::DEAD)
                    {
                        tree->grow_counter = Vegetation::sapling_to_tree_threshold;
                    }
                    break;
                }
            }
        }
    }
    else
    {
        int grown = 0;
        for(size_t i = 0 ; i < world->plants.all.size(); i++)
        {
            df::plant *p = world->plants.all[i];
            df::tiletype ttype = map.tiletypeAt(df::coord(p->pos.x,p->pos.y,p->pos.z));
            if(!p->flags.bits.is_shrub && tileShape(ttype) == tiletype_shape::SAPLING && tileSpecial(ttype) != tiletype_special::DEAD)
            {
                p->grow_counter = Vegetation::sapling_to_tree_threshold;
            }
        }
    }

    return CR_OK;
}

