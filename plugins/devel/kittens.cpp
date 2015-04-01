#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "MiscUtils.h"
#include <vector>
#include <string>
#include "modules/Maps.h"
#include "modules/Items.h"
#include <modules/Gui.h>
#include <llimits.h>
#include <df/caste_raw.h>
#include <df/creature_raw.h>

using std::vector;
using std::string;
using namespace DFHack;

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

//FIXME: possible race conditions with calling kittens from the IO thread and shutdown from Core.
volatile bool shutdown_flag = false;
volatile bool final_flag = true;
bool timering = false;
bool trackmenu_flg = false;
bool trackpos_flg = false;
bool statetrack = false;
int32_t last_designation[3] = {-30000, -30000, -30000};
int32_t last_mouse[2] = {-1, -1};
uint32_t last_menu = 0;
uint64_t timeLast = 0;

command_result kittens (color_ostream &out, vector <string> & parameters);
command_result ktimer (color_ostream &out, vector <string> & parameters);
command_result trackmenu (color_ostream &out, vector <string> & parameters);
command_result trackpos (color_ostream &out, vector <string> & parameters);
command_result trackstate (color_ostream &out, vector <string> & parameters);
command_result colormods (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("kittens");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("nyan","NYAN CAT INVASION!",kittens));
    commands.push_back(PluginCommand("ktimer","Measure time between game updates and console lag (toggle).",ktimer));
    commands.push_back(PluginCommand("trackmenu","Track menu ID changes (toggle).",trackmenu));
    commands.push_back(PluginCommand("trackpos","Track mouse and designation coords (toggle).",trackpos));
    commands.push_back(PluginCommand("trackstate","Track world and map state (toggle).",trackstate));
    commands.push_back(PluginCommand("colormods","Dump colormod vectors.",colormods));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    shutdown_flag = true;
    while(!final_flag)
    {
        Core::getInstance().getConsole().msleep(60);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    if(!statetrack)
        return CR_OK;
    switch (event) {
        case SC_MAP_LOADED:
            out << "Map loaded" << endl;
            break;
        case SC_MAP_UNLOADED:
            out << "Map unloaded" << endl;
            break;
        case SC_WORLD_LOADED:
            out << "World loaded" << endl;
            break;
        case SC_WORLD_UNLOADED:
            out << "World unloaded" << endl;
            break;
        case SC_VIEWSCREEN_CHANGED:
            out << "Screen changed" << endl;
            break;
        default:
            out << "Something else is happening, nobody knows what..." << endl;
            break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if(timering == true)
    {
        uint64_t time2 = GetTimeMs64();
        // harmless potential data race here...
        uint64_t delta = time2-timeLast;
        // harmless potential data race here...
        timeLast = time2;
        out.print("Time delta = %d ms\n", delta);
    }
    if(trackmenu_flg)
    {
        if (last_menu != df::global::ui->main.mode)
        {
            last_menu = df::global::ui->main.mode;
            out.print("Menu: %d\n",last_menu);
        }
    }
    if(trackpos_flg)
    {
        int32_t desig_x, desig_y, desig_z;
        Gui::getDesignationCoords(desig_x,desig_y,desig_z);
        if(desig_x != last_designation[0] || desig_y != last_designation[1] || desig_z != last_designation[2])
        {
            last_designation[0] = desig_x;
            last_designation[1] = desig_y;
            last_designation[2] = desig_z;
            out.print("Designation: %d %d %d\n",desig_x, desig_y, desig_z);
        }
        int mouse_x, mouse_y;
        Gui::getMousePos(mouse_x,mouse_y);
        if(mouse_x != last_mouse[0] || mouse_y != last_mouse[1])
        {
            last_mouse[0] = mouse_x;
            last_mouse[1] = mouse_y;
            out.print("Mouse: %d %d\n",mouse_x, mouse_y);
        }
    }
    return CR_OK;
}

command_result trackmenu (color_ostream &out, vector <string> & parameters)
{
    if(trackmenu_flg)
    {
        trackmenu_flg = false;
        return CR_OK;
    }
    else
    {
        if(df::global::ui)
        {
            trackmenu_flg = true;
            is_enabled = true;
            last_menu = df::global::ui->main.mode;
            out.print("Menu: %d\n",last_menu);
            return CR_OK;
        }
        else
        {
            out.printerr("Can't read menu state\n");
            return CR_FAILURE;
        }
    }
}
command_result trackpos (color_ostream &out, vector <string> & parameters)
{
    trackpos_flg = !trackpos_flg;
    is_enabled = true;
    return CR_OK;
}

command_result trackstate ( color_ostream& out, vector< string >& parameters )
{
    statetrack = !statetrack;
    return CR_OK;
}

command_result colormods (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;
    auto & vec = df::global::world->raws.creatures.alphabetic;
    for(int i = 0; i < vec.size();i++)
    {
        df::creature_raw* rawlion = vec[i];
        df::caste_raw * caste = rawlion->caste[0];
        out.print("%s\nCaste addr 0x%x\n",rawlion->creature_id.c_str(), &caste->color_modifiers);
        for(int j = 0; j < caste->color_modifiers.size();j++)
        {
            out.print("mod %d: 0x%x\n", j, caste->color_modifiers[j]);
        }
    }
    return CR_OK;
}

command_result ktimer (color_ostream &out, vector <string> & parameters)
{
    if(timering)
    {
        timering = false;
        return CR_OK;
    }
    uint64_t timestart = GetTimeMs64();
    {
        CoreSuspender suspend;
    }
    uint64_t timeend = GetTimeMs64();
    out.print("Time to suspend = %d ms\n",timeend - timestart);
    // harmless potential data race here...
    timeLast = timeend;
    timering = true;
    is_enabled = true;
    return CR_OK;
}

command_result kittens (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() >= 1)
    {
        if (parameters[0] == "stop")
        {
            shutdown_flag = true;
            while(!final_flag)
            {
                Core::getInstance().getConsole().msleep(60);
            }
            shutdown_flag = false;
            return CR_OK;
        }
    }
    final_flag = false;
    if (!out.is_console())
        return CR_FAILURE;
    Console &con = static_cast<Console&>(out);
    // http://evilzone.org/creative-arts/nyan-cat-ascii/
    const char * nyan []=
    {
        "NYAN NYAN NYAN NYAN NYAN NYAN NYAN",
        "+      o     +              o   ",
        "    +             o     +       +",
        "o          +",
        "    o  +           +        +",
        "+        o     o       +        o",
        "-_-_-_-_-_-_-_,------,      o ",
        "_-_-_-_-_-_-_-|   /\\_/\\  ",
        "-_-_-_-_-_-_-~|__( ^ .^)  +     +  ",
        "_-_-_-_-_-_-_-\"\"  \"\"      ",
        "+      o         o   +       o",
        "    +         +",
        "o        o         o      o     +",
        "    o           +",
        "+      +     o        o      +    ",
        "NYAN NYAN NYAN NYAN NYAN NYAN NYAN",
        0
    };
    const char * kittenz1 []=
    {
        "   ____",
        "  (.   \\",
        "    \\  |  ",
        "     \\ |___(\\--/)",
        "   __/    (  . . )",
        "  \"'._.    '-.O.'",
        "       '-.  \\ \"|\\",
        "          '.,,/'.,,mrf",
        0
    };
    con.cursor(false);
    con.clear();
    Console::color_value color = COLOR_BLUE;
    while(1)
    {
        if(shutdown_flag)
        {
            final_flag = true;
            con.reset_color();
            con << std::endl << "NYAN!" << std::endl << std::flush;
            return CR_OK;
        }
        con.color(color);
        int index = 0;
        const char * kit = nyan[index];
        con.gotoxy(1,1);
        //con << "Your DF is now full of kittens!" << std::endl;
        while (kit != 0)
        {
            con.gotoxy(1,1+index);
            con << kit << std::endl;
            index++;
            kit = nyan[index];
        }
        con.flush();
        con.msleep(60);
        ((int&)color) ++;
        if(color > COLOR_MAX)
            color = COLOR_BLUE;
    }
}
