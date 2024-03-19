//most of the code is shamelessly stolen from steam-engine.cpp
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
#include <limits>
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("building-hacks");
REQUIRE_GLOBAL(world);

constexpr uint32_t invalid_tile = std::numeric_limits<uint32_t>::max();
struct graphic_tile //could do just 31x31 and be done, but it's nicer to have flexible imho.
{
    int16_t tile=-1; //originally uint8_t but we need to indicate non-animated tiles
    int8_t fore;
    int8_t back;
    int8_t bright;
    //index of texpos
    uint32_t graphics_tile = invalid_tile;
    uint32_t overlay_tile = invalid_tile;
    uint32_t item_tile = invalid_tile;
    //only for first line
    uint32_t signpost_tile = invalid_tile;
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
                info->produced = ref->unk_1;
                info->consumed = ref->unk_2;
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
            ref->unk_1 = produced;
            ref->unk_2 = consumed;
        }
        else
        {
            ref = df::allocate<df::general_ref_creaturest>();
            ref->unk_1 = produced;
            ref->unk_2 = consumed;
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
        if (find_def())
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
                    color_ostream_proxy out(Core::getInstance().getConsole());
                    onUpdateAction(out,this);
                }
            }
        }
        INTERPOSE_NEXT(updateAction)();
    }
    DEFINE_VMETHOD_INTERPOSE(void, drawBuilding, (uint32_t curtick,df::building_drawbuffer *db, int16_t z_offset))
    {
        INTERPOSE_NEXT(drawBuilding)(curtick,db, z_offset);

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
            std::vector<graphic_tile> &cur_frame=def->frames[frame];
            for(size_t i=0;i<cur_frame.size();i++)
            {
                int tx = i % 31;
                int ty = i / 31;
                const auto& cf = cur_frame[i];
                if(cf.tile>=0)
                {
                    db->tile[tx][ty]= cf.tile;
                    db->back[tx][ty]= cf.back;
                    db->bright[tx][ty]= cf.bright;
                    db->fore[tx][ty]= cf.fore;
                }
                if (cf.graphics_tile != invalid_tile)
                    db->building_one_texpos[tx][ty] = cf.graphics_tile;
                if (cf.overlay_tile != invalid_tile)
                    db->building_two_texpos[tx][ty] = cf.overlay_tile;
                if (cf.item_tile != invalid_tile)
                    db->item_texpos[tx][ty] = cf.item_tile;
                //only first line has signpost graphics
                if (cf.item_tile != invalid_tile && ty==0)
                    db->signpost_texpos[tx] = cf.signpost_tile;
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
    const int max_idx = 31 * 31;

    luaL_checktype(L,stack_pos,LUA_TTABLE);

    int frame_index = 1;

    while (lua_geti(L,stack_pos,frame_index) != LUA_TNIL) { //get frame[i]
        luaL_checktype(L,-1,LUA_TTABLE); //ensure that it's a table
        std::vector<graphic_tile> frame(max_idx);

        for (int idx = 0; idx < max_idx; idx++)
        {
            auto& t = frame[idx];
            lua_geti(L, -1, idx); //get tile at idx i.e. frame[i][idx] where idx=x+y*31

            if (lua_isnil(L, -1))//allow sparse indexing
            {
                lua_pop(L, 1); //pop current tile (nil in this case)
                continue;
            }
            else
            {
                //load up tile, color, optionally graphics stuff
                lua_geti(L, -1, 1);
                //not sure why would anyone do nil tile, but for api consitency lets allow it
                t.tile= luaL_optinteger(L,-1,-1);
                lua_pop(L,1);

                lua_geti(L, -1, 2);
                t.fore= luaL_optinteger(L,-1,0);
                lua_pop(L,1);

                lua_geti(L, -1, 3);
                t.back= luaL_optinteger(L,-1,0);
                lua_pop(L,1);

                lua_geti(L, -1, 4);
                t.bright=luaL_optinteger(L,-1,0);
                lua_pop(L,1);

                lua_geti(L, -1, 5);
                t.graphics_tile = luaL_optinteger(L, -1,-1);
                lua_pop(L, 1);

                lua_geti(L, -1, 6);
                t.overlay_tile = luaL_optinteger(L, -1, -1);
                lua_pop(L, 1);

                lua_geti(L, -1, 7);
                t.signpost_tile = luaL_optinteger(L, -1, -1);
                lua_pop(L, 1);

                lua_geti(L, -1, 8);
                t.item_tile = luaL_optinteger(L, -1, -1);
                lua_pop(L, 1);

                lua_pop(L, 1); //pop current tile
            }
            frame.push_back(t);
        }
        def.frames.push_back(frame);
        frame_index++;
        lua_pop(L, 1); //pop current frame
    }

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
    if (ptr->find_def()) // check if it's really hacked workshop
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
    if (ptr->find_def()) // check if it's really hacked workshop
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
