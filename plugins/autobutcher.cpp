// full automation of marking live-stock for slaughtering
// races can be added to a watchlist and it can be set how many male/female kids/adults are left alive
// adding to the watchlist can be automated as well.
// config for autobutcher (state and sleep setting) is saved the first time autobutcher is started
// config for watchlist entries is saved when they are created or modified

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "df/building_cagest.h"
#include "df/building_civzonest.h"
#include "df/creature_raw.h"
#include "df/world.h"

#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

using std::endl;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("autobutcher");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

// logging levels can be dynamically controlled with the `debugfilter` command.
namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(autobutcher, status, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(autobutcher, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static const string WATCHLIST_CONFIG_KEY_PREFIX = string(plugin_name) + "/watchlist/";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_IS_ENABLED  = 0,
    CONFIG_CYCLE_TICKS = 1,
    CONFIG_AUTOWATCH   = 2,
    CONFIG_DEFAULT_FK  = 3,
    CONFIG_DEFAULT_MK  = 4,
    CONFIG_DEFAULT_FA  = 5,
    CONFIG_DEFAULT_MA  = 6,
};
static int get_config_val(int index) {
    if (!config.isValid())
        return -1;
    return config.ival(index);
}
static bool get_config_bool(int index) {
    return get_config_val(index) == 1;
}
static void set_config_val(int index, int value) {
    if (config.isValid())
        config.ival(index) = value;
}
static void set_config_bool(int index, bool value) {
    set_config_val(index, value ? 1 : 0);
}

struct WatchedRace;
// vector of races handled by autobutcher
// the name is a bit misleading since entries can be set to 'unwatched'
// to ignore them for a while but still keep the target count settings
static unordered_map<int, WatchedRace*> watched_races;
static unordered_map<string, int> race_to_id;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static void init_autobutcher(color_ostream &out);
static void cleanup_autobutcher(color_ostream &out);
static command_result df_autobutcher(color_ostream &out, vector<string> &parameters);
static void autobutcher_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        plugin_name,
        "Automatically butcher excess livestock.",
        df_autobutcher));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot enable %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(status,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        set_config_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            autobutcher_cycle(out);
    } else {
        DEBUG(status,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(status,out).print("shutting down %s\n", plugin_name);
    cleanup_autobutcher(out);
    return CR_OK;
}

DFhackCExport command_result plugin_load_data (color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(status,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
        set_config_bool(CONFIG_IS_ENABLED, is_enabled);
        set_config_val(CONFIG_CYCLE_TICKS, 6000);
        set_config_bool(CONFIG_AUTOWATCH, true);
        set_config_val(CONFIG_DEFAULT_FK, 4);
        set_config_val(CONFIG_DEFAULT_MK, 2);
        set_config_val(CONFIG_DEFAULT_FA, 4);
        set_config_val(CONFIG_DEFAULT_MA, 2);
    }

    // we have to copy our enabled flag into the global plugin variable, but
    // all the other state we can directly read/modify from the persistent
    // data structure.
    is_enabled = get_config_bool(CONFIG_IS_ENABLED);
    DEBUG(status,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");

    // load the persisted watchlist
    init_autobutcher(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(status,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
        cleanup_autobutcher(out);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (is_enabled && world->frame_counter - cycle_timestamp >= get_config_val(CONFIG_CYCLE_TICKS))
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

static bool get_options(color_ostream &out,
                        autobutcher_options &opts,
                        const vector<string> &parameters)
{
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, parameters.size() + 2) ||
        !Lua::PushModulePublic(
            out, L, "plugins.autobutcher", "parse_commandline")) {
        out.printerr("Failed to load autobutcher Lua code\n");
        return false;
    }

    Lua::Push(L, &opts);
    for (const string &param : parameters)
        Lua::Push(L, param);

    if (!Lua::SafeCall(out, L, parameters.size() + 1, 0))
        return false;

    return true;
}

static void doMarkForSlaughter(df::unit *unit) {
    unit->flags2.bits.slaughter = 1;
}

// getUnitAge() returns 0 if born in current year, therefore the look at birth_time in that case
// (assuming that the value from there indicates in which tick of the current year the unit was born)
static bool compareUnitAgesYounger(df::unit *i, df::unit *j) {
    int32_t age_i = (int32_t)Units::getAge(i, true);
    int32_t age_j = (int32_t)Units::getAge(j, true);
    if (age_i == 0 && age_j == 0) {
        age_i = i->birth_time;
        age_j = j->birth_time;
    }
    return age_i < age_j;
}

static bool compareUnitAgesOlder(df::unit* i, df::unit* j) {
    int32_t age_i = (int32_t)Units::getAge(i, true);
    int32_t age_j = (int32_t)Units::getAge(j, true);
    if(age_i == 0 && age_j == 0) {
        age_i = i->birth_time;
        age_j = j->birth_time;
    }
    return age_i > age_j;
}

enum unit_ptr_index {
 fk_index = 0,
 mk_index = 1,
 fa_index = 2,
 ma_index = 3
};

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
    vector<df::unit*> unit_ptr[4];

    // priority butcherable units
    vector<df::unit*> prot_ptr[4];

    WatchedRace(color_ostream &out, int id, bool watch, unsigned _fk, unsigned _mk, unsigned _fa, unsigned _ma) {
        raceId = id;
        isWatched = watch;
        fk = _fk;
        mk = _mk;
        fa = _fa;
        ma = _ma;
        fk_prot = fa_prot = mk_prot = ma_prot = 0;

        DEBUG(status,out).print("creating new WatchedRace: id=%d, watched=%s, fk=%u, mk=%u, fa=%u, ma=%u\n",
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
            rconfig = World::GetPersistentData(keyname, NULL);
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
            ERR(status,out).print("could not create persistent key for race: %s",
                                  Units::getRaceNameById(raceId).c_str());
        }
    }

    void RemoveConfig(color_ostream &out) {
        if(!rconfig.isValid())
            return;
        World::DeletePersistentData(rconfig);
    }

    void SortUnitsByAge() {
        sort(unit_ptr[fk_index].begin(), unit_ptr[fk_index].end(), compareUnitAgesOlder);
        sort(unit_ptr[mk_index].begin(), unit_ptr[mk_index].end(), compareUnitAgesOlder);
        sort(unit_ptr[fa_index].begin(), unit_ptr[fa_index].end(), compareUnitAgesYounger);
        sort(unit_ptr[ma_index].begin(), unit_ptr[ma_index].end(), compareUnitAgesYounger);
        sort(prot_ptr[fk_index].begin(), prot_ptr[fk_index].end(), compareUnitAgesOlder);
        sort(prot_ptr[mk_index].begin(), prot_ptr[mk_index].end(), compareUnitAgesOlder);
        sort(prot_ptr[fa_index].begin(), prot_ptr[fa_index].end(), compareUnitAgesYounger);
        sort(prot_ptr[ma_index].begin(), prot_ptr[ma_index].end(), compareUnitAgesYounger);
    }

    void PushUnit(df::unit *unit) {
        if(Units::isFemale(unit)) {
            if(Units::isBaby(unit) || Units::isChild(unit))
                unit_ptr[fk_index].push_back(unit);
            else
                unit_ptr[fa_index].push_back(unit);
        }
        else //treat sex n/a like it was male
        {
            if(Units::isBaby(unit) || Units::isChild(unit))
                unit_ptr[mk_index].push_back(unit);
            else
                unit_ptr[ma_index].push_back(unit);
        }
    }

    void PushPriorityUnit(df::unit *unit) {
        if(Units::isFemale(unit)) {
            if(Units::isBaby(unit) || Units::isChild(unit))
                prot_ptr[fk_index].push_back(unit);
            else
                prot_ptr[fa_index].push_back(unit);
        }
        else {
            if(Units::isBaby(unit) || Units::isChild(unit))
                prot_ptr[mk_index].push_back(unit);
            else
                prot_ptr[ma_index].push_back(unit);
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
        for (size_t i = 0; i < 4; i++) {
            unit_ptr[i].clear();
            prot_ptr[i].clear();
        }
    }

    int ProcessUnits(vector<df::unit*>& unit_ptr, vector<df::unit*>& unit_pri_ptr, unsigned prot, unsigned goal) {
        int subcount = 0;
        while (unit_pri_ptr.size() && (unit_ptr.size() + unit_pri_ptr.size() + prot > goal)) {
            df::unit *unit = unit_pri_ptr.back();
            doMarkForSlaughter(unit);
            unit_pri_ptr.pop_back();
            subcount++;
        }
        while (unit_ptr.size() && (unit_ptr.size() + prot > goal)) {
            df::unit *unit = unit_ptr.back();
            doMarkForSlaughter(unit);
            unit_ptr.pop_back();
            subcount++;
        }
        return subcount;
    }

    int ProcessUnits() {
        SortUnitsByAge();
        int slaughter_count = 0;
        slaughter_count += ProcessUnits(unit_ptr[fk_index], prot_ptr[fk_index], fk_prot, fk);
        slaughter_count += ProcessUnits(unit_ptr[mk_index], prot_ptr[mk_index], mk_prot, mk);
        slaughter_count += ProcessUnits(unit_ptr[fa_index], prot_ptr[fa_index], fa_prot, fa);
        slaughter_count += ProcessUnits(unit_ptr[ma_index], prot_ptr[ma_index], ma_prot, ma);
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

    std::vector<PersistentDataItem> watchlist;
    World::GetPersistentData(&watchlist, WATCHLIST_CONFIG_KEY_PREFIX, true);
    for (auto & p : watchlist) {
        DEBUG(status,out).print("Reading from save: %s\n", p.key().c_str());
        WatchedRace *w = new WatchedRace(out, p);
        watched_races.emplace(w->raceId, w);
    }
}

static void cleanup_autobutcher(color_ostream &out) {
    DEBUG(status,out).print("cleaning %s state\n", plugin_name);
    race_to_id.clear();
    for (auto w : watched_races)
        delete w.second;
    watched_races.clear();
}

static void autobutcher_export(color_ostream &out);
static void autobutcher_status(color_ostream &out);
static void autobutcher_target(color_ostream &out, const autobutcher_options &opts);
static void autobutcher_modify_watchlist(color_ostream &out, const autobutcher_options &opts);

static command_result df_autobutcher(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot run %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    autobutcher_options opts;
    if (!get_options(out, opts, parameters) || opts.help)
        return CR_WRONG_USAGE;

    if (opts.command == "now") {
        autobutcher_cycle(out);
    }
    else if (opts.command == "autowatch") {
        set_config_bool(CONFIG_AUTOWATCH, true);
    }
    else if (opts.command == "noautowatch") {
        set_config_bool(CONFIG_AUTOWATCH, false);
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
    else if (opts.command == "ticks") {
        set_config_val(CONFIG_CYCLE_TICKS, opts.ticks);
        INFO(status,out).print("New cycle timer: %d ticks.\n", opts.ticks);
    }
    else {
        autobutcher_status(out);
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
    out << "autobutcher ticks " << get_config_val(CONFIG_CYCLE_TICKS) << endl;
    out << "autobutcher " << (get_config_bool(CONFIG_AUTOWATCH) ? "" : "no")
        << "autowatch" << endl;
    out << "autobutcher target"
        << " " << get_config_val(CONFIG_DEFAULT_FK)
        << " " << get_config_val(CONFIG_DEFAULT_MK)
        << " " << get_config_val(CONFIG_DEFAULT_FA)
        << " " << get_config_val(CONFIG_DEFAULT_MA)
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

static void autobutcher_status(color_ostream &out) {
    out << "autobutcher is " << (is_enabled ? "" : "not ") << "enabled\n";
    if (is_enabled)
        out << "  running every " << get_config_val(CONFIG_CYCLE_TICKS) << " game ticks\n";
    out << "  " << (get_config_bool(CONFIG_AUTOWATCH) ? "" : "not ") << "autowatching for new races\n";

    out << "\ndefault setting for new races:"
        << " fk=" << get_config_val(CONFIG_DEFAULT_FK)
        << " mk=" << get_config_val(CONFIG_DEFAULT_MK)
        << " fa=" << get_config_val(CONFIG_DEFAULT_FA)
        << " ma=" << get_config_val(CONFIG_DEFAULT_MA)
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
        DEBUG(status,out).print("setting targets for new races to fk=%u, mk=%u, fa=%u, ma=%u\n",
                                opts.fk, opts.mk, opts.fa, opts.ma);
        set_config_val(CONFIG_DEFAULT_FK, opts.fk);
        set_config_val(CONFIG_DEFAULT_MK, opts.mk);
        set_config_val(CONFIG_DEFAULT_FA, opts.fa);
        set_config_val(CONFIG_DEFAULT_MA, opts.ma);
    }

    if (opts.races_all) {
        DEBUG(status,out).print("setting targets for all races on watchlist to fk=%u, mk=%u, fa=%u, ma=%u\n",
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
    unordered_set<int> ids;

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
                                    get_config_val(CONFIG_DEFAULT_FK),
                                    get_config_val(CONFIG_DEFAULT_MK),
                                    get_config_val(CONFIG_DEFAULT_FA),
                                    get_config_val(CONFIG_DEFAULT_MA)));
            }
            else if (!watched_races[id]->isWatched) {
                DEBUG(status,out).print("watching: %s\n", opts.command.c_str());
                watched_races[id]->isWatched = true;
            }
        }
        else if (opts.command == "unwatch") {
            if (!watched_races.count(id)) {
                watched_races.emplace(id,
                    new WatchedRace(out, id, false,
                                    get_config_val(CONFIG_DEFAULT_FK),
                                    get_config_val(CONFIG_DEFAULT_MK),
                                    get_config_val(CONFIG_DEFAULT_FA),
                                    get_config_val(CONFIG_DEFAULT_MA)));
            }
            else if (watched_races[id]->isWatched) {
                DEBUG(status,out).print("unwatching: %s\n", opts.command.c_str());
                watched_races[id]->isWatched = false;
            }
        }
        else if (opts.command == "forget") {
            if (watched_races.count(id)) {
                DEBUG(status,out).print("forgetting: %s\n", opts.command.c_str());
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

static void autobutcher_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    // check if there is anything to watch before walking through units vector
    if (!get_config_bool(CONFIG_AUTOWATCH)) {
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

    for (auto unit : world->units.all) {
        // this check is now divided into two steps, squeezed autowatch into the middle
        // first one ignores completely inappropriate units (dead, undead, not belonging to the fort, ...)
        // then let autowatch add units to the watchlist which will probably start breeding (owned pets, war animals, ...)
        // then process units counting those which can't be butchered (war animals, named pets, ...)
        // so that they are treated as "own stock" as well and count towards the target quota
        if (   !Units::isActive(unit)
            || Units::isUndead(unit)
            || Units::isMarkedForSlaughter(unit)
            || Units::isMerchant(unit) // ignore merchants' draft animals
            || Units::isForest(unit) // ignore merchants' caged animals
            || !Units::isOwnCiv(unit)
            || !Units::isTame(unit)
            )
            continue;

        // found a bugged unit which had invalid coordinates but was not in a cage.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        WatchedRace *w;
        if (watched_races.count(unit->race)) {
            w = watched_races[unit->race];
        }
        else if (!get_config_bool(CONFIG_AUTOWATCH)) {
            continue;
        }
        else {
            w = new WatchedRace(out, unit->race, true, get_config_val(CONFIG_DEFAULT_FK),
                get_config_val(CONFIG_DEFAULT_MK), get_config_val(CONFIG_DEFAULT_FA),
                get_config_val(CONFIG_DEFAULT_MA));
            w->UpdateConfig(out);
            watched_races.emplace(unit->race, w);

            string announce = "New race added to autobutcher watchlist: " + Units::getRaceNamePluralById(unit->race);
            Gui::showAnnouncement(announce, 2, false);
        }

        if (w->isWatched) {
            // don't butcher protected units, but count them as stock as well
            // this way they count towards target quota, so if you order that you want 1 female adult cat
            // and have 2 cats, one of them being a pet, the other gets butchered
            if(    Units::isWar(unit)    // ignore war dogs etc
                || Units::isHunter(unit) // ignore hunting dogs etc
                // ignore creatures in built cages which are defined as rooms to leave zoos alone
                // (TODO: better solution would be to allow some kind of slaughter cages which you can place near the butcher)
                || (isContainedInItem(unit) && isInBuiltCageRoom(unit))  // !!! see comments in isBuiltCageRoom()
                || Units::isAvailableForAdoption(unit)
                || unit->name.has_name)
                w->PushProtectedUnit(unit);
            else if (   Units::isGay(unit)
                     || Units::isGelded(unit))
                w->PushPriorityUnit(unit);
            else
                w->PushUnit(unit);
        }
    }

    for (auto w : watched_races) {
        int slaughter_count = w.second->ProcessUnits();
        if (slaughter_count) {
            std::stringstream ss;
            ss << slaughter_count;
            string announce = Units::getRaceNamePluralById(w.first) + " marked for slaughter: " + ss.str();
            DEBUG(cycle,out).print("%s\n", announce.c_str());
            Gui::showAnnouncement(announce, 2, false);
        }
    }
}

/////////////////////////////////////
// API functions to control autobutcher with a lua script

// abuse WatchedRace struct for counting stocks (since it sorts by gender and age)
// calling method must delete pointer!
static WatchedRace * checkRaceStocksTotal(color_ostream &out, int race) {
    WatchedRace * w = new WatchedRace(out, race, true, 0, 0, 0, 0);
    for (auto unit : world->units.all) {
        if (unit->race != race)
            continue;

        if (   !Units::isActive(unit)
            || Units::isUndead(unit)
            || Units::isMerchant(unit) // ignore merchants' draft animals
            || Units::isForest(unit) // ignore merchants' caged animals
            || !Units::isOwnCiv(unit)
            )
            continue;

        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        w->PushUnit(unit);
    }
    return w;
}

WatchedRace * checkRaceStocksProtected(color_ostream &out, int race) {
    WatchedRace * w = new WatchedRace(out, race, true, 0, 0, 0, 0);
    for (auto unit : world->units.all) {
        if (unit->race != race)
            continue;

        if (   !Units::isActive(unit)
            || Units::isUndead(unit)
            || Units::isMerchant(unit) // ignore merchants' draft animals
            || Units::isForest(unit) // ignore merchants' caged animals
            || !Units::isOwnCiv(unit)
            )
            continue;

        // found a bugged unit which had invalid coordinates but was not in a cage.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if (!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        if (  !Units::isTame(unit)
           || Units::isWar(unit) // ignore war dogs etc
           || Units::isHunter(unit) // ignore hunting dogs etc
            // ignore creatures in built cages which are defined as rooms to leave zoos alone
            // (TODO: better solution would be to allow some kind of slaughter cages which you can place near the butcher)
            || (isContainedInItem(unit) && isInBuiltCageRoom(unit))  // !!! see comments in isBuiltCageRoom()
            || Units::isAvailableForAdoption(unit)
            || unit->name.has_name )
            w->PushUnit(unit);
    }
    return w;
}

WatchedRace * checkRaceStocksButcherable(color_ostream &out, int race) {
    WatchedRace * w = new WatchedRace(out, race, true, 0, 0, 0, 0);
    for (auto unit : world->units.all) {
        if (unit->race != race)
            continue;

        if (   !Units::isActive(unit)
            || Units::isUndead(unit)
            || Units::isMerchant(unit) // ignore merchants' draft animals
            || Units::isForest(unit) // ignore merchants' caged animals
            || !Units::isOwnCiv(unit)
            || !Units::isTame(unit)
            || Units::isWar(unit) // ignore war dogs etc
            || Units::isHunter(unit) // ignore hunting dogs etc
            // ignore creatures in built cages which are defined as rooms to leave zoos alone
            // (TODO: better solution would be to allow some kind of slaughter cages which you can place near the butcher)
            || (isContainedInItem(unit) && isInBuiltCageRoom(unit))  // !!! see comments in isBuiltCageRoom()
            || Units::isAvailableForAdoption(unit)
            || unit->name.has_name
            )
            continue;

        if (!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        w->PushUnit(unit);
    }
    return w;
}

WatchedRace * checkRaceStocksButcherFlag(color_ostream &out, int race) {
    WatchedRace * w = new WatchedRace(out, race, true, 0, 0, 0, 0);
    for (auto unit : world->units.all) {
        if(unit->race != race)
            continue;

        if (   !Units::isActive(unit)
            || Units::isUndead(unit)
            || Units::isMerchant(unit) // ignore merchants' draft animals
            || Units::isForest(unit) // ignore merchants' caged animals
            || !Units::isOwnCiv(unit)
            )
            continue;

        if (!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        if (Units::isMarkedForSlaughter(unit))
            w->PushUnit(unit);
    }
    return w;
}

static bool autowatch_isEnabled() {
    return get_config_bool(CONFIG_AUTOWATCH);
}

static unsigned autobutcher_getSleep(color_ostream &out) {
    return get_config_val(CONFIG_CYCLE_TICKS);
}

static void autobutcher_setSleep(color_ostream &out, unsigned ticks) {

    set_config_val(CONFIG_CYCLE_TICKS, ticks);
}

static void autowatch_setEnabled(color_ostream &out, bool enable) {
    DEBUG(status,out).print("auto-adding to watchlist %s\n", enable ? "started" : "stopped");
    set_config_bool(CONFIG_AUTOWATCH, enable);
}

// set all data for a watchlist race in one go
// if race is not already on watchlist it will be added
// params: (id, fk, mk, fa, ma, watched)
static void autobutcher_setWatchListRace(color_ostream &out, unsigned id, unsigned fk, unsigned mk, unsigned fa, unsigned ma, bool watched) {
    if (watched_races.count(id)) {
        DEBUG(status,out).print("updating watchlist entry\n");
        WatchedRace * w = watched_races[id];
        w->fk = fk;
        w->mk = mk;
        w->fa = fa;
        w->ma = ma;
        w->isWatched = watched;
        w->UpdateConfig(out);
        return;
    }

    DEBUG(status,out).print("creating new watchlist entry\n");
    WatchedRace * w = new WatchedRace(out, id, watched, fk, mk, fa, ma);
    w->UpdateConfig(out);
    watched_races.emplace(id, w);

    string announce;
    announce = "New race added to autobutcher watchlist: " + Units::getRaceNamePluralById(id);
    Gui::showAnnouncement(announce, 2, false);
}

// remove entry from watchlist
static void autobutcher_removeFromWatchList(color_ostream &out, unsigned id) {
    if (watched_races.count(id)) {
        DEBUG(status,out).print("removing watchlist entry\n");
        WatchedRace * w = watched_races[id];
        w->RemoveConfig(out);
        watched_races.erase(id);
    }
}

// set default target values for new races
static void autobutcher_setDefaultTargetNew(color_ostream &out, unsigned fk, unsigned mk, unsigned fa, unsigned ma) {
    set_config_val(CONFIG_DEFAULT_FK, fk);
    set_config_val(CONFIG_DEFAULT_MK, mk);
    set_config_val(CONFIG_DEFAULT_FA, fa);
    set_config_val(CONFIG_DEFAULT_MA, ma);
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
    for (auto unit : world->units.all) {
        if(unit->race != id)
            continue;

        if(    !Units::isActive(unit)
            || Units::isUndead(unit)
            || Units::isMerchant(unit) // ignore merchants' draught animals
            || Units::isForest(unit) // ignore merchants' caged animals
            || !Units::isOwnCiv(unit)
            || !Units::isTame(unit)
            || Units::isWar(unit) // ignore war dogs etc
            || Units::isHunter(unit) // ignore hunting dogs etc
            // ignore creatures in built cages which are defined as rooms to leave zoos alone
            // (TODO: better solution would be to allow some kind of slaughter cages which you can place near the butcher)
            || (isContainedInItem(unit) && isInBuiltCageRoom(unit))  // !!! see comments in isBuiltCageRoom()
            || Units::isAvailableForAdoption(unit)
            || unit->name.has_name
            )
            continue;

        // found a bugged unit which had invalid coordinates but was not in a cage.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        doMarkForSlaughter(unit);
    }
}

// remove butcher flag for all units of a given race
static void autobutcher_unbutcherRace(color_ostream &out, int id) {
    for (auto unit : world->units.all) {
        if(unit->race != id)
            continue;

        if(    !Units::isActive(unit)
            || Units::isUndead(unit)
            || !Units::isMarkedForSlaughter(unit)
            )
            continue;

        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        unit->flags2.bits.slaughter = 0;
    }
}

// push autobutcher settings on lua stack
static int autobutcher_getSettings(lua_State *L) {
    lua_newtable(L);
    int ctable = lua_gettop(L);
    Lua::SetField(L, get_config_bool(CONFIG_IS_ENABLED), ctable, "enable_autobutcher");
    Lua::SetField(L, get_config_bool(CONFIG_AUTOWATCH), ctable, "enable_autowatch");
    Lua::SetField(L, get_config_val(CONFIG_DEFAULT_FK), ctable, "fk");
    Lua::SetField(L, get_config_val(CONFIG_DEFAULT_MK), ctable, "mk");
    Lua::SetField(L, get_config_val(CONFIG_DEFAULT_FA), ctable, "fa");
    Lua::SetField(L, get_config_val(CONFIG_DEFAULT_MA), ctable, "ma");
    Lua::SetField(L, get_config_val(CONFIG_CYCLE_TICKS), ctable, "sleep");
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
        Lua::SetField(L, tally->unit_ptr[fk_index].size(), ctable, "fk_total");
        Lua::SetField(L, tally->unit_ptr[mk_index].size(), ctable, "mk_total");
        Lua::SetField(L, tally->unit_ptr[fa_index].size(), ctable, "fa_total");
        Lua::SetField(L, tally->unit_ptr[ma_index].size(), ctable, "ma_total");
        delete tally;

        tally = checkRaceStocksProtected(*out, id);
        Lua::SetField(L, tally->unit_ptr[fk_index].size(), ctable, "fk_protected");
        Lua::SetField(L, tally->unit_ptr[mk_index].size(), ctable, "mk_protected");
        Lua::SetField(L, tally->unit_ptr[fa_index].size(), ctable, "fa_protected");
        Lua::SetField(L, tally->unit_ptr[ma_index].size(), ctable, "ma_protected");
        delete tally;

        tally = checkRaceStocksButcherable(*out, id);
        Lua::SetField(L, tally->unit_ptr[fk_index].size(), ctable, "fk_butcherable");
        Lua::SetField(L, tally->unit_ptr[mk_index].size(), ctable, "mk_butcherable");
        Lua::SetField(L, tally->unit_ptr[fa_index].size(), ctable, "fa_butcherable");
        Lua::SetField(L, tally->unit_ptr[ma_index].size(), ctable, "ma_butcherable");
        delete tally;

        tally = checkRaceStocksButcherFlag(*out, id);
        Lua::SetField(L, tally->unit_ptr[fk_index].size(), ctable, "fk_butcherflag");
        Lua::SetField(L, tally->unit_ptr[mk_index].size(), ctable, "mk_butcherflag");
        Lua::SetField(L, tally->unit_ptr[fa_index].size(), ctable, "fa_butcherflag");
        Lua::SetField(L, tally->unit_ptr[ma_index].size(), ctable, "ma_butcherflag");
        delete tally;

        lua_rawseti(L, -2, ++entry_index);
    }

    return 1;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(autowatch_isEnabled),
    DFHACK_LUA_FUNCTION(autowatch_setEnabled),
    DFHACK_LUA_FUNCTION(autobutcher_getSleep),
    DFHACK_LUA_FUNCTION(autobutcher_setSleep),
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
