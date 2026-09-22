
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

#include "modules/EventManager.h"
#include "modules/Maps.h"
#include "modules/World.h"

#include "df/block_column_print_infost.h"
#include "df/construction.h"
#include "df/entity_plot_invasion_mapst.h"
#include "df/global_objects.h"
#include "df/historical_entity.h"
#include "df/invasion_info.h"
#include "df/map_block.h"
#include "df/map_block_column.h"
#include "df/plotinfost.h"
#include "df/plot_invasion_mapst.h"
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

REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);

namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(infinitesky, control, DebugCategory::LINFO);
    // for logging during creation of z-levels
    DBG_DECLARE(infinitesky, cycle, DebugCategory::LINFO);
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
        out.printerr("Cannot enable {} without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }
    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control, out)
            .print("{} from the API; persisting\n",
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
            .print("{} from the API, but already {}; no action\n",
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
    if (config.get_bool(CONFIG_IS_ENABLED)) {
        plugin_enable(out, true);
    }
    DEBUG(control, out)
        .print("loading persisted enabled state: {}\n",
               is_enabled ? "true" : "false");
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out,
                                                  state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(control, out)
                .print("world unloaded; disabling {}\n", plugin_name);
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


void doInfiniteSky(color_ostream& out, int32_t quantity)
{
    Maps::addBlockColumns(world->map.z_count_block + quantity);
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
    {struct_field_info::PRIMITIVE, "n", offsetof(infinitesky_options, n), &df::identity_traits<int32_t>::identity, 0, 0},
    {struct_field_info::END}
};
struct_identity infinitesky_options::_identity{sizeof(infinitesky_options), &df::allocator_fn<infinitesky_options>, NULL, "infinitesky_options", NULL, infinitesky_options_fields};

command_result infiniteSky(color_ostream &out,
                           std::vector<std::string> &parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run {} without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    infinitesky_options opts;
    if (!Lua::CallLuaModuleFunction(out, "plugins.infinite-sky",
                                    "parse_commandline",
                                    std::make_tuple(&opts, parameters)) ||
        opts.help)
        return CR_WRONG_USAGE;

    if (opts.n > 0) {
        out.print("Infinite-sky: creating {} new z-level{} of sky.\n", opts.n,
                  opts.n == 1 ? "" : "s");
        doInfiniteSky(out, opts.n);
    } else {
        out.print("Construction monitoring is {}.\n",
                  is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}
