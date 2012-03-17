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
#include "SDL_events.h"

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
    class World;
    class Materials;
    class Notes;
    struct VersionInfo;
    class VersionInfoFactory;
    class PluginManager;
    class Core;
    class ServerMain;
    namespace Windows
    {
        class df_window;
    }
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
        friend int  ::egg_init(void);
        friend int  ::egg_shutdown(void);
        friend int  ::egg_tick(void);
        friend int  ::egg_prerender(void);
        friend int  ::egg_sdl_event(SDL::Event* event);
        friend int  ::egg_curses_event(int orig_return);
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

        /// get the world module
        World * getWorld();
        /// get the materials module
        Materials * getMaterials();
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

        static df::viewscreen *getTopViewscreen() { return getInstance().top_viewscreen; }

        DFHack::Console &getConsole() { return con; }

        DFHack::Process * p;
        DFHack::VersionInfo * vinfo;
        DFHack::Windows::df_window * screen_window;

        static void printerr(const char *format, ...);

    private:
        DFHack::Console con;

        Core();

        class Private;
        Private *d;

        bool Init();
        int Update (void);
        int TileUpdate (void);
        int Shutdown (void);
        int SDL_Event(SDL::Event* event);
        bool ncurses_wgetch(int in, int & out);

        Core(Core const&);              // Don't Implement
        void operator=(Core const&);    // Don't implement

        // report error to user while failing
        void fatal (std::string output, bool will_deactivate);

        // 1 = fatal failure
        bool errorstate;
        // regulate access to DF
        struct Cond;

        // FIXME: shouldn't be kept around like this
        DFHack::VersionInfoFactory * vif;
        // Module storage
        struct
        {
            World * pWorld;
            Materials * pMaterials;
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

        int UnicodeAwareSym(const SDL::KeyboardEvent& ke);
        bool SelectHotkey(int key, int modifiers);

        void *last_world_data_ptr; // for state change tracking
        df::viewscreen *top_viewscreen;
        // Very important!
        bool started;

        tthread::mutex * misc_data_mutex;
        std::map<std::string,void*> misc_data_map;

        friend class CoreService;
        friend class ServerConnection;
        ServerMain *server;
    };

    class CoreSuspender {
        Core *core;
    public:
        CoreSuspender() : core(&Core::getInstance()) { core->Suspend(); }
        CoreSuspender(Core *core) : core(core) { core->Suspend(); }
        ~CoreSuspender() { core->Resume(); }
    };
}
