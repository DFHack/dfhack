//most of the code is shamelessly stolen from steam-engine.cpp
#include "Core.h"
#include "Error.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include "LuaTools.h"
#include <VTableInterpose.h>
#include "MiscUtils.h"

#include "df/building_doorst.h"
#include "df/building_workshopst.h"
#include "df/machine.h"
#include "df/machine_tile_set.h"
#include "df/power_info.h"
#include "df/world.h"
#include "df/buildings_other_id.h"
#include "df/coord.h"
#include "df/tile_building_occ.h"

#include <map>

using namespace DFHack;
using namespace df::enums;
using df::global::world;

DFHACK_PLUGIN("building-hacks");
struct workshop_hack_data
{
    int32_t myType;
    //machine stuff
    df::machine_tile_set connections;
    df::power_info powerInfo;
    //animation
    std::vector<std::vector<int> > frames;
    bool machine_timing; //with machine timing only 4 frames are possible
};
typedef std::map<int32_t,workshop_hack_data> workshops_data_t;
workshops_data_t hacked_workshops;


struct work_hook : df::building_workshopst{
    typedef df::building_workshopst interpose_base;

    workshop_hack_data* find_def()
    {
        if (type == workshop_type::Custom)
        {
            auto it=hacked_workshops.find(this->getCustomType());
            if(it!=hacked_workshops.end())
                return &(it->second);
        }
        return NULL;
    }
    DEFINE_VMETHOD_INTERPOSE(uint32_t,getImpassableOccupancy,())
    {
        if(find_def())
            return tile_building_occ::Impassable;
        return INTERPOSE_NEXT(getImpassableOccupancy)();
    }

    DEFINE_VMETHOD_INTERPOSE(void, getPowerInfo, (df::power_info *info))
    {
        if (auto def = find_def())
        {
            info->produced = def->powerInfo.produced;
            info->consumed = def->powerInfo.consumed;
            return;
        }

        INTERPOSE_NEXT(getPowerInfo)(info);
    }
    DEFINE_VMETHOD_INTERPOSE(df::machine_info*, getMachineInfo, ())
    {
        if (find_def())
            return &machine;

        return INTERPOSE_NEXT(getMachineInfo)();
    }
    DEFINE_VMETHOD_INTERPOSE(bool, isPowerSource, ())
    {
        workshop_hack_data* def=find_def();
        if (def && def->powerInfo.produced>0)
            return true;

        return INTERPOSE_NEXT(isPowerSource)();
    }
    DEFINE_VMETHOD_INTERPOSE(void, categorize, (bool free))
    {
        if (find_def())
        {
            auto &vec = world->buildings.other[buildings_other_id::ANY_MACHINE];
            insert_into_vector(vec, &df::building::id, (df::building*)this);
        }

        INTERPOSE_NEXT(categorize)(free);
    }

    DEFINE_VMETHOD_INTERPOSE(void, uncategorize, ())
    {
        if (find_def())
        {
            auto &vec = world->buildings.other[buildings_other_id::ANY_MACHINE];
            erase_from_vector(vec, &df::building::id, id);
        }

        INTERPOSE_NEXT(uncategorize)();
    }
    DEFINE_VMETHOD_INTERPOSE(bool, canConnectToMachine, (df::machine_tile_set *info))
    {
        if (auto def = find_def())
        {
            int real_cx = centerx, real_cy = centery;
            bool ok = false;

            for (size_t i = 0; i < def->connections.tiles.size(); i++)
            {
                // the original function connects to the center tile
                centerx = x1 + def->connections.tiles[i].x;
                centery = y1 + def->connections.tiles[i].y;

                if (!INTERPOSE_NEXT(canConnectToMachine)(info))
                    continue;

                ok = true;
                break;
            }

            centerx = real_cx; centery = real_cy;
            return ok;
        }
        else
            return INTERPOSE_NEXT(canConnectToMachine)(info);
    }
    DEFINE_VMETHOD_INTERPOSE(bool, isUnpowered, ())
    {
        if (auto def = find_def())
        {
            if(machine.machine_id==-1)
                return true;
            df::machine* target_machine=df::machine::find(machine.machine_id);
            if(target_machine && target_machine->flags.bits.active)
                return false;
            return true;
        }

        return INTERPOSE_NEXT(isUnpowered)();
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, getImpassableOccupancy);

IMPLEMENT_VMETHOD_INTERPOSE(work_hook, getPowerInfo);
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, getMachineInfo);
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, isPowerSource);
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, categorize);
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, uncategorize);
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, canConnectToMachine);
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, isUnpowered);
//IMPLEMENT_VMETHOD_INTERPOSE(work_hook, updateAction);
// todo animations...
//IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, drawBuilding);
void clear_mapping()
{
    hacked_workshops.clear();
}
//arguments: custom type, consumed power, produced power, list of connection points.
static int addBuilding(lua_State* L)
{
    workshop_hack_data newDefinition;
    newDefinition.myType=luaL_checkint(L,1);
    newDefinition.powerInfo.consumed=luaL_checkint(L,2);
    newDefinition.powerInfo.produced=luaL_checkint(L,3);
    luaL_checktype(L,4,LUA_TTABLE);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        lua_getfield(L,-1,"x");
        int x=lua_tonumber(L,-1);
        lua_pop(L,1);
        lua_getfield(L,-1,"y");
        int y=lua_tonumber(L,-1);
        lua_pop(L,1);

        newDefinition.connections.can_connect.push_back(-1);//TODO add this too...
        newDefinition.connections.tiles.push_back(df::coord(x,y,0));
        hacked_workshops[newDefinition.myType]=newDefinition;
        lua_pop(L,1);
    }
    lua_pop(L,1);
    return 0;
}
DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(addBuilding),
    DFHACK_LUA_END
};
static void enable_hooks(bool enable)
{
    INTERPOSE_HOOK(work_hook,getImpassableOccupancy).apply(enable);
    //machine part
    INTERPOSE_HOOK(work_hook,getPowerInfo).apply(enable);
    INTERPOSE_HOOK(work_hook,getMachineInfo).apply(enable);
    INTERPOSE_HOOK(work_hook,isPowerSource).apply(enable);
    INTERPOSE_HOOK(work_hook,categorize).apply(enable);
    INTERPOSE_HOOK(work_hook,uncategorize).apply(enable);
    INTERPOSE_HOOK(work_hook,canConnectToMachine).apply(enable);
    INTERPOSE_HOOK(work_hook,isUnpowered).apply(enable);
}
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_LOADED:
        enable_hooks(true);
        break;
    case SC_WORLD_UNLOADED:
        enable_hooks(false);
        clear_mapping();
        break;
    default:
        break;
    }

    return CR_OK;
}
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    enable_hooks(true);
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    plugin_onstatechange(out,SC_WORLD_UNLOADED);
    return CR_OK;
}
