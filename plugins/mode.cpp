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
using namespace df::enums;

command_result mode (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("mode");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "mode",
        "View, change and track game mode.",
        mode,
        true));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

void printCurrentModes(t_gamemodes gm, Console & con)
{
    con << "Current game type:\t" << gm.g_type << " (";
    switch(gm.g_type)
    {
    case game_type::DWARF_MAIN:
        con << "Fortress)" << endl;
        break;
    case game_type::ADVENTURE_MAIN:
        con << "Adventurer)" << endl;
        break;
    case game_type::VIEW_LEGENDS:
        con << "Legends)" << endl;
        break;
    case game_type::DWARF_RECLAIM:
        con << "Reclaim)" << endl;
        break;
    case game_type::DWARF_ARENA:
        con << "Arena)" << endl;
        break;
    case game_type::ADVENTURE_ARENA:
        con << "Arena - control creature)" << endl;
        break;
    case game_type::num:
        con << "INVALID)" << endl;
        break;
    case game_type::NONE:
        con << "NONE)" << endl;
        break;
    default:
        con << "!!UNKNOWN!!)" << endl;
        break;
    }
    con << "Current game mode:\t" << gm.g_mode << " (";
    switch (gm.g_mode)
    {
    case game_mode::DWARF:
        con << "Dwarf)" << endl;
        break;
    case game_mode::ADVENTURE:
        con << "Adventure)" << endl;
        break;
    case game_mode::num:
        con << "INVALID)" << endl;
        break;
    case game_mode::NONE:
        con << "NONE)" << endl;
        break;
    default:
        con << "!!UNKNOWN!!)" << endl;
        break;
    }
}

command_result mode (color_ostream &out_, vector <string> & parameters)
{
    if(!out_.is_console())
        return CR_FAILURE;
    Console &out = static_cast<Console&>(out_);

    string command = "";
    bool set = false;
    bool abuse = false;
    int rv = 0;
    t_gamemodes gm;
    for(auto iter = parameters.begin(); iter != parameters.end(); iter++)
    {
        if((*iter) == "set")
        {
            set = true;
        }
        else if((*iter) == "abuse")
        {
            set = abuse = true;
        }
        else
            return CR_WRONG_USAGE;
    }

    World::ReadGameMode(gm);

    printCurrentModes(gm, out);

    if(set)
    {
        if(!abuse)
        {
            if( gm.g_mode == game_mode::NONE || gm.g_type == game_type::VIEW_LEGENDS)
            {
                out.printerr("It is not safe to set modes in menus.\n");
                return CR_FAILURE;
            }
            out << "\nPossible choices:" << endl
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
            while((rv = out.lineedit("Enter new mode: ",selected, hist))
                    == Console::RETRY);
            if(rv <= Console::FAILURE || selected == "c")
                return rv == Console::FAILURE ? CR_FAILURE : CR_OK;
            const char * start = selected.c_str();
            char * end = 0;
            select = strtol(start, &end, 10);
            if(!end || end==start || select > 4)
            {
                out.printerr("This is not a valid selection.\n");
                goto input_again;
            }
            switch(select)
            {
                case 0:
                    gm.g_mode = game_mode::DWARF;
                    gm.g_type = game_type::DWARF_MAIN;
                    break;
                case 1:
                    gm.g_mode = game_mode::ADVENTURE;
                    gm.g_type = game_type::ADVENTURE_MAIN;
                    break;
                case 2:
                    gm.g_mode = game_mode::DWARF;
                    gm.g_type = game_type::DWARF_ARENA;
                    break;
                case 3:
                    gm.g_mode = game_mode::ADVENTURE;
                    gm.g_type = game_type::ADVENTURE_ARENA;
                    break;
                case 4:
                    gm.g_mode = game_mode::DWARF;
                    gm.g_type = game_type::DWARF_RECLAIM;
                    break;
            }
        }
        else
        {
            CommandHistory hist;
            string selected;
            while ((rv = out.lineedit("Enter new game mode number (c for exit): ",selected, hist))
                    == Console::RETRY);
            if(rv <= Console::FAILURE || selected == "c")
                return rv == Console::FAILURE ? CR_FAILURE : CR_OK;
            const char * start = selected.c_str();
            gm.g_mode = (GameMode) strtol(start, 0, 10);
            while((rv = out.lineedit("Enter new game type number (c for exit): ",selected, hist))
                    == Console::RETRY);
            if(rv <= Console::FAILURE || selected == "c")
                return rv == Console::FAILURE ? CR_FAILURE : CR_OK;
            start = selected.c_str();
            gm.g_type = (GameType) strtol(start, 0, 10);
        }

        World::WriteGameMode(gm);

        out << endl;
    }
    return CR_OK;
}
