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

DFhackCExport command_result df_grow (Core * c, vector <string> & parameters);
DFhackCExport command_result df_immolate (Core * c, vector <string> & parameters);
DFhackCExport command_result df_extirpate (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "plants";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("grow", "Grows saplings into trees (with active cursor, only the targetted one).", df_grow));
    commands.push_back(PluginCommand("immolate", "Set plants on fire (under cursor, 'shrubs', 'trees' or 'all').", df_immolate));
    commands.push_back(PluginCommand("extirpate", "Kill plants (same mechanics as immolate).", df_extirpate));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
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
    for(int i = 0;i < parameters.size();i++)
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
static command_result immolations (Core * c, do_what what, bool shrubs, bool trees, bool help)
{
    static const char * what1 = "destroys";
    static const char * what2 = "burns";
    if(help)
    {
        c->con.print("Without any options, this command %s a plant under the cursor.\n"
        "Options:\n"
        "shrubs   - affect all shrubs\n"
        "trees    - affect all trees\n"
        "all      - affect all plants\n",
        what == do_immolate? what2 : what1
        );
        return CR_OK;
    }
    CoreSuspender suspend(c);
    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    DFHack::Gui * Gui = c->getGui();
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
        c->con.print("Praise Armok!\n");
    }
    else
    {
        int32_t x,y,z;
        if(Gui->getCursorCoords(x,y,z))
        {
            vector<df::plant *> * alltrees;
            if(Maps::ReadVegetation(x/16,y/16,z,alltrees))
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
            c->con.printerr("No mass destruction and no cursor...\n" );
        }
    }
    return CR_OK;
}

DFhackCExport command_result df_immolate (Core * c, vector <string> & parameters)
{
    bool shrubs = false, trees = false, help = false;
    if(getoptions(parameters,shrubs,trees,help))
    {
        return immolations(c,do_immolate,shrubs,trees, help);
    }
    else
    {
        c->con.printerr("Invalid parameter!\n");
        return CR_FAILURE;
    }
}

DFhackCExport command_result df_extirpate (Core * c, vector <string> & parameters)
{
    bool shrubs = false, trees = false, help = false;
    if(getoptions(parameters,shrubs,trees, help))
    {
        return immolations(c,do_extirpate,shrubs,trees, help);
    }
    else
    {
        c->con.printerr("Invalid parameter!\n");
        return CR_FAILURE;
    }
}

DFhackCExport command_result df_grow (Core * c, vector <string> & parameters)
{
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            c->con.print("This command turns all living saplings into full-grown trees.\n");
            return CR_OK;
        }
    }
    CoreSuspender suspend(c);
    Console & con = c->con;
    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    MapExtras::MapCache map;
    DFHack::Gui *Gui = c->getGui();
    int32_t x,y,z;
    if(Gui->getCursorCoords(x,y,z))
    {
        vector<df::plant *> * alltrees;
        if(Maps::ReadVegetation(x/16,y/16,z,alltrees))
        {
            for(size_t i = 0 ; i < alltrees->size(); i++)
            {
                df::plant * tree = alltrees->at(i);
                if(tree->pos.x == x && tree->pos.y == y && tree->pos.z == z)
                {
                    if(DFHack::tileShape(map.tiletypeAt(DFHack::DFCoord(x,y,z))) == DFHack::SAPLING_OK)
                    {
                        tree->grow_counter = DFHack::sapling_to_tree_threshold;
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
            uint16_t ttype = map.tiletypeAt(df::coord(p->pos.x,p->pos.y,p->pos.z));
            if(!p->flags.bits.is_shrub && DFHack::tileShape(ttype) == DFHack::SAPLING_OK)
            {
                p->grow_counter = DFHack::sapling_to_tree_threshold;
            }
        }
    }

    return CR_OK;
}

