#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Buildings.h"
#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/building_civzonest.h"
#include "df/historical_figure.h"
#include "df/histfig_hf_link.h"
#include "df/unit.h"
#include "df/world.h"

#include <string>
#include <unordered_map>
#include <vector>

using std::pair;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("preserve-rooms");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(preserverooms, control, DebugCategory::LINFO);
    DBG_DECLARE(preserverooms, cycle, DebugCategory::LINFO);
    DBG_DECLARE(preserverooms, event, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;

// zone id, hfids (main, spouse)
typedef vector<pair<int32_t, pair<int32_t, int32_t>>> ZoneAssignments;
static ZoneAssignments last_known_assignments_bedroom;
static ZoneAssignments last_known_assignments_office;
static ZoneAssignments last_known_assignments_dining;
static ZoneAssignments last_known_assignments_tomb;
// hfid -> zone ids
static unordered_map<int32_t, vector<int32_t>> pending_reassignment;
// zone id -> hfids
static unordered_map<int32_t, vector<int32_t>> reserved_zones;

// zone id -> noble/administrative position code
static unordered_map<int32_t, string> noble_zones;

// as a "system" plugin, we do not persist plugin enabled state, just feature enabled state
enum ConfigValues {
    CONFIG_TRACK_MISSIONS = 0,
    CONFIG_TRACK_ROLES = 1,
};

static const int32_t CYCLE_TICKS = 109;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void on_new_active_unit(color_ostream& out, void* data);
static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);
    commands.push_back(PluginCommand(
        plugin_name,
        "Manage room assignments for off-map units and noble roles.",
        do_command));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    static EventManager::EventHandler new_unit_handler(plugin_self, on_new_active_unit, CYCLE_TICKS);

    if (enable != is_enabled) {
        is_enabled = enable;
        if (enable)
            EventManager::registerListener(EventManager::EventType::UNIT_NEW_ACTIVE, new_unit_handler);
        else
            EventManager::unregisterAll(plugin_self);
    }

    DEBUG(control, out).print("now %s\n", is_enabled ? "enabled" : "disabled");
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    DEBUG(control,out).print("shutting down %s\n", plugin_name);
    return CR_OK;
}

static void clear_track_missions_state() {
    last_known_assignments_bedroom.clear();
    last_known_assignments_office.clear();
    last_known_assignments_dining.clear();
    last_known_assignments_tomb.clear();
    pending_reassignment.clear();
    reserved_zones.clear();
}

static void clear_track_roles_state() {
    noble_zones.clear();
}

static PersistentDataItem get_config(const char * suffix, bool create = false) {
    string key = CONFIG_KEY + "/" + suffix;

    PersistentDataItem c = World::GetPersistentSiteData(key);
    if (!c.isValid())
        c = World::AddPersistentSiteData(key);

    return c;
}

static void store_zone_assignments(const ZoneAssignments & data, const char * suffix) {
    vector<string> elems;
    for (auto elem : data) {
        vector<int32_t> ints = {elem.first, elem.second.first, elem.second.second};
        elems.push_back(join_strings("/", ints));
    }
    get_config(suffix, true).set_str(join_strings(",", elems));
}

static void load_zone_assignments(ZoneAssignments * data, const char * suffix) {
    PersistentDataItem c = get_config(suffix);
    if (!c.isValid())
        return;

    vector<string> elems;
    split_string(&elems, c.get_str(), ",");
    for (auto elem : elems) {
        vector<string> ints;
        split_string(&ints, elem, "/");
        if (ints.size() != 3)
            continue;
        data->emplace_back(string_to_int(ints[0], -1),
            std::make_pair(string_to_int(ints[1], -1), string_to_int(ints[2], -1)));
    }
}

static void store_associations(const unordered_map<int32_t, vector<int32_t>> & data, const char * suffix) {
    vector<string> elems;
    for (auto &[key, vec] : data) {
        vector<int32_t> combined = vec;
        combined.push_back(key);
        elems.push_back(join_strings("/", combined));
    }
    get_config(suffix, true).set_str(join_strings(",", elems));
}

static void load_associations(unordered_map<int32_t, vector<int32_t>> * data, const char * suffix) {
    PersistentDataItem c = get_config(suffix);
    if (!c.isValid())
        return;

    vector<string> elems;
    split_string(&elems, c.get_str(), ",");
    for (auto elem : elems) {
        vector<string> combined;
        split_string(&combined, elem, "/");
        if (combined.size() <= 1)
            continue;
        int32_t key = string_to_int(combined.back(), -1);
        combined.pop_back();
        vector<int32_t> vals;
        for (auto str : combined)
            vals.push_back(string_to_int(str, -1));
        data->emplace(key, vals);
    }
}

static void store_int_string_map(const unordered_map<int32_t, string> & data, const char * suffix) {
    vector<string> elems;
    for (auto &[key, str] : data)
        elems.push_back(int_to_string(key) + "/" + str);
    get_config(suffix, true).set_str(join_strings(",", elems));
}

static void load_int_string_map(unordered_map<int32_t, string> * data, const char * suffix) {
    PersistentDataItem c = get_config(suffix);
    if (!c.isValid())
        return;

    vector<string> elems;
    split_string(&elems, c.get_str(), ",");
    for (auto elem : elems) {
        vector<string> key_str;
        split_string(&key_str, elem, "/");
        if (key_str.size() != 2)
            continue;
        data->emplace(string_to_int(key_str[0], -1), key_str[1]);
    }
}

DFhackCExport command_result plugin_load_site_data(color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_TRACK_MISSIONS, true);
        config.set_bool(CONFIG_TRACK_ROLES, true);
    }

    clear_track_missions_state();
    clear_track_roles_state();

    load_zone_assignments(&last_known_assignments_bedroom, "bedroom");
    load_zone_assignments(&last_known_assignments_office, "office");
    load_zone_assignments(&last_known_assignments_dining, "dining");
    load_zone_assignments(&last_known_assignments_tomb, "tomb");
    load_associations(&pending_reassignment, "pending");
    load_associations(&reserved_zones, "reserved");
    load_int_string_map(&noble_zones, "noble");

    return CR_OK;
}

DFhackCExport command_result plugin_save_site_data (color_ostream &out) {
    store_zone_assignments(last_known_assignments_bedroom, "bedroom");
    store_zone_assignments(last_known_assignments_office, "office");
    store_zone_assignments(last_known_assignments_dining, "dining");
    store_zone_assignments(last_known_assignments_tomb, "tomb");
    store_associations(pending_reassignment, "pending");
    store_associations(reserved_zones, "reserved");
    store_int_string_map(noble_zones, "noble");

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode())
        return CR_OK;
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!World::isFortressMode() || !Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.preserve-rooms", "parse_commandline", std::make_tuple(parameters),
            1, [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

static bool is_noble_zone(int32_t zone_id, const string & code) {
    auto it = noble_zones.find(zone_id);
    return it != noble_zones.end() && it->second == code;
}

static void assign_nobles(color_ostream &out) {
    for (auto &[zone_id, code] : noble_zones) {
        auto zone = virtual_cast<df::building_civzonest>(df::building::find(zone_id));
        if (!zone)
            continue;
        vector<df::unit *> units;
        Units::getUnitsByNobleRole(units, code);
        // if zone is already assigned to a proper unit, skip
        if (linear_index(units, zone->assigned_unit) >= 0)
            continue;
        // assign to a relevant noble that does not already have a registered zone of this type assigned
        bool assigned = false;
        for (auto unit : units) {
            if (!Units::isCitizen(unit, true) && !Units::isResident(unit, true))
                continue;
            bool found = false;
            for (auto owned_zone : unit->owned_buildings) {
                if (owned_zone->type == zone->type && is_noble_zone(owned_zone->id, code)) {
                    found = true;
                    break;
                }
            }
            if (found)
                continue;
            zone->spec_sub_flag.bits.active = true;
            Buildings::setOwner(zone, unit);
            assigned = true;
            INFO(cycle,out).print("preserve-rooms: assigning %s to a %s-associated %s\n",
                DF2CONSOLE(Units::getReadableName(unit)).c_str(),
                toLower_cp437(code).c_str(),
                ENUM_KEY_STR(civzone_type, zone->type).c_str());
            break;
        }
        if (!assigned && (zone->spec_sub_flag.bits.active || zone->assigned_unit)) {
            DEBUG(cycle,out).print("noble zone now reserved for eventual office holder: %d\n", zone_id);
            zone->spec_sub_flag.bits.active = false;
            Buildings::setOwner(zone, NULL);
        }
    }
}

static void clear_reservation(color_ostream &out, int32_t zone_id, df::building_civzonest * zone = NULL) {
    auto it = reserved_zones.find(zone_id);
    if (it == reserved_zones.end())
        return;
    for (auto hfid : it->second) {
        auto pending_it = pending_reassignment.find(hfid);
        if (pending_it != pending_reassignment.end()) {
            auto & zone_ids = pending_it->second;
            vector_erase_at(zone_ids, linear_index(zone_ids, zone_id));
            if (zone_ids.empty())
                pending_reassignment.erase(pending_it);
        }
    }
    reserved_zones.erase(zone_id);
    if (!zone)
        zone = virtual_cast<df::building_civzonest>(df::building::find(zone_id));
    if (zone)
        zone->spec_sub_flag.bits.active = true;
}

static int32_t get_spouse_hfid(color_ostream &out, df::historical_figure *hf) {
    for (auto link : hf->histfig_links) {
        if (link->getType() == df::histfig_hf_link_type::SPOUSE)
            return link->target_hf;
    }
    return -1;
}

// handles when units disappear from their assignments compared to the last scan
static void handle_missing_assignments(color_ostream &out,
    const unordered_set<int32_t> & active_unit_ids,
    ZoneAssignments::iterator *pit,
    const ZoneAssignments::iterator & it_end,
    bool share_with_spouse,
    int32_t next_zone_id)
{
    for (auto & it = *pit; it != it_end && (next_zone_id == -1 || it->first <= next_zone_id); ++it) {
        int32_t zone_id = it->first;
        int32_t hfid = it->second.first;
        int32_t spouse_hfid = it->second.second;
        auto hf = df::historical_figure::find(hfid);
        auto unit = df::unit::find(hf->unit_id);
        if (!unit) {
            // if unit data is completely gone, then they're not likely to come back
            continue;
        }
        if (Units::isActive(unit) && !Units::isDead(unit) && active_unit_ids.contains(unit->id)) {
            // unit is still alive on the map; assume the unassigment was intentional/expected
            continue;
        }
        if (!Units::isCitizen(unit, true) && !Units::isResident(unit, true))
            continue;
        auto zone = virtual_cast<df::building_civzonest>(df::building::find(zone_id));
        if (!zone)
            continue;
        // unit is off-map or is dead; if we can assign room to spouse then we don't need to reserve the room
        auto spouse_hf = df::historical_figure::find(spouse_hfid);
        if (spouse_hf && share_with_spouse) {
            if (auto spouse = df::unit::find(spouse_hf->unit_id);
                spouse && Units::isActive(spouse) && !Units::isDead(spouse) && active_unit_ids.contains(spouse->id))
            {
                DEBUG(cycle,out).print("assigning zone %d (%s) to spouse %s\n",
                    zone_id, ENUM_KEY_STR(civzone_type, zone->type).c_str(),
                    DF2CONSOLE(Units::getReadableName(spouse)).c_str());
                Buildings::setOwner(zone, spouse);
                continue;
            }
        }
        if (Units::isDead(unit))
            continue;
        // register the hf ids for reassignment and reserve the room
        DEBUG(cycle,out).print("registering primary unit for reassignment to zone %d (%s): %d %s\n",
            zone_id, ENUM_KEY_STR(civzone_type, zone->type).c_str(), unit->id,
            DF2CONSOLE(Units::getReadableName(unit)).c_str());
        pending_reassignment[hfid].push_back(zone_id);
        reserved_zones[zone_id].push_back(hfid);
        if (share_with_spouse && spouse_hfid > -1) {
            DEBUG(cycle,out).print("registering spouse unit for reassignment to zone %d (%s): hfid=%d\n",
                zone_id, ENUM_KEY_STR(civzone_type, zone->type).c_str(), spouse_hfid);
            pending_reassignment[spouse_hfid].push_back(zone_id);
            reserved_zones[zone_id].push_back(spouse_hfid);
        }
        INFO(cycle,out).print("preserve-rooms: reserving %s for the return of %s%s%s\n",
                    toLower_cp437(ENUM_KEY_STR(civzone_type, zone->type)).c_str(),
                    DF2CONSOLE(Units::getReadableName(unit)).c_str(),
                    spouse_hf ? " or their spouse, " : "",
                    spouse_hf ? DF2CONSOLE(Units::getReadableName(spouse_hf)).c_str() : "");

        zone->spec_sub_flag.bits.active = false;
    }
}

static void process_rooms(color_ostream &out,
    const unordered_set<int32_t> & active_unit_ids,
    ZoneAssignments & last_known,
    const vector<df::building_civzonest *> & vec,
    bool share_with_spouse=true)
{
    ZoneAssignments assignments;
    auto it = last_known.begin();
    auto it_end = last_known.end();
    for (auto zone : vec) {
        if (!zone->assigned_unit) {
            handle_missing_assignments(out, active_unit_ids, &it, it_end, share_with_spouse, zone->id);
            continue;
        }
        auto hf = df::historical_figure::find(zone->assigned_unit->hist_figure_id);
        if (!hf)
            continue;
        int32_t spouse_hfid = share_with_spouse ? get_spouse_hfid(out, hf) : -1;
        assignments.emplace_back(zone->id, std::make_pair(hf->id, spouse_hfid));
    }
    handle_missing_assignments(out, active_unit_ids, &it, it_end, share_with_spouse, -1);

    last_known = assignments;
}

static void do_cycle(color_ostream &out) {
    cycle_timestamp = world->frame_counter;

    TRACE(cycle,out).print("running %s cycle\n", plugin_name);

    if (config.get_bool(CONFIG_TRACK_ROLES)) {
        assign_nobles(out);
    }

    if (config.get_bool(CONFIG_TRACK_MISSIONS)) {
        unordered_set<int32_t> active_unit_ids;
        std::transform(world->units.active.begin(), world->units.active.end(),
            std::inserter(active_unit_ids, active_unit_ids.end()), [](auto & unit){ return unit->id; });

        process_rooms(out, active_unit_ids, last_known_assignments_bedroom, world->buildings.other.ZONE_BEDROOM);
        process_rooms(out, active_unit_ids, last_known_assignments_office, world->buildings.other.ZONE_OFFICE);
        process_rooms(out, active_unit_ids, last_known_assignments_dining, world->buildings.other.ZONE_DINING_HALL);
        process_rooms(out, active_unit_ids, last_known_assignments_tomb, world->buildings.other.ZONE_TOMB, false);

        DEBUG(cycle,out).print("tracking zone assignments: bedrooms: %zd, offices: %zd, dining halls: %zd, tombs: %zd\n",
            last_known_assignments_bedroom.size(), last_known_assignments_office.size(),
            last_known_assignments_dining.size(), last_known_assignments_tomb.size());
    }
}

/////////////////////////////////////////////////////
// Event logic
//

static bool spouse_has_sharable_room(color_ostream& out, int32_t hfid, df::civzone_type ztype) {
    if (ztype == df::civzone_type::Tomb)
        return false;
    auto hf = df::historical_figure::find(hfid);
    if (!hf)
        return false;
    auto spouse_hf = df::historical_figure::find(get_spouse_hfid(out, hf));
    if (!spouse_hf)
        return false;
    auto spouse = df::unit::find(spouse_hf->unit_id);
    if (!spouse)
        return false;
    for (auto owned_zone : spouse->owned_buildings) {
        if (owned_zone->type == ztype) {
            DEBUG(event,out).print("spouse had sharable room; no need to set ownership\n");
            return true;
        }
    }
    return false;
}

static void on_new_active_unit(color_ostream& out, void* data) {
    int32_t unit_id = reinterpret_cast<std::intptr_t>(data);
    auto unit = df::unit::find(unit_id);
    if (!unit || unit->hist_figure_id < 0)
        return;
    TRACE(event,out).print("unit %d (%s) arrived on map (hfid: %d, in pending: %d)\n",
        unit->id, DF2CONSOLE(Units::getReadableName(unit)).c_str(), unit->hist_figure_id,
        pending_reassignment.contains(unit->hist_figure_id));
    auto hfid = unit->hist_figure_id;
    auto it = pending_reassignment.find(hfid);
    if (it == pending_reassignment.end())
        return;
    INFO(event,out).print("preserve-rooms: restoring room ownership for %s\n",
        DF2CONSOLE(Units::getReadableName(unit)).c_str());
    for (auto zone_id : it->second) {
        reserved_zones.erase(zone_id);
        auto zone = virtual_cast<df::building_civzonest>(df::building::find(zone_id));
        if (!zone)
            continue;
        zone->spec_sub_flag.bits.active = true;
        if (zone->assigned_unit || spouse_has_sharable_room(out, hfid, zone->type))
            continue;
        DEBUG(event,out).print("reassigning zone %d\n", zone->id);
        Buildings::setOwner(zone, unit);
    }
    pending_reassignment.erase(it);
}

/////////////////////////////////////////////////////
// Lua API
//

static void preserve_rooms_cycle(color_ostream &out) {
    DEBUG(control,out).print("preserve_rooms_cycle\n");
    do_cycle(out);
}

static bool preserve_rooms_setFeature(color_ostream &out, bool enabled, string feature) {
    DEBUG(control,out).print("preserve_rooms_setFeature: enabled=%d, feature=%s\n",
        enabled, feature.c_str());
    if (feature == "track-missions") {
        config.set_bool(CONFIG_TRACK_MISSIONS, enabled);
        if (is_enabled && enabled)
            do_cycle(out);
    } else if (feature == "track-roles") {
        config.set_bool(CONFIG_TRACK_ROLES, enabled);
    } else {
        return false;
    }

    return true;
}

static bool preserve_rooms_getFeature(color_ostream &out, string feature) {
    TRACE(control,out).print("preserve_rooms_getFeature: feature=%s\n", feature.c_str());
    if (feature == "track-missions")
        return config.get_bool(CONFIG_TRACK_MISSIONS);
    if (feature == "track-roles")
        return config.get_bool(CONFIG_TRACK_ROLES);
    return false;
}

static bool preserve_rooms_resetFeatureState(color_ostream &out, string feature) {
    DEBUG(control,out).print("preserve_rooms_resetFeatureState: feature=%s\n", feature.c_str());
    if (feature == "track-missions") {
        vector<int32_t> zone_ids;
        std::transform(reserved_zones.begin(), reserved_zones.end(), std::back_inserter(zone_ids), [](auto & elem){ return elem.first; });
        for (auto zone_id : zone_ids)
            clear_reservation(out, zone_id);
        clear_track_missions_state();
    } else if (feature == "track-roles") {
        clear_track_roles_state();
    } else {
        return false;
    }

    return true;
}

static void preserve_rooms_assignToRole(color_ostream &out, string code) {
    auto zone = Gui::getSelectedCivZone(out, true);
    if (!zone)
        return;
    DEBUG(control,out).print("preserve_rooms_assignToRole: zone_id=%d, code=%s\n", zone->id, code.c_str());
    if (code == "") {
        // if we're removing an assignment, activate the zone
        if (auto it = noble_zones.find(zone->id); it != noble_zones.end()) {
            zone->spec_sub_flag.bits.active = true;
            noble_zones.erase(it);
        }
        return;
    }
    noble_zones[zone->id] = code;
    do_cycle(out);
}

static string preserve_rooms_getRoleAssignment(color_ostream &out) {
    auto zone = Gui::getSelectedCivZone(out, true);
    if (!zone)
        return "";
    TRACE(control,out).print("preserve_rooms_getRoleAssignment: zone_id=%d\n", zone->id);
    auto it = noble_zones.find(zone->id);
    if (it == noble_zones.end())
        return "";
    return it->second;
}

static bool preserve_rooms_isReserved(color_ostream &out) {
    auto zone = Gui::getSelectedCivZone(out, true);
    if (!zone)
        return false;
    TRACE(control,out).print("preserve_rooms_isReserved: zone_id=%d\n", zone->id);
    auto it = reserved_zones.find(zone->id);
    return it != reserved_zones.end() && it->second.size() > 0;
}

static string preserve_rooms_getReservationName(color_ostream &out) {
    auto zone = Gui::getSelectedCivZone(out, true);
    if (!zone)
        return "";
    TRACE(control,out).print("preserve_rooms_getReservationName: zone_id=%d\n", zone->id);
    auto it = reserved_zones.find(zone->id);
    if (it != reserved_zones.end() && it->second.size() > 0) {
        if (auto hf = df::historical_figure::find(it->second.front())) {
            return Units::getReadableName(hf);
        }
    }
    return "";
}

static bool preserve_rooms_clearReservation(color_ostream &out) {
    auto zone = Gui::getSelectedCivZone(out, true);
    if (!zone)
        return false;
    DEBUG(control,out).print("preserve_rooms_clearReservation: zone_id=%d\n", zone->id);
    clear_reservation(out, zone->id, zone);
    return true;
}

static int preserve_rooms_getState(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("preserve_rooms_getState\n");

    unordered_map<string, bool> features;
    features.emplace("track-missions", config.get_bool(CONFIG_TRACK_MISSIONS));
    features.emplace("track-roles", config.get_bool(CONFIG_TRACK_ROLES));
    Lua::Push(L, features);

    unordered_map<string, size_t> stats;
    stats.emplace("travelers", pending_reassignment.size());
    stats.emplace("reservations", reserved_zones.size());
    Lua::Push(L, stats);

    return 2;
}

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(preserve_rooms_cycle),
    DFHACK_LUA_FUNCTION(preserve_rooms_setFeature),
    DFHACK_LUA_FUNCTION(preserve_rooms_getFeature),
    DFHACK_LUA_FUNCTION(preserve_rooms_resetFeatureState),
    DFHACK_LUA_FUNCTION(preserve_rooms_assignToRole),
    DFHACK_LUA_FUNCTION(preserve_rooms_getRoleAssignment),
    DFHACK_LUA_FUNCTION(preserve_rooms_isReserved),
    DFHACK_LUA_FUNCTION(preserve_rooms_getReservationName),
    DFHACK_LUA_FUNCTION(preserve_rooms_clearReservation),
    DFHACK_LUA_END};

DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(preserve_rooms_getState),
    DFHACK_LUA_END};
