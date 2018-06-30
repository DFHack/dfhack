#include <atomic>
#include <vector>
#include <string>

#include "Console.h"
#include "Core.h"
#include "Export.h"
#include "MiscUtils.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Maps.h"

#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/world.h"

using std::vector;
using std::string;
using namespace DFHack;

DFHACK_PLUGIN("kittens");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);

std::atomic<bool> shutdown_flag{false};
std::atomic<bool> final_flag{true};
std::atomic<bool> timering{false};
std::atomic<bool> trackmenu_flg{false};
std::atomic<uint8_t> trackpos_flg{0};
std::atomic<uint8_t> statetrack{0};
int32_t last_designation[3] = {-30000, -30000, -30000};
int32_t last_mouse[2] = {-1, -1};
df::ui_sidebar_mode last_menu = df::ui_sidebar_mode::Default;
uint64_t timeLast = 0;

command_result kittens (color_ostream &out, vector <string> & parameters);
command_result ktimer (color_ostream &out, vector <string> & parameters);
command_result trackmenu (color_ostream &out, vector <string> & parameters);
command_result trackpos (color_ostream &out, vector <string> & parameters);
command_result trackstate (color_ostream &out, vector <string> & parameters);
command_result colormods (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("nyan","NYAN CAT INVASION!",kittens));
    commands.push_back(PluginCommand("ktimer","Measure time between game updates and console lag.",ktimer));
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
    if(timering)
    {
        uint64_t time2 = GetTimeMs64();
        uint64_t delta = time2-timeLast;
        timeLast = time2;
        out.print("Time delta = %d ms\n", int(delta));
    }
    if(trackmenu_flg)
    {
        if (last_menu != ui->main.mode)
        {
            last_menu = ui->main.mode;
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
    bool is_running = trackmenu_flg.exchange(false);
    if(is_running)
    {
        return CR_OK;
    }
    else
    {
        is_enabled = true;
        last_menu = ui->main.mode;
        out.print("Menu: %d\n",last_menu);
        trackmenu_flg = true;
        return CR_OK;
    }
}
command_result trackpos (color_ostream &out, vector <string> & parameters)
{
    trackpos_flg.fetch_xor(1);
    is_enabled = true;
    return CR_OK;
}

command_result trackstate ( color_ostream& out, vector< string >& parameters )
{
    statetrack.fetch_xor(1);
    return CR_OK;
}

command_result colormods (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;
    auto & vec = world->raws.creatures.alphabetic;
    for(df::creature_raw* rawlion : vec)
    {
        df::caste_raw * caste = rawlion->caste[0];
        out.print("%s\nCaste addr %p\n",rawlion->creature_id.c_str(), &caste->color_modifiers);
        for(size_t j = 0; j < caste->color_modifiers.size();j++)
        {
            out.print("mod %zd: %p\n", j, caste->color_modifiers[j]);
        }
    }
    return CR_OK;
}

command_result ktimer (color_ostream &out, vector <string> & parameters)
{
    bool is_running = timering.exchange(false);
    if(is_running)
    {
        return CR_OK;
    }
    uint64_t timestart = GetTimeMs64();
    {
        CoreSuspender suspend;
        uint64_t timeend = GetTimeMs64();
        timeLast = timeend;
        timering = true;
        out.print("Time to suspend = %d ms\n", int(timeend - timestart));
    }
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
        if(shutdown_flag || !con.isInited())
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
        color = Console::color_value(int(color) + 1);
        if(color > COLOR_MAX)
            color = COLOR_BLUE;
    }
}
