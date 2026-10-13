#pragma once

struct lua_State;

namespace DFHack {

/**
 * Installs/shuts down the platform crash reporter. Implemented
 * per-platform (Crashlog.cpp on Linux, Crashlog-windows.cpp on Windows).
 */
void dfhack_crashlog_init();
void dfhack_crashlog_shutdown();

/**
 * Support for post-mortem crash reporting.
 *
 * The platform crash reporter (see Crashlog.cpp / Crashlog-windows.cpp)
 * consults this to produce a Lua stack traceback when a fatal exception
 * occurs while Lua code is executing.
 */
namespace Crashlog {

    /**
     * The lua_State currently executing a DFHack Lua entry point
     * (Lua::SafeCall, Lua::SafeResume, ...) on the calling thread, or
     * nullptr if this thread is not inside a Lua call.
     *
     * Note that a thread's active state may be a coroutine rather than the
     * main state, which is exactly what a traceback wants.
     */
    lua_State *active_state();

    /**
     * Sets the calling thread's active lua_State, returning the previous
     * value. Callers should generally prefer ActiveStateGuard.
     */
    lua_State *set_active_state(lua_State *state);

    /**
     * RAII helper to maintain active_state() around a Lua entry point.
     */
    class ActiveStateGuard {
        lua_State *previous;
    public:
        explicit ActiveStateGuard(lua_State *state) :
            previous(set_active_state(state)) {}
        ~ActiveStateGuard() { set_active_state(previous); }

        ActiveStateGuard(const ActiveStateGuard&) = delete;
        ActiveStateGuard& operator=(const ActiveStateGuard&) = delete;
    };
}
}
