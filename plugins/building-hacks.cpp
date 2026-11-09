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
#include "df/building_def_workshopst.h"
#include "modules/Buildings.h"

#include <unordered_map>
#include <limits>
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("building-hacks");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

constexpr int32_t invalid_tile = 0;
struct graphic_tile //could do just 31x31 and be done, but it's nicer to have flexible imho.
{
    int16_t tile=-1; //originally uint8_t but we need to indicate non-animated tiles
    int8_t fore;
    int8_t back;
    int8_t bright;
    //index of texpos
    int32_t graphics_tile = invalid_tile;
    int32_t overlay_tile = invalid_tile;
    int32_t item_tile = invalid_tile;
    //only for first line
    int32_t signpost_tile = invalid_tile;
};
struct workshop_hack_data
{
    bool impassible_fix = false;
    //machine stuff
    bool is_machine = false;
    df::machine_tile_set connections;
    df::power_info powerInfo;
    bool needs_power;
    //animation
    std::vector<std::vector<graphic_tile> > frames;
    bool machine_timing=false; //6 frames used in vanilla
    int frame_skip; // e.g. 2 means have to ticks between frames
    //updateCallback:
    int skip_updates=0;
    int room_subset=-1; //0 no, 1 yes, -1 default
};
typedef std::unordered_map<int32_t,workshop_hack_data> workshops_data_t;
workshops_data_t hacked_workshops;

DEFINE_LUA_EVENT_NH_1(onUpdateAction,df::building_workshopst*);
DEFINE_LUA_EVENT_NH_2(onSetTriggerState,df::building_workshopst*,int32_t);

DFHACK_PLUGIN_LUA_EVENTS{
    DFHACK_LUA_EVENT(onUpdateAction),
    DFHACK_LUA_EVENT(onSetTriggerState),
    DFHACK_LUA_END
};

static void enable_hooks(bool enable);

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
            //try getting ref, if not return from definition
            if (ref)
            {
                info->produced = ref->race;
                info->consumed = ref->caste;
                return true;
            }
            else
            {
                info->produced = def->powerInfo.produced;
                info->consumed = def->powerInfo.consumed;
                return true;
            }
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
        //if we have a setting then update it, else create a new ref for dynamic power tracking
        if (ref)
        {
            ref->race = produced;
            ref->caste = consumed;
        }
        else
        {
            ref = df::allocate<df::general_ref_creaturest>();
            ref->race = produced;
            ref->caste = consumed;
            general_refs.push_back(ref);
        }
    }
    DEFINE_VMETHOD_INTERPOSE(df::tile_building_occ,getImpassableOccupancy,())
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
        auto def = find_def();
        if (def && def->is_machine)
        {
            df::power_info power;
            get_current_power(info);
            return;
        }
        INTERPOSE_NEXT(getPowerInfo)(info);
    }
    DEFINE_VMETHOD_INTERPOSE(df::machine_info*, getMachineInfo, ())
    {
        auto def = find_def();
        if (def && def->is_machine)
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
        /*
            there are two ways to enter this:
                a) we have plugin enabled and building added from script (thus def exists) and we are placing a new building
                b) we are loading a game thus buildings are not added from script yet
        */
        auto def = find_def();
        //in "b" case this ref is used as indicator to signal that script added this building last time before we saved
        df::general_ref_creaturest* ref = static_cast<df::general_ref_creaturest*>(DFHack::Buildings::getGeneralRef(this, general_ref_type::CREATURE));
        if( ref || (def && def->is_machine))
        {
            auto &vec = world->buildings.other[buildings_other_id::ANY_MACHINE];
            insert_into_vector(vec, &df::building::id, (df::building*)this);
            //in "a" case we add a ref and set it's values to signal later that we are a modified workshop
            if (!ref)
                set_current_power(def->powerInfo.produced, def->powerInfo.consumed);
        }

        INTERPOSE_NEXT(categorize)(free);
    }

    DEFINE_VMETHOD_INTERPOSE(void, uncategorize, ())
    {
        auto def = find_def();
        if (def && def->is_machine)
        {
            auto &vec = world->buildings.other[buildings_other_id::ANY_MACHINE];
            erase_from_vector(vec, &df::building::id, id);
        }

        INTERPOSE_NEXT(uncategorize)();
    }
    DEFINE_VMETHOD_INTERPOSE(bool, canConnectToMachine, (df::machine_tile_set *info))
    {
        auto def = find_def();
        if (def && def->is_machine)
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
    DEFINE_VMETHOD_INTERPOSE(void, setTriggerState,(int32_t state))
    {
        color_ostream_proxy out(Core::getInstance().getConsole());
        onSetTriggerState(out, this, state);
        INTERPOSE_NEXT(setTriggerState)(state); //pretty sure default workshop has nothing related to this, but to be future proof lets do it like this
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
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, setTriggerState);
IMPLEMENT_VMETHOD_INTERPOSE(work_hook, drawBuilding);


int get_workshop_type(lua_State* L,int arg)
{
    size_t len;
    int is_num;
    int type;
    type=lua_tointegerx(L, arg, &is_num);
    if (is_num)
    {
        return type;
    }
    auto str = lua_tolstring(L, arg, &len);
    if (len)
    {
        std::string lua_name(str, len);
        const auto& raws = world->raws.buildings.workshops;
        for (size_t i = 0; i < raws.size(); i++)
        {
            if (raws[i]->code == lua_name)
                return raws[i]->id;
        }
    }
    luaL_argerror(L, arg, "expected int or string workshop id");
    return 0;
}
void clear_mapping()
{
    hacked_workshops.clear();
}
static void load_graphics_tile(lua_State* L,graphic_tile& t)
{
    lua_getfield(L, -1, "ch");
    t.tile = luaL_optinteger(L, -1, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "fg");
    t.fore = luaL_optinteger(L, -1, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "bg");
    t.back = luaL_optinteger(L, -1, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "bold");
    t.bright = luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);

    lua_getfield(L, -1, "tile");
    t.graphics_tile = luaL_optinteger(L, -1, invalid_tile);
    lua_pop(L, 1);

    lua_getfield(L, -1, "tile_overlay");
    t.overlay_tile = luaL_optinteger(L, -1, invalid_tile);
    lua_pop(L, 1);

    lua_getfield(L, -1, "tile_signpost");
    t.signpost_tile = luaL_optinteger(L, -1, invalid_tile);
    lua_pop(L, 1);

    lua_getfield(L, -1, "tile_item");
    t.item_tile = luaL_optinteger(L, -1, invalid_tile);
    lua_pop(L, 1);
}
static void loadFrames(lua_State* L,workshop_hack_data& def,int stack_pos)
{
    const int max_side = 31;
    const int max_idx = max_side * max_side;

    luaL_checktype(L,stack_pos,LUA_TTABLE);

    int frame_index = 1;
    def.frames.clear();

    while (lua_geti(L,stack_pos,frame_index) != LUA_TNIL) { //get frame[i]
        luaL_checktype(L,-1,LUA_TTABLE); //ensure that it's a table
        std::vector<graphic_tile> frame(max_idx);
        for (int x = 1; x <= max_side; x++)
        {
            lua_geti(L, -1, x); //get row at x
            if (lua_isnil(L, -1))//allow sparse indexing
            {
                lua_pop(L, 1); //pop current tile (nil in this case)
                continue;
            }

            for (int y = 1; y <= max_side; y++)
            {
                lua_geti(L, -1, y); //get cell at y
                if (lua_isnil(L, -1))//allow sparse indexing
                {
                    lua_pop(L, 1); //pop current tile (nil in this case)
                    continue;
                }

                load_graphics_tile(L, frame[(x-1)+(y-1)*max_side]);

                lua_pop(L, 1); //pop current tile
            }
            lua_pop(L, 1); //pop current row
        }
        def.frames.push_back(frame);
        frame_index++;
        lua_pop(L, 1); //pop current frame
    }

    return;
}

//fixImpassible(workshop_type) - changes how impassible tiles work with liquids.
static int fixImpassible(lua_State* L)
{
    int workshop_type = get_workshop_type(L, 1);

    auto& def = hacked_workshops[workshop_type];
    def.impassible_fix = true;

    enable_hooks(true);

    return 0;
}
//setMachineInfo(workshop_type,bool needs_power,int power_consumed=0,int power_produced=0,table [x=int,y=int] connection_points) -setups and enables machine (i.e. connected to gears, and co) behaviour of the building
static int setMachineInfo(lua_State* L)
{
    int workshop_type = get_workshop_type(L, 1);
    auto& def = hacked_workshops[workshop_type];
    def.is_machine = true;

    def.needs_power = lua_toboolean(L, 2);
    def.powerInfo.consumed = luaL_optinteger(L, 3,0);
    def.powerInfo.produced = luaL_optinteger(L, 4,0);

    //table of machine connection points
    luaL_checktype(L, 5, LUA_TTABLE);
    lua_pushvalue(L, 5);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        lua_getfield(L, -1, "x");
        int x = lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "y");
        int y = lua_tonumber(L, -1);
        lua_pop(L, 1);

        df::machine_conn_modes modes;
        modes.whole = -1;
        def.connections.can_connect.push_back(modes);//TODO add this too...
        def.connections.tiles.push_back(df::coord(x, y, 0));

        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    enable_hooks(true);

    return 0;
}
//setUpdateSkip(workshop_type,int skip_frames) - skips frames to lower onupdate event call rate, 0 to disable
static int setUpdateSkip(lua_State* L)
{
    int workshop_type = get_workshop_type(L, 1);
    auto& def = hacked_workshops[workshop_type];

    def.skip_updates = luaL_optinteger(L, 2, 0);

    enable_hooks(true);

    return 0;
}
//setAnimationInfo(workshop_type,table frames, [frame_skip]) - define animation and it's timing. If frame_skip is not set or set to -1, it will use machine timing (i.e. like gears/axels etc)
static int setAnimationInfo(lua_State* L)
{
    int workshop_type = get_workshop_type(L, 1);
    auto& def = hacked_workshops[workshop_type];
    //animation
    loadFrames(L, def, 2);
    def.frame_skip = luaL_optinteger(L, 3, -1);
    if (def.frame_skip <= 0)
        def.machine_timing = true;
    else
        def.machine_timing = false;

    enable_hooks(true);

    return 0;
}
//setOwnableBuilding(workshop_type)
static int setOwnableBuilding(lua_State* L)
{
    int workshop_type = get_workshop_type(L, 1);

    auto& def = hacked_workshops[workshop_type];
    def.room_subset = true;

    enable_hooks(true);

    return 0;
}
static void setPower(df::building_workshopst* workshop, int power_produced, int power_consumed)
{
    work_hook* ptr = static_cast<work_hook*>(workshop);
    auto def = ptr->find_def();
    if (def && def->is_machine) // check if it's really hacked workshop
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
    auto def = ptr->find_def();
    if (def && def->is_machine) // check if it's really hacked workshop
    {
        df::power_info info;
        ptr->get_current_power(&info);
        lua_pushinteger(L, info.consumed);
        lua_pushinteger(L, info.produced);
        return 2;
    }
    return 0;
}
DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(setPower),
    DFHACK_LUA_END
};
DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(getPower),
    DFHACK_LUA_COMMAND(setOwnableBuilding),
    DFHACK_LUA_COMMAND(setAnimationInfo),
    DFHACK_LUA_COMMAND(setUpdateSkip),
    DFHACK_LUA_COMMAND(setMachineInfo),
    DFHACK_LUA_COMMAND(fixImpassible),
    DFHACK_LUA_END
};
static void enable_hooks(bool enable)
{
    if (is_enabled == enable)
        return;
    is_enabled = enable;

    INTERPOSE_HOOK(work_hook,getImpassableOccupancy).apply(enable);
    //machine part
    INTERPOSE_HOOK(work_hook,getPowerInfo).apply(enable);
    INTERPOSE_HOOK(work_hook,getMachineInfo).apply(enable);
    INTERPOSE_HOOK(work_hook,isPowerSource).apply(enable);

    INTERPOSE_HOOK(work_hook,canConnectToMachine).apply(enable);
    INTERPOSE_HOOK(work_hook,isUnpowered).apply(enable);
    INTERPOSE_HOOK(work_hook,canBeRoomSubset).apply(enable);
    //update n render
    INTERPOSE_HOOK(work_hook,updateAction).apply(enable);
    INTERPOSE_HOOK(work_hook,setTriggerState).apply(enable);
    INTERPOSE_HOOK(work_hook,drawBuilding).apply(enable);
}
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_UNLOADED:
        enable_hooks(false);
        clear_mapping();
        break;
    default:
        break;
    }

    return CR_OK;
}
//this is needed as all other methods depend on world being loaded but categorize happens when we are loading stuff in
static void init_categorize(bool enable)
{
    INTERPOSE_HOOK(work_hook, categorize).apply(enable);
    INTERPOSE_HOOK(work_hook, uncategorize).apply(enable);
}
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    enable_hooks(true);
    init_categorize(true);
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    plugin_onstatechange(out,SC_WORLD_UNLOADED);
    init_categorize(false);
    return CR_OK;
}
