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

#pragma once

#include "dfhack/Pragma.h"
#include "dfhack/Export.h"
#include "dfhack/FakeSDL.h"
#include <vector>
#include <map>
#include <stdint.h>


namespace DFHack
{
    class Process;
    class Module;
    class Creatures;
    class Engravings;
    class Maps;
    class Gui;
    class World;
    class Materials;
    class Items;
    class Translation;
    class Vegetation;
    class Buildings;
    class Constructions;
    class VersionInfo;
    class VersionInfoFactory;
    class Console;
    class PluginManager;
    class Core;
    // anon type, pretty much
    struct DFLibrary;

    DFLibrary * OpenPlugin (const char * filename);
    void * LookupPlugin (DFLibrary * plugin ,const char * function);
    void ClosePlugin (DFLibrary * plugin);

    // Core is a singleton. Why? Because it is closely tied to SDL calls. It tracks the global state of DF.
    // There should never be more than one instance
    // Better than tracking some weird variables all over the place.
    class DFHACK_EXPORT Core
    {
        friend int  ::SDL_NumJoysticks(void);
        friend void ::SDL_Quit(void);
        friend int  ::SDL_PollEvent(SDL::Event *);
    public:
        /// Get the single Core instance or make one.
        static Core& getInstance()
        {
            // FIXME: add critical section for thread safety here.
            static Core instance;
            return instance;
        }
        /// try to acquire the activity lock
        void Suspend(void);
        /// return activity lock
        void Resume(void);
        /// Is everything OK?
        bool isValid(void) { return !errorstate; }

        /// get the creatures module
        Creatures * getCreatures();
        /// get the engravings module
        Engravings * getEngravings();
        /// get the maps module
        Maps * getMaps();
        /// get the gui module
        Gui * getGui();
        /// get the world module
        World * getWorld();
        /// get the materials module
        Materials * getMaterials();
        /// get the items module
        Items * getItems();
        /// get the translation module
        Translation * getTranslation();
        /// get the vegetation module
        Vegetation * getVegetation();
        /// get the buildings module
        Buildings * getBuildings();
        /// get the constructions module
        Constructions * getConstructions();
        /// sets the current hotkey command
        bool setHotkeyCmd( std::string cmd );
        /// removes the hotkey command and gives it to the caller thread
        std::string getHotkeyCmd( void );

        DFHack::Process * p;
        DFHack::VersionInfo * vinfo;
        DFHack::Console * con;
    private:
        Core();
        bool Init();
        int Update   (void);
        int Shutdown (void);
        int SDL_Event(SDL::Event* event, int orig_return);
        Core(Core const&);              // Don't Implement
        void operator=(Core const&);    // Don't implement
        bool errorstate;
        // mutex for access to DF
        SDL::Mutex * AccessMutex;
        // FIXME: shouldn't be kept around like this
        DFHack::VersionInfoFactory * vif;
        // Module storage
        struct
        {
            Creatures * pCreatures;
            Engravings * pEngravings;
            Maps * pMaps;
            Gui * pGui;
            World * pWorld;
            Materials * pMaterials;
            Items * pItems;
            Translation * pTranslation;
            Vegetation * pVegetation;
            Buildings * pBuildings;
            Constructions * pConstructions;
        } s_mods;
        std::vector <Module *> allModules;
        DFHack::PluginManager * plug_mgr;
        // hotkey-related stuff
        int hotkey_states[16];
        std::string hotkey_cmd;
        bool hotkey_set;
        SDL::Mutex * HotkeyMutex;
        SDL::Cond * HotkeyCond;
        // Very important!
        bool started;
    };
}