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

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <sstream>
using namespace std;

#include "dfhack/Error.h"
#include "dfhack/Process.h"
#include "dfhack/Core.h"
#include "dfhack/Console.h"
#include "dfhack/VersionInfoFactory.h"
#include "dfhack/PluginManager.h"
#include "ModuleFactory.h"

#include "dfhack/modules/Gui.h"
#include "dfhack/modules/Vegetation.h"
#include "dfhack/modules/Maps.h"
#include "dfhack/modules/World.h"
#include <stdio.h>
#include <iomanip>
using namespace DFHack;

void cheap_tokenise(string const& input, vector<string> &output)
{
    istringstream str(input);
    istream_iterator<string> cur(str), end;
    output.assign(cur, end);
}

struct IODATA
{
    Core * core;
    PluginManager * plug_mgr;
};

int fIOthread(void * iodata)
{
    Core * core = ((IODATA*) iodata)->core;
    PluginManager * plug_mgr = ((IODATA*) iodata)->plug_mgr;
    if(plug_mgr == 0)
    {
        dfout << "Something horrible happened to the plugin manager in Core's constructor..." << std::endl;
        return 0;
    }
    fprintf(dfout_C,"DFHack is ready. Have a nice day! Type in '?' or 'help' for help.\n");
    //dfterm <<  << endl;
    int clueless_counter = 0;
    while (true)
    {
        string command = "";
        dfout <<"[DFHack]# ";
        getline(cin, command);
        if (cin.eof())
        {
            command = "q";
            dfout << std::endl; // No newline from the user here!
        }
        if(command=="help" || command == "?")
        {
            dfout << "Available commands" << endl;
            dfout << "------------------" << endl;
            for(int i = 0; i < plug_mgr->size();i++)
            {
                const Plugin * plug = (plug_mgr->operator[](i));
                dfout << "Plugin " << plug->getName() << " :" << std::endl;
                for (int j = 0; j < plug->size();j++)
                {
                    const PluginCommand & pcmd = (plug->operator[](j));
                    dfout << setw(12) << pcmd.name << "| " << pcmd.description << endl;
                }
                dfout << endl;
            }
        }
        else if( command == "" )
        {
            clueless_counter++;
        }
        else
        {
            vector <string> parts;
            cheap_tokenise(command,parts);
            if(parts.size() == 0)
            {
                clueless_counter++;
            }
            else
            {
                string first = parts[0];
                parts.erase(parts.begin());
                command_result res = plug_mgr->InvokeCommand(first, parts);
                if(res == CR_NOT_IMPLEMENTED)
                {
                    dfout << "Invalid command." << endl;
                    clueless_counter ++;
                }
                else if(res == CR_FAILURE)
                {
                    dfout << "ERROR!" << endl;
                }
            }
        }
        if(clueless_counter == 3)
        {
            dfout << "Do 'help' or '?' for the list of available commands." << endl;
            clueless_counter = 0;
        }
    }
}

Core::Core()
{
    // init the console. This must be always the first step!
    con = new Console();
    plug_mgr = 0;
    // find out what we are...
    vif = new DFHack::VersionInfoFactory("Memory.xml");
    p = new DFHack::Process(vif);
    if (!p->isIdentified())
    {
        dfout << "Couldn't identify this version of DF." << std::endl;
        errorstate = true;
        delete p;
        p = NULL;
        return;
    }
    vinfo = p->getDescriptor();

    // init module storage
    allModules.clear();
    memset(&(s_mods), 0, sizeof(s_mods));

    // create mutex for syncing with interactive tasks
    AccessMutex = SDL_CreateMutex();
    if(!AccessMutex)
    {
        dfout << "Mutex creation failed." << std::endl;
        errorstate = true;
        return;
    }
    // all OK
    errorstate = false;
    // lock mutex
    SDL_mutexP(AccessMutex);
    plug_mgr = new PluginManager(this);
    // look for all plugins, 
    // create IO thread
    IODATA *temp = new IODATA;
    temp->core = this;
    temp->plug_mgr = plug_mgr;
    DFThread * IO = SDL_CreateThread(fIOthread, (void *) temp);
    delete temp;
    // and let DF do its thing.
};

void Core::Suspend()
{
    SDL_mutexP(AccessMutex);
}

void Core::Resume()
{
    for(unsigned int i = 0 ; i < allModules.size(); i++)
    {
        allModules[i]->OnResume();
    }
    SDL_mutexV(AccessMutex);
}

int Core::Update()
{
    if(errorstate)
        return -1;
    // notify all the plugins that a game tick is finished
    plug_mgr->OnUpdate();
    SDL_mutexV(AccessMutex);
        // other threads can claim the mutex here and use DFHack.
        // NO CODE SHOULD EVER BE PLACED HERE
    SDL_mutexP(AccessMutex);
    return 0;
};

int Core::Shutdown ( void )
{
    errorstate = 1;
    if(plug_mgr)
    {
        delete plug_mgr;
        plug_mgr = 0;
    }
    // invalidate all modules
    for(unsigned int i = 0 ; i < allModules.size(); i++)
    {
        delete allModules[i];
    }
    allModules.clear();
    memset(&(s_mods), 0, sizeof(s_mods));
    dfout << std::endl;
    // kill the console object
    delete con;
    con = 0;
    return -1;
}

/*******************************************************************************
                                M O D U L E S
*******************************************************************************/

#define MODULE_GETTER(TYPE) \
TYPE * Core::get##TYPE() \
{ \
    if(errorstate) return NULL;\
    if(!s_mods.p##TYPE)\
    {\
        Module * mod = create##TYPE();\
        s_mods.p##TYPE = (TYPE *) mod;\
        allModules.push_back(mod);\
    }\
    return s_mods.p##TYPE;\
}

MODULE_GETTER(Creatures);
MODULE_GETTER(Engravings);
MODULE_GETTER(Maps);
MODULE_GETTER(Gui);
MODULE_GETTER(World);
MODULE_GETTER(Materials);
MODULE_GETTER(Items);
MODULE_GETTER(Translation);
MODULE_GETTER(Vegetation);
MODULE_GETTER(Buildings);
MODULE_GETTER(Constructions);