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
#include "df/building_drawbuffer.h"
#include "df/general_ref_creaturest.h" // needed for power information storage
#include "modules/Buildings.h"

#include <map>

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("building-hacks");
REQUIRE_GLOBAL(world);

struct graphic_tile //could do just 31x31 and be done, but it's nicer to have flexible imho.
{
    int16_t tile; //originally uint8_t but we need to indicate non-animated tiles
    int8_t fore;
    int8_t back;
    int8_t bright;
};
struct workshop_hack_data
{
    int32_t myType;
    bool impassible_fix;
    //machine stuff
    df::machine_tile_set connections;
    df::power_info powerInfo;
    bool needs_power;
    //animation
    std::vector<std::vector<graphic_tile> > frames;
    bool machine_timing; //6 frames used in vanilla
    int frame_skip; // e.g. 2 means have to ticks between frames
    //updateCallback:
    int skip_updates;
    int room_subset; //0 no, 1 yes, -1 default
};
typedef std::map<int32_t,workshop_hack_data> workshops_data_t;
workshops_data_t hacked_workshops;

static void handle_update_action(color_ostream &out,df::building_workshopst*){};

DEFINE_LUA_EVENT_1(onUpdateAction,handle_update_action,df::building_workshopst*);
DFHACK_PLUGIN_LUA_EVENTS {
    DFHACK_LUA_EVENT(onUpdateAction),
    DFHACK_LUA_END
};

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
    inline bool is_fully_built()
    {
        return getBuildStage() >= getMaxBuildStage();
    }
    bool get_current_power(df::power_info* info)
    {
        if (workshop_hack_data* def = find_def())
        {
            df::general_ref_creaturest* ref = static_cast<df::general_ref_creaturest*>(DFHack::Buildings::getGeneralRef(this, general_ref_type::CREATURE));
            if (ref)
            {
                info->produced = ref->anon_1;
                info->consumed = ref->anon_2;
                return true;
            }
            else
            {
                info->produced = def->powerInfo.produced;
                info->consumed = def->powerInfo.consumed;
                return true;
            }
            //try getting ref, if not return from def
        }
        return false;
    }
    void set_current_power(int produced, int consumed)
    {
        if(machine.machine_id != -1) //if connected to machine, update the machine network production
        {
            df::machine* target_machine = df::machine::find(machine.machine_id);
            if (target_machine)
            {
                df::power_info old_power;
                get_current_power(&old_power);
                target_machine->min_power += consumed - old_power.consumed;
                target_machine->cur_power += produced - old_power.produced;
            }
        }
        df::general_ref_creaturest* ref = static_cast<df::general_ref_creaturest*>(DFHack::Buildings::getGeneralRef(this, general_ref_type::CREATURE));
        if (ref)
        {
            ref->anon_1 = produced;
            ref->anon_2 = consumed;
        }
        else
        {
            ref = df::allocate<df::general_ref_creaturest>();
            ref->anon_1 = produced;
            ref->anon_2 = consumed;
            general_refs.push_back(ref);
        }
    }
    DEFINE_VMETHOD_INTERPOSE(uint32_t,getImpassableOccupancy,())
    {
        if(auto def = find_def())
        {
            if(def->impassible_fix)
                return tile_building_occ::Impassable;
        }
        return INTERPOSE_NEXT(getImpassableOccupancy)();
    }

    DEFINE_VMETHOD_INTERPOSE(void, getPowerInfo, (df::power_info *info))
    {
        if (auto def = find_def())
        {
            df::power_info power;
            get_current_power(info);
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
        df::power_info power;
        get_current_power(&power);
        if (def && power.produced>0)
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
            if (!def->needs_power)
                return false;
            df::power_info power;
            get_current_power(&power);
            if (power.consumed == 0)
                return false;
            if(machine.machine_id==-1)
                return true;
            df::machine* target_machine=df::machine::find(machine.machine_id);
            if(target_machine && target_machine->flags.bits.active)
                return false;
            return true;
        }

        return INTERPOSE_NEXT(isUnpowered)();
    }
    DEFINE_VMETHOD_INTERPOSE(bool, canBeRoomSubset, ())
    {
        if(auto def = find_def())
        {
            if(def->room_subset==0)
                return false;
            if(def->room_subset==1)
                return true;
        }
        return INTERPOSE_NEXT(canBeRoomSubset)();
    }
    DEFINE_VMETHOD_INTERPOSE(void, updateAction, ())
    {
        if(auto def = find_def())
        {
            if(def->skip_updates!=0 && is_fully_built())
            {
                if(world->frame_counter % def->skip_updates == 0)
                {
                    CoreSuspendClaimer suspend;
                    color_ostream_proxy out(Core::getInstance().getConsole());
                    onUpdateAction(out,this);
                }
            }
        }
        INTERPOSE_NEXT(updateAction)();
    }
    DEFINE_VMETHOD_INTERPOSE(void, drawBuilding, (df::building_drawbuffer *db, int16_t unk))
    {
        INTERPOSE_NEXT(drawBuilding)(db, unk);

        if (auto def = find_def())
        {
            if (!is_fully_built() || def->frames.size()==0)
                return;
            int frame=0;
            if(!def->machine_timing)
            {
                int frame_mod=def->frames.size()* def->frame_skip;
                frame=(world->frame_counter % frame_mod)/def->frame_skip;
            }
            else
            {
                if(machine.machine_id!=-1)
                {
                    df::machine* target_machine=df::machine::find(machine.machine_id);
                    if(target_machine)
                    {
                        frame=target_machine->visual_phase % def->frames.size();
                    }
                }
            }
            int w=db->x2-db->x1+1;
            std::vector<graphic_tile> &cur_frame=def->frames[frame];
            for(int i=0;i<cur_frame.size();i++)
            {
                if(cur_frame[i].tile>=0)
                {
                    int tx=i % w;
                    int ty=i / w;
                    db->tile[tx][ty]=cur_frame[i].tile;
                    db->back[tx][ty]=cur_frame[i].back;
                    db->bright[tx][ty]=cur_frame[i].bright;
                    db->fore[tx][ty]=cur_frame[i].fore;
                }
            }
        }
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
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, canBeRoomSubset);
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, updateAction);
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, drawBuilding);



void clear_mapping()
{
    hacked_workshops.clear();
}
static void loadFrames(lua_State* L,workshop_hack_data& def,int stack_pos)
{
    luaL_checktype(L,stack_pos,LUA_TTABLE);
    lua_pushvalue(L,stack_pos);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        luaL_checktype(L,-1,LUA_TTABLE);
        lua_pushnil(L);
        std::vector<graphic_tile> frame;
        while (lua_next(L, -2) != 0) {
            graphic_tile t;
            lua_pushnumber(L,1);
            lua_gettable(L,-2);
            if(lua_isnil(L,-1))
            {
                t.tile=-1;
                lua_pop(L,1);
            }
            else
            {
                t.tile=lua_tonumber(L,-1);
                lua_pop(L,1);

                lua_pushnumber(L,2);
                lua_gettable(L,-2);
                t.fore=lua_tonumber(L,-1);
                lua_pop(L,1);

                lua_pushnumber(L,3);
                lua_gettable(L,-2);
                t.back=lua_tonumber(L,-1);
                lua_pop(L,1);

                lua_pushnumber(L,4);
                lua_gettable(L,-2);
                t.bright=lua_tonumber(L,-1);
                lua_pop(L,1);

            }
            frame.push_back(t);
            lua_pop(L,1);
        }
        lua_pop(L,1);
        def.frames.push_back(frame);
    }
    lua_pop(L,1);
    return ;
}
//arguments: custom type,impassible fix (bool), consumed power, produced power, list of connection points, update skip(0/nil to disable)
//          table of frames,frame to tick ratio (-1 for machine control)
static int addBuilding(lua_State* L)
{
    workshop_hack_data newDefinition;
    newDefinition.myType=luaL_checkint(L,1);
    newDefinition.impassible_fix=luaL_checkint(L,2);
    newDefinition.powerInfo.consumed=luaL_checkint(L,3);
    newDefinition.powerInfo.produced=luaL_checkint(L,4);
    newDefinition.needs_power = luaL_optinteger(L, 5, 1);
    //table of machine connection points
    luaL_checktype(L,6,LUA_TTABLE);
    lua_pushvalue(L,6);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        lua_getfield(L,-1,"x");
        int x=lua_tonumber(L,-1);
        lua_pop(L,1);
        lua_getfield(L,-1,"y");
        int y=lua_tonumber(L,-1);
        lua_pop(L,1);

        df::machine_conn_modes modes;
        modes.whole = -1;
        newDefinition.connections.can_connect.push_back(modes);//TODO add this too...
        newDefinition.connections.tiles.push_back(df::coord(x,y,0));

        lua_pop(L,1);
    }
    lua_pop(L,1);
    //updates
    newDefinition.skip_updates=luaL_optinteger(L,7,0);
    //animation
    if(!lua_isnil(L,8))
    {
        loadFrames(L,newDefinition,8);
        newDefinition.frame_skip=luaL_optinteger(L,9,-1);
        if(newDefinition.frame_skip==0)
            newDefinition.frame_skip=1;
        if(newDefinition.frame_skip<0)
            newDefinition.machine_timing=true;
        else
            newDefinition.machine_timing=false;
    }
    newDefinition.room_subset=luaL_optinteger(L,10,-1);
    hacked_workshops[newDefinition.myType]=newDefinition;
    return 0;
}
static void setPower(df::building_workshopst* workshop, int power_produced, int power_consumed)
{
    work_hook* ptr = static_cast<work_hook*>(workshop);
    if (workshop_hack_data* def = ptr->find_def())//check if it's really hacked workshop
    {
        ptr->set_current_power(power_produced, power_consumed);
    }
}
static int getPower(lua_State*L)
{
    auto workshop = Lua::CheckDFObject<df::building_workshopst>(L, 1);
    work_hook* ptr = static_cast<work_hook*>(workshop);
    if (!ptr)
        return 0;
    if (workshop_hack_data* def = ptr->find_def())//check if it's really hacked workshop
    {
        df::power_info info;
        ptr->get_current_power(&info);
        lua_pushinteger(L, info.produced);
        lua_pushinteger(L, info.consumed);
        return 2;
    }
    return 0;
}
DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(setPower),
    DFHACK_LUA_END
};
DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(addBuilding),
    DFHACK_LUA_COMMAND(getPower),
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
    INTERPOSE_HOOK(work_hook,canBeRoomSubset).apply(enable);
    //update n render
    INTERPOSE_HOOK(work_hook,updateAction).apply(enable);
    INTERPOSE_HOOK(work_hook,drawBuilding).apply(enable);
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
