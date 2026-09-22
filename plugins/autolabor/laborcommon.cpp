#include "laborcommon.h"

#include "modules/Units.h"
#include "modules/World.h"

#include <df/activity_info.h>
#include <df/building.h>
#include <df/building_tradedepotst.h>
#include <df/entity_position.h>
#include <df/entity_position_assignment.h>
#include <df/entity_position_responsibility.h>
#include <df/global_objects.h>
#include <df/histfig_entity_link.h>
#include <df/histfig_entity_link_positionst.h>
#include <df/historical_entity.h>
#include <df/historical_figure.h>
#include <df/plotinfost.h>
#include <df/unit_misc_trait.h>
#include <df/unit_skill.h>
#include <df/unit_soul.h>
#include <df/workshop_type.h>
#include <df/world.h>

using namespace DFHack;
using namespace df::enums;

using df::global::plotinfo;
using df::global::world;

namespace autolabor {

PersistentDataItem plugin_config;

void load_plugin_config()
{
    plugin_config = World::GetPersistentSiteData("autolabor/config", true);
    if (plugin_config.isValid() && plugin_config.ival(0) == -1)
        plugin_config.ival(0) = 0;
}

bool config_flag(unsigned flag)
{
    return plugin_config.isValid() && (plugin_config.ival(0) & flag) != 0;
}

void set_config_flag(unsigned flag, bool on)
{
    if (!plugin_config.isValid())
        return;
    if (on)
        plugin_config.ival(0) |= flag;
    else
        plugin_config.ival(0) &= ~flag;
}

int engine_mode()
{
    if (!plugin_config.isValid())
        return MODE_LEGACY;
    int m = plugin_config.get_int(1);
    return (m >= MODE_LEGACY && m <= MODE_MONITOR) ? m : MODE_LEGACY;
}

void store_engine_mode(int mode)
{
    if (plugin_config.isValid())
        plugin_config.ival(1) = mode;
}

int balance()
{
    if (!plugin_config.isValid())
        return NUM_BALANCE_STOPS / 2;
    int v = plugin_config.get_int(2);
    if (v < 0 || v >= NUM_BALANCE_STOPS)
        v = NUM_BALANCE_STOPS / 2; // default: balanced
    return v;
}

void store_balance(int v)
{
    if (plugin_config.isValid() && v >= 0 && v < NUM_BALANCE_STOPS)
        plugin_config.ival(2) = v;
}

static const char * const balance_names[] = {
    "staffing", "lean-staffing", "balanced", "lean-skills", "skills"
};

const char *balance_stop_name(int i)
{
    if (i < 0 || i >= NUM_BALANCE_STOPS)
        return NULL;
    return balance_names[i];
}

int idle_reserve()
{
    if (!plugin_config.isValid())
        return 30;
    int v = plugin_config.get_int(3);
    return (v < 0 || v > 100) ? 30 : v;
}

void store_idle_reserve(int v)
{
    if (plugin_config.isValid() && v >= 0 && v <= 100)
        plugin_config.ival(3) = v;
}

df::job_skill labor_to_skill[NUM_LABORS];

void init_labor_to_skill()
{
    for (int i = 0; i < NUM_LABORS; i++)
        labor_to_skill[i] = job_skill::NONE;

    FOR_ENUM_ITEMS(job_skill, skill)
    {
        int labor = ENUM_ATTR(job_skill, labor, skill);
        if (labor >= 0 && labor < NUM_LABORS)
            labor_to_skill[labor] = skill;
    }
}

bool is_exclusive_labor(df::unit_labor l)
{
    // labors that carry tools; toggling them causes equipment churn
    switch (l) {
    case unit_labor::MINE:
    case unit_labor::CUTWOOD:
    case unit_labor::HUNT:
        return true;
    default:
        return false;
    }
}

BuildingScan scan_buildings()
{
    BuildingScan scan;

    for (auto build : world->buildings.all)
    {
        auto type = build->getType();
        if (building_type::Workshop == type)
        {
            df::workshop_type subType = (df::workshop_type)build->getSubtype();
            if (workshop_type::Butchers == subType)
                scan.has_butchers = true;
            if (workshop_type::Fishery == subType)
                scan.has_fishery = true;
        }
        else if (building_type::TradeDepot == type)
        {
            df::building_tradedepotst* depot =
                strict_virtual_cast<df::building_tradedepotst>(build);
            if (depot)
                scan.trader_requested =
                    scan.trader_requested || depot->trade_flags.bits.trader_requested;
        }
    }

    return scan;
}

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

PositionInfo unit_position_info(df::unit *u)
{
    PositionInfo info;

    df::historical_figure* hf = df::historical_figure::find(u->hist_figure_id);
    if (!hf) // can be NULL, e.g. script-created citizens
        return info;

    for (auto hfelink : hf->entity_links)
    {
        if (hfelink->getType() != df::histfig_entity_link_type::POSITION)
            continue;

        df::histfig_entity_link_positionst *epos =
            (df::histfig_entity_link_positionst*) hfelink;
        df::historical_entity* entity = df::historical_entity::find(epos->entity_id);
        if (!entity)
            continue;
        df::entity_position_assignment* assignment =
            binsearch_in_vector(entity->positions.assignments, epos->assignment_id);
        if (!assignment)
            continue;
        df::entity_position* position =
            binsearch_in_vector(entity->positions.own, assignment->position_id);
        if (!position)
            continue;

        for (int n = 0; n < 25; n++)
            if (position->responsibilities[n])
                info.noble_penalty += responsibility_penalties[n];

        if (position->responsibilities[df::entity_position_responsibility::HEALTH_MANAGEMENT])
            info.medical = true;

        if (position->responsibilities[df::entity_position_responsibility::TRADE])
            info.trader = true;
    }

    return info;
}

bool in_diplomacy_meeting(df::unit *u)
{
    for (auto act : plotinfo->activities)
    {
        if (!act) continue;
        if (act->unit_actor == u->id || act->unit_noble == u->id)
            return true;
    }
    return false;
}

bool is_assignable(df::unit *u)
{
    return Units::isOwnCiv(u) &&
        Units::isOwnGroup(u) &&
        Units::isActive(u) &&
        !u->flags2.bits.visitor &&
        !u->flags3.bits.ghostly &&
        ENUM_ATTR(profession, can_assign_labor, u->profession);
}

dwarf_state get_dwarf_state(df::unit *u)
{
    if (Units::isBaby(u) || Units::isChild(u) ||
        u->profession == profession::DRUNK)
        return dwarf_state::CHILD;

    if (ENUM_ATTR(profession, military, u->profession))
        return dwarf_state::MILITARY;

    if (!u->job.current_job)
    {
        if (Units::getMiscTrait(u, misc_trait_type::Migrant) ||
            u->specific_refs.size() > 0)
            return dwarf_state::OTHER;
        return dwarf_state::IDLE;
    }

    int job = u->job.current_job->job_type;
    if (job >= 0 && size_t(job) < dwarf_state_count)
        return dwarf_states[job];

    return dwarf_state::OTHER;
}

int top_skill(df::unit *u, df::job_skill *skill_out)
{
    int best = 0;
    df::job_skill best_skill = job_skill::NONE;

    if (u->status.souls.size() > 0)
    {
        for (auto skill : u->status.souls[0]->skills)
        {
            df::job_skill_class skill_class = ENUM_ATTR(job_skill, type, skill->id);
            if (skill_class != job_skill_class::Normal &&
                skill_class != job_skill_class::Medical)
                continue;

            if (best < skill->rating)
            {
                best = skill->rating;
                best_skill = skill->id;
            }
        }
    }

    if (skill_out)
        *skill_out = best_skill;
    return best;
}

int total_skill(df::unit *u)
{
    int total = 0;
    if (u->status.souls.size() > 0)
    {
        for (auto skill : u->status.souls[0]->skills)
        {
            df::job_skill_class skill_class = ENUM_ATTR(job_skill, type, skill->id);
            if (skill_class != job_skill_class::Normal &&
                skill_class != job_skill_class::Medical)
                continue;
            total += skill->rating;
        }
    }
    return total;
}

}
