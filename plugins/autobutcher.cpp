// full automation of marking live-stock for slaughtering
// races can be added to a watchlist and it can be set how many male/female kids/adults are left alive
// adding to the watchlist can be automated as well.
// config for autobutcher (state and sleep setting) is saved the first time autobutcher is started
// config for watchlist entries is saved when they are created or modified

#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/building_cagest.h"
#include "df/building_civzonest.h"
#include "df/creature_raw.h"
#include "df/general_ref.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/world.h"

#include <queue>

using std::endl;
using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("autobutcher");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);

// logging levels can be dynamically controlled with the `debugfilter` command.
namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(autobutcher, control, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(autobutcher, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static const string WATCHLIST_CONFIG_KEY_PREFIX = string(plugin_name) + "/watchlist/";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_IS_ENABLED  = 0,
    // CONFIG_CYCLE_TICKS = 1, deprecated; no longer configurable
    CONFIG_AUTOWATCH   = 2,
    CONFIG_DEFAULT_FK  = 3,
    CONFIG_DEFAULT_MK  = 4,
    CONFIG_DEFAULT_FA  = 5,
    CONFIG_DEFAULT_MA  = 6,
};

struct WatchedRace;
// vector of races handled by autobutcher
// the name is a bit misleading since entries can be set to 'unwatched'
// to ignore them for a while but still keep the target count settings
static std::unordered_map<int, WatchedRace*> watched_races;
static std::unordered_map<string, int> race_to_id;

static const int32_t CYCLE_TICKS = 5987;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static void init_autobutcher(color_ostream &out);
static void cleanup_autobutcher(color_ostream &out);
static command_result df_autobutcher(color_ostream &out, vector<string> &parameters);
static void autobutcher_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        plugin_name,
        "Automatically butcher excess livestock.",
        df_autobutcher));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        // don't autorun cycle on first frame of fortress so we don't mark animals for butchering before
        // all initial configuration has been applied
        if (enable && plotinfo->fortress_age > 0)
            autobutcher_cycle(out);
    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(control,out).print("shutting down %s\n", plugin_name);
    cleanup_autobutcher(out);
    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        config.set_bool(CONFIG_AUTOWATCH, true);
        config.set_int(CONFIG_DEFAULT_FK, 4);
        config.set_int(CONFIG_DEFAULT_MK, 2);
        config.set_int(CONFIG_DEFAULT_FA, 4);
        config.set_int(CONFIG_DEFAULT_MA, 2);
    }

    // we have to copy our enabled flag into the global plugin variable, but
    // all the other state we can directly read/modify from the persistent
    // data structure.
    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");

    // load the persisted watchlist
    init_autobutcher(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(control,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
        cleanup_autobutcher(out);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        autobutcher_cycle(out);
    return CR_OK;
}

/////////////////////////////////////////////////////
// autobutcher config logic
//

struct autobutcher_options {
    // whether to display help
    bool help = false;

    // the command to run.
    string command;

    // the set of (unverified) races that the command should affect, and whether
    // "all" or "new" was specified as the race
    vector<string*> races;
    bool races_all = false;
    bool races_new = false;

    // params for the "target" command
    int32_t fk = -1;
    int32_t mk = -1;
    int32_t fa = -1;
    int32_t ma = -1;

    // how many ticks to wait between automatic cycles, -1 means unset
    int32_t ticks = -1;

    static struct_identity _identity;

    // non-virtual destructor so offsetof() still works for the fields
    ~autobutcher_options() {
        for (auto str : races)
            delete str;
    }
};
static const struct_field_info autobutcher_options_fields[] = {
    { struct_field_info::PRIMITIVE,      "help",      offsetof(autobutcher_options, help),      &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE,      "command",   offsetof(autobutcher_options, command),    df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::STL_VECTOR_PTR, "races",     offsetof(autobutcher_options, races),      df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::PRIMITIVE,      "races_all", offsetof(autobutcher_options, races_all), &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE,      "races_new", offsetof(autobutcher_options, races_new), &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE,      "fk",        offsetof(autobutcher_options, fk),        &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE,      "mk",        offsetof(autobutcher_options, mk),        &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE,      "fa",        offsetof(autobutcher_options, fa),        &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE,      "ma",        offsetof(autobutcher_options, ma),        &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE,      "ticks",     offsetof(autobutcher_options, ticks),     &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::END }
};
struct_identity autobutcher_options::_identity(sizeof(autobutcher_options), &df::allocator_fn<autobutcher_options>, NULL, "autobutcher_options", NULL, autobutcher_options_fields);

static bool isHighPriority(df::unit *unit) {
    return Units::isGay(unit) || Units::isGelded(unit);
}

static void doMarkForSlaughter(df::unit *unit) {
    DEBUG(cycle).print("marking unit %d for slaughter: %s, %s, high priority: %s, age: %.2f\n",
        unit->id, Units::getReadableName(unit).c_str(),
        Units::isFemale(unit) ? "female" : "male",
        isHighPriority(unit) ? "yes" : "no",
        Units::getAge(unit));
    unit->flags2.bits.slaughter = 1;
}

// returns true if b should be butchered before a
static bool compareKids(df::unit *a, df::unit *b) {
    if (isHighPriority(a) != isHighPriority(b))
        return isHighPriority(b);
    if (Units::isDomesticated(a) != Units::isDomesticated(b))
        return Units::isDomesticated(a);
    return Units::getAge(a, true) > Units::getAge(b, true);
}

// returns true if b should be butchered before a
static bool compareAdults(df::unit* a, df::unit* b) {
    if (isHighPriority(a) != isHighPriority(b))
        return isHighPriority(b);
    if (Units::isDomesticated(a) != Units::isDomesticated(b))
        return Units::isDomesticated(a);
    return Units::getAge(a, true) < Units::getAge(b, true);
}

struct WatchedRace {
public:
    PersistentDataItem rconfig;

    int raceId;
    bool isWatched; // if true, autobutcher will process this race

    // target amounts
    unsigned fk; // max female kids
    unsigned mk; // max male kids
    unsigned fa; // max female adults
    unsigned ma; // max male adults

    // amounts of protected (not butcherable) units
    unsigned fk_prot;
    unsigned fa_prot;
    unsigned mk_prot;
    unsigned ma_prot;

    // butcherable units
    typedef std::priority_queue<df::unit*, vector<df::unit*>, std::function<bool(df::unit*, df::unit*)>> UnitsPQ;
    UnitsPQ fk_units;
    UnitsPQ mk_units;
    UnitsPQ fa_units;
    UnitsPQ ma_units;

    WatchedRace(color_ostream &out, int id, bool watch, unsigned _fk, unsigned _mk, unsigned _fa, unsigned _ma)
        : raceId(id), isWatched(watch), fk(_fk), mk(_mk), fa(_fa), ma(_ma),
        fk_prot(0), fa_prot(0), mk_prot(0), ma_prot(0),
        fk_units(compareKids), mk_units(compareKids), fa_units(compareAdults), ma_units(compareAdults)
    {
        TRACE(control,out).print("creating new WatchedRace: id=%d, watched=%s, fk=%u, mk=%u, fa=%u, ma=%u\n",
                id, watch ? "true" : "false", fk, mk, fa, ma);
    }

    WatchedRace(color_ostream &out, const PersistentDataItem &p)
        : WatchedRace(out, p.ival(0), p.ival(1), p.ival(2), p.ival(3), p.ival(4), p.ival(5)) {
        rconfig = p;
    }

    ~WatchedRace() {
        ClearUnits();
    }

    void UpdateConfig(color_ostream &out) {
        if(!rconfig.isValid()) {
            string keyname = WATCHLIST_CONFIG_KEY_PREFIX + Units::getRaceNameById(raceId);
            rconfig = World::GetPersistentSiteData(keyname, true);
        }
        if(rconfig.isValid()) {
            rconfig.ival(0) = raceId;
            rconfig.ival(1) = isWatched;
            rconfig.ival(2) = fk;
            rconfig.ival(3) = mk;
            rconfig.ival(4) = fa;
            rconfig.ival(5) = ma;
        }
        else {
            ERR(control,out).print("could not create persistent key for race: %s",
                                  Units::getRaceNameById(raceId).c_str());
        }
    }

    void RemoveConfig(color_ostream &out) {
        if(!rconfig.isValid())
            return;
        World::DeletePersistentData(rconfig);
    }

    void PushButcherableUnit(df::unit *unit) {
        if(Units::isFemale(unit)) {
            if(Units::isBaby(unit) || Units::isChild(unit))
                fk_units.emplace(unit);
            else
                fa_units.emplace(unit);
        }
        else //treat sex n/a like it was male
        {
            if(Units::isBaby(unit) || Units::isChild(unit))
                mk_units.emplace(unit);
            else
                ma_units.emplace(unit);
        }
    }

    void PushProtectedUnit(df::unit *unit) {
        if(Units::isFemale(unit)) {
            if(Units::isBaby(unit) || Units::isChild(unit))
                fk_prot++;
            else
                fa_prot++;
        }
        else {  //treat sex n/a like it was male
            if(Units::isBaby(unit) || Units::isChild(unit))
                mk_prot++;
            else
                ma_prot++;
        }
    }

    void ClearUnits() {
        fk_prot = fa_prot = mk_prot = ma_prot = 0;
        fk_units = UnitsPQ(compareKids);
        fa_units = UnitsPQ(compareKids);
        mk_units = UnitsPQ(compareAdults);
        ma_units = UnitsPQ(compareAdults);
    }

    static int ProcessUnits(UnitsPQ& units, int limit) {
        limit = std::max(limit, 0);
        int count = 0;
        while (units.size() > (size_t)limit) {
            doMarkForSlaughter(units.top());
            units.pop();
            ++count;
        }
        return count;
    }

    int ProcessUnits() {
        int slaughter_count = 0;
        slaughter_count += ProcessUnits(fk_units, fk - fk_prot);
        slaughter_count += ProcessUnits(mk_units, mk - mk_prot);
        slaughter_count += ProcessUnits(fa_units, fa - fa_prot);
        slaughter_count += ProcessUnits(ma_units, ma - ma_prot);
        ClearUnits();
        return slaughter_count;
    }
};

static void init_autobutcher(color_ostream &out) {
    if (!race_to_id.size()) {
        const size_t num_races = world->raws.creatures.all.size();
        for(size_t i = 0; i < num_races; ++i)
            race_to_id.emplace(Units::getRaceNameById(i), i);
    }

    vector<PersistentDataItem> watchlist;
    World::GetPersistentSiteData(&watchlist, WATCHLIST_CONFIG_KEY_PREFIX, true);
    for (auto & p : watchlist) {
        DEBUG(control,out).print("Reading from save: %s\n", p.key().c_str());
        WatchedRace *w = new WatchedRace(out, p);
        watched_races.emplace(w->raceId, w);
    }
}

static void cleanup_autobutcher(color_ostream &out) {
    DEBUG(control,out).print("cleaning %s state\n", plugin_name);
    race_to_id.clear();
    for (auto w : watched_races)
        delete w.second;
    watched_races.clear();
}

static void autobutcher_export(color_ostream &out);
static void autobutcher_control(color_ostream &out);
static void autobutcher_target(color_ostream &out, const autobutcher_options &opts);
static void autobutcher_modify_watchlist(color_ostream &out, const autobutcher_options &opts);

static command_result df_autobutcher(color_ostream &out, vector<string> &parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    autobutcher_options opts;
    if (!Lua::CallLuaModuleFunction(out, "plugins.autobutcher", "parse_commandline", std::make_tuple(&opts, parameters))
            || opts.help)
        return CR_WRONG_USAGE;

    if (opts.command == "now") {
        autobutcher_cycle(out);
    }
    else if (opts.command == "autowatch") {
        config.set_bool(CONFIG_AUTOWATCH, true);
    }
    else if (opts.command == "noautowatch") {
        config.set_bool(CONFIG_AUTOWATCH, false);
    }
    else if (opts.command == "list_export") {
        autobutcher_export(out);
    }
    else if (opts.command == "target") {
        autobutcher_target(out, opts);
    }
    else if (opts.command == "watch" ||
            opts.command == "unwatch" ||
            opts.command == "forget") {
        autobutcher_modify_watchlist(out, opts);
    }
    else {
        autobutcher_control(out);
    }

    return CR_OK;
}

// helper for sorting the watchlist alphabetically
static bool compareRaceNames(WatchedRace* i, WatchedRace* j) {
    string name_i = Units::getRaceNamePluralById(i->raceId);
    string name_j = Units::getRaceNamePluralById(j->raceId);

    return name_i < name_j;
}

// sort watchlist alphabetically
static vector<WatchedRace *> getSortedWatchList() {
    vector<WatchedRace *> list;
    for (auto w : watched_races) {
        list.push_back(w.second);
    }
    sort(list.begin(), list.end(), compareRaceNames);
    return list;
}

static void autobutcher_export(color_ostream &out) {
    out << "enable autobutcher" << endl;
    out << "autobutcher " << (config.get_bool(CONFIG_AUTOWATCH) ? "" : "no")
        << "autowatch" << endl;
    out << "autobutcher target"
        << " " << config.get_int(CONFIG_DEFAULT_FK)
        << " " << config.get_int(CONFIG_DEFAULT_MK)
        << " " << config.get_int(CONFIG_DEFAULT_FA)
        << " " << config.get_int(CONFIG_DEFAULT_MA)
        << " new" << endl;

    for (auto w : getSortedWatchList()) {
        df::creature_raw *raw = world->raws.creatures.all[w->raceId];
        string name = raw->creature_id;
        out << "autobutcher target"
            << " " << w->fk
            << " " << w->mk
            << " " << w->fa
            << " " << w->ma
            << " " << name << endl;
        if (w->isWatched)
            out << "autobutcher watch " << name << endl;
    }
}

static void autobutcher_control(color_ostream &out) {
    out << "autobutcher is " << (is_enabled ? "" : "not ") << "enabled\n";
    out << "  " << (config.get_bool(CONFIG_AUTOWATCH) ? "" : "not ") << "autowatching for new races\n";

    out << "\ndefault setting for new races:"
        << " fk=" << config.get_int(CONFIG_DEFAULT_FK)
        << " mk=" << config.get_int(CONFIG_DEFAULT_MK)
        << " fa=" << config.get_int(CONFIG_DEFAULT_FA)
        << " ma=" << config.get_int(CONFIG_DEFAULT_MA)
        << endl << endl;

    if (!watched_races.size()) {
        out << "not currently watching any races. to find out how to add some, run:\n  help autobutcher" << endl;
        return;
    }

    out << "monitoring races: " << endl;
    for (auto w : getSortedWatchList()) {
        df::creature_raw *raw = world->raws.creatures.all[w->raceId];
        out << "  " << Units::getRaceNamePluralById(w->raceId) << "  \t";
        out << "(" << raw->creature_id;
        out << " fk=" << w->fk
            << " mk=" << w->mk
            << " fa=" << w->fa
            << " ma=" << w->ma;
        if (!w->isWatched)
            out << "; autobutchering is paused";
        out << ")" << endl;
    }
}

static void autobutcher_target(color_ostream &out, const autobutcher_options &opts) {
    if (opts.races_new) {
        DEBUG(control,out).print("setting targets for new races to fk=%u, mk=%u, fa=%u, ma=%u\n",
                                opts.fk, opts.mk, opts.fa, opts.ma);
        config.set_int(CONFIG_DEFAULT_FK, opts.fk);
        config.set_int(CONFIG_DEFAULT_MK, opts.mk);
        config.set_int(CONFIG_DEFAULT_FA, opts.fa);
        config.set_int(CONFIG_DEFAULT_MA, opts.ma);
    }

    if (opts.races_all) {
        DEBUG(control,out).print("setting targets for all races on watchlist to fk=%u, mk=%u, fa=%u, ma=%u\n",
                                opts.fk, opts.mk, opts.fa, opts.ma);
        for (auto w : watched_races) {
            w.second->fk = opts.fk;
            w.second->mk = opts.mk;
            w.second->fa = opts.fa;
            w.second->ma = opts.ma;
            w.second->UpdateConfig(out);
        }
    }

    for (auto race : opts.races) {
        if (!race_to_id.count(*race)) {
            out.printerr("race not found: '%s'", race->c_str());
            continue;
        }
        int id = race_to_id[*race];
        WatchedRace *w;
        if (!watched_races.count(id)) {
            w = new WatchedRace(out, id, true, opts.fk, opts.mk, opts.fa, opts.ma);
            watched_races.emplace(id, w);
        } else {
            w = watched_races[id];
            w->fk = opts.fk;
            w->mk = opts.mk;
            w->fa = opts.fa;
            w->ma = opts.ma;
        }
        w->UpdateConfig(out);
    }
}

static void autobutcher_modify_watchlist(color_ostream &out, const autobutcher_options &opts) {
    std::unordered_set<int> ids;

    if (opts.races_all) {
        for (auto w : watched_races)
            ids.emplace(w.first);
    }

    for (auto race : opts.races) {
        if (!race_to_id.count(*race)) {
            out.printerr("race not found: '%s'", race->c_str());
            continue;
        }
        ids.emplace(race_to_id[*race]);
    }

    for (int id : ids) {
        if (opts.command == "watch") {
            if (!watched_races.count(id)) {
                watched_races.emplace(id,
                    new WatchedRace(out, id, true,
                                    config.get_int(CONFIG_DEFAULT_FK),
                                    config.get_int(CONFIG_DEFAULT_MK),
                                    config.get_int(CONFIG_DEFAULT_FA),
                                    config.get_int(CONFIG_DEFAULT_MA)));
            }
            else if (!watched_races[id]->isWatched) {
                DEBUG(control,out).print("watching: %s\n", opts.command.c_str());
                watched_races[id]->isWatched = true;
            }
        }
        else if (opts.command == "unwatch") {
            if (!watched_races.count(id)) {
                watched_races.emplace(id,
                    new WatchedRace(out, id, false,
                                    config.get_int(CONFIG_DEFAULT_FK),
                                    config.get_int(CONFIG_DEFAULT_MK),
                                    config.get_int(CONFIG_DEFAULT_FA),
                                    config.get_int(CONFIG_DEFAULT_MA)));
            }
            else if (watched_races[id]->isWatched) {
                DEBUG(control,out).print("unwatching: %s\n", opts.command.c_str());
                watched_races[id]->isWatched = false;
            }
        }
        else if (opts.command == "forget") {
            if (watched_races.count(id)) {
                DEBUG(control,out).print("forgetting: %s\n", opts.command.c_str());
                watched_races[id]->RemoveConfig(out);
                delete watched_races[id];
                watched_races.erase(id);
            }
            continue;
        }
        watched_races[id]->UpdateConfig(out);
    }
}

/////////////////////////////////////////////////////
// cycle logic
//

// check if contained in item (e.g. animals in cages)
static bool isContainedInItem(df::unit *unit) {
    for (auto gref : unit->general_refs) {
        if (gref->getType() == df::general_ref_type::CONTAINED_IN_ITEM) {
            return true;
        }
    }
    return false;
}

// found a unit with weird position values on one of my maps (negative and in the thousands)
// it didn't appear in the animal stocks screen, but looked completely fine otherwise (alive, tame, own, etc)
// maybe a rare bug, but better avoid assigning such units to zones or slaughter etc.
static bool hasValidMapPos(df::unit *unit) {
    return unit->pos.x >= 0 && unit->pos.y >= 0 && unit->pos.z >= 0
        && unit->pos.x < world->map.x_count
        && unit->pos.y < world->map.y_count
        && unit->pos.z < world->map.z_count;
}

// built cage in a zone (supposed to detect zoo cages)
static bool isInBuiltCageRoom(df::unit *unit) {
    for (auto building : world->buildings.all) {
        if (building->getType() != df::building_type::Cage)
            continue;

        if (!building->relations.size())
            continue;

        df::building_cagest* cage = (df::building_cagest*)building;
        for (auto cu : cage->assigned_units)
            if (cu == unit->id) return true;
    }
    return false;
}

// This can be used to identify completely inappropriate units (dead, undead, not belonging to the fort, ...)
// that autobutcher should be ignoring.
static bool isInappropriateUnit(df::unit *unit) {
    return !Units::isActive(unit)
        || Units::isUndead(unit)
        || Units::isMerchant(unit) // ignore merchants' draft animals
        || Units::isForest(unit) // ignore merchants' caged animals
        || !Units::isOwnCiv(unit)
        || (!isContainedInItem(unit) && !hasValidMapPos(unit));
}

// This can be used to identify protected units that should be counted towards fort totals, but not scheduled
// for butchering. This way they count towards target quota, so if you order that you want 1 female adult cat
// and have 2 cats, one of them being a pet, the other gets butchered
static bool isProtectedUnit(df::unit *unit) {
    return Units::isWar(unit)    // ignore war dogs etc
        || Units::isHunter(unit) // ignore hunting dogs etc
        || Units::isMarkedForWarTraining(unit) // ignore units marked for any kind of training
        || Units::isMarkedForHuntTraining(unit)
        // ignore creatures in built cages which are defined as rooms to leave zoos alone
        // (TODO: better solution would be to allow some kind of slaughter cages which you can place near the butcher)
        || (isContainedInItem(unit) && isInBuiltCageRoom(unit))  // !!! see comments in isBuiltCageRoom()
        || (unit->pregnancy_timer != 0) // do not butcher pregnant animals (which includes brooding female egglayers)
        || Units::isAvailableForAdoption(unit)
        || unit->name.has_name
        || !unit->name.nickname.empty();
}

static void autobutcher_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    // check if there is anything to watch before walking through units vector
    if (!config.get_bool(CONFIG_AUTOWATCH)) {
        bool watching = false;
        for (auto w : watched_races) {
            if (w.second->isWatched) {
                watching = true;
                break;
            }
        }
        if (!watching)
            return;
    }

    for (auto unit : world->units.active) {
        // this check is now divided into two steps, squeezed autowatch into the middle
        // first one ignores completely inappropriate units (dead, undead, not belonging to the fort, ...)
        // then let autowatch add units to the watchlist which will probably start breeding (owned pets, war animals, ...)
        // then process units counting those which can't be butchered (war animals, named pets, ...)
        // so that they are treated as "own stock" as well and count towards the target quota
        if (isInappropriateUnit(unit)
            || Units::isMarkedForSlaughter(unit)
            || !Units::isTame(unit))
            continue;

        WatchedRace *w;
        if (watched_races.count(unit->race)) {
            w = watched_races[unit->race];
        }
        else if (!config.get_bool(CONFIG_AUTOWATCH)) {
            continue;
        }
        else {
            w = new WatchedRace(out, unit->race, true, config.get_int(CONFIG_DEFAULT_FK),
                config.get_int(CONFIG_DEFAULT_MK), config.get_int(CONFIG_DEFAULT_FA),
                config.get_int(CONFIG_DEFAULT_MA));
            w->UpdateConfig(out);
            watched_races.emplace(unit->race, w);

            INFO(cycle,out).print("New race added to autobutcher watchlist: %s\n",
                Units::getRaceNamePluralById(unit->race).c_str());
        }

        if (w->isWatched) {
            // don't butcher protected units, but count them as stock as well
            // this way they count towards target quota, so if you order that you want 1 female adult cat
            // and have 2 cats, one of them being a pet, the other gets butchered
            if(isProtectedUnit(unit))
                w->PushProtectedUnit(unit);
            else
                w->PushButcherableUnit(unit);
        }
    }

    for (auto w : watched_races) {
        int slaughter_count = w.second->ProcessUnits();
        if (slaughter_count) {
            std::stringstream ss;
            ss << slaughter_count;
            INFO(cycle,out).print("%s marked for slaughter: %s\n",
                Units::getRaceNamePluralById(w.first).c_str(), ss.str().c_str());
        }
    }
}

/////////////////////////////////////
// API functions to control autobutcher with a lua script

// abuse WatchedRace struct for counting stocks (since it sorts by gender and age)
// calling method must delete pointer!
static WatchedRace * checkRaceStocksTotal(color_ostream &out, int race) {
    WatchedRace * w = new WatchedRace(out, race, true, 0, 0, 0, 0);
    for (auto unit : world->units.active) {
        if (unit->race != race)
            continue;

        if (isInappropriateUnit(unit))
            continue;

        w->PushButcherableUnit(unit);
    }
    return w;
}

WatchedRace * checkRaceStocksProtected(color_ostream &out, int race) {
    WatchedRace * w = new WatchedRace(out, race, true, 0, 0, 0, 0);
    for (auto unit : world->units.active) {
        if (unit->race != race)
            continue;

        if (isInappropriateUnit(unit))
            continue;

        if (!Units::isTame(unit) || isProtectedUnit(unit))
            w->PushButcherableUnit(unit);
    }
    return w;
}

WatchedRace * checkRaceStocksButcherable(color_ostream &out, int race) {
    WatchedRace * w = new WatchedRace(out, race, true, 0, 0, 0, 0);
    for (auto unit : world->units.active) {
        if (unit->race != race)
            continue;

        if (   isInappropriateUnit(unit)
            || !Units::isTame(unit)
            || isProtectedUnit(unit)
            )
            continue;

        w->PushButcherableUnit(unit);
    }
    return w;
}

WatchedRace * checkRaceStocksButcherFlag(color_ostream &out, int race) {
    WatchedRace * w = new WatchedRace(out, race, true, 0, 0, 0, 0);
    for (auto unit : world->units.active) {
        if(unit->race != race)
            continue;

        if (isInappropriateUnit(unit))
            continue;

        if (Units::isMarkedForSlaughter(unit))
            w->PushButcherableUnit(unit);
    }
    return w;
}

static bool autowatch_isEnabled() {
    return config.get_bool(CONFIG_AUTOWATCH);
}

static void autowatch_setEnabled(color_ostream &out, bool enable) {
    DEBUG(control,out).print("auto-adding to watchlist %s\n", enable ? "started" : "stopped");
    config.set_bool(CONFIG_AUTOWATCH, enable);
    if (config.get_bool(CONFIG_IS_ENABLED))
        autobutcher_cycle(out);
}

// set all data for a watchlist race in one go
// if race is not already on watchlist it will be added
// params: (id, fk, mk, fa, ma, watched)
static void autobutcher_setWatchListRace(color_ostream &out, unsigned id, unsigned fk, unsigned mk, unsigned fa, unsigned ma, bool watched) {
    if (watched_races.count(id)) {
        DEBUG(control,out).print("updating watchlist entry\n");
        WatchedRace * w = watched_races[id];
        w->fk = fk;
        w->mk = mk;
        w->fa = fa;
        w->ma = ma;
        w->isWatched = watched;
        w->UpdateConfig(out);
        return;
    }

    DEBUG(control,out).print("creating new watchlist entry\n");
    WatchedRace * w = new WatchedRace(out, id, watched, fk, mk, fa, ma);
    w->UpdateConfig(out);
    watched_races.emplace(id, w);
    INFO(control,out).print("New race added to autobutcher watchlist: %s\n",
        Units::getRaceNamePluralById(id).c_str());
}

// remove entry from watchlist
static void autobutcher_removeFromWatchList(color_ostream &out, unsigned id) {
    if (watched_races.count(id)) {
        DEBUG(control,out).print("removing watchlist entry\n");
        WatchedRace * w = watched_races[id];
        w->RemoveConfig(out);
        watched_races.erase(id);
    }
}

// set default target values for new races
static void autobutcher_setDefaultTargetNew(color_ostream &out, unsigned fk, unsigned mk, unsigned fa, unsigned ma) {
    config.set_int(CONFIG_DEFAULT_FK, fk);
    config.set_int(CONFIG_DEFAULT_MK, mk);
    config.set_int(CONFIG_DEFAULT_FA, fa);
    config.set_int(CONFIG_DEFAULT_MA, ma);
}

// set default target values for ALL races (update watchlist and set new default)
static void autobutcher_setDefaultTargetAll(color_ostream &out, unsigned fk, unsigned mk, unsigned fa, unsigned ma) {
    for (auto w : watched_races) {
        w.second->fk = fk;
        w.second->mk = mk;
        w.second->fa = fa;
        w.second->ma = ma;
        w.second->UpdateConfig(out);
    }
    autobutcher_setDefaultTargetNew(out, fk, mk, fa, ma);
}

static void autobutcher_butcherRace(color_ostream &out, int id) {
    for (auto unit : world->units.active) {
        if(unit->race != id)
            continue;

        if(    isInappropriateUnit(unit)
            || !Units::isTame(unit)
            || isProtectedUnit(unit)
            )
            continue;

        doMarkForSlaughter(unit);
    }
}

// remove butcher flag for all units of a given race
static void autobutcher_unbutcherRace(color_ostream &out, int id) {
    for (auto unit : world->units.active) {
        if(unit->race != id)
            continue;

        if(isInappropriateUnit(unit) || !Units::isMarkedForSlaughter(unit))
            continue;

        unit->flags2.bits.slaughter = 0;
    }
}

// push autobutcher settings on lua stack
static int autobutcher_getSettings(lua_State *L) {
    lua_newtable(L);
    int ctable = lua_gettop(L);
    Lua::SetField(L, config.get_bool(CONFIG_IS_ENABLED), ctable, "enable_autobutcher");
    Lua::SetField(L, config.get_bool(CONFIG_AUTOWATCH), ctable, "enable_autowatch");
    Lua::SetField(L, config.get_int(CONFIG_DEFAULT_FK), ctable, "fk");
    Lua::SetField(L, config.get_int(CONFIG_DEFAULT_MK), ctable, "mk");
    Lua::SetField(L, config.get_int(CONFIG_DEFAULT_FA), ctable, "fa");
    Lua::SetField(L, config.get_int(CONFIG_DEFAULT_MA), ctable, "ma");
    return 1;
}

// push the watchlist vector as nested table on the lua stack
static int autobutcher_getWatchList(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();

    lua_newtable(L);
    int entry_index = 0;
    for (auto wr : watched_races) {
        lua_newtable(L);
        int ctable = lua_gettop(L);

        WatchedRace * w = wr.second;
        int id = w->raceId;
        Lua::SetField(L, id, ctable, "id");
        Lua::SetField(L, w->isWatched, ctable, "watched");
        Lua::SetField(L, Units::getRaceNamePluralById(id), ctable, "name");
        Lua::SetField(L, w->fk, ctable, "fk");
        Lua::SetField(L, w->mk, ctable, "mk");
        Lua::SetField(L, w->fa, ctable, "fa");
        Lua::SetField(L, w->ma, ctable, "ma");

        WatchedRace *tally = checkRaceStocksTotal(*out, id);
        Lua::SetField(L, tally->fk_units.size(), ctable, "fk_total");
        Lua::SetField(L, tally->mk_units.size(), ctable, "mk_total");
        Lua::SetField(L, tally->fa_units.size(), ctable, "fa_total");
        Lua::SetField(L, tally->ma_units.size(), ctable, "ma_total");
        delete tally;

        tally = checkRaceStocksProtected(*out, id);
        Lua::SetField(L, tally->fk_units.size(), ctable, "fk_protected");
        Lua::SetField(L, tally->mk_units.size(), ctable, "mk_protected");
        Lua::SetField(L, tally->fa_units.size(), ctable, "fa_protected");
        Lua::SetField(L, tally->ma_units.size(), ctable, "ma_protected");
        delete tally;

        tally = checkRaceStocksButcherable(*out, id);
        Lua::SetField(L, tally->fk_units.size(), ctable, "fk_butcherable");
        Lua::SetField(L, tally->mk_units.size(), ctable, "mk_butcherable");
        Lua::SetField(L, tally->fa_units.size(), ctable, "fa_butcherable");
        Lua::SetField(L, tally->ma_units.size(), ctable, "ma_butcherable");
        delete tally;

        tally = checkRaceStocksButcherFlag(*out, id);
        Lua::SetField(L, tally->fk_units.size(), ctable, "fk_butcherflag");
        Lua::SetField(L, tally->mk_units.size(), ctable, "mk_butcherflag");
        Lua::SetField(L, tally->fa_units.size(), ctable, "fa_butcherflag");
        Lua::SetField(L, tally->ma_units.size(), ctable, "ma_butcherflag");
        delete tally;

        lua_rawseti(L, -2, ++entry_index);
    }

    return 1;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(autowatch_isEnabled),
    DFHACK_LUA_FUNCTION(autowatch_setEnabled),
    DFHACK_LUA_FUNCTION(autobutcher_setWatchListRace),
    DFHACK_LUA_FUNCTION(autobutcher_setDefaultTargetNew),
    DFHACK_LUA_FUNCTION(autobutcher_setDefaultTargetAll),
    DFHACK_LUA_FUNCTION(autobutcher_butcherRace),
    DFHACK_LUA_FUNCTION(autobutcher_unbutcherRace),
    DFHACK_LUA_FUNCTION(autobutcher_removeFromWatchList),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(autobutcher_getSettings),
    DFHACK_LUA_COMMAND(autobutcher_getWatchList),
    DFHACK_LUA_END
};
