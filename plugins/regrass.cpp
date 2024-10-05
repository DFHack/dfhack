// Regrow surface grass and cavern moss.

#include "DataDefs.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Gui.h"
#include "modules/Maps.h"

#include "df/biome_type.h"
#include "df/block_square_event_grassst.h"
#include "df/block_square_event_material_spatterst.h"
#include "df/builtin_mats.h"
#include "df/map_block.h"
#include "df/map_block_column.h"
#include "df/plant.h"
#include "df/plant_raw.h"
#include "df/region_map_entry.h"
#include "df/world.h"

using std::string;
using std::vector;
using namespace DFHack;

DFHACK_PLUGIN("regrass");
REQUIRE_GLOBAL(world);

namespace DFHack
{
    DBG_DECLARE(regrass, log, DebugCategory::LINFO);
}

struct regrass_options {
    bool max_grass = false; // Fill all tile grass events.
    bool new_grass = false; // Add new valid grass events.
    bool force = false; // Force a grass regardless of no_grow and biome.
    bool ashes = false; // Regrass ashes.
    bool buildings = false; // Regrass under stockpiles and passable buildings.
    bool mud = false; // Regrass muddy stone.
    bool block = false; // Operate on single map block.
    bool zlevel = false; // Operate on entire z-levels.

    int32_t forced_plant = -1; // Plant raw index of grass to force; -2 means print all IDs.

    static struct_identity _identity;
};
static const struct_field_info regrass_options_fields[] =
{
    { struct_field_info::PRIMITIVE, "max_grass",    offsetof(regrass_options, max_grass),    &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "new_grass",    offsetof(regrass_options, new_grass),    &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "force",        offsetof(regrass_options, force),        &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "ashes",        offsetof(regrass_options, ashes),        &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "buildings",    offsetof(regrass_options, buildings),    &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "mud",          offsetof(regrass_options, mud),          &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "block",        offsetof(regrass_options, block),        &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "zlevel",       offsetof(regrass_options, zlevel),       &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "forced_plant", offsetof(regrass_options, forced_plant), &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::END }
};
struct_identity regrass_options::_identity(sizeof(regrass_options), &df::allocator_fn<regrass_options>, NULL, "regrass_options", NULL, regrass_options_fields);

command_result df_regrass(color_ostream &out, vector<string> &parameters);

static bool valid_tile(color_ostream &out, regrass_options options, df::map_block *block, int tx, int ty)
{   // Is valid tile for regrass.
    auto des = block->designation[tx][ty];
    auto tt = block->tiletype[tx][ty];
    auto shape = tileShape(tt);
    auto mat = tileMaterial(tt);
    auto spec = tileSpecial(tt);

    if (mat == tiletype_material::GRASS_LIGHT ||
        mat == tiletype_material::GRASS_DARK ||
        mat == tiletype_material::PLANT) // Shrubs and saplings can have grass underneath.
    {   // Refill existing grass.
        TRACE(log, out).print("Valid tile: Grass/Shrub/Sapling\n");
        return true;
    }
    else if (tt == tiletype::TreeTrunkPillar || tt == tiletype::TreeTrunkInterior ||
        (tt >= tiletype::TreeTrunkThickN && tt <= tiletype::TreeTrunkThickSE))
    {   // Trees can have grass for ground level tiles.
        auto p = df::coord(block->map_pos.x + tx, block->map_pos.y + ty, block->map_pos.z);
        auto plant = Maps::getPlantAtTile(p);
        if (plant && plant->pos.z == p.z)
        {
            TRACE(log, out).print("Valid tile: Tree\n");
            return true; // Ground tile.
        }

        TRACE(log, out).print("Invalid tile: Tree\n");
        return false; // Not ground tile.
    }
    else if (des.bits.flow_size > (des.bits.liquid_type == tile_liquid::Magma ? 0U : 3U))
    {   // Under water/magma (df::plant_raw::shrub_drown_level is usually 4).
        TRACE(log, out).print("Invalid tile: Liquid\n");
        return false;
    }
    else if (shape != tiletype_shape::FLOOR &&
        shape != tiletype_shape::RAMP &&
        shape != tiletype_shape::STAIR_UP &&
        shape != tiletype_shape::STAIR_DOWN &&
        shape != tiletype_shape::STAIR_UPDOWN)
    {
        TRACE(log, out).print("Invalid tile: Shape\n");
        return false;
    }
    else if (block->occupancy[tx][ty].bits.building >
        (options.buildings ? tile_building_occ::Passable : tile_building_occ::None))
    {   // Avoid stockpiles and planned/passable buildings unless enabled.
        TRACE(log, out).print("Invalid tile: Building (%s)\n",
            ENUM_KEY_STR(tile_building_occ, block->occupancy[tx][ty].bits.building).c_str());
        return false;
    }
    else if (!options.force && block->occupancy[tx][ty].bits.no_grow) {
        TRACE(log, out).print("Invalid tile: no_grow\n");
        return false;
    }
    else if (mat == tiletype_material::SOIL) {
        if (spec == tiletype_special::FURROWED || spec == tiletype_special::WET)
        {   // Dirt road or beach.
            TRACE(log, out).print("Invalid tile: Furrowed/Wet\n");
            return false;
        }
        TRACE(log, out).print("Valid tile: Soil\n");
        return true;
    }
    else if (options.ashes && mat == tiletype_material::ASHES) {
        TRACE(log, out).print("Valid tile: Ashes\n");
        return true;
    }
    else if (options.mud)
    {
        if (spec == tiletype_special::SMOOTH || spec == tiletype_special::TRACK)
        {   // Don't replace smoothed stone.
            TRACE(log, out).print("Invalid tile: Smooth (mud check)\n");
            return false;
        }
        else if (mat != tiletype_material::STONE &&
            mat != tiletype_material::FEATURE &&
            mat != tiletype_material::LAVA_STONE &&
            mat != tiletype_material::MINERAL)
        {   // Not any stone.
            TRACE(log, out).print("Invalid tile: Wrong tile mat (mud check)\n");
            return false;
        }

        for (auto blev : block->block_events)
        {   // Look for mud on tile. Note: DF doesn't allow for surface grass, but we do.
            auto ms_ev = virtual_cast<df::block_square_event_material_spatterst>(blev);
            if (ms_ev && ms_ev->mat_type == builtin_mats::MUD) {
                if (ms_ev->amount[tx][ty] > 0) {
                    TRACE(log, out).print("Valid tile: Muddy stone\n");
                    return true;
                }
                else {
                    TRACE(log, out).print("Invalid tile: Non-muddy stone\n");
                    return false;
                }
            }
        }
    }
    TRACE(log, out).print("Invalid tile: No success\n");
    return false;
}

static void grasses_for_tile(color_ostream &out, vector<int32_t> &vec, df::map_block *block, int tx, int ty)
{   // Fill vector with sorted valid grass IDs.
    vec.clear();
    if (block->occupancy[tx][ty].bits.no_grow) {
        TRACE(log, out).print("Skipping grass collection: no_grow\n");
        return;
    }
    // TODO: Use world.area_grasses.layer_grasses[] if not options.new_grass?
    TRACE(log, out).print("Collecting grasses...\n");
    if (block->designation[tx][ty].bits.subterranean) {
        for (auto p_raw : world->raws.plants.grasses)
        {   // Sorted by df::plant_raw::index
            if (p_raw->flags.is_set(plant_raw_flags::BIOME_SUBTERRANEAN_WATER)) {
                TRACE(log, out).print("Cave grass: %s\n", p_raw->id.c_str());
                vec.push_back(p_raw->index);
            }
        }
    }
    else
    {   // Surface
        auto rgn_pos = Maps::getBlockTileBiomeRgn(block, df::coord2d(tx, ty));
        if (!rgn_pos.isValid())
        {   // No biome (happens in sky).
            TRACE(log, out).print("No grass: No biome region!\n");
            return;
        }

        auto &biome_info = *Maps::getRegionBiome(rgn_pos); // TODO: Cache grasses based on region?
        auto plant_biome_flag = ENUM_ATTR(biome_type, plant_raw_flag, Maps::getBiomeType(rgn_pos.x, rgn_pos.y));

        bool good = (biome_info.evilness < 33);
        bool evil = (biome_info.evilness > 65);
        bool savage = (biome_info.savagery > 65);

        for (auto p_raw : world->raws.plants.grasses)
        {   // Sorted by df::plant_raw::index
            auto &flags = p_raw->flags;
            if (flags.is_set(plant_biome_flag) &&
                (good || !flags.is_set(plant_raw_flags::GOOD)) &&
                (evil || !flags.is_set(plant_raw_flags::EVIL)) &&
                (savage || !flags.is_set(plant_raw_flags::SAVAGE)))
            {
                TRACE(log, out).print("Surface grass: %s\n", p_raw->id.c_str());
                vec.push_back(p_raw->index);
            }
        }
    }
    TRACE(log, out).print("Done collecting grasses.\n");
}

static bool regrass_events(color_ostream &out, const regrass_options &options, df::map_block *block, int tx, int ty)
{   // Utility fn to modify grass block events.
    if (!valid_tile(out, options, block, tx, ty)) {
        DEBUG(log, out).print("Skipping invalid tile.\n");
        return false;
    }
    // Gather grass events for consideration.
    vector<df::block_square_event_grassst *> consider_grev;
    for (auto blev : block->block_events)
        if (auto gr_ev = virtual_cast<df::block_square_event_grassst>(blev))
            consider_grev.push_back(gr_ev);

    df::block_square_event_grassst *forced_grev = NULL;
    bool success = false; // Determine if max regrass worked.

    for (auto gr_ev : consider_grev)
    {   // Try to refill existing events.
        if (options.max_grass)
        {   // Refill all.
            gr_ev->amount[tx][ty] = 100;
            success = true;
        }
        else if (gr_ev->amount[tx][ty] > 0)
        {   // Refill first non-zero grass.
            DEBUG(log, out).print("Refilling existing grass (ID %d).\n", gr_ev->plant_index);
            gr_ev->amount[tx][ty] = 100;
            return true;
        }

        if (options.force && gr_ev->plant_index == options.forced_plant)
            forced_grev = gr_ev; // Avoid allocating a new event.
    }
    if (options.force)
        block->occupancy[tx][ty].bits.no_grow = false;

    vector<int32_t> valid_grasses;
    if (options.new_grass || !options.max_grass) // Find out what belongs.
        grasses_for_tile(out, valid_grasses, block, tx, ty);

    if (options.new_grass && !valid_grasses.empty()) {
        TRACE(log, out).print("Handling new grasses...\n");
        auto new_grasses(valid_grasses); // Copy vector.

        for (auto gr_ev : consider_grev)
        {   // These grass events are already present.
            if (erase_from_vector(new_grasses, gr_ev->plant_index)) {
                TRACE(log, out).print("Grass (ID %d) already present.\n", gr_ev->plant_index);
            }
        }

        for (auto id : new_grasses) {
            DEBUG(log, out).print("Allocating new grass event (ID %d).\n", id);
            auto gr_ev = df::allocate<df::block_square_event_grassst>();
            if (!gr_ev) {
                out.printerr("Failed to allocate new grass event! Aborting loop.\n");
                break;
            }
            gr_ev->plant_index = id;
            block->block_events.push_back(gr_ev);

            if (options.max_grass)
            {   // Initialize at max.
                gr_ev->amount[tx][ty] = 100;
                success = true;
            }
            else // Add as random choice.
                consider_grev.push_back(gr_ev);
        }
    }

    if (!options.max_grass) {
        TRACE(log, out).print("Ruling out invalid grasses for random select...\n");
        if (valid_grasses.empty()) {
            TRACE(log, out).print("No valid grasses. Remove all.\n");
            consider_grev.clear();
        }

        for (size_t i = consider_grev.size(); i-- > 0;)
        {   // Iterate backwards and remove invalid events from consideration.
            if (!vector_contains(valid_grasses, consider_grev[i]->plant_index)) {
                TRACE(log, out).print("Removing grass (ID %d) from consideration.\n",
                    consider_grev[i]->plant_index);
                consider_grev.erase(consider_grev.begin() + i);
            }
        }
    }

    if (options.force && !success && (options.max_grass || consider_grev.empty())) {
        TRACE(log, out).print("Handling forced grass...\n");
        if (forced_grev) {
            DEBUG(log, out).print("Forced regrass with existing event.\n");
            forced_grev->amount[tx][ty] = 100;
            return true;
        }

        DEBUG(log, out).print("Allocating new forced grass event.\n");
        forced_grev = df::allocate<df::block_square_event_grassst>();
        if (forced_grev)
        {   // Initialize it.
            forced_grev->plant_index = options.forced_plant;
            forced_grev->amount[tx][ty] = 100;
            block->block_events.push_back(forced_grev);
            return true;
        }
        out.printerr("Failed to allocate forced grass event!\n");
        return false;
    }

    if (options.max_grass)
    {   // Don't need random choice.
        DEBUG(log, out).print("Done with max regrass, %s.\n",
            success ? "success" : "failure");
        return success;
    }
    TRACE(log, out).print("Selecting random valid grass event...\n");

    if (auto rand_grev = vector_get_random(consider_grev)) {
        DEBUG(log, out).print("Refilling random grass (ID %d).\n", rand_grev->plant_index);
        rand_grev->amount[tx][ty] = 100;
        return true;
    }
    DEBUG(log, out).print("No existing valid grass event to select.\n");
    return false;
}

int regrass_tile(color_ostream &out, const regrass_options &options, df::map_block *block, int tx, int ty)
{   // Regrass single tile. Return 1 if tile success, else 0.
    CHECK_NULL_POINTER(block);
    if (!is_valid_tile_coord(df::coord2d(tx, ty))) {
        out.printerr("(%d, %d) not in range 0-15!\n", tx, ty);
        return 0;
    }

    DEBUG(log, out).print("Regrass tile (%d, %d, %d)\n",
        block->map_pos.x + tx, block->map_pos.y + ty, block->map_pos.z);
    if (!regrass_events(out, options, block, tx, ty))
        return 0;

    auto tt = block->tiletype[tx][ty];
    auto mat = tileMaterial(tt);
    auto shape = tileShape(tt);

    if (mat == tiletype_material::GRASS_LIGHT ||
        mat == tiletype_material::GRASS_DARK ||
        mat == tiletype_material::PLANT ||
        mat == tiletype_material::TREE)
    {   // Already appropriate tile.
        TRACE(log, out).print("Tiletype no change.\n");
        return 1;
    }

    if (shape == tiletype_shape::FLOOR)
    {   // Floor (including ashes).
        DEBUG(log, out).print("Tiletype to random grass floor.\n");
        block->tiletype[tx][ty] = findRandomVariant((rand() & 1) ? tiletype::GrassLightFloor1 : tiletype::GrassDarkFloor1);
    }
    else
    {   // Ramp or stairs.
        auto new_mat = (rand() & 1) ? tiletype_material::GRASS_LIGHT : tiletype_material::GRASS_DARK;
        auto new_tt = findTileType(shape, new_mat, tiletype_variant::NONE, tiletype_special::NONE, nullptr);
        DEBUG(log, out).print("Tiletype to %s.\n", ENUM_KEY_STR(tiletype, new_tt).c_str());
        block->tiletype[tx][ty] = new_tt;
    }
    return 1;
}

int regrass_cuboid(color_ostream &out, const regrass_options &options, const cuboid &bounds)
{   // Regrass all tiles in the defined cuboid.
    DEBUG(log, out).print("Regrassing cuboid (%d:%d, %d:%d, %d:%d), clipped to map.\n",
        bounds.x_min, bounds.x_max, bounds.y_min, bounds.y_max, bounds.z_min, bounds.z_max);

    int count = 0;
    bounds.forBlock([&](df::map_block *block, cuboid intersect) {
        DEBUG(log, out).print("Cuboid regrass block at (%d, %d, %d)\n",
            block->map_pos.x, block->map_pos.y, block->map_pos.z);
        intersect.forCoord([&](df::coord pos) {
            count += regrass_tile(out, options, block, pos.x&15, pos.y&15);
            return true;
        });
        return true;
    });
    return count;
}

int regrass_block(color_ostream &out, const regrass_options &options, df::map_block *block)
{   // Regrass all tiles in a single block.
    CHECK_NULL_POINTER(block);
    DEBUG(log, out).print("Regrass block at (%d, %d, %d)\n",
        block->map_pos.x, block->map_pos.y, block->map_pos.z);

    int count = 0;
    for (int tx = 0; tx < 16; tx++)
        for (int ty = 0; ty < 16; ty++)
            count += regrass_tile(out, options, block, tx, ty);
    return count;
}

int regrass_map(color_ostream &out, const regrass_options &options)
{   // Regrass entire map.
    int count = 0;
    for (auto block : world->map.map_blocks)
        count += regrass_block(out, options, block);
    return count;
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "regrass",
        "Regrow surface grass and cavern moss.",
        df_regrass));
    return CR_OK;
}

command_result df_regrass(color_ostream &out, vector<string> &parameters)
{
    regrass_options options;
    df::coord pos_1, pos_2;

    CoreSuspender suspend;

    if (!Lua::CallLuaModuleFunction(out, "plugins.regrass", "parse_commandline",
        std::make_tuple(&options, &pos_1, &pos_2, parameters)))
    {   // Failure in Lua script.
        return CR_WRONG_USAGE;
    }
    else if (options.forced_plant == -2)
    {   // Print all grass raw IDs.
        for (auto p_raw : world->raws.plants.grasses)
            out.print("%d: %s\n", p_raw->index, p_raw->id.c_str());
        return CR_OK;
    }

    DEBUG(log, out).print("pos_1 = (%d, %d, %d)\npos_2 = (%d, %d, %d)\n",
        pos_1.x, pos_1.y, pos_1.z, pos_2.x, pos_2.y, pos_2.z);

    if (options.block && options.zlevel) {
        out.printerr("Choose only --block or --zlevel!\n");
        return CR_WRONG_USAGE;
    }
    else if (options.block && (!pos_1.isValid() || pos_2.isValid())) {
        out.printerr("Invalid pos for --block (or used more than one!)\n");
        return CR_WRONG_USAGE;
    }
    else if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Map not loaded!\n");
        return CR_FAILURE;
    }

    if (options.force) {
        DEBUG(log, out).print("forced_plant = %d\n", options.forced_plant);
        auto p_raw = vector_get(world->raws.plants.all, options.forced_plant);
        if (p_raw) {
            if (!p_raw->flags.is_set(plant_raw_flags::GRASS)) {
                out.printerr("Plant raw wasn't grass: %d (%s)\n", options.forced_plant, p_raw->id.c_str());
                return CR_FAILURE;
            }
            out.print("Forced grass: %s\n", p_raw->id.c_str());
        }
        else {
            out.printerr("Plant raw not found for --force: %d\n", options.forced_plant);
            return CR_FAILURE;
        }
    }

    int count = 0;
    if (options.zlevel)
    {   // Specified z-levels or viewport z.
        auto z1 = pos_1.isValid() ? pos_1.z : Gui::getViewportPos().z;
        auto z2 = pos_2.isValid() ? pos_2.z : z1;
        DEBUG(log, out).print("Regrassing z-levels %d to %d...\n", z1, z2);
        count = regrass_cuboid(out, options, cuboid(0, 0, z1,
            world->map.x_count-1, world->map.y_count-1, z2));
    }
    else if (pos_1.isValid())
    {   // Block, cuboid, or point.
        if (!options.block && pos_2.isValid())
        {   // Cuboid
            DEBUG(log, out).print("Regrassing cuboid...\n");
            count = regrass_cuboid(out, options, cuboid(pos_1, pos_2));
        }
        else
        {   // Block or point.
            auto b = Maps::getTileBlock(pos_1);
            if (!b) {
                out.printerr("No map block at pos!\n");
                return CR_FAILURE;
            }

            if (options.block) {
                DEBUG(log, out).print("Regrassing block...\n");
                count = regrass_block(out, options, b);
            }
            else
            {   // Point
                DEBUG(log, out).print("Regrassing single tile...\n");
                count = regrass_tile(out, options, b, pos_1.x&15, pos_1.y&15);
            }
        }
    }
    else
    {   // Do entire map.
        DEBUG(log, out).print("Regrassing map...\n");
        count = regrass_map(out, options);
    }
    out.print("Regrew %d tiles of grass.\n", count);
    return CR_OK;
}
