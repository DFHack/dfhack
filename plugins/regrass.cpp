#include "modules/Gui.h"
#include "modules/Maps.h"
#include "PluginManager.h"
#include "TileTypes.h"

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

command_result df_regrass(color_ostream &out, vector<string> &parameters);

struct regrass_options
{
    bool max_grass = false;
    bool new_grass = false;
    bool ashes = false;
    bool mud = false;
    bool here = false;
    bool block = false;
};

#define P_R_FLAG(btype) case biome_type::btype: return plant_raw_flags::BIOME_##btype

static df::plant_raw_flags biome_flag(df::biome_type biome_type) // TODO: attr? update gui/biomes; plant_index mapping?
{
    switch (biome_type)
    {
        P_R_FLAG(MOUNTAIN);
        P_R_FLAG(GLACIER);
        P_R_FLAG(TUNDRA);
        P_R_FLAG(SWAMP_TEMPERATE_FRESHWATER);
        P_R_FLAG(SWAMP_TEMPERATE_SALTWATER);
        P_R_FLAG(MARSH_TEMPERATE_FRESHWATER);
        P_R_FLAG(MARSH_TEMPERATE_SALTWATER);
        P_R_FLAG(SWAMP_TROPICAL_FRESHWATER);
        P_R_FLAG(SWAMP_TROPICAL_SALTWATER);
        P_R_FLAG(SWAMP_MANGROVE);
        P_R_FLAG(MARSH_TROPICAL_FRESHWATER);
        P_R_FLAG(MARSH_TROPICAL_SALTWATER);
        P_R_FLAG(FOREST_TAIGA);
        P_R_FLAG(FOREST_TEMPERATE_CONIFER);
        P_R_FLAG(FOREST_TEMPERATE_BROADLEAF);
        P_R_FLAG(FOREST_TROPICAL_CONIFER);
        P_R_FLAG(FOREST_TROPICAL_DRY_BROADLEAF);
        P_R_FLAG(FOREST_TROPICAL_MOIST_BROADLEAF);
        P_R_FLAG(GRASSLAND_TEMPERATE);
        P_R_FLAG(SAVANNA_TEMPERATE);
        P_R_FLAG(SHRUBLAND_TEMPERATE);
        P_R_FLAG(GRASSLAND_TROPICAL);
        P_R_FLAG(SAVANNA_TROPICAL);
        P_R_FLAG(SHRUBLAND_TROPICAL);
        P_R_FLAG(DESERT_BADLAND);
        P_R_FLAG(DESERT_ROCK);
        P_R_FLAG(DESERT_SAND);
        P_R_FLAG(OCEAN_TROPICAL);
        P_R_FLAG(OCEAN_TEMPERATE);
        P_R_FLAG(OCEAN_ARCTIC);
        P_R_FLAG(POOL_TEMPERATE_FRESHWATER);
        P_R_FLAG(POOL_TEMPERATE_BRACKISHWATER);
        P_R_FLAG(POOL_TEMPERATE_SALTWATER);
        P_R_FLAG(POOL_TROPICAL_FRESHWATER);
        P_R_FLAG(POOL_TROPICAL_BRACKISHWATER);
        P_R_FLAG(POOL_TROPICAL_SALTWATER);
        P_R_FLAG(LAKE_TEMPERATE_FRESHWATER);
        P_R_FLAG(LAKE_TEMPERATE_BRACKISHWATER);
        P_R_FLAG(LAKE_TEMPERATE_SALTWATER);
        P_R_FLAG(LAKE_TROPICAL_FRESHWATER);
        P_R_FLAG(LAKE_TROPICAL_BRACKISHWATER);
        P_R_FLAG(LAKE_TROPICAL_SALTWATER);
        P_R_FLAG(RIVER_TEMPERATE_FRESHWATER);
        P_R_FLAG(RIVER_TEMPERATE_BRACKISHWATER);
        P_R_FLAG(RIVER_TEMPERATE_SALTWATER);
        P_R_FLAG(RIVER_TROPICAL_FRESHWATER);
        P_R_FLAG(RIVER_TROPICAL_BRACKISHWATER);
        P_R_FLAG(RIVER_TROPICAL_SALTWATER);
        P_R_FLAG(SUBTERRANEAN_WATER);
        P_R_FLAG(SUBTERRANEAN_CHASM);
        P_R_FLAG(SUBTERRANEAN_LAVA);
    }

    Core::printerr("Bad biome %d!\n", biome_type);
    return plant_raw_flags::TREE; // Always false
}

#undef P_R_FLAG

static bool valid_tile(df::map_block *block, int x, int y, regrass_options options) // Is valid tile for regrass
{
    auto tt = block->tiletype[x][y];
    auto shape = tileShape(tt);
    auto mat = tileMaterial(tt);
    auto spec = tileSpecial(tt);

    if (mat == tiletype_material::GRASS_LIGHT ||
        mat == tiletype_material::GRASS_DARK ||
        mat == tiletype_material::PLANT) // Shrubs and saplings can have grass underneath
    {
        return true; // Refill existing grass
    }
    else if (tt == tiletype::TreeTrunkPillar)
    {   // Trees can have grass under main tile
        auto p = df::coord(block->map_pos.x + x, block->map_pos.y + y, block->map_pos.z);
        auto mbc = world->map.column_index[(block->map_pos.x / 48) * 3][(block->map_pos.y / 48) * 3];
        if (!mbc)
        {
            Core::printerr("No MLT block column for tile at (%d, %d, %d)!\n", p.x, p.y, p.z);
            return false;
        }

        for (auto plant : mbc->plants)
        {
            if (plant->pos == p)
                return true; // Is main tile
        }

        return false; // Not main tile
    }
    else if (block->designation[x][y].bits.flow_size > 2)
        return false; // Under water/magma; TODO: research
    else if (shape != tiletype_shape::FLOOR &&
        shape != tiletype_shape::RAMP &&
        shape != tiletype_shape::STAIR_UP &&
        shape != tiletype_shape::STAIR_DOWN &&
        shape != tiletype_shape::STAIR_UPDOWN)
    {
        return false;
    }
    else if (block->occupancy[x][y].bits.building != tile_building_occ::None || // TODO: research (planned, stockpiles)
        block->occupancy[x][y].bits.no_grow) // TODO: pebbles?
    {
        return false;
    }
    else if (mat == tiletype_material::SOIL)
        return spec != tiletype_special::FURROWED && spec != tiletype_special::WET; // Beach; TODO: research (beach biome? driftwood?)
    else if (options.ashes && mat == tiletype_material::ASHES)
        return true;
    else if (options.mud)
    {
        if (spec == tiletype_special::SMOOTH || spec == tiletype_special::TRACK)
            return false; // Don't replace smoothed stone
        else if (mat != tiletype_material::STONE &&
            mat != tiletype_material::LAVA_STONE &&
            mat != tiletype_material::MINERAL) // TODO: will DF replace FEATURE?
        {
            return false;
        }

        for (auto blev : block->block_events)
        {
            if (blev->getType() != block_square_event_type::material_spatter)
                continue;

            auto &ms_ev = *(df::block_square_event_material_spatterst *)blev;
            if (ms_ev.mat_type == builtin_mats::MUD)
                return ms_ev.amount[x][y] > 0;
        }
    }

    return false;
}

static vector<int32_t> grasses_for_tile(df::map_block *block, int x, int y) // Return sorted vector of valid grass ids
{
    vector<int32_t> grasses;

    if (block->designation[x][y].bits.subterranean)
    {
        for (auto p_raw : world->raws.plants.grasses) // Sorted by df::plant_raw::index
        {
            if (p_raw->flags.is_set(plant_raw_flags::BIOME_SUBTERRANEAN_WATER))
                grasses.push_back(p_raw->index);
        }
    }
    else // Above ground
    {
        auto rgn_pos = Maps::getBlockTileBiomeRgn(block, df::coord2d(x, y)); // Okay to use x,y here

        if (!rgn_pos.isValid()) // No biome (happens in sky)
            return grasses;

        auto &biome_info = *Maps::getRegionBiome(rgn_pos);
        auto plant_biome_flag = biome_flag(Maps::getBiomeType(rgn_pos.x, rgn_pos.y));

        bool good = (biome_info.evilness < 33);
        bool evil = (biome_info.evilness > 65); // TODO: does necro modify separately?
        bool savage = (biome_info.savagery > 65);

        for (auto p_raw : world->raws.plants.grasses) // Sorted by df::plant_raw::index
        {
            auto &flags = p_raw->flags;
            if (flags.is_set(plant_biome_flag) &&
                (good || !flags.is_set(plant_raw_flags::GOOD)) &&
                (evil || !flags.is_set(plant_raw_flags::EVIL)) &&
                (savage || !flags.is_set(plant_raw_flags::SAVAGE)))
            {
                grasses.push_back(p_raw->index);
            }
        }
    }

    return grasses;
}

static bool regrass_events(df::map_block *block, int x, int y, regrass_options options) // Modify grass block events
{
    if (!valid_tile(block, x, y, options))
        return false;

    bool success = false;
    for (auto blev : block->block_events) // Refill existing events
    {
        if (blev->getType() != block_square_event_type::grass)
            continue;

        auto &gr_ev = *(df::block_square_event_grassst *)blev;

        if (options.max_grass)
        {   // Refill all
            gr_ev.amount[x][y] = 100;
            success = true;
        }
        else if (gr_ev.amount[x][y] > 0)
        {   // Refill first non-zero grass
            gr_ev.amount[x][y] = 100;
            return true;
        }
    }

    auto valid_grasses = grasses_for_tile(block, x, y);
    if (options.new_grass && !valid_grasses.empty()) // Try to add new grass events
    {
        auto new_grasses(valid_grasses); // Copy vector
        for (auto blev : block->block_events) // Find which grasses we're missing
        {
            if (blev->getType() == block_square_event_type::grass)
                erase_from_vector(new_grasses, ((df::block_square_event_grassst *)blev)->plant_index);
        }

        for (auto id : new_grasses) // Add new grass events
        {
            auto gr_ev = df::allocate<df::block_square_event_grassst>();
            block->block_events.push_back(gr_ev);
            gr_ev->plant_index = id;

            if (options.max_grass)
            {   // Initialize tile as full
                gr_ev->amount[x][y] = 100;
                success = true;
            }
        }
    }

    if (options.max_grass) // Already as full as it can get
        return success;

    vector<df::block_square_event_grassst *> temp;
    for (auto blev : block->block_events) // Gather all valid grass events
    {
        if (blev->getType() != block_square_event_type::grass)
            continue;

        auto gr_ev = (df::block_square_event_grassst *)blev;
        if (vector_contains(valid_grasses, gr_ev->plant_index))
            temp.push_back(gr_ev);
    }

    auto gr_ev = vector_get_random(temp);
    if (gr_ev)
    {
        gr_ev->amount[x][y] = 100;
        return true;
    }

    return false; // Can't support grass
}

int regrass_tile(df::map_block *block, int x, int y, regrass_options options)
{
    if (!regrass_events(block, x, y, options))
        return 0;

    auto tt = block->tiletype[x][y];
    auto mat = tileMaterial(tt);
    auto shape = tileShape(tt);

    if (mat == tiletype_material::GRASS_LIGHT ||
        mat == tiletype_material::GRASS_DARK ||
        mat == tiletype_material::PLANT ||
        tt == tiletype::TreeTrunkPillar)
    {
        return 1; // Already appropriate tile
    }
    else if (mat == tiletype_material::STONE ||
        mat == tiletype_material::LAVA_STONE ||
        mat == tiletype_material::MINERAL)
    {   // Muddy stone
        for (auto blev : block->block_events) // Remove mud spatter
        {
            if (blev->getType() != block_square_event_type::material_spatter)
                continue;

            auto &ms_ev = *(df::block_square_event_material_spatterst *)blev;
            if (ms_ev.mat_type == builtin_mats::MUD)
            {
                ms_ev.amount[x][y] = 0;
                break;
            }
        }
    }

    if (shape == tiletype_shape::FLOOR) // Handle random variant, ashes
        block->tiletype[x][y] = findRandomVariant((rand() & 1) ? tiletype::GrassLightFloor1 : tiletype::GrassDarkFloor1);
    else
    {
        auto new_mat = (rand() & 1) ? tiletype_material::GRASS_LIGHT : tiletype_material::GRASS_DARK;
        auto new_tt = findTileType(shape, new_mat, tiletype_variant::NONE, tiletype_special::NONE, nullptr);
        block->tiletype[x][y] = new_tt;
    }

    return 1;
}

int regrass_block(df::map_block *block, regrass_options options)
{
    int count = 0;
    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < 16; x++)
            count += regrass_tile(block, x, y, options);
    }

    return count;
}

int regrass_map(regrass_options options)
{
    int count = 0;
    for (auto block : world->map.map_blocks)
        count += regrass_block(block, options);

    return count;
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "regrass",
        "Regrow surface grass and cavern moss.",
        df_regrass));
    return CR_OK;
}

command_result df_regrass(color_ostream &out, vector<string> &parameters)
{
    regrass_options options;

    for (size_t i = 0; i < parameters.size(); i++) // TODO: "help"? better method
    {
        if (parameters[i] == "max")
            options.max_grass = true;
        else if (parameters[i] == "new")
            options.new_grass = true;
        else if (parameters[i] == "ashes")
            options.ashes = true;
        else if (parameters[i] == "mud")
            options.mud = true;
        else if (parameters[i] == "here")
            options.here = true;
        else if (parameters[i] == "block")
            options.block = true;
        else
            return CR_WRONG_USAGE;
    }

    if (!Core::getInstance().isMapLoaded())
    {
        out.printerr("Map not loaded!\n");
        return CR_FAILURE;
    }

    CoreSuspender suspend;

    int count = 0;
    if (options.here || options.block)
    {
        auto p = Gui::getCursorPos();
        if (!p.isValid())
        {
            out.printerr("Cursor required!\n");
            return CR_FAILURE;
        }

        auto b = Maps::getTileBlock(p);
        if (!b)
        {
            out.printerr("No map block at cursor!\n");
            return CR_FAILURE;
        }

        if (options.block)
            count = regrass_block(b, options);
        else
            count = regrass_tile(b, p.x&15, p.y&15, options);
    }
    else
        count = regrass_map(options);

    out.print("Regrew %d tiles of grass.\n", count);

    return CR_OK;
}
