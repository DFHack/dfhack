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
using namespace DFHack::Simple;
//FIXME: possible race conditions with calling kittens from the IO thread and shutdown from Core.
bool shutdown_flag = false;
bool final_flag = true;
bool timering = false;
bool trackmenu_flg = false;
bool trackpos_flg = false;
int32_t last_designation[3] = {-30000, -30000, -30000};
int32_t last_mouse[2] = {-1, -1};
uint32_t last_menu = 0;
uint64_t timeLast = 0;

command_result kittens (Core * c, vector <string> & parameters);
command_result ktimer (Core * c, vector <string> & parameters);
command_result trackmenu (Core * c, vector <string> & parameters);
command_result trackpos (Core * c, vector <string> & parameters);
command_result colormods (Core * c, vector <string> & parameters);
command_result zoom (Core * c, vector <string> & parameters);

DFHACK_PLUGIN("kittens");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("nyan","NYAN CAT INVASION!",kittens, true));
    commands.push_back(PluginCommand("ktimer","Measure time between game updates and console lag (toggle).",ktimer));
    commands.push_back(PluginCommand("trackmenu","Track menu ID changes (toggle).",trackmenu));
    commands.push_back(PluginCommand("trackpos","Track mouse and designation coords (toggle).",trackpos));
    commands.push_back(PluginCommand("colormods","Dump colormod vectors.",colormods));
    commands.push_back(PluginCommand("zoom","Zoom to x y z.",zoom));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    shutdown_flag = true;
    while(!final_flag)
    {
        c->con.msleep(60);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( Core * c )
{
    if(timering == true)
    {
        uint64_t time2 = GetTimeMs64();
        // harmless potential data race here...
        uint64_t delta = time2-timeLast;
        // harmless potential data race here...
        timeLast = time2;
        c->con.print("Time delta = %d ms\n", delta);
    }
    if(trackmenu_flg)
    {
        DFHack::Gui * g =c->getGui();
        if (last_menu != *g->df_menu_state)
        {
            last_menu = *g->df_menu_state;
            c->con.print("Menu: %d\n",last_menu);
        }
    }
    if(trackpos_flg)
    {
        DFHack::Gui * g =c->getGui();
        g->Start();
        int32_t desig_x, desig_y, desig_z;
        g->getDesignationCoords(desig_x,desig_y,desig_z);
        if(desig_x != last_designation[0] || desig_y != last_designation[1] || desig_z != last_designation[2])
        {
            last_designation[0] = desig_x;
            last_designation[1] = desig_y;
            last_designation[2] = desig_z;
            c->con.print("Designation: %d %d %d\n",desig_x, desig_y, desig_z);
        }
        int mouse_x, mouse_y;
        g->getMousePos(mouse_x,mouse_y);
        if(mouse_x != last_mouse[0] || mouse_y != last_mouse[1])
        {
            last_mouse[0] = mouse_x;
            last_mouse[1] = mouse_y;
            c->con.print("Mouse: %d %d\n",mouse_x, mouse_y);
        }
    }
    return CR_OK;
}

command_result trackmenu (Core * c, vector <string> & parameters)
{
    if(trackmenu_flg)
    {
        trackmenu_flg = false;
        return CR_OK;
    }
    else
    {
        DFHack::Gui * g =c->getGui();
        if(g->df_menu_state)
        {
            trackmenu_flg = true;
            last_menu =  *g->df_menu_state;
            c->con.print("Menu: %d\n",last_menu);
            return CR_OK;
        }
        else
        {
            c->con.printerr("Can't read menu state\n");
            return CR_FAILURE;
        }
    }
}
command_result trackpos (Core * c, vector <string> & parameters)
{
    trackpos_flg = !trackpos_flg;
    return CR_OK;
}

command_result colormods (Core * c, vector <string> & parameters)
{
    c->Suspend();
    auto & vec = df::global::world->raws.creatures.alphabetic;
    for(int i = 0; i < vec.size();i++)
    {
        df::creature_raw* rawlion = vec[i];
        df::caste_raw * caste = rawlion->caste[0];
        c->con.print("%s\nCaste addr 0x%x\n",rawlion->creature_id.c_str(), &caste->color_modifiers);
        for(int j = 0; j < caste->color_modifiers.size();j++)
        {
            c->con.print("mod %d: 0x%x\n", j, caste->color_modifiers[j]);
        }
    }
    c->Resume();
    return CR_OK;
}

command_result zoom (Core * c, vector <string> & parameters)
{
    if(parameters.size() < 3)
        return CR_FAILURE;
    int x = atoi( parameters[0].c_str());
    int y = atoi( parameters[1].c_str());
    int z = atoi( parameters[2].c_str());
    int xi, yi, zi;
    CoreSuspender cs (c);
    Gui * g = c->getGui();
    if(g->getCursorCoords(xi, yi, zi))
    {
        g->setCursorCoords(x,y,z);
    }
    g->setViewCoords(x,y,z);
}

command_result ktimer (Core * c, vector <string> & parameters)
{
    if(timering)
    {
        timering = false;
        return CR_OK;
    }
    uint64_t timestart = GetTimeMs64();
    c->Suspend();
    c->Resume();
    uint64_t timeend = GetTimeMs64();
    c->con.print("Time to suspend = %d ms\n",timeend - timestart);
    // harmless potential data race here...
    timeLast = timeend;
    timering = true;
    return CR_OK;
}

command_result kittens (Core * c, vector <string> & parameters)
{
    final_flag = false;
    Console & con = c->con;
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
    Console::color_value color = Console::COLOR_BLUE;
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
        if(color > Console::COLOR_MAX)
            color = Console::COLOR_BLUE;
    }
}
