#include <dfhack/Core.h>
#include <dfhack/Export.h>
#include <dfhack/extra/rlutil.h>

DFhackCExport const char * plugin_name ( void )
{
    return "kittens";
}

DFhackCExport int plugin_run (DFHack::Core * c)
{
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
    rlutil::hidecursor();
    rlutil::cls();
    int color = 1;
    while(1)
    {
        rlutil::setColor(color);
        int index = 0;
        const char * kit = kittenz1[index];
        rlutil::locate(1,1);
        std::cout << "Your DF is now full of kittens!" << std::endl;
        while (kit != 0)
        {
            rlutil::locate(5,5+index);
            std::cout << kit;
            index++;
            kit = kittenz1[index];
        }
        std::fflush(stdout);
        rlutil::msleep(60);
        color ++;
        if(color > 15)
            color = 1;
    }
    return 0;
}
