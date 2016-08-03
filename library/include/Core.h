/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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

#include "RemoteClient.h"

#define DFH_MOD_SHIFT 1
#define DFH_MOD_CTRL 2
#define DFH_MOD_ALT 4

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

    enum state_change_event
    {
        SC_UNKNOWN = -1,
        SC_WORLD_LOADED = 0,
        SC_WORLD_UNLOADED = 1,
        SC_MAP_LOADED = 2,
        SC_MAP_UNLOADED = 3,
        SC_VIEWSCREEN_CHANGED = 4,
        SC_CORE_INITIALIZED = 5,
        SC_BEGIN_UNLOAD = 6,
        SC_PAUSED = 7,
        SC_UNPAUSED = 8
    };

    class DFHACK_EXPORT StateChangeScript
    {
    public:
        state_change_event event;
        std::string path;
        bool save_specific;
        StateChangeScript(state_change_event event, std::string path, bool save_specific = false)
            :event(event), path(path), save_specific(save_specific)
        { }
        bool operator==(const StateChangeScript& other)
        {
            return event == other.event && path == other.path && save_specific == other.save_specific;
        }
    };

    // Core is a singleton. Why? Because it is closely tied to SDL calls. It tracks the global state of DF.
    // There should never be more than one instance
    // Better than tracking some weird variables all over the place.
    class DFHACK_EXPORT Core
    {
#ifdef _DARWIN
        friend int  ::DFH_SDL_NumJoysticks(void);
        friend void ::DFH_SDL_Quit(void);
        friend int  ::DFH_SDL_PollEvent(SDL::Event *);
        friend int  ::DFH_SDL_Init(uint32_t flags);
#else
        friend int  ::SDL_NumJoysticks(void);
        friend void ::SDL_Quit(void);
        friend int  ::SDL_PollEvent(SDL::Event *);
        friend int  ::SDL_Init(uint32_t flags);
#endif
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
        /// check if the activity lock is owned by this thread
        bool isSuspended(void);
        /// try to acquire the activity lock
        void Suspend(void);
        /// return activity lock
        void Resume(void);
        /// Is everything OK?
        bool isValid(void) { return !errorstate; }

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

        command_result runCommand(color_ostream &out, const std::string &command, std::vector <std::string> &parameters);
        command_result runCommand(color_ostream &out, const std::string &command);
        bool loadScriptFile(color_ostream &out, std::string fname, bool silent = false);

        bool addScriptPath(std::string path, bool search_before = false);
        bool removeScriptPath(std::string path);
        std::string findScript(std::string name);
        void getScriptPaths(std::vector<std::string> *dest);

        bool ClearKeyBindings(std::string keyspec);
        bool AddKeyBinding(std::string keyspec, std::string cmdline);
        std::vector<std::string> ListKeyBindings(std::string keyspec);
        int8_t getModstate() { return modstate; }

        std::string getHackPath();

        bool isWorldLoaded() { return (last_world_data_ptr != NULL); }
        bool isMapLoaded() { return (last_local_map_ptr != NULL && last_world_data_ptr != NULL); }

        static df::viewscreen *getTopViewscreen() { return getInstance().top_viewscreen; }

        DFHack::Console &getConsole() { return con; }

        DFHack::Process * p;
        DFHack::VersionInfo * vinfo;
        DFHack::Windows::df_window * screen_window;

        static void print(const char *format, ...);
        static void printerr(const char *format, ...);

        PluginManager *getPluginManager() { return plug_mgr; }

        static void cheap_tokenise(std::string const& input, std::vector<std::string> &output);

    private:
        DFHack::Console con;

        Core();

        struct Private;
        Private *d;

        friend class CoreSuspendClaimer;
        int ClaimSuspend(bool force_base);
        void DisclaimSuspend(int level);

        bool Init();
        int Update (void);
        int TileUpdate (void);
        int Shutdown (void);
        int DFH_SDL_Event(SDL::Event* event);
        bool ncurses_wgetch(int in, int & out);

        void doUpdate(color_ostream &out, bool first_update);
        void onUpdate(color_ostream &out);
        void onStateChange(color_ostream &out, state_change_event event);
        void handleLoadAndUnloadScripts(color_ostream &out, state_change_event event);

        Core(Core const&);              // Don't Implement
        void operator=(Core const&);    // Don't implement

        // report error to user while failing
        void fatal (std::string output);

        // 1 = fatal failure
        bool errorstate;
        // regulate access to DF
        struct Cond;

        // FIXME: shouldn't be kept around like this
        DFHack::VersionInfoFactory * vif;
        // Module storage
        struct
        {
            Materials * pMaterials;
            Notes * pNotes;
            Graphic * pGraphic;
        } s_mods;
        std::vector <Module *> allModules;
        DFHack::PluginManager * plug_mgr;

        std::vector<std::string> script_paths[2];
        tthread::mutex *script_path_mutex;

        // hotkey-related stuff
        struct KeyBinding {
            int modifiers;
            std::vector<std::string> command;
            std::string cmdline;
            std::string focus;
        };
        int8_t modstate;

        std::map<int, std::vector<KeyBinding> > key_bindings;
        std::map<int, bool> hotkey_states;
        std::string hotkey_cmd;
        bool hotkey_set;
        tthread::mutex * HotkeyMutex;
        tthread::condition_variable * HotkeyCond;

        bool SelectHotkey(int key, int modifiers);

        // for state change tracking
        void *last_world_data_ptr;
        // for state change tracking
        void *last_local_map_ptr;
        df::viewscreen *top_viewscreen;
        bool last_pause_state;
        // Very important!
        bool started;
        // Additional state change scripts
        std::vector<StateChangeScript> state_change_scripts;

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

    /** Claims the current thread already has the suspend lock.
     *  Strictly for use in callbacks from DF.
     */
    class CoreSuspendClaimer {
        Core *core;
        int level;
    public:
        CoreSuspendClaimer(bool base = false) : core(&Core::getInstance()) {
            level = core->ClaimSuspend(base);
        }
        CoreSuspendClaimer(Core *core, bool base = false) : core(core) {
            level = core->ClaimSuspend(base);
        }
        ~CoreSuspendClaimer() { core->DisclaimSuspend(level); }
    };
}
