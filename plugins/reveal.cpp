#include <stdint.h>
#include <iostream>
#include <map>
#include <vector>
#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Maps.h>
#include <dfhack/modules/World.h>
#include <dfhack/extra/MapExtras.h>
#include <dfhack/modules/Gui.h>
using MapExtras::MapCache;
using namespace DFHack;

/*
 * Anything that might reveal Hell is unsafe.
 */
bool isSafe(uint32_t x, uint32_t y, uint32_t z, DFHack::Maps *Maps)
{
    DFHack::t_feature *local_feature = NULL;
    DFHack::t_feature *global_feature = NULL;
    // get features of block
    // error -> obviously not safe to manipulate
    if(!Maps->ReadFeatures(x,y,z,&local_feature,&global_feature))
    {
        return false;
    }
    // Adamantine tubes and temples lead to Hell
    if (local_feature && local_feature->type != DFHack::feature_Other)
        return false;
    // And Hell *is* Hell.
    if (global_feature && global_feature->type == DFHack::feature_Underworld)
        return false;
    // otherwise it's safe.
    return true;
}

struct hideblock
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint8_t hiddens [16][16];
};

// the saved data. we keep map size to check if things still match
uint32_t x_max, y_max, z_max;
std::vector <hideblock> hidesaved;
bool nopause_state = false;

enum revealstate
{
    NOT_REVEALED,
    REVEALED,
    SAFE_REVEALED,
    DEMON_REVEALED
};

revealstate revealed = NOT_REVEALED;

DFhackCExport command_result reveal(DFHack::Core * c, std::vector<std::string> & params);
DFhackCExport command_result unreveal(DFHack::Core * c, std::vector<std::string> & params);
DFhackCExport command_result revtoggle(DFHack::Core * c, std::vector<std::string> & params);
DFhackCExport command_result revflood(DFHack::Core * c, std::vector<std::string> & params);
DFhackCExport command_result nopause(DFHack::Core * c, std::vector<std::string> & params);

DFhackCExport const char * plugin_name ( void )
{
    return "reveal";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("reveal","Reveal the map. 'reveal hell' will also reveal hell. 'reveal demon' won't pause.",reveal));
    commands.push_back(PluginCommand("unreveal","Revert the map to its previous state.",unreveal));
    commands.push_back(PluginCommand("revtoggle","Reveal/unreveal depending on state.",revtoggle));
    commands.push_back(PluginCommand("revflood","Hide all, reveal all tiles reachable from cursor position.",revflood));
    commands.push_back(PluginCommand("nopause","Disable pausing (doesn't affect pause forced by reveal).",nopause));
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( Core * c )
{
    DFHack::World *World =c->getWorld();
    t_gamemodes gm;
    World->ReadGameMode(gm);
    if(gm.g_mode == GAMEMODE_DWARF)
    {
        // if the map is revealed and we're in fortress mode, force the game to pause.
        if(revealed == REVEALED)
        {
            World->SetPauseState(true);
        }
        else if(nopause_state)
        {
            World->SetPauseState(false);
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

command_result nopause (Core * c, std::vector <std::string> & parameters)
{
    if (parameters.size() == 1 && (parameters[0] == "0" || parameters[0] == "1"))
    {
        if (parameters[0] == "0")
            nopause_state = 0;
        else
            nopause_state = 1;
        c->con.print("nopause %sactivated.\n", (nopause_state ? "" : "de"));
    }
    else
    {
        c->con.print("Disable pausing (doesn't affect pause forced by reveal).\nActivate with 'nopause 1', deactivate with 'nopause 0'.\nCurrent state: %d.\n", nopause_state);
    }

    return CR_OK;
}


DFhackCExport command_result reveal(DFHack::Core * c, std::vector<std::string> & params)
{
    bool no_hell = true;
    bool pause = true;
    for(int i = 0; i < params.size();i++)
    {
        if(params[i]=="hell")
            no_hell = false;
        else if(params[i] == "help" || params[i] == "?")
        {
            c->con.print("Reveals the map, by default ignoring hell.\n"
                         "Options:\n"
                         "hell     - also reveal hell, while forcing the game to pause.\n"
                         "demon    - reveal hell, do not pause.\n"
            );
            return CR_OK;
        }
    }
    if(params.size() && params[0] == "hell")
    {
        no_hell = false;
    }
    if(params.size() && params[0] == "demon")
    {
        no_hell = false;
        pause = false;
    }
    Console & con = c->con;
    if(revealed != NOT_REVEALED)
    {
        con.printerr("Map is already revealed or this is a different map.\n");
        return CR_FAILURE;
    }

    c->Suspend();
    DFHack::Maps *Maps =c->getMaps();
    DFHack::World *World =c->getWorld();
    t_gamemodes gm;
    World->ReadGameMode(gm);
    if(gm.g_mode != GAMEMODE_DWARF)
    {
        con.printerr("Only in fortress mode.\n");
        c->Resume();
        return CR_FAILURE;
    }
    if(!Maps->Start())
    {
        con.printerr("Can't init map.\n");
        c->Resume();
        return CR_FAILURE;
    }

    if(no_hell && !Maps->StartFeatures())
    {
        con.printerr("Unable to read local features; can't reveal map safely.\n");
        c->Resume();
        return CR_FAILURE;
    }

    Maps->getSize(x_max,y_max,z_max);
    hidesaved.reserve(x_max * y_max * z_max);
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                df_block *block = Maps->getBlock(x,y,z);
                if(block)
                {
                    // in 'no-hell'/'safe' mode, don't reveal blocks with hell and adamantine
                    if (no_hell && !isSafe(x, y, z, Maps))
                        continue;
                    hideblock hb;
                    hb.x = x;
                    hb.y = y;
                    hb.z = z;
                    DFHack::designations40d & designations = block->designation;
                    // for each tile in block
                    for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
                    {
                        // save hidden state of tile
                        hb.hiddens[i][j] = designations[i][j].bits.hidden;
                        // set to revealed
                        designations[i][j].bits.hidden = 0;
                    }
                    hidesaved.push_back(hb);
                }
            }
        }
    }
    if(no_hell)
    {
        revealed = SAFE_REVEALED;
    }
    else
    {
        if(pause)
        {
            revealed = REVEALED;
            World->SetPauseState(true);
        }
        else
            revealed = DEMON_REVEALED;
    }
    c->Resume();
    con.print("Map revealed.\n");
    if(!no_hell)
        con.print("Unpausing can unleash the forces of hell, so it has been temporarily disabled.\n");
    con.print("Run 'unreveal' to revert to previous state.\n");
    return CR_OK;
}

DFhackCExport command_result unreveal(DFHack::Core * c, std::vector<std::string> & params)
{
    Console & con = c->con;
    for(int i = 0; i < params.size();i++)
    {
        if(params[i] == "help" || params[i] == "?")
        {
            c->con.print("Reverts the previous reveal operation, hiding the map again.\n");
            return CR_OK;
        }
    }
    if(!revealed)
    {
        con.printerr("There's nothing to revert!\n");
        return CR_FAILURE;
    }
    c->Suspend();
    DFHack::Maps *Maps =c->getMaps();
    DFHack::World *World =c->getWorld();
    t_gamemodes gm;
    World->ReadGameMode(gm);
    if(gm.g_mode != GAMEMODE_DWARF)
    {
        con.printerr("Only in fortress mode.\n");
        c->Resume();
        return CR_FAILURE;
    }
    Maps = c->getMaps();
    if(!Maps->Start())
    {
        con.printerr("Can't init map.\n");
        c->Resume();
        return CR_FAILURE;
    }

    // Sanity check: map size
    uint32_t x_max_b, y_max_b, z_max_b;
    Maps->getSize(x_max_b,y_max_b,z_max_b);
    if(x_max != x_max_b || y_max != y_max_b || z_max != z_max_b)
    {
        con.printerr("The map is not of the same size...\n");
        c->Resume();
        return CR_FAILURE;
    }

    // FIXME: add more sanity checks / MAP ID

    for(size_t i = 0; i < hidesaved.size();i++)
    {
        hideblock & hb = hidesaved[i];
        df_block * b = Maps->getBlock(hb.x,hb.y,hb.z);
        for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
        {
            b->designation[i][j].bits.hidden = hb.hiddens[i][j];
        }
    }
    // give back memory.
    hidesaved.clear();
    revealed = NOT_REVEALED;
    con.print("Map hidden!\n");
    c->Resume();
    return CR_OK;
}

DFhackCExport command_result revtoggle (DFHack::Core * c, std::vector<std::string> & params)
{
    for(int i = 0; i < params.size();i++)
    {
        if(params[i] == "help" || params[i] == "?")
        {
            c->con.print("Toggles between reveal and unreveal.\nCurrently it: ");
            break;
        }
    }
    if(revealed)
    {
        return unreveal(c,params);
    }
    else
    {
        return reveal(c,params);
    }
}

DFhackCExport command_result revflood(DFHack::Core * c, std::vector<std::string> & params)
{
    for(int i = 0; i < params.size();i++)
    {
        if(params[i] == "help" || params[i] == "?")
        {
            c->con.print("This command hides the whole map. Then, starting from the cursor,\n"
                         "reveals all accessible tiles. Allows repairing parma-revealed maps.\n"
            );
            return CR_OK;
        }
    }
    c->Suspend();
    uint32_t x_max,y_max,z_max;
    Maps * Maps = c->getMaps();
    Gui * Gui = c->getGui();
    World * World = c->getWorld();
    // init the map
    if(!Maps->Start())
    {
        c->con.printerr("Can't init map. Make sure you have a map loaded in DF.\n");
        c->Resume();
        return CR_FAILURE;
    }
    if(revealed != NOT_REVEALED)
    {
        c->con.printerr("This is only safe to use with non-revealed map.\n");
        c->Resume();
        return CR_FAILURE;
    }
    t_gamemodes gm;
    World->ReadGameMode(gm);
    if(gm.g_type != GAMETYPE_DWARF_MAIN && gm.g_mode != GAMEMODE_DWARF )
    {
        c->con.printerr("Only in proper dwarf mode.\n");
        c->Resume();
        return CR_FAILURE;
    }
    int32_t cx, cy, cz;
    Maps->getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;

    Gui->getCursorCoords(cx,cy,cz);
    if(cx == -30000)
    {
        c->con.printerr("Cursor is not active. Point the cursor at some empty space you want to be unhidden.\n");
        c->Resume();
        return CR_FAILURE;
    }
    DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    MapCache * MCache = new MapCache(Maps);
    int16_t tt = MCache->tiletypeAt(xy);
    if(isWallTerrain(tt))
    {
        c->con.printerr("Point the cursor at some empty space you want to be unhidden.\n");
        delete MCache;
        c->Resume();
        return CR_FAILURE;
    }
    // hide all tiles, flush cache
    Maps->getSize(x_max,y_max,z_max);

    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                df_block * b = Maps->getBlock(x,y,z);
                if(b)
                {
                    // change the hidden flag to 0
                    for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
                    {
                        b->designation[i][j].bits.hidden = 1;
                    }
                }
            }
        }
    }
    MCache->trash();

    typedef std::pair <DFCoord, bool> foo;
    std::stack < foo > flood;
    flood.push( foo(xy,false) );

    while( !flood.empty() )
    {
        foo tile = flood.top();
        DFCoord & current = tile.first;
        bool & from_below = tile.second;
        flood.pop();

        if(!MCache->testCoord(current))
            continue;
        int16_t tt = MCache->tiletypeAt(current);
        t_designation des = MCache->designationAt(current);
        if(!des.bits.hidden)
        {
            continue;
        }
        const TileRow * r = getTileRow(tt);
        /*
        if(!r)
        {
            cerr << "unknown tiletype! " << dec << tt << endl;
            continue;
        }
        */
        bool below = 0;
        bool above = 0;
        bool sides = 0;
        bool unhide = 1;
        // by tile shape, determine behavior and action
        switch (r->shape)
        {
            // walls:
            case WALL:
            case PILLAR:
                if(from_below)
                    unhide = 0;
                break;
            // air/free space
            case EMPTY:
            case RAMP_TOP:
            case STAIR_UPDOWN:
            case STAIR_DOWN:
            case BROOK_TOP:
                above = below = sides = true;
                break;
            // has floor
            case FORTIFICATION:
            case STAIR_UP:
            case RAMP:
            case FLOOR:
            case TREE_DEAD:
            case TREE_OK:
            case SAPLING_DEAD:
            case SAPLING_OK:
            case SHRUB_DEAD:
            case SHRUB_OK:
            case BOULDER:
            case PEBBLES:
            case BROOK_BED:
            case RIVER_BED:
            case ENDLESS_PIT:
            case POOL:
                if(from_below)
                    unhide = 0;
                above = sides = true;
                break;
        }
        if(unhide)
        {
            des.bits.hidden = false;
            MCache->setDesignationAt(current,des);
        }
        if(sides)
        {
            flood.push(foo(DFCoord(current.x + 1, current.y ,current.z),0));
            flood.push(foo(DFCoord(current.x + 1, current.y + 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x, current.y + 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x - 1, current.y + 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x - 1, current.y ,current.z),0));
            flood.push(foo(DFCoord(current.x - 1, current.y - 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x, current.y - 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x + 1, current.y - 1 ,current.z),0));
        }
        if(above)
        {
            flood.push(foo(DFCoord(current.x, current.y ,current.z + 1),1));
        }
        if(below)
        {
            flood.push(foo(DFCoord(current.x, current.y ,current.z - 1),0));
        }
    }
    MCache->WriteAll();
    delete MCache;
    c->Resume();
    return CR_OK;
}