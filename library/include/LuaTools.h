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

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "DataDefs.h"

#include <lua.h>
#include <lauxlib.h>

/// Allocate a new user data object and push it on the stack
inline void *operator new (std::size_t size, lua_State *L) {
    return lua_newuserdata(L, size);
}

namespace DFHack {
    class function_identity_base;
    struct MaterialInfo;

    namespace Units {
        struct NoblePosition;
    }
    namespace Screen {
        struct Pen;
    };
}

namespace DFHack {namespace Lua {
    /**
     * Create or initialize a lua interpreter with access to DFHack tools.
     */
    DFHACK_EXPORT lua_State *Open(color_ostream &out, lua_State *state = NULL);

    DFHACK_EXPORT void PushDFHack(lua_State *state);
    DFHACK_EXPORT void PushBaseGlobals(lua_State *state);

    /**
     * Load a module using require(). Leaves the stack as is.
     */
    DFHACK_EXPORT bool Require(color_ostream &out, lua_State *state,
                               const std::string &module, bool setglobal = false);

    /**
     * Push the module table, loading it using require() if necessary.
     */
    DFHACK_EXPORT bool PushModule(color_ostream &out, lua_State *state, const char *module);

    /**
     * Push the public object name exported by the module. Uses PushModule.
     */
    DFHACK_EXPORT bool PushModulePublic(color_ostream &out, lua_State *state,
                                        const char *module, const char *name);

    /**
     * Check if the object at the given index is NIL or NULL.
     */
    DFHACK_EXPORT bool IsDFNull(lua_State *state, int val_index);

    enum ObjectClass {
        /** Not a DF wrapper object */
        OBJ_INVALID = 0,
        /** NIL or NULL */
        OBJ_NULL,
        /** A named type identity object */
        OBJ_TYPE,
        /** A void* reference, i.e. non-null lightuserdata */
        OBJ_VOIDPTR,
        /** A typed object reference */
        OBJ_REF
    };

    /**
     * Check if the object at the given index is a valid wrapper object.
     */
    DFHACK_EXPORT ObjectClass IsDFObject(lua_State *state, int val_index);

    /**
     * Push the pointer onto the stack as a wrapped DF object of the given type.
     */
    DFHACK_EXPORT void PushDFObject(lua_State *state, type_identity *type, void *ptr);

    /**
     * Check that the value is a wrapped DF object of the given type, and if so return the pointer.
     */
    DFHACK_EXPORT void *GetDFObject(lua_State *state, type_identity *type, int val_index, bool exact_type = false);

    /**
     * Check that the value is a wrapped DF object of the given type, and if so return the pointer.
     * Otherwise throw an argument type error.
     */
    DFHACK_EXPORT void *CheckDFObject(lua_State *state, type_identity *type, int val_index, bool exact_type = false);

    /**
     * Assign the value at val_index to the target of given identity using df.assign().
     * Return behavior is of SafeCall below.
     */
    DFHACK_EXPORT bool AssignDFObject(color_ostream &out, lua_State *state,
                                      type_identity *type, void *target, int val_index,
                                      bool exact_type = false, bool perr = true);

    /**
     * Assign the value at val_index to the target of given identity using df.assign().
     * Otherwise throws an error.
     */
    DFHACK_EXPORT void CheckDFAssign(lua_State *state, type_identity *type,
                                     void *target, int val_index, bool exact_type = false);

    /**
     * Push the pointer onto the stack as a wrapped DF object of a specific type.
     */
    template<class T>
    void PushDFObject(lua_State *state, T *ptr) {
        PushDFObject(state, df::identity_traits<T>::get(), ptr);
    }

    /**
     * Check that the value is a wrapped DF object of the correct type, and if so return the pointer.
     */
    template<class T>
    T *GetDFObject(lua_State *state, int val_index, bool exact_type = false) {
        return (T*)GetDFObject(state, df::identity_traits<T>::get(), val_index, exact_type);
    }

    /**
     * Check that the value is a wrapped DF object of the correct type, and if so return the pointer. Otherwise throw an argument type error.
     */
    template<class T>
    T *CheckDFObject(lua_State *state, int val_index, bool exact_type = false) {
        return (T*)CheckDFObject(state, df::identity_traits<T>::get(), val_index, exact_type);
    }

    /**
     * Assign the value at val_index to the target using df.assign().
     */
    template<class T>
    bool AssignDFObject(color_ostream &out, lua_State *state, T *target,
                        int val_index, bool exact_type = false, bool perr = true) {
        return AssignDFObject(out, state, df::identity_traits<T>::get(),
                              target, val_index, exact_type, perr);
    }

    /**
     * Assign the value at val_index to the target using df.assign().
     * Throws in case of an error.
     */
    template<class T>
    void CheckDFAssign(lua_State *state, T *target, int val_index, bool exact_type = false) {
        CheckDFAssign(state, df::identity_traits<T>::get(), target, val_index, exact_type);
    }

    /**
     * Check if the status is a success, i.e. LUA_OK or LUA_YIELD.
     */
    inline bool IsSuccess(int status) {
        return (status == LUA_OK || status == LUA_YIELD);
    }

    // Internal helper
    template<int (*cb)(lua_State*,int,int)>
    int TailPCallK_Thunk(lua_State *state) {
        int tmp;
        int rv = lua_getctx(state, &tmp);
        return cb(state, rv, tmp);
    }

    /**
     * A utility for using the restartable pcall feature more conveniently;
     * specifically, the callback is called with the same kind of arguments
     * in both yield and non-yield case.
     */
    template<int (*cb)(lua_State*,int,int)>
    int TailPCallK(lua_State *state, int narg, int nret, int errfun, int ctx) {
        int rv = lua_pcallk(state, narg, nret, errfun, ctx, &TailPCallK_Thunk<cb>);
        return cb(state, rv, ctx);
    }

    /**
     * Call through to the function with try/catch for C++ exceptions.
     */
    DFHACK_EXPORT int CallWithCatch(lua_State *, int (*fn)(lua_State*), const char *context = NULL);

    template<int (*cb)(lua_State*)>
    int CallWithCatchWrapper(lua_State *state) {
        return CallWithCatch(state, cb);
    }

    /**
     * Invoke lua function via pcall. Returns true if success.
     * If an error is signalled, and perr is true, it is printed and popped from the stack.
     */
    DFHACK_EXPORT bool SafeCall(color_ostream &out, lua_State *state, int nargs, int nres, bool perr = true);

    /**
     * Pops a function from the top of the stack, and pushes a new coroutine.
     */
    DFHACK_EXPORT lua_State *NewCoroutine(lua_State *state);

    /**
     * Resume the coroutine using nargs values from state from. Results or the error are moved back.
     * If an error is signalled, and perr is true, it is printed and popped from the stack.
     * Returns the lua_resume return value.
     */
    DFHACK_EXPORT int SafeResume(color_ostream &out, lua_State *from, lua_State *thread, int nargs, int nres, bool perr = true);

    /**
     * Works just like SafeCall, only expects a coroutine on the stack
     * instead of a function. Returns the lua_resume return value.
     */
    DFHACK_EXPORT int SafeResume(color_ostream &out, lua_State *from, int nargs, int nres, bool perr = true);

    /**
     * Parse code from string with debug_tag and env_idx, then call it using SafeCall.
     * In case of error, it is either left on the stack, or printed like SafeCall does.
     */
    DFHACK_EXPORT bool SafeCallString(color_ostream &out, lua_State *state, const std::string &code,
                                      int nargs, int nres, bool perr = true,
                                      const char *debug_tag = NULL, int env_idx = 0);

    /**
     * Returns the ostream passed to SafeCall.
     */
    DFHACK_EXPORT color_ostream *GetOutput(lua_State *state);

    /**
     * Run an interactive interpreter loop if possible, or return false.
     * Uses RunCoreQueryLoop internally.
     */
    DFHACK_EXPORT bool InterpreterLoop(color_ostream &out, lua_State *state,
                                       const char *prompt = NULL, const char *hfile = NULL);

    /**
     * Run an interactive prompt loop. All access to the lua state
     * is done inside CoreSuspender, while waiting for input happens
     * without the suspend lock.
     */
    DFHACK_EXPORT bool RunCoreQueryLoop(color_ostream &out, lua_State *state,
                                        bool (*init)(color_ostream&, lua_State*, void*),
                                        void *arg);

    /**
     * Attempt to interrupt the currently-executing lua function by raising a lua error
     * from a lua debug hook, similar to how SIGINT is handled in the lua interpreter (depends/lua/src/lua.c).
     * The flag set here will only be checked every 256 instructions by default.
     * Returns false if another debug hook is set and 'force' is false.
     *
     * force: Overwrite any existing debug hooks and interrupt the next instruction
     */

    DFHACK_EXPORT bool Interrupt (bool force=false);

    /**
     * Push utility functions
     */
#if 0
#define NUMBER_PUSH(type) inline void Push(lua_State *state, type value) { lua_pushnumber(state, value); }
    NUMBER_PUSH(char)
    NUMBER_PUSH(int8_t) NUMBER_PUSH(uint8_t)
    NUMBER_PUSH(int16_t) NUMBER_PUSH(uint16_t)
    NUMBER_PUSH(int32_t) NUMBER_PUSH(uint32_t)
    NUMBER_PUSH(int64_t) NUMBER_PUSH(uint64_t)
    NUMBER_PUSH(float) NUMBER_PUSH(double)
#undef NUMBER_PUSH
#else
    template<class T> inline void Push(lua_State *state, T value) {
        lua_pushnumber(state, lua_Number(value));
    }
#endif
    inline void Push(lua_State *state, bool value) {
        lua_pushboolean(state, value);
    }
    inline void Push(lua_State *state, const char *str) {
        lua_pushstring(state, str);
    }
    inline void Push(lua_State *state, const std::string &str) {
        lua_pushlstring(state, str.data(), str.size());
    }
    DFHACK_EXPORT void Push(lua_State *state, df::coord obj);
    DFHACK_EXPORT void Push(lua_State *state, df::coord2d obj);
    void Push(lua_State *state, const Units::NoblePosition &pos);
    DFHACK_EXPORT void Push(lua_State *state, MaterialInfo &info);
    DFHACK_EXPORT void Push(lua_State *state, const Screen::Pen &info);
    template<class T> inline void Push(lua_State *state, T *ptr) {
        PushDFObject(state, ptr);
    }

    template<class T> inline void SetField(lua_State *L, T val, int idx, const char *name) {
        if (idx < 0) idx = lua_absindex(L, idx);
        Push(L, val); lua_setfield(L, idx, name);
    }

    template<class T>
    void PushVector(lua_State *state, const T &pvec, bool addn = false)
    {
        lua_createtable(state,pvec.size(), addn?1:0);

        if (addn)
        {
            lua_pushinteger(state, pvec.size());
            lua_setfield(state, -2, "n");
        }

        for (size_t i = 0; i < pvec.size(); i++)
        {
            Push(state, pvec[i]);
            lua_rawseti(state, -2, i+1);
        }
    }

    DFHACK_EXPORT int PushPosXYZ(lua_State *state, df::coord pos);
    DFHACK_EXPORT int PushPosXY(lua_State *state, df::coord2d pos);

    DFHACK_EXPORT void CheckPen(lua_State *L, Screen::Pen *pen, int index, bool allow_nil = false, bool allow_color = true);

    DFHACK_EXPORT bool IsCoreContext(lua_State *state);

    namespace Event {
        struct DFHACK_EXPORT Owner {
            virtual ~Owner() {}
            virtual void on_count_changed(int new_cnt, int delta) {}
            virtual void on_invoked(lua_State *state, int nargs, bool from_c) {}
        };

        DFHACK_EXPORT void New(lua_State *state, Owner *owner = NULL);
        DFHACK_EXPORT void Make(lua_State *state, void *key, Owner *owner = NULL);
        DFHACK_EXPORT void SetPrivateCallback(lua_State *state, int ev_idx);
        DFHACK_EXPORT void Invoke(color_ostream &out, lua_State *state, void *key, int num_args);
    }

    class StackUnwinder {
        lua_State *state;
        int top;
    public:
        StackUnwinder(lua_State *state, int bias = 0) : state(state), top(0) {
            if (state) top = lua_gettop(state) - bias;
        }
        ~StackUnwinder() {
            if (state) lua_settop(state, top);
        }
        operator int () { return top; }
        int operator+ (int v) { return top + v; }
        int operator- (int v) { return top + v; }
        int operator[] (int v) { return top + v; }
        StackUnwinder &operator += (int v) { top += v; return *this; }
        StackUnwinder &operator -= (int v) { top += v; return *this; }
        StackUnwinder &operator ++ () { top++; return *this; }
        StackUnwinder &operator -- () { top--; return *this; }
    };

    /**
     * Namespace for the common lua interpreter state.
     * All accesses must be done under CoreSuspender.
     */
    namespace Core {
        DFHACK_EXPORT extern lua_State *State;

        // Not exported; for use by the Core class
        bool Init(color_ostream &out);
        void Reset(color_ostream &out, const char *where);

        // Events signalled by the core
        void onStateChange(color_ostream &out, int code);
        // Signals timers
        void onUpdate(color_ostream &out);

        template<class T> inline void Push(T &arg) { Lua::Push(State, arg); }
        template<class T> inline void Push(const T &arg) { Lua::Push(State, arg); }
        template<class T> inline void PushVector(const T &arg) { Lua::PushVector(State, arg); }

        inline bool SafeCall(color_ostream &out, int nargs, int nres, bool perr = true) {
            return Lua::SafeCall(out, State, nargs, nres, perr);
        }
        inline bool PushModule(color_ostream &out, const char *module) {
            return Lua::PushModule(out, State, module);
        }
        inline bool PushModulePublic(color_ostream &out, const char *module, const char *name) {
            return Lua::PushModulePublic(out, State, module, name);
        }
    }

    class DFHACK_EXPORT Notification : public Event::Owner {
        lua_State *state;
        void *key;
        function_identity_base *handler;
        int count;

    public:
        Notification(function_identity_base *handler = NULL)
            : state(NULL), key(NULL), handler(handler), count(0) {}

        int get_listener_count() { return count; }
        lua_State *get_state() { return state; }
        function_identity_base *get_handler() { return handler; }

        lua_State *state_if_count() { return (count > 0) ? state : NULL; }

        void on_count_changed(int new_cnt, int) { count = new_cnt; }

        void invoke(color_ostream &out, int nargs);

        void bind(lua_State *state, const char *name);
        void bind(lua_State *state, void *key);
    };
}}

#define DEFINE_LUA_EVENT_0(name, handler) \
    static DFHack::Lua::Notification name##_event(df::wrap_function(handler, true)); \
    void name(color_ostream &out) { \
        handler(out); \
        if (name##_event.state_if_count()) { \
            name##_event.invoke(out, 0); \
        } \
    }

#define DEFINE_LUA_EVENT_1(name, handler, arg_type1) \
    static DFHack::Lua::Notification name##_event(df::wrap_function(handler, true)); \
    void name(color_ostream &out, arg_type1 arg1) { \
        handler(out, arg1); \
        if (auto state = name##_event.state_if_count()) { \
            DFHack::Lua::Push(state, arg1); \
            name##_event.invoke(out, 1); \
        } \
    }

#define DEFINE_LUA_EVENT_2(name, handler, arg_type1, arg_type2) \
    static DFHack::Lua::Notification name##_event(df::wrap_function(handler, true)); \
    void name(color_ostream &out, arg_type1 arg1, arg_type2 arg2) { \
        handler(out, arg1, arg2); \
        if (auto state = name##_event.state_if_count()) { \
            DFHack::Lua::Push(state, arg1); \
            DFHack::Lua::Push(state, arg2); \
            name##_event.invoke(out, 2); \
        } \
    }

#define DEFINE_LUA_EVENT_3(name, handler, arg_type1, arg_type2, arg_type3) \
    static DFHack::Lua::Notification name##_event(df::wrap_function(handler, true)); \
    void name(color_ostream &out, arg_type1 arg1, arg_type2 arg2, arg_type3 arg3) { \
        handler(out, arg1, arg2, arg3); \
        if (auto state = name##_event.state_if_count()) { \
            DFHack::Lua::Push(state, arg1); \
            DFHack::Lua::Push(state, arg2); \
            DFHack::Lua::Push(state, arg3); \
            name##_event.invoke(out, 3); \
        } \
    }

#define DEFINE_LUA_EVENT_4(name, handler, arg_type1, arg_type2, arg_type3, arg_type4) \
    static DFHack::Lua::Notification name##_event(df::wrap_function(handler, true)); \
    void name(color_ostream &out, arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4) { \
        handler(out, arg1, arg2, arg3, arg4); \
        if (auto state = name##_event.state_if_count()) { \
            DFHack::Lua::Push(state, arg1); \
            DFHack::Lua::Push(state, arg2); \
            DFHack::Lua::Push(state, arg3); \
            DFHack::Lua::Push(state, arg4); \
            name##_event.invoke(out, 4); \
        } \
    }

#define DEFINE_LUA_EVENT_5(name, handler, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5) \
    static DFHack::Lua::Notification name##_event(df::wrap_function(handler, true)); \
    void name(color_ostream &out, arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4, arg_type5 arg5) { \
        handler(out, arg1, arg2, arg3, arg4, arg5); \
        if (auto state = name##_event.state_if_count()) { \
            DFHack::Lua::Push(state, arg1); \
            DFHack::Lua::Push(state, arg2); \
            DFHack::Lua::Push(state, arg3); \
            DFHack::Lua::Push(state, arg4); \
            DFHack::Lua::Push(state, arg5); \
            name##_event.invoke(out, 5); \
        } \
    }

#define DEFINE_LUA_EVENT_6(name, handler, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5,arg_type6) \
    static DFHack::Lua::Notification name##_event(df::wrap_function(handler, true)); \
    void name(color_ostream &out, arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4,arg_type5 arg5, arg_type6 arg6) { \
    handler(out, arg1, arg2, arg3, arg4, arg5, arg6); \
    if (auto state = name##_event.state_if_count()) { \
    DFHack::Lua::Push(state, arg1); \
    DFHack::Lua::Push(state, arg2); \
    DFHack::Lua::Push(state, arg3); \
    DFHack::Lua::Push(state, arg4); \
    DFHack::Lua::Push(state, arg5); \
    DFHack::Lua::Push(state, arg6); \
    name##_event.invoke(out, 6); \
    } \
}

#define DEFINE_LUA_EVENT_7(name, handler, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5,arg_type6,arg_type7) \
    static DFHack::Lua::Notification name##_event(df::wrap_function(handler, true)); \
    void name(color_ostream &out, arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4,arg_type5 arg5, arg_type6 arg6, arg_type7 arg7) { \
    handler(out, arg1, arg2, arg3, arg4, arg5, arg6, arg7); \
    if (auto state = name##_event.state_if_count()) { \
    DFHack::Lua::Push(state, arg1); \
    DFHack::Lua::Push(state, arg2); \
    DFHack::Lua::Push(state, arg3); \
    DFHack::Lua::Push(state, arg4); \
    DFHack::Lua::Push(state, arg5); \
    DFHack::Lua::Push(state, arg6); \
    DFHack::Lua::Push(state, arg7); \
    name##_event.invoke(out, 7); \
    } \
}
