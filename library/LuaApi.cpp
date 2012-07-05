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
#include "modules/Items.h"
#include "modules/Materials.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Burrows.h"
#include "modules/Buildings.h"
#include "modules/Constructions.h"

#include "LuaWrapper.h"
#include "LuaTools.h"

#include "MiscUtils.h"

#include "df/job.h"
#include "df/job_item.h"
#include "df/building.h"
#include "df/unit.h"
#include "df/item.h"
#include "df/material.h"
#include "df/viewscreen.h"
#include "df/assumed_identity.h"
#include "df/nemesis_record.h"
#include "df/historical_figure.h"
#include "df/historical_entity.h"
#include "df/entity_position.h"
#include "df/entity_position_assignment.h"
#include "df/histfig_entity_link_positionst.h"
#include "df/plant_raw.h"
#include "df/creature_raw.h"
#include "df/inorganic_raw.h"
#include "df/dfhack_material_category.h"
#include "df/job_material_category.h"
#include "df/burrow.h"
#include "df/building_civzonest.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

using namespace DFHack;
using namespace DFHack::LuaWrapper;

void dfhack_printerr(lua_State *S, const std::string &str);

void Lua::Push(lua_State *state, const Units::NoblePosition &pos)
{
    lua_createtable(state, 0, 3);
    Lua::PushDFObject(state, pos.entity);
    lua_setfield(state, -2, "entity");
    Lua::PushDFObject(state, pos.assignment);
    lua_setfield(state, -2, "assignment");
    Lua::PushDFObject(state, pos.position);
    lua_setfield(state, -2, "position");
}

void Lua::Push(lua_State *state, df::coord pos)
{
    lua_createtable(state, 0, 3);
    lua_pushinteger(state, pos.x);
    lua_setfield(state, -2, "x");
    lua_pushinteger(state, pos.y);
    lua_setfield(state, -2, "y");
    lua_pushinteger(state, pos.z);
    lua_setfield(state, -2, "z");
}

void Lua::Push(lua_State *state, df::coord2d pos)
{
    lua_createtable(state, 0, 2);
    lua_pushinteger(state, pos.x);
    lua_setfield(state, -2, "x");
    lua_pushinteger(state, pos.y);
    lua_setfield(state, -2, "y");
}

int Lua::PushPosXYZ(lua_State *state, df::coord pos)
{
    if (!pos.isValid())
    {
        lua_pushnil(state);
        return 1;
    }
    else
    {
        lua_pushinteger(state, pos.x);
        lua_pushinteger(state, pos.y);
        lua_pushinteger(state, pos.z);
        return 3;
    }
}

int Lua::PushPosXY(lua_State *state, df::coord2d pos)
{
    if (!pos.isValid())
    {
        lua_pushnil(state);
        return 1;
    }
    else
    {
        lua_pushinteger(state, pos.x);
        lua_pushinteger(state, pos.y);
        return 2;
    }
}

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

/**************************************************
 * Per-world persistent configuration storage API *
 **************************************************/

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

    // Retrieve existing or create a new entry
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

    // Auto-add if not found
    if (!ref.isValid())
    {
        ref = Core::getInstance().getWorld()->AddPersistentData(str);
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

/************************
 * Material info lookup *
 ************************/

static void push_matinfo(lua_State *state, MaterialInfo &info)
{
    if (!info.isValid())
    {
        lua_pushnil(state);
        return;
    }

    lua_newtable(state);
    lua_pushvalue(state, lua_upvalueindex(1));
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

        for (int i = 1; i < argc; i++)
            tokens.push_back(luaL_checkstring(state, i));

        info.find(tokens);
    }

    push_matinfo(state, info);
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
    push_matinfo(state, info);
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
    luaL_setfuncs(state, dfhack_matinfo_funcs, 1);

    lua_dup(state);
    lua_setfield(state, -2, "__index");

    lua_pop(state, 1);
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

static std::string getDFVersion() { return Core::getInstance().vinfo->getVersion(); }

static std::string getDFPath() { return Core::getInstance().p->getPath(); }
static std::string getHackPath() { return Core::getInstance().getHackPath(); }

static bool isWorldLoaded() { return Core::getInstance().isWorldLoaded(); }
static bool isMapLoaded() { return Core::getInstance().isMapLoaded(); }

static const LuaWrapper::FunctionReg dfhack_module[] = {
    WRAP(getOSType),
    WRAP(getDFVersion),
    WRAP(getDFPath),
    WRAP(getHackPath),
    WRAP(isWorldLoaded),
    WRAP(isMapLoaded),
    WRAPM(Translation, TranslateName),
    { NULL, NULL }
};

/***** Gui module *****/

static const LuaWrapper::FunctionReg dfhack_gui_module[] = {
    WRAPM(Gui, getCurViewscreen),
    WRAPM(Gui, getFocusString),
    WRAPM(Gui, getSelectedWorkshopJob),
    WRAPM(Gui, getSelectedJob),
    WRAPM(Gui, getSelectedUnit),
    WRAPM(Gui, getSelectedItem),
    WRAPM(Gui, showAnnouncement),
    WRAPM(Gui, showPopupAnnouncement),
    { NULL, NULL }
};

/***** Job module *****/

static bool jobEqual(df::job *job1, df::job *job2) { return *job1 == *job2; }
static bool jobItemEqual(df::job_item *job1, df::job_item *job2) { return *job1 == *job2; }

static const LuaWrapper::FunctionReg dfhack_job_module[] = {
    WRAPM(Job,cloneJobStruct),
    WRAPM(Job,printItemDetails),
    WRAPM(Job,printJobDetails),
    WRAPM(Job,getHolder),
    WRAPM(Job,getWorker),
    WRAPM(Job,checkBuildingsNow),
    WRAPM(Job,checkDesignationsNow),
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

/***** Units module *****/

static const LuaWrapper::FunctionReg dfhack_units_module[] = {
    WRAPM(Units, getContainer),
    WRAPM(Units, setNickname),
    WRAPM(Units, getVisibleName),
    WRAPM(Units, getIdentity),
    WRAPM(Units, getNemesis),
    WRAPM(Units, isDead),
    WRAPM(Units, isAlive),
    WRAPM(Units, isSane),
    WRAPM(Units, isDwarf),
    WRAPM(Units, isCitizen),
    WRAPM(Units, getAge),
    WRAPM(Units, getProfessionName),
    WRAPM(Units, getCasteProfessionName),
    { NULL, NULL }
};

static int units_getPosition(lua_State *state)
{
    return Lua::PushPosXYZ(state, Units::getPosition(Lua::CheckDFObject<df::unit>(state,1)));
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

static const luaL_Reg dfhack_units_funcs[] = {
    { "getPosition", units_getPosition },
    { "getNoblePositions", units_getNoblePositions },
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

static bool items_moveToBuilding(df::item *item, df::building_actual *building, int use_mode)
{
    MapExtras::MapCache mc;
    return Items::moveToBuilding(mc, item, building,use_mode);
}

static bool items_moveToInventory
    (df::item *item, df::unit *unit, df::unit_inventory_item::T_mode mode, int body_part)
{
    MapExtras::MapCache mc;
    return Items::moveToInventory(mc, item, unit, mode, body_part);
}

static const LuaWrapper::FunctionReg dfhack_items_module[] = {
    WRAPM(Items, getGeneralRef),
    WRAPM(Items, getSpecificRef),
    WRAPM(Items, getOwner),
    WRAPM(Items, setOwner),
    WRAPM(Items, getContainer),
    WRAPM(Items, getDescription),
    WRAPN(moveToGround, items_moveToGround),
    WRAPN(moveToContainer, items_moveToContainer),
    WRAPN(moveToBuilding, items_moveToBuilding),
    WRAPN(moveToInventory, items_moveToInventory),
    { NULL, NULL }
};

static int items_getPosition(lua_State *state)
{
    return Lua::PushPosXYZ(state, Items::getPosition(Lua::CheckDFObject<df::item>(state,1)));
}

static int items_getContainedItems(lua_State *state)
{
    std::vector<df::item*> pvec;
    Items::getContainedItems(Lua::CheckDFObject<df::item>(state,1),&pvec);
    Lua::PushVector(state, pvec);
    return 1;
}

static const luaL_Reg dfhack_items_funcs[] = {
    { "getPosition", items_getPosition },
    { "getContainedItems", items_getContainedItems },
    { NULL, NULL }
};

/***** Maps module *****/

static const LuaWrapper::FunctionReg dfhack_maps_module[] = {
    WRAPN(getBlock, (df::map_block* (*)(int32_t,int32_t,int32_t))Maps::getBlock),
    WRAPM(Maps, enableBlockUpdates),
    WRAPM(Maps, getGlobalInitFeature),
    WRAPM(Maps, getLocalInitFeature),
    WRAPM(Maps, canWalkBetween),
    { NULL, NULL }
};

static int maps_getTileBlock(lua_State *L)
{
    auto pos = CheckCoordXYZ(L, 1, true);
    Lua::PushDFObject(L, Maps::getTileBlock(pos));
    return 1;
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
    Lua::PushPosXY(L, Maps::getTileBiomeRgn(pos));
    return 1;
}

static const luaL_Reg dfhack_maps_funcs[] = {
    { "getTileBlock", maps_getTileBlock },
    { "getRegionBiome", maps_getRegionBiome },
    { "getTileBiomeRgn", maps_getTileBiomeRgn },
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

static bool buildings_containsTile(df::building *bld, int x, int y, bool room) {
    return Buildings::containsTile(bld, df::coord2d(x,y), room);
}

static const LuaWrapper::FunctionReg dfhack_buildings_module[] = {
    WRAPM(Buildings, allocInstance),
    WRAPM(Buildings, checkFreeTiles),
    WRAPM(Buildings, countExtentTiles),
    WRAPN(containsTile, buildings_containsTile),
    WRAPM(Buildings, hasSupport),
    WRAPM(Buildings, constructAbstract),
    WRAPM(Buildings, constructWithItems),
    WRAPM(Buildings, constructWithFilters),
    WRAPM(Buildings, deconstruct),
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

static int buildings_setSize(lua_State *state)
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

static const luaL_Reg dfhack_buildings_funcs[] = {
    { "findAtTile", buildings_findAtTile },
    { "findCivzonesAt", buildings_findCivzonesAt },
    { "getCorrectSize", buildings_getCorrectSize },
    { "setSize", buildings_setSize },
    { NULL, NULL }
};

/***** Constructions module *****/

static const LuaWrapper::FunctionReg dfhack_constructions_module[] = {
    WRAPM(Constructions, designateNew),
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

static const luaL_Reg dfhack_constructions_funcs[] = {
    { "designateRemove", constructions_designateRemove },
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

static int getRebaseDelta() { return Core::getInstance().vinfo->getRebaseDelta(); }

static const LuaWrapper::FunctionReg dfhack_internal_module[] = {
    WRAP(getRebaseDelta),
    { NULL, NULL }
};

static int internal_getAddress(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    uint32_t addr = Core::getInstance().vinfo->getAddress(name);
    if (addr)
        lua_pushnumber(L, addr);
    else
        lua_pushnil(L);
    return 1;
}

static int internal_setAddress(lua_State *L)
{
    std::string name = luaL_checkstring(L, 1);
    uint32_t addr = (uint32_t)checkaddr(L, 2, true);
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
    uint32_t iaddr = addr - Core::getInstance().vinfo->getRebaseDelta();
    fprintf(stderr, "Setting global '%s' to %x (%x)\n", name.c_str(), addr, iaddr);
    fflush(stderr);

    return 1;
}

static int internal_getVTable(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    uint32_t addr = (uint32_t)Core::getInstance().vinfo->getVTable(name);
    if (addr)
        lua_pushnumber(L, addr);
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
        lua_pushnumber(L, (uint32_t)ranges[i].start);
        lua_setfield(L, -2, "start_addr");
        lua_pushnumber(L, (uint32_t)ranges[i].end);
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

static const luaL_Reg dfhack_internal_funcs[] = {
    { "getAddress", internal_getAddress },
    { "setAddress", internal_setAddress },
    { "getVTable", internal_getVTable },
    { "getMemRanges", internal_getMemRanges },
    { "memmove", internal_memmove },
    { "memcmp", internal_memcmp },
    { "memscan", internal_memscan },
    { "diffscan", internal_diffscan },
    { NULL, NULL }
};


/************************
 *  Main Open function  *
 ************************/

void OpenDFHackApi(lua_State *state)
{
    OpenPersistent(state);
    OpenMatinfo(state);

    LuaWrapper::SetFunctionWrappers(state, dfhack_module);
    OpenModule(state, "gui", dfhack_gui_module);
    OpenModule(state, "job", dfhack_job_module, dfhack_job_funcs);
    OpenModule(state, "units", dfhack_units_module, dfhack_units_funcs);
    OpenModule(state, "items", dfhack_items_module, dfhack_items_funcs);
    OpenModule(state, "maps", dfhack_maps_module, dfhack_maps_funcs);
    OpenModule(state, "burrows", dfhack_burrows_module, dfhack_burrows_funcs);
    OpenModule(state, "buildings", dfhack_buildings_module, dfhack_buildings_funcs);
    OpenModule(state, "constructions", dfhack_constructions_module);
    OpenModule(state, "internal", dfhack_internal_module, dfhack_internal_funcs);
}
