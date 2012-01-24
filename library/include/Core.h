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

#include "Pragma.h"
#include "Export.h"
#include "Hooks.h"
#include <vector>
#include <stack>
#include <map>
#include <stdint.h>
#include "Console.h"
#include "modules/Graphic.h"

struct WINDOW;

namespace tthread
{
    class mutex;
    class condition_variable;
    class thread;
}

namespace df
{
    struct viewscreen;
}

namespace DFHack
{
    class Process;
    class Module;
    class Gui;
    class World;
    class Materials;
    class Constructions;
    class Notes;
    class VersionInfo;
    class VersionInfoFactory;
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
        friend int  ::SDL_Init(uint32_t flags);
        friend int  ::wgetch(WINDOW * w);
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

        /// get the gui module
        Gui * getGui();
        /// get the world module
        World * getWorld();
        /// get the materials module
        Materials * getMaterials();
        /// get the constructions module
        Constructions * getConstructions();
        /// get the notes module
        Notes * getNotes();
        /// get the graphic module
        Graphic * getGraphic();
        /// sets the current hotkey command
        bool setHotkeyCmd( std::string cmd );
        /// removes the hotkey command and gives it to the caller thread
        std::string getHotkeyCmd( void );

        /// adds a named pointer (for later or between plugins)
        void RegisterData(void *p,std::string key);
        /// returns a named pointer.
        void *GetData(std::string key);

        bool ClearKeyBindings(std::string keyspec);
        bool AddKeyBinding(std::string keyspec, std::string cmdline);
        std::vector<std::string> ListKeyBindings(std::string keyspec);

        bool isWorldLoaded() { return (last_world_data_ptr != NULL); }
        df::viewscreen *getTopViewscreen() { return top_viewscreen; }

        DFHack::Process * p;
        DFHack::VersionInfo * vinfo;
        DFHack::Console con;
    private:
        Core();
        bool Init();
        int Update   (void);
        int Shutdown (void);
        int SDL_Event(SDL::Event* event, int orig_return);
        bool ncurses_wgetch(int in, int & out);
        Core(Core const&);              // Don't Implement
        void operator=(Core const&);    // Don't implement
        // report error to user while failing
        void fatal (std::string output, bool will_deactivate);
        // 1 = fatal failure
        bool errorstate;
        // regulate access to DF
        struct Cond;
        tthread::mutex * AccessMutex;
        tthread::mutex * StackMutex;
        std::stack < Core::Cond * > suspended_tools;
        Core::Cond * core_cond;
        // FIXME: shouldn't be kept around like this
        DFHack::VersionInfoFactory * vif;
        // Module storage
        struct
        {
            Gui * pGui;
            World * pWorld;
            Materials * pMaterials;
            Constructions * pConstructions;
            Notes * pNotes;
            Graphic * pGraphic;
        } s_mods;
        std::vector <Module *> allModules;
        DFHack::PluginManager * plug_mgr;
        
        // hotkey-related stuff
        struct KeyBinding {
            int modifiers;
            std::vector<std::string> command;
            std::string cmdline;
        };

        std::map<int, std::vector<KeyBinding> > key_bindings;
        std::map<int, bool> hotkey_states;
        std::string hotkey_cmd;
        bool hotkey_set;
        tthread::mutex * HotkeyMutex;
        tthread::condition_variable * HotkeyCond;

        bool SelectHotkey(int key, int modifiers);

        void *last_world_data_ptr; // for state change tracking
        df::viewscreen *top_viewscreen;
        // Very important!
        bool started;

		tthread::mutex * misc_data_mutex;
		std::map<std::string,void*> misc_data_map;
    };

    class CoreSuspender {
        Core *core;
    public:
        CoreSuspender(Core *core) : core(core) { core->Suspend(); }
        ~CoreSuspender() { core->Resume(); }
    };
}
