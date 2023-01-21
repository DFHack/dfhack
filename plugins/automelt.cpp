
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/Items.h"
#include "modules/World.h"
#include "modules/Designations.h"
#include "modules/Persistence.h"
#include "modules/Units.h"

// #include "uicommon.h"

#include "df/world.h"
#include "df/building.h"
#include "df/world_raws.h"
#include "df/building_def.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/building_stockpilest.h"
#include "df/plotinfost.h"
#include "df/item_quality.h"

#include <map>
#include <unordered_map>

using df::building_stockpilest;
using std::map;
using std::multimap;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("automelt");
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cursor);
REQUIRE_GLOBAL(plotinfo);

namespace DFHack
{
    DBG_DECLARE(automelt, status, DebugCategory::LINFO);
    DBG_DECLARE(automelt, cycle, DebugCategory::LINFO);
    DBG_DECLARE(automelt, perf, DebugCategory::LTRACE);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static const string STOCKPILE_CONFIG_KEY_PREFIX = string(plugin_name) + "/stockpile/";
static PersistentDataItem config;

static vector<PersistentDataItem> watched_stockpiles;
static unordered_map<int, size_t> watched_stockpiles_indices;


enum ConfigValues
{
    CONFIG_IS_ENABLED = 0,

};

enum StockpileConfigValues
{
    STOCKPILE_CONFIG_ID = 0,
    STOCKPILE_CONFIG_MONITORED = 1,
    STOCKPILE_CONFIG_MELT = 2,

};

static int get_config_val(PersistentDataItem &c, int index)
{
    if (!c.isValid())
        return -1;
    return c.ival(index);
}
static bool get_config_bool(PersistentDataItem &c, int index)
{
    return get_config_val(c, index) == 1;
}
static void set_config_val(PersistentDataItem &c, int index, int value)
{
    if (c.isValid())
        c.ival(index) = value;
}
static void set_config_bool(PersistentDataItem &c, int index, bool value)
{
    set_config_val(c, index, value ? 1 : 0);
}

static PersistentDataItem &ensure_stockpile_config(color_ostream &out, int id)
{
    if (watched_stockpiles_indices.count(id))
        return watched_stockpiles[watched_stockpiles_indices[id]];
    string keyname = STOCKPILE_CONFIG_KEY_PREFIX + int_to_string(id);
    DEBUG(status, out).print("creating new persistent key for stockpile %d\n", id);
    watched_stockpiles.emplace_back(World::GetPersistentData(keyname, NULL));
    size_t idx = watched_stockpiles.size() - 1;
    watched_stockpiles_indices.emplace(id, idx);
    return watched_stockpiles[idx];
}

static void remove_stockpile_config(color_ostream &out, int id)
{
    if (!watched_stockpiles_indices.count(id))
        return;
    DEBUG(status, out).print("removing persistent key for stockpile %d\n", id);
    size_t idx = watched_stockpiles_indices[id];
    World::DeletePersistentData(watched_stockpiles[idx]);
    watched_stockpiles.erase(watched_stockpiles.begin() + idx);
    watched_stockpiles_indices.erase(id);
}

static void validate_stockpile_configs(color_ostream &out)
{
    for (auto &c : watched_stockpiles) {
        int id = get_config_val(c, STOCKPILE_CONFIG_ID);
        if (!df::building::find(id)){
            remove_stockpile_config(out, id);
        }
    }
}

static const int32_t CYCLE_TICKS = 1200;
static int32_t cycle_timestamp = 0; // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static int32_t do_cycle(color_ostream &out);

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands)
{
    DEBUG(status, out).print("initializing %s\n", plugin_name);

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
        "Auto melt items in designated stockpiles.",
        do_command));

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!Core::getInstance().isWorldLoaded())
    {
        out.printerr("Cannot enable %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled)
    {
        is_enabled = enable;
        DEBUG(status, out).print("%s from the API; persisting\n", is_enabled ? "enabled" : "disabled");
        set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
    }
    else
    {
        DEBUG(status, out).print("%s from the API, but already %s; no action\n", is_enabled ? "enabled" : "disabled", is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    DEBUG(status, out).print("shutting down %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_load_data(color_ostream &out)
{
    config = World::GetPersistentData(CONFIG_KEY);

    if (!config.isValid())
    {
        DEBUG(status, out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
        set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
    }

    // we have to copy our enabled flag into the global plugin variable, but
    // all the other state we can directly read/modify from the persistent
    // data structure.
    is_enabled = get_config_bool(config, CONFIG_IS_ENABLED);
    DEBUG(status, out).print("loading persisted enabled state: %s\n", is_enabled ? "true" : "false");
    World::GetPersistentData(&watched_stockpiles, STOCKPILE_CONFIG_KEY_PREFIX, true);
    watched_stockpiles_indices.clear();
    const size_t num_watched_stockpiles = watched_stockpiles.size();
    for (size_t idx = 0; idx < num_watched_stockpiles; ++idx)
    {
        auto &c = watched_stockpiles[idx];
        watched_stockpiles_indices.emplace(get_config_val(c, STOCKPILE_CONFIG_ID), idx);
    }
    validate_stockpile_configs(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    if (event == DFHack::SC_WORLD_UNLOADED)
    {
        if (is_enabled)
        {
            DEBUG(status, out).print("world unloaded; disabling %s\n", plugin_name);
            is_enabled = false;
        }
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
    {
        int32_t marked = do_cycle(out);
        if (0 < marked)
            out.print("automelt: marked %d item(s) for melting\n", marked);
    }
    return CR_OK;
}


static bool call_automelt_lua(color_ostream *out, const char *fn_name,
        int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA) {
    DEBUG(status).print("calling automelt lua function: '%s'\n", fn_name);

    CoreSuspender guard;

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!out)
        out = &Core::getInstance().getConsole();

    return Lua::CallLuaModuleFunction(*out, L, "plugins.automelt", fn_name,
            nargs, nres,
            std::forward<Lua::LuaLambda&&>(args_lambda),
            std::forward<Lua::LuaLambda&&>(res_lambda));
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot run %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!call_automelt_lua(&out, "parse_commandline", parameters.size(), 1,
            [&](lua_State *L) {
                for (const string &param : parameters)
                    Lua::Push(L, param);
            },
            [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

static inline bool is_metal_item(df::item *item)
{
    MaterialInfo mat(item);
    return (mat.getCraftClass() == craft_material_class::Metal);
}

static bool isStockpile(df::building * building) {
    return building->getType() == df::building_type::Stockpile;
}

// Copied from Kelly Martin's code
static inline bool can_melt(df::item *item)
{

    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
    F(dump);
    F(forbid);
    F(garbage_collect);
    F(in_job);
    F(hostile);
    F(on_fire);
    F(rotten);
    F(trader);
    F(in_building);
    F(construction);
    F(artifact);
    F(melt);
#undef F

    if (item->flags.whole & bad_flags.whole)
        return false;

    df::item_type t = item->getType();

    if (t == df::enums::item_type::BOX || t == df::enums::item_type::BAR)
        return false;

    if (!is_metal_item(item))
        return false;

    for (auto g = item->general_refs.begin(); g != item->general_refs.end(); g++)
    {
        switch ((*g)->getType())
        {
        case general_ref_type::CONTAINS_ITEM:
        case general_ref_type::UNIT_HOLDER:
        case general_ref_type::CONTAINS_UNIT:
            return false;
        case general_ref_type::CONTAINED_IN_ITEM:
        {
            df::item *c = (*g)->getItem();
            for (auto gg = c->general_refs.begin(); gg != c->general_refs.end(); gg++)
            {
                if ((*gg)->getType() == general_ref_type::UNIT_HOLDER)
                    return false;
            }
        }
        break;
        default:
            break;
        }
    }

    if (item->getQuality() >= item_quality::Masterful)
        return false;

    return true;
}

static inline bool is_set_to_melt(df::item *item)
{
    return item->flags.bits.melt;
}

static int mark_item(color_ostream &out, df::item *item, df::item_flags bad_flags, int32_t stockpile_id, int32_t &premarked_item_count, int32_t &item_count, bool should_melt)
{
    TRACE(perf,out).print("%s running mark_item\nshould_melt=%d\n", plugin_name,should_melt);
    string name = "";
    item->getItemDescription(&name, 0);
    TRACE(perf,out).print("item %s %d\n", name.c_str(), item->id);
    if (item->flags.whole & bad_flags.whole){
        TRACE(perf,out).print("rejected flag check\n");
        item_count++;
        return 0;
    }

    if (item->isAssignedToThisStockpile(stockpile_id))
    {
        TRACE(perf,out).print("assignedToStockpile\n");
        size_t marked_count = 0;
        std::vector<df::item *> contents;
        Items::getContainedItems(item, &contents);
        for (auto child = contents.begin(); child != contents.end(); child++)
        {
            TRACE(perf,out).print("inside child loop\n");
            marked_count += mark_item(out, *child, bad_flags, stockpile_id, premarked_item_count, item_count, should_melt);
        }
        return marked_count;
    }

    TRACE(perf,out).print("past recurse loop\n");

    if (is_set_to_melt(item)) {
        TRACE(perf,out).print("already set to melt\n");
        premarked_item_count++;
        item_count++;
        return 0;
    }

    if (!can_melt(item)) {
        TRACE(perf,out).print("cannot melt\n");
        item_count++;
        return 0;
    }

    TRACE(perf,out).print("increment item count\n");
    item_count++;

    //Only melt if told to, else count
    if (should_melt) {
        TRACE(perf,out).print("should_melt hit\n");
        insert_into_vector(world->items.other[items_other_id::ANY_MELT_DESIGNATED], &df::item::id, item);
        item->flags.bits.melt = 1;
        return 1;
    } else {
        return 0;
    }

}


static int32_t mark_all_in_stockpile(color_ostream &out, PersistentDataItem & stockpile, int32_t &premarked_item_count, int32_t &item_count, bool should_melt)
{
    TRACE(perf,out).print("%s running mark_all_in_stockpile\nshould_melt=%d", plugin_name, should_melt);
    // Precompute a bitmask with the bad flags
    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
    F(dump);
    F(forbid);
    F(garbage_collect);
    F(hostile);
    F(on_fire);
    F(rotten);
    F(trader);
    F(in_building);
    F(construction);
    F(artifact);
    F(spider_web);
    F(owned);
    F(in_job);
#undef F

    int32_t marked_count = 0;

    if(!stockpile.isValid()) {
        return 0;
    }

    int spid = get_config_val(stockpile, STOCKPILE_CONFIG_ID);
    auto found = df::building::find(spid);
    if (!isStockpile(found)){

        return 0;
    }

    df::building_stockpilest * pile_cast = virtual_cast<df::building_stockpilest>(found);

    if (!pile_cast)
        return 0;

    Buildings::StockpileIterator stored;
    for (stored.begin(pile_cast); !stored.done(); ++stored) {

        marked_count += mark_item(out, *stored, bad_flags, spid, premarked_item_count, item_count, should_melt);
    }
    TRACE(perf,out).print("marked_count %d\npremarked_count %d\n", marked_count, premarked_item_count);
    return marked_count;
}


static int32_t scan_stockpiles(color_ostream &out, bool disable_melt, map<int32_t, int32_t> &item_counts, map<int32_t, int32_t> &marked_item_counts, map<int32_t, int32_t> &premarked_item_counts) {
    TRACE(perf, out).print("%s running scan_stockpiles\n", plugin_name);
    int32_t newly_marked = 0;

    // if (item_counts)
    item_counts.clear();
    // if (marked_item_counts)
    marked_item_counts.clear();
    // if (premarked_item_counts)
    premarked_item_counts.clear();


    //Parse all the watched piles
    for (auto &c : watched_stockpiles) {
        int id = get_config_val(c, STOCKPILE_CONFIG_ID);
        //Check for monitor status
        bool monitored = get_config_bool(c, STOCKPILE_CONFIG_MONITORED);
        //Check melt status
        bool melt_enabled = get_config_bool(c, STOCKPILE_CONFIG_MELT);

        melt_enabled = melt_enabled && (!disable_melt);

        if (!monitored) continue;

        TRACE(perf,out).print("%s past monitor check\nmelt_enabled=%d", plugin_name, melt_enabled);

        int32_t item_count = 0;

        int32_t premarked_count = 0;

        int32_t marked = mark_all_in_stockpile(out, c, premarked_count, item_count, melt_enabled);

        item_counts.emplace(id, item_count);

        marked_item_counts.emplace(id, marked);

        premarked_item_counts.emplace(id, premarked_count);

        newly_marked += marked;

    }

    return newly_marked;
}


static int32_t do_cycle(color_ostream &out) {
    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);
    cycle_timestamp = world->frame_counter;

    validate_stockpile_configs(out);

    int32_t marked_items_total;
    map<int32_t, int32_t> item_counts, marked_items_counts, premarked_item_counts;

    marked_items_total = scan_stockpiles(out, false, item_counts, marked_items_counts, premarked_item_counts);

    return marked_items_total;
}

static void push_stockpile_config(lua_State *L, int id, bool monitored,
        bool melt) {
    map<string, int32_t> stockpile_config;
    stockpile_config.emplace("id", id);
    stockpile_config.emplace("monitored", monitored);
    stockpile_config.emplace("melt", melt);

    Lua::Push(L, stockpile_config);
}

static void push_stockpile_config(lua_State *L, PersistentDataItem &c) {
    push_stockpile_config(L, get_config_val(c, STOCKPILE_CONFIG_ID),
            get_config_bool(c, STOCKPILE_CONFIG_MONITORED),
            get_config_bool(c, STOCKPILE_CONFIG_MELT));
}

static void automelt_designate(color_ostream &out) {
    DEBUG(status, out).print("entering automelt designate\n");
    out.print("designated %d item(s) for melting\n", do_cycle(out));
}

static void automelt_printStatus(color_ostream &out) {
    DEBUG(status,out).print("entering automelt_printStatus\n");
    validate_stockpile_configs(out);
    out.print("automelt is %s\n\n", is_enabled ? "enabled" : "disabled");

    map<int32_t, int32_t> item_counts, marked_item_counts, premarked_item_counts;
    scan_stockpiles(out, true, item_counts, marked_item_counts, premarked_item_counts);

    int32_t total_items_all_piles = 0;
    int32_t premarked_items_all_piles = 0;


    for (auto &i : watched_stockpiles) {
        int id = get_config_val(i, STOCKPILE_CONFIG_ID);
        total_items_all_piles+= item_counts[id];
        premarked_items_all_piles += premarked_item_counts[id];
    }

    out.print("summary:\n");
    out.print("    total items in monitored piles: %d\n", total_items_all_piles);
    out.print("   marked items in monitored piles: %d\n", premarked_items_all_piles);

    int name_width = 11;
    for (auto &stockpile : world->buildings.other.STOCKPILE) {
        if (!isStockpile(stockpile)) continue;
        name_width = std::max(name_width, (int)stockpile->name.size());
    }
    name_width = -name_width;

    const char *fmt = "%*s  %4s  %4s  %4s\n";
    out.print(fmt, name_width, "name", " id ", "monitored", "melt");
    out.print(fmt, name_width, "----", "----", "---------", "----");

    for (auto &stockpile : world->buildings.other.STOCKPILE) {
        if (!isStockpile(stockpile)) continue;
        bool melt = false;
        bool monitored = false;
        if (watched_stockpiles_indices.count(stockpile->id)) {
            auto &c = watched_stockpiles[watched_stockpiles_indices[stockpile->id]];
            melt = get_config_bool(c, STOCKPILE_CONFIG_MELT);
            monitored = get_config_bool(c, STOCKPILE_CONFIG_MONITORED);
        }

        out.print(fmt, name_width, stockpile->name.c_str(), int_to_string(stockpile->id).c_str(),
                  monitored ? "[x]" : "[ ]", melt ? "[x]": "[ ]");
    }

}

static void automelt_setStockpileConfig(color_ostream &out, int id, bool monitored, bool melt) {
    DEBUG(status,out).print("entering automelt_setStockpileConfig\n");
    validate_stockpile_configs(out);
    bool isInvalidStockpile = !df::building::find(id);
    bool hasNoData = !melt && !monitored;
    if (isInvalidStockpile || hasNoData) {
        remove_stockpile_config(out, id);
        return;
    }

    PersistentDataItem &c = ensure_stockpile_config(out, id);
    set_config_val(c, STOCKPILE_CONFIG_ID, id);
    set_config_bool(c, STOCKPILE_CONFIG_MONITORED, monitored);
    set_config_bool(c, STOCKPILE_CONFIG_MELT, melt);
}

static int automelt_getStockpileConfig(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status, *out).print("entering automelt_getStockpileConfig\n");
    validate_stockpile_configs(*out);

    int id;
    if (lua_isnumber(L, -1)) {
        id = lua_tointeger(L, -1);
        if (!df::building::find(id))
            return 0;

    } else {
        const char * name = lua_tostring(L, -1);
        if (!name)
            return 0;
        string nameStr = name;
        bool found = false;
        for (auto &stockpile : world->buildings.other.STOCKPILE) {
            if (!isStockpile(stockpile)) continue;
            if (nameStr == stockpile->name) {
                id = stockpile->id;
                found = true;
                break;
            }

        }
        if (!found)
            return 0;
    }

    if (watched_stockpiles_indices.count(id)) {
        push_stockpile_config(L, watched_stockpiles[watched_stockpiles_indices[id]]);
    } else {
        push_stockpile_config(L, id, false, false);
    }
    return 1;
}

//TODO
static int automelt_getItemCountsAndStockpileConfigs(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering automelt_getItemCountsAndStockpileConfigs\n");
    validate_stockpile_configs(*out);

    map<int32_t, int32_t> item_counts, marked_item_counts, premarked_item_counts;
    int32_t marked_items = scan_stockpiles(*out, true, item_counts, marked_item_counts, premarked_item_counts);

    int32_t total_items_all_piles = 0;
    int32_t premarked_items_all_piles = 0;


    for (auto &i : watched_stockpiles) {
        int id = get_config_val(i, STOCKPILE_CONFIG_ID);
        total_items_all_piles+= item_counts[id];
        premarked_items_all_piles += premarked_item_counts[id];
    }

    map<string, int32_t> summary;
    summary.emplace("total_items", total_items_all_piles);
    summary.emplace("marked_items", marked_items);
    summary.emplace("premarked_items", premarked_items_all_piles);

    Lua::Push(L, summary);
    Lua::Push(L, item_counts);
    Lua::Push(L, marked_item_counts);
    Lua::Push(L, premarked_item_counts);
    int32_t bldg_count = 0;

    for (auto pile : world->buildings.other.STOCKPILE) {
        if (!isStockpile(pile))
            continue;
        bldg_count++;

        int id = pile->id;
        if (watched_stockpiles_indices.count(id)) {
            push_stockpile_config(L, watched_stockpiles[watched_stockpiles_indices[id]]);
        } else {
            push_stockpile_config(L, id, false, false);
        }
    }
    return 4+bldg_count;


}

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(automelt_printStatus),
    DFHACK_LUA_FUNCTION(automelt_designate),
    DFHACK_LUA_FUNCTION(automelt_setStockpileConfig),
    DFHACK_LUA_END};

DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(automelt_getStockpileConfig),
    DFHACK_LUA_COMMAND(automelt_getItemCountsAndStockpileConfigs),
    DFHACK_LUA_END};
    