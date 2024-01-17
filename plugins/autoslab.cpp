/* Simple plugin to check for ghosts and automatically queue jobs to engrave slabs for them.
 *
 * Enhancement idea: Queue up a ConstructSlab job, then link the engrave slab job to it. Avoids need to have slabs in stockpiles
 *                   Would require argument parsing, specifying materials
 * Enhancement idea: Automatically place the slab. This seems like a tricky problem but maybe solveable with named zones?
 *                   Might be made obsolete by people just using buildingplan to pre-place plans for slab?
 * Enhancement idea: Optionally enable autoengraving for pets.
 * Enhancement idea: Try to get ahead of ghosts by autoengraving for dead dwarves with no remains, or dwarves
 *                   whose remains are unreachable.
 */

#include "Core.h"
#include "Debug.h"
#include "PluginManager.h"

#include "modules/Persistence.h"
#include "modules/Translation.h"
#include "modules/World.h"

#include "df/historical_figure.h"
#include "df/item.h"
#include "df/item_slabst.h"
#include "df/manager_order.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/world.h"

using namespace DFHack;

DFHACK_PLUGIN("autoslab");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

// logging levels can be dynamically controlled with the `debugfilter` command.
namespace DFHack
{
    // for configuration-related logging
    DBG_DECLARE(autoslab, control, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(autoslab, cycle, DebugCategory::LINFO);
}

static const auto CONFIG_KEY = std::string(plugin_name) + "/config";
static PersistentDataItem config;
enum ConfigValues
{
    CONFIG_IS_ENABLED = 0,
};

static int32_t cycle_timestamp = 0; // world->frame_counter at last cycle

static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands)
{
    DEBUG(control, out).print("initializing %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!Core::getInstance().isMapLoaded() || !World::IsSiteLoaded())
    {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled)
    {
        is_enabled = enable;
        DEBUG(control, out).print("%s from the API; persisting\n", is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_cycle(out);
    }
    else
    {
        DEBUG(control, out).print("%s from the API, but already %s; no action\n", is_enabled ? "enabled" : "disabled", is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    DEBUG(control, out).print("shutting down %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data(color_ostream &out)
{
    cycle_timestamp = 0;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid())
    {
        DEBUG(control, out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
    }

    // we have to copy our enabled flag into the global plugin variable, but
    // all the other state we can directly read/modify from the persistent
    // data structure.
    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control, out).print("loading persisted enabled state: %s\n", is_enabled ? "true" : "false");
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    if (event == DFHack::SC_WORLD_UNLOADED)
    {
        if (is_enabled)
        {
            DEBUG(control, out).print("world unloaded; disabling %s\n", plugin_name);
            is_enabled = false;
        }
    }
    return CR_OK;
}

static const int32_t CYCLE_TICKS = 1289;

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    CoreSuspender suspend;
    if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

// Queue up a single order to engrave the slab for the given unit
static void createSlabJob(df::unit *unit)
{
    auto next_id = world->manager_order_next_id++;
    auto order = new df::manager_order();

    order->id = next_id;
    order->job_type = df::job_type::EngraveSlab;
    order->hist_figure_id = unit->hist_figure_id;
    order->amount_left = 1;
    order->amount_total = 1;
    world->manager_orders.push_back(order);
}

static void checkslabs(color_ostream &out)
{
    // Get existing orders for slab engraving as map hist_figure_id -> order ID
    std::map<int32_t, int32_t> histToJob;
    for (auto order : world->manager_orders)
    {
        if (order->job_type == df::job_type::EngraveSlab)
            histToJob[order->hist_figure_id] = order->id;
    }

    // Build list of ghosts
    std::vector<df::unit *> ghosts;
    std::copy_if(world->units.all.begin(), world->units.all.end(),
                 std::back_inserter(ghosts),
                 [](const df::unit *unit)
                 { return unit->flags3.bits.ghostly; });

    for (auto ghost : ghosts)
    {
        // Only create a job is the map has no existing jobs for that historical figure or no existing engraved slabs
        if (histToJob.count(ghost->hist_figure_id) == 0 &&
            !std::any_of(world->items.other.SLAB.begin(),
                        world->items.other.SLAB.end(),
                        [&ghost](df::item_slabst *slab){
                            return slab->engraving_type == df::slab_engraving_type::Memorial && slab->topic == ghost->hist_figure_id;
                        })
            )
        {
            createSlabJob(ghost);
            auto fullName = Translation::TranslateName(&ghost->name, false);
            out.print("Added slab order for ghost %s\n", fullName.c_str());
        }
    }
}

static void do_cycle(color_ostream &out)
{
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    checkslabs(out);
}
