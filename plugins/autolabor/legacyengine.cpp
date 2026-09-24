/*
 * Legacy engine for autolabor: direct labor matrix manipulation.
 *
 * This preserves the pre-v50 behavior of the autolabor plugin. It bypasses
 * the work detail system by setting automatic_professions_disabled and
 * writes unit->status.labors directly each cycle.
 */

#include "laborcommon.h"
#include "legacyengine.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>

#include "Debug.h"
#include "PluginManager.h"

#include "modules/Units.h"
#include "modules/World.h"

#include <df/activity_info.h>
#include <df/building.h>
#include <df/building_stockpilest.h>
#include <df/building_tradedepotst.h>
#include <df/entity_position.h>
#include <df/entity_position_assignment.h>
#include <df/entity_position_responsibility.h>
#include <df/gamest.h>
#include <df/global_objects.h>
#include <df/histfig_entity_link.h>
#include <df/histfig_entity_link_positionst.h>
#include <df/historical_entity.h>
#include <df/historical_figure.h>
#include <df/items_other_id.h>
#include <df/job.h>
#include <df/plotinfost.h>
#include <df/unit.h>
#include <df/unit_labor.h>
#include <df/unit_misc_trait.h>
#include <df/unit_skill.h>
#include <df/unit_soul.h>
#include <df/workshop_type.h>
#include <df/world.h>

using namespace DFHack;
using namespace df::enums;

using df::global::plotinfo;
using df::global::world;
using df::global::game;

#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))

namespace DFHack {
    DBG_DECLARE(autolabor, legacy_cycle, DebugCategory::LINFO);
}

using autolabor::plugin_config;
using autolabor::config_flag;
using autolabor::labor_to_skill;

static std::vector<int> state_count(NUM_STATE);

enum labor_mode {
    DISABLE,
    HAULERS,
    AUTOMATIC,
};

struct labor_info
{
    PersistentDataItem config;

    bool is_exclusive;
    int active_dwarfs;

    labor_mode mode() { return (labor_mode) config.ival(0); }
    void set_mode(labor_mode mode) { config.ival(0) = mode; }

    int minimum_dwarfs() { return config.ival(1); }
    void set_minimum_dwarfs(int minimum_dwarfs) { config.ival(1) = minimum_dwarfs; }

    int maximum_dwarfs() { return config.ival(2); }
    void set_maximum_dwarfs(int maximum_dwarfs) { config.ival(2) = maximum_dwarfs; }

    int talent_pool() { return config.ival(3); }
    void set_talent_pool(int talent_pool) { config.ival(3) = talent_pool; }
};

struct labor_default
{
    labor_mode mode;
    bool is_exclusive;
    int minimum_dwarfs;
    int maximum_dwarfs;
    int active_dwarfs;
};

// The percentage of the dwarves assigned as haulers at any one time.
static int hauler_pct = 33;

// The maximum percentage of dwarves who will be allowed to be idle.
// Decreasing this will encourage autolabor to keep dwarves busy,
// at the expense of making it harder for dwarves to specialize in
// specific skills.
static int idler_pct = 10;

static std::vector<struct labor_info> labor_infos;

static const struct labor_default default_labor_infos[] = {
    /* MINE */                  {AUTOMATIC, true, 2, 200, 0},
    /* HAUL_STONE */            {HAULERS, false, 1, 200, 0},
    /* HAUL_WOOD */             {HAULERS, false, 1, 200, 0},
    /* HAUL_BODY */             {HAULERS, false, 1, 200, 0},
    /* HAUL_FOOD */             {HAULERS, false, 1, 200, 0},
    /* HAUL_REFUSE */           {HAULERS, false, 1, 200, 0},
    /* HAUL_ITEM */             {HAULERS, false, 1, 200, 0},
    /* HAUL_FURNITURE */        {HAULERS, false, 1, 200, 0},
    /* HAUL_ANIMAL */           {HAULERS, false, 1, 200, 0},
    /* CLEAN */                 {HAULERS, false, 1, 200, 0},
    /* CUTWOOD */               {AUTOMATIC, true, 1, 200, 0},
    /* CARPENTER */             {AUTOMATIC, false, 1, 200, 0},
    /* STONECUTTER */           {AUTOMATIC, false, 1, 200, 0},
    /* STONE_CARVER */          {AUTOMATIC, false, 1, 200, 0},
    /* MASON */                 {AUTOMATIC, false, 1, 200, 0},
    /* ANIMALTRAIN */           {AUTOMATIC, false, 1, 200, 0},
    /* ANIMALCARE */            {AUTOMATIC, false, 1, 200, 0},
    /* DIAGNOSE */              {AUTOMATIC, false, 1, 200, 0},
    /* SURGERY */               {AUTOMATIC, false, 1, 200, 0},
    /* BONE_SETTING */          {AUTOMATIC, false, 1, 200, 0},
    /* SUTURING */              {AUTOMATIC, false, 1, 200, 0},
    /* DRESSING_WOUNDS */       {AUTOMATIC, false, 1, 200, 0},
    /* FEED_WATER_CIVILIANS */  {AUTOMATIC, false, 200, 200, 0},
    /* RECOVER_WOUNDED */       {HAULERS, false, 1, 200, 0},
    /* BUTCHER */               {AUTOMATIC, false, 1, 200, 0},
    /* TRAPPER */               {AUTOMATIC, false, 1, 200, 0},
    /* DISSECT_VERMIN */        {AUTOMATIC, false, 1, 200, 0},
    /* LEATHER */               {AUTOMATIC, false, 1, 200, 0},
    /* TANNER */                {AUTOMATIC, false, 1, 200, 0},
    /* BREWER */                {AUTOMATIC, false, 1, 200, 0},
    /* ALCHEMIST */             {AUTOMATIC, false, 1, 200, 0},
    /* SOAP_MAKER */            {AUTOMATIC, false, 1, 200, 0},
    /* WEAVER */                {AUTOMATIC, false, 1, 200, 0},
    /* CLOTHESMAKER */          {AUTOMATIC, false, 1, 200, 0},
    /* MILLER */                {AUTOMATIC, false, 1, 200, 0},
    /* PROCESS_PLANT */         {AUTOMATIC, false, 1, 200, 0},
    /* MAKE_CHEESE */           {AUTOMATIC, false, 1, 200, 0},
    /* MILK */                  {AUTOMATIC, false, 1, 200, 0},
    /* COOK */                  {AUTOMATIC, false, 1, 200, 0},
    /* PLANT */                 {AUTOMATIC, false, 1, 200, 0},
    /* HERBALIST */             {AUTOMATIC, false, 1, 200, 0},
    /* FISH */                  {AUTOMATIC, false, 1, 1, 0},
    /* CLEAN_FISH */            {AUTOMATIC, false, 1, 200, 0},
    /* DISSECT_FISH */          {AUTOMATIC, false, 1, 200, 0},
    /* HUNT */                  {AUTOMATIC, true, 1, 1, 0},
    /* SMELT */                 {AUTOMATIC, false, 1, 200, 0},
    /* FORGE_WEAPON */          {AUTOMATIC, false, 1, 200, 0},
    /* FORGE_ARMOR */           {AUTOMATIC, false, 1, 200, 0},
    /* FORGE_FURNITURE */       {AUTOMATIC, false, 1, 200, 0},
    /* METAL_CRAFT */           {AUTOMATIC, false, 1, 200, 0},
    /* CUT_GEM */               {AUTOMATIC, false, 1, 200, 0},
    /* ENCRUST_GEM */           {AUTOMATIC, false, 1, 200, 0},
    /* WOOD_CRAFT */            {AUTOMATIC, false, 1, 200, 0},
    /* STONE_CRAFT */           {AUTOMATIC, false, 1, 200, 0},
    /* BONE_CARVE */            {AUTOMATIC, false, 1, 200, 0},
    /* GLASSMAKER */            {AUTOMATIC, false, 1, 200, 0},
    /* EXTRACT_STRAND */        {AUTOMATIC, false, 1, 200, 0},
    /* SIEGECRAFT */            {AUTOMATIC, false, 1, 200, 0},
    /* SIEGEOPERATE */          {AUTOMATIC, false, 1, 200, 0},
    /* BOWYER */                {AUTOMATIC, false, 1, 200, 0},
    /* MECHANIC */              {AUTOMATIC, false, 1, 200, 0},
    /* POTASH_MAKING */         {AUTOMATIC, false, 1, 200, 0},
    /* LYE_MAKING */            {AUTOMATIC, false, 1, 200, 0},
    /* DYER */                  {AUTOMATIC, false, 1, 200, 0},
    /* BURN_WOOD */             {AUTOMATIC, false, 1, 200, 0},
    /* OPERATE_PUMP */          {AUTOMATIC, false, 1, 200, 0},
    /* SHEARER */               {AUTOMATIC, false, 1, 200, 0},
    /* SPINNER */               {AUTOMATIC, false, 1, 200, 0},
    /* POTTERY */               {AUTOMATIC, false, 1, 200, 0},
    /* GLAZING */               {AUTOMATIC, false, 1, 200, 0},
    /* PRESSING */              {AUTOMATIC, false, 1, 200, 0},
    /* BEEKEEPING */            {AUTOMATIC, false, 1, 200, 0},
    /* WAX_WORKING */           {AUTOMATIC, false, 1, 200, 0},
    /* HANDLE_VEHICLES */       {HAULERS, false, 1, 200, 0},
    /* HAUL_TRADE */            {HAULERS, false, 1, 200, 0},
    /* PULL_LEVER */            {HAULERS, false, 1, 200, 0},
    /* UNUSED_13 */             {DISABLE, false, 1, 0, 0},
    /* HAUL_WATER */            {HAULERS, false, 1, 200, 0},
    /* GELD */                  {AUTOMATIC, false, 1, 200, 0},
    /* BUILD_ROAD */            {AUTOMATIC, false, 1, 200, 0},
    /* BUILD_CONSTRUCTION */    {AUTOMATIC, false, 1, 200, 0},
    /* PAPERMAKING */           {AUTOMATIC, false, 1, 200, 0},
    /* BOOKBINDING */           {AUTOMATIC, false, 1, 200, 0},
    /* UNUSED_20 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_21 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_22 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_23 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_24 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_25 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_26 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_27 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_28 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_29 */             {DISABLE, false, 0, 0, 0},
    /* UNUSED_30 */             {DISABLE, false, 0, 0, 0},
};

// the table is indexed by labor enum value: one entry per item from 0 to
// the last labor (NONE is -1 and has no entry). a DF release that adds or
// removes labor types breaks the build here until the table is updated.
static_assert(ARRAY_COUNT(default_labor_infos) ==
    (int)ENUM_LAST_ITEM(unit_labor) + 1,
    "default_labor_infos must have one entry per unit_labor enum item");

struct dwarf_info_t
{
    int highest_skill;
    int total_skill;
    int mastery_penalty;
    int assigned_jobs;
    dwarf_state state;
    bool has_exclusive_labor;
    int noble_penalty; // penalty for assignment due to noble status
    bool medical; // this dwarf has medical responsibility
    bool trader;  // this dwarf has trade responsibility
    bool diplomacy; // this dwarf meets with diplomats
};

static const int32_t CYCLE_TICKS = 61;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static void reset_labor(df::unit_labor labor)
{
    labor_infos[labor].set_minimum_dwarfs(default_labor_infos[labor].minimum_dwarfs);
    labor_infos[labor].set_maximum_dwarfs(default_labor_infos[labor].maximum_dwarfs);
    labor_infos[labor].set_talent_pool(200);
    labor_infos[labor].set_mode(default_labor_infos[labor].mode);
}

namespace autolabor {

void legacy_labor_coverage(std::vector<std::string> &missing)
{
    int enum_count = 0;
    FOR_ENUM_ITEMS(unit_labor, l)
    {
        if (l == unit_labor::NONE)
            continue;
        enum_count++;
        // default_labor_infos is indexed by labor enum value; a labor added
        // by a new DF release has no default and would be invisible here
        if ((size_t)l >= ARRAY_COUNT(default_labor_infos))
            missing.push_back(ENUM_KEY_STR(unit_labor, l) +
                ": no entry in legacy labor defaults");
    }
    // a labor removed from the enum leaves stale entries behind
    if (enum_count != (int)ARRAY_COUNT(default_labor_infos))
        missing.push_back("labor defaults table has " +
            std::to_string(ARRAY_COUNT(default_labor_infos)) +
            " entries for " + std::to_string(enum_count) +
            " labor types");
}

} // namespace autolabor

void LegacyEngine::enable(color_ostream &out)
{
    // purge any state left behind by the modern engine
    wdm->reset();
    wdm->shutdown();

    auto cfg_haulpct = World::GetPersistentSiteData("autolabor/haulpct");
    hauler_pct = cfg_haulpct.isValid() ? cfg_haulpct.ival(0) : 33;

    // Load labors from save
    labor_infos.resize(ARRAY_COUNT(default_labor_infos));

    std::vector<PersistentDataItem> items;
    World::GetPersistentSiteData(&items, "autolabor/labors/", true);

    for (auto& p : items)
    {
        std::string key = p.key();
        df::unit_labor labor = (df::unit_labor) atoi(key.substr(strlen("autolabor/labors/")).c_str());
        if (labor >= 0 && size_t(labor) < labor_infos.size())
        {
            labor_infos[labor].config = p;
            labor_infos[labor].is_exclusive = default_labor_infos[labor].is_exclusive;
            labor_infos[labor].active_dwarfs = 0;
        }
    }

    // Add default labors for those not in save
    for (size_t i = 0; i < ARRAY_COUNT(default_labor_infos); i++) {
        if (labor_infos[i].config.isValid())
            continue;

        std::stringstream name;
        name << "autolabor/labors/" << i;

        labor_infos[i].config = World::AddPersistentSiteData(name.str());

        labor_infos[i].is_exclusive = default_labor_infos[i].is_exclusive;
        labor_infos[i].active_dwarfs = 0;
        reset_labor((df::unit_labor) i);
    }

    // bypass DF's work detail system
    game->external_flag.bits.automatic_professions_disabled = true;

    cycle_timestamp = 0;
    initialized = true;
    out << "Enabling autolabor (legacy mode)." << std::endl;
}

void LegacyEngine::disable(color_ostream &out)
{
    initialized = false;
    labor_infos.clear();

    // reinstate DF's work detail system and force a recompute so the
    // labor matrix is rebuilt from the player's work details immediately
    game->external_flag.bits.automatic_professions_disabled = false;
    for (auto u : world->units.active)
        if (Units::isCitizen(u))
            Units::setAutomaticProfessions(u);

    out << "Disabling autolabor." << std::endl;
}

void LegacyEngine::map_unload()
{
    initialized = false;
    labor_infos.clear();
    if (game)
        game->external_flag.bits.automatic_professions_disabled = false;
}

// sorting objects
struct dwarfinfo_sorter
{
    dwarfinfo_sorter(std::vector <dwarf_info_t> & info):dwarf_info(info){};
    bool operator() (int i,int j)
    {
        if (dwarf_info[i].state == IDLE && dwarf_info[j].state != IDLE)
            return true;
        if (dwarf_info[i].state != IDLE && dwarf_info[j].state == IDLE)
            return false;
        return dwarf_info[i].mastery_penalty > dwarf_info[j].mastery_penalty;
    };
    std::vector <dwarf_info_t> & dwarf_info;
};
struct laborinfo_sorter
{
    bool operator() (int i,int j)
    {
        if (labor_infos[i].mode() != labor_infos[j].mode())
            return labor_infos[i].mode() < labor_infos[j].mode();
        if (labor_infos[i].is_exclusive != labor_infos[j].is_exclusive)
            return labor_infos[i].is_exclusive;
        if (labor_infos[i].maximum_dwarfs() != labor_infos[j].maximum_dwarfs())
            return labor_infos[i].maximum_dwarfs() < labor_infos[j].maximum_dwarfs();
        return false;
    };
};

struct values_sorter
{
    values_sorter(std::vector <int> & values):values(values){};
    bool operator() (int i,int j)
    {
        return values[i] > values[j];
    };
    std::vector<int> & values;
};


static void assign_labor(unit_labor::unit_labor labor,
    int n_dwarfs,
    std::vector<dwarf_info_t>& dwarf_info,
    bool trader_requested,
    std::vector<df::unit *>& dwarfs,
    bool has_butchers,
    bool has_fishery,
    color_ostream& out)
{
    df::job_skill skill = labor_to_skill[labor];

        if (labor_infos[labor].mode() != AUTOMATIC)
            return;

        int best_skill = 0;

        std::vector<int> values(n_dwarfs);
        std::vector<int> candidates;
        std::map<int, int> dwarf_skill;
        std::map<int, int> dwarf_skillxp;
        std::vector<bool> previously_enabled(n_dwarfs);

        // Find candidate dwarfs, and calculate a preference value for each dwarf
        for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
        {
            if (dwarf_info[dwarf].state == CHILD)
                continue;
            if (dwarf_info[dwarf].state == MILITARY)
                continue;
            if (dwarf_info[dwarf].trader && trader_requested)
                continue;
            if (dwarf_info[dwarf].diplomacy)
                continue;

            if (labor_infos[labor].is_exclusive && dwarf_info[dwarf].has_exclusive_labor)
                continue;

            int value = dwarf_info[dwarf].mastery_penalty;

            if (skill != job_skill::NONE)
            {
                int skill_level = 0;
                int skill_experience = 0;

                for (auto s : dwarfs[dwarf]->status.souls[0]->skills)
                {
                    if (s->id == skill)
                    {
                        skill_level = s->rating;
                        skill_experience = s->experience;
                        break;
                    }
                }

                dwarf_skill[dwarf] = skill_level;
                dwarf_skillxp[dwarf] = skill_experience;

                if (best_skill < skill_level)
                    best_skill = skill_level;

                value += skill_level * 100;
                value += skill_experience / 20;
                if (skill_level > 0 || skill_experience > 0)
                    value += 200;
                if (skill_level >= 15)
                    value += 1000 * (skill_level - 14);
            }
            else
            {
                dwarf_skill[dwarf] = 0;
            }

            if (dwarfs[dwarf]->status.labors[labor])
            {
                value += 5;
                if (labor_infos[labor].is_exclusive)
                    value += 350;
            }

            if (dwarf_info[dwarf].has_exclusive_labor)
                value -= 500;

            // bias by happiness

            //value += dwarfs[dwarf]->status.happiness;

            values[dwarf] = value;

            candidates.push_back(dwarf);

        }

        int pool = labor_infos[labor].talent_pool();
        if (pool < 200 && candidates.size() > 1 && size_t(abs(pool)) < candidates.size())
        {
            // Sort in descending order
            std::sort(candidates.begin(), candidates.end(), [&](const int lhs, const int rhs) -> bool {
                if (dwarf_skill[lhs] == dwarf_skill[rhs])
                    if (pool > 0)
                        return dwarf_skillxp[lhs] > dwarf_skillxp[rhs];
                    else
                        return dwarf_skillxp[lhs] < dwarf_skillxp[rhs];
                else
                    if (pool > 0)
                        return dwarf_skill[lhs] > dwarf_skill[rhs];
                    else
                        return dwarf_skill[lhs] < dwarf_skill[rhs];
            });

            // Check if all dwarves have equivalent skills, usually zero
            int first_dwarf = candidates[0];
            int last_dwarf = candidates[candidates.size() - 1];
            if (dwarf_skill[first_dwarf] == dwarf_skill[last_dwarf] &&
                dwarf_skillxp[first_dwarf] == dwarf_skillxp[last_dwarf])
            {
                // There's no difference in skill, so change nothing
            }
            else
            {
                // Trim down to our top (or not) talents
                candidates.resize(abs(pool));
            }
        }

        // Sort candidates by preference value
        values_sorter ivs(values);
        std::sort(candidates.begin(), candidates.end(), ivs);

        // Disable the labor on everyone
        for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
        {
            if (dwarf_info[dwarf].state == CHILD)
                continue;

            previously_enabled[dwarf] = dwarfs[dwarf]->status.labors[labor];
            dwarfs[dwarf]->status.labors[labor] = false;
        }

        int min_dwarfs = labor_infos[labor].minimum_dwarfs();
        int max_dwarfs = labor_infos[labor].maximum_dwarfs();

        // Special - don't assign hunt without a butchers, or fish without a fishery
        if (unit_labor::HUNT == labor && !has_butchers)
            min_dwarfs = max_dwarfs = 0;
        if (unit_labor::FISH == labor && !has_fishery)
            min_dwarfs = max_dwarfs = 0;

        // If there are enough idle dwarves to choose from, enter an aggressive assignment
        // mode. "Enough" idle dwarves is defined as 2 or 10% of the total number of dwarves,
        // whichever is higher.
        //
        // In aggressive mode, we will always pick at least one idle dwarf for each skill,
        // in order to try to get the idle dwarves to start doing something. We also pick
        // any dwarf more preferable to the idle dwarf, since we'd rather have a more
        // preferable dwarf do a new job if one becomes available (probably because that
        // dwarf just finished a job).
        //
        // In non-aggressive mode, only dwarves that are good at a labor will be assigned
        // to it. Dwarves good at nothing, or nothing that needs doing, will tend to get
        // assigned to hauling by the hauler code. If there are no hauling jobs to do,
        // they will sit around idle and when enough build up they will trigger aggressive
        // mode again.
        bool aggressive_mode = state_count[IDLE] >= 2 && state_count[IDLE] >= n_dwarfs * idler_pct / 100;

        /*
         * Assign dwarfs to this labor. We assign at least the minimum number of dwarfs, in
         * order of preference, and then assign additional dwarfs that meet any of these conditions:
         * - We are in aggressive mode and have not yet assigned an idle dwarf
         * - The dwarf is good at this skill
         * - The labor is mining, hunting, or woodcutting and the dwarf currently has it enabled.
         * We stop assigning dwarfs when we reach the maximum allowed.
         * Note that only idle and busy dwarfs count towards the number of dwarfs. "Other" dwarfs
         * (sleeping, eating, on break, etc.) will have labors assigned, but will not be counted.
         * Military and children/nobles will not have labors assigned.
         * Dwarfs with the "health management" responsibility are always assigned DIAGNOSIS.
         */
        for (size_t i = 0; i < candidates.size() && labor_infos[labor].active_dwarfs < max_dwarfs; i++)
        {
            int dwarf = candidates[i];

            if (dwarf_info[dwarf].trader && trader_requested)
                continue;
            if (dwarf_info[dwarf].diplomacy)
                continue;

            bool preferred_dwarf = false;
            if (dwarf_skillxp[dwarf] > 0 && dwarf_skill[dwarf] >= best_skill / 2)
                preferred_dwarf = true;
            if (previously_enabled[dwarf] && labor_infos[labor].is_exclusive && dwarf_info[dwarf].state == EXCLUSIVE)
                preferred_dwarf = true;
            if (dwarf_info[dwarf].medical && labor == df::unit_labor::DIAGNOSE)
                preferred_dwarf = true;

            if (labor_infos[labor].active_dwarfs >= min_dwarfs && !preferred_dwarf && !aggressive_mode)
                continue;

            if (!dwarfs[dwarf]->status.labors[labor])
                dwarf_info[dwarf].assigned_jobs++;

            if (Units::isValidLabor(dwarfs[dwarf], labor))
                dwarfs[dwarf]->status.labors[labor] = true;

            if (labor_infos[labor].is_exclusive)
            {
                dwarf_info[dwarf].has_exclusive_labor = true;
                // all the exclusive labors require equipment so this should force the dorf to reequip if needed
                dwarfs[dwarf]->uniform.pickup_flags.bits.update = 1;
            }

            TRACE(legacy_cycle, out).print("Dwarf {} \"{}\" assigned {}: value {} {} {}\n",
                dwarf, dwarfs[dwarf]->name.first_name, ENUM_KEY_STR(unit_labor, labor), values[dwarf],
                dwarf_info[dwarf].trader ? "(trader)" : "",
                dwarf_info[dwarf].diplomacy ? "(diplomacy)" : "");

            if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY || dwarf_info[dwarf].state == EXCLUSIVE)
                labor_infos[labor].active_dwarfs++;

            if (dwarf_info[dwarf].state == IDLE)
                aggressive_mode = false;
        }
}

void LegacyEngine::update(color_ostream &out)
{
    if (!world || !world->map.block_index || !initialized)
        return;

    if (world->frame_counter - cycle_timestamp < CYCLE_TICKS)
        return;

    cycle_timestamp = world->frame_counter;

    std::vector<df::unit *> dwarfs;

    auto bscan = autolabor::scan_buildings();
    bool has_butchers = bscan.has_butchers;
    bool has_fishery = bscan.has_fishery;
    bool trader_requested = bscan.trader_requested;

    for (auto& cre : world->units.active)
    {
        if (Units::isCitizen(cre))
        {
            if (cre->burrows.size() > 0)
                continue;        // dwarfs assigned to active burrows are skipped entirely
            dwarfs.push_back(cre);
        }
    }

    int n_dwarfs = dwarfs.size();

    if (n_dwarfs == 0)
        return;

    std::vector<dwarf_info_t> dwarf_info(n_dwarfs);

    // Find total skill and highest skill for each dwarf. More skilled dwarves shouldn't be used for minor tasks.

    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        if (dwarfs[dwarf]->status.souls.size() <= 0)
            continue;

        // compute noble penalty

        auto pos = autolabor::unit_position_info(dwarfs[dwarf]);
        dwarf_info[dwarf].noble_penalty = pos.noble_penalty;
        dwarf_info[dwarf].medical = pos.medical;
        dwarf_info[dwarf].trader = pos.trader;

        // identify dwarfs who are needed for meetings and mark them for exclusion

        if (autolabor::in_diplomacy_meeting(dwarfs[dwarf]))
        {
            dwarf_info[dwarf].diplomacy = true;
            DEBUG(legacy_cycle, out).print("Dwarf {} \"{}\" has a meeting, will be cleared of all labors\n",
                dwarf, dwarfs[dwarf]->name.first_name);
        }

        dwarf_info[dwarf].total_skill = autolabor::total_skill(dwarfs[dwarf]);
        dwarf_info[dwarf].highest_skill = autolabor::top_skill(dwarfs[dwarf]);
    }

    // Calculate a base penalty for using each dwarf for a task he isn't good at.

    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        dwarf_info[dwarf].mastery_penalty -= 40 * dwarf_info[dwarf].highest_skill;
        dwarf_info[dwarf].mastery_penalty -= 10 * dwarf_info[dwarf].total_skill;
        dwarf_info[dwarf].mastery_penalty -= dwarf_info[dwarf].noble_penalty;

        FOR_ENUM_ITEMS(unit_labor, labor)
        {
            if (labor == unit_labor::NONE)
                continue;

            if (labor_infos[labor].is_exclusive && dwarfs[dwarf]->status.labors[labor])
                dwarf_info[dwarf].mastery_penalty -= 100;
        }
    }

    // Find the activity state for each dwarf. It's important to get this right - a dwarf who we think is IDLE but
    // can't work will gum everything up. In the future I might add code to auto-detect slacker dwarves.

    state_count.clear();
    state_count.resize(NUM_STATE);

    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        dwarf_info[dwarf].state = autolabor::get_dwarf_state(dwarfs[dwarf]);
        if (dwarf_info[dwarf].state == OTHER &&
            dwarfs[dwarf]->job.current_job &&
            (dwarfs[dwarf]->job.current_job->job_type < 0 ||
             size_t(dwarfs[dwarf]->job.current_job->job_type) >= dwarf_state_count))
        {
            WARN(legacy_cycle, out).print("Dwarf {} \"{}\" has unknown job {}\n",
                dwarf, dwarfs[dwarf]->name.first_name, int(dwarfs[dwarf]->job.current_job->job_type));
        }

        state_count[dwarf_info[dwarf].state]++;

        autolabor::probe_labor_observation(dwarfs[dwarf]);

        TRACE(legacy_cycle, out).print("Dwarf {} \"{}\": penalty {}, state {}\n",
            dwarf, dwarfs[dwarf]->name.first_name, dwarf_info[dwarf].mastery_penalty, state_names[dwarf_info[dwarf].state]);
    }

    std::vector<df::unit_labor> labors;

    FOR_ENUM_ITEMS(unit_labor, labor)
    {
        if (labor == unit_labor::NONE)
            continue;

        labor_infos[labor].active_dwarfs = 0;

        labors.push_back(labor);
    }
    laborinfo_sorter lasorter;
    std::sort(labors.begin(), labors.end(), lasorter);

    // Handle DISABLED skills (just bookkeeping).
    // Note that autolabor should *NEVER* enable or disable a skill that has been marked as DISABLED, for any reason.
    // The user has told us that they want manage this skill manually, and we must respect that.
    for (auto& labor: labors)
    {
        if (labor_infos[labor].mode() != DISABLE)
            continue;

        for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
        {
            if (dwarfs[dwarf]->status.labors[labor])
            {
                if (labor_infos[labor].is_exclusive)
                    dwarf_info[dwarf].has_exclusive_labor = true;

                dwarf_info[dwarf].assigned_jobs++;
            }
        }
    }

    // Handle all skills except those marked HAULERS

    for (auto& labor : labors)
    {
        assign_labor(labor, n_dwarfs, dwarf_info, trader_requested, dwarfs, has_butchers, has_fishery, out);
    }

    // Set about 1/3 of the dwarfs as haulers. The haulers have all HAULER labors enabled. Having a lot of haulers helps
    // make sure that hauling jobs are handled quickly rather than building up.

    int num_haulers = state_count[IDLE] + (state_count[BUSY] + state_count[EXCLUSIVE]) * hauler_pct / 100;

    if (num_haulers < 1)
        num_haulers = 1;

    std::vector<int> hauler_ids;
    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        if ((dwarf_info[dwarf].trader && trader_requested) ||
            dwarf_info[dwarf].diplomacy)
        {
            FOR_ENUM_ITEMS(unit_labor, labor)
            {
                if (labor == unit_labor::NONE)
                    continue;
                if (labor_infos[labor].mode() != HAULERS)
                    continue;
                dwarfs[dwarf]->status.labors[labor] = false;
            }
            if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY || dwarf_info[dwarf].state == EXCLUSIVE)
            {
                num_haulers--;
            }
            continue;
        }

        if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY || dwarf_info[dwarf].state == EXCLUSIVE)
            hauler_ids.push_back(dwarf);
    }
    dwarfinfo_sorter sorter(dwarf_info);
    // Idle dwarves come first, then we sort from least-skilled to most-skilled.
    std::sort(hauler_ids.begin(), hauler_ids.end(), sorter);

    // don't set any haulers if everyone is off drinking or something
    if (hauler_ids.size() == 0) {
        num_haulers = 0;
    }

    FOR_ENUM_ITEMS(unit_labor, labor)
    {
        if (labor == unit_labor::NONE)
            continue;

        if (labor_infos[labor].mode() != HAULERS)
            continue;

        for (int i = 0; i < num_haulers; i++)
        {
            int dwarf = hauler_ids[i];

            dwarfs[dwarf]->status.labors[labor] = true;
            dwarf_info[dwarf].assigned_jobs++;

            if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY || dwarf_info[dwarf].state == EXCLUSIVE)
                labor_infos[labor].active_dwarfs++;

            TRACE(legacy_cycle, out).print("Dwarf {} \"{}\" assigned {}: hauler\n",
                dwarf, dwarfs[dwarf]->name.first_name, ENUM_KEY_STR(unit_labor, labor));
        }

        for (size_t i = num_haulers; i < hauler_ids.size(); i++)
        {
            int dwarf = hauler_ids[i];

            dwarfs[dwarf]->status.labors[labor] = false;
        }
    }
}

static void print_labor (df::unit_labor labor, color_ostream &out)
{
    std::string labor_name = ENUM_KEY_STR(unit_labor, labor);
    out << labor_name << ": ";
    for (int i = 0; i < 20 - (int)labor_name.length(); i++)
        out << ' ';
    if (labor_infos[labor].mode() == DISABLE)
        out << "disabled" << std::endl;
    else
    {
        if (labor_infos[labor].mode() == HAULERS)
            out << "haulers";
        else
            out << "minimum " << labor_infos[labor].minimum_dwarfs() << ", maximum " << labor_infos[labor].maximum_dwarfs()
                << ", pool " << labor_infos[labor].talent_pool();
        out << ", currently " << labor_infos[labor].active_dwarfs << " dwarfs" << std::endl;
    }
}

bool LegacyEngine::command(color_ostream &out, std::vector <std::string> & parameters)
{
    if (parameters.empty())
        return false;
    if (!initialized)
    {
        out << "Error: The plugin is not enabled in legacy mode." << std::endl;
        return true;
    }

    if (parameters.size() == 2 && parameters[0] == "haulpct")
    {
        int pct = atoi (parameters[1].c_str());
        hauler_pct = pct;
        return true;
    }
    else if (parameters.size() >= 2 && parameters.size() <= 4)
    {
        df::unit_labor labor = unit_labor::NONE;

        FOR_ENUM_ITEMS(unit_labor, test_labor)
        {
            if (parameters[0] == ENUM_KEY_STR(unit_labor, test_labor))
                labor = test_labor;
        }

        if (labor == unit_labor::NONE)
        {
            out.printerr("Could not find labor {}.\n", parameters[0]);
            return true;
        }

        if (parameters[1] == "haulers")
        {
            labor_infos[labor].set_mode(HAULERS);
            print_labor(labor, out);
            return true;
        }
        if (parameters[1] == "disable")
        {
            labor_infos[labor].set_mode(DISABLE);
            print_labor(labor, out);
            return true;
        }
        if (parameters[1] == "reset")
        {
            reset_labor(labor);
            print_labor(labor, out);
            return true;
        }

        int minimum = atoi (parameters[1].c_str());
        int maximum = 200;
        int pool = 200;

        if (parameters.size() >= 3)
            maximum = atoi (parameters[2].c_str());
        if (parameters.size() == 4)
            pool = std::stoi(parameters[3]);

        if (maximum < minimum || maximum < 0 || minimum < 0)
        {
            out.printerr("Syntax: autolabor <labor> <minimum> [<maximum>] [<talent pool>], {} > {}\n", maximum, minimum);
            return true;
        }

        labor_infos[labor].set_minimum_dwarfs(minimum);
        labor_infos[labor].set_maximum_dwarfs(maximum);
        labor_infos[labor].set_talent_pool(pool);
        labor_infos[labor].set_mode(AUTOMATIC);
        print_labor(labor, out);

        return true;
    }
    else if (parameters.size() == 1 && parameters[0] == "reset-all")
    {
        for (size_t i = 0; i < labor_infos.size(); i++)
        {
            reset_labor((df::unit_labor) i);
        }
        out << "All labors reset." << std::endl;
        return true;
    }
    else if (parameters.size() == 1 && (parameters[0] == "list" || parameters[0] == "status"))
    {
        bool need_comma = 0;
        for (int i = 0; i < NUM_STATE; i++)
        {
            if (state_count[i] == 0)
                continue;
            if (need_comma)
                out << ", ";
            out << state_count[i] << ' ' << state_names[i];
            need_comma = 1;
        }
        out << std::endl;

        if (parameters[0] == "list")
        {
            FOR_ENUM_ITEMS(unit_labor, labor)
            {
                if (labor == unit_labor::NONE)
                    continue;

                print_labor(labor, out);
            }
        }

        return true;
    }

    return false;
}
