#include <iostream>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include <vector>
#include <string>
#include "modules/World.h"
#include <stdlib.h>
using namespace DFHack;


command_result mode (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "mode";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand(
        "mode","View, change and track game mode.",
        mode, true,
        "  Without any parameters, this command prints the current game mode\n"
        "  You can interactively set the game mode with 'mode set'.\n"
        "!!Setting the game modes can be dangerous and break your game!!\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( Core * c )
{
    // add tracking here
    return CR_OK;
}

void printCurrentModes(t_gamemodes gm, Console & con)
{
    con << "Current game type:\t" << gm.g_type << " (";
    switch(gm.g_type)
    {
    case GAMETYPE_DWARF_MAIN:
        con << "Fortress)" << endl;
        break;
    case GAMETYPE_ADVENTURE_MAIN:
        con << "Adventurer)" << endl;
        break;
    case GAMETYPE_VIEW_LEGENDS:
        con << "Legends)" << endl;
        break;
    case GAMETYPE_DWARF_RECLAIM:
        con << "Reclaim)" << endl;
        break;
    case GAMETYPE_DWARF_ARENA:
        con << "Arena)" << endl;
        break;
    case GAMETYPE_ADVENTURE_ARENA:
        con << "Arena - control creature)" << endl;
        break;
    case GAMETYPENUM:
        con << "INVALID)" << endl;
        break;
    case GAMETYPE_NONE:
        con << "NONE)" << endl;
        break;
    }
    con << "Current game mode:\t" << gm.g_mode << " (";
    switch (gm.g_mode)
    {
    case GAMEMODE_DWARF:
        con << "Dwarf)" << endl;
        break;
    case GAMEMODE_ADVENTURE:
        con << "Adventure)" << endl;
        break;
    case GAMEMODENUM:
        con << "INVALID)" << endl;
        break;
    case GAMEMODE_NONE:
        con << "NONE)" << endl;
        break;
    }
}

command_result mode (Core * c, vector <string> & parameters)
{
    string command = "";
    bool set = false;
    t_gamemodes gm;
    if(parameters.size())
    {
        if(parameters[0] == "set")
        {
            set = true;
        }
        else
            return CR_WRONG_USAGE;
    }
    c->Suspend();
    World *world = c->getWorld();
    world->Start();
    world->ReadGameMode(gm);
    c->Resume();
    printCurrentModes(gm, c->con);
    if(set)
    {
        if( gm.g_mode == GAMEMODE_NONE || gm.g_type == GAMETYPE_VIEW_LEGENDS )
        {
            c->con.printerr("It is not safe to set modes in menus.\n");
            return CR_FAILURE;
        }
        c->con << "\nPossible choices:" << endl
               << "0 = Fortress Mode" << endl
               << "1 = Adventurer Mode" << endl
               << "2 = Arena Mode" << endl
               << "3 = Arena, controlling creature" << endl
               << "4 = Reclaim Fortress Mode" << endl
               << "c = cancel/do nothing" << endl;
        uint32_t select=99;

        string selected;
        input_again:
        CommandHistory hist;
        c->con.lineedit("Enter new mode: ",selected, hist);
        if(selected == "c")
            return CR_OK;
        const char * start = selected.c_str();
        char * end = 0;
        select = strtol(start, &end, 10);
        if(!end || end==start || select > 4)
        {
            c->con.printerr("This is not a valid selection.\n");
            goto input_again;
        }

        switch(select)
        {
            case 0:
                gm.g_mode = GAMEMODE_DWARF;
                gm.g_type = GAMETYPE_DWARF_MAIN;
                break;
            case 1:
                gm.g_mode = GAMEMODE_ADVENTURE;
                gm.g_type = GAMETYPE_ADVENTURE_MAIN;
                break;
            case 2:
                gm.g_mode = GAMEMODE_DWARF;
                gm.g_type = GAMETYPE_DWARF_ARENA;
                break;
            case 3:
                gm.g_mode = GAMEMODE_ADVENTURE;
                gm.g_type = GAMETYPE_ADVENTURE_ARENA;
                break;
            case 4:
                gm.g_mode = GAMEMODE_DWARF;
                gm.g_type = GAMETYPE_DWARF_RECLAIM;
                break;
        }
        c->Suspend();
        world->WriteGameMode(gm);
        c->Resume();
        c->con << endl;
    }
    return CR_OK;
}
