#include "laborstatemap.h"

#include "Debug.h"
#include "PluginManager.h"

#include "modules/Units.h"
#include "modules/World.h"
#include "modules/MapCache.h"
#include "modules/Items.h"
#include "modules/Units.h"

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

DFHACK_PLUGIN("autolabor");
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(game);

#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))

/*
 * Autolabor module for dfhack
 *
 * The idea behind this module is to constantly adjust labors so that the right dwarves
 * are assigned to new tasks. The key is that, for almost all labors, once a dwarf begins
 * a job it will finish that job even if the associated labor is removed. Thus the
 * strategy is to frequently decide, for each labor, which dwarves should possibly take
 * a new job for that labor if it comes in and which shouldn't, and then set the labors
 * appropriately. The updating should happen as often as can be reasonably done without
 * causing lag.
 *
 * The obvious thing to do is to just set each labor on a single idle dwarf who is best
 * suited to doing new jobs of that labor. This works in a way, but it leads to a lot
 * of idle dwarves since only one dwarf will be dispatched for each labor in an update
 * cycle, and dwarves that finish tasks will wait for the next update before being
 * dispatched. An improvement is to also set some labors on dwarves that are currently
 * doing a job, so that they will immediately take a new job when they finish. The
 * details of which dwarves should have labors set is mostly a heuristic.
 *
 * A complication to the above simple scheme is labors that have associated equipment.
 * Enabling/disabling these labors causes dwarves to change equipment, and disabling
 * them in the middle of a job may cause the job to be abandoned. Those labors
 * (mining, hunting, and woodcutting) need to be handled carefully to minimize churn.
 */

DFHACK_PLUGIN_IS_ENABLED(enable_autolabor);

namespace DFHack {
    DBG_DECLARE(autolabor, cycle, DebugCategory::LINFO);
}

static std::vector<int> state_count(NUM_STATE);

static PersistentDataItem config;

command_result autolabor (color_ostream &out, std::vector <std::string> & parameters);

static bool isOptionEnabled(unsigned flag)
{
    return config.isValid() && (config.ival(0) & flag) != 0;
}

enum ConfigFlags {
    CF_ENABLED = 1,
};

static void setOptionEnabled(ConfigFlags flag, bool on)
{
    if (!config.isValid())
        return;

    if (on)
        config.ival(0) |= flag;
    else
        config.ival(0) &= ~flag;
}


static void generate_labor_to_skill_map();

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

static const int responsibility_penalties[] = {
    0,      /* LAW_MAKING */
    0,      /* LAW_ENFORCEMENT */
    3000,   /* RECEIVE_DIPLOMATS */
    0,      /* MEET_WORKERS */
    1000,   /* MANAGE_PRODUCTION */
    3000,   /* TRADE */
    1000,   /* ACCOUNTING */
    0,      /* ESTABLISH_COLONY_TRADE_AGREEMENTS */
    0,      /* MAKE_INTRODUCTIONS */
    0,      /* MAKE_PEACE_AGREEMENTS */
    0,      /* MAKE_TOPIC_AGREEMENTS */
    0,      /* COLLECT_TAXES */
    0,      /* ESCORT_TAX_COLLECTOR */
    0,      /* EXECUTIONS */
    0,      /* TAME_EXOTICS */
    0,      /* RELIGION */
    0,      /* ATTACK_ENEMIES */
    0,      /* PATROL_TERRITORY */
    0,      /* MILITARY_GOALS */
    0,      /* MILITARY_STRATEGY */
    0,      /* UPGRADE_SQUAD_EQUIPMENT */
    0,      /* EQUIPMENT_MANIFESTS */
    0,      /* SORT_AMMUNITION */
    0,      /* BUILD_MORALE */
    5000    /* HEALTH_MANAGEMENT */
};

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

static void cleanup_state()
{
    enable_autolabor = false;
    labor_infos.clear();

    // reinstate DF's work detail system
    game->external_flag.bits.automatic_professions_disabled = false;
}

static void reset_labor(df::unit_labor labor)
{
    labor_infos[labor].set_minimum_dwarfs(default_labor_infos[labor].minimum_dwarfs);
    labor_infos[labor].set_maximum_dwarfs(default_labor_infos[labor].maximum_dwarfs);
    labor_infos[labor].set_talent_pool(200);
    labor_infos[labor].set_mode(default_labor_infos[labor].mode);
}

static void init_state()
{
    config = World::GetPersistentSiteData("autolabor/config");
    if (config.isValid() && config.ival(0) == -1)
        config.ival(0) = 0;

    enable_autolabor = isOptionEnabled(CF_ENABLED);

    if (!enable_autolabor)
        return;

    // bypass DF's work detail system
    game->external_flag.bits.automatic_professions_disabled = true;

    auto cfg_haulpct = World::GetPersistentSiteData("autolabor/haulpct");
    if (cfg_haulpct.isValid())
    {
        hauler_pct = cfg_haulpct.ival(0);
    }
    else
    {
        hauler_pct = 33;
    }

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

    generate_labor_to_skill_map();

}

static df::job_skill labor_to_skill[ENUM_LAST_ITEM(unit_labor) + 1];

static void generate_labor_to_skill_map()
{
    // Generate labor -> skill mapping

    for (int i = 0; i <= ENUM_LAST_ITEM(unit_labor); i++)
        labor_to_skill[i] = job_skill::NONE;

    FOR_ENUM_ITEMS(job_skill, skill)
    {
        int labor = ENUM_ATTR(job_skill, labor, skill);
        if (labor != unit_labor::NONE)
        {
            /*
            assert(labor >= 0);
            assert(labor < ARRAY_COUNT(labor_to_skill));
            */

            labor_to_skill[labor] = skill;
        }
    }

}


static void enable_plugin(color_ostream &out)
{
    if (!config.isValid())
    {
        config = World::AddPersistentSiteData("autolabor/config");
        config.ival(0) = 0;
    }

    setOptionEnabled(CF_ENABLED, true);
    enable_autolabor = true;
    out << "Enabling autolabor." << std::endl;

    cleanup_state();
    init_state();
}

static void disable_plugin(color_ostream& out)
{
    if (config.isValid())
        setOptionEnabled(CF_ENABLED, false);

    enable_autolabor = false;
    out << "Disabling autolabor." << std::endl;

    cleanup_state();
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // initialize labor infos table from default table
    if(ARRAY_COUNT(default_labor_infos) != ENUM_LAST_ITEM(unit_labor) + 1)
    {
        out.printerr("autolabor: labor size mismatch\n");
        return CR_FAILURE;
    }

    commands.push_back(PluginCommand(
        "autolabor",
        "Automatically manage dwarf labors.",
        autolabor));

    init_state();

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    cleanup_state();

    return CR_OK;
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

            assert(dwarf >= 0);
            assert(dwarf < n_dwarfs);

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

            TRACE(cycle, out).print("Dwarf % i \"%s\" assigned %s: value %i %s %s\n",
                dwarf, dwarfs[dwarf]->name.first_name.c_str(), ENUM_KEY_STR(unit_labor, labor).c_str(), values[dwarf],
                dwarf_info[dwarf].trader ? "(trader)" : "",
                dwarf_info[dwarf].diplomacy ? "(diplomacy)" : "");

            if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY || dwarf_info[dwarf].state == EXCLUSIVE)
                labor_infos[labor].active_dwarfs++;

            if (dwarf_info[dwarf].state == IDLE)
                aggressive_mode = false;
        }
}

static const int32_t CYCLE_TICKS = 61;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    cleanup_state();
    init_state();
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_UNLOADED:
        cleanup_state();
        break;
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if(!world || !world->map.block_index || !enable_autolabor)
    {
        return CR_OK;
    }

    if (world->frame_counter - cycle_timestamp <= CYCLE_TICKS)
        return CR_OK;

    cycle_timestamp = world->frame_counter;

    std::vector<df::unit *> dwarfs;

    bool has_butchers = false;
    bool has_fishery = false;
    bool trader_requested = false;

    for (auto& build : world->buildings.all)
    {
        auto type = build->getType();
        if (building_type::Workshop == type)
        {
            df::workshop_type subType = (df::workshop_type)build->getSubtype();
            if (workshop_type::Butchers == subType)
                has_butchers = true;
            if (workshop_type::Fishery == subType)
                has_fishery = true;
        }
        else if (building_type::TradeDepot == type)
        {
            df::building_tradedepotst* depot = (df::building_tradedepotst*) build;
            trader_requested = trader_requested || depot->trade_flags.bits.trader_requested;
            TRACE(cycle,out).print(trader_requested
                ? "Trade depot found and trader requested, trader will be excluded from all labors.\n"
                : "Trade depot found but trader is not requested.\n"
                );
        }
    }

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
        return CR_OK;

    std::vector<dwarf_info_t> dwarf_info(n_dwarfs);

    // Find total skill and highest skill for each dwarf. More skilled dwarves shouldn't be used for minor tasks.

    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        if (dwarfs[dwarf]->status.souls.size() <= 0)
            continue;

        // compute noble penalty

        int noble_penalty = 0;

        df::historical_figure* hf = df::historical_figure::find(dwarfs[dwarf]->hist_figure_id);
        if(hf!=NULL) //can be NULL. E.g. script created citizens
        for (auto& hfelink : hf->entity_links)
        {
            if (hfelink->getType() == df::histfig_entity_link_type::POSITION)
            {
                df::histfig_entity_link_positionst *epos =
                    (df::histfig_entity_link_positionst*) hfelink;
                df::historical_entity* entity = df::historical_entity::find(epos->entity_id);
                if (!entity)
                    continue;
                df::entity_position_assignment* assignment = binsearch_in_vector(entity->positions.assignments, epos->assignment_id);
                if (!assignment)
                    continue;
                df::entity_position* position = binsearch_in_vector(entity->positions.own, assignment->position_id);
                if (!position)
                    continue;

                for (int n = 0; n < 25; n++)
                    if (position->responsibilities[n])
                        noble_penalty += responsibility_penalties[n];

                if (position->responsibilities[df::entity_position_responsibility::HEALTH_MANAGEMENT])
                    dwarf_info[dwarf].medical = true;

                if (position->responsibilities[df::entity_position_responsibility::TRADE])
                    dwarf_info[dwarf].trader = true;

            }

        }

        dwarf_info[dwarf].noble_penalty = noble_penalty;

        // identify dwarfs who are needed for meetings and mark them for exclusion

        for (auto& act : plotinfo->activities)
        {
            if (!act) continue;
            bool p1 = act->unit_actor == dwarfs[dwarf]->id;
            bool p2 = act->unit_noble == dwarfs[dwarf]->id;

            if (p1 || p2)
            {
                dwarf_info[dwarf].diplomacy = true;
                DEBUG(cycle, out).print("Dwarf %i \"%s\" has a meeting, will be cleared of all labors\n",
                    dwarf, dwarfs[dwarf]->name.first_name.c_str());
                break;
            }
        }

        for (auto& skill : dwarfs[dwarf]->status.souls[0]->skills)
        {
            df::job_skill_class skill_class = ENUM_ATTR(job_skill, type, skill->id);

            int skill_level = skill->rating;

            // Track total & highest skill among normal/medical skills. (We don't care about personal or social skills.)

            if (skill_class != job_skill_class::Normal && skill_class != job_skill_class::Medical)
                continue;

            if (dwarf_info[dwarf].highest_skill < skill_level)
                dwarf_info[dwarf].highest_skill = skill_level;
            dwarf_info[dwarf].total_skill += skill_level;
        }
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
        if (Units::isBaby(dwarfs[dwarf]) ||
            Units::isChild(dwarfs[dwarf]) ||
            dwarfs[dwarf]->profession == profession::DRUNK)
        {
            dwarf_info[dwarf].state = CHILD;
        }
        else if (ENUM_ATTR(profession, military, dwarfs[dwarf]->profession))
            dwarf_info[dwarf].state = MILITARY;
        else if (dwarfs[dwarf]->job.current_job == NULL)
        {
            if (Units::getMiscTrait(dwarfs[dwarf], misc_trait_type::Migrant))
                dwarf_info[dwarf].state = OTHER;
            else if (dwarfs[dwarf]->specific_refs.size() > 0)
                dwarf_info[dwarf].state = OTHER;
            else
                dwarf_info[dwarf].state = IDLE;
        }
        else
        {
            int job = dwarfs[dwarf]->job.current_job->job_type;
            if (job >= 0 && size_t(job) < ARRAY_COUNT(dwarf_states))
                dwarf_info[dwarf].state = dwarf_states[job];
            else
            {
                WARN(cycle, out).print("Dwarf %i \"%s\" has unknown job %i\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(), job);
                dwarf_info[dwarf].state = OTHER;
            }
        }

        state_count[dwarf_info[dwarf].state]++;

        TRACE(cycle, out).print("Dwarf %i \"%s\": penalty %i, state %s\n",
            dwarf, dwarfs[dwarf]->name.first_name.c_str(), dwarf_info[dwarf].mastery_penalty, state_names[dwarf_info[dwarf].state]);
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
            assert(size_t(i) < hauler_ids.size());

            int dwarf = hauler_ids[i];

            assert(dwarf >= 0);
            assert(dwarf < n_dwarfs);
            dwarfs[dwarf]->status.labors[labor] = true;
            dwarf_info[dwarf].assigned_jobs++;

            if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY || dwarf_info[dwarf].state == EXCLUSIVE)
                labor_infos[labor].active_dwarfs++;

            TRACE(cycle, out).print("Dwarf %i \"%s\" assigned %s: hauler\n",
                dwarf, dwarfs[dwarf]->name.first_name.c_str(), ENUM_KEY_STR(unit_labor, labor).c_str());
        }

        for (size_t i = num_haulers; i < hauler_ids.size(); i++)
        {
            assert(i < hauler_ids.size());

            int dwarf = hauler_ids[i];

            assert(dwarf >= 0);
            assert(dwarf < n_dwarfs);

            dwarfs[dwarf]->status.labors[labor] = false;
        }
    }

    return CR_OK;
}

void print_labor (df::unit_labor labor, color_ostream &out)
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

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable )
{
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable && !enable_autolabor)
    {
        enable_plugin(out);
    }
    else if(!enable && enable_autolabor)
    {
        disable_plugin(out);
    }

    return CR_OK;
}

command_result autolabor (color_ostream &out, std::vector <std::string> & parameters)
{
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (parameters.size() == 1 &&
        (parameters[0] == "0" || parameters[0] == "enable" ||
         parameters[0] == "1" || parameters[0] == "disable"))
    {
        bool enable = (parameters[0] == "1" || parameters[0] == "enable");

        return plugin_enable(out, enable);
    }
    else if (parameters.size() == 2 && parameters[0] == "haulpct")
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << std::endl;
            return CR_FAILURE;
        }

        int pct = atoi (parameters[1].c_str());
        hauler_pct = pct;
        return CR_OK;
    }
    else if (parameters.size() >= 2 && parameters.size() <= 4)
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << std::endl;
            return CR_FAILURE;
        }

        df::unit_labor labor = unit_labor::NONE;

        FOR_ENUM_ITEMS(unit_labor, test_labor)
        {
            if (parameters[0] == ENUM_KEY_STR(unit_labor, test_labor))
                labor = test_labor;
        }

        if (labor == unit_labor::NONE)
        {
            out.printerr("Could not find labor %s.\n", parameters[0].c_str());
            return CR_WRONG_USAGE;
        }

        if (parameters[1] == "haulers")
        {
            labor_infos[labor].set_mode(HAULERS);
            print_labor(labor, out);
            return CR_OK;
        }
        if (parameters[1] == "disable")
        {
            labor_infos[labor].set_mode(DISABLE);
            print_labor(labor, out);
            return CR_OK;
        }
        if (parameters[1] == "reset")
        {
            reset_labor(labor);
            print_labor(labor, out);
            return CR_OK;
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
            out.printerr("Syntax: autolabor <labor> <minimum> [<maximum>] [<talent pool>], %d > %d\n", maximum, minimum);
            return CR_WRONG_USAGE;
        }

        labor_infos[labor].set_minimum_dwarfs(minimum);
        labor_infos[labor].set_maximum_dwarfs(maximum);
        labor_infos[labor].set_talent_pool(pool);
        labor_infos[labor].set_mode(AUTOMATIC);
        print_labor(labor, out);

        return CR_OK;
    }
    else if (parameters.size() == 1 && parameters[0] == "reset-all")
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << std::endl;
            return CR_FAILURE;
        }

        for (size_t i = 0; i < labor_infos.size(); i++)
        {
            reset_labor((df::unit_labor) i);
        }
        out << "All labors reset." << std::endl;
        return CR_OK;
    }
    else if (parameters.size() == 1 && (parameters[0] == "list" || parameters[0] == "status"))
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << std::endl;
            return CR_FAILURE;
        }

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

        return CR_OK;
    }
    else
    {
        out.print("Automatically assigns labors to dwarves.\n"
            "Activate with 'enable autolabor', deactivate with 'disable autolabor'.\n"
            "Current state: %d.\n", enable_autolabor);

        return CR_OK;
    }
}
