#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>
#include "dfhack/extra/stopwatch.h"

using std::vector;
using std::string;
using namespace DFHack;
//FIXME: possible race conditions with calling kittens from the IO thread and shutdown from Core.
bool shutdown_flag = false;
bool final_flag = true;
bool timering = false;
uint64_t timeLast = 0;

DFhackCExport command_result kittens (Core * c, vector <string> & parameters);
DFhackCExport command_result ktimer (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "kittens";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("kittens","Rainbow kittens. What else?",kittens));
    commands.push_back(PluginCommand("ktimer","Time events...",ktimer));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    shutdown_flag = true;
    while(!final_flag)
    {
        c->con->msleep(60);
    }
    return CR_OK;
}



DFhackCExport command_result plugin_onupdate ( Core * c )
{
    if(timering == true)
    {
        uint64_t time2 = GetTimeMs64();
        uint64_t delta = time2-timeLast;
        timeLast = time2;
        dfout << "Time delta = " << delta << " ms" << std::endl;
    }
    return CR_OK;
}

DFhackCExport command_result ktimer (Core * c, vector <string> & parameters)
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
    dfout << "Time to suspend = " << timeend - timestart << " ms" << std::endl;
    timeLast = timeend;
    timering = true;
    return CR_OK;
}

DFhackCExport command_result kittens (Core * c, vector <string> & parameters)
{
    final_flag = false;
    Console * con = c->con;
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
    con->cursor(false);
    con->clear();
    int color = 1;
    while(1)
    {
        if(shutdown_flag)
        {
            final_flag = true;
            c->con->reset_color();
            dfout << std::endl << "MEOW!" << std::endl << std::flush;
            return CR_OK;
        }
        con->color(color);
        int index = 0;
        const char * kit = kittenz1[index];
        con->gotoxy(1,1);
        dfout << "Your DF is now full of kittens!" << std::endl;
        while (kit != 0)
        {
            con->gotoxy(5,5+index);
            dfout << kit;
            index++;
            kit = kittenz1[index];
        }
        dfout.flush();
        con->msleep(60);
        color ++;
        if(color > 15)
            color = 1;
    }
}
