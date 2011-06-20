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
using namespace std;

#include "dfhack/Core.h"
#include "dfhack/VersionInfoFactory.h"
#include "ModuleFactory.h"
#include "dfhack/Error.h"
#include "dfhack/Process.h"
#include "dfhack/modules/Gui.h"
#include "dfhack/modules/Vegetation.h"
#include "dfhack/modules/Maps.h"
#include "dfhack/modules/World.h"
#include "dfhack/extra/rlutil.h"
#include <stdio.h>
#ifdef LINUX_BUILD
    #include <dirent.h>
    #include <errno.h>
#else
    #include "wdirent.h"
#endif
using namespace DFHack;

static int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
    {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL) {
    files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}


int fIOthread(void * _core)
{
    Core * core = (Core *) _core;
    vector <string> filez;
    map <string, int (*)(Core *)> plugins;
    getdir(core->p->getPath(), filez);
    const char * (*_PlugName)(void) = 0;
    int (*_PlugRun)(Core *) = 0;
    for(int i = 0; i < filez.size();i++)
    {
        if(strstr(filez[i].c_str(),".plug."))
        {
            DFLibrary * plug = OpenPlugin(filez[i].c_str());
            if(!plug)
            {
                cerr << "Can't load plugin " << filez[i] << endl;
                continue;
            }
            _PlugName = (const char * (*)()) LookupPlugin(plug, "plugin_name");
            if(!_PlugName)
            {
                cerr << "Plugin " << filez[i] << " has no name." << endl;
                ClosePlugin(plug);
                continue;
            }
            _PlugRun = (int (*)(Core * c)) LookupPlugin(plug, "plugin_run");
            if(!_PlugRun)
            {
                cerr << "Plugin " << filez[i] << " has no run function." << endl;
                ClosePlugin(plug);
                continue;
            }
            cout << filez[i] << endl;
            plugins[string(_PlugName())] = _PlugRun;
        }
    }
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
            for (map <string, int (*)(Core *)>::iterator iter = plugins.begin(); iter != plugins.end(); iter++)
            {
                cout << iter->first << endl;
            }
        }
        // TODO: commands will be registered. We'll scan a map of command -> function pointer and call stuff.
        else
        {
            map <string, int (*)(Core *)>::iterator iter = plugins.find(command);
            if(iter != plugins.end())
            {
                iter->second(core);
            }
            else
            {
                cout << "Do 'help' or '?' for the list of available commands." << endl;
            }
        }
    }
}

Core::Core()
{
    // find out what we are...
    vif = new DFHack::VersionInfoFactory("Memory.xml");
    p = new DFHack::Process(vif);
    if (!p->isIdentified())
    {
        std::cerr << "Couldn't identify this version of DF." << std::endl;
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
        std::cerr << "Mutex creation failed." << std::endl;
        errorstate = true;
        return;
    }
    // all OK
    errorstate = false;
    // lock mutex
    SDL_mutexP(AccessMutex);
    // look for all plugins, 
    // create IO thread
    DFThread * IO = SDL_CreateThread(fIOthread, this);
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
    // do persistent stuff here
    SDL_mutexV(AccessMutex);
        // other threads can claim the mutex here and use DFHack.
        // NO CODE SHOULD EVER BE PLACED HERE
    SDL_mutexP(AccessMutex);
    return 0;
};

int Core::Shutdown ( void )
{
    errorstate = 1;
    // invalidate all modules
    for(unsigned int i = 0 ; i < allModules.size(); i++)
    {
        delete allModules[i];
    }
    allModules.clear();
    memset(&(s_mods), 0, sizeof(s_mods));
    // maybe do more
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