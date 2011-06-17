/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "Internal.h"
#include "PlatformInternal.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
using namespace std;

#include "dfhack/Core.h"
#include "dfhack/VersionInfoFactory.h"
#include "dfhack/Error.h"
#include "dfhack/Process.h"
#include "dfhack/Context.h"
#include "dfhack/modules/Gui.h"
#include "dfhack/modules/Vegetation.h"
#include "dfhack/modules/Maps.h"
#include <dfhack/modules/World.h>
#include "rlutil.h"
#include <stdio.h>
using namespace DFHack;

int kittenz (void)
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
        cout << "Your DF is now full of kittens!" << endl;
        while (kit != 0)
        {
            rlutil::locate(5,5+index);
            cout << kit;
            index++;
            kit = kittenz1[index];
        }
        fflush(stdout);
        rlutil::msleep(60);
        color ++;
        if(color > 15)
            color = 1;
        rlutil::cls();
    }
}
struct hideblock
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint8_t hiddens [16][16];
};
int reveal (void)
{
    Core & c = DFHack::Core::getInstance();
    Context * DF = c.getContext();
    c.Suspend();
    DFHack::Maps *Maps =DF->getMaps();
    DFHack::World *World =DF->getWorld();

    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map." << endl;
        c.Resume();
        return 1;
    }

    cout << "Revealing, please wait..." << endl;

    uint32_t x_max, y_max, z_max;
    DFHack::designations40d designations;
    Maps->getSize(x_max,y_max,z_max);
    vector <hideblock> hidesaved;

    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(Maps->isValidBlock(x,y,z))
                {
                    hideblock hb;
                    hb.x = x;
                    hb.y = y;
                    hb.z = z;
                    // read block designations
                    Maps->ReadDesignations(x,y,z, &designations);
                    // change the hidden flag to 0
                    for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
                    {
                        hb.hiddens[i][j] = designations[i][j].bits.hidden;
                        designations[i][j].bits.hidden = 0;
                    }
                    hidesaved.push_back(hb);
                    // write the designations back
                    Maps->WriteDesignations(x,y,z, &designations);
                }
            }
        }
    }
    World->SetPauseState(true);
    c.Resume();
    cout << "Map revealed. The game has been paused for you." << endl;
    cout << "Unpausing can unleash the forces of hell!" << endl;
    cout << "Saving will make this state permanent. Don't do it." << endl << endl;
    cout << "Press any key to unreveal." << endl;
    cin.ignore();
    cout << "Unrevealing... please wait." << endl;
    // FIXME: do some consistency checks here!
    c.Suspend();
    Maps = DF->getMaps();
    Maps->Start();
    for(size_t i = 0; i < hidesaved.size();i++)
    {
        hideblock & hb = hidesaved[i];
        Maps->ReadDesignations(hb.x,hb.y,hb.z, &designations);
        for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
        {
            designations[i][j].bits.hidden = hb.hiddens[i][j];
        }
        Maps->WriteDesignations(hb.x,hb.y,hb.z, &designations);
    }
    c.Resume();
    return 0;
}


int fIOthread(void * _core)
{
    Core * core = (Core *) _core;
    cout << "Hello from the IO thread. Have a nice day!" << endl;
    while (true)
    {
        string command = "";
        cout <<"[DFHack]# ";
        getline(cin, command);
        if (std::cin.eof())
        {
            command = "q";
            std::cout << std::endl; // No newline from the user here!
        }
        if(command=="help" || command == "?")
        {
            cout << "Available commands:" << endl;
            // TODO: generic list of available commands!
            cout << "reveal" << endl;
            cout << "kittens" << endl;
        }
        // TODO: commands will be registered. We'll scan a map of command -> function pointer and call stuff.
        else if(command == "reveal")
        {
            reveal();
        }
        else if(command == "kittens")
        {
            kittenz();
        }
        else
        {
            cout << "Do 'help' or '?' for the list of available commands." << endl;
        }
    }
}

Core::Core()
{
    vif = new DFHack::VersionInfoFactory("Memory.xml");
    p = new DFHack::Process(vif);
    if (!p->isIdentified())
    {
        std::cerr << "Couldn't identify this version of DF." << std::endl;
        errorstate = true;
        return;
    }
    c = new DFHack::Context(p);
    AccessMutex = SDL_CreateMutex();
    if(!AccessMutex)
    {
        std::cerr << "Mutex creation failed." << std::endl;
        errorstate = true;
        return;
    }
    errorstate = false;
    // lock mutex
    SDL_mutexP(AccessMutex);
    // create IO thread
    DFThread * IO = SDL_CreateThread(fIOthread, 0);
    // and let DF do its thing.
};

void Core::Suspend()
{
    SDL_mutexP(AccessMutex);
}

void Core::Resume()
{
    SDL_mutexV(AccessMutex);
}

int Core::Update()
{
    if(errorstate)
        return -1;
    // do persistent stuff here
    SDL_mutexV(AccessMutex);
        // other threads can claim the mutex here and use DFHack.
        // NO CODE SHOULD EVER BE PLACED HERE
    SDL_mutexP(AccessMutex);
    return 0;
};

int Core::Shutdown ( void )
{
    if(errorstate)
        return -1;
    return 0;
    // do something here, eventually.
}
