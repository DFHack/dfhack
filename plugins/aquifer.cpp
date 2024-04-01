#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Maps.h"

#include "df/map_block.h"
#include "df/world.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("aquifer");

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(aquifer, log, DebugCategory::LINFO);
}

static command_result do_command(color_ostream &out, vector<string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        plugin_name,
        "Add, remove, or modify aquifers.",
        do_command));

    return CR_OK;
}

static bool call_aquifer_lua(color_ostream &out, const char *fn_name,
        int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA)
{
    DEBUG(log,out).print("calling aquifer lua function: '%s'\n", fn_name);

    CoreSuspender guard;

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    return Lua::CallLuaModuleFunction(out, L, "plugins.aquifer", fn_name,
            nargs, nres,
            std::forward<Lua::LuaLambda&&>(args_lambda),
            std::forward<Lua::LuaLambda&&>(res_lambda));
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot run %s without a loaded map.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!call_aquifer_lua(out, "parse_commandline", 1, 1,
            [&](lua_State *L) {
                Lua::PushVector(L, parameters);
            },
            [&](lua_State *L) {
                show_help = !lua_toboolean(L, 1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

////////////////////////////////////
// Lua API
//

bool is_open(int16_t x, int16_t y, int16_t z) {
    df::tiletype *tt = Maps::getTileType(x, y, z);
    if (!tt) return false;
    return ENUM_ATTR(tiletype_shape, passable_flow, tileShape(*tt));
}

bool is_leaky(int16_t x, int16_t y, int16_t z) {
        return is_open(x, y, z-1) ||
            is_open(x,   y-1, z) ||
            is_open(x-1, y,   z) ||
            is_open(x+1, y,   z) ||
            is_open(x,   y+1, z);
}

bool is_leaky(const df::coord & pos) {
    return is_leaky(pos.x, pos.y, pos.z);
}

static bool is_rough_natural_wall(int16_t x, int16_t y, int16_t z) {
    df::tiletype *tt = Maps::getTileType(x, y, z);
    if (!tt)
        return false;

    return tileShape(*tt) == df::tiletype_shape::WALL &&
        (tileMaterial(*tt) == df::tiletype_material::STONE ||
            tileMaterial(*tt) == df::tiletype_material::SOIL) &&
        tileSpecial(*tt) != df::tiletype_special::SMOOTH;
}

static bool is_rough_natural_wall(const df::coord & pos) {
    return is_rough_natural_wall(pos.x, pos.y, pos.z);
}

static bool is_aquifer(int16_t x, int16_t y, int16_t z, df::tile_designation *des = NULL) {
    if (!des)
        des = Maps::getTileDesignation(x, y, z);
    return des && des->bits.water_table;
}

static bool is_aquifer(const df::coord & pos, df::tile_designation *des = NULL) {
    return is_aquifer(pos.x, pos.y, pos.z, des);
}

static bool is_heavy_aquifer(int16_t x, int16_t y, int16_t z, df::tile_occupancy *occ = NULL) {
    if (!occ)
        occ = Maps::getTileOccupancy(x, y, z);
    return occ && occ->bits.heavy_aquifer;
}

static bool is_heavy_aquifer(const df::coord & pos, df::tile_occupancy *occ = NULL) {
    return is_heavy_aquifer(pos.x, pos.y, pos.z, occ);
}

static auto DEFAULT_PRE_BLOCK_FN = [](df::map_block *block){return block->flags.bits.has_aquifer;};

static void for_block(int minz, int maxz, const df::coord & pos1, const df::coord & pos2,
    std::function<void(const df::coord &)> pos_fn,
    std::function<bool(df::map_block *)> pre_block_fn = DEFAULT_PRE_BLOCK_FN,
    std::function<void(df::map_block *)> post_block_fn = [](df::map_block *){})
{
    for (auto block : world->map.map_blocks) {
        const df::coord & bpos = block->map_pos;
        if (maxz < bpos.z || minz > bpos.z ||
            bpos.x > pos2.x || bpos.x + 15 < pos1.x ||
            bpos.y > pos2.y || bpos.y + 15 < pos1.y)
        {
            continue;
        }

        if (!pre_block_fn(block))
            continue;

        for (int xoff = 0; xoff <= 15; ++xoff) {
            for (int yoff = 0; yoff <= 15; ++yoff) {
                df::coord pos = bpos + df::coord(xoff, yoff, 0);
                if (pos.x >= pos1.x && pos.x <= pos2.x && pos.y >= pos1.y && pos.y <= pos2.y)
                    pos_fn(pos);
            }
        }

        post_block_fn(block);
    }
}

static int get_max_ground_z(color_ostream &out, const df::coord &pos1, const df::coord &pos2)
{
    int maxz = -1;
    for_block(pos1.z, pos2.z, pos1, pos2, [&](const df::coord &pos){
        if (maxz < pos.z && is_rough_natural_wall(pos))
            maxz = pos.z;
    }, [&](df::map_block *block){
        return maxz < block->map_pos.z;
    });
    return maxz;
}

static int get_max_aq_z(color_ostream &out, const df::coord &pos1, const df::coord &pos2)
{
    int maxz = -1;
    for_block(pos1.z, pos2.z, pos1, pos2, [](const df::coord &pos){
    }, [&](df::map_block *block){
        if (block->flags.bits.has_aquifer && maxz < block->map_pos.z)
            maxz = block->map_pos.z;
        return false;
    });
    return maxz;
}

static void get_z_range(color_ostream &out, int & minz, int & maxz, const df::coord & pos1,
    const df::coord & pos2, int levels, bool top_is_aq = false, int skip_top = 0)
{
    DEBUG(log,out).print("get_z_range: top_is_aq=%d, skip_top=%d\n", top_is_aq, skip_top);

    if (!top_is_aq)
        maxz = get_max_ground_z(out, pos1, pos2) - skip_top;
    else
        maxz = get_max_aq_z(out, pos1, pos2) - skip_top;

    minz = std::max((int)pos1.z, maxz - levels + 1);

    DEBUG(log,out).print("calculated z range: minz=%d, maxz=%d\n", minz, maxz);
}

static void aquifer_list(color_ostream &out, df::coord pos1, df::coord pos2, int levels, bool leaky) {
    DEBUG(log,out).print("entering aquifer_list: pos1=%d,%d,%d, pos2=%d,%d,%d, levels=%d, leaky=%d\n",
        pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, levels, leaky);

    std::map<int, int, std::greater<int>> light_tiles, heavy_tiles;

    int minz = 0, maxz = -1;
    get_z_range(out, minz, maxz, pos1, pos2, levels);

    for_block(minz, maxz, pos1, pos2, [&](const df::coord &pos){
        if (is_aquifer(pos) && (!leaky || is_leaky(pos))) {
            if (is_heavy_aquifer(pos))
                ++heavy_tiles[pos.z];
            else
                ++light_tiles[pos.z];
        }
    });

    if (light_tiles.empty() && heavy_tiles.empty()) {
        out.print("No %saquifer tiles in the specified range.\n", leaky ? "leaking " : "");
    } else {
        int elev_off = world->map.region_z - 100;
        for (int z = maxz; z >= minz; --z) {
            int lcount = light_tiles.contains(z) ? light_tiles[z] : 0;
            int hcount = heavy_tiles.contains(z) ? heavy_tiles[z] : 0;
            if (lcount || hcount)
                out.print("z-level %3d (elevation %4d) has %6d %slight aquifer tile(s) and %6d %sheavy aquifer tile(s)\n",
                    z, z+elev_off, lcount, leaky ? "leaking " : "", hcount, leaky ? "leaking " : "");
        }
    }
}

static int aquifer_drain(color_ostream &out, string aq_type,
    df::coord pos1, df::coord pos2, int skip_top, int levels, bool leaky)
{
    DEBUG(log,out).print("entering aquifer_drain: aq_type=%s, pos1=%d,%d,%d, pos2=%d,%d,%d,"
        " skip_top=%d, levels=%d, leaky=%d\n", aq_type.c_str(),
        pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, skip_top, levels, leaky);

    int minz = 0, maxz = -1;
    get_z_range(out, minz, maxz, pos1, pos2, levels, true, skip_top);

    const bool all = aq_type == "all";
    const bool heavy_state = aq_type == "heavy";

    int modified = 0;
    for_block(minz, maxz, pos1, pos2, [&](const df::coord & pos){
        TRACE(log,out).print("examining tile: pos=%d,%d,%d\n", pos.x, pos.y, pos.z);
        auto des = Maps::getTileDesignation(pos);
        if (!is_aquifer(pos, des))
            return;
        if (!all && is_heavy_aquifer(pos) != heavy_state)
            return;
        if (leaky && !is_leaky(pos))
            return;
        des->bits.water_table = false;
        DEBUG(log,out).print("drained aquifer tile: pos=%d,%d,%d\n", pos.x, pos.y, pos.z);
        ++modified;
    },
    DEFAULT_PRE_BLOCK_FN,
    [](df::map_block *block){
        const df::coord & bpos = block->map_pos;
        for (int xoff = 0; xoff <= 15; ++xoff)
            for (int yoff = 0; yoff <= 15; ++yoff)
                if (is_aquifer(bpos + df::coord(xoff, yoff, 0)))
                    return;

        block->flags.bits.has_aquifer = false;
        block->flags.bits.check_aquifer = false;
    });

    return modified;
}

static int aquifer_convert(color_ostream &out, string aq_type,
    df::coord pos1, df::coord pos2, int skip_top, int levels, bool leaky)
{
    DEBUG(log,out).print("entering aquifer_convert: aq_type=%s, pos1=%d,%d,%d, pos2=%d,%d,%d,"
        " skip_top=%d, levels=%d, leaky=%d\n", aq_type.c_str(),
        pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, skip_top, levels, leaky);

    const bool heavy_state = aq_type == "heavy";

    int minz = 0, maxz = -1;
    get_z_range(out, minz, maxz, pos1, pos2, levels, true, skip_top);

    int modified = 0;
    for_block(minz, maxz, pos1, pos2, [&](const df::coord & pos){
        TRACE(log,out).print("examining tile: pos=%d,%d,%d\n", pos.x, pos.y, pos.z);
        auto des = Maps::getTileDesignation(pos);
        if (!is_aquifer(pos, des))
            return;
        auto occ = Maps::getTileOccupancy(pos);
        if (is_heavy_aquifer(pos, occ) == heavy_state)
            return;
        if (leaky && !is_leaky(pos))
            return;
        occ->bits.heavy_aquifer = heavy_state;
        DEBUG(log,out).print("converted aquifer tile: pos=%d,%d,%d, heavy_state=%d\n",
                pos.x, pos.y, pos.z, heavy_state);
        ++modified;
    });

    return modified;
}

static int aquifer_add(color_ostream &out, string aq_type,
    df::coord pos1, df::coord pos2, int skip_top, int levels, bool leaky)
{
    DEBUG(log,out).print("entering aquifer_add: aq_type=%s, pos1=%d,%d,%d, pos2=%d,%d,%d,"
        " skip_top=%d, levels=%d, leaky=%d\n", aq_type.c_str(),
        pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, skip_top, levels, leaky);

    const bool heavy_state = aq_type == "heavy";

    int minz = 0, maxz = -1;
    get_z_range(out, minz, maxz, pos1, pos2, levels, false, skip_top);

    int modified = 0;
    bool added = false;
    for_block(minz, maxz, pos1, pos2, [&](const df::coord & pos){
        TRACE(log,out).print("examining tile: pos=%d,%d,%d\n", pos.x, pos.y, pos.z);
        if (!leaky && is_leaky(pos))
            return;
        auto des = Maps::getTileDesignation(pos);
        bool changed = !is_aquifer(pos, des);
        des->bits.water_table = true;
        auto occ = Maps::getTileOccupancy(pos);
        changed = changed || is_heavy_aquifer(pos, occ) != heavy_state;
        occ->bits.heavy_aquifer = heavy_state;
        if (changed) {
            DEBUG(log,out).print("added aquifer tile: pos=%d,%d,%d, heavy_state=%d\n",
                pos.x, pos.y, pos.z, heavy_state);
            ++modified;
        } else {
            DEBUG(log,out).print("tile already in target state: pos=%d,%d,%d, heavy_state=%d\n",
                pos.x, pos.y, pos.z, heavy_state);
        }
        added = true;
    }, [&](df::map_block *block){
        added = false;
        return true;
    }, [&](df::map_block *block){
        if (added) {
            block->flags.bits.has_aquifer = true;
            block->flags.bits.check_aquifer = true;
            block->flags.bits.update_liquid = true;
            block->flags.bits.update_liquid_twice = true;
        }
    });

    return modified;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(aquifer_list),
    DFHACK_LUA_FUNCTION(aquifer_drain),
    DFHACK_LUA_FUNCTION(aquifer_convert),
    DFHACK_LUA_FUNCTION(aquifer_add),
    DFHACK_LUA_END
};
