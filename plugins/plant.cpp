// Grow and remove shrubs or trees.

#include "Debug.h"
#include "Error.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"
#include "TileTypes.h"

#include "modules/Gui.h"
#include "modules/Maps.h"

#include "df/block_square_event_grassst.h"
#include "df/block_square_event_material_spatterst.h"
#include "df/builtin_mats.h"
#include "df/map_block.h"
#include "df/map_block_column.h"
#include "df/plant.h"
#include "df/plant_raw.h"
#include "df/plant_root_tile.h"
#include "df/plant_tree_info.h"
#include "df/plant_tree_tile.h"
#include "df/world.h"

using std::string;
using std::vector;
using namespace DFHack;

DFHACK_PLUGIN("plant");
REQUIRE_GLOBAL(world);

namespace DFHack
{
    DBG_DECLARE(plant, log, DebugCategory::LINFO);
}

struct plant_options
{
    bool create = false; // Create a plant
    bool grow = false; // Grow saplings into trees
    bool del = false; // Remove plants
    bool force = false; // Create plants on no_grow or incompatible wet/dry
    bool shrubs = false; // Remove shrubs
    bool saplings = false; // Remove saplings
    bool trees = false; // Remove grown trees
    bool dead = false; // Remove only dead plants
    bool filter_ex = false; // Filter vector excludes if true, else requires its plants
    bool zlevel = false; // Operate on entire z-levels
    bool dry_run = false; // Don't actually grow or remove anything

    int32_t plant_idx = -1; // Plant raw index of plant to create; -2 means print all non-grass IDs
    int32_t age = -1; // Set plant to this age for grow/create; -1 for default

    static struct_identity _identity;
};
static const struct_field_info plant_options_fields[] =
{
    { struct_field_info::PRIMITIVE, "create",    offsetof(plant_options, create),    &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "grow",      offsetof(plant_options, grow),      &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "del",       offsetof(plant_options, del),       &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "force",     offsetof(plant_options, force),     &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "shrubs",    offsetof(plant_options, shrubs),    &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "saplings",  offsetof(plant_options, saplings),  &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "trees",     offsetof(plant_options, trees),     &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "dead",      offsetof(plant_options, dead),      &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "filter_ex", offsetof(plant_options, filter_ex), &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "zlevel",    offsetof(plant_options, zlevel),    &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "dry_run",   offsetof(plant_options, dry_run),   &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "plant_idx", offsetof(plant_options, plant_idx), &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "age",       offsetof(plant_options, age),       &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::END }
};
struct_identity plant_options::_identity(sizeof(plant_options), &df::allocator_fn<plant_options>, NULL, "plant_options", NULL, plant_options_fields);

const int32_t sapling_to_tree_threshold = 120 * 28 * 12 * 3 - 1; // 3 years minus 1; let the game handle the actual growing-up

static bool tile_muddy(const df::coord &pos)
{   // True if tile has mud spatter
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return false;

    for (auto blev : block->block_events)
    {
        if (blev->getType() != block_square_event_type::material_spatter)
            continue;

        auto &ms_ev = *(df::block_square_event_material_spatterst *)blev;
        if (ms_ev.mat_type == builtin_mats::MUD)
            return ms_ev.amount[pos.x&15][pos.y&15] > 0;
    }

    return false;
}

static bool tile_watery(const df::coord &pos)
{   // Determines if plant should be in wet or dry vector
    int32_t x = pos.x, y = pos.y, z = pos.z - 1;

    for (int32_t dx = -2; dx <= 2; dx++)
    {   // Check 5x5 area under tile, skipping corners
        for (int32_t dy = -2; dy <= 2; dy++)
        {
            if (abs(dx) == 2 && abs(dy) == 2)
                continue; // Skip corners

            auto tt = Maps::getTileType(x+dx, y+dy, z);
            if (!tt)
                continue; // Invalid tile

            auto mat = tileMaterial(*tt);
            if (mat == tiletype_material::POOL ||
                mat == tiletype_material::RIVER ||
                tileShape(*tt) == tiletype_shape::BROOK_BED)
            {
                return true;
            }
        }
    }

    return false;
}

command_result df_createplant(color_ostream &out, const df::coord &pos, const plant_options &options)
{
    auto col = Maps::getBlockColumn((pos.x / 48)*3, (pos.y / 48)*3);
    auto tt = Maps::getTileType(pos);
    if (!tt || !col)
    {
        out.printerr("No map block at pos!\n");
        return CR_FAILURE;
    }

    if (tileShape(*tt) != tiletype_shape::FLOOR)
    {
        out.printerr("Can't create plant: Not floor!\n");
        return CR_FAILURE;
    }

    auto occ = Maps::getTileOccupancy(pos);
    CHECK_NULL_POINTER(occ);
    if (occ->bits.building > tile_building_occ::None)
    {
        out.printerr("Can't create plant: Building present!\n");
        return CR_FAILURE;
    }
    else if (!options.force && occ->bits.no_grow)
    {
        out.printerr("Can't create plant: Tile flagged no_grow and not --force!\n");
        return CR_FAILURE;
    }

    auto des = Maps::getTileDesignation(pos);
    CHECK_NULL_POINTER(des);
    if (des->bits.flow_size > (des->bits.liquid_type == tile_liquid::Magma ? 0U : 3U))
    {
        out.printerr("Can't create plant: Too much liquid!\n");
        return CR_FAILURE;
    }

    auto spec = tileSpecial(*tt);
    auto mat = tileMaterial(*tt);
    switch (mat)
    {
        case tiletype_material::SOIL:
        case tiletype_material::GRASS_LIGHT:
        case tiletype_material::GRASS_DARK:
        case tiletype_material::ASHES:
            break;
        case tiletype_material::STONE:
        case tiletype_material::LAVA_STONE:
        case tiletype_material::MINERAL:
            if (spec == tiletype_special::SMOOTH || spec == tiletype_special::TRACK)
            {
                out.printerr("Can't create plant: Smooth stone!\n");
                return CR_FAILURE;
            }
            else if (!tile_muddy(pos))
            {
                out.printerr("Can't create plant: Non-muddy stone!\n");
                return CR_FAILURE;
            }

            break;
        default:
            out.printerr("Can't create plant: Wrong tile material!\n");
            return CR_FAILURE;
    }

    auto p_raw = vector_get(world->raws.plants.all, options.plant_idx);
    if (!p_raw)
    {
        out.printerr("Can't create plant: Plant raw not found!\n");
        return CR_FAILURE;
    }

    bool is_watery = tile_watery(pos);
    if (!options.force)
    {   // Check if plant compatible with wet/dry
        if ((is_watery && !p_raw->flags.is_set(plant_raw_flags::WET)) ||
            (!is_watery && !p_raw->flags.is_set(plant_raw_flags::DRY)))
        {
            out.printerr("Can't create plant: Plant type can't grow this %s water feature!\n"
                "Override with --force\n", is_watery ? "close to" : "far from");
            return CR_FAILURE;
        }
    }

    auto plant = df::allocate<df::plant>();
    if (p_raw->flags.is_set(plant_raw_flags::TREE))
    {
        plant->type = is_watery ? plant_type::WET_TREE : plant_type::DRY_TREE;
        plant->hitpoints = 400000;
    }
    else // Shrub
    {
        plant->type = is_watery ? plant_type::WET_PLANT : plant_type::DRY_PLANT;
        plant->hitpoints = 100000;
    }

    plant->material = options.plant_idx;
    plant->pos = pos;
    plant->grow_counter = options.age < 0 ? 0 : options.age;
    plant->update_order = rand() % 10;

    world->plants.all.push_back(plant);
    col->plants.push_back(plant);

    switch (plant->type)
    {
        case plant_type::DRY_TREE:
            world->plants.tree_dry.push_back(plant);
            *tt = tiletype::Sapling;
            break;
        case plant_type::WET_TREE:
            world->plants.tree_wet.push_back(plant);
            *tt = tiletype::Sapling;
            break;
        case plant_type::DRY_PLANT:
            world->plants.shrub_dry.push_back(plant);
            *tt = tiletype::Shrub;
            break;
        case plant_type::WET_PLANT:
            world->plants.shrub_wet.push_back(plant);
            *tt = tiletype::Shrub;
            break;
    }

    occ->bits.no_grow = false;

    return CR_OK;
}

static bool plant_in_cuboid(const df::plant *plant, const cuboid &bounds)
{   // Will detect tree tiles
    if (bounds.containsPos(plant->pos))
        return true;
    else if (!plant->tree_info)
        return false;

    auto &pos = plant->pos;
    auto &t = *(plant->tree_info);

    // Northwest x/y pos of tree bounds
    int x_NW = pos.x - (t.dim_x >> 1);
    int y_NW = pos.y - (t.dim_y >> 1);

    if (!cuboid(std::max(0, x_NW), std::max(0, y_NW), std::max(0, pos.z - t.roots_depth),
        x_NW + t.dim_x, y_NW + t.dim_y, pos.z + t.body_height - 1).clamp(bounds).isValid())
    {   // No intersection of tree bounds with cuboid
        return false;
    }

    int xy_size = t.dim_x * t.dim_y;
    // Iterate tree body
    for (int z_idx = 0; z_idx < t.body_height; z_idx++)
        for (int xy_idx = 0; xy_idx < xy_size; xy_idx++)
            if ((t.body[z_idx][xy_idx].whole & 0x7F) != 0 && // Any non-blocked
                bounds.containsPos(x_NW + xy_idx % t.dim_x, y_NW + xy_idx / t.dim_x, pos.z + z_idx))
            {
                return true;
            }

    // Iterate tree roots
    for (int z_idx = 0; z_idx < t.roots_depth; z_idx++)
        for (int xy_idx = 0; xy_idx < xy_size; xy_idx++)
            if ((t.roots[z_idx][xy_idx].whole & 0x7F) != 0 && // Any non-blocked
                bounds.containsPos(x_NW + xy_idx % t.dim_x, y_NW + xy_idx / t.dim_x, pos.z - z_idx - 1))
            {
                return true;
            }

    return false;
}

command_result df_grow(color_ostream &out, const cuboid &bounds, const plant_options &options, vector<int32_t> *filter = nullptr)
{
    if (!bounds.isValid())
    {
        out.printerr("Invalid cuboid! (%d:%d, %d:%d, %d:%d)\n",
            bounds.x_min, bounds.x_max, bounds.y_min, bounds.y_max, bounds.z_min, bounds.z_max);
        return CR_FAILURE;
    }

    bool do_filter = filter && !filter->empty();
    if (do_filter) // Sort filter vector
        std::sort(filter->begin(), filter->end());

    int32_t age = options.age < 0 ? sapling_to_tree_threshold : options.age; // Enforce default
    bool do_trees = age > sapling_to_tree_threshold;

    int grown = 0, grown_trees = 0;
    for (auto plant : world->plants.all)
    {
        if (ENUM_ATTR(plant_type, is_shrub, plant->type))
            continue; // Shrub
        else if (!plant_in_cuboid(plant, bounds))
            continue; // Outside cuboid
        else if (do_filter && (vector_contains(*filter, (int32_t)plant->material) == options.filter_ex))
            continue; // Filtered out
        else if (plant->tree_info)
        {   // Tree
            if (do_trees && !plant->damage_flags.bits.dead && plant->grow_counter < age)
            {
                if (!options.dry_run)
                    plant->grow_counter = age;
                grown_trees++;
            }
            continue; // Next plant
        }

        auto tt = Maps::getTileType(plant->pos);
        if (!tt || tileShape(*tt) != tiletype_shape::SAPLING)
        {
            out.printerr("Invalid sapling tiletype at (%d, %d, %d): %s!\n",
                plant->pos.x, plant->pos.y, plant->pos.z,
                tt ? ENUM_KEY_STR(tiletype, *tt).c_str() : "No map block!");
            continue; // Bad tiletype
        }
        else if (*tt == tiletype::SaplingDead)
            *tt = tiletype::Sapling; // Revive sapling

        if (!options.dry_run)
        {
            plant->damage_flags.bits.dead = false;
            plant->grow_counter = age;
        }
        grown++;
    }

    if (do_trees)
        out.print("%d saplings and %d trees%s set to grow.\n", grown, grown_trees, options.dry_run ? " would be" : "");
    else
        out.print("%d saplings%s set to grow.\n", grown, options.dry_run ? " would be" : "");

    return CR_OK;
}

static bool uncat_plant(df::plant *plant)
{   // Remove plant from extra vectors
    vector<df::plant *> *vec = NULL;

    switch (plant->type)
    {
        case plant_type::DRY_TREE: vec = &world->plants.tree_dry; break;
        case plant_type::WET_TREE: vec = &world->plants.tree_wet; break;
        case plant_type::DRY_PLANT: vec = &world->plants.shrub_dry; break;
        case plant_type::WET_PLANT: vec = &world->plants.shrub_wet; break;
    }

    for (size_t i = vec->size(); i-- > 0;)
    {   // Not sorted, but more likely near end
        if ((*vec)[i] == plant)
        {
            vec->erase(vec->begin() + i);
            break;
        }
    }

    auto col = Maps::getBlockColumn((plant->pos.x / 48)*3, (plant->pos.y / 48)*3);
    if (!col)
        return false;

    vec = &col->plants;
    for (size_t i = vec->size(); i-- > 0;)
    {   // Not sorted, but more likely near end
        if ((*vec)[i] == plant)
        {
            vec->erase(vec->begin() + i);
            break;
        }
    }

    return true;
}

static bool has_grass(df::map_block *block, int tx, int ty)
{   // Block tile has grass
    for (auto blev : block->block_events)
    {
        if (blev->getType() != block_square_event_type::grass)
            continue;

        auto &g_ev = *(df::block_square_event_grassst *)blev;
        if (g_ev.amount[tx][ty] > 0)
            return true;
    }

    return false;
}

static void set_tt(const df::coord &pos)
{   // Set tiletype to grass or soil floor
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return;

    int tx = pos.x & 15, ty = pos.y & 15;
    if (has_grass(block, tx, ty))
        block->tiletype[tx][ty] = findRandomVariant((rand() & 1) ? tiletype::GrassLightFloor1 : tiletype::GrassDarkFloor1);
    else
        block->tiletype[tx][ty] = findRandomVariant(tiletype::SoilFloor1);
}

command_result df_removeplant(color_ostream &out, const cuboid &bounds, const plant_options &options, vector<int32_t> *filter = nullptr)
{
    if (!bounds.isValid())
    {
        out.printerr("Invalid cuboid! (%d:%d, %d:%d, %d:%d)\n",
            bounds.x_min, bounds.x_max, bounds.y_min, bounds.y_max, bounds.z_min, bounds.z_max);
        return CR_FAILURE;
    }

    bool by_type = options.shrubs || options.saplings || options.trees;
    bool do_filter = by_type && filter && !filter->empty();
    if (do_filter) // Sort filter vector
        std::sort(filter->begin(), filter->end());


    int count = 0, count_bad = 0;
    auto &vec = world->plants.all;
    for (size_t i = vec.size(); i-- > 0;)
    {
        auto &plant = *vec[i];
        auto tt = Maps::getTileType(plant.pos);

        if (plant.tree_info) // TODO: handle trees
            continue; // Not implemented
        else if (by_type)
        {
            if (options.dead && !plant.damage_flags.bits.dead && tt && tileSpecial(*tt) != tiletype_special::DEAD)
                continue; // Not removing living
            /*else if (plant->tree_info && !options.trees)
                continue; // Not removing trees*/
            else if (ENUM_ATTR(plant_type, is_shrub, plant.type))
            {
                if (!options.shrubs)
                    continue; // Not removing shrubs
            }
            else if (!options.saplings)
                continue; // Not removing saplings
        }

        if (!plant_in_cuboid(&plant, bounds))
            continue; // Outside cuboid
        else if (do_filter && (vector_contains(*filter, (int32_t)plant.material) == options.filter_ex))
            continue; // Filtered out

        bool bad_tt = false;
        if (tt)
        {
            if (ENUM_ATTR(plant_type, is_shrub, plant.type))
            {
                if (tileShape(*tt) != tiletype_shape::SHRUB)
                {
                    out.printerr("Bad shrub tiletype at (%d, %d, %d): %s\n",
                        plant.pos.x, plant.pos.y, plant.pos.z,
                        ENUM_KEY_STR(tiletype, *tt).c_str());
                    bad_tt = true;
                }
            }
            else if (!plant.tree_info)
            {
                if (tileShape(*tt) != tiletype_shape::SAPLING)
                {
                    out.printerr("Bad sapling tiletype at (%d, %d, %d): %s\n",
                        plant.pos.x, plant.pos.y, plant.pos.z,
                        ENUM_KEY_STR(tiletype, *tt).c_str());
                    bad_tt = true;
                }
            }
            // TODO: trees
        }
        else
        {
            out.printerr("Bad plant tiletype at (%d, %d, %d): No map block!\n",
                plant.pos.x, plant.pos.y, plant.pos.z);
            bad_tt = true;
        }

        if (!by_type && !bad_tt)
            continue; // Only remove bad plants

        count++;
        if (bad_tt)
            count_bad++;

        if (!options.dry_run)
        {
            if (!uncat_plant(&plant))
                out.printerr("Remove plant: No block column at (%d, %d)!\n", plant.pos.x, plant.pos.y);

            if (!bad_tt) // TODO: trees
                set_tt(plant.pos);

            vec.erase(vec.begin() + i);
            delete &plant;
        }
    }

    out.print("Plants%s removed: %d (%d bad)\n", options.dry_run ? " that would be" : "", count, count_bad);
    return CR_OK;
}

command_result df_plant(color_ostream &out, vector<string> &parameters)
{
    plant_options options;
    cuboid bounds;
    df::coord pos_1, pos_2;
    vector<int32_t> filter; // Unsorted

    if (!Lua::CallLuaModuleFunction(out, "plugins.plant", "parse_commandline",
        std::make_tuple(&options, &pos_1, &pos_2, &filter, parameters)))
    {
        return CR_WRONG_USAGE;
    }
    else if (options.plant_idx == -2)
    {   // Print all non-grass raw IDs ("plant list")
        out.print("--- Shrubs ---\n");
        for (auto p_raw : world->raws.plants.bushes)
            out.print("%d: %s\n", p_raw->index, p_raw->id.c_str());

        out.print("\n--- Saplings ---\n");
        for (auto p_raw : world->raws.plants.trees)
            out.print("%d: %s\n", p_raw->index, p_raw->id.c_str());

        return CR_OK;
    }
    else if (options.force && !options.create)
    {
        out.printerr("Can't use --force without create!\n");
        return CR_WRONG_USAGE;
    }

    bool by_type = options.shrubs || options.saplings || options.trees; // Remove only invalid plants otherwise
    if (!options.del && (by_type || options.dead))
    {   // Don't use remove options outside remove
        out.printerr("Can't use remove's options without remove!\n");
        return CR_WRONG_USAGE;
    }
    else if (!by_type && options.dead)
    {   // Don't target dead plants while fixing invalid
        out.printerr("Can't use --dead without targeting shrubs/saplings!\n"); // TODO: trees
        return CR_WRONG_USAGE;
    }
    else if (options.del && options.age >= 0)
    {   // Can't set age with remove
        out.printerr("Can't use --age with remove!\n");
        return CR_WRONG_USAGE;
    }
    else if (options.trees)
    {   // TODO: implement
        out.printerr("--trees not implemented!\n");
        return CR_FAILURE;
    }

    DEBUG(log, out).print("pos_1 = (%d, %d, %d)\npos_2 = (%d, %d, %d)\n",
        pos_1.x, pos_1.y, pos_1.z, pos_2.x, pos_2.y, pos_2.z);

    if (!Core::getInstance().isMapLoaded())
    {
        out.printerr("Map not loaded!\n");
        return CR_FAILURE;
    }

    if (options.create)
    {   // Check improper options and plant raw
        if (options.zlevel || options.dry_run)
        {
            out.printerr("Cannot use --zlevel or --dry-run with create!\n");
            return CR_FAILURE;
        }
        else if (!filter.empty())
        {
            out.printerr("Cannot use filter/exclude with create!\n");
            return CR_FAILURE;
        }

        if (!pos_1.isValid())
        {   // Attempt to use cursor for pos if active
            Gui::getCursorCoords(pos_1);
            DEBUG(log, out).print("Try to use cursor (%d, %d, %d) for pos_1.\n",
                pos_1.x, pos_1.y, pos_1.z);

            if (!pos_1.isValid())
            {
                out.printerr("Invalid pos for create! Make sure keyboard cursor is active if not entering pos manually!\n");
                return CR_WRONG_USAGE;
            }
        }

        DEBUG(log, out).print("plant_idx = %d\n", options.plant_idx);
        auto p_raw = vector_get(world->raws.plants.all, options.plant_idx);
        if (p_raw)
        {
            DEBUG(log, out).print("Plant raw: %s\n", p_raw->id.c_str());
            if (p_raw->flags.is_set(plant_raw_flags::GRASS))
            {
                out.printerr("Plant raw was grass: %d (%s)\n", options.plant_idx, p_raw->id.c_str());
                return CR_FAILURE;
            }
        }
        else
        {
            out.printerr("Plant raw not found for create: %d\n", options.plant_idx);
            return CR_FAILURE;
        }
    }
    else // options.grow || options.remove
    {   // Check filter and setup cuboid
        if (!filter.empty())
        {   // Validate filter plant raws
            if (!by_type && options.del)
            {
                out.printerr("Filter/exclude set, but not targeting shrubs/saplings!\n"); // TODO: trees
                return CR_WRONG_USAGE;
            }

            for (auto idx : filter)
            {
                DEBUG(log, out).print("Filter/exclude test idx: %d\n", idx);
                auto p_raw = vector_get(world->raws.plants.all, idx);
                if (p_raw)
                {
                    DEBUG(log, out).print("Filter/exclude raw: %s\n", p_raw->id.c_str());
                    if (p_raw->flags.is_set(plant_raw_flags::GRASS))
                    {
                        out.printerr("Filter/exclude plant raw was grass: %d (%s)\n", idx, p_raw->id.c_str());
                        return CR_FAILURE;
                    }
                    else if (options.grow && !p_raw->flags.is_set(plant_raw_flags::TREE))
                    {   // User might copy-paste filters between grow and remove, so just log this
                        DEBUG(log, out).print("Filter/exclude shrub with grow: %d (%s)\n", idx, p_raw->id.c_str());
                    }
                }
                else
                {
                    out.printerr("Plant raw not found for filter/exclude: %d\n", idx);
                    return CR_FAILURE;
                }
            }
        }

        if (options.zlevel)
        {   // Adjusted cuboid
            if (!pos_1.isValid())
            {
                DEBUG(log, out).print("pos_1 invalid and --zlevel. Using viewport.\n");
                pos_1.z = Gui::getViewportPos().z;
            }

            if (!pos_2.isValid())
            {
                DEBUG(log, out).print("pos_2 invalid and --zlevel. Using pos_1.\n");
                pos_2.z = pos_1.z;
            }

            bounds.addPos(0, world->map.y_count-1, pos_1.z);
            bounds.addPos(world->map.x_count-1, 0, pos_2.z);
        }
        else if (pos_1.isValid())
        {   // Cuboid or single point
            bounds.addPos(pos_1);
            bounds.addPos(pos_2); // Point if invalid
        }
        else // Entire map
        {
            bounds.addPos(0, 0, world->map.z_count-1);
            bounds.addPos(world->map.x_count-1, world->map.y_count-1, 0);
        }

        DEBUG(log, out).print("bounds = (%d:%d, %d:%d, %d:%d)\n",
            bounds.x_min, bounds.x_max, bounds.y_min, bounds.y_max, bounds.z_min, bounds.z_max);

        if (!bounds.isValid())
        {
            out.printerr("Invalid cuboid! (%d:%d, %d:%d, %d:%d)\n",
                bounds.x_min, bounds.x_max, bounds.y_min, bounds.y_max, bounds.z_min, bounds.z_max);
            return CR_FAILURE;
        }
    }

    if (options.create)
        return df_createplant(out, pos_1, options);
    else if (options.grow)
        return df_grow(out, bounds, options, &filter);
    else if (options.del)
        return df_removeplant(out, bounds, options, &filter);

    return CR_WRONG_USAGE;
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "plant",
        "Grow and remove shrubs or trees.",
        df_plant));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    return CR_OK;
}
