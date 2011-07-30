#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>
#include "dfhack/extra/stopwatch.h"
#include "dfhack/modules/Maps.h"
#include <dfhack/modules/Gui.h>

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
DFhackCExport command_result bflags (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "kittens";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("nyan","NYAN CAT INVASION!",kittens));
    commands.push_back(PluginCommand("ktimer","Measure time between game updates and console lag.",ktimer));
    commands.push_back(PluginCommand("blockflags","Look up block flags",bflags));
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
    return CR_OK;
}

DFhackCExport command_result bflags (Core * c, vector <string> & parameters)
{
    c->Suspend();
    Gui * g = c-> getGui();
    Maps* m = c->getMaps();
    if(!m->Start())
    {
        c->con.printerr("No map to probe\n");
        return CR_FAILURE;
    }
    int32_t cx,cy,cz;
    g->getCursorCoords(cx,cy,cz);
    if(cx == -30000)
    {
        // get map size in blocks
        uint32_t sx,sy,sz;
        m->getSize(sx,sy,sz);
        std::map <uint8_t, df_block *> counts;
        // for each block
        for(size_t x = 0; x < sx; x++)
            for(size_t y = 0; y < sx; y++)
                for(size_t z = 0; z < sx; z++)
                {
                    df_block * b = m->getBlock(x,y,z);
                    if(!b) continue;
                    auto iter = counts.find(b->flags.size);
                    if(iter == counts.end())
                    {
                        counts[b->flags.bits[0]] = b;
                    }
                }
        for(auto iter = counts.begin(); iter != counts.end(); iter++)
        {
            c->con.print("%2x : 0x%x\n",iter->first, iter->second);
        }
    }
    else
    {
        df_block * b = m->getBlock(cx/16,cy/16,cz);
        if(b)
        {
            c->con << "Block flags:" << b->flags << std::endl;
        }
        else
        {
            c->con.printerr("No block here\n");
            return CR_FAILURE;
        }
    }
    c->Resume();
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
    c->con.print("Time to suspend = %d ms\n",timeend - timestart);
    // harmless potential data race here...
    timeLast = timeend;
    timering = true;
    return CR_OK;
}

DFhackCExport command_result kittens (Core * c, vector <string> & parameters)
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
