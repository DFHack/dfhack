#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

#include "modules/Buildings.h"
#include "modules/Job.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/building.h"
#include "df/buildingitemst.h"
#include "df/building_stockpilest.h"
#include "df/building_tradedepotst.h"
#include "df/caravan_state.h"
#include "df/furniture_type.h"
#include "df/general_ref_building_holderst.h"
#include "df/item.h"
#include "df/plotinfost.h"
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
static const string CONFIG_KEY = CONFIG_KEY_PREFIX + "config";
static PersistentDataItem config;

// as a "system" plugin, we do not persist plugin enabled state
enum ConfigValues {
    CONFIG_TRAIN_PARTIAL = 0,
};

static unordered_map<int32_t, PersistentDataItem> watched_stockpiles;

enum StockpileConfigValues {
    STOCKPILE_CONFIG_STOCKPILE_NUMBER = 0,
    STOCKPILE_CONFIG_MELT = 1,
    STOCKPILE_CONFIG_TRADE = 2,
    STOCKPILE_CONFIG_DUMP = 3,
    STOCKPILE_CONFIG_TRAIN = 4,
    STOCKPILE_CONFIG_MELT_MASTERWORKS = 5,
    STOCKPILE_CONFIG_FORBID = 6,
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
    c.set_int(STOCKPILE_CONFIG_FORBID, 0);
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
static void do_cycle(color_ostream& out,
        int32_t& melt_count, int32_t& trade_count,
        int32_t& dump_count, int32_t& train_count,
        int32_t& forbid_count, int32_t& claim_count);
static void logistics_cycle(color_ostream &out, bool quiet);

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
                !c.get_bool(STOCKPILE_CONFIG_TRAIN) &&
                !(c.get_int(STOCKPILE_CONFIG_FORBID) > 0))) {
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
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_TRAIN_PARTIAL, false);
    }

    vector<PersistentDataItem> loaded_persist_data;
    World::GetPersistentSiteData(&loaded_persist_data, CONFIG_KEY_PREFIX, true);
    watched_stockpiles.clear();
    const size_t num_watched_stockpiles = loaded_persist_data.size();
    for (size_t idx = 0; idx < num_watched_stockpiles; ++idx) {
        auto& c = loaded_persist_data[idx];
        if (c.key() == CONFIG_KEY)
            continue;
        if (c.get_int(STOCKPILE_CONFIG_FORBID) == -1) // remove this once saves from 51.01 are no longer compatible
            c.set_int(STOCKPILE_CONFIG_FORBID, 0);
        watched_stockpiles.emplace(c.get_int(STOCKPILE_CONFIG_STOCKPILE_NUMBER), c);
    }
    migrate_old_keys(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode())
        return CR_OK;
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS) {
        logistics_cycle(out, true);
    }
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.logistics", "parse_commandline", std::make_tuple(parameters),
            1, [&](lua_State *L) {
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
        return !item->flags.bits.forbid && Items::canMelt(item);
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
        return !item->flags.bits.forbid && Items::canTradeAnyWithContents(item);
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

            if (bld->contained_items.empty() || !bld->contained_items[0] ||
                    !bld->contained_items[0]->item || bld->contained_items[0]->item->flags.bits.forbid)
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
        return !item->flags.bits.forbid;
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
        if (item->flags.bits.forbid) return false;
        auto unit = get_caged_unit(item);
        return unit &&
                Units::isTamable(unit) &&
                !Units::isDomesticated(unit) &&
                !Units::isMarkedForTraining(unit);
    }

    bool designate(color_ostream& out, df::item* item) override {
        auto unit = get_caged_unit(item);
        if (!unit)
            return false;
        return Units::assignTrainer(unit);
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

class ForbidStockProcessor : public StockProcessor {
public:
    ForbidStockProcessor(int32_t stockpile_number, bool enabled, ProcessorStats& stats)
        : StockProcessor("forbid", stockpile_number, enabled, stats) { }

    bool is_designated(color_ostream& out, df::item* item) override {
        return item->flags.bits.forbid;
    }

    bool can_designate(color_ostream& out, df::item* item) override {
        return true;
    }

    bool designate(color_ostream& out, df::item* item) override {
        item->flags.bits.forbid = true;
        return true;
    }
};

class ClaimStockProcessor : public StockProcessor {
public:
    ClaimStockProcessor(int32_t stockpile_number, bool enabled, ProcessorStats& stats)
        : StockProcessor("claim", stockpile_number, enabled, stats) { }

    bool is_designated(color_ostream& out, df::item* item) override {
        return !(item->flags.bits.forbid);
    }

    bool can_designate(color_ostream& out, df::item* item) override {
        return true;
    }

    bool designate(color_ostream& out, df::item* item) override {
        item->flags.bits.forbid = false;
        return true;
    }
};

static const struct BadFlags {
    const uint32_t whole;

    BadFlags() : whole(get_bad_flags()) { }

private:
    uint32_t get_bad_flags() {
        df::item_flags flags;
        #define F(x) flags.bits.x = true;
        F(garbage_collect); F(hostile); F(on_fire);
        F(trader); F(in_building); F(construction);
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

// quick check for type. this is intended to catch the case of newly empty storage item
// that happens to still be on the stockpile. we can do a more careful check (e.g. including
// item quality) if this isn't sufficient
static bool non_accepted_storage_item(df::building_stockpilest *bld, df::item *item) {
    auto & flags = bld->settings.flags;
    if (item->getType() == df::item_type::BIN)
        return !flags.bits.furniture || !bld->settings.furniture.type[df::furniture_type::BIN];
    if (item->hasToolUse(df::tool_uses::HEAVY_OBJECT_HAULING))
        return !flags.bits.furniture || !bld->settings.furniture.type[df::furniture_type::WHEELBARROW];
    if (item->isFoodStorage())
        return !flags.bits.furniture || !bld->settings.furniture.type[df::furniture_type::FOOD_STORAGE];
    return false;
}

static void scan_stockpile(color_ostream &out, df::building_stockpilest *bld,
        MeltStockProcessor &melt_stock_processor,
        TradeStockProcessor &trade_stock_processor,
        DumpStockProcessor &dump_stock_processor,
        TrainStockProcessor &train_stock_processor,
        ForbidStockProcessor &forbid_stock_processor,
        ClaimStockProcessor &claim_stock_processor) {
    auto id = bld->id;
    Buildings::StockpileIterator items;
    for (items.begin(bld); !items.done(); ++items) {
        df::item *item = *items;
        if (item->flags.whole & bad_flags.whole) {
            TRACE(cycle,out).print("rejected flag check\n");
            continue;
        }
        if (!item->isAssignedToThisStockpile(id) && non_accepted_storage_item(bld, item))
            continue;
        scan_item(out, item, trade_stock_processor);
        if (item->isAssignedToThisStockpile(id)) {
            TRACE(cycle,out).print("assignedToStockpile\n");
            vector<df::item *> contents;
            Items::getContainedItems(item, &contents);
            for (df::item *contained_item : contents) {
                scan_item(out, contained_item, forbid_stock_processor);
                scan_item(out, contained_item, claim_stock_processor);
                scan_item(out, contained_item, melt_stock_processor);
                scan_item(out, contained_item, dump_stock_processor);
            }
            continue;
        }
        scan_item(out, item, forbid_stock_processor);
        scan_item(out, item, claim_stock_processor);
        scan_item(out, item, melt_stock_processor);
        scan_item(out, item, dump_stock_processor);
        scan_item(out, item, train_stock_processor);
    }
}

#include <df/unit.h>
static void train_partials(color_ostream& out, int32_t& train_count) {
    for (auto unit : world->units.active) {
        if (!Units::isActive(unit) ||
            !Units::isTamable(unit) ||
            Units::isDomesticated(unit) ||
            Units::isWar(unit) ||
            Units::isHunter(unit) ||
            Units::isMarkedForTraining(unit) ||
            !Units::isFortControlled(unit))
            continue;

        if (Units::assignTrainer(unit)) {
            DEBUG(cycle,out).print("assigned trainer to unit %d\n", unit->id);
            ++train_count;
        }
    }
}

static void do_cycle(color_ostream& out,
        int32_t& melt_count, int32_t& trade_count,
        int32_t& dump_count, int32_t& train_count,
        int32_t& forbid_count, int32_t& claim_count) {
    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);
    cycle_timestamp = world->frame_counter;

    ProcessorStats melt_stats, trade_stats, dump_stats, train_stats, forbid_stats, claim_stats;
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
        bool forbid = 1 == c.get_int(STOCKPILE_CONFIG_FORBID);
        bool claim = 2 == c.get_int(STOCKPILE_CONFIG_FORBID);

        MeltStockProcessor melt_stock_processor(stockpile_number, melt, melt_stats, melt_masterworks);
        TradeStockProcessor trade_stock_processor(stockpile_number, trade, trade_stats);
        DumpStockProcessor dump_stock_processor(stockpile_number, dump, dump_stats);
        TrainStockProcessor train_stock_processor(stockpile_number, train, train_stats);
        ForbidStockProcessor forbid_stock_processor(stockpile_number, forbid, forbid_stats);
        ClaimStockProcessor claim_stock_processor(stockpile_number, claim, claim_stats);

        scan_stockpile(out, bld,
                melt_stock_processor, trade_stock_processor,
                dump_stock_processor, train_stock_processor,
                forbid_stock_processor, claim_stock_processor);
    }

    melt_count = melt_stats.newly_designated;
    trade_count = trade_stats.newly_designated;
    dump_count = dump_stats.newly_designated;
    train_count = train_stats.newly_designated;
    forbid_count = forbid_stats.newly_designated;
    claim_count = claim_stats.newly_designated;

    if (config.get_bool(CONFIG_TRAIN_PARTIAL)) {
        train_partials(out, train_count);
    }

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

    ProcessorStats melt_stats, trade_stats, dump_stats, train_stats, forbid_stats, claim_stats;

    for (auto bld : world->buildings.other.STOCKPILE) {
        int32_t stockpile_number = bld->stockpile_number;
        MeltStockProcessor melt_stock_processor(stockpile_number, false, melt_stats, false);
        TradeStockProcessor trade_stock_processor(stockpile_number, false, trade_stats);
        DumpStockProcessor dump_stock_processor(stockpile_number, false, dump_stats);
        TrainStockProcessor train_stock_processor(stockpile_number, false, train_stats);
        ForbidStockProcessor forbid_stock_processor(stockpile_number, false, forbid_stats);
        ClaimStockProcessor claim_stock_processor(stockpile_number, false, claim_stats);

        scan_stockpile(*out, bld,
                melt_stock_processor, trade_stock_processor,
                dump_stock_processor, train_stock_processor,
                forbid_stock_processor, claim_stock_processor);
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
    stats.emplace("forbid_designated", forbid_stats.designated_counts);
    stats.emplace("forbid_can_designate", forbid_stats.can_designate_counts);
    stats.emplace("claim_designated", claim_stats.designated_counts);
    stats.emplace("claim_can_designate", claim_stats.can_designate_counts);
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
        bool forbid = 1 == c.get_int(STOCKPILE_CONFIG_FORBID);
        bool claim = 2 == c.get_int(STOCKPILE_CONFIG_FORBID);

        unordered_map<string, string> sconfig;
        sconfig.emplace("melt", melt ? "true" : "false");
        sconfig.emplace("melt_masterworks", melt_masterworks ? "true" : "false");
        sconfig.emplace("trade", trade ? "true" : "false");
        sconfig.emplace("dump", dump ? "true" : "false");
        sconfig.emplace("train", train ? "true" : "false");
        sconfig.emplace("forbid", forbid ? "true" : "false");
        sconfig.emplace("claim", claim ? "true" : "false");

        configs.emplace(bld->stockpile_number, sconfig);
    }
    Lua::Push(L, configs);

    TRACE(cycle, *out).print("exit logistics_getStockpileData\n");

    return 2;
}

static void logistics_cycle(color_ostream &out, bool quiet = false) {
    DEBUG(control, out).print("entering logistics_cycle%s\n", quiet ? " (quiet)" : "");
    int32_t melt_count = 0, trade_count = 0, dump_count = 0, train_count = 0, forbid_count = 0, claim_count = 0;
    do_cycle(out, melt_count, trade_count, dump_count, train_count, forbid_count, claim_count);
    if (0 < melt_count || !quiet)
        out.print("logistics: designated %d item%s for melting\n", melt_count, (melt_count == 1) ? "" : "s");
    if (0 < trade_count || !quiet)
        out.print("logistics: designated %d item%s for trading\n", trade_count, (trade_count == 1) ? "" : "s");
    if (0 < dump_count || !quiet)
        out.print("logistics: designated %d item%s for dumping\n", dump_count, (dump_count == 1) ? "" : "s");
    if (0 < train_count || !quiet)
        out.print("logistics: designated %d animal%s for training\n", train_count, (train_count == 1) ? "" : "s");
    if (0 < forbid_count || !quiet)
        out.print("logistics: designated %d item%s forbidden\n", forbid_count, (forbid_count == 1) ? "" : "s");
    if (0 < claim_count || !quiet)
        out.print("logistics: claimed %d forbidden item%s\n", claim_count, (claim_count == 1) ? "" : "s");
}

static void find_stockpiles(lua_State *L, int idx,
        vector<df::building_stockpilest*> &sps) {
    if (lua_isnumber(L, idx)) {
        if (auto sp = find_stockpile(lua_tointeger(L, idx)))
            sps.emplace_back(sp);
        return;
    }

    const char * pname = lua_tostring(L, idx);
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
        stockpile_config.emplace("forbid", c.get_int(STOCKPILE_CONFIG_FORBID));
    } else {
        stockpile_config.emplace("melt", false);
        stockpile_config.emplace("melt_masterworks", false);
        stockpile_config.emplace("trade", false);
        stockpile_config.emplace("dump", false);
        stockpile_config.emplace("train", false);
        stockpile_config.emplace("forbid", 0);
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
    find_stockpiles(L, 1, sps);
    if (sps.empty())
        return 0;

    vector<unordered_map<string, int>> configs;
    for (auto sp : sps)
        configs.emplace_back(get_stockpile_config(sp->stockpile_number));
    Lua::PushVector(L, configs);
    return 1;
}

static void logistics_setStockpileConfig(color_ostream& out, int stockpile_number,
        bool melt, bool trade,
        bool dump, bool train,
        int forbid, bool melt_masterworks) {
    DEBUG(control, out).print("entering logistics_setStockpileConfig\n");
    DEBUG(control, out).print("stockpile_number=%d, melt=%d, trade=%d, dump=%d, train=%d, forbid=%d, melt_masterworks=%d\n",
            stockpile_number, melt, trade, dump, train, forbid, melt_masterworks);

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
    c.set_int(STOCKPILE_CONFIG_FORBID, forbid);
}

static int logistics_clearStockpileConfig(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control, *out).print("entering logistics_clearStockpileConfig\n");

    vector<df::building_stockpilest*> sps;
    find_stockpiles(L, 1, sps);
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

    size_t num_melt = world->items.other.ANY_MELT_DESIGNATED.size();

    size_t num_trade = 0;
    for (auto link = world->jobs.list.next; link; link = link->next) {
        df::job *job = link->item;
        if (job->job_type == df::job_type::BringItemToDepot)
            ++num_trade;
    }

    size_t num_dump = 0;
    size_t num_forbid = 0;
    size_t num_claim = 0;
    for (auto item : world->items.other.IN_PLAY) {
        if (item->flags.whole & bad_flags.whole)
            continue;
        if (item->flags.bits.dump)
            ++num_dump;
        if (item->flags.bits.forbid)
            ++num_forbid;
        else
            ++num_claim;
    }

    size_t num_train = 0;
    // TODO

    unordered_map<string, size_t> results;
    results.emplace("total_melt", num_melt);
    results.emplace("total_trade", num_trade);
    results.emplace("total_dump", num_dump);
    results.emplace("total_train", num_train);
    results.emplace("total_forbid", num_forbid);
    results.emplace("total_claim", num_claim);
    Lua::Push(L, results);

    TRACE(cycle, *out).print("exit logistics_getGlobalCounts\n");

    return 1;
}

static bool logistics_setFeature(color_ostream &out, bool enabled, string feature) {
    DEBUG(control, out).print("entering logistics_setFeature (enabled=%d, feature=%s)\n",
        enabled, feature.c_str());
    if (feature != "autoretrain")
        return false;
    config.set_bool(CONFIG_TRAIN_PARTIAL, enabled);
    if (is_enabled && enabled)
        logistics_cycle(out, false);
    return true;
}

static bool logistics_getFeature(color_ostream &out, string feature) {
    DEBUG(control, out).print("entering logistics_getFeature (feature=%s)\n", feature.c_str());
    if (feature != "autoretrain")
        return false;
    return config.get_bool(CONFIG_TRAIN_PARTIAL);
}

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(logistics_cycle),
    DFHACK_LUA_FUNCTION(logistics_setStockpileConfig),
    DFHACK_LUA_FUNCTION(logistics_clearAllStockpileConfigs),
    DFHACK_LUA_FUNCTION(logistics_setFeature),
    DFHACK_LUA_FUNCTION(logistics_getFeature),
    DFHACK_LUA_END};

DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(logistics_getStockpileData),
    DFHACK_LUA_COMMAND(logistics_getStockpileConfigs),
    DFHACK_LUA_COMMAND(logistics_clearStockpileConfig),
    DFHACK_LUA_COMMAND(logistics_getGlobalCounts),
    DFHACK_LUA_END};
