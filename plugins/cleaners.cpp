// Cleans spatter and/or grass from map tiles.

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Buildings.h"
#include "modules/Items.h"
#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/Units.h"

#include "df/block_square_event.h"
#include "df/block_square_event_grassst.h"
#include "df/block_square_event_item_spatterst.h"
#include "df/block_square_event_material_spatterst.h"
#include "df/building.h"
#include "df/builtin_mats.h"
#include "df/item_actual.h"
#include "df/map_block.h"
#include "df/spatter.h"
#include "df/unit.h"
#include "df/unit_spatter.h"
#include "df/world.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("cleaners");
REQUIRE_GLOBAL(world);

namespace DFHack
{
    DBG_DECLARE(cleaners, log, DebugCategory::LINFO);
}

struct clean_options
{
    bool map = false; // Clean spatter from the ground
    bool mud = false; // Clean mud when doing map
    bool snow = false; // Clean snow when doing map
    bool item_spat = false; // Clean item spatter when doing map
    bool grass = false; // Delete surplus grass events when doing map
    bool desolate = false; // Remove all grass when doing map. Requires --grass. Careful!
    bool only = false; // Ignore blood/other when doing map, only do specified options
    bool units = false; // Clean spatter from units
    bool items = false; // Clean spatter from items
    bool zlevel = false; // Operate on entire z-levels

    static struct_identity _identity;
};
static const struct_field_info clean_options_fields[] =
{
    { struct_field_info::PRIMITIVE, "map",       offsetof(clean_options, map),       &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "mud",       offsetof(clean_options, mud),       &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "snow",      offsetof(clean_options, snow),      &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "item_spat", offsetof(clean_options, item_spat), &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "grass",     offsetof(clean_options, grass),     &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "desolate",  offsetof(clean_options, desolate),  &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "only",      offsetof(clean_options, only),      &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "units",     offsetof(clean_options, units),     &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "items",     offsetof(clean_options, items),     &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "zlevel",    offsetof(clean_options, zlevel),    &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::END }
};
struct_identity clean_options::_identity(sizeof(clean_options), &df::allocator_fn<clean_options>, NULL, "clean_options", NULL, clean_options_fields);

static bool clean_mud_safely(color_ostream &out, df::block_square_event_material_spatterst *spatter, const df::coord &pos)
{   // Avoid cleaning mud on farm tiles that need it, return true on success
    auto &amt = spatter->amount[pos.x&15][pos.y&15];
    if (amt == 0)
        return false; // Nothing cleaned
    auto tt = Maps::getTileType(pos);

    if (tt && *tt != tiletype::FurrowedSoil)
    {   // Not furrowed soil, mud might be required
        auto bld = Buildings::findAtTile(pos);
        if (bld && bld->getType() == building_type::FarmPlot)
        {   // A farm needs the mud
            DEBUG(log, out).print("Protecting mud at (%d,%d,%d)\n", pos.x, pos.y, pos.z);
            return false; // Won't clean
        }
    }
    amt = 0;
    TRACE(log, out).print("Cleaned mud at (%d,%d,%d)\n", pos.x, pos.y, pos.z);
    return true;
}

static void degrass_tt(color_ostream &out, df::map_block *block, int tx, int ty)
{   // Convert grass to soil
    auto &tt = block->tiletype[tx][ty];
    auto mat = tileMaterial(tt);

    if (mat < tiletype_material::GRASS_LIGHT ||
        mat > tiletype_material::GRASS_DEAD)
        return; // Grass under a sapling, etc.

    auto shape = tileShape(tt);
    df::tiletype new_tt = tiletype::Void;

    if (shape == tiletype_shape::FLOOR)
        new_tt = findRandomVariant(tiletype::SoilFloor1);
    else // Ramp or stairs
        new_tt = findTileType(shape, tiletype_material::SOIL, tiletype_variant::NONE, tiletype_special::NONE, nullptr);

    TRACE(log, out).print("Degrass %s to %s\n", ENUM_KEY_STR(tiletype, tt).c_str(), ENUM_KEY_STR(tiletype, new_tt).c_str());
    if (new_tt != tiletype::Void)
        tt = new_tt; // Assign new tiletype
}

#define DEL_BLEV block->block_events.erase(block->block_events.begin() + i); delete blev; cleaned = true;

static bool clean_block(color_ostream &out, df::map_block *block, const cuboid &inter, const clean_options &options)
{   // Perform cleaning on intersection of map block, return true if any tile cleaned
    if (!block || !inter.isValid())
    {
        DEBUG(log, out).print("Failed cleaning block <%p>\n", block);
        return false;
    }
    DEBUG(log, out).print("Cleaning block at (%d,%d,%d)\n",
        block->map_pos.x, block->map_pos.y, block->map_pos.z);
    bool cleaned = false;
    // Clean arrow debris
    inter.forCoord([&](df::coord pos) {
        auto &occ = block->occupancy[pos.x&15][pos.y&15];
        cleaned |= (bool)occ.bits.arrow_color;

        occ.bits.arrow_color = 0;
        occ.bits.arrow_variant = 0;
        return true; // Next pos
    });
    // Knowing if block is fully inside cuboid helps optimize
    bool full_block = (inter.x_max - inter.x_min) == 15 && (inter.y_max - inter.y_min) == 15;
    TRACE(log, out).print("full_block = %d\n", full_block);
    // Clean relevant spatter
    for (size_t i = block->block_events.size(); i-- > 0;)
    {   // Iterate block events (blev) backwards
        auto blev = block->block_events[i];
        if (auto ms_ev = virtual_cast<df::block_square_event_material_spatterst>(blev))
        {   // Material spatter
            if (ms_ev->mat_type == builtin_mats::MUD &&
                ms_ev->mat_state == matter_state::Solid)
            {   // Mud
                if (!options.mud)
                    continue; // Not doing mud, skip
                // Handle mud without messing up farms
                inter.forCoord([&](df::coord pos) {
                    cleaned |= clean_mud_safely(out, ms_ev, pos);
                    return true; // Next pos
                });
                // Only delete blev if empty
                if (blev->isEmpty())
                {   // Block was already empty of mud, or we just made it so
                    DEBUG(log, out).print("Deleting mud blev at index %lu\n", i);
                    DEL_BLEV
                }
                continue; // Next blev
            }
            else if (ms_ev->mat_type == builtin_mats::WATER &&
                ms_ev->mat_state == matter_state::Powder)
            {   // Snow
                if (options.snow)
                    continue; // Not doing snow, skip
            }
            else if (options.only)
                continue; // Not doing blood/other, skip

            if (!full_block)
            {   // Non-mud ms_ev, partial clean
                inter.forCoord([&](df::coord pos) {
                    auto &amt = ms_ev->amount[pos.x&15][pos.y&15];
                    cleaned |= (bool)amt;
                    amt = 0;
                    return true; // Next pos
                });
                // Will now check blev->isEmpty()
            }
            // else full_block, delete blev
        }
        else if (auto is_ev = virtual_cast<df::block_square_event_item_spatterst>(blev))
        {   // Item spatter
            if (!options.item_spat)
                continue; // Not doing item spatter, skip
            else if (!full_block)
            {   // Partial clean
                inter.forCoord([&](df::coord pos) {
                    auto &amt = is_ev->amount[pos.x&15][pos.y&15];
                    cleaned |= (bool)amt;
                    amt = 0;
                    return true; // Next pos
                });
                // Will now check blev->isEmpty()
            }
            // else full_block, delete blev
        }
        else if (auto gr_ev = virtual_cast<df::block_square_event_grassst>(blev))
        {   // Grass
            if (!options.grass)
                continue; // Not doing grass, skip
            else if (options.desolate)
            {   // We're actively removing grass tiles
                inter.forCoord([&](df::coord pos) {
                    auto &amt = gr_ev->amount[pos.x&15][pos.y&15];
                    cleaned |= (bool)amt;
                    amt = 0;
                    degrass_tt(out, block, pos.x&15, pos.y&15);
                    return true; // Next pos
                });
            }
            // Only delete blev if empty
            if (blev->isEmpty())
            {   // Block was already empty of grass type, or we just made it so
                DEBUG(log, out).print("Deleting grass blev at index %lu\n", i);
                DEL_BLEV
            }
            continue; // Next blev
        }
        else // Unhandled blev type
            continue; // Skip

        if (full_block || blev->isEmpty())
        {   // Always delete a full block, else ensure blev empty
            DEBUG(log, out).print("Deleting blev at index %lu\n", i);
            DEL_BLEV
        }
        // Next blev
    }
    DEBUG(log, out).print("cleaned = %d\n", cleaned);
    return cleaned;
}
#undef DEL_BLEV

command_result cleanmap(color_ostream &out, const cuboid &bounds, const clean_options &options)
{   // Invoked from clean(), already suspended
    DEBUG(log, out).print("Cleaning map...\n");
    cuboid my_bounds; // Local copy
    if (bounds.isValid())
        my_bounds = bounds;
    else
    {   // Do full map
        my_bounds.addPos(0, 0, 0);
        my_bounds.addPos(world->map.x_count-1,
            world->map.y_count-1, world->map.z_count-1);
        DEBUG(log, out).print("Invalid cuboid, selecting full map.\n");
    }
    int num_blocks = 0, max_blocks = 0;

    my_bounds.forBlock([&](df::map_block *block, cuboid inter) {
        num_blocks += clean_block(out, block, inter, options);
        max_blocks++;
        return true; // Next block
    });

    if(num_blocks > 0)
        out.print("Cleaned %d of %d selected map blocks.\n", num_blocks, max_blocks);
    return CR_OK;
}

command_result cleanitems(color_ostream &out, const cuboid &bounds)
{   // Invoked from clean(), already suspended
    DEBUG(log, out).print("Cleaning items...\n");
    bool valid_cuboid = bounds.isValid(); // Allow for items outside map if false
    int cleaned_items = 0, cleaned_total = 0;
    for (auto i : world->items.other.IN_PLAY)
    {
        TRACE(log, out).print("Considering item #%d\n", i->id);
        auto item = virtual_cast<df::item_actual>(i);
        if (!item)
        {
            out.printerr("Item #%d isn't item_actual!\n", i->id);
            continue;
        }
        else if (valid_cuboid)
        {   // Check if item is inside cuboid
            auto pos = Items::getPosition(item);
            if (!pos.isValid() || !bounds.containsPos(pos))
                continue;
        }
        TRACE(log, out).print("Selected\n");

        if (item->contaminants && !item->contaminants->empty())
        {
            vector<df::spatter *> saved;
            for (size_t j = 0; j < item->contaminants->size(); j++)
            {
                auto obj = (*item->contaminants)[j];
                if (obj->flags.whole & 0x8000) // DFHack-generated contaminant
                    saved.push_back(obj);
                else
                    delete obj;
            }
            cleaned_items++;
            cleaned_total += item->contaminants->size() - saved.size();
            item->contaminants->swap(saved);
            DEBUG(log, out).print("Cleaned item #%d\n", item->id);
        }
    }
    if (cleaned_total > 0)
        out.print("Removed %d contaminants from %d items.\n", cleaned_total, cleaned_items);
    return CR_OK;
}

command_result cleanunits(color_ostream &out, const cuboid &bounds)
{   // Invoked from clean(), already suspended
    DEBUG(log, out).print("Cleaning units...\n");
    bool valid_cuboid = bounds.isValid(); // Allow for dead/inactive units if false
    int cleaned_units = 0, cleaned_total = 0;
    for (auto unit : world->units.active)
    {
        TRACE(log, out).print("Considering unit #%d\n", unit->id);
        if (valid_cuboid)
        {   // Check if unit is inside cuboid
            if (!Units::isActive(unit))
                continue; // Dead or off map
            auto pos = Units::getPosition(unit);
            if (!pos.isValid() || !bounds.containsPos(pos))
                continue;
        }
        TRACE(log, out).print("Selected\n");

        if (!unit->body.spatters.empty())
        {
            for (size_t j = 0; j < unit->body.spatters.size(); j++)
                delete unit->body.spatters[j];
            cleaned_units++;
            cleaned_total += unit->body.spatters.size();
            unit->body.spatters.clear();
            DEBUG(log, out).print("Cleaned unit #%d\n", unit->id);
        }
    }
    if (cleaned_total > 0)
        out.print("Removed %d contaminants from %d creatures.\n", cleaned_total, cleaned_units);
    return CR_OK;
}

command_result spotclean(color_ostream &out, vector<string> &parameters)
{
    DEBUG(log, out).print("Doing spotclean.\n");
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available.\n");
        return CR_FAILURE;
    }
    auto pos = Gui::getCursorPos();
    if (!pos.isValid())
    {
        out.printerr("The keyboard cursor is not active.\n");
        return CR_WRONG_USAGE;
    }
    auto block = Maps::getTileBlock(pos);
    if (!block)
    {
        out.printerr("Invalid map block selected!\n");
        return CR_FAILURE;
    }
    clean_options options;
    options.mud = true;
    options.snow = true;

    clean_block(out, block, cuboid(pos), options);
    return CR_OK;
}

command_result clean(color_ostream &out, vector<string> &parameters)
{
    clean_options options;
    df::coord pos_1, pos_2;
    cuboid bounds;

    if (!Lua::CallLuaModuleFunction(out, "plugins.cleaners", "parse_commandline",
        std::make_tuple(&options, &pos_1, &pos_2, parameters)))
    {
        return CR_WRONG_USAGE;
    }

    DEBUG(log, out).print("pos_1 = (%d, %d, %d)\npos_2 = (%d, %d, %d)\n",
        pos_1.x, pos_1.y, pos_1.z, pos_2.x, pos_2.y, pos_2.z);

    bool map_target = options.mud || options.snow || options.item_spat || options.grass;
    if (!options.map)
    {
        if (!options.units && !options.items)
        {
            out.printerr("Choose at least: --map, --units, or --items. Use --all for all.\n");
            return CR_WRONG_USAGE;
        }
        else if (options.item_spat && !options.items)
        {
            out.printerr("Must use --map (or --all) with --item. Did you mean --items?\n");
            return CR_WRONG_USAGE;
        }
        else if (map_target || options.only)
        {
            out.printerr("Must use --map (or --all) with --mud, --snow, --item, --grass, or --only.\n");
            return CR_WRONG_USAGE;
        }
    }

    if (options.desolate && !options.grass)
    {
        out.printerr("Must use --grass with --desolate. This kills grass!\n");
        return CR_WRONG_USAGE;
    }
    else if (options.only && !map_target)
    {
        out.printerr("Specified --only for map, but there's nothing else to do.\n");
        return CR_WRONG_USAGE;
    }
    else if (!Maps::IsValid())
    {
        out.printerr("Map not loaded!\n");
        return CR_FAILURE;
    }

    if (options.zlevel)
    {   // Specified z-levels or viewport z
        auto z1 = pos_1.isValid() ? pos_1.z : Gui::getViewportPos().z;
        auto z2 = pos_2.isValid() ? pos_2.z : z1;
        DEBUG(log, out).print("Selecting z-levels %d to %d\n", z1, z2);
        bounds.addPos(0, 0, z1);
        bounds.addPos(world->map.x_count-1, world->map.y_count-1, z2);
    }
    else if (pos_1.isValid())
    {   // Point or cuboid
        DEBUG(log, out).print("Selecting %s.\n", pos_2.isValid() ? "cuboid" : "point");
        bounds.addPos(pos_1);
        bounds.addPos(pos_2); // Ignored if invalid

        if (!bounds.clampMap().isValid()) // Clamp to map, check selection
        {   // No intersection. Don't don't entire map, just fail
            out.printerr("Invalid position!\n");
            return CR_FAILURE;
        }
    }
    else
    {   // Entire map (plus units and items outside map edge)
        DEBUG(log, out).print("Selecting entire map.\n");
    }
    DEBUG(log, out).print("bounds = (%d:%d, %d:%d, %d:%d)\n",
        bounds.x_min, bounds.x_max, bounds.y_min, bounds.y_max, bounds.z_min, bounds.z_max);

    if(options.map)
        cleanmap(out, bounds, options);
    if(options.units)
        cleanunits(out, bounds);
    if(options.items)
        cleanitems(out, bounds);
    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "clean",
        "Removes contaminants.",
        clean));
    commands.push_back(PluginCommand(
        "spotclean",
        "Remove contaminants from the tile under the cursor.",
        spotclean, Gui::cursor_hotkey));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    return CR_OK;
}
