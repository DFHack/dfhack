// Just show some position data

#include <iostream>
#include <climits>
#include <vector>
#include <ctime>
using namespace std;

#include <DFHack.h>
#include "termutil.h"
std::ostream &operator<<(std::ostream &stream, DFHack::t_gamemodes funzies)
{
    const char * gm[]=
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
    const char * cm[]=
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
    bool temporary_terminal = TemporaryTerminal();
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
    DFHack::Maps * Maps = 0;
    DFHack::World * World = 0;
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    bool have_maps = false;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
        Gui = DF->getGui();
        Maps = DF->getMaps();
        World = DF->getWorld();
        have_maps = Maps->Start();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        if(temporary_terminal)
            cin.ignore();
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
        int32_t vx,vy,vz;
        int32_t cx,cy,cz;
        int32_t wx,wy,wz;
        int32_t width,height;
        if(have_maps)
        {
            Maps->getPosition(wx,wy,wz);
            cout << "Map world offset: " << wx << "/" << wy << "/" << wz << " embark squares." << endl;
        }
        bool have_cursor = Gui->getCursorCoords(cx,cy,cz);
        bool have_view = Gui->getViewCoords(vx,vy,vz);
        if(have_view)
        {
            cout << "view coords: " << vx << "/" << vy << "/" << vz << endl;
            if(have_maps)
                cout << "      world: " << vx+wx*48 << "/" << vy+wy*48 << "/" << vz+wz << endl;
        }
        if(have_cursor)
        {
            cout << "cursor coords: " << cx << "/" << cy << "/" << cz << endl;
            if(have_maps)
                cout << "      world: " << cx+wx*48 << "/" << cy+wy*48 << "/" << cz+wz << endl;
        }
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

    if(!quiet && temporary_terminal)
    {
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    }
    return 0;
}
