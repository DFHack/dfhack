#include <iostream>
using namespace std;

#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>
#include <dfhack/modules/World.h>
#include <stdlib.h>
using namespace DFHack;


DFhackCExport command_result mode (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "mode";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("mode","View, change and track game mode.", mode, true));
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

DFhackCExport command_result mode (Core * c, vector <string> & parameters)
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
        else if(parameters[0] == "?" || parameters[0] == "help")
        {
            c->con.print("Without any parameters, this command prints the current game mode\n"
                         "You can interactively set the game mode with 'mode set'.\n");
            c->con.printerr("!!Setting the game modes can be dangerous and break your game!!\n");
            return CR_OK;
        }
        else
        {
            c->con.printerr("Unrecognized parameter: %s\n",parameters[0].c_str());
        }
    }
    c->Suspend();
    World *world = c->getWorld();
    world->Start();
    world->ReadGameMode(gm);
    c->Resume();
    printCurrentModes(gm, c->con);
    if(set)
    {
        if(gm.g_mode == GAMEMODE_NONE || gm.g_type == GAMETYPE_VIEW_LEGENDS || gm.g_type == GAMETYPE_DWARF_RECLAIM)
        {
            c->con.printerr("It is not safe to set modes in menus.\n");
            return CR_FAILURE;
        }
        c->con << "\nPossible choices:" << endl
               << "0 = Fortress Mode" << endl
               << "1 = Adventurer Mode" << endl
               << "2 = Arena Mode" << endl
               << "3 = Arena, controlling creature" << endl
               << "c = cancel/do nothing" << endl;
        uint32_t select=99;

        string selected;
        input_again:
        c->con.lineedit("Enter new mode: ",selected);
        if(selected == "c")
            return CR_OK;
        const char * start = selected.c_str();
        char * end = 0;
        select = strtol(start, &end, 10);
        if(!end || end==start || select > 3)
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
        }
        c->Suspend();
        world->WriteGameMode(gm);
        c->Resume();
        cout << endl;
    }
    return CR_OK;
}