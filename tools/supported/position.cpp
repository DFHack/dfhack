// Just show some position data

#include <iostream>
#include <climits>
#include <vector>
#include <ctime>
using namespace std;

#include <DFHack.h>

std::ostream &operator<<(std::ostream &stream, DFHack::t_gamemodes funzies)
{
    char * gm[]=
    {
        "Fort",
        "Adventurer",
        "Legends",
        "Menus",
        "Arena",
		"Arena - Assumed",
		"Kittens!",
		"Worldgen"
    };
    char * cm[]=
    {
        "Managing",
        "Direct Control",
        "Kittens!",
        "Menus"
    };
    if(funzies.game_mode <= DFHack::GM_MAX && funzies.control_mode <= DFHack::CM_MAX)
        stream << "Game mode: " << gm[funzies.game_mode] << ", Control mode: " << cm[funzies.control_mode];
    else
        stream << "Game mode is too funky: (" << funzies.game_mode << "," << funzies.control_mode << ")";
    return stream;
}

int main (int argc, char** argv)
{
    bool quiet = false;
    for(int i = 1; i < argc; i++)
    {
        string test = argv[i];
        if(test == "-q")
        {
            quiet = true;
        }
    }

    DFHack::Gui * Gui = 0;
    DFHack::World * World = 0;
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
        Gui = DF->getGui();
        World = DF->getWorld();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    DFHack::t_gamemodes gmm;
    if(World->ReadGameMode(gmm))
    {
        cout << gmm << endl;
    }
        cout << "Year: " << World->ReadCurrentYear()
             << " Month: " << World->ReadCurrentMonth()
             << " Day: " << World->ReadCurrentDay()
             << " Tick: " << World->ReadCurrentTick() << endl;
    if (Gui)
    {
        int32_t x,y,z;
        int32_t width,height;

        if(Gui->getViewCoords(x,y,z))
            cout << "view coords: " << x << "/" << y << "/" << z << endl;
        if(Gui->getCursorCoords(x,y,z))
            cout << "cursor coords: " << x << "/" << y << "/" << z << endl;
        if(Gui->getWindowSize(width,height))
            cout << "window size : " << width << " " << height << endl;
    }
    else
    {
        cerr << "cursor and window parameters are unsupported on your version of DF" << endl;
    }

    if(!DF->Detach())
    {
        cerr << "Can't detach from DF" << endl;
    }

    #ifndef LINUX_BUILD
        if(!quiet)
        {
            cout << "Done. Press any key to continue" << endl;
            cin.ignore();
        }
    #endif
    return 0;
}
