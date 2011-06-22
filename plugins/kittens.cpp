#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/extra/rlutil.h>

DFhackCExport const char * plugin_name ( void )
{
    return "kittens";
}

DFhackCExport int plugin_run (DFHack::Core * c)
{
    DFHack::Console * con = c->con;
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
        rlutil::msleep(60); // FIXME: replace!
        con->clear();
        color ++;
        if(color > 15)
            color = 1;
    }
    return 0;
}
