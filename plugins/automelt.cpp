#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Buildings.h"
#include "modules/Items.h"
#include "modules/World.h"
#include "modules/Persistence.h"
#include "modules/Gui.h"

#include "df/world.h"
#include "df/building_stockpilest.h"
#include "df/item_quality.h"

#include <map>
#include <unordered_map>

using std::map;
using std::string;
using std::unordered_map;
using std::vector;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("automelt");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(world);

namespace DFHack
{
    DBG_DECLARE(automelt, status, DebugCategory::LINFO);
    DBG_DECLARE(automelt, cycle, DebugCategory::LINFO);
    DBG_DECLARE(automelt, perf, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static const string STOCKPILE_CONFIG_KEY_PREFIX = string(plugin_name) + "/stockpile/";
static PersistentDataItem config;

static unordered_map<int32_t, PersistentDataItem> watched_stockpiles;

enum StockpileConfigValues
{
    STOCKPILE_CONFIG_ID = 0,
    STOCKPILE_CONFIG_MONITORED = 1,
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
    DEBUG(cycle,out).print("ensuring stockpile config id=%d\n", id);
    if (watched_stockpiles.count(id)){
        DEBUG(cycle,out).print("stockpile exists in watched_indices\n");
        return watched_stockpiles[id];
    }

    string keyname = STOCKPILE_CONFIG_KEY_PREFIX + int_to_string(id);
    DEBUG(status,out).print("creating new persistent key for stockpile %d\n", id);
    watched_stockpiles.emplace(id, World::GetPersistentData(keyname, NULL));
    return watched_stockpiles[id];
}

static void remove_stockpile_config(color_ostream &out, int id)
{
    if (!watched_stockpiles.count(id))
        return;
    DEBUG(status, out).print("removing persistent key for stockpile %d\n", id);
    World::DeletePersistentData(watched_stockpiles[id]);
    watched_stockpiles.erase(id);
}

static bool isStockpile(df::building * bld) {
    return bld && bld->getType() == df::building_type::Stockpile;
}

static void validate_stockpile_configs(color_ostream &out)
{
    for (auto &c : watched_stockpiles) {
        int id = get_config_val(c.second, STOCKPILE_CONFIG_ID);
        auto bld = df::building::find(id);
        if (!isStockpile(bld))
            remove_stockpile_config(out, id);
    }
}

static const int32_t CYCLE_TICKS = 1200;
static int32_t cycle_timestamp = 0; // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static int32_t do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands)
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
    if (enable != is_enabled)
    {
        is_enabled = enable;
        DEBUG(status, out).print("%s from the API; persisting\n", is_enabled ? "enabled" : "disabled");
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
    cycle_timestamp = 0;
    config = World::GetPersistentData(CONFIG_KEY);

    if (!config.isValid())
    {
        DEBUG(status, out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
    }

    DEBUG(status, out).print("loading persisted enabled state: %s\n", is_enabled ? "true" : "false");
    vector<PersistentDataItem> loaded_persist_data;
    World::GetPersistentData(&loaded_persist_data, STOCKPILE_CONFIG_KEY_PREFIX, true);
    watched_stockpiles.clear();
    const size_t num_watched_stockpiles = loaded_persist_data.size();
    for (size_t idx = 0; idx < num_watched_stockpiles; ++idx)
    {
        auto &c = loaded_persist_data[idx];
        watched_stockpiles.emplace(get_config_val(c, STOCKPILE_CONFIG_ID), c);
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
    if (!Core::getInstance().isWorldLoaded())
        return CR_OK;
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
    if (!item)
        return false;
    MaterialInfo mat(item);
    return (mat.getCraftClass() == craft_material_class::Metal);
}

struct BadFlagsCanMelt {
    uint32_t whole;

    BadFlagsCanMelt() {
        df::item_flags flags;
        #define F(x) flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(artifact);
        F(spider_web); F(owned); F(in_job);
        #undef F
        whole = flags.whole;
    }
};

struct BadFlagsMarkItem {
    uint32_t whole;

    BadFlagsMarkItem() {
        df::item_flags flags;
        #define F(x) flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(artifact);
        F(spider_web); F(owned);
        #undef F
        whole = flags.whole;
    }
};

// Copied from Kelly Martin's code
static inline bool can_melt(df::item *item)
{
    static const BadFlagsCanMelt bad_flags;

    if (!is_metal_item(item))
        return false;

    if (item->flags.whole & bad_flags.whole)
        return false;

    df::item_type t = item->getType();

    if (t == df::enums::item_type::BOX || t == df::enums::item_type::BAR)
        return false;

    for (auto &g : item->general_refs)
    {
        switch (g->getType())
        {
        case general_ref_type::CONTAINS_ITEM:
        case general_ref_type::UNIT_HOLDER:
        case general_ref_type::CONTAINS_UNIT:
            return false;
        case general_ref_type::CONTAINED_IN_ITEM:
        {
            df::item *c = g->getItem();
            for (auto &gg : c->general_refs)
            {
                if (gg->getType() == general_ref_type::UNIT_HOLDER)
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

static int mark_item(color_ostream &out, df::item *item, BadFlagsMarkItem bad_flags, int32_t stockpile_id,
                     int32_t &premarked_item_count, int32_t &item_count, map<int32_t, bool> &tracked_item_map, bool should_melt)
{
    DEBUG(perf,out).print("%s running mark_item: should_melt=%d\n", plugin_name, should_melt);

    if (DBG_NAME(perf).isEnabled(DebugCategory::LDEBUG)) {
        string name = "";
        item->getItemDescription(&name, 0);
        DEBUG(perf,out).print("item %s %d\n", name.c_str(), item->id);
    }

    if (item->flags.whole & bad_flags.whole){
        DEBUG(perf,out).print("rejected flag check\n");
        return 0;
    }

    if (item->isAssignedToThisStockpile(stockpile_id))
    {
        DEBUG(perf,out).print("assignedToStockpile\n");
        size_t marked_count = 0;
        vector<df::item *> contents;
        Items::getContainedItems(item, &contents);
        for (auto child = contents.begin(); child != contents.end(); child++)
        {
            DEBUG(perf,out).print("inside child loop\n");
            marked_count += mark_item(out, *child, bad_flags, stockpile_id, premarked_item_count, item_count, tracked_item_map, should_melt);
        }
        return marked_count;
    }

    DEBUG(perf,out).print("past recurse loop\n");

    if (is_set_to_melt(item)) {
        DEBUG(perf,out).print("already set to melt\n");
        tracked_item_map.emplace(item->id, true);
        premarked_item_count++;
        DEBUG(perf,out).print("premarked_item_count=%d\n", premarked_item_count);
        item_count++;
        return 0;
    }

    if (!can_melt(item)) {
        DEBUG(perf,out).print("cannot melt\n");
        return 0;
    }

    DEBUG(perf,out).print("increment item count\n");
    item_count++;

    //Only melt if told to, else count
    if (should_melt) {
        DEBUG(perf,out).print("should_melt hit\n");
        insert_into_vector(world->items.other[items_other_id::ANY_MELT_DESIGNATED], &df::item::id, item);
        item->flags.bits.melt = 1;
        tracked_item_map.emplace(item->id, true);
        return 1;
    } else {
        return 0;
    }

}

static int32_t mark_all_in_stockpile(color_ostream &out, PersistentDataItem & stockpile, int32_t &premarked_item_count, int32_t &item_count,  map<int32_t, bool> &tracked_item_map, bool should_melt)
{
    DEBUG(perf,out).print("%s running mark_all_in_stockpile\nshould_melt=%d\n", plugin_name, should_melt);

    static const BadFlagsMarkItem bad_flags;

    int32_t marked_count = 0;

    if(!stockpile.isValid()) {
        return 0;
    }

    int spid = get_config_val(stockpile, STOCKPILE_CONFIG_ID);
    auto found = df::building::find(spid);
    if (!isStockpile(found))
        return 0;

    df::building_stockpilest * pile_cast = virtual_cast<df::building_stockpilest>(found);

    if (!pile_cast)
        return 0;

    Buildings::StockpileIterator stored;
    DEBUG(perf,out).print("starting item iter. loop\n");
    for (stored.begin(pile_cast); !stored.done(); ++stored) {
        DEBUG(perf,out).print("calling mark_item\n");
        marked_count += mark_item(out, *stored, bad_flags, spid, premarked_item_count, item_count, tracked_item_map, should_melt);
        DEBUG(perf,out).print("end mark_item call\npremarked_item_count=%d\n", premarked_item_count);
    }
    DEBUG(perf,out).print("end item iter. loop\n");
    DEBUG(perf,out).print("exit mark_all_in_stockpile\nmarked_count %d\npremarked_count %d\n", marked_count, premarked_item_count);
    return marked_count;
}

static int32_t scan_stockpiles(color_ostream &out, bool should_melt, map<int32_t, int32_t> &item_count_piles, map<int32_t, int32_t> &premarked_item_count_piles,
                                                                map<int32_t, int32_t> &marked_item_count_piles, map<int32_t, bool> &tracked_item_map) {
    DEBUG(perf,out).print("running scan_stockpiles\n");
    int32_t newly_marked = 0;

    item_count_piles.clear();
    premarked_item_count_piles.clear();
    marked_item_count_piles.clear();

    //Parse all the watched piles
    for (auto &c : watched_stockpiles) {
        int id = get_config_val(c.second, STOCKPILE_CONFIG_ID);
        //Check monitor status
        bool monitored = get_config_bool(c.second, STOCKPILE_CONFIG_MONITORED);

        if (!monitored) continue;

        DEBUG(perf,out).print("%s past monitor check\nmonitored=%d\n", plugin_name, monitored);

        int32_t item_count = 0;

        int32_t premarked_count = 0;

        int32_t marked = mark_all_in_stockpile(out, c.second, premarked_count, item_count, tracked_item_map, should_melt);

        DEBUG(perf,out).print("post mark_all_in_stockpile premarked_count=%d\n", premarked_count);

        item_count_piles.emplace(id, item_count);

        premarked_item_count_piles.emplace(id, premarked_count);

        marked_item_count_piles.emplace(id, marked);

        newly_marked += marked;

    }
    DEBUG(perf,out).print("exit scan_stockpiles\n");
    return newly_marked;
}

static int32_t scan_all_melt_designated(color_ostream &out, map<int32_t, bool> &tracked_item_map) {

    DEBUG(perf,out).print("running scan_all_melt_designated\n");
    int32_t marked_item_count = 0;
    //loop over all items marked as melt-designated
    for (auto item : world->items.other[items_other_id::ANY_MELT_DESIGNATED]) {
        //item has already been marked/counted as inside a stockpile. Don't double-count.
        if (tracked_item_map.count(item->id)) {
            continue;
        }
        marked_item_count++;
    }
    DEBUG(perf,out).print("exiting scan_all_melt_designated\n");
    return marked_item_count;
}

static int32_t scan_count_all(color_ostream &out, bool should_melt, int32_t &marked_item_count_total, int32_t &marked_total_count_all_piles, int32_t &marked_item_count_global,
                              int32_t &total_items_all_piles, map<int32_t, int32_t> &item_count_piles, map<int32_t, int32_t> &premarked_item_count_piles, map<int32_t, int32_t> &marked_item_count_piles) {

    //Wraps both scan_stockpiles and scan_all_melt_designated
    //Returns count of items in piles freshly marked

    int32_t newly_marked_items_piles = 0;

    map<int32_t, bool> tracked_item_map_piles;

    newly_marked_items_piles = scan_stockpiles(out, should_melt, item_count_piles, premarked_item_count_piles, marked_item_count_piles, tracked_item_map_piles);
    marked_item_count_global = scan_all_melt_designated(out, tracked_item_map_piles);

    for (auto &i : watched_stockpiles) {
        int id = get_config_val(i.second, STOCKPILE_CONFIG_ID);
        total_items_all_piles+= item_count_piles[id];
        marked_total_count_all_piles += premarked_item_count_piles[id];
    }

    marked_item_count_total = marked_item_count_global + marked_total_count_all_piles;


    return newly_marked_items_piles;

}

static int32_t do_cycle(color_ostream &out) {
    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);
    cycle_timestamp = world->frame_counter;

    validate_stockpile_configs(out);

    int32_t marked_item_count_total = 0;
    int32_t marked_item_count_global = 0;
    int32_t newly_marked_items_piles = 0;
    int32_t total_items_all_piles = 0;
    int32_t marked_total_count_all_piles = 0;
    map<int32_t, int32_t> item_count_piles, premarked_item_count_piles, marked_item_count_piles;

    newly_marked_items_piles = scan_count_all(out, true, marked_item_count_total, marked_total_count_all_piles, marked_item_count_global,
                                              total_items_all_piles, item_count_piles, premarked_item_count_piles, marked_item_count_piles);

    DEBUG(perf,out).print("exit %s do_cycle\n", plugin_name);
    return newly_marked_items_piles;
}

static int getSelectedStockpile(color_ostream &out) {
    df::building *bld = Gui::getSelectedBuilding(out, true);
    if (!isStockpile(bld)) {
        DEBUG(status,out).print("Selected building is not stockpile\n");
        return -1;
    }

    return bld->id;
}

static PersistentDataItem *getSelectedStockpileConfig(color_ostream &out) {
    int32_t bldg_id = getSelectedStockpile(out);
    if (bldg_id == -1) {
        return NULL;
    }

    validate_stockpile_configs(out);
    PersistentDataItem *c = NULL;
    if (watched_stockpiles.count(bldg_id)) {
        c = &(watched_stockpiles[bldg_id]);
        return c;
    }

    DEBUG(status,out).print("No existing config\n");
    return NULL;
}

static void push_stockpile_config(lua_State *L, int id, bool monitored) {
    map<string, int32_t> stockpile_config;
    stockpile_config.emplace("id", id);
    stockpile_config.emplace("monitored", monitored);
    Lua::Push(L, stockpile_config);
}

static void push_stockpile_config(lua_State *L, PersistentDataItem &c) {
    push_stockpile_config(L, get_config_val(c, STOCKPILE_CONFIG_ID),
            get_config_bool(c, STOCKPILE_CONFIG_MONITORED));
}

static void emplace_bulk_stockpile_config(lua_State *L, int id, bool monitored, map<int32_t, map<string, int32_t>> &stockpiles) {
    map<string, int32_t> stockpile_config;
    stockpile_config.emplace("id", id);
    stockpile_config.emplace("monitored", monitored);

    stockpiles.emplace(id, stockpile_config);
}

static void emplace_bulk_stockpile_config(lua_State *L, PersistentDataItem &c, map<int32_t, map<string, int32_t>> &stockpiles) {
    int32_t id = get_config_val(c, STOCKPILE_CONFIG_ID);
    bool monitored = get_config_bool(c, STOCKPILE_CONFIG_MONITORED);
    emplace_bulk_stockpile_config(L, id, monitored, stockpiles);
}

static void automelt_designate(color_ostream &out) {
    DEBUG(status, out).print("entering automelt designate\n");
    out.print("designated %d item(s) for melting\n", do_cycle(out));
}

static void automelt_printStatus(color_ostream &out) {
    DEBUG(status,out).print("entering automelt_printStatus\n");
    validate_stockpile_configs(out);
    out.print("automelt is %s\n\n", is_enabled ? "enabled" : "disabled");

    int32_t marked_item_count_total = 0;
    int32_t marked_item_count_global = 0;
    int32_t total_items_all_piles = 0;
    int32_t marked_total_count_all_piles = 0;
    map<int32_t, int32_t> item_count_piles, premarked_item_count_piles, marked_item_count_piles;

    scan_count_all(out, false, marked_item_count_total, marked_total_count_all_piles, marked_item_count_global,
                                              total_items_all_piles, item_count_piles, premarked_item_count_piles, marked_item_count_piles);

    out.print("summary:\n");
    out.print("                         total items in monitored piles: %d\n", total_items_all_piles);
    out.print("                        marked items in monitored piles: %d\n", marked_total_count_all_piles);
    out.print("marked items global (excludes those in monitored piles): %d\n", marked_item_count_global);
    out.print("         marked items global (monitored piles + others): %d\n", marked_item_count_total);

    int name_width = 11;
    for (auto &stockpile : world->buildings.other.STOCKPILE) {
        if (!isStockpile(stockpile)) continue;
        if (stockpile->name.empty()) {
            string stock_name = "Stockpile #" + int_to_string(stockpile->stockpile_number);
            name_width = std::max(name_width, (int)(stock_name.size()));
        } else {
            name_width = std::max(name_width, (int)stockpile->name.size());
        }
    }
    name_width = -name_width;

    const char *fmt = "%*s  %9s  %5s  %6s\n";
    out.print(fmt, name_width, "name", "monitored", "items", "marked");
    out.print(fmt, name_width, "----", "---------", "-----", "------");

    for (auto &stockpile : world->buildings.other.STOCKPILE) {
        if (!isStockpile(stockpile)) continue;
        bool monitored = false;
        int32_t item_count = 0;
        int32_t marked_item_count = 0;
        if (watched_stockpiles.count(stockpile->id)) {
            auto &c = watched_stockpiles[stockpile->id];
            monitored = get_config_bool(c, STOCKPILE_CONFIG_MONITORED);
            int id = get_config_val(c, STOCKPILE_CONFIG_ID);
            item_count = item_count_piles[id];
            marked_item_count = premarked_item_count_piles[id];
        }

        if (stockpile->name.empty()) {
            string stock_name = "Stockpile #" + int_to_string(stockpile->stockpile_number);
            out.print(fmt, name_width, stock_name.c_str(), monitored ? "[x]": "[ ]",
                        int_to_string(item_count).c_str(), int_to_string(marked_item_count).c_str());
        } else {
            out.print(fmt, name_width, stockpile->name.c_str(), monitored ? "[x]": "[ ]",
                        int_to_string(item_count).c_str(), int_to_string(marked_item_count).c_str());
        }


    }
    DEBUG(status,out).print("exiting automelt_printStatus\n");

}

static void automelt_setStockpileConfig(color_ostream &out, int id, bool monitored) {
    DEBUG(status,out).print("entering automelt_setStockpileConfig for id=%d and monitored=%d\n", id, monitored);
    validate_stockpile_configs(out);
    auto bldg = df::building::find(id);
    bool isInvalidStockpile = !isStockpile(bldg);
    bool hasNoData = !monitored;
    if (isInvalidStockpile || hasNoData) {
        DEBUG(cycle,out).print("calling remove_stockpile_config with id=%d monitored=%d\n", id, monitored);
        remove_stockpile_config(out, id);
        return;
    }

    PersistentDataItem &c = ensure_stockpile_config(out, id);
    set_config_val(c, STOCKPILE_CONFIG_ID, id);
    set_config_bool(c, STOCKPILE_CONFIG_MONITORED, monitored);

    //If we're marking something as monitored, go ahead and designate contents.
    if (monitored) {
        automelt_designate(out);
    }
}

static int automelt_getStockpileConfig(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status, *out).print("entering automelt_getStockpileConfig\n");
    validate_stockpile_configs(*out);

    int id;
    if (lua_isnumber(L, -1)) {
        bool found = false;
        id = lua_tointeger(L, -1);

        for (auto &stockpile : world->buildings.other.STOCKPILE) {
            if (!isStockpile(stockpile)) continue;
            if (id == stockpile->stockpile_number){
                id = stockpile->id;
                found = true;
                break;
            }
        }

        if (!found)
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
            } else {
                string stock_name = "Stockpile #" + int_to_string(stockpile->stockpile_number);
                if (stock_name == nameStr) {
                    id = stockpile->id;
                    found = true;
                    break;
                }
            }

        }
        if (!found)
            return 0;
    }

    if (watched_stockpiles.count(id)) {
        push_stockpile_config(L, watched_stockpiles[id]);
    } else {
        push_stockpile_config(L, id, false);
    }
    return 1;
}

static int automelt_getSelectedStockpileConfig(lua_State *L){
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status, *out).print("entering automelt_getSelectedStockpileConfig\n");
    validate_stockpile_configs(*out);

    int32_t stock_id = getSelectedStockpile(*out);
    PersistentDataItem *c = getSelectedStockpileConfig(*out);

    //If we have a valid config entry, use that. Else assume all false.
    if (c) {
        push_stockpile_config(L, *c);
    } else {
        push_stockpile_config(L, stock_id, false);
    }

    return 1;
}

static int automelt_getItemCountsAndStockpileConfigs(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering automelt_getItemCountsAndStockpileConfigs\n");
    validate_stockpile_configs(*out);

    int32_t marked_item_count_total = 0;
    int32_t marked_item_count_global = 0;
    int32_t total_items_all_piles = 0;
    int32_t marked_total_count_all_piles = 0;
    map<int32_t, int32_t> item_count_piles, premarked_item_count_piles, marked_item_count_piles;

    scan_count_all(*out, false, marked_item_count_total, marked_total_count_all_piles, marked_item_count_global,
                                              total_items_all_piles, item_count_piles, premarked_item_count_piles, marked_item_count_piles);

    map<string, int32_t> summary;
    summary.emplace("total_items", total_items_all_piles);
    summary.emplace("premarked_items", marked_total_count_all_piles);
    summary.emplace("marked_item_count_global", marked_item_count_global);
    summary.emplace("marked_item_count_total", marked_item_count_total);

    Lua::Push(L, summary);
    Lua::Push(L, item_count_piles);
    Lua::Push(L, marked_item_count_piles);
    Lua::Push(L, premarked_item_count_piles);

    map<int32_t, map<string, int32_t>> stockpile_config_map;

    for (auto pile : world->buildings.other.STOCKPILE) {
        if (!isStockpile(pile))
            continue;

        int id = pile->id;
        if (watched_stockpiles.count(id)) {
            emplace_bulk_stockpile_config(L, watched_stockpiles[id], stockpile_config_map);

        } else {
            emplace_bulk_stockpile_config(L, id, false, stockpile_config_map);
        }
    }

    Lua::Push(L, stockpile_config_map);


    DEBUG(perf, *out).print("exit automelt_getItemCountsAndStockpileConfigs\n");

    return 5;
}

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(automelt_printStatus),
    DFHACK_LUA_FUNCTION(automelt_designate),
    DFHACK_LUA_FUNCTION(automelt_setStockpileConfig),
    DFHACK_LUA_END};

DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(automelt_getStockpileConfig),
    DFHACK_LUA_COMMAND(automelt_getItemCountsAndStockpileConfigs),
    DFHACK_LUA_COMMAND(automelt_getSelectedStockpileConfig),
    DFHACK_LUA_END};
