#pragma once

#include "laborstatemap.h"

#include <string>
#include <vector>

#include "modules/Persistence.h"

#include <df/job_skill.h>
#include <df/unit.h>
#include <df/unit_labor.h>

using namespace DFHack;
using namespace df::enums;

// Shared infrastructure for the autolabor plugin's two engines.

namespace autolabor {

// ---------------------------------------------------------------------------
// plugin-global persisted config ("autolabor/config" site data)
//   ival(0): flags      ival(1): mode      ival(2): balance stop (0-4)
//   ival(3): idle reserve pct
// ---------------------------------------------------------------------------

extern PersistentDataItem plugin_config;
void load_plugin_config();

enum ConfigFlags {
    CF_ENABLED = 1,
    CF_ALLOW_FISHING = 2,
    CF_ALLOW_HUNTING = 4,
};

enum EngineMode {
    MODE_LEGACY = 0,  // classic autolabor: writes unit->status.labors directly
    MODE_MODERN = 1,  // labormanager: works through the work detail system
    MODE_MONITOR = 2, // starvation warnings only; no labor management
};

bool config_flag(unsigned flag);
void set_config_flag(unsigned flag, bool on);

int engine_mode();
void store_engine_mode(int mode);

int balance();              // 0..4 slider stop; 0 = max staffing
void store_balance(int v);
int idle_reserve();         // percent idle time reserved for guilded specialists
void store_idle_reserve(int v);

const int NUM_BALANCE_STOPS = 5;
// CLI/overlay label for a balance stop; NULL if out of range
const char *balance_stop_name(int i);

// ---------------------------------------------------------------------------

const int NUM_LABORS = ENUM_LAST_ITEM(unit_labor) + 1;

// labor_to_skill[l] is the job_skill trained by labor l, or job_skill::NONE
// for labors that require no skill.
extern df::job_skill labor_to_skill[];
void init_labor_to_skill();

inline bool is_unskilled(df::unit_labor l) {
    return l >= 0 && l < NUM_LABORS && labor_to_skill[l] == job_skill::NONE;
}

// does the labor require a tool (and thus churn equipment when toggled)?
bool is_exclusive_labor(df::unit_labor l);

struct BuildingScan {
    bool has_butchers = false;
    bool has_fishery = false;
    bool trader_requested = false;
};
BuildingScan scan_buildings();

struct PositionInfo {
    int noble_penalty = 0;
    bool medical = false;
    bool trader = false;
};
// noble status, medical/trade responsibilities from entity positions
PositionInfo unit_position_info(df::unit *u);

// is the unit currently involved in a diplomatic/noble meeting?
bool in_diplomacy_meeting(df::unit *u);

// ---------------------------------------------------------------------------
// labor coverage self-checks
// ---------------------------------------------------------------------------

// Appends names of unit_labor enum items that the engine's static labor
// data doesn't cover (e.g. a labor added by a new DF release that has no
// entry in a fixed-size table). Used by the autolabor_checkLaborCoverage
// lua function; an empty result means every labor is handled.
void legacy_labor_coverage(std::vector<std::string> &missing);
void modern_labor_coverage(std::vector<std::string> &missing);

// can the unit take labors at all (own civ/group, active, not visitor/ghost)?
bool is_assignable(df::unit *u);

// classify what the unit is currently doing
dwarf_state get_dwarf_state(df::unit *u);

// highest rating in a Normal or Medical skill; also reports the skill id
int top_skill(df::unit *u, df::job_skill *skill_out = nullptr);
int total_skill(df::unit *u);

}
