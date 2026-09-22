/*
 * Modern engine for the autolabor plugin ("labormanager" command).
 *
 * Expresses labor policy through DF's work detail system rather than writing
 * the labor matrix directly. The v50+ job auction already prefers skilled
 * workers for skilled jobs, so this engine focuses on what the auction does
 * badly: starving jobs that require no skill, and giving skilled dwarves
 * room to practice and teach.
 */

#include "joblabormapper.h"
#include "modernengine.h"
#include "monitorengine.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "Debug.h"
#include "MiscUtils.h"
#include "PluginManager.h"

#include "modules/Job.h"
#include "modules/Units.h"
#include "modules/World.h"

#include <df/abstract_building_guildhallst.h>
#include <df/building.h>
#include <df/caste_raw_flags.h>
#include <df/gamest.h>
#include <df/general_ref_unit_workerst.h>
#include <df/global_objects.h>
#include <df/item_weaponst.h>
#include <df/itemdef_weaponst.h>
#include <df/items_other_id.h>
#include <df/job.h>
#include <df/job_postingst.h>
#include <df/personality_needst.h>
#include <df/plotinfost.h>
#include <df/reaction.h>
#include <df/skill_rating.h>
#include <df/unit.h>
#include <df/unit_health_info.h>
#include <df/unit_skill.h>
#include <df/unit_soul.h>
#include <df/vehicle.h>
#include <df/workshop_profile.h>
#include <df/world.h>
#include <df/world_site.h>

using namespace DFHack;
using namespace df::enums;

using df::global::plotinfo;
using df::global::world;
using df::global::game;

namespace DFHack {
    DBG_DECLARE(autolabor, modern_cycle, DebugCategory::LINFO);
}

using namespace autolabor;

static const int32_t CYCLE_TICKS = 61;
static int32_t cycle_timestamp = 0;

// a posting that sits unclaimed for this many cycles counts as starving
static const int STARVE_CYCLES = 5;
// a posting on the board longer than this (one game day) triggers a
// task starvation warning in the DFHack notification panel
static const int32_t STARVE_TICKS = 1200;
// specialists work this many consecutive cycles before earning rest
static const int BUSY_LIMIT = 10;

static const std::string LABORER_DETAIL = "auto:Laborers";
static const std::string CARE_DETAIL = "auto:Care";
static const std::string SKILL_PREFIX = "auto:Skill:";

// labors that should stay available to every unit even when specialized;
// these are covered by the Care detail so flag-restricted units keep them
static bool is_care_labor(df::unit_labor l)
{
    switch (l) {
    case unit_labor::RECOVER_WOUNDED:
    case unit_labor::FEED_WATER_CIVILIANS:
    case unit_labor::HAUL_WATER:
        return true;
    default:
        return false;
    }
}

enum tools_enum {
    TOOL_NONE, TOOL_PICK, TOOL_AXE, TOOL_CROSSBOW,
    TOOLS_MAX
};

static tools_enum labor_tool(df::unit_labor l)
{
    switch (l) {
    case unit_labor::MINE:    return TOOL_PICK;
    case unit_labor::CUTWOOD: return TOOL_AXE;
    case unit_labor::HUNT:    return TOOL_CROSSBOW;
    default:                  return TOOL_NONE;
    }
}

namespace autolabor {

void modern_labor_coverage(std::vector<std::string> &missing)
{
    FOR_ENUM_ITEMS(unit_labor, l)
    {
        if (l == unit_labor::NONE)
            continue;
        // the exclusive-labor check and the tool-cap mapping must agree;
        // a labor that needs a tool without being flagged would let the
        // engine oversubscribe equipment
        if (is_exclusive_labor(l) != (labor_tool(l) != TOOL_NONE))
            missing.push_back(ENUM_KEY_STR(unit_labor, l) +
                ": is_exclusive_labor/labor_tool disagree");
    }
}

} // namespace autolabor

// ---------------------------------------------------------------------------
// per-labor managed flag ("autolabor/modern/labors/<n>", ival(0) != 0 managed)
// ---------------------------------------------------------------------------

static bool labor_managed(df::unit_labor l)
{
    std::stringstream key;
    key << "autolabor/modern/labors/" << int(l);
    auto item = World::GetPersistentSiteData(key.str());
    return !item.isValid() || item.ival(0) != 0;
}

static void set_labor_managed(df::unit_labor l, bool managed)
{
    std::stringstream key;
    key << "autolabor/modern/labors/" << int(l);
    auto item = World::GetPersistentSiteData(key.str(), true);
    if (item.isValid())
        item.ival(0) = managed ? 1 : 0;
}

// migrate "unmanaged" marks from the old labormanager/2.0 config once
static void migrate_old_config()
{
    auto marker = World::GetPersistentSiteData("autolabor/modern/migrated", true);
    if (!marker.isValid() || marker.ival(0) == 1)
        return;
    marker.ival(0) = 1;

    std::vector<PersistentDataItem> items;
    World::GetPersistentWorldData(&items, "labormanager/2.0/labors/", true);
    for (auto &p : items)
    {
        std::string key = p.key();
        int labor = atoi(key.substr(strlen("labormanager/2.0/labors/")).c_str());
        // old config: maximum_dwarfs in ival(2), -1 meant unmanaged
        if (labor >= 0 && labor < NUM_LABORS && p.ival(2) == -1)
            set_labor_managed((df::unit_labor)labor, false);
    }
}

// ---------------------------------------------------------------------------
// per-unit scan info
// ---------------------------------------------------------------------------

// how a unit's unmet needs (personality.needs, focus_level < 0) bias the
// role assignment. pull values are scaled 0..50.
struct NeedBias {
    int laborer_pull = 0;                  // hauling/care work satisfies the need
    int idle_pull = 0;                     // needs downtime (socialize, rest)
    std::map<df::job_skill, int> favored;  // skills that satisfy unmet needs
};

struct UnitInfo {
    df::unit *u = nullptr;
    dwarf_state state = OTHER;
    df::job_skill skill = job_skill::NONE;   // top Normal/Medical skill
    int rating = 0;
    int total = 0;
    int noble_penalty = 0;
    bool trader = false;
    bool diplomacy = false;
    bool excluded = false;                   // cannot take assignments at all
    NeedBias needs;
};

// per-unit persistent tracking for guild-hall rest rotation
struct SpecTrack {
    int busy_streak = 0;
    int rest_left = 0;
};

static JobLaborMapper *labor_mapper = nullptr;

// per-posting tracking: how long it's been on the board and which skill
// the job exercises (job pointers go bad once a posting is dead, so the
// skill is recorded up front)
struct PostingTrack {
    int32_t first_seen = 0;
    df::job_skill skill = df::job_skill::NONE;
};
static std::map<int32_t, PostingTrack> posting_track;   // posting idx -> info

// job-history skill usage, decayed per cycle so recently-used skills
// outweigh ones the fort abandoned
static std::map<df::job_skill, double> skill_usage;
static std::map<int32_t, int> unclaimed_streak;          // labor -> cycles
static std::map<int32_t, SpecTrack> unit_track;          // unit id -> tracking
static std::vector<bool> managed_labor_cache;

// status for the overlay
static int stat_laborers = 0;
static int stat_specialists = 0;
static int stat_resting = 0;
static int stat_generalists = 0;

// job-board starvation stats (since the engine was last enabled)
static int stat_starving = 0;            // live postings older than STARVE_TICKS
static int32_t stat_oldest_wait = 0;     // age of the oldest live posting
static df::job_type stat_oldest_job = df::job_type::NONE;  // oldest posting's job
static std::string stat_oldest_reaction; // custom reaction code, if any
static df::coord stat_oldest_pos;        // oldest posting's job location
static int stat_resolved = 0;            // postings that left the board
static int64_t stat_resolved_ticks = 0;  // total ticks they spent on the board
static int32_t stat_max_wait = 0;        // longest time on the board

// workshop profile constraints collected during the demand scan: a job at
// a shop with a permitted-worker list needs a listed worker assigned the
// job's labor; a nondefault skill range needs an in-range worker
struct ShopNeed {
    df::unit_labor labor = df::unit_labor::NONE;
    std::set<int32_t> permitted;         // empty = no worker restriction
    int32_t min_level = 0;
    int32_t max_level = df::skill_rating::Legendary5;
};
static std::vector<ShopNeed> shop_needs;
// unit id -> skills exercised by jobs at shops where the unit is on the
// permitted list; units on any list are dispreferred for other labors so
// they stay available for their shop's work
static std::map<int32_t, std::set<df::job_skill>> shop_reserved;
static std::set<int32_t> permitted_anywhere;

static bool has_worker(df::job *j)
{
    for (auto ref : j->general_refs)
        if (ref->getType() == df::general_ref_type::UNIT_WORKER)
            return true;
    return false;
}

// ---------------------------------------------------------------------------
// working-state persistence
// ---------------------------------------------------------------------------

// The engine's accumulated state (posting ages, skill usage history,
// starvation streaks, guild rest rotation, stats) is written to site data
// every cycle so it's included whenever the fort is saved, then restored
// on enable after a load.

static std::vector<std::string> split_str(const std::string &s, char delim)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s)
    {
        if (c == delim)
        {
            out.push_back(cur);
            cur.clear();
        }
        else
            cur += c;
    }
    out.push_back(cur);
    return out;
}

static void save_state()
{
    auto p = World::GetPersistentSiteData("autolabor/engine-state", true);
    if (!p.isValid())
        return;

    std::stringstream ss;
    ss << "stats:" << stat_resolved << ',' << stat_resolved_ticks << ','
       << stat_max_wait << ';';
    ss << "post:";
    for (auto &kv : posting_track)
        ss << kv.first << ',' << kv.second.first_seen << ','
           << (int)kv.second.skill << '|';
    ss << ";usage:";
    for (auto &kv : skill_usage)
        ss << (int)kv.first << ',' << int(kv.second * 1000) << '|';
    ss << ";streak:";
    for (auto &kv : unclaimed_streak)
        ss << kv.first << ',' << kv.second << '|';
    ss << ";track:";
    for (auto &kv : unit_track)
        ss << kv.first << ',' << kv.second.busy_streak << ','
           << kv.second.rest_left << '|';
    p.val() = ss.str();
}

static void restore_state()
{
    auto p = World::GetPersistentSiteData("autolabor/engine-state");
    if (!p.isValid())
        return;

    for (auto &sec : split_str(p.val(), ';'))
    {
        size_t colon = sec.find(':');
        if (colon == std::string::npos)
            continue;
        std::string name = sec.substr(0, colon);
        std::string body = sec.substr(colon + 1);

        if (name == "stats")
        {
            auto f = split_str(body, ',');
            if (f.size() == 3)
            {
                stat_resolved = atoi(f[0].c_str());
                stat_resolved_ticks = atoll(f[1].c_str());
                stat_max_wait = atoi(f[2].c_str());
            }
        }
        else if (name == "post")
        {
            for (auto &e : split_str(body, '|'))
            {
                auto f = split_str(e, ',');
                if (f.size() == 3)
                {
                    PostingTrack t;
                    t.first_seen = atoi(f[1].c_str());
                    t.skill = (df::job_skill)atoi(f[2].c_str());
                    posting_track[atoi(f[0].c_str())] = t;
                }
            }
        }
        else if (name == "usage")
        {
            for (auto &e : split_str(body, '|'))
            {
                auto f = split_str(e, ',');
                if (f.size() == 2)
                    skill_usage[(df::job_skill)atoi(f[0].c_str())] =
                        atoi(f[1].c_str()) / 1000.0;
            }
        }
        else if (name == "streak")
        {
            for (auto &e : split_str(body, '|'))
            {
                auto f = split_str(e, ',');
                if (f.size() == 2)
                    unclaimed_streak[atoi(f[0].c_str())] = atoi(f[1].c_str());
            }
        }
        else if (name == "track")
        {
            for (auto &e : split_str(body, '|'))
            {
                auto f = split_str(e, ',');
                if (f.size() == 3)
                {
                    SpecTrack t;
                    t.busy_streak = atoi(f[1].c_str());
                    t.rest_left = atoi(f[2].c_str());
                    unit_track[atoi(f[0].c_str())] = t;
                }
            }
        }
    }
}

void ModernEngine::enable(color_ostream &out)
{
    game->external_flag.bits.automatic_professions_disabled = false;

    if (!labor_mapper)
        labor_mapper = new JobLaborMapper();
    init_labor_to_skill();
    migrate_old_config();

    managed_labor_cache.assign(NUM_LABORS, true);
    FOR_ENUM_ITEMS(unit_labor, l)
        if (l != unit_labor::NONE)
            managed_labor_cache[l] = labor_managed(l);

    wdm->reset();
    posting_track.clear();
    skill_usage.clear();
    unclaimed_streak.clear();
    unit_track.clear();
    cycle_timestamp = 0;
    stat_starving = 0;
    stat_oldest_wait = 0;
    stat_oldest_job = df::job_type::NONE;
    stat_oldest_reaction.clear();
    stat_resolved = 0;
    stat_resolved_ticks = 0;
    stat_max_wait = 0;
    restore_state();
    initialized = true;

    out << "Enabling labormanager (work detail mode)." << std::endl;
}

void ModernEngine::disable(color_ostream &out)
{
    initialized = false;
    wdm->shutdown();
    out << "Disabling labormanager." << std::endl;
}

void ModernEngine::map_unload()
{
    initialized = false;
    posting_track.clear();
    skill_usage.clear();
    unclaimed_streak.clear();
    unit_track.clear();
    stat_starving = 0;
    stat_oldest_wait = 0;
    stat_oldest_job = df::job_type::NONE;
    stat_oldest_reaction.clear();
    stat_resolved = 0;
    stat_resolved_ticks = 0;
    stat_max_wait = 0;
}

int ModernEngine::starving_jobs()
{
    return initialized ? stat_starving : 0;
}

df::job_type ModernEngine::oldest_starving_job()
{
    return initialized && stat_starving > 0 ? stat_oldest_job
                                            : df::job_type::NONE;
}

int32_t ModernEngine::oldest_starving_wait()
{
    return initialized && stat_starving > 0 ? stat_oldest_wait : 0;
}

df::coord ModernEngine::oldest_starving_pos()
{
    return initialized && stat_starving > 0 ? stat_oldest_pos
                                            : df::coord();
}

// display name for the oldest starving job: for custom reactions, resolve
// the job's reaction code to the reaction's own name
static std::string oldest_starving_name()
{
    if (stat_oldest_job == df::job_type::CustomReaction &&
            !stat_oldest_reaction.empty())
        for (auto r : df::reaction::get_vector())
            if (r->code == stat_oldest_reaction && !r->name.empty())
                return r->name;
    const char *cap = ENUM_ATTR(job_type, caption, stat_oldest_job);
    return cap ? cap : ENUM_KEY_STR(job_type, stat_oldest_job);
}

std::string ModernEngine::oldest_starving_name()
{
    return initialized && stat_starving > 0 ? ::oldest_starving_name() : "";
}

std::string ModernEngine::status_line()
{
    std::stringstream ss;
    ss << stat_laborers << " laborers, " << stat_specialists << " specialists";
    if (stat_resting > 0)
        ss << " (" << stat_resting << " on guild rest)";
    ss << ", " << stat_generalists << " generalists";
    return ss.str();
}

// ---------------------------------------------------------------------------
// demand scan
// ---------------------------------------------------------------------------

static void reconcile(WorkDetailManager *wdm, df::work_detail *wd,
    const std::set<int32_t> &desired);
static df::work_detail_icon_type skill_icon(df::job_skill skill);

static void scan_demand(std::map<df::unit_labor, int> &backlog)
{
    backlog.clear();
    stat_starving = 0;
    stat_oldest_wait = 0;
    stat_oldest_job = df::job_type::NONE;
    stat_oldest_reaction.clear();
    shop_needs.clear();
    shop_reserved.clear();
    permitted_anywhere.clear();

    // every unit on a workshop's permitted list is reserved for that
    // shop's work; the labors each shop actually needs come from the
    // postings below
    for (auto b : world->buildings.all)
    {
        df::workshop_profile *prof = b->getWorkshopProfile();
        if (prof && !prof->permitted_workers.empty())
            permitted_anywhere.insert(prof->permitted_workers.begin(),
                prof->permitted_workers.end());
    }

    std::set<int32_t> live_postings;

    for (auto jp : world->jobs.postings)
    {
        if (jp->flags.bits.dead || !jp->job)
            continue;
        df::job *j = jp->job;
        if (j->flags.bits.suspend || j->flags.bits.item_lost)
            continue;
        if (has_worker(j))
            continue;

        live_postings.insert(jp->idx);

        df::unit_labor labor = labor_mapper->find_job_labor(j);

        if (!posting_track.count(jp->idx))
        {
            PostingTrack t;
            t.first_seen = world->frame_counter;
            if (labor >= 0 && labor < NUM_LABORS)
                t.skill = labor_to_skill[labor];
            posting_track[jp->idx] = t;
        }

        // starvation stats: how long has this posting been on the board
        int32_t age = world->frame_counter - posting_track[jp->idx].first_seen;
        if (age > stat_oldest_wait)
        {
            stat_oldest_wait = age;
            stat_oldest_job = j->job_type;
            stat_oldest_reaction = j->reaction_name;
            stat_oldest_pos = j->pos;
        }
        if (age >= STARVE_TICKS)
            stat_starving++;

        if (labor < 0 || labor >= NUM_LABORS)
            continue;
        backlog[labor]++;

        // a job at a shop with a permitted-worker list or a nondefault
        // skill range needs a qualifying worker holding the labor
        if (df::building *bld = Job::getHolder(j))
        {
            df::workshop_profile *prof = bld->getWorkshopProfile();
            if (prof && (!prof->permitted_workers.empty() ||
                prof->min_level > 0 ||
                prof->max_level < df::skill_rating::Legendary5))
            {
                ShopNeed need;
                need.labor = labor;
                need.permitted.insert(prof->permitted_workers.begin(),
                    prof->permitted_workers.end());
                need.min_level = prof->min_level;
                need.max_level = prof->max_level;
                shop_needs.push_back(need);
                if (labor_to_skill[labor] != job_skill::NONE)
                    for (auto uid : prof->permitted_workers)
                        shop_reserved[uid].insert(labor_to_skill[labor]);
            }
        }
    }

    // postings that left the board (assigned, cancelled, or suspended):
    // record how long they spent on the board and the skill they exercised
    for (auto it = posting_track.begin(); it != posting_track.end();)
    {
        if (!live_postings.count(it->first))
        {
            int32_t wait = world->frame_counter - it->second.first_seen;
            stat_resolved++;
            stat_resolved_ticks += wait;
            stat_max_wait = std::max(stat_max_wait, wait);
            if (it->second.skill != df::job_skill::NONE)
                skill_usage[it->second.skill] += 1.0;
            it = posting_track.erase(it);
        }
        else
            ++it;
    }

    // decay usage so recently-used skills outweigh abandoned ones
    for (auto &kv : skill_usage)
        kv.second *= 0.995;

    // stockpile job counters catch hauling demand that hasn't spawned jobs yet
    backlog[unit_labor::HAUL_STONE]     += world->stockpile.num_jobs[1];
    backlog[unit_labor::HAUL_WOOD]      += world->stockpile.num_jobs[2];
    backlog[unit_labor::HAUL_ITEM]      += world->stockpile.num_jobs[3];
    backlog[unit_labor::HAUL_ITEM]      += world->stockpile.num_jobs[4];
    backlog[unit_labor::HAUL_BODY]      += world->stockpile.num_jobs[5];
    backlog[unit_labor::HAUL_FOOD]      += world->stockpile.num_jobs[6];
    backlog[unit_labor::HAUL_REFUSE]    += world->stockpile.num_jobs[7];
    backlog[unit_labor::HAUL_FURNITURE] += world->stockpile.num_jobs[8];
    backlog[unit_labor::HAUL_ANIMALS]   += world->stockpile.num_jobs[9];

    for (auto v : world->vehicles.all)
        if (v->route_id != -1)
            backlog[unit_labor::HANDLE_VEHICLES]++;

    // track how many consecutive cycles each labor has had unclaimed work
    FOR_ENUM_ITEMS(unit_labor, l)
    {
        if (l == unit_labor::NONE)
            continue;
        if (backlog[l] > 0)
            unclaimed_streak[l]++;
        else
            unclaimed_streak[l] = 0;
    }
}

// ---------------------------------------------------------------------------
// guild halls
// ---------------------------------------------------------------------------

static std::set<df::profession> guilded_professions()
{
    std::set<df::profession> profs;

    auto site = df::world_site::find(plotinfo->site_id);
    if (!site)
        return profs;

    for (auto ab : site->buildings)
    {
        if (auto gh = virtual_cast<df::abstract_building_guildhallst>(ab))
        {
            df::profession p = gh->contents.profession;
            if (p != profession::NONE)
                profs.insert(p);
        }
    }

    return profs;
}

// does this skill's profession (or an ancestor profession) have a guild hall?
static bool is_guilded(df::job_skill skill, const std::set<df::profession> &guilded)
{
    df::profession p = (df::profession)ENUM_ATTR(job_skill, profession, skill);
    while (p != profession::NONE)
    {
        if (guilded.count(p))
            return true;
        p = (df::profession)ENUM_ATTR(profession, parent, p);
    }
    return false;
}

// ---------------------------------------------------------------------------
// tools
// ---------------------------------------------------------------------------

static void count_tools(int tool_count[TOOLS_MAX])
{
    for (int e = 0; e < TOOLS_MAX; e++)
        tool_count[e] = 0;

    df::item_flags bad_flags;
    bad_flags.whole = 0;
#define F(x) bad_flags.bits.x = true;
    F(dump); F(forbid); F(garbage_collect);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction);
#undef F

    for (auto item : world->items.other.IN_PLAY)
    {
        if (item->flags.whole & bad_flags.whole)
            continue;
        if (!item->isWeapon())
            continue;

        df::itemdef_weaponst* weapondef = ((df::item_weaponst*)item)->subtype;
        if ((df::job_skill)weapondef->skill_melee == df::job_skill::MINING)
            tool_count[TOOL_PICK]++;
        else if ((df::job_skill)weapondef->skill_melee == df::job_skill::AXE)
            tool_count[TOOL_AXE]++;
        else if ((df::job_skill)weapondef->skill_ranged == df::job_skill::CROSSBOW)
            tool_count[TOOL_CROSSBOW]++;
    }
}

// ---------------------------------------------------------------------------
// needs
// ---------------------------------------------------------------------------

static int rating_in_skill(df::unit *u, df::job_skill skill)
{
    if (u->status.current_soul)
        for (auto s : u->status.current_soul->skills)
            if (s->id == skill)
                return s->rating;
    return 0;
}

// craft labors satisfy the BeCreative / CraftObject / ThinkAbstractly needs
static const df::unit_labor craft_labors[] = {
    unit_labor::WOOD_CRAFT, unit_labor::STONE_CRAFT, unit_labor::BONE_CARVE,
    unit_labor::METAL_CRAFT, unit_labor::GLASSMAKER, unit_labor::POTTERY,
    unit_labor::BOWYER, unit_labor::CLOTHESMAKER, unit_labor::WEAVER,
    unit_labor::LEATHER, unit_labor::CUT_GEM, unit_labor::ENCRUST_GEM,
    unit_labor::PAPERMAKING, unit_labor::BOOKBINDING, unit_labor::WAX_WORKING,
    unit_labor::EXTRACT_STRAND, unit_labor::SPINNER, unit_labor::DYER,
};

// labors whose products carry a quality modifier that scales with the
// worker's skill -- skilled specialists matter most for these
static const df::unit_labor quality_labors[] = {
    unit_labor::CARPENTER, unit_labor::STONECUTTER, unit_labor::STONE_CARVER,
    unit_labor::ENGRAVER, unit_labor::MASON, unit_labor::MECHANIC,
    unit_labor::WOOD_CRAFT, unit_labor::STONE_CRAFT, unit_labor::BONE_CARVE,
    unit_labor::METAL_CRAFT, unit_labor::FORGE_WEAPON, unit_labor::FORGE_ARMOR,
    unit_labor::FORGE_FURNITURE, unit_labor::GLASSMAKER, unit_labor::POTTERY,
    unit_labor::GLAZING, unit_labor::BOWYER, unit_labor::CLOTHESMAKER,
    unit_labor::LEATHER, unit_labor::CUT_GEM, unit_labor::ENCRUST_GEM,
    unit_labor::SIEGECRAFT, unit_labor::COOK, unit_labor::PAPERMAKING,
    unit_labor::BOOKBINDING,
};

// labors whose skills a strange mood can raise to legendary
static const df::unit_labor moodable_labors[] = {
    unit_labor::CARPENTER, unit_labor::STONECUTTER, unit_labor::STONE_CARVER,
    unit_labor::WOOD_CRAFT, unit_labor::BONE_CARVE, unit_labor::METAL_CRAFT,
    unit_labor::FORGE_WEAPON, unit_labor::FORGE_ARMOR,
    unit_labor::FORGE_FURNITURE, unit_labor::BOWYER, unit_labor::MECHANIC,
    unit_labor::MASON, unit_labor::CLOTHESMAKER, unit_labor::WEAVER,
    unit_labor::LEATHER, unit_labor::TANNER, unit_labor::CUT_GEM,
    unit_labor::ENCRUST_GEM, unit_labor::GLASSMAKER, unit_labor::POTTERY,
    unit_labor::GLAZING, unit_labor::WAX_WORKING, unit_labor::SMELT,
    unit_labor::SIEGECRAFT,
};

static bool is_labor_skill(const df::unit_labor *labors, size_t count,
    df::job_skill skill)
{
    for (size_t i = 0; i < count; i++)
        if (labor_to_skill[labors[i]] == skill)
            return true;
    return false;
}

// products of these skills carry skill-scaled quality modifiers
static bool is_quality_skill(df::job_skill skill)
{
    return is_labor_skill(quality_labors,
        sizeof(quality_labors)/sizeof(df::unit_labor), skill);
}

static bool has_moodable_skill(df::unit *u)
{
    for (auto l : moodable_labors)
        if (rating_in_skill(u, labor_to_skill[l]) > 0)
            return true;
    return false;
}

// frequently-used skills are worth specializing more aggressively
static int usage_bonus(df::job_skill skill)
{
    auto it = skill_usage.find(skill);
    if (it == skill_usage.end())
        return 0;
    return int(std::min(it->second, 20.0) * 5);   // 0..100
}

static NeedBias scan_needs(df::unit *u)
{
    NeedBias bias;
    if (!u->status.current_soul)
        return bias;

    for (auto need : u->status.current_soul->personality.needs)
    {
        // focus_level runs -999 (desperate) to 400 (satisfied)
        if (need->focus_level >= 0)
            continue;
        int urgency = std::min(-need->focus_level, 400) / 8;  // 0..50
        if (!urgency)
            continue;

        switch (need->id)
        {
        // staying occupied, acquiring things (stockpiling, hauling to the
        // depot), and helping (patients, prisoners, wounded) are all
        // satisfied by unskilled laborer work
        case need_type::StayOccupied:
        case need_type::AcquireObject:
            bias.laborer_pull += urgency;
            break;
        // helping: patient care is laborer work, and leading guildhall
        // demonstrations needs idle time
        case need_type::HelpSomebody:
            bias.laborer_pull += urgency;
            bias.idle_pull += urgency / 2;
            break;
        // social needs are satisfied by free time; keep these units out of
        // the restrictive pools so they can idle and socialize, and let
        // guilded ones earn rest (and teach) sooner
        case need_type::Socialize:
        case need_type::MakeMerry:
        case need_type::TakeItEasy:
        case need_type::BeWithFamily:
        case need_type::BeWithFriends:
        case need_type::HearEloquence:
        case need_type::LearnSomething:  // watching demos is guild idle time
            bias.idle_pull += urgency;
            break;
        // wandering satisfied by fishing, hunting, plant gathering
        case need_type::Wander:
            bias.favored[labor_to_skill[unit_labor::FISH]] += urgency;
            bias.favored[labor_to_skill[unit_labor::HUNT]] += urgency;
            bias.favored[labor_to_skill[unit_labor::HERBALIST]] += urgency;
            break;
        // excitement: catching vermin, danger (hunting); performing and
        // socializing are downtime activities
        case need_type::Excitement:
            bias.favored[labor_to_skill[unit_labor::TRAPPER]] += urgency;
            bias.favored[labor_to_skill[unit_labor::HUNT]] += urgency;
            bias.idle_pull += urgency / 2;
            break;
        // creativity: artwork and crafts
        case need_type::BeCreative:
        case need_type::CraftObject:
        case need_type::ThinkAbstractly:
            for (auto l : craft_labors)
                bias.favored[labor_to_skill[l]] += urgency;
            break;
        default:
            break;
        }
    }
    return bias;
}

// ---------------------------------------------------------------------------
// main cycle
// ---------------------------------------------------------------------------

void ModernEngine::update(color_ostream &out)
{
    if (!world || !world->map.block_index || !initialized)
        return;
    if (world->frame_counter - cycle_timestamp < CYCLE_TICKS)
        return;
    cycle_timestamp = world->frame_counter;

    auto bscan = scan_buildings();
    int tool_count[TOOLS_MAX];
    count_tools(tool_count);

    std::map<df::unit_labor, int> backlog;
    scan_demand(backlog);

    auto guilded = guilded_professions();

    // --- collect citizens ---------------------------------------------------

    std::vector<UnitInfo> units;
    for (auto cre : world->units.active)
    {
        if (!is_assignable(cre) || cre->burrows.size() > 0)
            continue;

        UnitInfo info;
        info.u = cre;
        info.state = get_dwarf_state(cre);
        info.rating = top_skill(cre, &info.skill);
        info.total = total_skill(cre);

        auto pos = unit_position_info(cre);
        info.noble_penalty = pos.noble_penalty;
        info.trader = pos.trader && bscan.trader_requested;
        info.diplomacy = in_diplomacy_meeting(cre);

        info.excluded = info.diplomacy || info.trader ||
            info.state == CHILD || info.state == MILITARY ||
            info.state == OTHER || cre->status2.limbs_grasp_count == 0;

        info.needs = scan_needs(cre);

        units.push_back(info);
    }

    if (units.empty())
        return;

    int n = units.size();
    std::map<int32_t, int> unit_idx;
    for (int i = 0; i < n; i++)
        unit_idx[units[i].u->id] = i;

    // --- priority weights from the balance slider ---------------------------

    double starve_w = double(NUM_BALANCE_STOPS - 1 - balance()) / (NUM_BALANCE_STOPS - 1);
    double skill_w  = double(balance()) / (NUM_BALANCE_STOPS - 1);

    // --- laborer pool sizing ------------------------------------------------

    int unskilled_backlog = 0;
    FOR_ENUM_ITEMS(unit_labor, l)
    {
        if (l == unit_labor::NONE || !is_unskilled(l))
            continue;
        if (!managed_labor_cache[l])
            continue;
        int count = backlog[l];
        if (unclaimed_streak[l] >= STARVE_CYCLES)
            count *= 2;   // starving labors get doubled weight
        unskilled_backlog += count;
    }

    int laborer_target = 1 + unskilled_backlog / 2 +
        int(std::round(n * 0.25 * starve_w));
    laborer_target = std::min(laborer_target, n);

    // --- pick laborers ------------------------------------------------------
    // pool score: lower is a better laborer; skilled dwarves are protected in
    // proportion to the skill-priority weight

    std::vector<int> order;
    std::map<int, int> pool_score;
    for (int i = 0; i < n; i++)
    {
        if (units[i].excluded)
            continue;
        int score = 0;
        switch (units[i].state) {
        case IDLE:      score = 0;    break;
        case BUSY:      score = 200;  break;
        case EXCLUSIVE: score = 400;  break;
        default:        score = 800;  break;
        }
        score += units[i].total * 10;
        // skilled specialists are protected in proportion to the skill
        // weight; quality-affecting and frequently-used skills count extra
        // since losing their time costs the fort more
        int prot = units[i].rating * 40;
        if (is_quality_skill(units[i].skill))
            prot = prot * 3 / 2;
        prot += usage_bonus(units[i].skill);
        score += int(prot * skill_w * 4);
        score += units[i].noble_penalty / 100;
        // units whose unmet needs are satisfied by hauling/patient care get
        // pulled in; units needing social downtime get pushed out
        score -= units[i].needs.laborer_pull * 4;
        score += units[i].needs.idle_pull * 4;
        // permitted workers stay available for their shop's jobs rather
        // than getting drafted into the laborer pool
        if (permitted_anywhere.count(units[i].u->id))
            score += 150;
        pool_score[i] = score;
        order.push_back(i);
    }
    std::sort(order.begin(), order.end(),
        [&](int a, int b) { return pool_score[a] < pool_score[b]; });

    std::set<int32_t> laborer_ids;
    for (int i = 0; i < (int)order.size() && (int)laborer_ids.size() < laborer_target; i++)
        laborer_ids.insert(units[order[i]].u->id);

    // --- specialists --------------------------------------------------------
    // every remaining citizen with a usable skill gets pinned to that skill's
    // detail; guilded specialists rotate through rest periods

    // skills that map to a managed labor AND are actually used by the fort
    // (per posting history); a citizen whose top skill has no labor (e.g.
    // teaching) or whose skill is never exercised (gelders, animal
    // dissectors in most forts) stays a generalist rather than getting a
    // reserved detail
    std::set<df::job_skill> viable_skills;
    FOR_ENUM_ITEMS(unit_labor, l)
        if (l != unit_labor::NONE && managed_labor_cache[l] && !is_unskilled(l) &&
            skill_usage[labor_to_skill[l]] >= 0.5)
            viable_skills.insert(labor_to_skill[l]);

    std::map<df::job_skill, std::vector<int32_t>> skill_members; // skill -> unit ids
    std::set<int32_t> specialist_ids;
    stat_resting = 0;

    auto skill_allowed = [&](df::job_skill s) {
        if (s == labor_to_skill[unit_labor::FISH])
            return config_flag(CF_ALLOW_FISHING) && bscan.has_fishery;
        if (s == labor_to_skill[unit_labor::HUNT])
            return config_flag(CF_ALLOW_HUNTING) && bscan.has_butchers;
        return true;
    };

    for (int i = 0; i < n; i++)
    {
        UnitInfo &info = units[i];
        if (info.excluded || laborer_ids.count(info.u->id))
            continue;

        // pick the specialty. normally the unit's top skill; need-favored
        // skills and fort usage stats can pull the pick elsewhere.
        // mood-shaping: a unit that can still have a strange mood and has
        // started a moodable craft gets steered toward moodable skills --
        // preferably valuable ones -- so a mood pays out in a skill the
        // fort wants. needs a caste STRANGE_MOODS token and no spent mood.
        bool mood_prot =
            Units::casteFlagSet(info.u->race, info.u->caste,
                df::caste_raw_flags::STRANGE_MOODS) &&
            !info.u->flags1.bits.had_mood && has_moodable_skill(info.u);

        df::job_skill chosen = job_skill::NONE;
        int best_score = -1;
        auto consider = [&](df::job_skill want, int bonus) {
            if (want == job_skill::NONE || !viable_skills.count(want) ||
                !skill_allowed(want))
                return;
            int r = rating_in_skill(info.u, want);
            int score = r * 100 + bonus + usage_bonus(want) +
                (is_quality_skill(want) ? r * 20 : 0);
            // permitted workers are dispreferred for specialties their
            // shop doesn't need and preferred for the ones it does
            if (permitted_anywhere.count(info.u->id))
                score += shop_reserved[info.u->id].count(want) ? 150 : -150;
            if (score > best_score)
            {
                best_score = score;
                chosen = want;
            }
        };

        if (mood_prot)
        {
            // candidates restricted to moodable skills; quality-affecting
            // skills get a large bonus so the pick trends toward valuable
            // mood outcomes, and need-favored moodable skills still count
            for (auto l : moodable_labors)
            {
                df::job_skill want = labor_to_skill[l];
                int bonus = is_quality_skill(want) ? 300 : 100;
                auto it = info.needs.favored.find(want);
                if (it != info.needs.favored.end())
                    bonus += it->second * 20;
                consider(want, bonus);
            }
        }
        else
        {
            if (info.skill != job_skill::NONE && info.rating > 0)
                consider(info.skill, 0);
            for (auto &kv : info.needs.favored)
                consider(kv.first, kv.second * 20);
        }
        if (chosen == job_skill::NONE)
            continue;

        specialist_ids.insert(info.u->id);

        // guild-hall rest rotation; units with unmet social/helping needs
        // earn rest (and the chance to teach or learn) sooner
        SpecTrack &track = unit_track[info.u->id];
        bool guilded_unit = is_guilded(chosen, guilded);
        if (guilded_unit)
        {
            int busy_limit = std::max(4, BUSY_LIMIT - info.needs.idle_pull / 5);
            if (info.u->job.current_job)
                track.busy_streak++;
            else
                track.busy_streak = 0;

            if (track.rest_left > 0)
            {
                track.rest_left--;
                stat_resting++;
                continue;   // resting: keep flag, join no skill detail
            }
            if (track.busy_streak >= busy_limit)
            {
                int reserve = idle_reserve();
                track.rest_left = std::max(1,
                    busy_limit * reserve / std::max(1, 100 - reserve));
                track.busy_streak = 0;
                stat_resting++;
                continue;
            }
        }

        skill_members[chosen].push_back(info.u->id);
    }

    // --- apprentices --------------------------------------------------------
    // sustained backlog on a skilled labor: pull in unskilled generalists so
    // the job gets done and they train up

    for (int i = 0; i < n; i++)
    {
        UnitInfo &info = units[i];
        if (info.excluded || laborer_ids.count(info.u->id) ||
            specialist_ids.count(info.u->id))
            continue;
        if (info.state != IDLE && info.state != BUSY)
            continue;

        // find the most-starving skilled labor this unit could learn
        df::job_skill want = job_skill::NONE;
        int worst_streak = STARVE_CYCLES;
        FOR_ENUM_ITEMS(unit_labor, labor)
        {
            if (labor == unit_labor::NONE || !managed_labor_cache[labor] ||
                is_unskilled(labor))
                continue;
            if (unclaimed_streak[labor] < worst_streak)
                continue;
            df::job_skill skill = labor_to_skill[labor];
            tools_enum tool = labor_tool(labor);
            if (tool != TOOL_NONE &&
                (int)skill_members[skill].size() >= tool_count[tool])
                continue;
            if (!Units::isValidLabor(info.u, labor))
                continue;
            want = skill;
            worst_streak = unclaimed_streak[labor];
        }
        if (want != job_skill::NONE)
        {
            skill_members[want].push_back(info.u->id);
            specialist_ids.insert(info.u->id);  // restrict them to the detail
        }
    }

    // --- workshop restrictions ----------------------------------------------
    // a posting at a shop with a permitted-worker list needs a listed worker
    // holding the job's labor; a nondefault skill range needs an in-range
    // holder. only managed skilled labors need enforcement -- unskilled and
    // unmanaged labors stay available to everyone via the EverybodyDoesThis
    // details

    for (auto &need : shop_needs)
    {
        df::unit_labor labor = need.labor;
        if (labor < 0 || labor >= NUM_LABORS || !managed_labor_cache[labor] ||
            is_unskilled(labor))
            continue;
        df::job_skill skill = labor_to_skill[labor];
        if (skill == job_skill::NONE)
            continue;

        auto &members = skill_members[skill];
        auto is_member = [&](int32_t uid) {
            return std::find(members.begin(), members.end(), uid) !=
                members.end();
        };
        auto in_range = [&](int i) {
            int r = rating_in_skill(units[i].u, skill);
            return r >= need.min_level && r <= need.max_level;
        };

        bool permit_ok = need.permitted.empty();
        bool range_ok = need.min_level <= 0 &&
            need.max_level >= df::skill_rating::Legendary5;
        for (auto uid : members)
        {
            auto it = unit_idx.find(uid);
            if (it == unit_idx.end())
                continue;
            if (!permit_ok && need.permitted.count(uid))
                permit_ok = true;
            if (!range_ok && in_range(it->second))
                range_ok = true;
        }
        if (permit_ok && range_ok)
            continue;

        // pick the best qualifying citizen; satisfy each constraint with a
        // unit that also covers the other one when possible
        auto satisfy = [&](bool must_permit, bool must_range) {
            int best = -1, best_i = -1;
            for (int i = 0; i < n; i++)
            {
                if (units[i].excluded || is_member(units[i].u->id) ||
                    !Units::isValidLabor(units[i].u, labor))
                    continue;
                if (must_permit && !need.permitted.count(units[i].u->id))
                    continue;
                if (must_range && !in_range(i))
                    continue;
                int score = rating_in_skill(units[i].u, skill) * 100;
                if (need.permitted.count(units[i].u->id))
                    score += 500;
                if (in_range(i))
                    score += 300;
                if (score > best)
                {
                    best = score;
                    best_i = i;
                }
            }
            if (best_i < 0)
                return;
            int32_t uid = units[best_i].u->id;
            members.push_back(uid);
            specialist_ids.insert(uid);
            // shop work takes priority over the laborer pool
            laborer_ids.erase(uid);
        };

        if (!permit_ok)
            satisfy(true, false);
        // the permitted pick may have covered the range too
        if (!range_ok)
        {
            for (auto uid : members)
            {
                auto it = unit_idx.find(uid);
                if (it != unit_idx.end() && in_range(it->second))
                {
                    range_ok = true;
                    break;
                }
            }
            if (!range_ok)
                satisfy(false, true);
        }
    }

    // --- apply --------------------------------------------------------------

    std::set<int32_t> specialized_ids = laborer_ids;
    specialized_ids.insert(specialist_ids.begin(), specialist_ids.end());

    // Laborers: unskilled labors for the pool (Everybody so generalists keep
    // them too; membership is what matters for flag-restricted units)
    auto *wd_laborers = wdm->ensure_detail(LABORER_DETAIL,
        work_detail_icon_type::HAULERS, work_detail_mode::EverybodyDoesThis);
    FOR_ENUM_ITEMS(unit_labor, l)
        if (l != unit_labor::NONE && is_unskilled(l) && managed_labor_cache[l])
            wdm->cover(wd_laborers, l, true);
    reconcile(wdm, wd_laborers, laborer_ids);

    // Care: safety labors for all specialized units
    auto *wd_care = wdm->ensure_detail(CARE_DETAIL,
        work_detail_icon_type::ORDERLIES, work_detail_mode::EverybodyDoesThis);
    FOR_ENUM_ITEMS(unit_labor, l)
        if (l != unit_labor::NONE && is_care_labor(l) && managed_labor_cache[l])
            wdm->cover(wd_care, l, true);
    reconcile(wdm, wd_care, specialized_ids);

    // per-skill details
    std::set<std::string> live_skill_details;
    for (auto &kv : skill_members)
    {
        df::job_skill skill = kv.first;
        std::set<int32_t> members(kv.second.begin(), kv.second.end());
        if (members.empty())
            continue;

        const char *cap = ENUM_ATTR(job_skill, caption_noun, skill);
        std::string name = SKILL_PREFIX + (cap ? cap : ENUM_KEY_STR(job_skill, skill));
        live_skill_details.insert(name);

        auto *wd = wdm->ensure_detail(name, skill_icon(skill),
            work_detail_mode::OnlySelectedDoesThis);
        FOR_ENUM_ITEMS(unit_labor, l)
            if (l != unit_labor::NONE && managed_labor_cache[l] &&
                labor_to_skill[l] == skill)
                wdm->cover(wd, l, true);
        reconcile(wdm, wd, members);
    }

    // delete skill details that no longer have members
    for (auto wd : wdm->managed_details())
        if (wd->name.compare(0, SKILL_PREFIX.size(), SKILL_PREFIX) == 0 &&
            !live_skill_details.count(wd->name))
            wdm->delete_detail(wd);

    // specialization flags
    for (int i = 0; i < n; i++)
        wdm->set_specialized(units[i].u, specialized_ids.count(units[i].u->id));
    // also clear flags on units no longer assignable (burrows, gone military)
    for (int32_t id : std::set<int32_t>(wdm->flagged_units()))
        if (auto u = df::unit::find(id))
            if (!is_assignable(u) || u->burrows.size() > 0)
                wdm->set_specialized(u, false);

    wdm->commit();

    save_state();

    stat_laborers = laborer_ids.size();
    stat_specialists = specialist_ids.size();
    stat_generalists = n - stat_laborers - stat_specialists;

    DEBUG(modern_cycle, out).print(
        "cycle: {} units, {} laborers, {} specialists ({} resting), backlog={}\n",
        n, stat_laborers, stat_specialists, stat_resting, unskilled_backlog);
}

// reconcile a detail's membership with the desired id set
static void reconcile(WorkDetailManager *wdm, df::work_detail *wd,
    const std::set<int32_t> &desired)
{
    // remove stale members
    for (auto it = wd->assigned_units.begin(); it != wd->assigned_units.end();)
    {
        if (!desired.count(*it))
        {
            int32_t id = *it;
            it = wd->assigned_units.erase(it);
            if (auto u = df::unit::find(id))
                wdm->touch(u);
        }
        else
            ++it;
    }
    // add new members
    for (int32_t id : desired)
        wdm->set_membership(wd, id, true);
}

static df::work_detail_icon_type skill_icon(df::job_skill skill)
{
    df::unit_labor l = (df::unit_labor)ENUM_ATTR(job_skill, labor, skill);
    switch (l) {
    case unit_labor::MINE:      return work_detail_icon_type::MINERS;
    case unit_labor::CUTWOOD:   return work_detail_icon_type::WOODCUTTERS;
    case unit_labor::HUNT:      return work_detail_icon_type::HUNTERS;
    case unit_labor::PLANT:     return work_detail_icon_type::PLANTERS;
    case unit_labor::FISH:      return work_detail_icon_type::FISHERMEN;
    case unit_labor::HERBALIST: return work_detail_icon_type::PLANT_GATHERERS;
    case unit_labor::SIEGEOPERATE: return work_detail_icon_type::SIEGE_OPERATORS;
    default:                    return work_detail_icon_type::CUSTOM_1;
    }
}

// ---------------------------------------------------------------------------
// command dialect
// ---------------------------------------------------------------------------

bool ModernEngine::command(color_ostream &out, std::vector<std::string> &parameters)
{
    if (parameters.empty())
        return false;

    if (managed_labor_cache.empty())
    {
        managed_labor_cache.assign(NUM_LABORS, true);
        FOR_ENUM_ITEMS(unit_labor, l)
            if (l != unit_labor::NONE)
                managed_labor_cache[l] = labor_managed(l);
    }

    if (parameters.size() == 2 && parameters[0] == "balance")
    {
        bool ok = false;
        for (int i = 0; i < NUM_BALANCE_STOPS; i++)
            if (parameters[1] == balance_stop_name(i))
            {
                store_balance(i);
                ok = true;
            }
        if (!ok)
        {
            int v = atoi(parameters[1].c_str());
            if (v >= 0 && v < NUM_BALANCE_STOPS)
            {
                store_balance(v);
                ok = true;
            }
        }
        if (!ok)
        {
            out.printerr("Syntax: labormanager balance <0-{}|{}|...|{}>\n",
                NUM_BALANCE_STOPS - 1, balance_stop_name(0),
                balance_stop_name(NUM_BALANCE_STOPS - 1));
            return true;
        }
        out << "Balance: " << balance_stop_name(balance()) << std::endl;
        return true;
    }
    else if (parameters.size() == 2 && parameters[0] == "idle-reserve")
    {
        int v = atoi(parameters[1].c_str());
        if (v < 0 || v > 100)
        {
            out.printerr("Syntax: labormanager idle-reserve <0-100>\n");
            return true;
        }
        store_idle_reserve(v);
        out << "Idle reserve: " << v << "%" << std::endl;
        return true;
    }
    else if (parameters.size() == 3 && parameters[0] == "labor")
    {
        df::unit_labor labor = unit_labor::NONE;
        FOR_ENUM_ITEMS(unit_labor, test)
            if (parameters[1] == ENUM_KEY_STR(unit_labor, test))
                labor = test;
        if (labor == unit_labor::NONE)
        {
            out.printerr("Could not find labor {}.\n", parameters[1]);
            return true;
        }
        bool managed = parameters[2] == "managed";
        if (!managed && parameters[2] != "unmanaged")
        {
            out.printerr("Syntax: labormanager labor <labor> managed|unmanaged\n");
            return true;
        }
        set_labor_managed(labor, managed);
        managed_labor_cache[labor] = managed;
        wdm->touch_all();
        wdm->commit();
        out << ENUM_KEY_STR(unit_labor, labor) << ": "
            << (managed ? "managed" : "unmanaged") << std::endl;
        return true;
    }
    else if (parameters.size() == 1 &&
             (parameters[0] == "allow-fishing" || parameters[0] == "forbid-fishing"))
    {
        set_config_flag(CF_ALLOW_FISHING, parameters[0] == "allow-fishing");
        return true;
    }
    else if (parameters.size() == 1 &&
             (parameters[0] == "allow-hunting" || parameters[0] == "forbid-hunting"))
    {
        set_config_flag(CF_ALLOW_HUNTING, parameters[0] == "allow-hunting");
        return true;
    }
    else if (parameters.size() == 1 && (parameters[0] == "list" || parameters[0] == "status"))
    {
        out << "Balance: " << balance_stop_name(balance())
            << ", idle reserve " << idle_reserve() << "%" << std::endl;
        out << status_line() << std::endl;
        out << "job board: " << stat_resolved << " postings resolved";
        if (stat_resolved > 0)
            out << ", avg wait " << (stat_resolved_ticks / stat_resolved)
                << " ticks, longest " << stat_max_wait << " ticks";
        if (stat_starving > 0)
            out << "; " << stat_starving << " starving (> " << STARVE_TICKS
                << " ticks, oldest " << stat_oldest_wait << ")";
        out << std::endl;
        // most-used skills drive specialization priorities
        std::vector<std::pair<df::job_skill, double>> top_used;
        for (auto &kv : skill_usage)
            if (kv.second >= 0.5)
                top_used.push_back(kv);
        std::sort(top_used.begin(), top_used.end(),
            [](const std::pair<df::job_skill, double> &a,
               const std::pair<df::job_skill, double> &b) {
                return a.second > b.second;
            });
        if (!top_used.empty())
        {
            out << "top skills in use:";
            for (size_t i = 0; i < top_used.size() && i < 5; i++)
                out << " " << ENUM_KEY_STR(job_skill, top_used[i].first)
                    << "(" << int(top_used[i].second) << ")";
            out << std::endl;
        }
        if (parameters[0] == "list")
        {
            FOR_ENUM_ITEMS(unit_labor, l)
            {
                if (l == unit_labor::NONE)
                    continue;
                out << ENUM_KEY_STR(unit_labor, l) << ": "
                    << (managed_labor_cache[l] ? "managed" : "unmanaged");
                if (unclaimed_streak[l] >= STARVE_CYCLES)
                    out << " (starved " << unclaimed_streak[l] << " cycles)";
                out << std::endl;
            }
        }
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// monitor mode
// ---------------------------------------------------------------------------
// shares the posting-tracking machinery and persisted state above, but
// performs no labor management -- it just watches the job board and feeds
// the task starvation notification.

void MonitorEngine::enable(color_ostream &out)
{
    // undo the legacy engine's flag if switching straight from it, so
    // vanilla work details govern labors again
    game->external_flag.bits.automatic_professions_disabled = false;

    if (!labor_mapper)
        labor_mapper = new JobLaborMapper();
    init_labor_to_skill();

    posting_track.clear();
    skill_usage.clear();
    stat_starving = 0;
    stat_oldest_wait = 0;
    stat_oldest_job = df::job_type::NONE;
    stat_oldest_reaction.clear();
    stat_resolved = 0;
    stat_resolved_ticks = 0;
    stat_max_wait = 0;
    restore_state();
    initialized = true;

    out << "Enabling labor monitor (starvation warnings only)." << std::endl;
}

void MonitorEngine::disable(color_ostream &out)
{
    initialized = false;
    out << "Disabling labor monitor." << std::endl;
}

void MonitorEngine::map_unload()
{
    initialized = false;
    posting_track.clear();
    skill_usage.clear();
    stat_starving = 0;
    stat_oldest_wait = 0;
    stat_oldest_job = df::job_type::NONE;
    stat_oldest_reaction.clear();
    stat_resolved = 0;
    stat_resolved_ticks = 0;
    stat_max_wait = 0;
}

void MonitorEngine::update(color_ostream &out)
{
    if (!world || !world->map.block_index || !initialized)
        return;
    if (world->frame_counter - cycle_timestamp < CYCLE_TICKS)
        return;
    cycle_timestamp = world->frame_counter;

    // the demand scan does the posting-age tracking and starvation stats;
    // the per-labor backlog it also fills is unused here
    std::map<df::unit_labor, int> backlog;
    scan_demand(backlog);
    save_state();
}

int MonitorEngine::starving_jobs()
{
    return initialized ? stat_starving : 0;
}

df::job_type MonitorEngine::oldest_starving_job()
{
    return initialized && stat_starving > 0 ? stat_oldest_job
                                            : df::job_type::NONE;
}

int32_t MonitorEngine::oldest_starving_wait()
{
    return initialized && stat_starving > 0 ? stat_oldest_wait : 0;
}

df::coord MonitorEngine::oldest_starving_pos()
{
    return initialized && stat_starving > 0 ? stat_oldest_pos
                                            : df::coord();
}

std::string MonitorEngine::oldest_starving_name()
{
    return initialized && stat_starving > 0 ? ::oldest_starving_name() : "";
}

bool MonitorEngine::command(color_ostream &out, std::vector<std::string> &parameters)
{
    if (parameters.size() == 1 &&
        (parameters[0] == "list" || parameters[0] == "status"))
    {
        out << "labor monitor: " << stat_starving << " starving postings (> "
            << STARVE_TICKS << " ticks";
        if (stat_starving > 0 && stat_oldest_job != df::job_type::NONE)
            out << ", oldest " << ::oldest_starving_name()
                << " at " << stat_oldest_wait << " ticks";
        out << ")" << std::endl;
        out << stat_resolved << " postings resolved";
        if (stat_resolved > 0)
            out << ", avg wait " << (stat_resolved_ticks / stat_resolved)
                << " ticks, longest " << stat_max_wait << " ticks";
        out << std::endl;
        return true;
    }
    return false;
}
