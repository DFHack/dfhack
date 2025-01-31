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

#include "Console.h"
#include "CoreDefs.h"
#include "Export.h"
#include "Hooks.h"

#include "modules/Graphic.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <stack>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdint.h>

#define DFH_MOD_SHIFT 1
#define DFH_MOD_CTRL 2
#define DFH_MOD_ALT 4

struct WINDOW;
struct lua_State;

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

    class DFHACK_EXPORT PerfCounters
    {
    public:
        uint32_t baseline_elapsed_ms;
        uint32_t elapsed_ms;
        uint32_t total_update_ms;
        uint32_t update_event_manager_ms;
        uint32_t update_plugin_ms;
        uint32_t update_lua_ms;
        uint32_t total_keybinding_ms;
        uint32_t total_overlay_ms;
        std::unordered_map<int32_t, uint32_t> event_manager_event_total_ms;
        std::unordered_map<int32_t, std::unordered_map<std::string, uint32_t>> event_manager_event_per_plugin_ms;
        std::unordered_map<std::string, uint32_t> update_per_plugin;
        std::unordered_map<std::string, uint32_t> state_change_per_plugin;
        std::unordered_map<std::string, uint32_t> update_lua_per_repeat;
        std::unordered_map<std::string, uint32_t> overlay_per_widget;
        std::unordered_map<std::string, uint32_t> zscreen_per_focus;

        void reset(bool ignorePauseState = false);
        bool getIgnorePauseState();

        // noop if game is paused and getIgnorePauseState() returns false
        void incCounter(uint32_t &perf_counter, uint32_t baseline_ms);

        void registerTick(uint32_t baseline_ms);
        uint32_t getUnpausedFps();

    private:
        bool ignore_pause_state = false;

        static const size_t RECENT_TICKS_HISTORY_SIZE = 1000;
        int32_t last_frame_counter;
        uint32_t last_tick_baseline_ms;
        struct {
            uint32_t history[RECENT_TICKS_HISTORY_SIZE];
            size_t head_idx;
            bool full;
            uint32_t sum_ms;
        } recent_ticks;
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
        bool const operator==(const StateChangeScript& other)
        {
            return event == other.event && path == other.path && save_specific == other.save_specific;
        }
        bool const operator!=(const StateChangeScript& other)
        {
            return !(operator==(other));
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
        static Core& getInstance();
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

        command_result runCommand(color_ostream &out, const std::string &command, std::vector <std::string> &parameters, bool no_autocomplete = false);
        command_result runCommand(color_ostream &out, const std::string &command);
        void getAutoCompletePossibles(const std::string &command, std::vector<std::string> &possibles);
        bool loadScriptFile(color_ostream &out, std::string fname, bool silent = false);

        bool addScriptPath(std::string path, bool search_before = false);
        bool setModScriptPaths(const std::vector<std::string> &mod_script_paths);
        bool removeScriptPath(std::string path);
        std::string findScript(std::string name);
        void getScriptPaths(std::vector<std::string> *dest);

        bool getSuppressDuplicateKeyboardEvents();
        void setSuppressDuplicateKeyboardEvents(bool suppress);
        void setMortalMode(bool value);
        void setArmokTools(const std::vector<std::string> &tool_names);

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

        static df::viewscreen *getTopViewscreen();

        DFHack::Console &getConsole() {
            static std::unique_ptr<Console> con = Console::makeConsole();
            return *con;
        }

        std::unique_ptr<DFHack::Process> p;
        std::shared_ptr<DFHack::VersionInfo> vinfo;

        static void print(const char *format, ...) Wformat(printf,1,2);
        static void printerr(const char *format, ...) Wformat(printf,1,2);

        PluginManager *getPluginManager() { return plug_mgr; }

        static void cheap_tokenise(std::string const& input, std::vector<std::string> &output);

        PerfCounters perf_counters;

        lua_State* getLuaState(bool bypass_assertion = false) {
            assert(bypass_assertion || isSuspended());
            return State;
        }

    private:
        DFHack::Console& con;

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

        bool doSdlInputEvent(SDL_Event* event);
        void doUpdate(color_ostream &out);
        void onUpdate(color_ostream &out);
        void onStateChange(color_ostream &out, state_change_event event);
        void handleLoadAndUnloadScripts(color_ostream &out, state_change_event event);

        Core(Core const&);              // Don't Implement
        void operator=(Core const&);    // Don't implement

        // report error to user while failing
        void fatal (std::string output, const char * title = NULL);

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

        bool suppress_duplicate_keyboard_events;
        bool mortal_mode;
        std::unordered_set<std::string> armok_tools;
        std::map<int, std::vector<KeyBinding> > key_bindings;
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
        std::recursive_timed_mutex CoreSuspendMutex;
        std::condition_variable_any CoreWakeup;
        std::atomic<std::thread::id> ownerThread;
        std::atomic<size_t> toolCount;
        std::atomic<bool> shutdown;
        //! \}

        std::thread::id df_render_thread;
        std::thread::id df_simulation_thread;

        lua_State* State;

        friend class CoreService;
        friend class ServerConnection;
        friend class CoreSuspender;
        friend class CoreSuspenderBase;
        friend struct CoreSuspendClaimMain;
        friend struct CoreSuspendReleaseMain;
    };

    class CoreSuspenderBase : protected std::unique_lock< decltype(Core::CoreSuspendMutex) > {
    protected:
        using mutex_type = decltype(Core::CoreSuspendMutex);
        using parent_t = std::unique_lock< mutex_type >;
        std::thread::id tid;
        Core& core;

        CoreSuspenderBase() : CoreSuspenderBase{ Core::getInstance() } {}

        CoreSuspenderBase(Core& core) :
            /* Lock the core (jk not really) */
            parent_t{core.CoreSuspendMutex,std::defer_lock},
            /* Mark this thread to be the core owner */
            tid{},
            core{ core }
        {
            assert(core.df_render_thread != std::thread::id{} && core.df_render_thread != std::this_thread::get_id());
        }
    public:
        void lock()
        {
            parent_t::lock();
            complete_lock();
        }

        bool try_lock()
        {
            bool locked = parent_t::try_lock_for(std::chrono::milliseconds(100));
            if (locked) complete_lock();
            return locked;
        }

        void unlock()
        {
            /* Restore core owner to previous value */
            if (tid == std::thread::id{})
                Lua::Core::Reset(core.getConsole(), "suspend");
            core.ownerThread.store(tid, std::memory_order_release);
            parent_t::unlock();
        }

        ~CoreSuspenderBase() {
            if (owns_lock())
                unlock();
        }

    protected:
        void complete_lock()
        {
            tid = core.ownerThread.exchange(std::this_thread::get_id(),
                std::memory_order_acquire);
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
     *
     * The public API for CoreSuspender only supports construction and destruction;
     * all other locking operations are reserved
     */

    class CoreSuspender : protected CoreSuspenderBase {
        using parent_t = CoreSuspenderBase;
    public:
        CoreSuspender() : CoreSuspender{Core::getInstance()} {}

        CoreSuspender(Core& core) : CoreSuspenderBase{core}
        {
            lock();
        }

        // note that this is needed so the destructor will call CoreSuspender::unlock instead of CoreSuspenderBase::unlock
        ~CoreSuspender() {
            if (owns_lock())
                unlock();
        }

    protected:
        // deferred locking is not part of CoreSuspender's public API
        // these constructors are only for use in derived classes,
        // specifically ConditionalCoreSuspender
        CoreSuspender(std::defer_lock_t d) : CoreSuspender{ Core::getInstance(),d } {}
        CoreSuspender(Core& core, std::defer_lock_t d) : CoreSuspenderBase{ core } {}

        void lock()
        {
            inc_tool_count();
            parent_t::lock();
        }

        bool try_lock()
        {
            inc_tool_count();
            if (parent_t::try_lock())
                return true;
            dec_tool_count();
            return false;
        }

        void unlock()
        {
            parent_t::unlock();
            dec_tool_count();
        }

        void inc_tool_count()
        {
            core.toolCount.fetch_add(1, std::memory_order_relaxed);
        }

        void dec_tool_count()
        {
            /* Notify core to continue when all queued tools have completed
             * 0 = None wants to own the core
             * 1+ = There are tools waiting core access
             * fetch_add returns old value before subtraction
             */
            if (core.toolCount.fetch_add(-1, std::memory_order_relaxed) == 1)
                core.CoreWakeup.notify_one();
        }
    };

    /*!
     * ConditionalCoreSuspender attempts to acquire a CoreSuspender, but will fail if doing so
     * would cause a thread wait. The caller can determine if the suspender was acquired by casting
     * the created object to bool:
     *     ConditionalCoreSuspender suspend;
     *     if (suspend) {
     *         ...
     *     }
     */

    class ConditionalCoreSuspender : protected CoreSuspender {
    public:
        ConditionalCoreSuspender() : ConditionalCoreSuspender{ Core::getInstance() } { }
        ConditionalCoreSuspender(Core& core) : CoreSuspender{ core, std::defer_lock } { try_lock(); }

        operator bool() const { return owns_lock(); }
    };

}
