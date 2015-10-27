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

#include "Internal.h"

#include <csignal>
#include <string>
#include <vector>
#include <map>

#include "MemAccess.h"
#include "Core.h"
#include "VersionInfo.h"
#include "tinythread.h"
// must be last due to MS stupidity
#include "DataDefs.h"
#include "DataIdentity.h"
#include "DataFuncs.h"

#include "modules/World.h"
#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/Translation.h"
#include "modules/Units.h"

#include "LuaWrapper.h"
#include "LuaTools.h"

#include "MiscUtils.h"
#include "DFHackVersion.h"
#include "PluginManager.h"

#include "df/job.h"
#include "df/job_item.h"
#include "df/building.h"
#include "df/unit.h"
#include "df/item.h"
#include "df/world.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <lstate.h>

using namespace DFHack;
using namespace DFHack::LuaWrapper;

lua_State *DFHack::Lua::Core::State = NULL;

void dfhack_printerr(lua_State *S, const std::string &str);

inline bool is_null_userdata(lua_State *L, int idx)
{
    return lua_islightuserdata(L, idx) && !lua_touserdata(L, idx);
}

inline void AssertCoreSuspend(lua_State *state)
{
    assert(!Lua::IsCoreContext(state) || DFHack::Core::getInstance().isSuspended());
}

/*
 * Public DF object reference handling API
 */

void DFHack::Lua::PushDFObject(lua_State *state, type_identity *type, void *ptr)
{
    push_object_internal(state, type, ptr, false);
}

void *DFHack::Lua::GetDFObject(lua_State *state, type_identity *type, int val_index, bool exact_type)
{
    return get_object_internal(state, type, val_index, exact_type, false);
}

static void check_valid_ptr_index(lua_State *state, int val_index)
{
    if (lua_type(state, val_index) == LUA_TNONE)
    {
        if (val_index > 0)
            luaL_argerror(state, val_index, "pointer expected");
        else
            luaL_error(state, "at index %d: pointer expected", val_index);
    }
}

static void signal_typeid_error(color_ostream *out, lua_State *state,
                                type_identity *type, const char *msg,
                                int val_index, bool perr, bool signal)
{
    std::string typestr = type ? type->getFullName() : "any pointer";
    std::string error = stl_sprintf(msg, typestr.c_str());

    if (signal)
    {
        if (val_index > 0)
            luaL_argerror(state, val_index, error.c_str());
        else
            luaL_error(state, "at index %d: %s", val_index, error.c_str());
    }
    else if (perr)
    {
        if (out)
            out->printerr("%s", error.c_str());
        else
            dfhack_printerr(state, error);
    }
    else
        lua_pushstring(state, error.c_str());
}


void *DFHack::Lua::CheckDFObject(lua_State *state, type_identity *type, int val_index, bool exact_type)
{
    check_valid_ptr_index(state, val_index);

    if (lua_isnil(state, val_index))
        return NULL;
    if (lua_islightuserdata(state, val_index) && !lua_touserdata(state, val_index))
        return NULL;

    void *rv = get_object_internal(state, type, val_index, exact_type, false);

    if (!rv)
        signal_typeid_error(NULL, state, type, "invalid pointer type; expected: %s",
                            val_index, false, true);

    return rv;
}

/*
 * Console I/O wrappers
 */

static int DFHACK_OSTREAM_TOKEN = 0;

color_ostream *DFHack::Lua::GetOutput(lua_State *L)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_OSTREAM_TOKEN);
    auto rv = (color_ostream*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return rv;
}

df::cur_lua_ostream_argument::cur_lua_ostream_argument(lua_State *state)
{
    out = DFHack::Lua::GetOutput(state);
    if (!out)
        LuaWrapper::field_error(state, UPVAL_METHOD_NAME, "no output stream", "invoke");
}

static void set_dfhack_output(lua_State *L, color_ostream *p)
{
    lua_pushlightuserdata(L, p);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &DFHACK_OSTREAM_TOKEN);
}

static Console *get_console(lua_State *state)
{
    color_ostream *pstream = Lua::GetOutput(state);

    if (!pstream)
    {
        lua_pushnil(state);
        lua_pushstring(state, "no output stream");
        return NULL;
    }

    if (!pstream->is_console())
    {
        lua_pushnil(state);
        lua_pushstring(state, "not an interactive console");
        return NULL;
    }

    return static_cast<Console*>(pstream);
}

static int DFHACK_TOSTRING_TOKEN = 0;

static std::string lua_print_fmt(lua_State *L)
{
    /* Copied from lua source to fully replicate builtin print */
    int n = lua_gettop(L);  /* number of arguments */
    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_TOSTRING_TOKEN);

    std::stringstream ss;

    for (int i=1; i<=n; i++) {
        lua_pushvalue(L, -1);  /* function to be called */
        lua_pushvalue(L, i);   /* value to print */
        lua_call(L, 1, 1);
        const char *s = lua_tostring(L, -1);  /* get result */
        if (s == NULL)
            luaL_error(L, "tostring must return a string to print");
        if (i>1)
            ss << '\t';
        ss << s;
        lua_pop(L, 1);  /* pop result */
    }

    return ss.str();
}

static int lua_dfhack_print(lua_State *S)
{
    std::string str = lua_print_fmt(S);
    if (color_ostream *out = Lua::GetOutput(S))
        out->print("%s", str.c_str());//*out << str;
    else
        Core::print("%s", str.c_str());
    return 0;
}

static int lua_dfhack_println(lua_State *S)
{
    std::string str = lua_print_fmt(S);
    if (color_ostream *out = Lua::GetOutput(S))
        *out << str << std::endl;
    else
        Core::print("%s\n", str.c_str());
    return 0;
}

void dfhack_printerr(lua_State *S, const std::string &str)
{
    if (color_ostream *out = Lua::GetOutput(S))
        out->printerr("%s\n", str.c_str());
    else
        Core::printerr("%s\n", str.c_str());
}

static int lua_dfhack_printerr(lua_State *S)
{
    std::string str = lua_print_fmt(S);
    dfhack_printerr(S, str);
    return 0;
}

static int lua_dfhack_color(lua_State *S)
{
    int cv = luaL_optint(S, 1, -1);

    if (cv < -1 || cv > COLOR_MAX)
        luaL_argerror(S, 1, "invalid color value");

    color_ostream *out = Lua::GetOutput(S);
    if (out) {
        lua_pushinteger(S, (int)out->color());
        out->color(color_ostream::color_value(cv));
        return 1;
    }
    return 0;
}

static int lua_dfhack_is_interactive(lua_State *S)
{
    lua_pushboolean(S, get_console(S) != NULL);
    return 1;
}

static int dfhack_lineedit_sync(lua_State *S, Console *pstream)
{
    if (!pstream)
        return 2;

    const char *prompt = luaL_optstring(S, 1, ">> ");
    const char *hfile = luaL_optstring(S, 2, NULL);

    DFHack::CommandHistory hist;
    if (hfile)
        hist.load(hfile);

    std::string ret;
    int rv = pstream->lineedit(prompt, ret, hist);

    if (rv < 0)
    {
        lua_pushnil(S);
        lua_pushstring(S, "input error");
        return 2;
    }
    else
    {
        if (hfile)
            hist.save(hfile);
        lua_pushlstring(S, ret.data(), ret.size());
        return 1;
    }
}

static int DFHACK_QUERY_COROTABLE_TOKEN = 0;

static int yield_helper(lua_State *S)
{
    return lua_yield(S, lua_gettop(S));
}

namespace {
    int dfhack_lineedit_cont(lua_State *L, int status, int)
    {
        if (Lua::IsSuccess(status))
            return lua_gettop(L) - 2;
        else
            return dfhack_lineedit_sync(L, get_console(L));
    }
}

static int dfhack_lineedit(lua_State *S)
{
    lua_settop(S, 2);

    Console *pstream = get_console(S);
    if (!pstream)
        return 2;

    lua_rawgetp(S, LUA_REGISTRYINDEX, &DFHACK_QUERY_COROTABLE_TOKEN);
    lua_rawgetp(S, -1, S);
    bool in_coroutine = !lua_isnil(S, -1);
    lua_settop(S, 2);

    if (in_coroutine)
    {
        lua_pushcfunction(S, yield_helper);
        lua_pushvalue(S, 1);
        lua_pushvalue(S, 2);
        return Lua::TailPCallK<dfhack_lineedit_cont>(S, 2, LUA_MULTRET, 0, 0);
    }

    return dfhack_lineedit_sync(S, pstream);
}

/*
 * Exception handling
 */

volatile std::sig_atomic_t lstop = 0;

static void interrupt_hook (lua_State *L, lua_Debug *ar);
static void interrupt_init (lua_State *L)
{
    lua_sethook(L, interrupt_hook, LUA_MASKCOUNT, 256);
}

static void interrupt_hook (lua_State *L, lua_Debug *ar)
{
    if (lstop)
    {
        lstop = 0;
        interrupt_init(L);  // Restore default settings if necessary
        luaL_error(L, "interrupted!");
    }
}

bool DFHack::Lua::Interrupt (bool force)
{
    lua_State *L = Lua::Core::State;
    if (L->hook != interrupt_hook && !force)
        return false;
    if (force)
        lua_sethook(L, interrupt_hook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT, 1);
    lstop = 1;
    return true;
}

static int DFHACK_EXCEPTION_META_TOKEN = 0;

static void error_tostring(lua_State *L, bool keep_old = false)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_TOSTRING_TOKEN);
    if (keep_old)
        lua_pushvalue(L, -2);
    else
        lua_swap(L);

    bool ok = lua_pcall(L, 1, 1, 0) == LUA_OK;

    const char *msg = lua_tostring(L, -1);
    if (!msg)
    {
        msg = "tostring didn't return a string";
        ok = false;
    }

    if (!ok)
    {
        lua_pushfstring(L, "(invalid error: %s)", msg);
        lua_remove(L, -2);
    }
}

static void report_error(lua_State *L, color_ostream *out = NULL, bool pop = false)
{
    error_tostring(L, true);

    const char *msg = lua_tostring(L, -1);
    assert(msg);

    if (out)
        out->printerr("%s\n", msg);
    else
        dfhack_printerr(L, msg);

    lua_pop(L, pop?2:1);
}

static bool convert_to_exception(lua_State *L, int slevel, lua_State *thread = NULL)
{
    if (!thread)
        thread = L;

    if (thread == L)
        lua_pushthread(L);
    else
    {
        lua_pushthread(thread);
        lua_xmove(thread, L, 1);
    }

    lua_swap(L);

    int base = lua_gettop(L);

    bool force_unknown = false;

    if (lua_istable(L, base) && lua_getmetatable(L, base))
    {
        lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_EXCEPTION_META_TOKEN);
        bool is_exception = lua_rawequal(L, -1, -2);
        lua_settop(L, base);

        if (is_exception)
        {
            // If it is an exception from the same thread, return as is
            lua_getfield(L, base, "thread");
            bool same_thread = lua_rawequal(L, -1, base-1);
            lua_settop(L, base);

            if (same_thread)
            {
                lua_remove(L, base-1);
                return false;
            }

            // Create a new exception for this thread
            lua_newtable(L);
            luaL_where(L, slevel);
            lua_setfield(L, -2, "where");
            lua_pushstring(L, "coroutine resume failed");
            lua_setfield(L, -2, "message");
            lua_getfield(L, -2, "verbose");
            lua_setfield(L, -2, "verbose");
            lua_swap(L);
            lua_setfield(L, -2, "cause");
        }
        else
            force_unknown = true;
    }

    // Promote non-table to table, and do some sanity checks
    if (!lua_istable(L, base) || force_unknown)
    {
        lua_newtable(L);
        lua_swap(L);

        if (lua_isstring(L, -1))
            lua_setfield(L, base, "message");
        else
        {
            error_tostring(L, true);
            lua_setfield(L, base, "message");
            lua_setfield(L, base, "object");
        }
    }
    else
    {
        lua_getfield(L, base, "message");

        if (!lua_isstring(L, -1))
        {
            error_tostring(L);
            lua_setfield(L, base, "message");
        }

        lua_settop(L, base);
    }

    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_EXCEPTION_META_TOKEN);
    lua_setmetatable(L, base);
    lua_swap(L);
    lua_setfield(L, -2, "thread");
    luaL_traceback(L, thread, NULL, slevel);
    lua_setfield(L, -2, "stacktrace");
    return true;
}

static int dfhack_onerror(lua_State *L)
{
    luaL_checkany(L, 1);
    lua_settop(L, 1);

    convert_to_exception(L, 1);
    return 1;
}

static int dfhack_error(lua_State *L)
{
    luaL_checkany(L, 1);
    lua_settop(L, 3);
    int level = std::max(1, luaL_optint(L, 2, 1));

    lua_pushvalue(L, 1);

    if (convert_to_exception(L, level))
    {
        luaL_where(L, level);
        lua_setfield(L, -2, "where");

        if (!lua_isnil(L, 3))
        {
            lua_pushvalue(L, 3);
            lua_setfield(L, -2, "verbose");
        }
    }

    return lua_error(L);
}

static int dfhack_exception_tostring(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 2);

    if (lua_isnil(L, 2))
    {
        lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_EXCEPTION_META_TOKEN);
        lua_getfield(L, -1, "verbose");
        lua_insert(L, 2);
        lua_settop(L, 2);
    }

    lua_getfield(L, 1, "verbose");

    bool verbose =
        lua_toboolean(L, 2) || lua_toboolean(L, 3) ||
        (lua_isnil(L, 2) && lua_isnil(L, 3));

    int base = lua_gettop(L);

    if (verbose || lua_isnil(L, 3))
    {
        lua_getfield(L, 1, "where");
        if (!lua_isstring(L, -1))
            lua_pop(L, 1);
    }

    lua_getfield(L, 1, "message");
    if (!lua_isstring(L, -1))
    {
        lua_pop(L, 1);
        lua_pushstring(L, "(error message is not a string)");
    }

    if (verbose)
    {
        lua_pushstring(L, "\n");
        lua_getfield(L, 1, "stacktrace");
        if (!lua_isstring(L, -1))
            lua_pop(L, 2);
    }

    lua_pushstring(L, "\ncaused by:\n");
    lua_getfield(L, 1, "cause");
    if (lua_isnil(L, -1))
        lua_pop(L, 2);
    else if (lua_istable(L, -1))
    {
        lua_pushcfunction(L, dfhack_exception_tostring);
        lua_swap(L);
        lua_pushvalue(L, 2);
        if (lua_pcall(L, 2, 1, 0) != LUA_OK)
            error_tostring(L);
    }
    else
        error_tostring(L);

    lua_concat(L, lua_gettop(L) - base);
    return 1;
}

static void push_simple_error(lua_State *L, const char *str)
{
    lua_pushstring(L, str);

    if (lua_checkstack(L, LUA_MINSTACK))
        convert_to_exception(L, 0);
}

static bool do_finish_pcall(lua_State *L, bool success, int base = 1, int space = 2)
{
    if (!lua_checkstack(L, space))
    {
        lua_settop(L, base-1);  /* create space for return values */
        lua_pushboolean(L, 0);
        push_simple_error(L, "stack overflow");
        return false;
    }
    else
    {
        lua_pushboolean(L, success);
        lua_replace(L, base); /* put first result in first slot */
        return success;
    }
}

namespace {
    int safecall_cont(lua_State *L, int status, int)
    {
        bool success = do_finish_pcall(L, Lua::IsSuccess(status));

        if (!success)
            report_error(L);

        return lua_gettop(L);
    }
}

static int dfhack_safecall (lua_State *L)
{
    luaL_checkany(L, 1);
    lua_pushcfunction(L, dfhack_onerror);
    lua_insert(L, 1);
    return Lua::TailPCallK<safecall_cont>(L, lua_gettop(L) - 2, LUA_MULTRET, 1, 0);
}

bool DFHack::Lua::SafeCall(color_ostream &out, lua_State *L, int nargs, int nres, bool perr)
{
    AssertCoreSuspend(L);

    int base = lua_gettop(L) - nargs;

    color_ostream *cur_out = Lua::GetOutput(L);
    set_dfhack_output(L, &out);

    lua_pushcfunction(L, dfhack_onerror);
    lua_insert(L, base);

    bool ok = lua_pcall(L, nargs, nres, base) == LUA_OK;

    if (!ok && perr)
        report_error(L, &out, true);

    lua_remove(L, base);
    set_dfhack_output(L, cur_out);

    return ok;
}

// Copied from lcorolib.c, with error handling modifications
static int resume_helper(lua_State *L, lua_State *co, int narg, int nres)
{
    if (!co) {
        lua_pop(L, narg);
        push_simple_error(L, "coroutine expected in resume");
        return LUA_ERRRUN;
    }
    if (!lua_checkstack(co, narg)) {
        lua_pop(L, narg);
        push_simple_error(L, "too many arguments to resume");
        return LUA_ERRRUN;
    }
    if (lua_status(co) == LUA_OK && lua_gettop(co) == 0) {
        lua_pop(L, narg);
        push_simple_error(L, "cannot resume dead coroutine");
        return LUA_ERRRUN;
    }
    lua_xmove(L, co, narg);
    int status = lua_resume(co, L, narg);
    if (Lua::IsSuccess(status))
    {
        int nact = lua_gettop(co);
        if (nres == LUA_MULTRET)
            nres = nact;
        else if (nres < nact)
            lua_settop(co, nact = nres);
        if (!lua_checkstack(L, nres + 1)) {
            lua_settop(co, 0);
            push_simple_error(L, "too many results to resume");
            return LUA_ERRRUN;
        }
        int ttop = lua_gettop(L) + nres;
        lua_xmove(co, L, nact);
        lua_settop(L, ttop);
    }
    else
    {
        lua_xmove(co, L, 1);

        // A cross-thread version of dfhack_onerror
        if (lua_checkstack(L, LUA_MINSTACK))
            convert_to_exception(L, 0, co);
    }
    return status;
}

static int dfhack_coresume (lua_State *L) {
    lua_State *co = lua_tothread(L, 1);
    luaL_argcheck(L, !!co, 1, "coroutine expected");
    int r = resume_helper(L, co, lua_gettop(L) - 1, LUA_MULTRET);
    bool ok = Lua::IsSuccess(r);
    lua_pushboolean(L, ok);
    lua_insert(L, 2);
    return lua_gettop(L) - 1;
}

static int dfhack_saferesume (lua_State *L) {
    lua_State *co = lua_tothread(L, 1);
    luaL_argcheck(L, !!co, 1, "coroutine expected");
    int r = resume_helper(L, co, lua_gettop(L) - 1, LUA_MULTRET);
    bool ok = Lua::IsSuccess(r);
    lua_pushboolean(L, ok);
    lua_insert(L, 2);
    if (!ok)
        report_error(L);
    return lua_gettop(L) - 1;
}

static int dfhack_coauxwrap (lua_State *L) {
    lua_State *co = lua_tothread(L, lua_upvalueindex(1));
    int r = resume_helper(L, co, lua_gettop(L), LUA_MULTRET);
    if (Lua::IsSuccess(r))
        return lua_gettop(L);
    else
    {
        if (lua_checkstack(L, LUA_MINSTACK))
            convert_to_exception(L, 1);

        return lua_error(L);
    }
}

static int dfhack_cowrap (lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_settop(L, 1);
    Lua::NewCoroutine(L);
    lua_pushcclosure(L, dfhack_coauxwrap, 1);
    return 1;
}

lua_State *DFHack::Lua::NewCoroutine(lua_State *L) {
    lua_State *NL = lua_newthread(L);
    lua_swap(L);
    lua_xmove(L, NL, 1); /* move function from L to NL */
    return NL;
}

int DFHack::Lua::SafeResume(color_ostream &out, lua_State *from, lua_State *thread, int nargs, int nres, bool perr)
{
    AssertCoreSuspend(from);

    color_ostream *cur_out = Lua::GetOutput(from);
    set_dfhack_output(from, &out);

    int rv = resume_helper(from, thread, nargs, nres);

    if (!Lua::IsSuccess(rv) && perr)
        report_error(from, &out, true);

    set_dfhack_output(from, cur_out);

    return rv;
}

int DFHack::Lua::SafeResume(color_ostream &out, lua_State *from, int nargs, int nres, bool perr)
{
    int base = lua_gettop(from) - nargs;
    lua_State *thread = lua_tothread(from, base);

    int rv = SafeResume(out, from, thread, nargs, nres, perr);

    lua_remove(from, base);
    return rv;
}

/*
 * Module loading
 */

static int DFHACK_LOADED_TOKEN = 0;
static int DFHACK_DFHACK_TOKEN = 0;
static int DFHACK_BASE_G_TOKEN = 0;
static int DFHACK_REQUIRE_TOKEN = 0;

void Lua::PushDFHack(lua_State *state)
{
    lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_DFHACK_TOKEN);
}

void Lua::PushBaseGlobals(lua_State *state)
{
    lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_BASE_G_TOKEN);
}

bool DFHack::Lua::PushModule(color_ostream &out, lua_State *state, const char *module)
{
    AssertCoreSuspend(state);

    // Check if it is already loaded
    lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_LOADED_TOKEN);
    lua_pushstring(state, module);
    lua_rawget(state, -2);

    if (lua_toboolean(state, -1))
    {
        lua_remove(state, -2);
        return true;
    }

    lua_pop(state, 2);
    lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_REQUIRE_TOKEN);
    lua_pushstring(state, module);

    return Lua::SafeCall(out, state, 1, 1);
}

bool DFHack::Lua::PushModulePublic(color_ostream &out, lua_State *state,
                                   const char *module, const char *name)
{
    if (!PushModule(out, state, module))
        return false;

    if (!lua_istable(state, -1))
    {
        lua_pop(state, 1);
        return false;
    }

    lua_pushstring(state, name);
    lua_rawget(state, -2);
    lua_remove(state, -2);
    return true;
}

bool DFHack::Lua::Require(color_ostream &out, lua_State *state,
                          const std::string &module, bool setglobal)
{
    if (!PushModule(out, state, module.c_str()))
        return false;

    if (setglobal)
    {
        lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_BASE_G_TOKEN);
        lua_swap(state);
        lua_setfield(state, -2, module.c_str());
    }
    else
        lua_pop(state, 1);

    return true;
}

static bool doAssignDFObject(color_ostream *out, lua_State *state,
                             type_identity *type, void *target, int val_index,
                             bool exact, bool perr, bool signal)
{
    if (signal)
        check_valid_ptr_index(state, val_index);

    if (lua_istable(state, val_index))
    {
        val_index = lua_absindex(state, val_index);
        lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_ASSIGN_NAME);
        Lua::PushDFObject(state, type, target);
        lua_pushvalue(state, val_index);

        if (signal)
        {
            lua_call(state, 2, 0);
            return true;
        }
        else
            return Lua::SafeCall(*out, state, 2, 0, perr);
    }
    else if (!lua_isuserdata(state, val_index))
    {
        signal_typeid_error(out, state, type, "pointer to %s expected",
                            val_index, perr, signal);
        return false;
    }
    else
    {
        void *in_ptr = Lua::GetDFObject(state, type, val_index, exact);
        if (!in_ptr)
        {
            signal_typeid_error(out, state, type, "incompatible pointer type: %s expected",
                                val_index, perr, signal);
            return false;
        }
        if (!type->copy(target, in_ptr))
        {
            signal_typeid_error(out, state, type, "no copy support for %s",
                                val_index, perr, signal);
            return false;
        }
        return true;
    }
}

bool DFHack::Lua::AssignDFObject(color_ostream &out, lua_State *state,
                                 type_identity *type, void *target, int val_index,
                                 bool exact_type, bool perr)
{
    return doAssignDFObject(&out, state, type, target, val_index, exact_type, perr, false);
}

void DFHack::Lua::CheckDFAssign(lua_State *state, type_identity *type,
                                void *target, int val_index, bool exact_type)
{
    doAssignDFObject(NULL, state, type, target, val_index, exact_type, false, true);
}

bool DFHack::Lua::SafeCallString(color_ostream &out, lua_State *state, const std::string &code,
                                 int nargs, int nres, bool perr,
                                 const char *debug_tag, int env_idx)
{
    AssertCoreSuspend(state);

    if (!debug_tag)
        debug_tag = code.c_str();
    if (env_idx)
        env_idx = lua_absindex(state, env_idx);

    int base = lua_gettop(state);

    // Parse the code
    if (luaL_loadbuffer(state, code.data(), code.size(), debug_tag) != LUA_OK)
    {
        if (perr)
            report_error(state, &out, true);

        return false;
    }

    // Replace _ENV
    if (env_idx)
    {
        lua_pushvalue(state, env_idx);
        lua_setupvalue(state, -2, 1);
        assert(lua_gettop(state) == base+1);
    }

    if (nargs > 0)
        lua_insert(state, -1-nargs);

    return Lua::SafeCall(out, state, nargs, nres, perr);
}

/*
 * Coroutine interactive query loop
 */

static int resume_query_loop(color_ostream &out,
                             lua_State *thread, lua_State *state, int nargs,
                             std::string &prompt, std::string &histfile)
{
    int rv = Lua::SafeResume(out, state, thread, nargs, 2);

    if (Lua::IsSuccess(rv))
    {
        prompt = ifnull(lua_tostring(state, -2), "");
        histfile = ifnull(lua_tostring(state, -1), "");
        lua_pop(state, 2);
    }

    return rv;
}

bool DFHack::Lua::RunCoreQueryLoop(color_ostream &out, lua_State *state,
                                   bool (*init)(color_ostream&, lua_State*, void*),
                                   void *arg)
{
    if (!lua_checkstack(state, 20))
        return false;

    lua_State *thread;
    int rv;
    std::string prompt;
    std::string histfile;

    DFHack::CommandHistory hist;
    std::string histname;

    {
        CoreSuspender suspend;

        int base = lua_gettop(state);

        if (!init(out, state, arg))
        {
            lua_settop(state, base);
            return false;
        }

        // If not interactive, run without coroutine and bail out
        if (!out.is_console())
            return SafeCall(out, state, lua_gettop(state)-base-1, 0);

        lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_QUERY_COROTABLE_TOKEN);
        lua_pushvalue(state, base+1);
        lua_remove(state, base+1);
        thread = Lua::NewCoroutine(state);
        lua_rawsetp(state, -2, thread);
        lua_pop(state, 1);

        rv = resume_query_loop(out, thread, state, lua_gettop(state)-base, prompt, histfile);
    }

    Console &con = static_cast<Console&>(out);

    while (rv == LUA_YIELD)
    {
        if (histfile != histname)
        {
            if (!histname.empty())
                hist.save(histname.c_str());

            hist.clear();
            histname = histfile;

            if (!histname.empty())
                hist.load(histname.c_str());
        }

        if (prompt.empty())
            prompt = ">> ";

        std::string curline;
        con.lineedit(prompt,curline,hist);
        hist.add(curline);

        {
            CoreSuspender suspend;

            lua_pushlstring(state, curline.data(), curline.size());
            rv = resume_query_loop(out, thread, state, 1, prompt, histfile);
        }
    }

    if (!histname.empty())
        hist.save(histname.c_str());

    {
        CoreSuspender suspend;

        lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_QUERY_COROTABLE_TOKEN);
        lua_pushnil(state);
        lua_rawsetp(state, -2, thread);
        lua_pop(state, 1);
    }

    return (rv == LUA_OK);
}

namespace {
    struct InterpreterArgs {
        const char *prompt;
        const char *hfile;
    };
}

static bool init_interpreter(color_ostream &out, lua_State *state, void *info)
{
    auto args = (InterpreterArgs*)info;
    lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_DFHACK_TOKEN);
    lua_getfield(state, -1, "interpreter");
    lua_remove(state, -2);
    lua_pushstring(state, args->prompt);
    lua_pushstring(state, args->hfile);
    return true;
}

bool DFHack::Lua::InterpreterLoop(color_ostream &out, lua_State *state,
                                  const char *prompt, const char *hfile)
{
    if (!out.is_console())
        return false;

    if (!hfile)
        hfile = "lua.history";
    if (!prompt)
        prompt = "lua";

    InterpreterArgs args;
    args.prompt = prompt;
    args.hfile = hfile;

    return RunCoreQueryLoop(out, state, init_interpreter, &args);
}

static bool do_invoke_cleanup(lua_State *L, int nargs, int errorfun, bool success)
{
    bool ok = lua_pcall(L, nargs, 0, errorfun) == LUA_OK;

    if (!ok)
    {
        // If finalization failed, attach the previous error
        if (lua_istable(L, -1) && !success)
        {
            lua_swap(L);
            lua_setfield(L, -2, "cause");
        }

        success = false;
    }

    return success;
}

int dfhack_cleanup_cont(lua_State *L, int status, int)
{
    bool success = Lua::IsSuccess(status);

    int nargs = lua_tointeger(L, 1);
    bool always = lua_toboolean(L, 2);
    int rvbase = 4+nargs;

    // stack: [nargs] [always] [errorfun] [cleanup fun] [cleanup args...] |rvbase+1:| [rvals/error...]

    int numret = lua_gettop(L) - rvbase;

    if (!success || always)
    {
        if (numret > 0)
        {
            if (numret == 1)
            {
                // Inject the only result instead of pulling cleanup args
                lua_insert(L, 4);
            }
            else if (!lua_checkstack(L, nargs+1))
            {
                success = false;
                lua_settop(L, rvbase);
                push_simple_error(L, "stack overflow");
                lua_insert(L, 4);
            }
            else
            {
                for (int i = 0; i <= nargs; i++)
                    lua_pushvalue(L, 4+i);
            }
        }

        success = do_invoke_cleanup(L, nargs, 3, success);
    }

    if (!success)
        lua_error(L);

    return numret;
}

static int dfhack_call_with_finalizer (lua_State *L)
{
    int nargs = luaL_checkint(L, 1);
    if (nargs < 0)
        luaL_argerror(L, 1, "invalid cleanup argument count");
    luaL_checktype(L, 3, LUA_TFUNCTION);

    // Inject errorfun
    lua_pushcfunction(L, dfhack_onerror);
    lua_insert(L, 3);

    int rvbase = 4+nargs; // rvbase+1 points to the function argument

    if (lua_gettop(L) < rvbase)
        luaL_error(L, "not enough arguments even to invoke cleanup");

    // stack: [nargs] [always] [errorfun] [cleanup fun] [cleanup args...] |rvbase+1:| [fun] [args...]

    // Not enough stack to call and post-cleanup, or nothing to call?
    bool no_args = lua_gettop(L) == rvbase;

    if (!lua_checkstack(L, nargs+2) || no_args)
    {
        push_simple_error(L, no_args ? "fn argument expected" : "stack overflow");
        lua_insert(L, 4);

        // stack: ... [errorfun] [error] [cleanup fun] [cleanup args...]
        do_invoke_cleanup(L, nargs, 3, false);
        lua_error(L);
    }

    // Actually invoke

    // stack: [nargs] [always] [errorfun] [cleanup fun] [cleanup args...] |rvbase+1:| [fun] [args...]
    return Lua::TailPCallK<dfhack_cleanup_cont>(L, lua_gettop(L)-rvbase-1, LUA_MULTRET, 3, 0);
}

static int lua_dfhack_with_suspend(lua_State *L)
{
    int nargs = lua_gettop(L);
    luaL_checktype(L, 1, LUA_TFUNCTION);

    CoreSuspender suspend;
    lua_call(L, nargs-1, LUA_MULTRET);

    return lua_gettop(L);
}

static int dfhack_open_plugin(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TSTRING);
    const char *name = lua_tostring(L, 2);

    PluginManager *pmgr = Core::getInstance().getPluginManager();
    Plugin *plugin = pmgr->getPluginByName(name);

    if (!plugin)
        luaL_error(L, "plugin not found: '%s'", name);

    plugin->open_lua(L, 1);
    return 0;
}

static int dfhack_curry_wrap(lua_State *L)
{
    int nargs = lua_gettop(L);
    int ncurry = lua_tointeger(L, lua_upvalueindex(1));
    int scount = nargs + ncurry;

    luaL_checkstack(L, ncurry, "stack overflow in curry");

    // Insert values in O(N+M) by first shifting the existing data
    lua_settop(L, scount);
    for (int i = 0; i < nargs; i++)
        lua_copy(L, nargs-i, scount-i);
    for (int i = 1; i <= ncurry; i++)
        lua_copy(L, lua_upvalueindex(i+1), i);

    lua_callk(L, scount-1, LUA_MULTRET, 0, lua_gettop);

    return lua_gettop(L);
}

static int dfhack_curry(lua_State *L)
{
    luaL_checkany(L, 1);
    if (lua_isnil(L, 1))
        luaL_argerror(L, 1, "nil function in curry");
    if (lua_gettop(L) == 1)
        return 1;
    lua_pushinteger(L, lua_gettop(L));
    lua_insert(L, 1);
    lua_pushcclosure(L, dfhack_curry_wrap, lua_gettop(L));
    return 1;
}

bool Lua::IsCoreContext(lua_State *state)
{
    // This uses a private field of the lua state to
    // evaluate the condition without accessing the lua
    // stack, and thus requiring a lock on the core state.
    return state && Lua::Core::State &&
           state->l_G == Lua::Core::State->l_G;
}

static const luaL_Reg dfhack_funcs[] = {
    { "print", lua_dfhack_print },
    { "println", lua_dfhack_println },
    { "printerr", lua_dfhack_printerr },
    { "color", lua_dfhack_color },
    { "is_interactive", lua_dfhack_is_interactive },
    { "lineedit", dfhack_lineedit },
    { "safecall", dfhack_safecall },
    { "saferesume", dfhack_saferesume },
    { "onerror", dfhack_onerror },
    { "error", dfhack_error },
    { "call_with_finalizer", dfhack_call_with_finalizer },
    { "with_suspend", lua_dfhack_with_suspend },
    { "open_plugin", dfhack_open_plugin },
    { "curry", dfhack_curry },
    { NULL, NULL }
};

static const luaL_Reg dfhack_coro_funcs[] = {
    { "resume", dfhack_coresume },
    { "wrap", dfhack_cowrap },
    { NULL, NULL }
};

/************
 *  Events  *
 ************/

static int DFHACK_EVENT_META_TOKEN = 0;

namespace {
    struct EventObject {
        int item_count;
        Lua::Event::Owner *owner;
    };
}

void DFHack::Lua::Event::New(lua_State *state, Owner *owner)
{
    auto obj = (EventObject *)lua_newuserdata(state, sizeof(EventObject));
    obj->item_count = 0;
    obj->owner = owner;

    lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_EVENT_META_TOKEN);
    lua_setmetatable(state, -2);
    lua_newtable(state);
    lua_setuservalue(state, -2);
}

void DFHack::Lua::Event::SetPrivateCallback(lua_State *L, int event)
{
    lua_getuservalue(L, event);
    lua_swap(L);
    lua_rawsetp(L, -2, NULL);
    lua_pop(L, 1);
}

static int dfhack_event_new(lua_State *L)
{
    Lua::Event::New(L);
    return 1;
}

static int dfhack_event_len(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TUSERDATA);
    auto obj = (EventObject *)lua_touserdata(L, 1);
    lua_pushinteger(L, obj->item_count);
    return 1;
}

static int dfhack_event_tostring(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TUSERDATA);
    auto obj = (EventObject *)lua_touserdata(L, 1);
    lua_pushfstring(L, "<event: %d listeners>", obj->item_count);
    return 1;
}

static int dfhack_event_index(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TUSERDATA);
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    return 1;
}

static int dfhack_event_next(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TUSERDATA);
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    while (lua_next(L, -2))
    {
        if (is_null_userdata(L, -2))
            lua_pop(L, 1);
        else
            return 2;
    }
    lua_pushnil(L);
    return 1;
}

static int dfhack_event_pairs(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TUSERDATA);
    lua_pushcfunction(L, dfhack_event_next);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    return 3;
}

static int dfhack_event_newindex(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TUSERDATA);
    if (is_null_userdata(L, 2))
        luaL_argerror(L, 2, "Key NULL is reserved in events.");

    lua_settop(L, 3);
    lua_getuservalue(L, 1);
    bool new_nil = lua_isnil(L, 3);

    lua_pushvalue(L, 2);
    lua_rawget(L, 4);
    bool old_nil = lua_isnil(L, -1);
    lua_settop(L, 4);

    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, 4);

    int delta = 0;
    if (old_nil && !new_nil) delta = 1;
    else if (new_nil && !old_nil) delta = -1;

    if (delta != 0)
    {
        auto obj = (EventObject *)lua_touserdata(L, 1);
        obj->item_count += delta;
        if (obj->owner)
            obj->owner->on_count_changed(obj->item_count, delta);
    }

    return 0;
}

static void do_invoke_event(lua_State *L, int argbase, int num_args, int errorfun)
{
    for (int i = 0; i < num_args; i++)
        lua_pushvalue(L, argbase+i);

    if (lua_pcall(L, num_args, 0, errorfun) != LUA_OK)
        report_error(L, NULL, true);
}

static void dfhack_event_invoke(lua_State *L, int base, bool from_c)
{
    int event = base+1;
    int num_args = lua_gettop(L)-event;

    int errorfun = base+2;
    lua_pushcfunction(L, dfhack_onerror);
    lua_insert(L, errorfun);

    int argbase = base+3;

    // stack: |base| event errorfun (args)

    if (!from_c)
    {
        // Invoke the NULL key first
        lua_rawgetp(L, event, NULL);

        if (lua_isnil(L, -1))
            lua_pop(L, 1);
        else
            do_invoke_event(L, argbase, num_args, errorfun);
    }

    lua_pushnil(L);

    // stack: |base| event errorfun (args) key || cb (args)

    while (lua_next(L, event))
    {
        // Skip the NULL key in the main loop
        if (is_null_userdata(L, -2))
            lua_pop(L, 1);
        else
            do_invoke_event(L, argbase, num_args, errorfun);
    }

    lua_settop(L, base);
}

static int dfhack_event_call(lua_State *state)
{
    luaL_checktype(state, 1, LUA_TUSERDATA);
    luaL_checkstack(state, lua_gettop(state)+2, "stack overflow in event dispatch");

    auto obj = (EventObject *)lua_touserdata(state, 1);
    if (obj->owner)
        obj->owner->on_invoked(state, lua_gettop(state)-1, false);

    lua_getuservalue(state, 1);
    lua_replace(state, 1);
    dfhack_event_invoke(state, 0, false);
    return 0;
}

void DFHack::Lua::Event::Invoke(color_ostream &out, lua_State *state, void *key, int num_args)
{
    AssertCoreSuspend(state);

    int base = lua_gettop(state) - num_args;

    if (!lua_checkstack(state, num_args+4))
    {
        out.printerr("Stack overflow in Lua::InvokeEvent");
        lua_settop(state, base);
        return;
    }

    lua_rawgetp(state, LUA_REGISTRYINDEX, key);

    if (!lua_isuserdata(state, -1))
    {
        if (!lua_isnil(state, -1))
            out.printerr("Invalid event object in Lua::InvokeEvent");
        lua_settop(state, base);
        return;
    }

    auto obj = (EventObject *)lua_touserdata(state, -1);
    lua_insert(state, base+1);

    if (obj->owner)
        obj->owner->on_invoked(state, num_args, true);

    lua_getuservalue(state, base+1);
    lua_replace(state, base+1);

    color_ostream *cur_out = Lua::GetOutput(state);
    set_dfhack_output(state, &out);
    dfhack_event_invoke(state, base, true);
    set_dfhack_output(state, cur_out);
}

void DFHack::Lua::Event::Make(lua_State *state, void *key, Owner *owner)
{
    lua_rawgetp(state, LUA_REGISTRYINDEX, key);

    if (lua_isnil(state, -1))
    {
        lua_pop(state, 1);
        New(state, owner);
    }

    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, key);
}

void DFHack::Lua::Notification::invoke(color_ostream &out, int nargs)
{
    assert(state);
    Event::Invoke(out, state, key, nargs);
}

void DFHack::Lua::Notification::bind(lua_State *state, void *key)
{
    this->state = state;
    this->key = key;
}

void DFHack::Lua::Notification::bind(lua_State *state, const char *name)
{
    Event::Make(state, this);

    if (handler)
    {
        PushFunctionWrapper(state, 0, name, handler);
        Event::SetPrivateCallback(state, -2);
    }

    this->state = state;
    this->key = this;
}

/************************
 *  Main Open function  *
 ************************/

void OpenDFHackApi(lua_State *state);

namespace DFHack { namespace Lua { namespace Core {
    static void InitCoreContext();
}}}

lua_State *DFHack::Lua::Open(color_ostream &out, lua_State *state)
{
    if (!state)
        state = luaL_newstate();

    interrupt_init(state);

    luaL_openlibs(state);
    AttachDFGlobals(state);

    // Table of query coroutines
    lua_newtable(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_QUERY_COROTABLE_TOKEN);

    // Replace the print function of the standard library
    lua_pushcfunction(state, lua_dfhack_println);
    lua_setglobal(state, "print");

    lua_getglobal(state, "require");
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_REQUIRE_TOKEN);
    lua_getglobal(state, "tostring");
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_TOSTRING_TOKEN);

    // Create the dfhack global
    lua_newtable(state);

    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_DFHACK_TOKEN);

    lua_rawgeti(state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_BASE_G_TOKEN);
    lua_setfield(state, -2, "BASE_G");

    lua_pushstring(state, Version::dfhack_version());
    lua_setfield(state, -2, "VERSION");
    lua_pushstring(state, Version::df_version());
    lua_setfield(state, -2, "DF_VERSION");
    lua_pushstring(state, Version::dfhack_release());
    lua_setfield(state, -2, "RELEASE");

    lua_pushboolean(state, IsCoreContext(state));
    lua_setfield(state, -2, "is_core_context");

    // Create the metatable for exceptions
    lua_newtable(state);
    lua_pushcfunction(state, dfhack_exception_tostring);
    lua_setfield(state, -2, "__tostring");
    lua_pushcfunction(state, dfhack_exception_tostring);
    lua_setfield(state, -2, "tostring");
    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_EXCEPTION_META_TOKEN);
    lua_setfield(state, -2, "exception");

    lua_newtable(state);
    lua_pushcfunction(state, dfhack_event_call);
    lua_setfield(state, -2, "__call");
    lua_pushcfunction(state, dfhack_event_len);
    lua_setfield(state, -2, "__len");
    lua_pushcfunction(state, dfhack_event_tostring);
    lua_setfield(state, -2, "__tostring");
    lua_pushcfunction(state, dfhack_event_index);
    lua_setfield(state, -2, "__index");
    lua_pushcfunction(state, dfhack_event_newindex);
    lua_setfield(state, -2, "__newindex");
    lua_pushcfunction(state, dfhack_event_pairs);
    lua_setfield(state, -2, "__pairs");
    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_EVENT_META_TOKEN);

    lua_newtable(state);
    lua_pushcfunction(state, dfhack_event_new);
    lua_setfield(state, -2, "new");
    lua_dup(state);
    lua_setfield(state, -3, "__metatable");
    lua_setfield(state, -3, "event");
    lua_pop(state, 1);

    // Initialize the dfhack global
    luaL_setfuncs(state, dfhack_funcs, 0);

    OpenDFHackApi(state);

    lua_setglobal(state, "dfhack");

    // stash the loaded module table into our own registry key
    lua_getglobal(state, "package");
    assert(lua_istable(state, -1));
    lua_getfield(state, -1, "loaded");
    assert(lua_istable(state, -1));
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_LOADED_TOKEN);
    lua_pop(state, 1);

    // replace some coroutine functions
    lua_getglobal(state, "coroutine");
    luaL_setfuncs(state, dfhack_coro_funcs, 0);
    lua_pop(state, 1);

    // split the global environment
    lua_newtable(state);
    lua_newtable(state);
    lua_rawgeti(state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_setfield(state, -2, "__index");
    lua_setmetatable(state, -2);
    lua_dup(state);
    lua_setglobal(state, "_G");
    lua_dup(state);
    lua_rawseti(state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);

    // Init core-context specific stuff before loading dfhack.lua
    if (IsCoreContext(state))
        Lua::Core::InitCoreContext();

    // load dfhack.lua
    if (!Require(out, state, "dfhack"))
    {
        out.printerr("Could not load dfhack.lua\n");
        return NULL;
    }

    lua_settop(state, 0);
    if (!lua_checkstack(state, 64))
        out.printerr("Could not extend initial lua stack size to 64 items.\n");

    return state;
}

static int next_timeout_id = 0;
static int frame_idx = 0;
static std::multimap<int,int> frame_timers;
static std::multimap<int,int> tick_timers;

int DFHACK_TIMEOUTS_TOKEN = 0;

static const char *const timeout_modes[] = {
    "frames", "ticks", "days", "months", "years", NULL
};

int dfhack_timeout(lua_State *L)
{
    using df::global::world;
    using df::global::enabler;

    // Parse arguments
    lua_Number time = luaL_checknumber(L, 1);
    int mode = luaL_checkoption(L, 2, NULL, timeout_modes);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_settop(L, 3);

    if (mode > 0 && !Core::getInstance().isWorldLoaded())
    {
        lua_pushnil(L);
        return 1;
    }

    // Compute timeout value
    switch (mode)
    {
    case 2:
        time *= 1200;
        break;
    case 3:
        time *= 33600;
        break;
    case 4:
        time *= 403200;
        break;
    default:;
    }

    int delta = time;

    if (delta <= 0)
        luaL_error(L, "Invalid timeout: %d", delta);

    // Queue the timeout
    int id = next_timeout_id++;
    if (mode)
        tick_timers.insert(std::pair<int,int>(world->frame_counter+delta, id));
    else
        frame_timers.insert(std::pair<int,int>(frame_idx+delta, id));

    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_TIMEOUTS_TOKEN);
    lua_swap(L);
    lua_rawseti(L, -2, id);

    lua_pushinteger(L, id);
    return 1;
}

int dfhack_timeout_active(lua_State *L)
{
    int id = luaL_optint(L, 1, -1);
    bool set_cb = (lua_gettop(L) >= 2);
    lua_settop(L, 2);
    if (!lua_isnil(L, 2))
        luaL_checktype(L, 2, LUA_TFUNCTION);

    if (id < 0)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_TIMEOUTS_TOKEN);
    lua_rawgeti(L, 3, id);
    if (set_cb && !lua_isnil(L, -1))
    {
        lua_pushvalue(L, 2);
        lua_rawseti(L, 3, id);
    }
    return 1;
}

static void cancel_timers(std::multimap<int,int> &timers)
{
    using Lua::Core::State;

    Lua::StackUnwinder frame(State);
    lua_rawgetp(State, LUA_REGISTRYINDEX, &DFHACK_TIMEOUTS_TOKEN);

    for (auto it = timers.begin(); it != timers.end(); ++it)
    {
        lua_pushnil(State);
        lua_rawseti(State, frame[1], it->second);
    }

    timers.clear();
}

void DFHack::Lua::Core::onStateChange(color_ostream &out, int code) {
    if (!State) return;

    switch (code)
    {
    case SC_MAP_UNLOADED:
    case SC_WORLD_UNLOADED:
        cancel_timers(tick_timers);
        break;

    default:;
    }

    Lua::Push(State, code);
    Lua::Event::Invoke(out, State, (void*)onStateChange, 1);
}

static void run_timers(color_ostream &out, lua_State *L,
                       std::multimap<int,int> &timers, int table, int bound)
{
    while (!timers.empty() && timers.begin()->first <= bound)
    {
        int id = timers.begin()->second;
        timers.erase(timers.begin());

        lua_rawgeti(L, table, id);

        if (lua_isnil(L, -1))
            lua_pop(L, 1);
        else
        {
            lua_pushnil(L);
            lua_rawseti(L, table, id);

            Lua::SafeCall(out, L, 0, 0);
        }
    }
}

void DFHack::Lua::Core::onUpdate(color_ostream &out)
{
    using df::global::world;

    if (frame_timers.empty() && tick_timers.empty())
        return;

    Lua::StackUnwinder frame(State);
    lua_rawgetp(State, LUA_REGISTRYINDEX, &DFHACK_TIMEOUTS_TOKEN);

    run_timers(out, State, frame_timers, frame[1], ++frame_idx);

    if (world)
        run_timers(out, State, tick_timers, frame[1], world->frame_counter);
}

bool DFHack::Lua::Core::Init(color_ostream &out)
{
    if (State) {
        out.printerr("state already exists\n");
        return false;
    }

    State = luaL_newstate();

    // Calls InitCoreContext after checking IsCoreContext
    return (Lua::Open(out, State) != NULL);
}

static void Lua::Core::InitCoreContext()
{
    lua_newtable(State);
    lua_rawsetp(State, LUA_REGISTRYINDEX, &DFHACK_TIMEOUTS_TOKEN);

    // Register events
    lua_rawgetp(State, LUA_REGISTRYINDEX, &DFHACK_DFHACK_TOKEN);

    Event::Make(State, (void*)onStateChange);
    lua_setfield(State, -2, "onStateChange");

    lua_pushcfunction(State, dfhack_timeout);
    lua_setfield(State, -2, "timeout");
    lua_pushcfunction(State, dfhack_timeout_active);
    lua_setfield(State, -2, "timeout_active");

    lua_pop(State, 1);
}

void DFHack::Lua::Core::Reset(color_ostream &out, const char *where)
{
    int top = lua_gettop(State);

    if (top != 0)
    {
        out.printerr("Common lua context stack top left at %d after %s.\n", top, where);
        lua_settop(State, 0);
    }
}
