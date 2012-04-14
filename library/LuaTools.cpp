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

#include "df/job.h"
#include "df/job_item.h"
#include "df/building.h"
#include "df/unit.h"
#include "df/item.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

using namespace DFHack;
using namespace DFHack::LuaWrapper;

void DFHack::Lua::PushDFObject(lua_State *state, type_identity *type, void *ptr)
{
    push_object_internal(state, type, ptr, false);
}

void *DFHack::Lua::GetDFObject(lua_State *state, type_identity *type, int val_index, bool exact_type)
{
    return get_object_internal(state, type, val_index, exact_type, false);
}

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

static std::string lua_print_fmt(lua_State *L)
{
    /* Copied from lua source to fully replicate builtin print */
    int n = lua_gettop(L);  /* number of arguments */
    lua_getglobal(L, "tostring");

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
        *out << str;
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

static void dfhack_printerr(lua_State *S, const std::string &str)
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

    if (cv < -1 || cv > color_ostream::COLOR_MAX)
        luaL_argerror(S, 1, "invalid color value");

    color_ostream *out = Lua::GetOutput(S);
    if (out)
        out->color(color_ostream::color_value(cv));
    return 0;
}

static int lua_dfhack_is_interactive(lua_State *S)
{
    lua_pushboolean(S, get_console(S) != NULL);
    return 1;
}

static int lua_dfhack_lineedit(lua_State *S)
{
    const char *prompt = luaL_optstring(S, 1, ">> ");
    const char *hfile = luaL_optstring(S, 2, NULL);

    Console *pstream = get_console(S);
    if (!pstream)
        return 2;

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

static int DFHACK_EXCEPTION_META_TOKEN = 0;

static void error_tostring(lua_State *L, bool keep_old = false)
{
    lua_getglobal(L, "tostring");
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

static void report_error(lua_State *L, color_ostream *out = NULL)
{
    error_tostring(L, true);

    const char *msg = lua_tostring(L, -1);
    assert(msg);

    if (out)
        out->printerr("%s\n", msg);
    else
        dfhack_printerr(L, msg);

    lua_pop(L, 1);
}

static bool convert_to_exception(lua_State *L)
{
    int base = lua_gettop(L);

    bool force_unknown = false;

    if (lua_istable(L, base) && lua_getmetatable(L, base))
    {
        lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_EXCEPTION_META_TOKEN);
        bool is_exception = lua_rawequal(L, -1, -2);
        lua_settop(L, base);

        // If it is an exception, return as is
        if (is_exception)
            return false;

        force_unknown = true;
    }

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
    return true;
}

static int dfhack_onerror(lua_State *L)
{
    luaL_checkany(L, 1);
    lua_settop(L, 1);

    bool changed = convert_to_exception(L);
    if (!changed)
        return 1;

    luaL_traceback(L, L, NULL, 1);
    lua_setfield(L, 1, "stacktrace");

    return 1;
}

static int dfhack_exception_tostring(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    int base = lua_gettop(L);

    lua_getfield(L, 1, "message");
    if (!lua_isstring(L, -1))
    {
        lua_pop(L, 1);
        lua_pushstring(L, "(error message is not a string)");
    }

    lua_pushstring(L, "\n");
    lua_getfield(L, 1, "stacktrace");
    if (!lua_isstring(L, -1))
        lua_pop(L, 2);

    lua_pushstring(L, "\ncaused by:\n");
    lua_getfield(L, 1, "cause");
    if (lua_isnil(L, -1))
        lua_pop(L, 2);
    else
        error_tostring(L);

    lua_concat(L, lua_gettop(L) - base);
    return 1;
}

static void push_simple_error(lua_State *L, const char *str)
{
    lua_pushstring(L, str);

    if (lua_checkstack(L, 5))
        convert_to_exception(L);

    if (lua_checkstack(L, LUA_MINSTACK))
    {
        luaL_traceback(L, L, NULL, 1);
        lua_setfield(L, -2, "stacktrace");
    }
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
        return true;
    }
}

static int finish_dfhack_safecall (lua_State *L, bool success)
{
    success = do_finish_pcall(L, success);

    if (!success)
        report_error(L);

    return lua_gettop(L);
}

static int safecall_cont (lua_State *L)
{
    int status = lua_getctx(L, NULL);
    return finish_dfhack_safecall(L, (status == LUA_YIELD));
}

static int lua_dfhack_safecall (lua_State *L)
{
    luaL_checkany(L, 1);
    lua_pushcfunction(L, dfhack_onerror);
    lua_insert(L, 1);
    int status = lua_pcallk(L, lua_gettop(L) - 2, LUA_MULTRET, 1, 0, safecall_cont);
    return finish_dfhack_safecall(L, (status == LUA_OK));
}

bool DFHack::Lua::SafeCall(color_ostream &out, lua_State *L, int nargs, int nres, bool perr)
{
    int base = lua_gettop(L) - nargs;

    color_ostream *cur_out = Lua::GetOutput(L);
    set_dfhack_output(L, &out);

    lua_pushcfunction(L, dfhack_onerror);
    lua_insert(L, base);

    bool ok = lua_pcall(L, nargs, nres, base) == LUA_OK;

    if (!ok && perr)
    {
        report_error(L, &out);
        lua_pop(L, 1);
    }

    lua_remove(L, base);
    set_dfhack_output(L, cur_out);

    return ok;
}

bool DFHack::Lua::Require(color_ostream &out, lua_State *state,
                          const std::string &module, bool setglobal)
{
    lua_getglobal(state, "require");
    lua_pushstring(state, module.c_str());

    if (!Lua::SafeCall(out, state, 1, 1))
        return false;

    if (setglobal)
        lua_setglobal(state, module.c_str());
    else
        lua_pop(state, 1);

    return true;
}

bool DFHack::Lua::AssignDFObject(color_ostream &out, lua_State *state,
                                 type_identity *type, void *target, int val_index, bool perr)
{
    val_index = lua_absindex(state, val_index);
    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_ASSIGN_NAME);
    PushDFObject(state, type, target);
    lua_pushvalue(state, val_index);
    return Lua::SafeCall(out, state, 2, 0, perr);
}

bool DFHack::Lua::SafeCallString(color_ostream &out, lua_State *state, const std::string &code,
                                 int nargs, int nres, bool perr,
                                 const char *debug_tag, int env_idx)
{
    if (!debug_tag)
        debug_tag = code.c_str();
    if (env_idx)
        env_idx = lua_absindex(state, env_idx);

    int base = lua_gettop(state);

    // Parse the code
    if (luaL_loadbuffer(state, code.data(), code.size(), debug_tag) != LUA_OK)
    {
        if (perr)
        {
            report_error(state, &out);
            lua_pop(state, 1);
        }

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

bool DFHack::Lua::InterpreterLoop(color_ostream &out, lua_State *state,
                                  const char *prompt, int env, const char *hfile)
{
    if (!out.is_console())
        return false;
    if (!lua_checkstack(state, 20))
        return false;

    if (!hfile)
        hfile = "lua.history";
    if (!prompt)
        prompt = "lua";

    DFHack::CommandHistory hist;
    hist.load(hfile);

    out.print("Type quit to exit interactive lua interpreter.\n");

    static bool print_banner = true;
    if (print_banner) {
        out.print("Shortcuts:\n"
                  " '= foo' => '_1,_2,... = foo'\n"
                  " '! foo' => 'print(foo)'\n"
                  "Both save the first result as '_'.\n");
        print_banner = false;
    }

    Console &con = static_cast<Console&>(out);

    // Make a proxy global environment.
    lua_newtable(state);
    int base = lua_gettop(state);

    lua_newtable(state);
    if (env)
        lua_pushvalue(state, env);
    else
        lua_rawgeti(state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_setfield(state, -2, "__index");
    lua_setmetatable(state, -2);

    // Main interactive loop
    int vcnt = 1;
    string curline;
    string prompt_str = "[" + string(prompt) + "]# ";

    for (;;) {
        lua_settop(state, base);

        con.lineedit(prompt_str,curline,hist);

        if (curline.empty())
            continue;
        if (curline == "quit")
            break;

        hist.add(curline);

        char pfix = curline[0];

        if (pfix == '=' || pfix == '!')
        {
            curline = "return " + curline.substr(1);

            if (!Lua::SafeCallString(out, state, curline, 0, LUA_MULTRET, true, "=(interactive)", base))
                continue;

            int numret = lua_gettop(state) - base;

            if (numret >= 1)
            {
                lua_pushvalue(state, base+1);
                lua_setfield(state, base, "_");

                if (pfix == '!')
                {
                    lua_pushcfunction(state, lua_dfhack_println);
                    lua_insert(state, base+1);
                    SafeCall(out, state, numret, 0);
                    continue;
                }
            }

            for (int i = 1; i <= numret; i++)
            {
                std::string name = stl_sprintf("_%d", vcnt++);
                lua_pushvalue(state, base + i);
                lua_setfield(state, base, name.c_str());

                out.print("%s = ", name.c_str());

                lua_pushcfunction(state, lua_dfhack_println);
                lua_pushvalue(state, base + i);
                SafeCall(out, state, 1, 0);
            }
        }
        else
        {
            if (!Lua::SafeCallString(out, state, curline, 0, LUA_MULTRET, true, "=(interactive)", base))
                continue;
        }
    }

    lua_settop(state, base-1);

    hist.save(hfile);
    return true;
}

static int lua_dfhack_interpreter(lua_State *state)
{
    Console *pstream = get_console(state);
    if (!pstream)
        return 2;

    int argc = lua_gettop(state);

    const char *prompt = (argc >= 1 ? lua_tostring(state, 1) : NULL);
    int env = (argc >= 2 && !lua_isnil(state,2) ? 2 : 0);
    const char *hfile = (argc >= 3 ? lua_tostring(state, 3) : NULL);

    lua_pushboolean(state, Lua::InterpreterLoop(*pstream, state, prompt, env, hfile));
    return 1;
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

static int finish_dfhack_cleanup (lua_State *L, bool success)
{
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

static int dfhack_cleanup_cont (lua_State *L)
{
    int status = lua_getctx(L, NULL);
    return finish_dfhack_cleanup(L, (status == LUA_YIELD));
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
    int status = lua_pcallk(L, lua_gettop(L)-rvbase-1, LUA_MULTRET, 3, 0, dfhack_cleanup_cont);
    return finish_dfhack_cleanup(L, (status == LUA_OK));
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

static const luaL_Reg dfhack_funcs[] = {
    { "print", lua_dfhack_print },
    { "println", lua_dfhack_println },
    { "printerr", lua_dfhack_printerr },
    { "color", lua_dfhack_color },
    { "is_interactive", lua_dfhack_is_interactive },
    { "lineedit", lua_dfhack_lineedit },
    { "interpreter", lua_dfhack_interpreter },
    { "safecall", lua_dfhack_safecall },
    { "onerror", dfhack_onerror },
    { "call_with_finalizer", dfhack_call_with_finalizer },
    { "with_suspend", lua_dfhack_with_suspend },
    { "open_plugin", dfhack_open_plugin },
    { NULL, NULL }
};

/************************
 *  Main Open function  *
 ************************/

void OpenDFHackApi(lua_State *state);

lua_State *DFHack::Lua::Open(color_ostream &out, lua_State *state)
{
    if (!state)
        state = luaL_newstate();

    luaL_openlibs(state);
    AttachDFGlobals(state);

    // Replace the print function of the standard library
    lua_pushcfunction(state, lua_dfhack_println);
    lua_setglobal(state, "print");

    // Create the dfhack global
    lua_newtable(state);

    // Create the metatable for exceptions
    lua_newtable(state);
    lua_pushcfunction(state, dfhack_exception_tostring);
    lua_setfield(state, -2, "__tostring");
    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_EXCEPTION_META_TOKEN);
    lua_setfield(state, -2, "exception");

    // Initialize the dfhack global
    luaL_setfuncs(state, dfhack_funcs, 0);

    OpenDFHackApi(state);

    lua_setglobal(state, "dfhack");

    // load dfhack.lua
    Require(out, state, "dfhack");

    return state;
}

