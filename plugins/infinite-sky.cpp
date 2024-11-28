
#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/Maps.h"
#include "modules/World.h"

#include "df/construction.h"
#include "df/map_block.h"
#include "df/map_block_column.h"
#include "df/world.h"
#include "df/z_level_flags.h"

#include <cstring>
#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("infinite-sky");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(infiniteSky, control, DebugCategory::LINFO);
    // for logging during creation of z-levels
    DBG_DECLARE(infiniteSky, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;
enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
};

command_result infiniteSky (color_ostream &out, std::vector <std::string> & parameters);

static void constructionEventHandler(color_ostream& out, void* ptr);
EventManager::EventHandler handler(plugin_self, constructionEventHandler,11);

DFhackCExport command_result plugin_init(color_ostream &out,
                                         std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "infinite-sky", "Automatically allocate new z-levels of sky.",
        infiniteSky));
    return CR_OK;
}

void cleanup() {
    EventManager::unregister(EventManager::EventType::CONSTRUCTION, handler);
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }
    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control, out)
            .print("%s from the API; persisting\n",
                   is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);

        if (enable) {
            EventManager::registerListener(
                EventManager::EventType::CONSTRUCTION, handler);
        } else {
            cleanup();
        }
    } else {
        DEBUG(control, out)
            .print("%s from the API, but already %s; no action\n",
                   is_enabled ? "enabled" : "disabled",
                   is_enabled ? "enabled" : "disabled");
    }

    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data(color_ostream &out) {
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control, out)
            .print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
    }

    // Call plugin_enable to set value to ensure the event handler is properly registered
    plugin_enable(out, config.get_bool(CONFIG_IS_ENABLED));
    DEBUG(control, out)
        .print("loading persisted enabled state: %s\n",
               is_enabled ? "true" : "false");
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out,
                                                  state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(control, out)
                .print("world unloaded; disabling %s\n", plugin_name);
            is_enabled = false;
            cleanup();
        }
    }
    return CR_OK;
}

void doInfiniteSky(color_ostream& out, int32_t howMany);

static void constructionEventHandler(color_ostream &out, void *ptr) {
    df::construction *constr = (df::construction *)ptr;

    if (constr->pos.z >= world->map.z_count_block - 2)
        doInfiniteSky(out, 1);
}

void doInfiniteSky(color_ostream& out, int32_t howMany) {
    CoreSuspender suspend;
    int32_t z_count_block = world->map.z_count_block;
    df::map_block ****block_index = world->map.block_index;

    cuboid last_air_layer(
        0, 0, world->map.z_count_block - 1,
        world->map.x_count_block - 1, world->map.y_count_block - 1, world->map.z_count_block - 1);

    last_air_layer.forCoord([&](df::coord bpos) {
        // Allocate a new block column and copy over data from the old
        df::map_block **blockColumn =
            new df::map_block *[z_count_block + howMany];
        memcpy(blockColumn, block_index[bpos.x][bpos.y],
               z_count_block * sizeof(df::map_block *));
        delete[] block_index[bpos.x][bpos.y];
        block_index[bpos.x][bpos.y] = blockColumn;

        df::map_block *last_air_block = blockColumn[bpos.z];
        for (int32_t count = 0; count < howMany; count++) {
            df::map_block *air_block = new df::map_block();
            std::fill(&air_block->tiletype[0][0],
                      &air_block->tiletype[0][0] + (16 * 16),
                      df::tiletype::OpenSpace);

            // Set block positions properly (based on prior air layer)
            air_block->map_pos = last_air_block->map_pos;
            air_block->map_pos.z += count + 1;
            air_block->region_pos = last_air_block->region_pos;

            // Copy other potentially important metadata from prior air
            // layer
            std::memcpy(air_block->lighting, last_air_block->lighting,
                        sizeof(air_block->lighting));
            std::memcpy(air_block->temperature_1, last_air_block->temperature_1,
                        sizeof(air_block->temperature_1));
            std::memcpy(air_block->temperature_2, last_air_block->temperature_2,
                        sizeof(air_block->temperature_2));
            std::memcpy(air_block->region_offset, last_air_block->region_offset,
                        sizeof(air_block->region_offset));

            // Create tile designations to inform lighting and
            // outside markers
            df::tile_designation designation{};
            designation.bits.light = true;
            designation.bits.outside = true;
            std::fill(&air_block->designation[0][0],
                      &air_block->designation[0][0] + (16 * 16), designation);

            blockColumn[z_count_block + count] = air_block;
            world->map.map_blocks.push_back(air_block);

            // deal with map_block_column stuff even though it'd probably be
            // fine
            df::map_block_column *column =
                world->map.column_index[bpos.x][bpos.y];
            if (!column) {
                DEBUG(cycle, out)
                    .print("%s, line %d: column is null (%d, %d).\n", __FILE__,
                           __LINE__, bpos.x, bpos.y);
                continue;
            }
            df::map_block_column::T_unmined_glyphs *glyphs =
                new df::map_block_column::T_unmined_glyphs;
            glyphs->x[0] = 0;
            glyphs->x[1] = 1;
            glyphs->x[2] = 2;
            glyphs->x[3] = 3;
            glyphs->y[0] = 0;
            glyphs->y[1] = 0;
            glyphs->y[2] = 0;
            glyphs->y[3] = 0;
            glyphs->tile[0] = 'e';
            glyphs->tile[1] = 'x';
            glyphs->tile[2] = 'p';
            glyphs->tile[3] = '^';
            column->unmined_glyphs.push_back(glyphs);
        }
        return true;
    });

    // Update global z level flags
    df::z_level_flags *flags = new df::z_level_flags[z_count_block + howMany];
    memcpy(flags, world->map_extras.z_level_flags,
           z_count_block * sizeof(df::z_level_flags));
    for (int32_t count = 0; count < howMany; count++) {
        flags[z_count_block + count].whole = 0;
        flags[z_count_block + count].bits.update = 1;
    }
    world->map.z_count_block += howMany;
    world->map.z_count += howMany;
    delete[] world->map_extras.z_level_flags;
    world->map_extras.z_level_flags = flags;
}

struct infinitesky_options {
    // whether to display help
    bool help = false;

    // how many z levels to generate immediately (0 for none)
    int32_t n = 0;

    static struct_identity _identity;
};
static const struct_field_info infinitesky_options_fields[] = {
    {struct_field_info::PRIMITIVE, "help", offsetof(infinitesky_options, help), &df::identity_traits<bool>::identity, 0, 0},
    {struct_field_info::PRIMITIVE, "n", offsetof(infinitesky_options, n), &df::identity_traits<int32_t>::identity, 0, 0}
};
struct_identity infinitesky_options::_identity{sizeof(infinitesky_options), &df::allocator_fn<infinitesky_options>, NULL, "infinitesky_options", NULL, infinitesky_options_fields};

command_result infiniteSky(color_ostream &out,
                           std::vector<std::string> &parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    infinitesky_options opts;
    if (!Lua::CallLuaModuleFunction(out, "plugins.infinite-sky",
                                    "parse_commandline",
                                    std::make_tuple(&opts, parameters)) ||
        opts.help)
        return CR_WRONG_USAGE;

    if (opts.n > 0) {
        out.print("Infinite-sky: creating %d new z-level%s of sky.\n", opts.n,
                  opts.n == 1 ? "" : "s");
        doInfiniteSky(out, opts.n);
    } else {
        out.print("Construction monitoring is %s.\n",
                  is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}
