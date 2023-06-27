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
#include <memory>
#include <stdint.h>
#include "Console.h"
#include "modules/Graphic.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "RemoteClient.h"

#define DFH_MOD_SHIFT 1
#define DFH_MOD_CTRL 2
#define DFH_MOD_ALT 4

struct WINDOW;

namespace df
{
    struct viewscreen;
}

namespace DFHack
{
    class Process;
    class Module;
    class Materials;
    struct VersionInfo;
    class VersionInfoFactory;
    class PluginManager;
    class Core;
    class ServerMain;
    class CoreSuspender;

    namespace Lua { namespace Core {
        DFHACK_EXPORT void Reset(color_ostream &out, const char *where);
    } }

    namespace Screen
    {
        struct Hide;
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
        friend void ::dfhooks_init();
        friend void ::dfhooks_shutdown();
        friend void ::dfhooks_update();
        friend void ::dfhooks_prerender();
        friend bool ::dfhooks_sdl_event(SDL_Event* event);
        friend bool ::dfhooks_ncurses_key(int key);
    public:
        /// Get the single Core instance or make one.
        static Core& getInstance()
        {
            static Core instance;
            return instance;
        }
        /// check if the activity lock is owned by this thread
        bool isSuspended(void);
        /// Is everything OK?
        bool isValid(void) { return !errorstate; }

        /// get the materials module
        Materials * getMaterials();
        /// get the graphic module
        Graphic * getGraphic();
        /// sets the current hotkey command
        bool setHotkeyCmd( std::string cmd );
        /// removes the hotkey command and gives it to the caller thread
        std::string getHotkeyCmd( bool &keep_going );

        /// adds a named pointer (for later or between plugins)
        void RegisterData(void *p,std::string key);
        /// returns a named pointer.
        void *GetData(std::string key);

        command_result runCommand(color_ostream &out, const std::string &command, std::vector <std::string> &parameters);
        command_result runCommand(color_ostream &out, const std::string &command);
        bool loadScriptFile(color_ostream &out, std::string fname, bool silent = false);

        bool addScriptPath(std::string path, bool search_before = false);
        bool setModScriptPaths(const std::vector<std::string> &mod_script_paths);
        bool removeScriptPath(std::string path);
        std::string findScript(std::string name);
        void getScriptPaths(std::vector<std::string> *dest);

        bool ClearKeyBindings(std::string keyspec);
        bool AddKeyBinding(std::string keyspec, std::string cmdline);
        std::vector<std::string> ListKeyBindings(std::string keyspec);
        int8_t getModstate() { return modstate; }

        bool AddAlias(const std::string &name, const std::vector<std::string> &command, bool replace = false);
        bool RemoveAlias(const std::string &name);
        bool IsAlias(const std::string &name);
        bool RunAlias(color_ostream &out, const std::string &name,
            const std::vector<std::string> &parameters, command_result &result);
        std::map<std::string, std::vector<std::string>> ListAliases();
        std::string GetAliasCommand(const std::string &name, bool ignore_params = false);

        std::string getHackPath();

        bool isWorldLoaded() { return (last_world_data_ptr != NULL); }
        bool isMapLoaded() { return (last_local_map_ptr != NULL && last_world_data_ptr != NULL); }

        static df::viewscreen *getTopViewscreen() { return getInstance().top_viewscreen; }

        DFHack::Console &getConsole() { return con; }

        std::unique_ptr<DFHack::Process> p;
        std::shared_ptr<DFHack::VersionInfo> vinfo;

        static void print(const char *format, ...) Wformat(printf,1,2);
        static void printerr(const char *format, ...) Wformat(printf,1,2);

        PluginManager *getPluginManager() { return plug_mgr; }

        static void cheap_tokenise(std::string const& input, std::vector<std::string> &output);

    private:
        DFHack::Console con;

        Core();
        ~Core();

        struct Private;
        std::unique_ptr<Private> d;

        bool InitMainThread();
        bool InitSimulationThread();
        int Update (void);
        int Shutdown (void);
        bool DFH_SDL_Event(SDL_Event* event);
        bool ncurses_wgetch(int in, int & out);
        bool DFH_ncurses_key(int key);

        void doUpdate(color_ostream &out);
        void onUpdate(color_ostream &out);
        void onStateChange(color_ostream &out, state_change_event event);
        void handleLoadAndUnloadScripts(color_ostream &out, state_change_event event);
        void doSaveData(color_ostream &out);
        void doLoadData(color_ostream &out);

        Core(Core const&);              // Don't Implement
        void operator=(Core const&);    // Don't implement

        // report error to user while failing
        void fatal (std::string output);

        // 1 = fatal failure
        bool errorstate;
        // regulate access to DF
        struct Cond;

        // FIXME: shouldn't be kept around like this
        std::unique_ptr<DFHack::VersionInfoFactory> vif;
        // Module storage
        struct
        {
            Materials * pMaterials;
            Graphic * pGraphic;
        } s_mods;
        std::vector<std::unique_ptr<Module>> allModules;
        DFHack::PluginManager * plug_mgr;

        std::vector<std::string> script_paths[3];
        std::mutex script_path_mutex;

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
        enum hotkey_set_t {
            NO,
            SET,
            SHUTDOWN,
        };
        hotkey_set_t hotkey_set;
        std::mutex HotkeyMutex;
        std::condition_variable HotkeyCond;

        std::map<std::string, std::vector<std::string>> aliases;
        std::recursive_mutex alias_mutex;

        bool SelectHotkey(int key, int modifiers);

        // for state change tracking
        void *last_world_data_ptr;
        // for state change tracking
        void *last_local_map_ptr;
        friend struct Screen::Hide;
        df::viewscreen *top_viewscreen;
        bool last_pause_state;
        // Very important!
        std::atomic<bool> started;
        // Additional state change scripts
        std::vector<StateChangeScript> state_change_scripts;

        std::mutex misc_data_mutex;
        std::map<std::string,void*> misc_data_map;

        /*!
         * \defgroup core_suspend CoreSuspender state handling serialization to
         * DF memory.
         * \sa DFHack::CoreSuspender
         * \{
         */
        std::recursive_mutex CoreSuspendMutex;
        std::condition_variable_any CoreWakeup;
        std::atomic<std::thread::id> ownerThread;
        std::atomic<size_t> toolCount;
        //! \}

        friend class CoreService;
        friend class ServerConnection;
        friend class CoreSuspender;
        friend class CoreSuspenderBase;
        friend struct CoreSuspendClaimMain;
        friend struct CoreSuspendReleaseMain;
    };

    class CoreSuspenderBase  : protected std::unique_lock<std::recursive_mutex> {
    protected:
        using parent_t = std::unique_lock<std::recursive_mutex>;
        std::thread::id tid;

        CoreSuspenderBase(std::defer_lock_t d) : CoreSuspenderBase{&Core::getInstance(), d} {}

        CoreSuspenderBase(Core* core, std::defer_lock_t) :
            /* Lock the core */
            parent_t{core->CoreSuspendMutex,std::defer_lock},
            /* Mark this thread to be the core owner */
            tid{}
        {}
    public:
        void lock()
        {
            auto& core = Core::getInstance();
            parent_t::lock();
            tid = core.ownerThread.exchange(std::this_thread::get_id(),
                    std::memory_order_acquire);
        }

        void unlock()
        {
            auto& core = Core::getInstance();
            /* Restore core owner to previous value */
            core.ownerThread.store(tid, std::memory_order_release);
            if (tid == std::thread::id{})
                Lua::Core::Reset(core.getConsole(), "suspend");
            parent_t::unlock();
        }

        bool owns_lock() const noexcept
        {
            return parent_t::owns_lock();
        }

        ~CoreSuspenderBase() {
            if (owns_lock())
                unlock();
        }
        friend class MainThread;
    };

    /*!
     * CoreSuspender allows serialization to DF data with std::unique_lock like
     * interface. It includes handling for recursive CoreSuspender calls and
     * notification to main thread after all queue tools have been handled.
     *
     * State transitions are:
     * - Startup setups Core::SuspendMutex to unlocked states
     * - Core::Init locks Core::SuspendMutex until the thread exits or that thread
     *   calls Core::Shutdown or Core::~Core.
     * - Other thread request core suspend by atomic incrementation of Core::toolCount
     *   and then locking Core::CoreSuspendMutex. After locking CoreSuspendMutex
     *   success callers exchange their std::thread::id to Core::ownerThread.
     * - Core::Update() makes sure that queued tools are run when it calls
     *   Core::CoreWakup::wait. The wait keeps Core::CoreSuspendMutex unlocked
     *   and waits until Core::toolCount is reduced back to zero.
     * - CoreSuspender::~CoreSuspender() first stores the previous Core::ownerThread
     *   back. In case of recursive call Core::ownerThread equals tid. If tis is
     *   zero then we are releasing the recursive_mutex which means suspend
     *   context is over. It is time to reset lua.
     *   The last step is to decrement Core::toolCount and wakeup main thread if
     *   no more tools are queued trying to acquire the
     *   Core::CoreSuspenderMutex.
     */
    class CoreSuspender : public CoreSuspenderBase {
        using parent_t = CoreSuspenderBase;
    public:
        CoreSuspender() : CoreSuspender{&Core::getInstance()} { }
        CoreSuspender(std::defer_lock_t d) : CoreSuspender{&Core::getInstance(),d} { }
        CoreSuspender(bool) : CoreSuspender{&Core::getInstance()} { }
        CoreSuspender(Core* core, bool) : CoreSuspender{core} { }
        CoreSuspender(Core* core) :
            CoreSuspenderBase{core, std::defer_lock}
        {
            lock();
        }
        CoreSuspender(Core* core, std::defer_lock_t) :
            CoreSuspenderBase{core, std::defer_lock}
        {}

        void lock()
        {
            auto& core = Core::getInstance();
            core.toolCount.fetch_add(1, std::memory_order_relaxed);
            parent_t::lock();
        }

        void unlock()
        {
            auto& core = Core::getInstance();
            parent_t::unlock();
            /* Notify core to continue when all queued tools have completed
             * 0 = None wants to own the core
             * 1+ = There are tools waiting core access
             * fetch_add returns old value before subtraction
             */
            if (core.toolCount.fetch_add(-1, std::memory_order_relaxed) == 1)
                core.CoreWakeup.notify_one();
        }

        ~CoreSuspender() {
            if (owns_lock())
                unlock();
        }
    };

    /*!
     * Temporary release main thread ownership to allow alternative thread
     * implement DF logic thread loop
     */
    struct DFHACK_EXPORT CoreSuspendReleaseMain {
        CoreSuspendReleaseMain();
        ~CoreSuspendReleaseMain();
    };

    /*!
     * Temporary claim main thread ownership. This allows caller to call
     * Core::Update from a different thread than original DF logic thread if
     * logic thread has released main thread ownership with
     * CoreSuspendReleaseMain
     */
    struct DFHACK_EXPORT CoreSuspendClaimMain {
        CoreSuspendClaimMain();
        ~CoreSuspendClaimMain();
    };

    using CoreSuspendClaimer = CoreSuspender;
}
