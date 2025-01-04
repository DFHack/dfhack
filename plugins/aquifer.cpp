#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"
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

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot run %s without a loaded map.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.aquifer", "parse_commandline", std::make_tuple(parameters),
            1, [&](lua_State *L) {
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

static auto DEFAULT_PRE_BLOCK_FN = [](df::map_block *block){return block->flags.bits.has_aquifer;};

static void for_block(int minz, int maxz, const df::coord & pos1, const df::coord & pos2,
    std::function<void(const df::coord &)> pos_fn,
    std::function<bool(df::map_block *)> pre_block_fn = DEFAULT_PRE_BLOCK_FN)
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
        if (Maps::isTileAquifer(pos) && (!leaky || is_leaky(pos))) {
            if (Maps::isTileHeavyAquifer(pos))
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

    const bool all = aq_type == "all";
    const bool heavy_state = aq_type == "heavy";

    int modified = Maps::removeAreaAquifer(pos1, pos2, [&](df::coord pos, df::map_block* block) -> bool {
        TRACE(log, out).print("examining tile: pos=%d,%d,%d\n", pos.x, pos.y, pos.z);
        return Maps::isTileAquifer(pos)
            && (all || Maps::isTileHeavyAquifer(pos) == heavy_state)
            && (!leaky || is_leaky(pos));
        });

    DEBUG(log, out).print("drained aquifer tiles in area: pos1=%d,%d,%d, pos2=%d,%d,%d, heavy_state=%d, all=%d, count=%d\n",
        pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, heavy_state, all, modified);

    return modified;
}

static int aquifer_convert(color_ostream &out, string aq_type,
    df::coord pos1, df::coord pos2, int skip_top, int levels, bool leaky)
{
    DEBUG(log,out).print("entering aquifer_convert: aq_type=%s, pos1=%d,%d,%d, pos2=%d,%d,%d,"
        " skip_top=%d, levels=%d, leaky=%d\n", aq_type.c_str(),
        pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, skip_top, levels, leaky);

    const bool heavy_state = aq_type == "heavy";

    int modified = Maps::setAreaAquifer(pos1, pos2, heavy_state, [&](df::coord pos, df::map_block* block) -> bool {
        TRACE(log, out).print("examining tile: pos=%d,%d,%d\n", pos.x, pos.y, pos.z);
        return Maps::isTileAquifer(pos)
            && Maps::isTileHeavyAquifer(pos) != heavy_state
            && (!leaky || is_leaky(pos));
    });

    DEBUG(log, out).print("converted aquifer tiles in area: pos1=%d,%d,%d, pos2=%d,%d,%d, heavy_state=%d, count=%d\n",
        pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, heavy_state, modified);

    return modified;
}

static int aquifer_add(color_ostream &out, string aq_type,
    df::coord pos1, df::coord pos2, int skip_top, int levels, bool leaky)
{
    DEBUG(log,out).print("entering aquifer_add: aq_type=%s, pos1=%d,%d,%d, pos2=%d,%d,%d,"
        " skip_top=%d, levels=%d, leaky=%d\n", aq_type.c_str(),
        pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, skip_top, levels, leaky);

    const bool heavy_state = aq_type == "heavy";

    int modified = Maps::setAreaAquifer(pos1, pos2, heavy_state, [&](df::coord pos, df::map_block* block) -> bool {
        TRACE(log, out).print("examining tile: pos=%d,%d,%d\n", pos.x, pos.y, pos.z);
        return (leaky || !is_leaky(pos))
            && (!Maps::isTileAquifer(pos) || Maps::isTileHeavyAquifer(pos) != heavy_state);
        });

    DEBUG(log, out).print("added aquifer tiles in area: pos1=%d,%d,%d, pos2=%d,%d,%d, heavy_state=%d, count=%d\n",
        pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, heavy_state, modified);

    return modified;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(aquifer_list),
    DFHACK_LUA_FUNCTION(aquifer_drain),
    DFHACK_LUA_FUNCTION(aquifer_convert),
    DFHACK_LUA_FUNCTION(aquifer_add),
    DFHACK_LUA_END
};
