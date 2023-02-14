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

#include <cstring>
#include <string>
#include <vector>
#include <map>

#include "MemAccess.h"
#include "Core.h"
#include "Error.h"
#include "VersionInfo.h"
#include "tinythread.h"
// must be last due to MS stupidity
#include "DataDefs.h"
#include "DataIdentity.h"
#include "DataFuncs.h"
#include "DFHackVersion.h"
#include "PluginManager.h"
#include "tinythread.h"
#include "md5wrapper.h"

#include "modules/Buildings.h"
#include "modules/Burrows.h"
#include "modules/Constructions.h"
#include "modules/Designations.h"
#include "modules/Filesystem.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Kitchen.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Random.h"
#include "modules/Screen.h"
#include "modules/Textures.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "LuaWrapper.h"
#include "LuaTools.h"

#include "MiscUtils.h"

#include "df/activity_entry.h"
#include "df/activity_event.h"
#include "df/enabler.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/building.h"
#include "df/unit.h"
#include "df/item.h"
#include "df/material.h"
#include "df/viewscreen.h"
#include "df/identity.h"
#include "df/nemesis_record.h"
#include "df/historical_figure.h"
#include "df/histfig_entity_link_positionst.h"
#include "df/plant_raw.h"
#include "df/creature_raw.h"
#include "df/inorganic_raw.h"
#include "df/dfhack_material_category.h"
#include "df/job_material_category.h"
#include "df/burrow.h"
#include "df/building_cagest.h"
#include "df/building_civzonest.h"
#include "df/region_map_entry.h"
#include "df/flow_info.h"
#include "df/unit_misc_trait.h"
#include "df/proj_itemst.h"
#include "df/itemdef.h"
#include "df/feature_init.h"
#include "df/plant.h"
#include "df/specific_ref.h"
#include "df/specific_ref_type.h"
#include "df/vermin.h"
#include "df/report_init.h"
#include "df/report_zoom_type.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

using namespace DFHack;
using namespace DFHack::LuaWrapper;

using Screen::Pen;
using Random::MersenneRNG;
using Random::PerlinNoise1D;
using Random::PerlinNoise2D;
using Random::PerlinNoise3D;

void dfhack_printerr(lua_State *S, const std::string &str);

static df::coord2d CheckCoordXY(lua_State *state, int base, bool vararg = false)
{
    df::coord2d p;
    if (vararg && lua_gettop(state) <= base)
        Lua::CheckDFAssign(state, &p, base);
    else
    {
        p = df::coord2d(
            luaL_checkint(state, base),
            luaL_checkint(state, base+1)
        );
    }
    return p;
}

static df::coord CheckCoordXYZ(lua_State *state, int base, bool vararg = false)
{
    df::coord p;
    if (vararg && lua_gettop(state) <= base)
        Lua::CheckDFAssign(state, &p, base);
    else
    {
        p = df::coord(
            luaL_checkint(state, base),
            luaL_checkint(state, base+1),
            luaL_checkint(state, base+2)
        );
    }
    return p;
}

template<class T>
static bool get_int_field(lua_State *L, T *pf, int idx, const char *name, int defval)
{
    lua_getfield(L, idx, name);
    bool nil = lua_isnil(L, -1);
    if (nil) *pf = T(defval);
    else if (lua_isnumber(L, -1)) *pf = T(lua_tointeger(L, -1));
    else luaL_error(L, "Field %s is not a number.", name);
    lua_pop(L, 1);
    return !nil;
}

static bool get_char_field(lua_State *L, char *pf, int idx, const char *name, char defval)
{
    lua_getfield(L, idx, name);

    if (lua_type(L, -1) == LUA_TSTRING)
    {
        *pf = lua_tostring(L, -1)[0];
        lua_pop(L, 1);
        return true;
    }
    else
    {
        lua_pop(L, 1);
        return get_int_field(L, pf, idx, name, defval);
    }
}

static bool get_bool_field(lua_State *L, bool *pf, int idx, const char *name, bool defval) {
    lua_getfield(L, idx, name);
    bool nil = lua_isnil(L, -1);
    if (nil)
        *pf = defval;
    else
        *pf = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return !nil;
}

static void decode_pen(lua_State *L, Pen &pen, int idx)
{
    idx = lua_absindex(L, idx);

    get_char_field(L, &pen.ch, idx, "ch", 0);

    get_int_field(L, &pen.fg, idx, "fg", 7);
    get_int_field(L, &pen.bg, idx, "bg", 0);

    lua_getfield(L, idx, "bold");
    if (lua_isnil(L, -1))
    {
        pen.bold = (pen.fg & 8) != 0;
        pen.fg &= 7;
    }
    else pen.bold = lua_toboolean(L, -1);
    lua_pop(L, 1);

    get_int_field(L, &pen.tile, idx, "tile", 0);

    bool tcolor = get_int_field(L, &pen.tile_fg, idx, "tile_fg", 7);
    tcolor = get_int_field(L, &pen.tile_bg, idx, "tile_bg", 0) || tcolor;

    if (tcolor)
        pen.tile_mode = Pen::TileColor;
    else
    {
        lua_getfield(L, idx, "tile_color");
        pen.tile_mode = (lua_toboolean(L, -1) ? Pen::CharColor : Pen::AsIs);
        lua_pop(L, 1);
    }

    get_bool_field(L, &pen.keep_lower, idx, "keep_lower", false);
    get_bool_field(L, &pen.write_to_lower, idx, "write_to_lower", false);
    get_bool_field(L, &pen.top_of_text, idx, "top_of_text", false);
    get_bool_field(L, &pen.bottom_of_text, idx, "bottom_of_text", false);
}

/**************************************************
 * Per-world persistent configuration storage API *
 **************************************************/

static PersistentDataItem persistent_by_struct(lua_State *state, int idx)
{
    lua_getfield(state, idx, "entry_id");
    int id = lua_tointeger(state, -1);
    lua_pop(state, 1);

    PersistentDataItem ref = World::GetPersistentData(id);

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
        Lua::StackUnwinder frame(state);

        if (!lua_getmetatable(state, 1) ||
            !lua_rawequal(state, -1, lua_upvalueindex(1)))
            luaL_argerror(state, 1, "invalid table type");

        return persistent_by_struct(state, 1);
    }
    else
    {
        const char *str = luaL_checkstring(state, 1);

        return World::GetPersistentData(str);
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

    bool ok = World::DeletePersistentData(ref);

    lua_pushboolean(state, ok);
    return 1;
}

static int dfhack_persistent_get_all(lua_State *state)
{
    CoreSuspender suspend;

    const char *str = luaL_checkstring(state, 1);
    bool prefix = (lua_gettop(state)>=2 ? lua_toboolean(state,2) : false);

    std::vector<PersistentDataItem> data;
    World::GetPersistentData(&data, str, prefix);

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

    // Retrieve existing or create a new entry
    PersistentDataItem ref;
    bool added = false;

    if (add)
    {
        ref = World::AddPersistentData(str);
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
        ref = World::GetPersistentData(str);
    }

    // Auto-add if not found
    if (!ref.isValid())
    {
        ref = World::AddPersistentData(str);
        if (!ref.isValid())
            luaL_error(state, "cannot create persistent entry");
        added = true;
    }

    // Copy data from lua to C++ memory
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

    // Reinitialize lua from C++ and return
    read_persistent(state, ref, false);
    lua_pushboolean(state, added);
    return 2;
}

static int dfhack_persistent_getTilemask(lua_State *state)
{
    CoreSuspender suspend;

    lua_settop(state, 3);
    auto ref = get_persistent(state);
    auto block = Lua::CheckDFObject<df::map_block>(state, 2);
    bool create = lua_toboolean(state, 3);

    Lua::PushDFObject(state, World::getPersistentTilemask(ref, block, create));
    return 1;
}

static int dfhack_persistent_deleteTilemask(lua_State *state)
{
    CoreSuspender suspend;

    lua_settop(state, 2);
    auto ref = get_persistent(state);
    auto block = Lua::CheckDFObject<df::map_block>(state, 2);

    lua_pushboolean(state, World::deletePersistentTilemask(ref, block));
    return 1;
}

static const luaL_Reg dfhack_persistent_funcs[] = {
    { "get", dfhack_persistent_get },
    { "delete", dfhack_persistent_delete },
    { "get_all", dfhack_persistent_get_all },
    { "save", dfhack_persistent_save },
    { "getTilemask", dfhack_persistent_getTilemask },
    { "deleteTilemask", dfhack_persistent_deleteTilemask },
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

/************************
 * Material info lookup *
 ************************/

static int DFHACK_MATINFO_TOKEN = 0;

void Lua::Push(lua_State *state, const MaterialInfo &info)
{
    if (!info.isValid())
    {
        lua_pushnil(state);
        return;
    }

    lua_newtable(state);
    lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_MATINFO_TOKEN);
    lua_setmetatable(state, -2);

    lua_pushinteger(state, info.type);
    lua_setfield(state, -2, "type");
    lua_pushinteger(state, info.index);
    lua_setfield(state, -2, "index");

#define SETOBJ(name) { \
    Lua::PushDFObject(state, info.name); \
    lua_setfield(state, -2, #name); \
}
    SETOBJ(material);
    if (info.plant) SETOBJ(plant);
    if (info.creature) SETOBJ(creature);
    if (info.inorganic) SETOBJ(inorganic);
    if (info.figure) SETOBJ(figure);
#undef SETOBJ

    if (info.mode != MaterialInfo::Builtin)
    {
        lua_pushinteger(state, info.subtype);
        lua_setfield(state, -2, "subtype");
    }

    const char *id = "builtin";
    switch (info.mode)
    {
        case MaterialInfo::Plant: id = "plant"; break;
        case MaterialInfo::Creature: id = "creature"; break;
        case MaterialInfo::Inorganic: id = "inorganic"; break;
        default: break;
    }

    lua_pushstring(state, id);
    lua_setfield(state, -2, "mode");
}

static int dfhack_matinfo_find(lua_State *state)
{
    MaterialInfo info;
    int argc = lua_gettop(state);

    if (argc == 1)
        info.find(luaL_checkstring(state, 1));
    else
    {
        std::vector<std::string> tokens;

        for (int i = 1; i <= argc; i++)
            tokens.push_back(luaL_checkstring(state, i));

        info.find(tokens);
    }

    Lua::Push(state, info);
    return 1;
}

static bool decode_matinfo(lua_State *state, MaterialInfo *info, bool numpair = false)
{
    int curtop = lua_gettop(state);

    luaL_checkany(state, 1);

    if (!lua_isnumber(state, 1))
    {
        if (lua_isnil(state, 1))
            return false;

        if (lua_getmetatable(state, 1))
        {
            if (lua_rawequal(state, -1, lua_upvalueindex(1)))
            {
                lua_getfield(state, 1, "type");
                lua_getfield(state, 1, "index");
                goto int_pair;
            }

            lua_pop(state, 1);
        }

        if (lua_isuserdata(state, 1))
        {
            if (auto item = Lua::GetDFObject<df::item>(state, 1))
                return info->decode(item);
            if (auto mvec = Lua::GetDFObject<df::material_vec_ref>(state, 1))
                return info->decode(*mvec, luaL_checkint(state, 2));
        }

        lua_getfield(state, 1, "mat_type");
        lua_getfield(state, 1, "mat_index");
        goto int_pair;
    }
    else
    {
        if (!numpair)
            luaL_argerror(state, 1, "material info object expected");

        if (curtop < 2)
            lua_settop(state, 2);
    }

int_pair:
    {
        int ok;
        int type = lua_tointegerx(state, -2, &ok);
        if (!ok)
            luaL_argerror(state, 1, "material id is not a number");
        int index = lua_tointegerx(state, -1, &ok);
        if (!ok)
            index = -1;

        lua_settop(state, curtop);

        return info->decode(type, index);
    }
}

static int dfhack_matinfo_decode(lua_State *state)
{
    MaterialInfo info;
    decode_matinfo(state, &info, true);
    Lua::Push(state, info);
    return 1;
}

static int dfhack_matinfo_getToken(lua_State *state)
{
    MaterialInfo info;
    decode_matinfo(state, &info, true);
    auto str = info.getToken();
    lua_pushstring(state, str.c_str());
    return 1;
}

static int dfhack_matinfo_toString(lua_State *state)
{
    MaterialInfo info;
    decode_matinfo(state, &info);

    lua_settop(state, 3);
    auto str = info.toString(luaL_optint(state, 2, 10015), lua_toboolean(state, 3));
    lua_pushstring(state, str.c_str());
    return 1;
}

static int dfhack_matinfo_getCraftClass(lua_State *state)
{
    MaterialInfo info;
    if (decode_matinfo(state, &info, true))
        lua_pushinteger(state, info.getCraftClass());
    else
        lua_pushnil(state);
    return 1;
}

static int dfhack_matinfo_matches(lua_State *state)
{
    MaterialInfo info;
    if (!decode_matinfo(state, &info))
        luaL_argerror(state, 1, "material info object expected");

    luaL_checkany(state, 2);

    if (lua_isuserdata(state, 2))
    {
        if (auto mc = Lua::GetDFObject<df::job_material_category>(state, 2))
            lua_pushboolean(state, info.matches(*mc));
        else if (auto mc = Lua::GetDFObject<df::dfhack_material_category>(state, 2))
            lua_pushboolean(state, info.matches(*mc));
        else if (auto mc = Lua::GetDFObject<df::job_item>(state, 2))
            lua_pushboolean(state, info.matches(*mc));
        else
            luaL_argerror(state, 2, "material category object expected");
    }
    else if (lua_istable(state, 2))
    {
        df::dfhack_material_category tmp;
        Lua::CheckDFAssign(state, &tmp, 2, false);
        lua_pushboolean(state, info.matches(tmp));
    }
    else
        luaL_argerror(state, 2, "material category object expected");

    return 1;
}

static const luaL_Reg dfhack_matinfo_funcs[] = {
    { "find", dfhack_matinfo_find },
    { "decode", dfhack_matinfo_decode },
    { "getToken", dfhack_matinfo_getToken },
    { "toString", dfhack_matinfo_toString },
    { "getCraftClass", dfhack_matinfo_getCraftClass },
    { "matches", dfhack_matinfo_matches },
    { NULL, NULL }
};

static void OpenMatinfo(lua_State *state)
{
    luaL_getsubtable(state, lua_gettop(state), "matinfo");

    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_MATINFO_TOKEN);

    lua_dup(state);
    luaL_setfuncs(state, dfhack_matinfo_funcs, 1);

    lua_dup(state);
    lua_setfield(state, -2, "__index");

    lua_pop(state, 1);
}

/**************
 * Pen object *
 **************/

static int DFHACK_PEN_TOKEN = 0;

void Lua::Push(lua_State *L, const Screen::Pen &info)
{
    if (!info.valid())
    {
        lua_pushnil(L);
        return;
    }

    new (L) Pen(info);

    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_PEN_TOKEN);
    lua_setmetatable(L, -2);
}

static Pen *check_pen_native(lua_State *L, int index)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_PEN_TOKEN);

    if (!lua_getmetatable(L, index) || !lua_rawequal(L, -1, -2))
        luaL_argerror(L, index, "not a pen object");

    lua_pop(L, 2);

    return (Pen*)lua_touserdata(L, index);
}

void Lua::CheckPen(lua_State *L, Screen::Pen *pen, int index, bool allow_nil, bool allow_color)
{
    index = lua_absindex(L, index);

    luaL_checkany(L, index);

    if (lua_isnil(L, index))
    {
        if (!allow_nil)
            luaL_argerror(L, index, "nil pen not allowed");

        *pen = Pen(0,0,0,-1);
    }
    else if (lua_isuserdata(L, index))
    {
        *pen = *check_pen_native(L, index);
    }
    else if (allow_color && lua_isnumber(L, index))
    {
        *pen = Pen(0, lua_tointeger(L, index)&15, 0);
    }
    else
    {
        luaL_checktype(L, index, LUA_TTABLE);
        decode_pen(L, *pen, index);
    }
}

static int adjust_pen(lua_State *L, bool no_copy)
{
    lua_settop(L, 4);

    Pen pen;
    int iidx = 1;
    Lua::CheckPen(L, &pen, 1, true, true);

    if (!lua_isnil(L, 2) || !lua_isnil(L, 3) || !lua_isnil(L, 4))
    {
        if (lua_isnumber(L, 2) || lua_isnil(L, 2))
        {
            if (!pen.valid())
                pen = Pen();

            iidx = -1;

            pen.fg = luaL_optint(L, 2, pen.fg) & 15;
            pen.bg = luaL_optint(L, 3, pen.bg);

            if (!lua_isnil(L, 4))
                pen.bold = lua_toboolean(L, 4);
            else if (!lua_isnil(L, 2))
            {
                pen.bold = !!(pen.fg & 8);
                pen.fg &= 7;
            }
        }
        else
        {
            iidx = 2;
            Lua::CheckPen(L, &pen, 2, false, false);
        }
    }

    if (no_copy && iidx > 0 && lua_isuserdata(L, iidx))
        lua_pushvalue(L, iidx);
    else
        Lua::Push(L, pen);

    return 1;
}

static int dfhack_pen_parse(lua_State *L)
{
    return adjust_pen(L, true);
}

static int dfhack_pen_make(lua_State *L)
{
    return adjust_pen(L, false);
}

static void make_pen_table(lua_State *L, Pen &pen)
{
    if (!pen.valid())
        luaL_error(L, "invalid pen state");
    else
    {
        lua_newtable(L);
        lua_pushinteger(L, (unsigned char)pen.ch); lua_setfield(L, -2, "ch");
        lua_pushinteger(L, pen.fg); lua_setfield(L, -2, "fg");
        lua_pushinteger(L, pen.bg); lua_setfield(L, -2, "bg");
        lua_pushboolean(L, pen.bold); lua_setfield(L, -2, "bold");

        if (pen.tile)
        {
            lua_pushinteger(L, pen.tile); lua_setfield(L, -2, "tile");
        }

        switch (pen.tile_mode) {
            case Pen::CharColor:
                lua_pushboolean(L, true); lua_setfield(L, -2, "tile_color");
                break;
            case Pen::TileColor:
                lua_pushinteger(L, pen.tile_fg); lua_setfield(L, -2, "tile_fg");
                lua_pushinteger(L, pen.tile_bg); lua_setfield(L, -2, "tile_bg");
                break;
            default:
                lua_pushboolean(L, false); lua_setfield(L, -2, "tile_color");
                break;
        }
    }
}

static void get_pen_mirror(lua_State *L, int idx)
{
    lua_getuservalue(L, idx);

    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);

        Pen pen;
        Lua::CheckPen(L, &pen, idx, false, false);
        make_pen_table(L, pen);

        lua_dup(L);
        lua_setuservalue(L, idx);
    }
}

static int dfhack_pen_index(lua_State *L)
{
    lua_settop(L, 2);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    // check metatable
    if (!lua_getmetatable(L, 1))
        luaL_argerror(L, 1, "must be a pen");
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1))
        return 1;

    // otherwise read from the mirror table, creating it if necessary
    lua_settop(L, 2);
    get_pen_mirror(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    return 1;
}

static int pen_pnext(lua_State *L)
{
    lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
    if (lua_next(L, lua_upvalueindex(1)))
        return 2;
    lua_pushnil(L);
    return 1;
}

static int dfhack_pen_pairs(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TUSERDATA);
    get_pen_mirror(L, 1);
    lua_pushcclosure(L, pen_pnext, 1);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3;
}

const char *const pen_fields[] = {
    "ch", "fg", "bold", "bg", "tile", "tile_color", "tile_fg", "tile_bg", NULL
};

static int dfhack_pen_newindex(lua_State *L)
{
    lua_settop(L, 3);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    int id = luaL_checkoption(L, 2, NULL, pen_fields);
    int arg = 0;
    Pen &pen = *check_pen_native(L, 1);
    bool wipe_tile = false, wipe_tc = false;

    switch (id) {
    case 0:
        if (lua_type(L, 3) != LUA_TNUMBER)
            arg = (unsigned char)*luaL_checkstring(L, 3);
        else
            arg = luaL_checkint(L, 3);
        pen.ch = arg;
        lua_pushinteger(L, (unsigned char)pen.ch);
        break;
    case 1:
        pen.fg = luaL_checkint(L, 3) & 15;
        lua_pushinteger(L, pen.fg);
        break;
    case 2:
        pen.bold = lua_toboolean(L, 3);
        lua_pushboolean(L, pen.bold);
        break;
    case 3:
        pen.bg = luaL_checkint(L, 3) & 15;
        lua_pushinteger(L, pen.bg);
        break;
    case 4:
        arg = lua_isnil(L, 3) ? 0 : luaL_checkint(L, 3);
        if (arg < 0)
            luaL_argerror(L, 3, "invalid tile index");
        pen.tile = arg;
        if (pen.tile)
            lua_pushinteger(L, pen.tile);
        else
            lua_pushnil(L);
        break;
    case 5:
        wipe_tile = (pen.tile_mode == Pen::TileColor);
        pen.tile_mode = lua_toboolean(L, 3) ? Pen::CharColor : Pen::AsIs;
        lua_pushboolean(L, pen.tile_mode == Pen::CharColor);
        break;
    case 6:
        if (pen.tile_mode != Pen::TileColor) { wipe_tc = true; pen.tile_bg = 0; }
        pen.tile_fg = luaL_checkint(L, 3) & 15;
        pen.tile_mode = Pen::TileColor;
        lua_pushinteger(L, pen.tile_fg);
        break;
    case 7:
        if (pen.tile_mode != Pen::TileColor) { wipe_tc = true; pen.tile_fg = 7; }
        pen.tile_bg = luaL_checkint(L, 3) & 15;
        pen.tile_mode = Pen::TileColor;
        lua_pushinteger(L, pen.tile_bg);
        break;
    }

    lua_getuservalue(L, 1);

    if (!lua_isnil(L, -1))
    {
        lua_remove(L, 3);
        lua_insert(L, 2);
        lua_rawset(L, 2);

        if (wipe_tc) {
            lua_pushnil(L); lua_setfield(L, 2, "tile_color");
            lua_pushinteger(L, pen.tile_fg); lua_setfield(L, 2, "tile_fg");
            lua_pushinteger(L, pen.tile_bg); lua_setfield(L, 2, "tile_bg");
        }
        if (wipe_tile) {
            lua_pushnil(L); lua_setfield(L, 2, "tile_fg");
            lua_pushnil(L); lua_setfield(L, 2, "tile_bg");
        }
    }

    return 0;
}

static const luaL_Reg dfhack_pen_funcs[] = {
    { "parse", dfhack_pen_parse },
    { "make", dfhack_pen_make },
    { "__index", dfhack_pen_index },
    { "__pairs", dfhack_pen_pairs },
    { "__newindex", dfhack_pen_newindex },
    { NULL, NULL }
};

static void OpenPen(lua_State *state)
{
    luaL_getsubtable(state, lua_gettop(state), "pen");

    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_PEN_TOKEN);

    luaL_setfuncs(state, dfhack_pen_funcs, 0);

    lua_pop(state, 1);
}

/******************
* PenArray object *
******************/

static int DFHACK_PENARRAY_TOKEN = 0;
using Screen::PenArray;

static PenArray *check_penarray_native(lua_State *L, int index)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_PENARRAY_TOKEN);

    if (!lua_getmetatable(L, index) || !lua_rawequal(L, -1, -2))
        luaL_argerror(L, index, "not a penarray object");

    lua_pop(L, 2);

    return (PenArray*)lua_touserdata(L, index);
}

static int dfhack_penarray_new(lua_State *L)
{
    int bufwidth = luaL_checkint(L, 1);
    int bufheight = luaL_checkint(L, 2);
    void *buf = lua_newuserdata(L, sizeof(PenArray) + (sizeof(Pen) * bufwidth * bufheight));
    new (buf) PenArray(bufwidth, bufheight, buf);

    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_PENARRAY_TOKEN);
    lua_setmetatable(L, -2);

    return 1;
}

static int dfhack_penarray_clear(lua_State *L)
{
    PenArray *parr = check_penarray_native(L, 1);
    parr->clear();
    return 0;
}

static int dfhack_penarray_get_dims(lua_State *L)
{
    PenArray *parr = check_penarray_native(L, 1);
    lua_pushinteger(L, parr->get_dimx());
    lua_pushinteger(L, parr->get_dimy());
    return 2;
}

static int dfhack_penarray_get_tile(lua_State *L)
{
    PenArray *parr = check_penarray_native(L, 1);
    unsigned int x = luaL_checkint(L, 2);
    unsigned int y = luaL_checkint(L, 3);
    if (x < parr->get_dimx() && y < parr->get_dimy())
    {
        Pen pen = parr->get_tile(x, y);
        Lua::Push(L, pen);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

static int dfhack_penarray_set_tile(lua_State *L)
{
    PenArray *parr = check_penarray_native(L, 1);
    unsigned int x = luaL_checkint(L, 2);
    unsigned int y = luaL_checkint(L, 3);
    Pen pen;
    Lua::CheckPen(L, &pen, 4);
    parr->set_tile(x, y, pen);
    return 0;
}

static int dfhack_penarray_draw(lua_State *L)
{
    PenArray *parr = check_penarray_native(L, 1);
    unsigned int x = (unsigned int)luaL_checkint(L, 2);
    unsigned int y = (unsigned int)luaL_checkint(L, 3);
    unsigned int w = (unsigned int)luaL_checkint(L, 4);
    unsigned int h = (unsigned int)luaL_checkint(L, 5);
    unsigned int bufx = (unsigned int)luaL_optint(L, 6, 0);
    unsigned int bufy = (unsigned int)luaL_optint(L, 7, 0);
    parr->draw(x, y, w, h, bufx, bufy);
    return 0;
}

static const luaL_Reg dfhack_penarray_funcs[] = {
    { "new", dfhack_penarray_new },
    { "clear", dfhack_penarray_clear },
    { "get_dims", dfhack_penarray_get_dims },
    { "get_tile", dfhack_penarray_get_tile },
    { "set_tile", dfhack_penarray_set_tile },
    { "draw", dfhack_penarray_draw },
    { NULL, NULL }
};

static void OpenPenArray(lua_State *state)
{
    luaL_getsubtable(state, lua_gettop(state), "penarray");

    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_PENARRAY_TOKEN);

    luaL_setfuncs(state, dfhack_penarray_funcs, 0);

    lua_pop(state, 1);
}

/********************
 * Random generator *
 ********************/

static int DFHACK_RANDOM_TOKEN = 0;

static MersenneRNG *check_random_native(lua_State *L, int index)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_RANDOM_TOKEN);

    if (!lua_getmetatable(L, index) || !lua_rawequal(L, -1, -2))
        luaL_argerror(L, index, "not a random generator object");

    lua_pop(L, 2);

    return (MersenneRNG*)lua_touserdata(L, index);
}

static int dfhack_random_init(lua_State *L)
{
    lua_settop(L, 3);

    MersenneRNG *prng = check_random_native(L, 1);

    if (lua_isnil(L, 2))
        prng->init();
    else
    {
        std::vector<uint32_t> data;
        int tcnt = luaL_optint(L, 3, 1);

        if (lua_isnumber(L, 2))
            data.push_back(lua_tounsigned(L, 2));
        else if (lua_istable(L, 2))
        {
            int cnt = lua_rawlen(L, 2);
            if (cnt <= 0)
                luaL_argerror(L, 2, "empty list in dfhack.random.init");

            for (int i = 1; i <= cnt; i++)
            {
                lua_rawgeti(L, 2, i);
                if (!lua_isnumber(L, -1))
                    luaL_argerror(L, 2, "not a number in dfhack.random.init argument");

                data.push_back(lua_tounsigned(L, -1));
                lua_pop(L, 1);
            }
        }
        else
            luaL_argerror(L, 2, "dfhack.random.init argument not number or table");

        prng->init(data.data(), data.size(), tcnt);
    }

    lua_settop(L, 1);
    return 1;
}

static int dfhack_random_new(lua_State *L)
{
    new (L) MersenneRNG();

    lua_rawgetp(L, LUA_REGISTRYINDEX, &DFHACK_RANDOM_TOKEN);
    lua_setmetatable(L, -2);

    lua_insert(L, 1);
    return dfhack_random_init(L);
}

static int dfhack_random_random(lua_State *L)
{
    MersenneRNG *prng = check_random_native(L, 1);

    lua_settop(L, 2);
    if (lua_gettop(L) < 2 || lua_isnil(L, 2))
        lua_pushunsigned(L, prng->random());
    else
        lua_pushunsigned(L, prng->random(luaL_optunsigned(L, 2, 0)));
    return 1;
}

static int dfhack_random_drandom(lua_State *L)
{
    lua_pushnumber(L, check_random_native(L, 1)->drandom());
    return 1;
}
static int dfhack_random_drandom0(lua_State *L)
{
    lua_pushnumber(L, check_random_native(L, 1)->drandom0());
    return 1;
}
static int dfhack_random_drandom1(lua_State *L)
{
    lua_pushnumber(L, check_random_native(L, 1)->drandom1());
    return 1;
}
static int dfhack_random_unitrandom(lua_State *L)
{
    lua_pushnumber(L, check_random_native(L, 1)->unitrandom());
    return 1;
}
static int dfhack_random_unitvector(lua_State *L)
{
    MersenneRNG *prng = check_random_native(L, 1);
    int size = luaL_optint(L, 2, 3);
    if (size <= 0 || size > 32)
        luaL_argerror(L, 2, "vector size must be positive");
    luaL_checkstack(L, size, "not enough stack in dfhack.random.unitvector");

    std::vector<double> buf(size);
    prng->unitvector(buf.data(), size);

    for (int i = 0; i < size; i++)
        lua_pushnumber(L, buf[i]);
    return size;
}

static int eval_perlin_1(lua_State *L)
{
    auto &gen = *(PerlinNoise1D<float>*)lua_touserdata(L, lua_upvalueindex(1));
    lua_pushnumber(L, gen(luaL_checknumber(L, 1)));
    return 1;
}
static int eval_perlin_2(lua_State *L)
{
    auto &gen = *(PerlinNoise2D<float>*)lua_touserdata(L, lua_upvalueindex(1));
    lua_pushnumber(L, gen(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
    return 1;
}
static int eval_perlin_3(lua_State *L)
{
    auto &gen = *(PerlinNoise3D<float>*)lua_touserdata(L, lua_upvalueindex(1));
    lua_pushnumber(L, gen(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3)));
    return 1;
}

static int dfhack_random_perlin(lua_State *L)
{
    MersenneRNG *prng = check_random_native(L, 1);
    int size = luaL_optint(L, 2, 3);

    switch (size)
    {
        case 1: {
            auto pdata = new (L) PerlinNoise1D<float>();
            pdata->init(*prng);
            lua_pushcclosure(L, eval_perlin_1, 1);
            break;
        }
        case 2: {
            auto pdata = new (L) PerlinNoise2D<float>();
            pdata->init(*prng);
            lua_pushcclosure(L, eval_perlin_2, 1);
            break;
        }
        case 3: {
            auto pdata = new (L) PerlinNoise3D<float>();
            pdata->init(*prng);
            lua_pushcclosure(L, eval_perlin_3, 1);
            break;
        }
        default:
            luaL_argerror(L, 2, "perlin noise dimension must be 1, 2 or 3");
    }

    return 1;
}

static const luaL_Reg dfhack_random_funcs[] = {
    { "new", dfhack_random_new },
    { "init", dfhack_random_init },
    { "random", dfhack_random_random },
    { "drandom", dfhack_random_drandom },
    { "drandom0", dfhack_random_drandom0 },
    { "drandom1", dfhack_random_drandom1 },
    { "unitrandom", dfhack_random_unitrandom },
    { "unitvector", dfhack_random_unitvector },
    { "perlin", dfhack_random_perlin },
    { NULL, NULL }
};

static void OpenRandom(lua_State *state)
{
    luaL_getsubtable(state, lua_gettop(state), "random");

    lua_dup(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &DFHACK_RANDOM_TOKEN);

    luaL_setfuncs(state, dfhack_random_funcs, 0);

    lua_pop(state, 1);
}


/*********************************
* Commandline history repository *
**********************************/

static std::map<std::string, CommandHistory> commandHistories;

static CommandHistory * ensureCommandHistory(std::string id,
                                             std::string src_file) {
    if (!commandHistories.count(id)) {
        commandHistories[id].load(src_file.c_str());
    }
    return &commandHistories[id];
}

static int getCommandHistory(lua_State *state)
{
    std::string id = lua_tostring(state, 1);
    std::string src_file = lua_tostring(state, 2);
    std::vector<std::string> entries;
    ensureCommandHistory(id, src_file)->getEntries(entries);
    Lua::PushVector(state, entries);
    return 1;
}

static void addCommandToHistory(std::string id, std::string src_file,
                                std::string command) {
    CommandHistory *history = ensureCommandHistory(id, src_file);
    history->add(command);
    history->save(src_file.c_str());
}

/************************
 * Wrappers for C++ API *
 ************************/

static void OpenModule(lua_State *state, const char *mname,
                       const LuaWrapper::FunctionReg *reg, const luaL_Reg *reg2 = NULL)
{
    luaL_getsubtable(state, lua_gettop(state), mname);
    LuaWrapper::SetFunctionWrappers(state, reg);
    if (reg2)
        luaL_setfuncs(state, reg2, 0);
    lua_pop(state, 1);
}

#define WRAPM(module, function) { #function, df::wrap_function(module::function,true) }
#define WRAP(function) { #function, df::wrap_function(function,true) }
#define WRAPN(name, function) { #name, df::wrap_function(function,true) }
#define CWRAP(name, function) { #name, &Lua::CallWithCatchWrapper<function> }

/***** DFHack module *****/

static std::string getOSType()
{
    switch (Core::getInstance().vinfo->getOS())
    {
        case OS_WINDOWS:
            return "windows";

        case OS_LINUX:
            return "linux";

        case OS_APPLE:
            return "darwin";

        default:
            return "unknown";
    }
}

static int getArchitecture()
{
    return sizeof(void*) * 8;
}

static std::string getArchitectureName()
{
    return getArchitecture() == 64 ? "x86_64" : "x86";
}

static std::string getDFVersion() { return Core::getInstance().vinfo->getVersion(); }
static uint32_t getTickCount() { return Core::getInstance().p->getTickCount(); }

static std::string getDFPath() { return Core::getInstance().p->getPath(); }
static std::string getHackPath() { return Core::getInstance().getHackPath(); }

static bool isWorldLoaded() { return Core::getInstance().isWorldLoaded(); }
static bool isMapLoaded() { return Core::getInstance().isMapLoaded(); }

static std::string df2utf(std::string s) { return DF2UTF(s); }
static std::string utf2df(std::string s) { return UTF2DF(s); }
static std::string df2console(color_ostream &out, std::string s) { return DF2CONSOLE(out, s); }
static std::string toSearchNormalized(std::string s) { return to_search_normalized(s); }

#define WRAP_VERSION_FUNC(name, function) WRAPN(name, DFHack::Version::function)

static const LuaWrapper::FunctionReg dfhack_module[] = {
    WRAP(getOSType),
    WRAP(getArchitecture),
    WRAP(getArchitectureName),
    WRAP(getDFVersion),
    WRAP(getDFPath),
    WRAP(getTickCount),
    WRAP(getHackPath),
    WRAP(isWorldLoaded),
    WRAP(isMapLoaded),
    WRAPM(Translation, TranslateName),
    WRAP(df2utf),
    WRAP(utf2df),
    WRAP(df2console),
    WRAP(toSearchNormalized),
    WRAP_VERSION_FUNC(getDFHackVersion, dfhack_version),
    WRAP_VERSION_FUNC(getDFHackRelease, dfhack_release),
    WRAP_VERSION_FUNC(getDFHackBuildID, dfhack_build_id),
    WRAP_VERSION_FUNC(getCompiledDFVersion, df_version),
    WRAP_VERSION_FUNC(getGitDescription, git_description),
    WRAP_VERSION_FUNC(getGitCommit, git_commit),
    WRAP_VERSION_FUNC(getGitXmlCommit, git_xml_commit),
    WRAP_VERSION_FUNC(getGitXmlExpectedCommit, git_xml_expected_commit),
    WRAP_VERSION_FUNC(gitXmlMatch, git_xml_match),
    WRAP_VERSION_FUNC(isRelease, is_release),
    WRAP_VERSION_FUNC(isPrerelease, is_prerelease),
    WRAP(addCommandToHistory),
    { NULL, NULL }
};

static const luaL_Reg dfhack_funcs[] = {
    { "getCommandHistory", getCommandHistory },
    { NULL, NULL }
};

/***** Gui module *****/

static int gui_getDwarfmodeViewDims(lua_State *state)
{
    auto dims = Gui::getDwarfmodeViewDims();
    lua_newtable(state);
    Lua::TableInsert(state, "map_x1", dims.map_x1);
    Lua::TableInsert(state, "map_x2", dims.map_x2);
    Lua::TableInsert(state, "map_y1", dims.map_y1);
    Lua::TableInsert(state, "map_y2", dims.map_y2);
    return 1;
}

static int gui_getMousePos(lua_State *L)
{
    auto pos = Gui::getMousePos();
    if (pos.isValid())
        Lua::Push(L, pos);
    else
        lua_pushnil(L);
    return 1;
}

static const LuaWrapper::FunctionReg dfhack_gui_module[] = {
    WRAPM(Gui, getCurViewscreen),
    WRAPM(Gui, getDFViewscreen),
    WRAPM(Gui, getSelectedWorkshopJob),
    WRAPM(Gui, getSelectedJob),
    WRAPM(Gui, getSelectedUnit),
    WRAPM(Gui, getSelectedItem),
    WRAPM(Gui, getSelectedBuilding),
    WRAPM(Gui, getSelectedCivZone),
    WRAPM(Gui, getSelectedStockpile),
    WRAPM(Gui, getSelectedPlant),
    WRAPM(Gui, getAnyUnit),
    WRAPM(Gui, getAnyItem),
    WRAPM(Gui, getAnyBuilding),
    WRAPM(Gui, getAnyPlant),
    WRAPM(Gui, writeToGamelog),
    WRAPM(Gui, makeAnnouncement),
    WRAPM(Gui, addCombatReport),
    WRAPM(Gui, addCombatReportAuto),
    WRAPM(Gui, showAnnouncement),
    WRAPM(Gui, showZoomAnnouncement),
    WRAPM(Gui, showPopupAnnouncement),
    WRAPM(Gui, showAutoAnnouncement),
    WRAPM(Gui, resetDwarfmodeView),
    WRAPM(Gui, refreshSidebar),
    WRAPM(Gui, inRenameBuilding),
    WRAPM(Gui, getDepthAt),
    WRAPM(Gui, matchFocusString),
    { NULL, NULL }
};

static int gui_getFocusStrings(lua_State *state) {
    df::viewscreen *r = Lua::GetDFObject<df::viewscreen>(state, 1);
    std::vector<std::string> focusStrings = Gui::getFocusStrings(r);
    Lua::PushVector(state, focusStrings);
    return 1;
}

static int gui_getCurFocus(lua_State *state) {
    bool skip_dismissed = lua_toboolean(state, 1);
    std::vector<std::string> cur_focus = Gui::getCurFocus(skip_dismissed);
    Lua::PushVector(state, cur_focus);
    return 1;
}

static int gui_autoDFAnnouncement(lua_State *state)
{
    bool rv;
    df::report_init *r = Lua::GetDFObject<df::report_init>(state, 1);

    if (r)
    {
        std::string message = luaL_checkstring(state, 2);
        rv = Gui::autoDFAnnouncement(*r, message);
    }
    else
    {
        df::coord pos;
        int color = 0; // initialize these to prevent warning
        bool bright = false, is_sparring = false;
        df::unit *unit1 = NULL, *unit2 = NULL;

        auto type = (df::announcement_type)lua_tointeger(state, 1);
        Lua::CheckDFAssign(state, &pos, 2);
        std::string message = luaL_checkstring(state, 3);

        switch (lua_gettop(state))
        {
            default:
            case 8:
                is_sparring = lua_toboolean(state, 8);
            case 7:
                unit2 = Lua::CheckDFObject<df::unit>(state, 7);
            case 6:
                unit1 = Lua::CheckDFObject<df::unit>(state, 6);
            case 5:
                bright = lua_toboolean(state, 5);
            case 4:
                color = lua_tointeger(state, 4);
            case 3:
                break;
        }

        switch (lua_gettop(state))
        {   // Use the defaults in Gui.h
            default:
            case 8:
                rv = Gui::autoDFAnnouncement(type, pos, message, color, bright, unit1, unit2, is_sparring);
                break;
            case 7:
                rv = Gui::autoDFAnnouncement(type, pos, message, color, bright, unit1, unit2);
                break;
            case 6:
                rv = Gui::autoDFAnnouncement(type, pos, message, color, bright, unit1);
                break;
            case 5:
                rv = Gui::autoDFAnnouncement(type, pos, message, color, bright);
                break;
            case 4:
                rv = Gui::autoDFAnnouncement(type, pos, message, color);
                break;
            case 3:
                rv = Gui::autoDFAnnouncement(type, pos, message);
        }
    }

    lua_pushboolean(state, rv);
    return 1;
}

static int gui_pauseRecenter(lua_State *state)
{
    bool rv;
    df::coord p;

    switch (lua_gettop(state))
    {
        default:
        case 4:
            rv = Gui::pauseRecenter(CheckCoordXYZ(state, 1, false), lua_toboolean(state, 4));
            break;
        case 3:
            rv = Gui::pauseRecenter(CheckCoordXYZ(state, 1, false));
            break;
        case 2:
            Lua::CheckDFAssign(state, &p, 1);
            rv = Gui::pauseRecenter(p, lua_toboolean(state, 2));
            break;
        case 1:
            Lua::CheckDFAssign(state, &p, 1);
            rv = Gui::pauseRecenter(p);
    }

    lua_pushboolean(state, rv);
    return 1;
}

static int gui_revealInDwarfmodeMap(lua_State *state)
{
    bool rv;
    df::coord p;

    switch (lua_gettop(state))
    {
        default:
        case 4:
            rv = Gui::revealInDwarfmodeMap(CheckCoordXYZ(state, 1, false), lua_toboolean(state, 4));
            break;
        case 3:
            rv = Gui::revealInDwarfmodeMap(CheckCoordXYZ(state, 1, false));
            break;
        case 2:
            Lua::CheckDFAssign(state, &p, 1);
            rv = Gui::revealInDwarfmodeMap(p, lua_toboolean(state, 2));
            break;
        case 1:
            Lua::CheckDFAssign(state, &p, 1);
            rv = Gui::revealInDwarfmodeMap(p);
    }

    lua_pushboolean(state, rv);
    return 1;
}

static const luaL_Reg dfhack_gui_funcs[] = {
    { "autoDFAnnouncement", gui_autoDFAnnouncement },
    { "getDwarfmodeViewDims", gui_getDwarfmodeViewDims },
    { "pauseRecenter", gui_pauseRecenter },
    { "revealInDwarfmodeMap", gui_revealInDwarfmodeMap },
    { "getMousePos", gui_getMousePos },
    { "getFocusStrings", gui_getFocusStrings },
    { "getCurFocus", gui_getCurFocus },
    { NULL, NULL }
};

/***** Job module *****/

static bool jobEqual(const df::job *job1, const df::job *job2)
{
    CHECK_NULL_POINTER(job1);
    CHECK_NULL_POINTER(job2);
    return *job1 == *job2;
}
static bool jobItemEqual(const df::job_item *job1, const df::job_item *job2)
{
    CHECK_NULL_POINTER(job1);
    CHECK_NULL_POINTER(job2);
    return *job1 == *job2;
}

static const LuaWrapper::FunctionReg dfhack_job_module[] = {
    WRAPM(Job,cloneJobStruct),
    WRAPM(Job,printItemDetails),
    WRAPM(Job,printJobDetails),
    WRAPM(Job,getGeneralRef),
    WRAPM(Job,getSpecificRef),
    WRAPM(Job,getHolder),
    WRAPM(Job,getWorker),
    WRAPM(Job,setJobCooldown),
    WRAPM(Job,removeWorker),
    WRAPM(Job,checkBuildingsNow),
    WRAPM(Job,checkDesignationsNow),
    WRAPM(Job,isSuitableItem),
    WRAPM(Job,isSuitableMaterial),
    WRAPM(Job,getName),
    WRAPM(Job,linkIntoWorld),
    WRAPM(Job,removePostings),
    WRAPM(Job,disconnectJobItem),
    WRAPM(Job,disconnectJobGeneralRef),
    WRAPM(Job,removeJob),
    WRAPN(is_equal, jobEqual),
    WRAPN(is_item_equal, jobItemEqual),
    { NULL, NULL }
};

static int job_listNewlyCreated(lua_State *state)
{
    int nxid = luaL_checkint(state, 1);

    lua_settop(state, 1);

    std::vector<df::job*> pvec;
    if (Job::listNewlyCreated(&pvec, &nxid))
    {
        lua_pushinteger(state, nxid);
        Lua::PushVector(state, pvec);
        return 2;
    }
    else
        return 1;
}

static const luaL_Reg dfhack_job_funcs[] = {
    { "listNewlyCreated", job_listNewlyCreated },
    { NULL, NULL }
};

/***** Textures module *****/

static const LuaWrapper::FunctionReg dfhack_textures_module[] = {
    WRAPM(Textures, getDfhackLogoTexposStart),
    WRAPM(Textures, getGreenPinTexposStart),
    WRAPM(Textures, getRedPinTexposStart),
    WRAPM(Textures, getIconsTexposStart),
    WRAPM(Textures, getOnOffTexposStart),
    WRAPM(Textures, getControlPanelTexposStart),
    WRAPM(Textures, getThinBordersTexposStart),
    WRAPM(Textures, getMediumBordersTexposStart),
    WRAPM(Textures, getPanelBordersTexposStart),
    WRAPM(Textures, getWindowBordersTexposStart),
    { NULL, NULL }
};

/***** Units module *****/

static const LuaWrapper::FunctionReg dfhack_units_module[] = {
    WRAPM(Units, isUnitInBox),
    WRAPM(Units, isActive),
    WRAPM(Units, isVisible),
    WRAPM(Units, isCitizen),
    WRAPM(Units, isFortControlled),
    WRAPM(Units, isOwnCiv),
    WRAPM(Units, isOwnGroup),
    WRAPM(Units, isOwnRace),
    WRAPM(Units, isAlive),
    WRAPM(Units, isDead),
    WRAPM(Units, isKilled),
    WRAPM(Units, isSane),
    WRAPM(Units, isCrazed),
    WRAPM(Units, isGhost),
    WRAPM(Units, isHidden),
    WRAPM(Units, isHidingCurse),
    WRAPM(Units, isMale),
    WRAPM(Units, isFemale),
    WRAPM(Units, isBaby),
    WRAPM(Units, isChild),
    WRAPM(Units, isAdult),
    WRAPM(Units, isGay),
    WRAPM(Units, isNaked),
    WRAPM(Units, isVisiting),
    WRAPM(Units, isTrainableHunting),
    WRAPM(Units, isTrainableWar),
    WRAPM(Units, isTrained),
    WRAPM(Units, isHunter),
    WRAPM(Units, isWar),
    WRAPM(Units, isTame),
    WRAPM(Units, isTamable),
    WRAPM(Units, isDomesticated),
    WRAPM(Units, isMarkedForSlaughter),
    WRAPM(Units, isMarkedForGelding),
    WRAPM(Units, isGeldable),
    WRAPM(Units, isGelded),
    WRAPM(Units, isEggLayer),
    WRAPM(Units, isGrazer),
    WRAPM(Units, isMilkable),
    WRAPM(Units, isForest),
    WRAPM(Units, isMischievous),
    WRAPM(Units, isAvailableForAdoption),
    WRAPM(Units, isPet),
    WRAPM(Units, hasExtravision),
    WRAPM(Units, isOpposedToLife),
    WRAPM(Units, isBloodsucker),
    WRAPM(Units, isDwarf),
    WRAPM(Units, isAnimal),
    WRAPM(Units, isMerchant),
    WRAPM(Units, isDiplomat),
    WRAPM(Units, isVisitor),
    WRAPM(Units, isInvader),
    WRAPM(Units, isUndead),
    WRAPM(Units, isNightCreature),
    WRAPM(Units, isSemiMegabeast),
    WRAPM(Units, isMegabeast),
    WRAPM(Units, isTitan),
    WRAPM(Units, isDemon),
    WRAPM(Units, isDanger),
    WRAPM(Units, isGreatDanger),
    WRAPM(Units, teleport),
    WRAPM(Units, getGeneralRef),
    WRAPM(Units, getSpecificRef),
    WRAPM(Units, getContainer),
    WRAPM(Units, setNickname),
    WRAPM(Units, getVisibleName),
    WRAPM(Units, getIdentity),
    WRAPM(Units, getNemesis),
    WRAPM(Units, getPhysicalAttrValue),
    WRAPM(Units, getMentalAttrValue),
    WRAPM(Units, getMiscTrait),
    WRAPM(Units, getAge),
    WRAPM(Units, getKillCount),
    WRAPM(Units, getNominalSkill),
    WRAPM(Units, getEffectiveSkill),
    WRAPM(Units, getExperience),
    WRAPM(Units, isValidLabor),
    WRAPM(Units, setLaborValidity),
    WRAPM(Units, computeMovementSpeed),
    WRAPM(Units, computeSlowdownFactor),
    WRAPM(Units, getProfessionName),
    WRAPM(Units, getCasteProfessionName),
    WRAPM(Units, getProfessionColor),
    WRAPM(Units, getCasteProfessionColor),
    WRAPM(Units, getGoalType),
    WRAPM(Units, getGoalName),
    WRAPM(Units, isGoalAchieved),
    WRAPM(Units, getSquadName),
    WRAPM(Units, getPhysicalDescription),
    WRAPM(Units, getRaceName),
    WRAPM(Units, getRaceNamePlural),
    WRAPM(Units, getRaceNameById),
    WRAPM(Units, getRaceBabyName),
    WRAPM(Units, getRaceBabyNameById),
    WRAPM(Units, getRaceChildName),
    WRAPM(Units, getRaceChildNameById),
    WRAPM(Units, getMainSocialActivity),
    WRAPM(Units, getMainSocialEvent),
    WRAPM(Units, getStressCategory),
    WRAPM(Units, getStressCategoryRaw),
    WRAPM(Units, subtractActionTimers),
    WRAPM(Units, subtractGroupActionTimers),
    WRAPM(Units, multiplyActionTimers),
    WRAPM(Units, multiplyGroupActionTimers),
    WRAPM(Units, setActionTimers),
    WRAPM(Units, setGroupActionTimers),
    { NULL, NULL }
};

static int units_getPosition(lua_State *state)
{
    return Lua::PushPosXYZ(state, Units::getPosition(Lua::CheckDFObject<df::unit>(state,1)));
}

static int units_getOuterContainerRef(lua_State *state)
{
    auto ref = Units::getOuterContainerRef(Lua::CheckDFObject<df::unit>(state, 1));

    lua_newtable(state);
    Lua::TableInsert(state, "type", ref.type);

    switch (ref.type)
    {
        case specific_ref_type::UNIT:
            Lua::TableInsert(state, "object", ref.data.unit);
            break;
        case specific_ref_type::ITEM_GENERAL:
            Lua::TableInsert(state, "object", (df::item*)ref.data.object);
            break;
        case specific_ref_type::VERMIN_EVENT:
            Lua::TableInsert(state, "object", ref.data.vermin);
            break;
        default:
            Lua::TableInsert(state, "object", NULL);
    }

    return 1;
}

static int units_getNoblePositions(lua_State *state)
{
    std::vector<Units::NoblePosition> np;

    if (Units::getNoblePositions(&np, Lua::CheckDFObject<df::unit>(state,1)))
        Lua::PushVector(state, np);
    else
        lua_pushnil(state);

    return 1;
}

static int units_getUnitsInBox(lua_State *state)
{
    std::vector<df::unit*> units;
    int x1 = luaL_checkint(state, 1);
    int y1 = luaL_checkint(state, 2);
    int z1 = luaL_checkint(state, 3);
    int x2 = luaL_checkint(state, 4);
    int y2 = luaL_checkint(state, 5);
    int z2 = luaL_checkint(state, 6);

    bool ok = Units::getUnitsInBox(units, x1, y1, z1, x2, y2, z2);

    if (ok && !lua_isnone(state, 7))
    {
        luaL_checktype(state, 7, LUA_TFUNCTION);
        units.erase(std::remove_if(units.begin(), units.end(), [&state](df::unit *unit) -> bool {
            // todo: merging this filter into the base function would be welcomed by plugins
            //  (it would also be faster, and less obfuscated than this [ie. erase(remove_if)])
            lua_dup(state); // copy function
            Lua::PushDFObject(state, unit);
            lua_call(state, 1, 1);
            bool ret = lua_toboolean(state, -1);
            lua_pop(state, 1); // remove return value
            return !ret;
        }), units.end());
    }

    Lua::PushVector(state, units);
    lua_pushboolean(state, ok);
    return 2;
}

static int units_getCitizens(lua_State *L) {
    bool ignore_sanity = lua_toboolean(L, -1); // defaults to false

    std::vector<df::unit *> citizens;
    if (Units::getCitizens(citizens, ignore_sanity)) {
        Lua::PushVector(L, citizens);
        return 1;
    }
    return 0;
}

static int units_getStressCutoffs(lua_State *L)
{
    lua_newtable(L);
    for (size_t i = 0; i < Units::stress_cutoffs.size(); i++)
        Lua::TableInsert(L, i, Units::stress_cutoffs[i]);
    return 1;
}

static const luaL_Reg dfhack_units_funcs[] = {
    { "getPosition", units_getPosition },
    { "getOuterContainerRef", units_getOuterContainerRef },
    { "getNoblePositions", units_getNoblePositions },
    { "getUnitsInBox", units_getUnitsInBox },
    { "getCitizens", units_getCitizens },
    { "getStressCutoffs", units_getStressCutoffs },
    { NULL, NULL }
};

/***** Items module *****/

static bool items_moveToGround(df::item *item, df::coord pos)
{
    MapExtras::MapCache mc;
    return Items::moveToGround(mc, item, pos);
}

static bool items_moveToContainer(df::item *item, df::item *container)
{
    MapExtras::MapCache mc;
    return Items::moveToContainer(mc, item, container);
}

static bool items_moveToInventory
    (df::item *item, df::unit *unit, df::unit_inventory_item::T_mode mode, int body_part)
{
    MapExtras::MapCache mc;
    return Items::moveToInventory(mc, item, unit, mode, body_part);
}

static bool items_remove(df::item *item, bool no_uncat)
{
    MapExtras::MapCache mc;
    return Items::remove(mc, item, no_uncat);
}

static df::proj_itemst *items_makeProjectile(df::item *item)
{
    MapExtras::MapCache mc;
    return Items::makeProjectile(mc, item);
}

static int16_t items_findType(std::string token)
{
    DFHack::ItemTypeInfo result;
    result.find(token);
    return result.type;
}

static int32_t items_findSubtype(std::string token)
{
    DFHack::ItemTypeInfo result;
    result.find(token);
    return result.subtype;
}

static const LuaWrapper::FunctionReg dfhack_items_module[] = {
    WRAPM(Items, getGeneralRef),
    WRAPM(Items, getSpecificRef),
    WRAPM(Items, getOwner),
    WRAPM(Items, setOwner),
    WRAPM(Items, getContainer),
    WRAPM(Items, getHolderBuilding),
    WRAPM(Items, getHolderUnit),
    WRAPM(Items, getBookTitle),
    WRAPM(Items, getDescription),
    WRAPM(Items, isCasteMaterial),
    WRAPM(Items, getSubtypeCount),
    WRAPM(Items, getSubtypeDef),
    WRAPM(Items, getItemBaseValue),
    WRAPM(Items, getValue),
    WRAPM(Items, createItem),
    WRAPM(Items, checkMandates),
    WRAPM(Items, canTrade),
    WRAPM(Items, canTradeWithContents),
    WRAPM(Items, isRouteVehicle),
    WRAPM(Items, isSquadEquipment),
    WRAPN(moveToGround, items_moveToGround),
    WRAPN(moveToContainer, items_moveToContainer),
    WRAPN(moveToInventory, items_moveToInventory),
    WRAPN(makeProjectile, items_makeProjectile),
    WRAPN(remove, items_remove),
    WRAPN(findType, items_findType),
    WRAPN(findSubtype, items_findSubtype),
    { NULL, NULL }
};

static int items_getPosition(lua_State *state)
{
    return Lua::PushPosXYZ(state, Items::getPosition(Lua::CheckDFObject<df::item>(state,1)));
}

static int items_getOuterContainerRef(lua_State *state)
{
    auto ref = Items::getOuterContainerRef(Lua::CheckDFObject<df::item>(state, 1));

    lua_newtable(state);
    Lua::TableInsert(state, "type", ref.type);

    switch (ref.type)
    {
        case specific_ref_type::UNIT:
            Lua::TableInsert(state, "object", ref.data.unit);
            break;
        case specific_ref_type::ITEM_GENERAL:
            Lua::TableInsert(state, "object", (df::item*)ref.data.object);
            break;
        case specific_ref_type::VERMIN_EVENT:
            Lua::TableInsert(state, "object", ref.data.vermin);
            break;
        default:
            Lua::TableInsert(state, "object", NULL);
    }

    return 1;
}

static int items_getContainedItems(lua_State *state)
{
    std::vector<df::item*> pvec;
    Items::getContainedItems(Lua::CheckDFObject<df::item>(state,1),&pvec);
    Lua::PushVector(state, pvec);
    return 1;
}

static int items_moveToBuilding(lua_State *state)
{
    MapExtras::MapCache mc;
    auto item = Lua::CheckDFObject<df::item>(state, 1);
    auto building = Lua::CheckDFObject<df::building_actual>(state, 2);
    int use_mode = luaL_optint(state, 3, 0);
    bool force_in_building = lua_toboolean(state, 4);
    lua_pushboolean(state, Items::moveToBuilding(mc, item, building, use_mode, force_in_building));
    return 1;
}


static const luaL_Reg dfhack_items_funcs[] = {
    { "getPosition", items_getPosition },
    { "getOuterContainerRef", items_getOuterContainerRef },
    { "getContainedItems", items_getContainedItems },
    { "moveToBuilding", items_moveToBuilding },
    { NULL, NULL }
};

/***** Maps module *****/

static bool hasTileAssignment(df::tile_bitmask *bm) {
    return bm && bm->has_assignments();
}
static bool getTileAssignment(df::tile_bitmask *bm, int x, int y) {
    return bm && bm->getassignment(x,y);
}
static void setTileAssignment(df::tile_bitmask *bm, int x, int y, bool val) {
    CHECK_NULL_POINTER(bm);
    bm->setassignment(x,y,val);
}
static void resetTileAssignment(df::tile_bitmask *bm, bool val) {
    CHECK_NULL_POINTER(bm);
    if (val) bm->set_all();
    else bm->clear();
}

static const LuaWrapper::FunctionReg dfhack_maps_module[] = {
    WRAPN(getBlock, (df::map_block* (*)(int32_t,int32_t,int32_t))Maps::getBlock),
    WRAPM(Maps, enableBlockUpdates),
    WRAPM(Maps, getGlobalInitFeature),
    WRAPM(Maps, getLocalInitFeature),
    WRAPM(Maps, canWalkBetween),
    WRAPM(Maps, spawnFlow),
    WRAPN(hasTileAssignment, hasTileAssignment),
    WRAPN(getTileAssignment, getTileAssignment),
    WRAPN(setTileAssignment, setTileAssignment),
    WRAPN(resetTileAssignment, resetTileAssignment),
    { NULL, NULL }
};

static int maps_isValidTilePos(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    lua_pushboolean(L, Maps::isValidTilePos(pos));
    return 1;
}

static int maps_isTileVisible(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    lua_pushboolean(L, Maps::isTileVisible(pos));
    return 1;
}

static int maps_getTileBlock(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    Lua::PushDFObject(L, Maps::getTileBlock(pos));
    return 1;
}

static int maps_ensureTileBlock(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    Lua::PushDFObject(L, Maps::ensureTileBlock(pos));
    return 1;
}

static int maps_getTileType(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    auto ptype = Maps::getTileType(pos);
    if (ptype)
        lua_pushinteger(L, *ptype);
    else
        lua_pushnil(L);
    return 1;
}

static int maps_getTileFlags(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    Lua::PushDFObject(L, Maps::getTileDesignation(pos));
    Lua::PushDFObject(L, Maps::getTileOccupancy(pos));
    return 2;
}

static int maps_getRegionBiome(lua_State *L)
{
    auto pos = CheckCoordXY(L, 1, true);
    Lua::PushDFObject(L, Maps::getRegionBiome(pos));
    return 1;
}

static int maps_getTileBiomeRgn(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    return Lua::PushPosXY(L, Maps::getTileBiomeRgn(pos));
}

static int maps_getPlantAtTile(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    Lua::PushDFObject(L, Maps::getPlantAtTile(pos));
    return 1;
}

static int maps_getBiomeType(lua_State *L)
{
    auto pos = CheckCoordXY(L, 1, true);
    lua_pushinteger(L, Maps::getBiomeType(pos.x, pos.y));
    return 1;
}

static const luaL_Reg dfhack_maps_funcs[] = {
    { "isValidTilePos", maps_isValidTilePos },
    { "isTileVisible", maps_isTileVisible },
    { "getTileBlock", maps_getTileBlock },
    { "ensureTileBlock", maps_ensureTileBlock },
    { "getTileType", maps_getTileType },
    { "getTileFlags", maps_getTileFlags },
    { "getRegionBiome", maps_getRegionBiome },
    { "getTileBiomeRgn", maps_getTileBiomeRgn },
    { "getPlantAtTile", maps_getPlantAtTile },
    { "getBiomeType", maps_getBiomeType },
    { NULL, NULL }
};

/****** World module ******/

static const LuaWrapper::FunctionReg dfhack_world_module[] = {
    WRAPM(World, ReadPauseState),
    WRAPM(World, SetPauseState),
    WRAPM(World, ReadCurrentTick),
    WRAPM(World, ReadCurrentYear),
    WRAPM(World, ReadCurrentMonth),
    WRAPM(World, ReadCurrentDay),
    WRAPM(World, ReadCurrentWeather),
    WRAPM(World, SetCurrentWeather),
    WRAPM(World, ReadWorldFolder),
    { NULL, NULL }
};

#define WORLD_GAMEMODE_WRAPPER(func) \
    static int world_gamemode_##func(lua_State *L) \
    { \
        int gametype = luaL_optint(L, 1, -1); \
        lua_pushboolean(L, World::func((df::game_type)gametype)); \
        return 1;\
    }
#define WORLD_GAMEMODE_FUNC(func) \
    {#func, world_gamemode_##func}

WORLD_GAMEMODE_WRAPPER(isFortressMode);
WORLD_GAMEMODE_WRAPPER(isAdventureMode);
WORLD_GAMEMODE_WRAPPER(isArena);
WORLD_GAMEMODE_WRAPPER(isLegends);

static const luaL_Reg dfhack_world_funcs[] = {
    WORLD_GAMEMODE_FUNC(isFortressMode),
    WORLD_GAMEMODE_FUNC(isAdventureMode),
    WORLD_GAMEMODE_FUNC(isArena),
    WORLD_GAMEMODE_FUNC(isLegends),
    { NULL, NULL }
};

/***** Burrows module *****/

static bool burrows_isAssignedBlockTile(df::burrow *burrow, df::map_block *block, int x, int y)
{
    return Burrows::isAssignedBlockTile(burrow, block, df::coord2d(x,y));
}

static bool burrows_setAssignedBlockTile(df::burrow *burrow, df::map_block *block, int x, int y, bool enable)
{
    return Burrows::setAssignedBlockTile(burrow, block, df::coord2d(x,y), enable);
}

static const LuaWrapper::FunctionReg dfhack_burrows_module[] = {
    WRAPM(Burrows, findByName),
    WRAPM(Burrows, clearUnits),
    WRAPM(Burrows, isAssignedUnit),
    WRAPM(Burrows, setAssignedUnit),
    WRAPM(Burrows, clearTiles),
    WRAPN(isAssignedBlockTile, burrows_isAssignedBlockTile),
    WRAPN(setAssignedBlockTile, burrows_setAssignedBlockTile),
    WRAPM(Burrows, isAssignedTile),
    WRAPM(Burrows, setAssignedTile),
    { NULL, NULL }
};

static int burrows_listBlocks(lua_State *state)
{
    std::vector<df::map_block*> pvec;
    Burrows::listBlocks(&pvec, Lua::CheckDFObject<df::burrow>(state,1));
    Lua::PushVector(state, pvec);
    return 1;
}

static const luaL_Reg dfhack_burrows_funcs[] = {
    { "listBlocks", burrows_listBlocks },
    { NULL, NULL }
};

/***** Buildings module *****/

static bool buildings_containsTile(df::building *bld, int x, int y) {
    return Buildings::containsTile(bld, df::coord2d(x,y));
}

static const LuaWrapper::FunctionReg dfhack_buildings_module[] = {
    WRAPM(Buildings, getGeneralRef),
    WRAPM(Buildings, getSpecificRef),
    WRAPM(Buildings, setOwner),
    WRAPM(Buildings, allocInstance),
    WRAPM(Buildings, checkFreeTiles),
    WRAPM(Buildings, countExtentTiles),
    WRAPN(containsTile, buildings_containsTile),
    WRAPM(Buildings, hasSupport),
    WRAPM(Buildings, constructAbstract),
    WRAPM(Buildings, constructWithItems),
    WRAPM(Buildings, constructWithFilters),
    WRAPM(Buildings, deconstruct),
    WRAPM(Buildings, notifyCivzoneModified),
    WRAPM(Buildings, markedForRemoval),
    WRAPM(Buildings, getRoomDescription),
    WRAPM(Buildings, isActivityZone),
    WRAPM(Buildings, isPenPasture),
    WRAPM(Buildings, isPitPond),
    WRAPM(Buildings, isActive),
    { NULL, NULL }
};

static int buildings_findAtTile(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    Lua::PushDFObject(L, Buildings::findAtTile(pos));
    return 1;
}

static int buildings_findCivzonesAt(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    std::vector<df::building_civzonest*> pvec;
    if (Buildings::findCivzonesAt(&pvec, pos))
        Lua::PushVector(L, pvec);
    else
        lua_pushnil(L);
    return 1;
}

static int buildings_findPenPitAt(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    Lua::PushDFObject(L, Buildings::findPenPitAt(pos));
    return 1;
}

static int buildings_getCorrectSize(lua_State *state)
{
    df::coord2d size(luaL_optint(state, 1, 1), luaL_optint(state, 2, 1));

    auto t = (df::building_type)luaL_optint(state, 3, -1);
    int st = luaL_optint(state, 4, -1);
    int cu = luaL_optint(state, 5, -1);
    int d = luaL_optint(state, 6, 0);

    df::coord2d center;
    bool flexible = Buildings::getCorrectSize(size, center, t, st, cu, d);

    lua_pushboolean(state, flexible);
    lua_pushinteger(state, size.x);
    lua_pushinteger(state, size.y);
    lua_pushinteger(state, center.x);
    lua_pushinteger(state, center.y);
    return 5;
}

namespace {

int buildings_setSize(lua_State *state)
{
    auto bld = Lua::CheckDFObject<df::building>(state, 1);
    df::coord2d size(luaL_optint(state, 2, 1), luaL_optint(state, 3, 1));
    int dir = luaL_optint(state, 4, 0);
    bool ok = Buildings::setSize(bld, size, dir);
    lua_pushboolean(state, ok);
    if (ok)
    {
        auto size = Buildings::getSize(bld).second;
        int area = size.x*size.y;
        lua_pushinteger(state, size.x);
        lua_pushinteger(state, size.y);
        lua_pushinteger(state, area);
        lua_pushinteger(state, Buildings::countExtentTiles(&bld->room, area));
        return 5;
    }
    else
        return 1;
}

}

static int buildings_getStockpileContents(lua_State *state)
{
    std::vector<df::item*> pvec;
    Buildings::getStockpileContents(Lua::CheckDFObject<df::building_stockpilest>(state,1),&pvec);
    Lua::PushVector(state, pvec);
    return 1;
}

static int buildings_getCageOccupants(lua_State *state)
{
    std::vector<df::unit*> units;
    Buildings::getCageOccupants(Lua::CheckDFObject<df::building_cagest>(state, 1), units);
    Lua::PushVector(state, units);
    return 1;
}

static const luaL_Reg dfhack_buildings_funcs[] = {
    { "findAtTile", buildings_findAtTile },
    { "findCivzonesAt", buildings_findCivzonesAt },
    { "getCorrectSize", buildings_getCorrectSize },
    CWRAP(setSize, buildings_setSize),
    CWRAP(getStockpileContents, buildings_getStockpileContents),
    { "findPenPitAt", buildings_findPenPitAt },
    CWRAP(getCageOccupants, buildings_getCageOccupants),
    { NULL, NULL }
};

/***** Constructions module *****/

static const LuaWrapper::FunctionReg dfhack_constructions_module[] = {
    WRAPM(Constructions, designateNew),
    WRAPM(Constructions, insert),
    { NULL, NULL }
};

static int constructions_designateRemove(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    bool imm = false;
    lua_pushboolean(L, Constructions::designateRemove(pos, &imm));
    lua_pushboolean(L, imm);
    return 2;
}

static int constructions_findAtTile(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    Lua::PushDFObject(L, Constructions::findAtTile(pos));
    return 1;
}

static const luaL_Reg dfhack_constructions_funcs[] = {
    { "designateRemove", constructions_designateRemove },
    { "findAtTile", constructions_findAtTile },
    { NULL, NULL }
};

/***** Screen module *****/

static const LuaWrapper::FunctionReg dfhack_screen_module[] = {
    WRAPM(Screen, inGraphicsMode),
    WRAPM(Screen, clear),
    WRAPM(Screen, invalidate),
    WRAPM(Screen, getKeyDisplay),
    { NULL, NULL }
};

static int screen_getMousePos(lua_State *L)
{
    return Lua::PushPosXY(L, Screen::getMousePos());
}

static int screen_getMousePixels(lua_State *L)
{
    return Lua::PushPosXY(L, Screen::getMousePixels());
}

static int screen_getWindowSize(lua_State *L)
{
    return Lua::PushPosXY(L, Screen::getWindowSize());
}

static int screen_paintTile(lua_State *L)
{
    Pen pen;
    Lua::CheckPen(L, &pen, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    if (lua_gettop(L) >= 4 && !lua_isnil(L, 4))
    {
        if (lua_type(L, 4) == LUA_TSTRING)
            pen.ch = lua_tostring(L, 4)[0];
        else
            pen.ch = luaL_checkint(L, 4);
    }
    if (lua_gettop(L) >= 5 && !lua_isnil(L, 5))
        pen.tile = luaL_checkint(L, 5);
    bool map = lua_toboolean(L, 6);
    lua_pushboolean(L, Screen::paintTile(pen, x, y, map));
    return 1;
}

static int screen_readTile(lua_State *L)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    bool map = lua_toboolean(L, 3);
    Pen pen = Screen::readTile(x, y, map);
    Lua::Push(L, pen);
    return 1;
}

static int screen_paintString(lua_State *L)
{
    Pen pen;
    Lua::CheckPen(L, &pen, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    const char *text = luaL_checkstring(L, 4);
    bool map = lua_toboolean(L, 5);
    lua_pushboolean(L, Screen::paintString(pen, x, y, text, map));
    return 1;
}

static int screen_fillRect(lua_State *L)
{
    Pen pen;
    Lua::CheckPen(L, &pen, 1);
    int x1 = luaL_checkint(L, 2);
    int y1 = luaL_checkint(L, 3);
    int x2 = luaL_checkint(L, 4);
    int y2 = luaL_checkint(L, 5);
    bool map = lua_toboolean(L, 6);
    lua_pushboolean(L, Screen::fillRect(pen, x1, y1, x2, y2, map));
    return 1;
}

static int screen_findGraphicsTile(lua_State *L)
{
    auto str = luaL_checkstring(L, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int tile, tile_gs;
    if (Screen::findGraphicsTile(str, x, y, &tile, &tile_gs))
    {
        lua_pushinteger(L, tile);
        lua_pushinteger(L, tile_gs);
        return 2;
    }
    else
    {
        lua_pushnil(L);
        return 1;
    }
}

static int screen_raise(lua_State *L) {
    df::viewscreen *screen = dfhack_lua_viewscreen::get_pointer(L, 1, false);

    // remove screen from the stack so it doesn't get returned as an output
    lua_remove(L, 1);

    Screen::raise(screen);

    return 0;
}

static int screen_hideGuard(lua_State *L) {
    df::viewscreen *screen = dfhack_lua_viewscreen::get_pointer(L, 1, false);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // remove screen from the stack so it doesn't get returned as an output
    lua_remove(L, 1);

    Screen::Hide hideGuard(screen, Screen::Hide::RESTORE_AT_TOP);

    int nargs = lua_gettop(L) - 1;
    lua_call(L, nargs, LUA_MULTRET);

    return lua_gettop(L);
}

namespace {

int screen_show(lua_State *L)
{
    df::viewscreen *before = NULL;
    if (lua_gettop(L) >= 2)
        before = Lua::CheckDFObject<df::viewscreen>(L, 2);

    df::viewscreen *screen = dfhack_lua_viewscreen::get_pointer(L, 1, true);

    bool ok = Screen::show(std::unique_ptr<df::viewscreen>{screen}, before);

    lua_pushboolean(L, ok);
    return 1;
}

static int screen_dismiss(lua_State *L)
{
    df::viewscreen *screen = dfhack_lua_viewscreen::get_pointer(L, 1, false);
    bool to_first = lua_toboolean(L, 2);
    Screen::dismiss(screen, to_first);
    return 0;
}

static int screen_isDismissed(lua_State *L)
{
    df::viewscreen *screen = dfhack_lua_viewscreen::get_pointer(L, 1, false);
    lua_pushboolean(L, Screen::isDismissed(screen));
    return 1;
}

static int screen_doSimulateInput(lua_State *L)
{
    auto screen = Lua::CheckDFObject<df::viewscreen>(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    if (!screen)
        luaL_argerror(L, 1, "NULL screen");

    int sz = lua_rawlen(L, 2);
    std::set<df::interface_key> keys;

    for (int j = 1; j <= sz; j++)
    {
        lua_rawgeti(L, 2, j);
        keys.insert((df::interface_key)lua_tointeger(L, -1));
        lua_pop(L, 1);
    }

    screen->feed(&keys);
    return 0;
}

static int screen_keyToChar(lua_State *L)
{
    auto keycode = (df::interface_key)luaL_checkint(L, 1);
    int charcode = Screen::keyToChar(keycode);
    if (charcode >= 0)
        lua_pushinteger(L, charcode);
    else
        lua_pushnil(L);
    return 1;
}

static int screen_charToKey(lua_State *L)
{
    auto charcode = (char)luaL_checkint(L, 1);
    df::interface_key keycode = Screen::charToKey(charcode);
    if (keycode)
        lua_pushinteger(L, keycode);
    else
        lua_pushnil(L);
    return 1;
}

static int screen_zoom(lua_State *L)
{
    using df::global::enabler;
    df::zoom_commands cmd = (df::zoom_commands)luaL_checkint(L, 1);
    if (cmd < df::enum_traits<df::zoom_commands>::first_item_value ||
        cmd > df::enum_traits<df::zoom_commands>::last_item_value)
    {
        luaL_error(L, "Invalid zoom command: %d", cmd);
    }
    if (!enabler)
    {
        luaL_error(L, "enabler unavailable");
    }
    enabler->zoom_display(cmd);
    return 0;
}

}

static const luaL_Reg dfhack_screen_funcs[] = {
    { "getMousePos", screen_getMousePos },
    { "getMousePixels", screen_getMousePixels },
    { "getWindowSize", screen_getWindowSize },
    { "paintTile", screen_paintTile },
    { "readTile", screen_readTile },
    { "paintString", screen_paintString },
    { "fillRect", screen_fillRect },
    { "findGraphicsTile", screen_findGraphicsTile },
    CWRAP(raise, screen_raise),
    CWRAP(hideGuard, screen_hideGuard),
    CWRAP(show, screen_show),
    CWRAP(dismiss, screen_dismiss),
    CWRAP(isDismissed, screen_isDismissed),
    { "_doSimulateInput", screen_doSimulateInput },
    { "keyToChar", screen_keyToChar },
    { "charToKey", screen_charToKey },
    { "zoom", screen_zoom },
    { NULL, NULL }
};

/***** Filesystem module *****/

static const LuaWrapper::FunctionReg dfhack_filesystem_module[] = {
    WRAPM(Filesystem, getcwd),
    WRAPM(Filesystem, restore_cwd),
    WRAPM(Filesystem, get_initial_cwd),
    WRAPM(Filesystem, chdir),
    WRAPM(Filesystem, mkdir),
    WRAPM(Filesystem, mkdir_recursive),
    WRAPM(Filesystem, rmdir),
    WRAPM(Filesystem, exists),
    WRAPM(Filesystem, isfile),
    WRAPM(Filesystem, isdir),
    WRAPM(Filesystem, atime),
    WRAPM(Filesystem, ctime),
    WRAPM(Filesystem, mtime),
    {NULL, NULL}
};

static int filesystem_listdir(lua_State *L)
{
    luaL_checktype(L,1,LUA_TSTRING);
    std::string dir=lua_tostring(L,1);
    std::vector<std::string> files;
    int err = DFHack::Filesystem::listdir(dir, files);
    if (err)
    {
        lua_pushnil(L);
        lua_pushstring(L, strerror(err));
        lua_pushinteger(L, err);
        return 3;
    }
    lua_newtable(L);
    for(size_t i=0;i<files.size();i++)
    {
        lua_pushinteger(L,i+1);
        lua_pushstring(L,files[i].c_str());
        lua_settable(L,-3);
    }
    return 1;
}

static int filesystem_listdir_recursive(lua_State *L)
{
    luaL_checktype(L,1,LUA_TSTRING);
    std::string dir=lua_tostring(L,1);
    int depth = 10;
    if (lua_gettop(L) >= 2 && !lua_isnil(L, 2))
        depth = luaL_checkint(L, 2);
    bool include_prefix = true;
    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3))
        include_prefix = lua_toboolean(L, 3);
    std::map<std::string, bool> files;
    int err = DFHack::Filesystem::listdir_recursive(dir, files, depth, include_prefix);
    if (err)
    {
        lua_pushnil(L);
        if (err == -1)
            lua_pushfstring(L, "max depth exceeded: %d", depth);
        else
            lua_pushstring(L, strerror(err));
        lua_pushinteger(L, err);
        return 3;
    }
    lua_newtable(L);
    int i = 1;
    for (auto it = files.begin(); it != files.end(); ++it)
    {
        lua_pushinteger(L, i++);
        lua_newtable(L);
        lua_pushstring(L, "path");
        lua_pushstring(L, (it->first).c_str());
        lua_settable(L, -3);
        lua_pushstring(L, "isdir");
        lua_pushboolean(L, it->second);
        lua_settable(L, -3);
        lua_settable(L, -3);
    }
    return 1;
}

static const luaL_Reg dfhack_filesystem_funcs[] = {
    {"listdir", filesystem_listdir},
    {"listdir_recursive", filesystem_listdir_recursive},
    {NULL, NULL}
};

/***** Designations module *****/

static const LuaWrapper::FunctionReg dfhack_designations_module[] = {
    WRAPM(Designations, markPlant),
    WRAPM(Designations, unmarkPlant),
    WRAPM(Designations, canMarkPlant),
    WRAPM(Designations, canUnmarkPlant),
    WRAPM(Designations, isPlantMarked),
    {NULL, NULL}
};

static int designations_getPlantDesignationTile(lua_State *state)
{
    return Lua::PushPosXYZ(state, Designations::getPlantDesignationTile(Lua::CheckDFObject<df::plant>(state, 1)));
}

static const luaL_Reg dfhack_designations_funcs[] = {
    {"getPlantDesignationTile", designations_getPlantDesignationTile},
    {NULL, NULL}
};

/***** Kitchen module *****/

static const LuaWrapper::FunctionReg dfhack_kitchen_module[] = {
    WRAPM(Kitchen, findExclusion),
    WRAPM(Kitchen, addExclusion),
    WRAPM(Kitchen, removeExclusion),
    {NULL, NULL}
};

/***** Console module *****/

namespace console {
    void clear() {
        Core::getInstance().getConsole().clear();
    }
    void flush() {
        Core::getInstance().getConsole() << std::flush;
    }
}

static const LuaWrapper::FunctionReg dfhack_console_module[] = {
    WRAPM(console, clear),
    WRAPM(console, flush),
    { NULL, NULL }
};

/***** Internal module *****/

static void *checkaddr(lua_State *L, int idx, bool allow_null = false)
{
    luaL_checkany(L, idx);
    void *rv;
    if (lua_isnil(L, idx))
        rv = NULL;
    else if (lua_type(L, idx) == LUA_TNUMBER)
        rv = (void*)lua_tounsigned(L, idx);
    else
        rv = Lua::CheckDFObject(L, NULL, idx);
    if (!rv && !allow_null)
        luaL_argerror(L, idx, "null pointer");
    return rv;
}

static md5wrapper md5_wrap;

static uintptr_t getImageBase() { return Core::getInstance().p->getBase(); }
static intptr_t getRebaseDelta() { return Core::getInstance().vinfo->getRebaseDelta(); }
static int8_t getModstate() { return Core::getInstance().getModstate(); }
static std::string internal_strerror(int n) { return strerror(n); }
static std::string internal_md5(std::string s) { return md5_wrap.getHashFromString(s); }

static const LuaWrapper::FunctionReg dfhack_internal_module[] = {
    WRAP(getImageBase),
    WRAP(getRebaseDelta),
    WRAP(getModstate),
    WRAPN(strerror, internal_strerror),
    WRAPN(md5, internal_md5),
    { NULL, NULL }
};

static int internal_getmd5(lua_State *L)
{
    auto& p = Core::getInstance().p;
    if (p->getDescriptor()->getOS() == OS_WINDOWS)
        luaL_error(L, "process MD5 not available on Windows");
    lua_pushstring(L, p->getMD5().c_str());
    return 1;
}

static int internal_getPE(lua_State *L)
{
    auto& p = Core::getInstance().p;
    if (p->getDescriptor()->getOS() != OS_WINDOWS)
        luaL_error(L, "process PE timestamp not available on non-Windows");
    lua_pushinteger(L, p->getPE());
    return 1;
}

static int internal_getAddress(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    uintptr_t addr = Core::getInstance().vinfo->getAddress(name);
    if (addr)
        lua_pushinteger(L, addr);
    else
        lua_pushnil(L);
    return 1;
}

static int internal_setAddress(lua_State *L)
{
    std::string name = luaL_checkstring(L, 1);
    uintptr_t addr = (uintptr_t)checkaddr(L, 2, true);
    internal_getAddress(L);

    // Set the address
    Core::getInstance().vinfo->setAddress(name, addr);

    auto fields = df::global::_identity.getFields();

    for (int i = 0; fields && fields[i].mode != struct_field_info::END; ++i)
    {
        if (fields[i].name != name)
            continue;

        *(void**)fields[i].offset = (void*)addr;
    }

    // Print via printerr, so that it is definitely logged to stderr.log.
    uintptr_t iaddr = addr - Core::getInstance().vinfo->getRebaseDelta();
    fprintf(stderr, "Setting global '%s' to %p (%p)\n", name.c_str(), (void*)addr, (void*)iaddr);
    fflush(stderr);

    return 1;
}

static int internal_getVTable(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    uintptr_t addr = (uintptr_t)Core::getInstance().vinfo->getVTable(name);
    if (addr)
        lua_pushinteger(L, addr);
    else
        lua_pushnil(L);
    return 1;
}

static int internal_adjustOffset(lua_State *L)
{
    lua_settop(L, 2);
    int off = luaL_checkint(L, 1);
    int rv = Core::getInstance().p->adjustOffset(off, lua_toboolean(L, 2));
    if (rv >= 0)
        lua_pushinteger(L, rv);
    else
        lua_pushnil(L);
    return 1;
}

static int internal_getMemRanges(lua_State *L)
{
    std::vector<DFHack::t_memrange> ranges;
    Core::getInstance().p->getMemRanges(ranges);

    lua_newtable(L);

    for(size_t i = 0; i < ranges.size(); i++)
    {
        lua_newtable(L);
        lua_pushinteger(L, (uintptr_t)ranges[i].start);
        lua_setfield(L, -2, "start_addr");
        lua_pushinteger(L, (uintptr_t)ranges[i].end);
        lua_setfield(L, -2, "end_addr");
        lua_pushstring(L, ranges[i].name);
        lua_setfield(L, -2, "name");
        lua_pushboolean(L, ranges[i].read);
        lua_setfield(L, -2, "read");
        lua_pushboolean(L, ranges[i].write);
        lua_setfield(L, -2, "write");
        lua_pushboolean(L, ranges[i].execute);
        lua_setfield(L, -2, "execute");
        lua_pushboolean(L, ranges[i].shared);
        lua_setfield(L, -2, "shared");
        lua_pushboolean(L, ranges[i].valid);
        lua_setfield(L, -2, "valid");
        lua_rawseti(L, -2, i+1);
    }

    return 1;
}

static int internal_patchMemory(lua_State *L)
{
    void *dest = checkaddr(L, 1);
    void *src = checkaddr(L, 2);
    int size = luaL_checkint(L, 3);
    if (size < 0) luaL_argerror(L, 1, "negative size");
    bool ok = Core::getInstance().p->patchMemory(dest, src, size);
    lua_pushboolean(L, ok);
    return 1;
}

static int internal_patchBytes(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 2);

    MemoryPatcher patcher;

    if (!lua_isnil(L, 2))
    {
        luaL_checktype(L, 2, LUA_TTABLE);

        lua_pushnil(L);

        while (lua_next(L, 2))
        {
            uint8_t *addr = (uint8_t*)checkaddr(L, -2, true);
            int isnum;
            uint8_t value = (uint8_t)lua_tounsignedx(L, -1, &isnum);
            if (!isnum)
                luaL_error(L, "invalid value in verify table");
            lua_pop(L, 1);

            if (!patcher.verifyAccess(addr, 1, false))
            {
                lua_pushnil(L);
                lua_pushstring(L, "invalid verify address");
                lua_pushvalue(L, -3);
                return 3;
            }

            if (*addr != value)
            {
                lua_pushnil(L);
                lua_pushstring(L, "wrong verify value");
                lua_pushvalue(L, -3);
                return 3;
            }
        }
    }

    lua_pushnil(L);

    while (lua_next(L, 1))
    {
        uint8_t *addr = (uint8_t*)checkaddr(L, -2, true);
        int isnum;
        lua_tounsignedx(L, -1, &isnum);
        if (!isnum)
            luaL_error(L, "invalid value in write table");
        lua_pop(L, 1);

        if (!patcher.verifyAccess(addr, 1, true))
        {
            lua_pushnil(L);
            lua_pushstring(L, "invalid write address");
            lua_pushvalue(L, -3);
            return 3;
        }
    }

    lua_pushnil(L);

    while (lua_next(L, 1))
    {
        uint8_t *addr = (uint8_t*)checkaddr(L, -2, true);
        uint8_t value = (uint8_t)lua_tounsigned(L, -1);
        lua_pop(L, 1);

        *addr = value;
    }

    lua_pushboolean(L, true);
    return 1;
}

static int internal_memmove(lua_State *L)
{
    void *dest = checkaddr(L, 1);
    void *src = checkaddr(L, 2);
    int size = luaL_checkint(L, 3);
    if (size < 0) luaL_argerror(L, 1, "negative size");
    memmove(dest, src, size);
    return 0;
}

static int internal_memcmp(lua_State *L)
{
    void *dest = checkaddr(L, 1);
    void *src = checkaddr(L, 2);
    int size = luaL_checkint(L, 3);
    if (size < 0) luaL_argerror(L, 1, "negative size");
    lua_pushinteger(L, memcmp(dest, src, size));
    return 1;
}

static int internal_memscan(lua_State *L)
{
    uint8_t *haystack = (uint8_t*)checkaddr(L, 1);
    int hcount = luaL_checkint(L, 2);
    int hstep = luaL_checkint(L, 3);
    if (hstep == 0) luaL_argerror(L, 3, "zero step");
    void *needle = checkaddr(L, 4);
    int nsize = luaL_checkint(L, 5);
    if (nsize < 0) luaL_argerror(L, 5, "negative size");

    for (int i = 0; i <= hcount; i++)
    {
        uint8_t *p = haystack + i*hstep;
        if (p + nsize > haystack + (hcount * hstep)) {
            break;
        }
        if (memcmp(p, needle, nsize) == 0) {
            lua_pushinteger(L, i);
            lua_pushinteger(L, (lua_Integer)p);
            return 2;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int internal_diffscan(lua_State *L)
{
    lua_settop(L, 8);
    void *old_data = checkaddr(L, 1);
    void *new_data = checkaddr(L, 2);
    int start_idx = luaL_checkint(L, 3);
    int end_idx = luaL_checkint(L, 4);
    int eltsize = luaL_checkint(L, 5);
    bool has_oldv = !lua_isnil(L, 6);
    bool has_newv = !lua_isnil(L, 7);
    bool has_diffv = !lua_isnil(L, 8);

#define LOOP(esz, etype) \
    case esz: {          \
        etype *pold = (etype*)old_data; \
        etype *pnew = (etype*)new_data; \
        etype oldv = (etype)luaL_optint(L, 6, 0); \
        etype newv = (etype)luaL_optint(L, 7, 0); \
        etype diffv = (etype)luaL_optint(L, 8, 0); \
        for (int i = start_idx; i < end_idx; i++) { \
            if (pold[i] == pnew[i]) continue; \
            if (has_oldv && pold[i] != oldv) continue; \
            if (has_newv && pnew[i] != newv) continue; \
            if (has_diffv && etype(pnew[i]-pold[i]) != diffv) continue; \
            lua_pushinteger(L, i); return 1; \
        } \
        break; \
    }
    switch (eltsize) {
        LOOP(1, uint8_t);
        LOOP(2, uint16_t);
        LOOP(4, uint32_t);
        default:
            luaL_argerror(L, 5, "invalid element size");
    }
#undef LOOP

    lua_pushnil(L);
    return 1;
}

static int internal_runCommand(lua_State *L)
{
    color_ostream *out = NULL;
    std::unique_ptr<buffered_color_ostream> out_buffer;
    command_result res;
    if (lua_gettop(L) == 0)
    {
        lua_pushstring(L, "");
    }
    int type_1 = lua_type(L, 1);
    bool use_console = lua_toboolean(L, 2);
    if (use_console)
    {
        out = Lua::GetOutput(L);
        if (!out)
        {
            out = &Core::getInstance().getConsole();
        }
    }
    else
    {
        out_buffer.reset(new buffered_color_ostream());
        out = out_buffer.get();
    }

    if (type_1 == LUA_TTABLE)
    {
        std::string command = "";
        std::vector<std::string> args;
        lua_pushnil(L);   // first key
        while (lua_next(L, 1) != 0)
        {
            if (command == "")
                command = lua_tostring(L, -1);
            else
                args.push_back(lua_tostring(L, -1));
            lua_pop(L, 1);  // remove value, leave key
        }
        CoreSuspender suspend;
        res = Core::getInstance().runCommand(*out, command, args);
    }
    else if (type_1 == LUA_TSTRING)
    {
        std::string command = lua_tostring(L, 1);
        CoreSuspender suspend;
        res = Core::getInstance().runCommand(*out, command);
    }
    else
    {
        lua_pushnil(L);
        lua_pushfstring(L, "Expected table, got %s", lua_typename(L, type_1));
        return 2;
    }

    lua_newtable(L);
    lua_pushinteger(L, (int)res);
    lua_setfield(L, -2, "status");

    if (out_buffer)
    {
        auto fragments = out_buffer->fragments();
        int i = 1;
        for (auto iter = fragments.begin(); iter != fragments.end(); iter++, i++)
        {
            int color = iter->first;
            std::string output = iter->second;
            lua_createtable(L, 2, 0);
            lua_pushinteger(L, color);
            lua_rawseti(L, -2, 1);
            lua_pushstring(L, output.c_str());
            lua_rawseti(L, -2, 2);
            lua_rawseti(L, -2, i);
        }
    }

    lua_pushvalue(L, -1);
    return 1;
}

static int internal_getModifiers(lua_State *L)
{
    int8_t modstate = Core::getInstance().getModstate();
    lua_newtable(L);
    lua_pushstring(L, "shift");
    lua_pushboolean(L, modstate & DFH_MOD_SHIFT);
    lua_settable(L, -3);
    lua_pushstring(L, "ctrl");
    lua_pushboolean(L, modstate & DFH_MOD_CTRL);
    lua_settable(L, -3);
    lua_pushstring(L, "alt");
    lua_pushboolean(L, modstate & DFH_MOD_ALT);
    lua_settable(L, -3);
    return 1;
}

static int internal_addScriptPath(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    bool search_before = (lua_gettop(L) > 1 && lua_toboolean(L, 2));
    lua_pushboolean(L, Core::getInstance().addScriptPath(path, search_before));
    return 1;
}

static int internal_removeScriptPath(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, Core::getInstance().removeScriptPath(path));
    return 1;
}

static int internal_getScriptPaths(lua_State *L)
{
    int i = 1;
    lua_newtable(L);
    std::vector<std::string> paths;
    Core::getInstance().getScriptPaths(&paths);
    for (auto it = paths.begin(); it != paths.end(); ++it)
    {
        lua_pushinteger(L, i++);
        lua_pushstring(L, it->c_str());
        lua_settable(L, -3);
    }
    return 1;
}

static int internal_findScript(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    std::string path = Core::getInstance().findScript(name);
    if (path.size())
        lua_pushstring(L, path.c_str());
    else
        lua_pushnil(L);
    return 1;
}

static int internal_listPlugins(lua_State *L)
{
    auto plugins = Core::getInstance().getPluginManager();

    int i = 1;
    lua_newtable(L);
    for (auto it = plugins->begin(); it != plugins->end(); ++it)
    {
        lua_pushinteger(L, i++);
        lua_pushstring(L, it->first.c_str());
        lua_settable(L, -3);
    }
    return 1;
}

static int internal_listCommands(lua_State *L)
{
    auto plugins = Core::getInstance().getPluginManager();

    const char *name = luaL_checkstring(L, 1);

    auto plugin = plugins->getPluginByName(name);
    if (!plugin)
    {
        lua_pushnil(L);
        return 1;
    }

    size_t num_commands = plugin->size();
    lua_newtable(L);
    for (size_t i = 0; i < num_commands; ++i)
    {
        lua_pushinteger(L, i + 1);
        lua_pushstring(L, (*plugin)[i].name.c_str());
        lua_settable(L, -3);
    }
    return 1;
}

static const PluginCommand * getPluginCommand(const char * command)
{
    auto plugins = Core::getInstance().getPluginManager();
    auto plugin = plugins->getPluginByCommand(command);
    if (!plugin)
    {
        return NULL;
    }

    size_t num_commands = plugin->size();
    for (size_t i = 0; i < num_commands; ++i)
    {
        if ((*plugin)[i].name == command)
            return &(*plugin)[i];
    }

    // not found (somehow)
    return NULL;
}

static int internal_getCommandHelp(lua_State *L)
{
    const PluginCommand *pc = getPluginCommand(luaL_checkstring(L, 1));
    if (!pc)
    {
        lua_pushnil(L);
        return 1;
    }

    std::string help = pc->description;
    if (help.size() && help[help.size()-1] != '.')
        help += ".";
    if (pc->usage.size())
        help += "\n" + pc->usage;
    lua_pushstring(L, help.c_str());
    return 1;
}

static int internal_getCommandDescription(lua_State *L)
{
    const PluginCommand *pc = getPluginCommand(luaL_checkstring(L, 1));
    if (!pc)
    {
        lua_pushnil(L);
        return 1;
    }

    std::string help = pc->description;
    if (help.size() && help[help.size()-1] != '.')
        help += ".";
    lua_pushstring(L, help.c_str());
    return 1;
}

static int internal_threadid(lua_State *L)
{
    std::stringstream ss;
    ss << tthread::this_thread::get_id();
    int i;
    ss >> i;
    lua_pushinteger(L, i);
    return 1;
}

static int internal_md5file(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    uint32_t len;
    char *first_kb_raw = nullptr;
    std::vector<char> first_kb;
    if (lua_toboolean(L, 2))
        first_kb_raw = new char[1024];

    std::string hash = md5_wrap.getHashFromFile(s, len, first_kb_raw);
    bool err = (hash.find("file") != std::string::npos);

    if (first_kb_raw)
    {
        first_kb.assign(first_kb_raw, first_kb_raw + 1024);
        delete[] first_kb_raw;
    }

    if (err)
    {
        lua_pushnil(L);
        lua_pushstring(L, hash.c_str());
        return 2;
    }
    else
    {
        lua_pushstring(L, hash.c_str());
        lua_pushinteger(L, len);
        if (!first_kb.empty())
        {
            Lua::PushVector(L, first_kb);
            return 3;
        }
        else
        {
            return 2;
        }
    }
}

static const luaL_Reg dfhack_internal_funcs[] = {
    { "getPE", internal_getPE },
    { "getMD5", internal_getmd5 },
    { "getAddress", internal_getAddress },
    { "setAddress", internal_setAddress },
    { "getVTable", internal_getVTable },

    { "adjustOffset", internal_adjustOffset },
    { "getMemRanges", internal_getMemRanges },
    { "patchMemory", internal_patchMemory },
    { "patchBytes", internal_patchBytes },
    { "memmove", internal_memmove },
    { "memcmp", internal_memcmp },
    { "memscan", internal_memscan },
    { "diffscan", internal_diffscan },
    { "getDir", filesystem_listdir },
    { "runCommand", internal_runCommand },
    { "getModifiers", internal_getModifiers },
    { "addScriptPath", internal_addScriptPath },
    { "removeScriptPath", internal_removeScriptPath },
    { "getScriptPaths", internal_getScriptPaths },
    { "findScript", internal_findScript },
    { "listPlugins", internal_listPlugins },
    { "listCommands", internal_listCommands },
    { "getCommandHelp", internal_getCommandHelp },
    { "getCommandDescription", internal_getCommandDescription },
    { "threadid", internal_threadid },
    { "md5File", internal_md5file },
    { NULL, NULL }
};


/************************
 *  Main Open function  *
 ************************/

void OpenDFHackApi(lua_State *state)
{
    OpenPersistent(state);
    OpenMatinfo(state);
    OpenPen(state);
    OpenPenArray(state);
    OpenRandom(state);

    LuaWrapper::SetFunctionWrappers(state, dfhack_module);
    luaL_setfuncs(state, dfhack_funcs, 0);
    OpenModule(state, "gui", dfhack_gui_module, dfhack_gui_funcs);
    OpenModule(state, "job", dfhack_job_module, dfhack_job_funcs);
    OpenModule(state, "textures", dfhack_textures_module);
    OpenModule(state, "units", dfhack_units_module, dfhack_units_funcs);
    OpenModule(state, "items", dfhack_items_module, dfhack_items_funcs);
    OpenModule(state, "maps", dfhack_maps_module, dfhack_maps_funcs);
    OpenModule(state, "world", dfhack_world_module, dfhack_world_funcs);
    OpenModule(state, "burrows", dfhack_burrows_module, dfhack_burrows_funcs);
    OpenModule(state, "buildings", dfhack_buildings_module, dfhack_buildings_funcs);
    OpenModule(state, "constructions", dfhack_constructions_module, dfhack_constructions_funcs);
    OpenModule(state, "screen", dfhack_screen_module, dfhack_screen_funcs);
    OpenModule(state, "filesystem", dfhack_filesystem_module, dfhack_filesystem_funcs);
    OpenModule(state, "designations", dfhack_designations_module, dfhack_designations_funcs);
    OpenModule(state, "kitchen", dfhack_kitchen_module);
    OpenModule(state, "console", dfhack_console_module);
    OpenModule(state, "internal", dfhack_internal_module, dfhack_internal_funcs);
}
