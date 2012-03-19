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

#include "MiscUtils.h"

#include <lua.h>
#include <lauxlib.h>

using namespace DFHack;

static luaL_Reg no_functions[] = { { NULL, NULL } };

inline void lua_dup(lua_State *state) { lua_pushvalue(state, -1); }
inline void lua_swap(lua_State *state) { lua_insert(state, -2); }

static int change_error(lua_State *state)
{
    luaL_error(state, "Attempt to change a read-only table.\n");
}

static void freeze_table(lua_State *state, bool leave_metatable = false, const char *name = NULL)
{
    // rv = {}; setmetatable(rv, { __index = in, __newindex = change_error, __metatable = name })
    int base = lua_gettop(state);
    lua_newtable(state);
    lua_swap(state);
    lua_setfield(state, base, "__index");
    lua_getfield(state, LUA_REGISTRYINDEX, "DFHack.ChangeError");
    lua_setfield(state, base, "__newindex");
    lua_newtable(state);
    lua_swap(state);
    lua_dup(state);
    lua_setmetatable(state, base);
    if (name)
    {
        lua_pushstring(state, name);
        lua_setfield(state, -2, "__metatable");
    }
    // result: [frozen table] [metatable]
    if (!leave_metatable)
        lua_pop(state, 1);
}

static void SaveTypeInfo(lua_State *state, void *node)
{
    lua_getfield(state, LUA_REGISTRYINDEX, "DFHack.DFTypes");
    lua_pushlightuserdata(state, node);
    lua_pushvalue(state, -3);
    lua_settable(state, -3);
    lua_pop(state, 1);
}

static bool RegisterTypeInfo(lua_State *state, void *node)
{
    lua_getfield(state, LUA_REGISTRYINDEX, "DFHack.DFTypes");
    int base = lua_gettop(state);

    lua_pushlightuserdata(state, node);
    lua_rawget(state, base);

    bool added = false;

    if (lua_isnil(state, -1))
    {
        lua_pop(state, 1);

        lua_newtable(state);
        lua_pushlightuserdata(state, node);
        lua_pushvalue(state, -2);
        lua_rawset(state, base);

        added = true;
    }

    lua_remove(state, -2);
    return added;
}

static void RenderTypeChildren(lua_State *state, const std::vector<compound_identity*> &children);

static void RenderType(lua_State *state, compound_identity *node)
{
    assert(node->getName());

    lua_newtable(state);
    if (!lua_checkstack(state, 20))
        return;

    int base = lua_gettop(state);

    lua_pushlightuserdata(state, node);
    lua_setfield(state, base, "_identity");

    switch (node->type())
    {
    case IDTYPE_ENUM:
        {
            enum_identity *eid = (enum_identity*)node;
            const char *const *keys = eid->getKeys();

            // For enums, set mapping between keys and values
            for (int64_t i = eid->getFirstItem(), j = 0; i <= eid->getLastItem(); i++, j++)
            {
                if (!keys[j])
                    continue;

                lua_pushinteger(state, i);
                lua_pushstring(state, keys[j]);
                lua_dup(state);
                lua_pushinteger(state, i);

                lua_settable(state, base);
                lua_settable(state, base);
            }

            if (eid->getFirstItem() <= eid->getLastItem())
            {
                lua_pushstring(state, "_first_item");
                lua_pushinteger(state, eid->getFirstItem());
                lua_settable(state, base);

                lua_pushstring(state, "_last_item");
                lua_pushinteger(state, eid->getLastItem());
                lua_settable(state, base);
            }

            SaveTypeInfo(state, node);
        }
        break;

    default:
        break;
    }

    RenderTypeChildren(state, node->getScopeChildren());

    assert(base == lua_gettop(state));

    freeze_table(state, false, node->getName());
}

static void RenderTypeChildren(lua_State *state, const std::vector<compound_identity*> &children)
{
    for (size_t i = 0; i < children.size(); i++)
    {
        RenderType(state, children[i]);
        lua_pushstring(state, children[i]->getName());
        lua_swap(state);
        lua_settable(state, -3);
    }
}

static void DoAttach(lua_State *state)
{
    int base = lua_gettop(state);

    lua_pushcfunction(state, change_error);
    lua_setfield(state, LUA_REGISTRYINDEX, "DFHack.ChangeError");

    luaL_register(state, "df", no_functions);

    {
        // Assign df a metatable with read-only contents
        lua_newtable(state);

        // Render the type structure
        RenderTypeChildren(state, compound_identity::getTopScope());

        freeze_table(state, true, "df");
        lua_remove(state, -2);
        lua_setmetatable(state, -2);
    }

    lua_pop(state, 1);
}

void DFHack::AttachDFGlobals(lua_State *state)
{
    if (luaL_newmetatable(state, "DFHack.DFTypes"))
        DoAttach(state);

    lua_pop(state, 1);
}
