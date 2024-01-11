#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Buildings.h"
#include "modules/Job.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/building.h"
#include "df/building_stockpilest.h"
#include "df/building_tradedepotst.h"
#include "df/caravan_state.h"
#include "df/general_ref_building_holderst.h"
#include "df/plotinfost.h"
#include "df/training_assignment.h"
#include "df/world.h"

using std::string;
using std::unordered_map;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("logistics");

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);

namespace DFHack {
DBG_DECLARE(logistics, control, DebugCategory::LINFO);
DBG_DECLARE(logistics, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY_PREFIX = string(plugin_name) + "/";
static unordered_map<int32_t, PersistentDataItem> watched_stockpiles;

enum StockpileConfigValues {
    STOCKPILE_CONFIG_STOCKPILE_NUMBER = 0,
    STOCKPILE_CONFIG_MELT = 1,
    STOCKPILE_CONFIG_TRADE = 2,
    STOCKPILE_CONFIG_DUMP = 3,
    STOCKPILE_CONFIG_TRAIN = 4,
    STOCKPILE_CONFIG_MELT_MASTERWORKS = 5,
};

static PersistentDataItem& ensure_stockpile_config(color_ostream& out, int stockpile_number) {
    TRACE(control, out).print("ensuring stockpile config stockpile_number=%d\n", stockpile_number);
    if (watched_stockpiles.count(stockpile_number)) {
        TRACE(control, out).print("stockpile exists in watched_stockpiles\n");
        return watched_stockpiles[stockpile_number];
    }

    string keyname = CONFIG_KEY_PREFIX + int_to_string(stockpile_number);
    DEBUG(control, out).print("creating new persistent key for stockpile %d\n", stockpile_number);
    watched_stockpiles.emplace(stockpile_number, World::GetPersistentSiteData(keyname, true));
    PersistentDataItem& c = watched_stockpiles[stockpile_number];
    c.set_int(STOCKPILE_CONFIG_STOCKPILE_NUMBER, stockpile_number);
    c.set_bool(STOCKPILE_CONFIG_MELT, false);
    c.set_bool(STOCKPILE_CONFIG_TRADE, false);
    c.set_bool(STOCKPILE_CONFIG_DUMP, false);
    c.set_bool(STOCKPILE_CONFIG_TRAIN, false);
    c.set_bool(STOCKPILE_CONFIG_MELT_MASTERWORKS, false);
    return c;
}

static void remove_stockpile_config(color_ostream& out, int stockpile_number) {
    if (!watched_stockpiles.count(stockpile_number))
        return;
    DEBUG(control, out).print("removing persistent key for stockpile %d\n", stockpile_number);
    World::DeletePersistentData(watched_stockpiles[stockpile_number]);
    watched_stockpiles.erase(stockpile_number);
}

static const int32_t CYCLE_TICKS = 601;
static int32_t cycle_timestamp = 0; // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream& out, int32_t& melt_count, int32_t& trade_count, int32_t& dump_count, int32_t& train_count);

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    DEBUG(control, out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        plugin_name,
        "Automatically mark and route items in monitored stockpiles.",
        do_command));

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    is_enabled = enable;
    DEBUG(control, out).print("now %s\n", is_enabled ? "enabled" : "disabled");
    return CR_OK;
}

static df::building_stockpilest* find_stockpile(int32_t stockpile_number) {
    return binsearch_in_vector(world->buildings.other.STOCKPILE,
            &df::building_stockpilest::stockpile_number, stockpile_number);
}

static void validate_stockpile_configs(color_ostream& out,
        unordered_map<df::building_stockpilest *, PersistentDataItem> &cache) {
    vector<int> to_remove;
    for (auto& entry : watched_stockpiles) {
        int stockpile_number = entry.first;
        PersistentDataItem &c = entry.second;
        auto bld = find_stockpile(stockpile_number);
        if (!bld || (
                !c.get_bool(STOCKPILE_CONFIG_MELT) &&
                !c.get_bool(STOCKPILE_CONFIG_TRADE) &&
                !c.get_bool(STOCKPILE_CONFIG_DUMP) &&
                !c.get_bool(STOCKPILE_CONFIG_TRAIN))) {
            to_remove.push_back(stockpile_number);
            continue;
        }
        cache.emplace(bld, c);
    }
    for (int stockpile_number : to_remove)
        remove_stockpile_config(out, stockpile_number);
}

// remove this function once saves from 50.08 are no longer compatible
static void migrate_old_keys(color_ostream &out) {
    vector<PersistentDataItem> old_data;
    World::GetPersistentSiteData(&old_data, "automelt/stockpile/", true);
    const size_t num_old_keys = old_data.size();
    for (size_t idx = 0; idx < num_old_keys; ++idx) {
        auto& old_c = old_data[idx];
        int32_t bld_id = old_c.get_int(0);
        bool melt_was_on = old_c.get_bool(1);
        World::DeletePersistentData(old_c);
        auto bld = df::building::find(bld_id);
        if (!bld || bld->getType() != df::building_type::Stockpile ||
                watched_stockpiles.count(static_cast<df::building_stockpilest *>(bld)->stockpile_number))
            continue;
        auto &c = ensure_stockpile_config(out, static_cast<df::building_stockpilest *>(bld)->stockpile_number);
        c.set_bool(STOCKPILE_CONFIG_MELT, melt_was_on);
    }
}

DFhackCExport command_result plugin_load_site_data(color_ostream &out) {
    cycle_timestamp = 0;

    vector<PersistentDataItem> loaded_persist_data;
    World::GetPersistentSiteData(&loaded_persist_data, CONFIG_KEY_PREFIX, true);
    watched_stockpiles.clear();
    const size_t num_watched_stockpiles = loaded_persist_data.size();
    for (size_t idx = 0; idx < num_watched_stockpiles; ++idx) {
        auto& c = loaded_persist_data[idx];
        watched_stockpiles.emplace(c.get_int(STOCKPILE_CONFIG_STOCKPILE_NUMBER), c);
    }
    migrate_old_keys(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (!is_enabled || !Core::getInstance().isMapLoaded())
        return CR_OK;
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS) {
        int32_t melt_count = 0, trade_count = 0, dump_count = 0, train_count = 0;
        do_cycle(out, melt_count, trade_count, dump_count, train_count);
        if (0 < melt_count)
            out.print("logistics: marked %d item(s) for melting\n", melt_count);
        if (0 < trade_count)
            out.print("logistics: marked %d item(s) for trading\n", trade_count);
        if (0 < dump_count)
            out.print("logistics: marked %d item(s) for dumping\n", dump_count);
        if (0 < train_count)
            out.print("logistics: marked %d animal(s) for training\n", dump_count);
    }
    return CR_OK;
}

static bool call_logistics_lua(color_ostream* out, const char* fn_name,
        int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA) {
    DEBUG(control).print("calling logistics lua function: '%s'\n", fn_name);

    CoreSuspender guard;

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!out)
        out = &Core::getInstance().getConsole();

    return Lua::CallLuaModuleFunction(*out, L, "plugins.logistics", fn_name,
            nargs, nres,
            std::forward<Lua::LuaLambda&&>(args_lambda),
            std::forward<Lua::LuaLambda&&>(res_lambda));
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot run %s without a loaded map.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!call_logistics_lua(&out, "parse_commandline", 1, 1,
            [&](lua_State *L) {
                Lua::PushVector(L, parameters);
            },
            [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

/////////////////////////////////////////////////////
// cycle
//

typedef unordered_map<int32_t, size_t> StatMap;

struct ProcessorStats {
    size_t newly_designated = 0;
    StatMap designated_counts, can_designate_counts;
};

class StockProcessor {
public:
    const string name;
    const int32_t stockpile_number;
    const bool enabled;
    ProcessorStats &stats;
protected:
    StockProcessor(const string &name, int32_t stockpile_number, bool enabled, ProcessorStats &stats)
            : name(name), stockpile_number(stockpile_number), enabled(enabled), stats(stats) { }
public:
    virtual bool is_designated(color_ostream &out, df::item *item) = 0;
    virtual bool can_designate(color_ostream &out, df::item *item) = 0;
    virtual bool designate(color_ostream &out, df::item *item) = 0;
};

class MeltStockProcessor : public StockProcessor {
public:
    MeltStockProcessor(int32_t stockpile_number, bool enabled, ProcessorStats &stats, bool melt_masterworks)
            : StockProcessor("melt", stockpile_number, enabled, stats), melt_masterworks(melt_masterworks) { }

    bool is_designated(color_ostream &out, df::item *item) override {
        return item->flags.bits.melt;
    }

    bool can_designate(color_ostream &out, df::item *item) override {
        if (!melt_masterworks && item->getQuality() >= df::item_quality::Masterful)
            return false;
        return Items::canMelt(item);
    }

    bool designate(color_ostream &out, df::item *item) override {
        Items::markForMelting(item);
        return true;
    }

    private:
    const bool melt_masterworks;
};

class TradeStockProcessor: public StockProcessor {
public:
    TradeStockProcessor(int32_t stockpile_number, bool enabled, ProcessorStats& stats)
            : StockProcessor("trade", stockpile_number, enabled && get_active_trade_depot(), stats),
              depot(get_active_trade_depot()) { }

    bool is_designated(color_ostream& out, df::item* item) override {
        auto ref = Items::getSpecificRef(item, df::specific_ref_type::JOB);
        return ref && ref->data.job &&
                ref->data.job->job_type == df::job_type::BringItemToDepot;
    }

    bool can_designate(color_ostream& out, df::item* item) override {
        return Items::canTradeAnyWithContents(item);
    }

    bool designate(color_ostream& out, df::item* item) override {
        if (!depot)
            return false;
        return Items::markForTrade(item, depot);
    }

private:
    df::building_tradedepotst * const depot;

    static df::building_tradedepotst * get_active_trade_depot() {
        // at least one non-tribute caravan must be approaching or ready to trade
        if (!plotinfo->caravans.size())
            return NULL;
        bool found = false;
        for (auto caravan : plotinfo->caravans) {
            if (caravan->flags.bits.tribute)
                continue;
            auto trade_state = caravan->trade_state;
            auto time_remaining = caravan->time_remaining;
            if ((trade_state == df::caravan_state::T_trade_state::Approaching ||
                    trade_state == df::caravan_state::T_trade_state::AtDepot) && time_remaining != 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return NULL;

        // at least one trade depot must be ready to receive goods
        for (auto bld : world->buildings.other.TRADE_DEPOT) {
            if (bld->getBuildStage() < bld->getMaxBuildStage())
                continue;

            if (bld->jobs.size() == 1 &&
                    bld->jobs[0]->job_type == df::job_type::DestroyBuilding)
                continue;

            return bld;
        }
        return NULL;
    }
};

class DumpStockProcessor: public StockProcessor {
public:
    DumpStockProcessor(int32_t stockpile_number, bool enabled, ProcessorStats& stats)
        : StockProcessor("dump", stockpile_number, enabled, stats) { }

    bool is_designated(color_ostream& out, df::item* item) override {
        return item->flags.bits.dump;
    }

    bool can_designate(color_ostream& out, df::item* item) override {
        return true;
    }

    bool designate(color_ostream& out, df::item* item) override {
        item->flags.bits.dump = true;
        return true;
     }
};

class TrainStockProcessor : public StockProcessor {
public:
    TrainStockProcessor(int32_t stockpile_number, bool enabled, ProcessorStats& stats)
        : StockProcessor("train", stockpile_number, enabled, stats) {}

    bool is_designated(color_ostream& out, df::item* item) override {
        auto unit = get_caged_unit(item);
        return unit && Units::isMarkedForTraining(unit);
    }

    bool can_designate(color_ostream& out, df::item* item) override {
        auto unit = get_caged_unit(item);
        return unit && !Units::isInvader(unit) &&
            Units::isTamable(unit) && !Units::isTame(unit) &&
            !Units::isMarkedForTraining(unit);
    }

    bool designate(color_ostream& out, df::item* item) override {
        auto unit = get_caged_unit(item);
        if (!unit)
            return false;
        df::training_assignment *assignment = new df::training_assignment();
        assignment->animal_id = unit->id;
        assignment->trainer_id = -1;
        assignment->flags.bits.any_trainer = true;
        insert_into_vector(df::global::plotinfo->equipment.training_assignments,
            &df::training_assignment::animal_id, assignment);
        return true;
    }

private:
    static df::unit* get_caged_unit(df::item* item) {
        if (item->getType() != df::item_type::CAGE)
            return NULL;
        auto gref = Items::getGeneralRef(item, df::general_ref_type::CONTAINS_UNIT);
        if (!gref)
            return NULL;
        return gref->getUnit();
    }
};

static const struct BadFlags {
    const uint32_t whole;

    BadFlags() : whole(get_bad_flags()) { }

private:
    uint32_t get_bad_flags() {
        df::item_flags flags;
        #define F(x) flags.bits.x = true;
        F(forbid); F(garbage_collect); F(hostile); F(on_fire);
        F(rotten); F(trader); F(in_building); F(construction);
        F(artifact); F(spider_web); F(owned); F(in_job);
        #undef F
        return flags.whole;
    }
} bad_flags;

static void scan_item(color_ostream &out, df::item *item, StockProcessor &processor) {
    DEBUG(cycle,out).print("scan_item [%s] item_id=%d\n", processor.name.c_str(), item->id);

    if (DBG_NAME(cycle).isEnabled(DebugCategory::LTRACE)) {
        string name = "";
        item->getItemDescription(&name, 0);
        TRACE(cycle,out).print("item: %s\n", name.c_str());
    }

    if (item->flags.whole & bad_flags.whole) {
        TRACE(cycle,out).print("rejected flag check\n");
        return;
    }

    if (processor.is_designated(out, item)) {
        TRACE(cycle,out).print("already designated\n");
        ++processor.stats.designated_counts[processor.stockpile_number];
        return;
    }

    if (!processor.can_designate(out, item)) {
        TRACE(cycle,out).print("cannot designate\n");
        return;
    }

    if (!processor.enabled) {
        ++processor.stats.can_designate_counts[processor.stockpile_number];
        return;
    }

    processor.designate(out, item);
    ++processor.stats.newly_designated;
    ++processor.stats.designated_counts[processor.stockpile_number];
}

static void scan_stockpile(color_ostream &out, df::building_stockpilest *bld,
        MeltStockProcessor &melt_stock_processor,
        TradeStockProcessor &trade_stock_processor,
        DumpStockProcessor &dump_stock_processor,
        TrainStockProcessor &train_stock_processor) {
    auto id = bld->id;
    Buildings::StockpileIterator items;
    for (items.begin(bld); !items.done(); ++items) {
        df::item *item = *items;
        scan_item(out, item, trade_stock_processor);
        if (0 == (item->flags.whole & bad_flags.whole) &&
                item->isAssignedToThisStockpile(id)) {
            TRACE(cycle,out).print("assignedToStockpile\n");
            vector<df::item *> contents;
            Items::getContainedItems(item, &contents);
            for (df::item *contained_item : contents) {
                scan_item(out, contained_item, melt_stock_processor);
                scan_item(out, contained_item, dump_stock_processor);
            }
            continue;
        }
        scan_item(out, item, melt_stock_processor);
        scan_item(out, item, dump_stock_processor);
        scan_item(out, item, train_stock_processor);
    }
}

static void do_cycle(color_ostream& out, int32_t& melt_count, int32_t& trade_count, int32_t& dump_count, int32_t& train_count) {
    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);
    cycle_timestamp = world->frame_counter;

    ProcessorStats melt_stats, trade_stats, dump_stats, train_stats;
    unordered_map<df::building_stockpilest *, PersistentDataItem> cache;
    validate_stockpile_configs(out, cache);

    for (auto &entry : cache) {
        df::building_stockpilest *bld = entry.first;
        PersistentDataItem &c = entry.second;
        int32_t stockpile_number = bld->stockpile_number;

        bool melt = c.get_bool(STOCKPILE_CONFIG_MELT);
        bool melt_masterworks = c.get_bool(STOCKPILE_CONFIG_MELT_MASTERWORKS);
        bool trade = c.get_bool(STOCKPILE_CONFIG_TRADE);
        bool dump = c.get_bool(STOCKPILE_CONFIG_DUMP);
        bool train = c.get_bool(STOCKPILE_CONFIG_TRAIN);

        MeltStockProcessor melt_stock_processor(stockpile_number, melt, melt_stats, melt_masterworks);
        TradeStockProcessor trade_stock_processor(stockpile_number, trade, trade_stats);
        DumpStockProcessor dump_stock_processor(stockpile_number, dump, dump_stats);
        TrainStockProcessor train_stock_processor(stockpile_number, train, train_stats);

        scan_stockpile(out, bld, melt_stock_processor,
                trade_stock_processor, dump_stock_processor, train_stock_processor);
    }

    melt_count = melt_stats.newly_designated;
    trade_count = trade_stats.newly_designated;
    dump_count = dump_stats.newly_designated;
    TRACE(cycle,out).print("exit %s do_cycle\n", plugin_name);
}

/////////////////////////////////////////////////////
// Lua API
//

static int logistics_getStockpileData(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("entering logistics_getStockpileData\n");

    unordered_map<df::building_stockpilest *, PersistentDataItem> cache;
    validate_stockpile_configs(*out, cache);

    ProcessorStats melt_stats, trade_stats, dump_stats, train_stats;

    for (auto bld : df::global::world->buildings.other.STOCKPILE) {
        int32_t stockpile_number = bld->stockpile_number;
        MeltStockProcessor melt_stock_processor(stockpile_number, false, melt_stats, false);
        TradeStockProcessor trade_stock_processor(stockpile_number, false, trade_stats);
        DumpStockProcessor dump_stock_processor(stockpile_number, false, dump_stats);
        TrainStockProcessor train_stock_processor(stockpile_number, false, train_stats);

        scan_stockpile(*out, bld, melt_stock_processor,
                trade_stock_processor, dump_stock_processor, train_stock_processor);
    }

    unordered_map<string, StatMap> stats;
    stats.emplace("melt_designated", melt_stats.designated_counts);
    stats.emplace("melt_can_designate", melt_stats.can_designate_counts);
    stats.emplace("trade_designated", trade_stats.designated_counts);
    stats.emplace("trade_can_designate", trade_stats.can_designate_counts);
    stats.emplace("dump_designated", dump_stats.designated_counts);
    stats.emplace("dump_can_designate", dump_stats.can_designate_counts);
    stats.emplace("train_designated", train_stats.designated_counts);
    stats.emplace("train_can_designate", train_stats.can_designate_counts);
    Lua::Push(L, stats);

    unordered_map<int32_t, unordered_map<string, string>> configs;
    for (auto &entry : cache) {
        df::building_stockpilest *bld = entry.first;
        PersistentDataItem &c = entry.second;

        bool melt = c.get_bool(STOCKPILE_CONFIG_MELT);
        bool melt_masterworks = c.get_bool(STOCKPILE_CONFIG_MELT_MASTERWORKS);
        bool trade = c.get_bool(STOCKPILE_CONFIG_TRADE);
        bool dump = c.get_bool(STOCKPILE_CONFIG_DUMP);
        bool train = c.get_bool(STOCKPILE_CONFIG_TRAIN);

        unordered_map<string, string> config;
        config.emplace("melt", melt ? "true" : "false");
        config.emplace("melt_masterworks", melt_masterworks ? "true" : "false");
        config.emplace("trade", trade ? "true" : "false");
        config.emplace("dump", dump ? "true" : "false");
        config.emplace("train", train ? "true" : "false");

        configs.emplace(bld->stockpile_number, config);
    }
    Lua::Push(L, configs);

    TRACE(cycle, *out).print("exit logistics_getStockpileData\n");

    return 2;
}

static void logistics_cycle(color_ostream &out) {
    DEBUG(control, out).print("entering logistics_cycle\n");
    int32_t melt_count = 0, trade_count = 0, dump_count = 0, train_count = 0;
    do_cycle(out, melt_count, trade_count, dump_count, train_count);
    out.print("logistics: marked %d item(s) for melting\n", melt_count);
    out.print("logistics: marked %d item(s) for trading\n", trade_count);
    out.print("logistics: marked %d item(s) for dumping\n", dump_count);
    out.print("logistics: marked %d animal(s) for train\n", train_count);
}

static void find_stockpiles(lua_State *L, int idx,
        vector<df::building_stockpilest*> &sps) {
    if (lua_isnumber(L, idx)) {
        sps.emplace_back(find_stockpile(lua_tointeger(L, -1)));
        return;
    }

    const char * pname = lua_tostring(L, -1);
    if (!pname || !*pname)
        return;
    string name(pname);
    for (auto sp : world->buildings.other.STOCKPILE) {
        if (name == sp->name)
            sps.emplace_back(sp);
    }
}

static unordered_map<string, int> get_stockpile_config(int32_t stockpile_number) {
    unordered_map<string, int> stockpile_config;
    stockpile_config.emplace("stockpile_number", stockpile_number);
    if (watched_stockpiles.count(stockpile_number)) {
        PersistentDataItem &c = watched_stockpiles[stockpile_number];
        stockpile_config.emplace("melt", c.get_bool(STOCKPILE_CONFIG_MELT));
        stockpile_config.emplace("melt_masterworks", c.get_bool(STOCKPILE_CONFIG_MELT_MASTERWORKS));
        stockpile_config.emplace("trade", c.get_bool(STOCKPILE_CONFIG_TRADE));
        stockpile_config.emplace("dump", c.get_bool(STOCKPILE_CONFIG_DUMP));
        stockpile_config.emplace("train", c.get_bool(STOCKPILE_CONFIG_TRAIN));
    } else {
        stockpile_config.emplace("melt", false);
        stockpile_config.emplace("melt_masterworks", false);
        stockpile_config.emplace("trade", false);
        stockpile_config.emplace("dump", false);
        stockpile_config.emplace("train", false);
    }
    return stockpile_config;
}

static int logistics_getStockpileConfigs(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control, *out).print("entering logistics_getStockpileConfigs\n");

    unordered_map<df::building_stockpilest*, PersistentDataItem> cache;
    validate_stockpile_configs(*out, cache);

    vector<df::building_stockpilest*> sps;
    find_stockpiles(L, -1, sps);
    if (sps.empty())
        return 0;

    vector<unordered_map<string, int>> configs;
    for (auto sp : sps)
        configs.emplace_back(get_stockpile_config(sp->stockpile_number));
    Lua::PushVector(L, configs);
    return 1;
}

static void logistics_setStockpileConfig(color_ostream& out, int stockpile_number, bool melt, bool trade, bool dump, bool train, bool melt_masterworks) {
    DEBUG(control, out).print("entering logistics_setStockpileConfig stockpile_number=%d, melt=%d, trade=%d, dump=%d, train=%d, melt_masterworks=%d\n",
        stockpile_number, melt, trade, dump, train, melt_masterworks);

    if (!find_stockpile(stockpile_number)) {
        out.printerr("invalid stockpile number: %d\n", stockpile_number);
        return;
    }

    auto &c = ensure_stockpile_config(out, stockpile_number);
    c.set_bool(STOCKPILE_CONFIG_MELT, melt);
    c.set_bool(STOCKPILE_CONFIG_MELT_MASTERWORKS, melt_masterworks);
    c.set_bool(STOCKPILE_CONFIG_TRADE, trade);
    c.set_bool(STOCKPILE_CONFIG_DUMP, dump);
    c.set_bool(STOCKPILE_CONFIG_TRAIN, train);
}

static int logistics_clearStockpileConfig(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control, *out).print("entering logistics_clearStockpileConfig\n");

    vector<df::building_stockpilest*> sps;
    find_stockpiles(L, -1, sps);
    if (sps.empty())
        return 0;

    for (auto sp : sps)
        remove_stockpile_config(*out, sp->stockpile_number);
    return 0;
}

static void logistics_clearAllStockpileConfigs(color_ostream &out) {
    DEBUG(control, out).print("entering logistics_clearAllStockpileConfigs\n");
    for (auto &entry : watched_stockpiles)
        World::DeletePersistentData(entry.second);
    watched_stockpiles.clear();
}

static int logistics_getGlobalCounts(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("entering logistics_getGlobalCounts\n");

    size_t num_melt = df::global::world->items.other.ANY_MELT_DESIGNATED.size();

    size_t num_trade = 0;
    for (auto link = world->jobs.list.next; link; link = link->next) {
        df::job *job = link->item;
        if (job->job_type == df::job_type::BringItemToDepot)
            ++num_trade;
    }

    size_t num_dump = 0;
    for (auto item : world->items.other.IN_PLAY) {
        if (item->flags.bits.dump)
            ++num_dump;
    }

    size_t num_train = 0;
    // TODO

    unordered_map<string, size_t> results;
    results.emplace("total_melt", num_melt);
    results.emplace("total_trade", num_trade);
    results.emplace("total_dump", num_dump);
    results.emplace("total_train", num_train);
    Lua::Push(L, results);

    TRACE(cycle, *out).print("exit logistics_getGlobalCounts\n");

    return 1;
}

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(logistics_cycle),
    DFHACK_LUA_FUNCTION(logistics_setStockpileConfig),
    DFHACK_LUA_FUNCTION(logistics_clearAllStockpileConfigs),
    DFHACK_LUA_END};

DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(logistics_getStockpileData),
    DFHACK_LUA_COMMAND(logistics_getStockpileConfigs),
    DFHACK_LUA_COMMAND(logistics_clearStockpileConfig),
    DFHACK_LUA_COMMAND(logistics_getGlobalCounts),
    DFHACK_LUA_END};
