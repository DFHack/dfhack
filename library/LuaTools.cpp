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

#include "modules/World.h"

#include "LuaWrapper.h"
#include "LuaTools.h"

#include "MiscUtils.h"

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

static void error_tostring(lua_State *L)
{
    lua_getglobal(L, "tostring");
    lua_pushvalue(L, -2);
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
    lua_dup(L);
    error_tostring(L);

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
            error_tostring(L);
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

    lua_concat(L, lua_gettop(L) - base);
    return 1;
}

static int finish_dfhack_safecall (lua_State *L, bool success)
{
    if (!lua_checkstack(L, 2))
    {
        lua_settop(L, 0);  /* create space for return values */
        lua_pushboolean(L, 0);
        lua_pushstring(L, "stack overflow in dfhack.safecall()");
        success = false;
    }
    else
    {
        lua_pushboolean(L, success);
        lua_replace(L, 1); /* put first result in first slot */
    }

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

static int lua_dfhack_with_suspend(lua_State *L)
{
    int rv = lua_getctx(L, NULL);

    // Non-resume entry point:
    if (rv == LUA_OK)
    {
        int nargs = lua_gettop(L);

        luaL_checktype(L, 1, LUA_TFUNCTION);

        Core::getInstance().Suspend();

        lua_pushcfunction(L, dfhack_onerror);
        lua_insert(L, 1);

        rv = lua_pcallk(L, nargs-1, LUA_MULTRET, 1, 0, lua_dfhack_with_suspend);
    }

    // Return, resume, or error entry point:
    lua_remove(L, 1);

    Core::getInstance().Resume();

    if (rv != LUA_OK && rv != LUA_YIELD)
        lua_error(L);

    return lua_gettop(L);
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
    { "with_suspend", lua_dfhack_with_suspend },
    { NULL, NULL }
};

/*
 * Per-world persistent configuration storage.
 */

static PersistentDataItem persistent_by_struct(lua_State *state, int idx)
{
    lua_getfield(state, idx, "entry_id");
    int id = lua_tointeger(state, -1);
    lua_pop(state, 1);

    PersistentDataItem ref = Core::getInstance().getWorld()->GetPersistentData(id);

    if (ref.isValid())
    {
        lua_getfield(state, idx, "key");
        const char *str = lua_tostring(state, -1);
        if (!str || str != ref.key())
            luaL_argerror(state, idx, "inconsistent id and key");
        lua_pop(state, 1);
    }

    return ref;
}

static int read_persistent(lua_State *state, PersistentDataItem ref, bool create)
{
    if (!ref.isValid())
    {
        lua_pushnil(state);
        lua_pushstring(state, "entry not found");
        return 2;
    }

    if (create)
        lua_createtable(state, 0, 4);

    lua_pushvalue(state, lua_upvalueindex(1));
    lua_setmetatable(state, -2);

    lua_pushinteger(state, ref.entry_id());
    lua_setfield(state, -2, "entry_id");
    lua_pushstring(state, ref.key().c_str());
    lua_setfield(state, -2, "key");
    lua_pushstring(state, ref.val().c_str());
    lua_setfield(state, -2, "value");

    lua_createtable(state, PersistentDataItem::NumInts, 0);
    for (int i = 0; i < PersistentDataItem::NumInts; i++)
    {
        lua_pushinteger(state, ref.ival(i));
        lua_rawseti(state, -2, i+1);
    }
    lua_setfield(state, -2, "ints");

    return 1;
}

static PersistentDataItem get_persistent(lua_State *state)
{
    luaL_checkany(state, 1);

    if (lua_istable(state, 1))
    {
        if (!lua_getmetatable(state, 1) ||
            !lua_rawequal(state, -1, lua_upvalueindex(1)))
            luaL_argerror(state, 1, "invalid table type");

        lua_settop(state, 1);

        return persistent_by_struct(state, 1);
    }
    else
    {
        const char *str = luaL_checkstring(state, 1);

        return Core::getInstance().getWorld()->GetPersistentData(str);
    }
}

static int dfhack_persistent_get(lua_State *state)
{
    CoreSuspender suspend;

    auto ref = get_persistent(state);

    return read_persistent(state, ref, !lua_istable(state, 1));
}

static int dfhack_persistent_delete(lua_State *state)
{
    CoreSuspender suspend;

    auto ref = get_persistent(state);

    bool ok = Core::getInstance().getWorld()->DeletePersistentData(ref);

    lua_pushboolean(state, ok);
    return 1;
}

static int dfhack_persistent_get_all(lua_State *state)
{
    CoreSuspender suspend;

    const char *str = luaL_checkstring(state, 1);
    bool prefix = (lua_gettop(state)>=2 ? lua_toboolean(state,2) : false);

    std::vector<PersistentDataItem> data;
    Core::getInstance().getWorld()->GetPersistentData(&data, str, prefix);

    if (data.empty())
    {
        lua_pushnil(state);
    }
    else
    {
        lua_createtable(state, data.size(), 0);
        for (size_t i = 0; i < data.size(); ++i)
        {
            read_persistent(state, data[i], true);
            lua_rawseti(state, -2, i+1);
        }
    }

    return 1;
}

static int dfhack_persistent_save(lua_State *state)
{
    CoreSuspender suspend;

    lua_settop(state, 2);
    luaL_checktype(state, 1, LUA_TTABLE);
    bool add = lua_toboolean(state, 2);

    lua_getfield(state, 1, "key");
    const char *str = lua_tostring(state, -1);
    if (!str)
        luaL_argerror(state, 1, "no key field");

    lua_settop(state, 1);

    PersistentDataItem ref;
    bool added = false;

    if (add)
    {
        ref = Core::getInstance().getWorld()->AddPersistentData(str);
        added = true;
    }
    else if (lua_getmetatable(state, 1))
    {
        if (!lua_rawequal(state, -1, lua_upvalueindex(1)))
            return luaL_argerror(state, 1, "invalid table type");
        lua_pop(state, 1);

        ref = persistent_by_struct(state, 1);
    }
    else
    {
        ref = Core::getInstance().getWorld()->GetPersistentData(str);
    }

    if (!ref.isValid())
    {
        ref = Core::getInstance().getWorld()->AddPersistentData(str);
        if (!ref.isValid())
            luaL_error(state, "cannot create persistent entry");
        added = true;
    }

    lua_getfield(state, 1, "value");
    if (const char *str = lua_tostring(state, -1))
        ref.val() = str;
    lua_pop(state, 1);

    lua_getfield(state, 1, "ints");
    if (lua_istable(state, -1))
    {
        for (int i = 0; i < PersistentDataItem::NumInts; i++)
        {
            lua_rawgeti(state, -1, i+1);
            if (lua_isnumber(state, -1))
                ref.ival(i) = lua_tointeger(state, -1);
            lua_pop(state, 1);
        }
    }
    lua_pop(state, 1);

    read_persistent(state, ref, false);
    lua_pushboolean(state, added);
    return 2;
}

static const luaL_Reg dfhack_persistent_funcs[] = {
    { "get", dfhack_persistent_get },
    { "delete", dfhack_persistent_delete },
    { "get_all", dfhack_persistent_get_all },
    { "save", dfhack_persistent_save },
    { NULL, NULL }
};

static void OpenPersistent(lua_State *state)
{
    luaL_getsubtable(state, lua_gettop(state), "persistent");

    lua_dup(state);
    luaL_setfuncs(state, dfhack_persistent_funcs, 1);

    lua_dup(state);
    lua_setfield(state, -2, "__index");

    lua_pop(state, 1);
}

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

    OpenPersistent(state);

    lua_setglobal(state, "dfhack");

    // load dfhack.lua
    Require(out, state, "dfhack");

    return state;
}

