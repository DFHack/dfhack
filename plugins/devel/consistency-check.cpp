// devel/consistency-check - periodically scan DF data structures for violated invariants
//
// The invariants checked here were collected from:
//   - df-structures annotations (key-field/instance-vector -> binary search,
//     ref-target fields, has-bad-pointers exemptions, other-vector type attrs)
//   - DFHack modules that rely on structure invariants (Job.cpp, Items.cpp,
//     Buildings.cpp, EventManager.cpp, fix-occupancy, army-controller-sanity)
//   - fix/* scripts, which encode historically observed corruption modes
//     (corrupt jobs with id == -1, dangling equipment entries, broken zone
//     ownership links, stuck written materials, bad map occupancy, ...)
//   - DFHack GitHub issues/PRs: #3861 (jobs with id -1), bug 11014 (equipment
//     corruption), #5581 (corrupt animal->pasture links), ...

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "Core.h"
#include "DataDefs.h"
#include "Debug.h"
#include "Format.h"
#include "PluginManager.h"

#include "modules/Buildings.h"
#include "modules/Items.h"
#include "modules/Maps.h"

#include "df/global_objects.h"

#include "df/activity_entry.h"
#include "df/agreement.h"
#include "df/army.h"
#include "df/army_controller.h"
#include "df/art_image_chunk.h"
#include "df/artifact_record.h"
#include "df/belief_system.h"
#include "df/building.h"
#include "df/building_actual.h"
#include "df/building_civzonest.h"
#include "df/buildingitemst.h"
#include "df/buildings_other.h"
#include "df/buildings_other_id.h"
#include "df/burrow.h"
#include "df/coin_batch.h"
#include "df/creature_raw.h"
#include "df/crime.h"
#include "df/cultural_identity.h"
#include "df/dance_form.h"
#include "df/divination_set.h"
#include "df/engraving.h"
#include "df/entity_population.h"
#include "df/flow_guide.h"
#include "df/formationst.h"
#include "df/general_ref.h"
#include "df/general_ref_abstract_buildingst.h"
#include "df/general_ref_activity_eventst.h"
#include "df/general_ref_artifact.h"
#include "df/general_ref_building.h"
#include "df/general_ref_coinbatch.h"
#include "df/general_ref_creaturest.h"
#include "df/general_ref_dance_formst.h"
#include "df/general_ref_entity.h"
#include "df/general_ref_entity_art_image.h"
#include "df/general_ref_entity_popst.h"
#include "df/general_ref_feature_layerst.h"
#include "df/general_ref_historical_eventst.h"
#include "df/general_ref_historical_figurest.h"
#include "df/general_ref_item.h"
#include "df/general_ref_musical_formst.h"
#include "df/general_ref_nemesis.h"
#include "df/general_ref_poetic_formst.h"
#include "df/general_ref_projectile.h"
#include "df/general_ref_sitest.h"
#include "df/general_ref_subregionst.h"
#include "df/general_ref_unit.h"
#include "df/general_ref_written_contentst.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/history_event.h"
#include "df/history_event_collection.h"
#include "df/identity.h"
#include "df/image_set.h"
#include "df/incident.h"
#include "df/interaction_instance.h"
#include "df/item.h"
#include "df/items_other.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/job_item_ref.h"
#include "df/job_list_link.h"
#include "df/machine.h"
#include "df/map_block.h"
#include "df/musical_form.h"
#include "df/nemesis_record.h"
#include "df/occupation.h"
#include "df/plotinfost.h"
#include "df/poetic_form.h"
#include "df/proj_itemst.h"
#include "df/proj_list_link.h"
#include "df/proj_unitst.h"
#include "df/rhythm.h"
#include "df/scale.h"
#include "df/schedule_info.h"
#include "df/specific_ref.h"
#include "df/squad.h"
#include "df/squad_position.h"
#include "df/tile_occupancy.h"
#include "df/tiletype.h"
#include "df/unit.h"
#include "df/unit_chunk.h"
#include "df/unit_relationship_type.h"
#include "df/unit_inventory_item.h"
#include "df/units_other.h"
#include "df/units_other_id.h"
#include "df/vehicle.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_history.h"
#include "df/world_region.h"
#include "df/world_site.h"
#include "df/world_underground_region.h"
#include "df/written_content.h"

using namespace DFHack;

DFHACK_PLUGIN("consistency-check");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(cur_year);
REQUIRE_GLOBAL(cur_year_tick);

namespace DFHack {
    DBG_DECLARE(consistency_check, log, DebugCategory::LWARNING);
}

static const int TICKS_PER_YEAR = 403200; // 12 * 28 * 1200

// plugin state (not persisted across sessions)
static int scan_interval = 1200;    // game ticks between stages; ~1 in-game day
static size_t scan_stage = 0;       // next stage to run
static int64_t last_scan_tick = -1; // last time a stage ran
static bool opt_deep = true;        // include the (heavier) map block checks
static std::set<std::string> seen_reports;
static size_t total_report_count = 0;

static int64_t current_tick()
{
    // cur_year_tick resets to 0 on year rollover
    return int64_t(*cur_year) * TICKS_PER_YEAR + *cur_year_tick;
}

struct Scanner
{
    color_ostream & out;
    size_t errors = 0;

    // lazily-built membership sets for the current stage
    bool collected_units = false, collected_items = false,
         collected_buildings = false, collected_jobs = false,
         collected_projectiles = false, collected_job_items = false,
         collected_histfigs = false;
    std::unordered_set<df::unit *> units;
    std::unordered_set<df::item *> items;
    std::unordered_set<df::building *> buildings;
    std::unordered_set<df::job *> jobs;
    std::unordered_map<int32_t, df::job *> jobs_by_id;
    std::unordered_set<df::army_controller *> army_controllers;
    std::unordered_set<df::historical_figure *> histfigs;
    std::unordered_set<int32_t> projectile_ids;
    std::unordered_set<int32_t> job_item_ids;

    Scanner(color_ostream & out) : out(out) {}

    // at most this many reports of each category (message template) are
    // printed per run; additional reports are still counted and deduplicated
    static const size_t MAX_REPORTS_PER_CATEGORY = 20;
    std::unordered_map<std::string, size_t> category_counts;

    void report_str(const std::string & msg, const std::string & category)
    {
        if (!seen_reports.insert(msg).second)
            return; // already reported this exact issue
        errors++;
        total_report_count++;
        auto count = ++category_counts[category];
        if (count == MAX_REPORTS_PER_CATEGORY + 1)
        {
            WARN(log, out).print("consistency-check: report limit ({}) reached "
                                 "for '{}'; further issues of this kind are "
                                 "suppressed for this run\n",
                                 MAX_REPORTS_PER_CATEGORY, category);
        }
        else if (count <= MAX_REPORTS_PER_CATEGORY)
        {
            WARN(log, out).print("consistency-check: {}\n", msg);
        }
    }

    template<typename... Args>
    void report(fmt::format_string<Args...> fstr, Args &&... args)
    {
        report_str(fmt::format(fstr, std::forward<Args>(args)...),
                   std::string(fstr.get().data(), fstr.get().size()));
    }

    // ---- shared lookups ----------------------------------------------------

    void collect_units()
    {
        if (collected_units)
            return;
        collected_units = true;
        for (auto u : world->units.all)
            units.insert(u);
    }
    void collect_items()
    {
        if (collected_items)
            return;
        collected_items = true;
        for (auto item : world->items.all)
            items.insert(item);
    }
    void collect_buildings()
    {
        if (collected_buildings)
            return;
        collected_buildings = true;
        for (auto bld : world->buildings.all)
            buildings.insert(bld);
    }
    void collect_jobs()
    {
        if (collected_jobs)
            return;
        collected_jobs = true;
        for (auto link = world->jobs.list.next; link; link = link->next)
        {
            if (!link->item)
                continue;
            jobs.insert(link->item);
            jobs_by_id[link->item->id] = link->item;
        }
    }
    void collect_projectiles()
    {
        if (collected_projectiles)
            return;
        collected_projectiles = true;
        for (auto link = world->projectiles.all.next; link; link = link->next)
        {
            if (link->item)
                projectile_ids.insert(link->item->id);
        }
    }
    void collect_job_items()
    {
        if (collected_job_items)
            return;
        collected_job_items = true;
        collect_jobs();
        for (auto job : jobs)
            for (auto jref : job->items)
                if (jref->item)
                    job_item_ids.insert(jref->item->id);
    }
    void collect_histfigs()
    {
        if (collected_histfigs)
            return;
        collected_histfigs = true;
        for (auto hf : world->history.figures)
            histfigs.insert(hf);
    }

    df::job *find_job(int32_t id)
    {
        collect_jobs();
        auto it = jobs_by_id.find(id);
        return it == jobs_by_id.end() ? nullptr : it->second;
    }

    // ---- generic reference checks ------------------------------------------

    void check_general_ref(df::general_ref *ref, const char *context)
    {
        if (!ref)
        {
            report("{}: null general_ref", context);
            return;
        }

        if (auto r = virtual_cast<df::general_ref_unit>(ref))
        {
            if (!df::unit::find(r->unit_id))
                report("{}: {} has dangling unit_id {} (not in world.units.all)",
                       context, enum_item_key(ref->getType()), r->unit_id);
        }
        else if (auto r = virtual_cast<df::general_ref_item>(ref))
        {
            if (!df::item::find(r->item_id))
                report("{}: {} has dangling item_id {} (not in world.items.all)",
                       context, enum_item_key(ref->getType()), r->item_id);
        }
        else if (auto r = virtual_cast<df::general_ref_building>(ref))
        {
            auto target = df::building::find(r->building_id);
            if (!target)
                report("{}: {} has dangling building_id {} (not in world.buildings.all)",
                       context, enum_item_key(ref->getType()), r->building_id);
            else if (ref->getType() == df::general_ref_type::BUILDING_CIVZONE_ASSIGNED &&
                     !virtual_cast<df::building_civzonest>(target))
                report("{}: BUILDING_CIVZONE_ASSIGNED ref to non-civzone "
                       "building {} (issue #5580)", context, r->building_id);
        }
        else if (auto r = virtual_cast<df::general_ref_artifact>(ref))
        {
            if (!df::artifact_record::find(r->artifact_id))
                report("{}: {} has dangling artifact_id {}",
                       context, enum_item_key(ref->getType()), r->artifact_id);
        }
        else if (auto r = virtual_cast<df::general_ref_nemesis>(ref))
        {
            if (!df::nemesis_record::find(r->nemesis_id))
                report("{}: {} has dangling nemesis_id {}",
                       context, enum_item_key(ref->getType()), r->nemesis_id);
        }
        else if (auto r = virtual_cast<df::general_ref_historical_figurest>(ref))
        {
            if (!df::historical_figure::find(r->hist_figure_id))
                report("{}: {} has dangling hist_figure_id {}",
                       context, enum_item_key(ref->getType()), r->hist_figure_id);
        }
        else if (auto r = virtual_cast<df::general_ref_historical_eventst>(ref))
        {
            if (!df::history_event::find(r->event_id))
                report("{}: {} has dangling event_id {}",
                       context, enum_item_key(ref->getType()), r->event_id);
        }
        else if (auto r = virtual_cast<df::general_ref_entity>(ref))
        {
            if (!df::historical_entity::find(r->entity_id))
                report("{}: {} has dangling entity_id {}",
                       context, enum_item_key(ref->getType()), r->entity_id);
        }
        else if (auto r = virtual_cast<df::general_ref_entity_art_image>(ref))
        {
            if (!df::historical_entity::find(r->entity_id))
                report("{}: {} has dangling entity_id {}",
                       context, enum_item_key(ref->getType()), r->entity_id);
        }
        else if (auto r = virtual_cast<df::general_ref_entity_popst>(ref))
        {
            if (r->pop_id != -1 && !df::entity_population::find(r->pop_id))
                report("{}: {} has dangling pop_id {}",
                       context, enum_item_key(ref->getType()), r->pop_id);
            if (r->race != -1 && !df::creature_raw::find(r->race))
                report("{}: {} has dangling race {}",
                       context, enum_item_key(ref->getType()), r->race);
        }
        else if (auto r = virtual_cast<df::general_ref_creaturest>(ref))
        {
            if (auto craw = df::creature_raw::find(r->race))
            {
                if (r->caste >= 0 && r->caste >= (int16_t)craw->caste.size())
                    report("{}: {} has out-of-range caste {} (race {} has {} castes)",
                           context, enum_item_key(ref->getType()), r->caste, r->race,
                           craw->caste.size());
            }
            else
                report("{}: {} has dangling race {}",
                       context, enum_item_key(ref->getType()), r->race);
            if (r->pop_id != -1 && !df::entity_population::find(r->pop_id))
                report("{}: {} has dangling pop_id {}",
                       context, enum_item_key(ref->getType()), r->pop_id);
        }
        else if (auto r = virtual_cast<df::general_ref_coinbatch>(ref))
        {
            if (!df::coin_batch::find(r->batch))
                report("{}: {} has dangling coinbatch {}",
                       context, enum_item_key(ref->getType()), r->batch);
        }
        else if (auto r = virtual_cast<df::general_ref_written_contentst>(ref))
        {
            if (!df::written_content::find(r->written_content_id))
                report("{}: {} has dangling written_content_id {}",
                       context, enum_item_key(ref->getType()), r->written_content_id);
        }
        else if (auto r = virtual_cast<df::general_ref_poetic_formst>(ref))
        {
            if (!df::poetic_form::find(r->poetic_form_id))
                report("{}: {} has dangling poetic_form_id {}",
                       context, enum_item_key(ref->getType()), r->poetic_form_id);
        }
        else if (auto r = virtual_cast<df::general_ref_musical_formst>(ref))
        {
            if (!df::musical_form::find(r->musical_form_id))
                report("{}: {} has dangling musical_form_id {}",
                       context, enum_item_key(ref->getType()), r->musical_form_id);
        }
        else if (auto r = virtual_cast<df::general_ref_dance_formst>(ref))
        {
            if (!df::dance_form::find(r->dance_form_id))
                report("{}: {} has dangling dance_form_id {}",
                       context, enum_item_key(ref->getType()), r->dance_form_id);
        }
        else if (auto r = virtual_cast<df::general_ref_sitest>(ref))
        {
            if (!df::world_site::find(r->site_id))
                report("{}: {} has dangling site_id {}",
                       context, enum_item_key(ref->getType()), r->site_id);
        }
        else if (auto r = virtual_cast<df::general_ref_subregionst>(ref))
        {
            if (!df::world_region::find(r->region_id))
                report("{}: {} has dangling region_id {}",
                       context, enum_item_key(ref->getType()), r->region_id);
        }
        else if (auto r = virtual_cast<df::general_ref_feature_layerst>(ref))
        {
            if (!df::world_underground_region::find(r->underground_region_id))
                report("{}: {} has dangling underground_region_id {}",
                       context, enum_item_key(ref->getType()), r->underground_region_id);
        }
        else if (auto r = virtual_cast<df::general_ref_activity_eventst>(ref))
        {
            if (!df::activity_entry::find(r->activity_id))
                report("{}: {} has dangling activity_id {} "
                       "(cf. fix/stuck-written-materials, fix/stuck-instruments)",
                       context, enum_item_key(ref->getType()), r->activity_id);
            // note: event_id here is an activity-event index, not a
            // history_event id
        }
        else if (auto r = virtual_cast<df::general_ref_projectile>(ref))
        {
            collect_projectiles();
            if (!projectile_ids.count(r->projectile_id))
                report("{}: {} has dangling projectile_id {}",
                       context, enum_item_key(ref->getType()), r->projectile_id);
        }
        else if (auto r = virtual_cast<df::general_ref_abstract_buildingst>(ref))
        {
            if (auto site = df::world_site::find(r->site_id))
            {
                // building_id indexes the site's buildings list
                if (r->building_id < 0 ||
                    r->building_id >= (int32_t)site->buildings.size())
                    report("{}: {} has out-of-range building_id {} "
                           "(site {} has {} buildings)", context,
                           enum_item_key(ref->getType()), r->building_id,
                           r->site_id, site->buildings.size());
            }
            else
                report("{}: {} has dangling site_id {}",
                       context, enum_item_key(ref->getType()), r->site_id);
        }
        // remaining ref types have no simply-resolvable targets
    }

    void check_specific_ref(df::specific_ref *ref, const char *context)
    {
        if (!ref)
        {
            report("{}: null specific_ref", context);
            return;
        }
        switch (ref->type)
        {
            case df::specific_ref_type::JOB:
                collect_jobs();
                if (!ref->data.job || !jobs.count(ref->data.job))
                    report("{}: specific_ref JOB points at a job not in world.jobs.list",
                           context);
                break;
            case df::specific_ref_type::UNIT:
                collect_units();
                if (!ref->data.unit || !units.count(ref->data.unit))
                    report("{}: specific_ref UNIT points at a unit not in world.units.all",
                           context);
                break;
            default:
                break;
        }
    }

    void check_all_refs(std::vector<df::general_ref*> &grefs,
                        std::vector<df::specific_ref*> &srefs, const char *context)
    {
        for (auto ref : grefs)
            check_general_ref(ref, context);
        for (auto ref : srefs)
            check_specific_ref(ref, context);
    }

    // ---- stage 0: sorted vectors, next_id counters, other vectors ----------

    template<typename T>
    void check_sorted_ids(const char *path, const std::vector<T*> &vec, int32_t *next_id)
    {
        int64_t prev = INT64_MIN;
        int32_t max_id = -1;
        size_t i = 0;
        for (auto entry : vec)
        {
            if (!entry)
            {
                report("{}[{}] is null", path, i++);
                continue;
            }
            if (entry->id < prev)
                report("{}[{}]: id {} is out of order (preceded by {}); "
                       "binsearch lookups may fail", path, i, entry->id, prev);
            else if (entry->id == prev)
                report("{}[{}]: duplicate id {}", path, i, entry->id);
            prev = entry->id;
            max_id = std::max(max_id, entry->id);
            i++;
        }
        if (next_id && *next_id <= max_id)
            report("{}: next_id {} but an object with id {} exists", path, *next_id, max_id);
    }

    void check_other_vectors()
    {
        // items.other: every entry must be in items.all and, for master
        // vectors, be of the type declared in the items_other_id enum attrs
        // (cf. devel/check-other-ids, fix/corrupt-equipment)
        collect_items();
        auto fields = df::items_other::_identity.getFields();
        for (auto field = fields; field->mode != struct_field_info::END; field++)
        {
            if (field->mode != struct_field_info::CONTAINER &&
                field->mode != struct_field_info::STL_VECTOR_PTR)
                continue;
            df::items_other_id id;
            if (!find_enum_item(&id, field->name))
                continue;
            auto vec = reinterpret_cast<const std::vector<df::item*> *>(
                uintptr_t(&world->items.other) + field->offset);
            auto expected = ENUM_ATTR(items_other_id, item, id);
            auto generic = ENUM_ATTR(items_other_id, generic_item, id);
            size_t i = 0;
            for (auto item : *vec)
            {
                std::string path = fmt::format("world.items.other.{}[{}]", field->name, i++);
                if (!item)
                {
                    report("{}: null item", path);
                    continue;
                }
                if (!items.count(item))
                {
                    // may be dangling; don't dereference
                    report("{}: item pointer {} not in world.items.all",
                           path, static_cast<void*>(item));
                    continue;
                }
                auto type = item->getType();
                if (expected != df::item_type::NONE && type != expected)
                    report("{}: item {} has type {}, expected {}",
                           path, item->id, enum_item_key(type), enum_item_key(expected));
                else if (generic.size > 0)
                {
                    bool found = false;
                    for (size_t j = 0; j < generic.size && !found; j++)
                        found = generic.items[j] == type;
                    if (!found)
                        report("{}: item {} has unexpected type {}", path, item->id,
                               enum_item_key(type));
                }
            }
        }

        // buildings.other: same, keyed by buildings_other_id
        collect_buildings();
        fields = df::buildings_other::_identity.getFields();
        for (auto field = fields; field->mode != struct_field_info::END; field++)
        {
            if (field->mode != struct_field_info::CONTAINER &&
                field->mode != struct_field_info::STL_VECTOR_PTR)
                continue;
            df::buildings_other_id id;
            if (!find_enum_item(&id, field->name))
                continue;
            auto vec = reinterpret_cast<const std::vector<df::building*> *>(
                uintptr_t(&world->buildings.other) + field->offset);
            auto expected = ENUM_ATTR(buildings_other_id, building, id);
            size_t i = 0;
            for (auto bld : *vec)
            {
                std::string path = fmt::format("world.buildings.other.{}[{}]", field->name, i++);
                if (!bld)
                {
                    report("{}: null building", path);
                    continue;
                }
                if (!buildings.count(bld))
                {
                    // may be dangling; don't dereference
                    report("{}: building pointer {} not in world.buildings.all",
                           path, static_cast<void*>(bld));
                    continue;
                }
                auto type = bld->getType();
                if (expected != df::building_type::NONE && type != expected)
                    report("{}: building {} has type {}, expected {}",
                           path, bld->id, enum_item_key(type), enum_item_key(expected));
            }
        }

        // units.other: membership only
        collect_units();
        fields = df::units_other::_identity.getFields();
        for (auto field = fields; field->mode != struct_field_info::END; field++)
        {
            if (field->mode != struct_field_info::CONTAINER &&
                field->mode != struct_field_info::STL_VECTOR_PTR)
                continue;
            auto vec = reinterpret_cast<const std::vector<df::unit*> *>(
                uintptr_t(&world->units.other) + field->offset);
            size_t i = 0;
            for (auto unit : *vec)
            {
                std::string path = fmt::format("world.units.other.{}[{}]", field->name, i++);
                if (!unit)
                    report("{}: null unit", path);
                else if (!units.count(unit))
                    // may be dangling; don't dereference
                    report("{}: unit pointer {} not in world.units.all",
                           path, static_cast<void*>(unit));
            }
        }

        // units.active is a subset of units.all and has no duplicates
        std::unordered_set<df::unit*> active_seen;
        size_t i = 0;
        for (auto unit : world->units.active)
        {
            if (!unit)
            {
                report("world.units.active[{}] is null", i++);
                continue;
            }
            if (!units.count(unit))
            {
                // may be dangling; don't dereference
                report("world.units.active[{}]: unit pointer {} not in "
                       "world.units.all", i++, static_cast<void*>(unit));
                continue;
            }
            if (!active_seen.insert(unit).second)
                report("world.units.active[{}]: unit {} appears twice", i, unit->id);
            i++;
        }
    }

    void check_sorted_vectors()
    {
        check_sorted_ids("world.units.all", world->units.all, df::global::unit_next_id);
        check_sorted_ids("world.items.all", world->items.all, df::global::item_next_id);
        check_sorted_ids("world.buildings.all", world->buildings.all, df::global::building_next_id);
        check_sorted_ids("world.entities.all", world->entities.all, df::global::entity_next_id);
        check_sorted_ids("world.nemesis.all", world->nemesis.all, df::global::nemesis_next_id);
        check_sorted_ids("world.artifacts.all", world->artifacts.all, df::global::artifact_next_id);
        check_sorted_ids("world.squads.all", world->squads.all, df::global::squad_next_id);
        check_sorted_ids("world.armies.all", world->armies.all, df::global::army_next_id);
        check_sorted_ids("world.army_controllers.all", world->army_controllers.all,
                         df::global::army_controller_next_id);
        check_sorted_ids("world.activities.all", world->activities.all, df::global::activity_next_id);
        check_sorted_ids("world.written_contents.all", world->written_contents.all,
                         df::global::written_content_next_id);
        check_sorted_ids("world.crimes.all", world->crimes.all, df::global::crime_next_id);
        check_sorted_ids("world.agreements.all", world->agreements.all, df::global::agreement_next_id);
        check_sorted_ids("world.incidents.all", world->incidents.all, df::global::incident_next_id);
        check_sorted_ids("world.identities.all", world->identities.all, df::global::identity_next_id);
        check_sorted_ids("world.image_sets.all", world->image_sets.all, df::global::image_set_next_id);
        check_sorted_ids("world.divination_sets.all", world->divination_sets.all,
                         df::global::divination_set_next_id);
        check_sorted_ids("world.belief_systems.all", world->belief_systems.all,
                         df::global::belief_system_next_id);
        check_sorted_ids("world.cultural_identities.all", world->cultural_identities.all,
                         df::global::cultural_identity_next_id);
        check_sorted_ids("world.poetic_forms.all", world->poetic_forms.all,
                         df::global::poetic_form_next_id);
        check_sorted_ids("world.musical_forms.all", world->musical_forms.all,
                         df::global::musical_form_next_id);
        check_sorted_ids("world.dance_forms.all", world->dance_forms.all,
                         df::global::dance_form_next_id);
        check_sorted_ids("world.art_image_chunks.all", world->art_image_chunks.all,
                         df::global::art_image_chunk_next_id);
        check_sorted_ids("world.unit_chunks.all", world->unit_chunks.all,
                         df::global::unit_chunk_next_id);
        check_sorted_ids("world.machines.all", world->machines.all, df::global::machine_next_id);
        check_sorted_ids("world.flow_guides.all", world->flow_guides.all,
                         df::global::flow_guide_next_id);
        check_sorted_ids("world.formations.all", world->formations.all,
                         df::global::formation_next_id);
        check_sorted_ids("world.schedules.all", world->schedules.all,
                         df::global::schedule_next_id);
        check_sorted_ids("world.occupations.all", world->occupations.all,
                         df::global::occupation_next_id);
        check_sorted_ids("world.vehicles.all", world->vehicles.all, df::global::vehicle_next_id);
        check_sorted_ids("world.scales.all", world->scales.all, df::global::scale_next_id);
        check_sorted_ids("world.rhythms.all", world->rhythms.all, df::global::rhythm_next_id);
        check_sorted_ids("world.interaction_instances.all", world->interaction_instances.all,
                         df::global::interaction_instance_next_id);
        check_sorted_ids("world.history.figures", world->history.figures,
                         df::global::hist_figure_next_id);
        check_sorted_ids("world.history.events", world->history.events,
                         df::global::hist_event_next_id);
        check_sorted_ids("world.history.event_collections.all", world->history.event_collections.all,
                         df::global::hist_event_collection_next_id);
        if (plotinfo)
            check_sorted_ids("plotinfo.burrows.list", plotinfo->burrows.list, nullptr);

        // linked lists must be non-decreasing in id, acyclic, and self-consistent
        check_linked_list("world.jobs.list", &world->jobs.list, df::global::job_next_id);
        check_linked_list("world.projectiles.all", &world->projectiles.all,
                          df::global::proj_next_id);
    }

    void check_linked_list(const char *path, df::job_list_link *head, int32_t *next_id)
    {
        std::unordered_set<df::job_list_link*> seen_links;
        int64_t prev_id = INT64_MIN;
        int32_t max_id = -1;
        df::job_list_link *prev_link = head;
        for (auto link = head->next; link; link = link->next)
        {
            if (!seen_links.insert(link).second)
            {
                report("{}: cycle detected", path);
                break;
            }
            if (link->prev != prev_link)
                report("{}: broken prev link near job id {}",
                       path, link->item ? link->item->id : -1);
            if (auto job = link->item)
            {
                if (job->list_link != link)
                    report("{}: job {} list_link does not point back at its link",
                           path, job->id);
                if (job->id < prev_id)
                    report("{}: job id {} is out of order (preceded by {}); "
                           "job list is expected to be sorted", path, job->id, prev_id);
                prev_id = job->id;
                max_id = std::max(max_id, job->id);
            }
            else
                report("{}: link with null item", path);
            prev_link = link;
        }
        if (next_id && *next_id <= max_id)
            report("{}: next_id {} but a job with id {} exists", path, *next_id, max_id);
    }

    void check_linked_list(const char *path, df::proj_list_link *head, int32_t *next_id)
    {
        std::unordered_set<df::proj_list_link*> seen_links;
        int32_t max_id = -1;
        df::proj_list_link *prev_link = head;
        for (auto link = head->next; link; link = link->next)
        {
            if (!seen_links.insert(link).second)
            {
                report("{}: cycle detected", path);
                break;
            }
            if (link->prev != prev_link)
                report("{}: broken prev link near projectile id {}",
                       path, link->item ? link->item->id : -1);
            if (auto proj = link->item)
            {
                if (proj->link != link)
                    report("{}: projectile {} link does not point back at its link",
                           path, proj->id);
                max_id = std::max(max_id, proj->id);
            }
            else
                report("{}: link with null item", path);
            prev_link = link;
        }
        if (next_id && *next_id <= max_id)
            report("{}: next_id {} but a projectile with id {} exists",
                   path, *next_id, max_id);
    }

    // ---- stage 1: units ----------------------------------------------------

    void check_units()
    {
        collect_units();
        collect_jobs();
        collect_items();
        collect_buildings();
        for (auto ac : world->army_controllers.all)
            army_controllers.insert(ac);

        int32_t dim_x, dim_y, dim_z;
        Maps::getTileSize(dim_x, dim_y, dim_z);
        bool have_map = Core::getInstance().isMapLoaded() && dim_x > 0;

        for (auto unit : world->units.all)
        {
            if (!unit)
                continue;
            std::string ctx = fmt::format("unit {}", unit->id);

            // race/caste must resolve (dangling race indices crash DF)
            if (auto craw = df::creature_raw::find(unit->race))
            {
                // caste -1 is not reported; it may be legitimate for some
                // unit kinds (e.g. wagons)
                if (unit->caste >= (int16_t)craw->caste.size())
                    report("{}: caste {} out of range (race {} has {} castes)",
                           ctx, unit->caste, unit->race, craw->caste.size());
            }
            else
                report("{}: race {} not in world.raws.creatures.all", ctx, unit->race);

            if (unit->civ_id != -1 && !df::historical_entity::find(unit->civ_id))
                report("{}: dangling civ_id {}", ctx, unit->civ_id);

            if (unit->hist_figure_id != -1 &&
                !df::historical_figure::find(unit->hist_figure_id))
                report("{}: dangling hist_figure_id {}", ctx, unit->hist_figure_id);

            // corrupted jobs: current_job with id -1 or not in the job list
            // (issue #3861, fixed by fix/corrupt-jobs)
            if (auto job = unit->job.current_job)
            {
                if (!jobs.count(job))
                    // may be dangling; don't dereference
                    report("{}: current_job pointer {} not in world.jobs.list",
                           ctx, static_cast<void*>(job));
                else if (job->id == -1)
                    report("{}: current_job has id -1 (fix/corrupt-jobs)", ctx);
                else if (!job_has_ref_to_unit(job, unit->id))
                    report("{}: current_job {} lacks UNIT_WORKER back-reference",
                           ctx, job->id);
            }

            // army controller link consistency (army-controller-sanity plugin)
            auto ac = unit->enemy.army_controller;
            if (ac)
            {
                if (!army_controllers.count(ac))
                    report("{}: enemy.army_controller pointer {} not in "
                           "world.army_controllers.all", ctx,
                           static_cast<void*>(ac));
                else if (ac->id != unit->enemy.army_controller_id)
                    report("{}: army_controller id mismatch ({} != {})",
                           ctx, unit->enemy.army_controller_id, ac->id);
            }
            else if (unit->enemy.army_controller_id != -1 &&
                     unit->enemy.army_controller_id != 0)
                report("{}: enemy.army_controller_id is {} but pointer is null",
                       ctx, unit->enemy.army_controller_id);

            // inventory items must point back at the unit via UNIT_HOLDER
            size_t i = 0;
            for (auto inv : unit->inventory)
            {
                std::string ictx = fmt::format("{}.inventory[{}]", ctx, i++);
                if (!inv || !inv->item)
                {
                    report("{}: null inventory item", ictx);
                    continue;
                }
                if (!items.count(inv->item))
                    report("{}: item pointer {} not in world.items.all",
                           ictx, static_cast<void*>(inv->item));
                else if (!item_has_unit_holder_ref(inv->item, unit->id))
                    report("{}: item {} lacks UNIT_HOLDER ref back to unit",
                           ictx, inv->item->id);
            }

            // owned items must have a UNIT_ITEMOWNER ref back to the unit
            // (fix/ownership)
            for (auto item_id : unit->owned_items)
            {
                auto item = df::item::find(item_id);
                if (!item)
                    report("{}: owned_items entry {} not in world.items.all", ctx, item_id);
                else if (!item_has_unit_ref(item, df::general_ref_type::UNIT_ITEMOWNER,
                                            unit->id))
                    report("{}: owned item {} lacks UNIT_ITEMOWNER back-reference",
                           ctx, item_id);
            }

            // a unit's zone-assignment ref must be mirrored in the zone's
            // assigned_units (issue #5580, plugins/zone.cpp)
            for (auto ref : unit->general_refs)
            {
                if (ref->getType() != df::general_ref_type::BUILDING_CIVZONE_ASSIGNED)
                    continue;
                auto bref = virtual_cast<df::general_ref_building>(ref);
                if (!bref)
                    continue;
                auto zone = virtual_cast<df::building_civzonest>(
                    df::building::find(bref->building_id));
                if (zone &&
                    std::find(zone->assigned_units.begin(),
                              zone->assigned_units.end(),
                              unit->id) == zone->assigned_units.end())
                    report("{}: BUILDING_CIVZONE_ASSIGNED ref to zone {} but the "
                           "zone's assigned_units does not list the unit",
                           ctx, bref->building_id);
            }

            // owned zones must list this unit as their assigned unit
            for (auto zone : unit->owned_buildings)
            {
                if (!zone)
                    continue;
                collect_buildings();
                if (!buildings.count(zone))
                    report("{}: owned_buildings entry {} not in "
                           "world.buildings.all", ctx, static_cast<void*>(zone));
                else if (zone->assigned_unit_id != unit->id &&
                         // permissible: zones usable as spouse rooms are also
                         // pushed to the spouse's owned_buildings
                         // (Buildings::setOwner)
                         !(zone->canUseSpouseRoom() &&
                           zone->assigned_unit_id ==
                               unit->relationship_ids[
                                   df::unit_relationship_type::Spouse]))
                    report("{}: owned zone {} has assigned_unit_id {} (expected {})",
                           ctx, zone->id, zone->assigned_unit_id, unit->id);
            }

            // valid positions must be inside the map
            if (have_map && unit->pos.isValid() &&
                (unit->pos.x < 0 || unit->pos.x >= dim_x ||
                 unit->pos.y < 0 || unit->pos.y >= dim_y ||
                 unit->pos.z < 0 || unit->pos.z >= dim_z))
                report("{}: position ({}, {}, {}) is outside map bounds ({}x{}x{})",
                       ctx, unit->pos.x, unit->pos.y, unit->pos.z, dim_x, dim_y, dim_z);

            check_all_refs(unit->general_refs, unit->specific_refs, ctx.c_str());
        }

        // nemesis records: unit links must be self-consistent. a nemesis may
        // refer to an offloaded unit, which lives in a unit chunk (unit-*.dat)
        // rather than world.units.all; save_file_id is a unit_chunk ref-target
        // and member_idx the slot within it. unit chunks are lazily loaded, so
        // a chunk missing from world.unit_chunks.all is not itself an error,
        // but save_file_id must be -1 or a previously allocated chunk id
        collect_histfigs();
        for (auto nemesis : world->nemesis.all)
        {
            if (!nemesis)
                continue;
            std::string nctx = fmt::format("nemesis {}", nemesis->id);

            df::unit *unit = nullptr;
            if (nemesis->unit_id != -1)
                unit = df::unit::find(nemesis->unit_id);

            if (nemesis->save_file_id < -1 ||
                (df::global::unit_chunk_next_id &&
                 nemesis->save_file_id >= *df::global::unit_chunk_next_id))
                report("{}: invalid save_file_id {}", nctx,
                       nemesis->save_file_id);
            if (nemesis->save_file_id != -1 &&
                (nemesis->member_idx < 0 || nemesis->member_idx >= 100))
                report("{}: member_idx {} out of range", nctx,
                       nemesis->member_idx);

            df::unit *chunk_unit = nullptr;
            if (nemesis->unit_id != -1 && !unit)
            {
                if (auto chunk = df::unit_chunk::find(nemesis->save_file_id))
                {
                    if (nemesis->member_idx >= 0 &&
                        (size_t)nemesis->member_idx < chunk->units.size())
                    {
                        chunk_unit = chunk->units[nemesis->member_idx].unit;
                        if (chunk_unit && chunk_unit->id != nemesis->unit_id)
                            report("{}: unit_id {} but offloaded slot {}[{}] "
                                   "holds unit {}", nctx, nemesis->unit_id,
                                   nemesis->save_file_id, nemesis->member_idx,
                                   chunk_unit->id);
                        // an empty slot means the offloaded unit hasn't been
                        // materialized; nothing more can be checked
                    }
                }
                else if (nemesis->save_file_id == -1)
                    report("{}: dangling unit_id {} (not in units.all, no "
                           "unit chunk)", nctx, nemesis->unit_id);
                // otherwise the chunk simply isn't loaded; can't verify
            }
            if (unit && nemesis->unit != unit)
                report("{}: unit pointer does not match unit_id {}", nctx,
                       nemesis->unit_id);
            else if (!unit && nemesis->unit && nemesis->unit != chunk_unit)
                report("{}: unit pointer set but unit_id {} does not resolve",
                       nctx, nemesis->unit_id);

            if (nemesis->figure && !histfigs.count(nemesis->figure))
                report("{}: figure pointer {} not in world.history.figures",
                       nctx, static_cast<void*>(nemesis->figure));
            if (nemesis->group_leader_id != -1 &&
                !df::nemesis_record::find(nemesis->group_leader_id))
                report("{}: dangling group_leader_id {}", nctx, nemesis->group_leader_id);
            if (nemesis->travel_link_nemid != -1 &&
                !df::nemesis_record::find(nemesis->travel_link_nemid))
                report("{}: dangling travel_link_nemid {}", nctx,
                       nemesis->travel_link_nemid);
            for (auto nemid : nemesis->companions)
                if (!df::nemesis_record::find(nemid))
                    report("{}: dangling companions entry {}", nctx, nemid);
        }
    }

    bool job_has_ref_to_unit(df::job *job, int32_t unit_id)
    {
        for (auto ref : job->general_refs)
            if (auto r = virtual_cast<df::general_ref_unit>(ref))
                if (ref->getType() == df::general_ref_type::UNIT_WORKER &&
                    r->unit_id == unit_id)
                    return true;
        return false;
    }

    bool item_has_unit_holder_ref(df::item *item, int32_t unit_id)
    {
        return item_has_unit_ref(item, df::general_ref_type::UNIT_HOLDER, unit_id);
    }

    bool item_has_unit_ref(df::item *item, df::general_ref_type type, int32_t unit_id)
    {
        for (auto ref : item->general_refs)
            if (auto r = virtual_cast<df::general_ref_unit>(ref))
                if (ref->getType() == type && (unit_id < 0 || r->unit_id == unit_id))
                    return true;
        return false;
    }

    bool item_has_item_ref(df::item *item, df::general_ref_type type, int32_t item_id)
    {
        for (auto ref : item->general_refs)
            if (auto r = virtual_cast<df::general_ref_item>(ref))
                if (ref->getType() == type && r->item_id == item_id)
                    return true;
        return false;
    }

    int32_t get_item_container_id(df::item *item)
    {
        for (auto ref : item->general_refs)
            if (auto r = virtual_cast<df::general_ref_item>(ref))
                if (ref->getType() == df::general_ref_type::CONTAINED_IN_ITEM)
                    return r->item_id;
        return -1;
    }

    bool item_has_building_ref(df::item *item, df::general_ref_type type, int32_t bld_id)
    {
        for (auto ref : item->general_refs)
            if (auto r = virtual_cast<df::general_ref_building>(ref))
                if (ref->getType() == type && (bld_id < 0 || r->building_id == bld_id))
                    return true;
        return false;
    }

    bool job_has_building_ref(df::job *job, int32_t bld_id)
    {
        for (auto ref : job->general_refs)
            if (auto r = virtual_cast<df::general_ref_building>(ref))
                if (ref->getType() == df::general_ref_type::BUILDING_HOLDER &&
                    r->building_id == bld_id)
                    return true;
        return false;
    }

    // ---- stage 2: items ----------------------------------------------------

    void check_items()
    {
        collect_items();
        collect_job_items();

        for (auto item : world->items.all)
        {
            if (!item)
                continue;
            std::string ctx = fmt::format("item {}", item->id);

            check_all_refs(item->general_refs, item->specific_refs, ctx.c_str());

            // containment reciprocals (CONTAINED_IN_ITEM <-> CONTAINS_ITEM)
            for (auto ref : item->general_refs)
            {
                auto iref = virtual_cast<df::general_ref_item>(ref);
                if (!iref)
                    continue;
                if (ref->getType() == df::general_ref_type::CONTAINED_IN_ITEM)
                {
                    if (auto parent = df::item::find(iref->item_id))
                        if (!item_has_item_ref(parent,
                                               df::general_ref_type::CONTAINS_ITEM,
                                               item->id))
                            report("{}: contained in item {} which lacks a "
                                   "CONTAINS_ITEM back-reference", ctx, iref->item_id);
                }
                else if (ref->getType() == df::general_ref_type::CONTAINS_ITEM)
                {
                    if (auto child = df::item::find(iref->item_id))
                        if (!item_has_item_ref(child,
                                               df::general_ref_type::CONTAINED_IN_ITEM,
                                               item->id))
                            report("{}: contains item {} which lacks a "
                                   "CONTAINED_IN_ITEM back-reference", ctx, iref->item_id);
                }
                else if (ref->getType() == df::general_ref_type::BUILDING_HOLDER)
                {
                    auto bref = virtual_cast<df::general_ref_building>(ref);
                    if (auto bld = df::building::find(bref->building_id))
                    {
                        if (auto actual = virtual_cast<df::building_actual>(bld))
                        {
                            bool found = false;
                            for (auto ci : actual->contained_items)
                                if (ci && ci->item == item)
                                    found = true;
                            if (!found)
                                report("{}: BUILDING_HOLDER ref to building {} but the "
                                       "building does not contain the item",
                                       ctx, bref->building_id);
                        }
                    }
                }
            }

            // flag consistency (fix/general-strike, fix/stuck-written-materials,
            // fix/stuck-instruments)
            if (item->flags.bits.in_building &&
                !item_has_building_ref(item, df::general_ref_type::BUILDING_HOLDER, -2))
                report("{}: flags.in_building set but no BUILDING_HOLDER ref "
                       "(fix/general-strike)", ctx);
            if (item->flags.bits.in_inventory &&
                !item_has_unit_ref(item, df::general_ref_type::UNIT_HOLDER, -2) &&
                get_item_container_id(item) == -1)
                report("{}: flags.in_inventory set but no UNIT_HOLDER or "
                       "CONTAINED_IN_ITEM ref", ctx);
            if (item->flags.bits.in_job && !item_is_in_job(item))
                report("{}: flags.in_job set but no live job references it "
                       "(fix/stuck-written-materials)", ctx);

            // containment chains must be acyclic
            std::unordered_set<df::item*> chain;
            auto cur = item;
            for (int depth = 0; depth < 256; depth++)
            {
                int32_t container_id = get_item_container_id(cur);
                if (container_id == -1)
                    break;
                auto parent = df::item::find(container_id);
                if (!parent)
                    break;
                if (!chain.insert(parent).second)
                {
                    report("{}: containment cycle involving item {}", ctx, parent->id);
                    break;
                }
                cur = parent;
            }
        }
    }

    bool item_is_in_job(df::item *item)
    {
        // contents of a container inherit the job attachment of the container
        std::unordered_set<df::item*> seen_items;
        for (auto cur = item; cur; )
        {
            if (job_item_ids.count(cur->id))
                return true;
            int32_t container_id = get_item_container_id(cur);
            if (container_id == -1)
                return false;
            cur = df::item::find(container_id);
            if (cur && !seen_items.insert(cur).second)
                return false; // cycle; bail
        }
        return false;
    }

    // ---- stage 3: buildings -------------------------------------------------

    void check_buildings()
    {
        collect_buildings();
        collect_jobs();
        collect_units();
        collect_items();

        int32_t dim_x, dim_y, dim_z;
        Maps::getTileSize(dim_x, dim_y, dim_z);
        bool have_map = Core::getInstance().isMapLoaded() && dim_x > 0;

        for (auto bld : world->buildings.all)
        {
            if (!bld)
                continue;
            std::string ctx = fmt::format("building {}", bld->id);

            if (bld->x1 > bld->x2 || bld->y1 > bld->y2)
                report("{}: inverted extents ({},{} - {},{})", ctx,
                       bld->x1, bld->y1, bld->x2, bld->y2);
            else if (have_map && bld->x1 != -30000 &&
                     (bld->x1 < 0 || bld->x2 >= dim_x ||
                      bld->y1 < 0 || bld->y2 >= dim_y ||
                      bld->z < 0 || bld->z >= dim_z))
                report("{}: extents ({},{} - {},{}, z={}) outside map bounds ({}x{}x{})",
                       ctx, bld->x1, bld->y1, bld->x2, bld->y2, bld->z,
                       dim_x, dim_y, dim_z);

            // bld->jobs must be real jobs with a BUILDING_HOLDER ref back
            size_t i = 0;
            for (auto job : bld->jobs)
            {
                std::string jctx = fmt::format("{}.jobs[{}]", ctx, i++);
                if (!job)
                {
                    report("{}: null job", jctx);
                    continue;
                }
                if (!jobs.count(job))
                    report("{}: job pointer {} not in world.jobs.list",
                           jctx, static_cast<void*>(job));
                else if (!job_has_building_ref(job, bld->id))
                    report("{}: job {} lacks BUILDING_HOLDER back-reference",
                           jctx, job->id);
            }

            // contained items must hold a BUILDING_HOLDER ref back
            if (auto actual = virtual_cast<df::building_actual>(bld))
            {
                i = 0;
                for (auto ci : actual->contained_items)
                {
                    std::string cctx = fmt::format("{}.contained_items[{}]", ctx, i++);
                    if (!ci || !ci->item)
                    {
                        report("{}: null contained item", cctx);
                        continue;
                    }
                    if (!items.count(ci->item))
                        report("{}: item pointer {} not in world.items.all",
                               cctx, static_cast<void*>(ci->item));
                    else if (!item_has_building_ref(
                                 ci->item, df::general_ref_type::BUILDING_HOLDER,
                                 bld->id))
                        report("{}: item {} lacks BUILDING_HOLDER back-reference",
                               cctx, ci->item->id);
                }
            }

            // civzone ownership links must be reciprocal (fix/ownership)
            if (auto zone = virtual_cast<df::building_civzonest>(bld))
            {
                if (zone->assigned_unit_id != -1)
                {
                    auto unit = df::unit::find(zone->assigned_unit_id);
                    if (!unit)
                        report("{}: dangling assigned_unit_id {}", ctx,
                               zone->assigned_unit_id);
                    else
                    {
                        bool found = false;
                        for (auto owned : unit->owned_buildings)
                            if (owned == zone)
                                found = true;
                        // permissible: spouse rooms are also listed in the
                        // spouse's owned_buildings (Buildings::setOwner)
                        if (!found && zone->canUseSpouseRoom())
                        {
                            if (auto spouse = df::unit::find(
                                    unit->relationship_ids[
                                        df::unit_relationship_type::Spouse]))
                                for (auto owned : spouse->owned_buildings)
                                    if (owned == zone)
                                        found = true;
                        }
                        if (!found)
                            report("{}: assigned to unit {} but unit's "
                                   "owned_buildings does not list it", ctx,
                                   zone->assigned_unit_id);
                    }
                }

                // animals assigned to a pasture/pit must carry a
                // BUILDING_CIVZONE_ASSIGNED ref back to the zone
                // (issue #5580/#5581, plugins/zone.cpp)
                for (auto unit_id : zone->assigned_units)
                {
                    auto unit = df::unit::find(unit_id);
                    if (!unit)
                    {
                        report("{}: dangling assigned_units entry {}", ctx,
                               unit_id);
                        continue;
                    }
                    bool found = false;
                    for (auto ref : unit->general_refs)
                    {
                        if (auto bref = virtual_cast<df::general_ref_building>(ref))
                            if (ref->getType() ==
                                    df::general_ref_type::BUILDING_CIVZONE_ASSIGNED &&
                                bref->building_id == zone->id)
                                found = true;
                    }
                    if (!found)
                        report("{}: assigned_units contains unit {} but the unit "
                               "lacks a BUILDING_CIVZONE_ASSIGNED ref back",
                               ctx, unit_id);
                }
                for (auto item_id : zone->assigned_items)
                    if (!df::item::find(item_id))
                        report("{}: dangling assigned_items entry {}", ctx,
                               item_id);
            }
        }
    }

    // ---- stage 4: jobs ------------------------------------------------------

    void check_jobs()
    {
        collect_jobs();
        collect_units();
        collect_buildings();
        collect_items();

        for (auto link = world->jobs.list.next; link; link = link->next)
        {
            auto job = link->item;
            if (!job)
                continue;
            std::string ctx = fmt::format("job {}", job->id);

            // items attached to the job must exist and be flagged in_job
            size_t i = 0;
            for (auto jref : job->items)
            {
                std::string ictx = fmt::format("{}.items[{}]", ctx, i++);
                if (!jref || !jref->item)
                {
                    report("{}: null item ref", ictx);
                    continue;
                }
                if (!items.count(jref->item))
                    report("{}: item pointer {} not in world.items.all",
                           ictx, static_cast<void*>(jref->item));
                else if (!jref->item->flags.bits.in_job)
                    report("{}: item {} lacks flags.in_job", ictx, jref->item->id);
            }

            // BUILDING_HOLDER -> building's jobs list must contain this job
            for (auto ref : job->general_refs)
            {
                auto bref = virtual_cast<df::general_ref_building>(ref);
                if (bref && ref->getType() == df::general_ref_type::BUILDING_HOLDER)
                {
                    if (auto bld = df::building::find(bref->building_id))
                    {
                        bool found = false;
                        for (auto bj : bld->jobs)
                            if (bj == job)
                                found = true;
                        if (!found)
                            report("{}: BUILDING_HOLDER ref to building {} but the "
                                   "building's job list does not contain it",
                                   ctx, bref->building_id);
                    }
                }
                else
                {
                    auto uref = virtual_cast<df::general_ref_unit>(ref);
                    if (uref && ref->getType() == df::general_ref_type::UNIT_WORKER)
                    {
                        if (auto unit = df::unit::find(uref->unit_id))
                            if (unit->job.current_job != job)
                                report("{}: UNIT_WORKER ref to unit {} but "
                                       "unit.job.current_job is {}", ctx,
                                       uref->unit_id,
                                       unit->job.current_job
                                           ? unit->job.current_job->id : -1);
                    }
                }
            }

            check_all_refs(job->general_refs, job->specific_refs, ctx.c_str());
        }
    }

    // ---- stage 5: squads, equipment, armies ---------------------------------

    void check_squads_and_armies()
    {
        collect_items();

        // equipment lists store item ids indexed by item_type (bug 11014,
        // fix/corrupt-equipment)
        if (plotinfo)
        {
            auto &eq = plotinfo->equipment;
            const size_t num_types = eq.items_unmanifested.size();
            auto check_eq_vec = [&](const std::vector<int32_t> &vec,
                                    const char *vec_name, df::item_type type) {
                size_t i = 0;
                for (auto item_id : vec)
                {
                    auto item = df::item::find(item_id);
                    std::string ctx = fmt::format("plotinfo.equipment.{}[{}][{}]",
                                                  vec_name, enum_item_key(type), i++);
                    if (!item)
                        report("{}: item id {} not in world.items.all "
                               "(fix/corrupt-equipment)", ctx, item_id);
                    else if (item->getType() != type)
                        report("{}: item {} has type {}, expected {}",
                               ctx, item_id, enum_item_key(item->getType()),
                               enum_item_key(type));
                }
            };
            for (size_t t = 0; t < num_types; t++)
            {
                auto type = df::item_type(t);
                check_eq_vec(eq.items_unmanifested[t], "items_unmanifested", type);
                check_eq_vec(eq.items_unassigned[t], "items_unassigned", type);
                check_eq_vec(eq.items_assigned[t], "items_assigned", type);
            }
            for (auto item_id : eq.work_weapons)
                if (!df::item::find(item_id))
                    report("plotinfo.equipment.work_weapons: dangling item id {}",
                           item_id);
            for (auto unit_id : eq.work_units)
                if (!df::unit::find(unit_id))
                    report("plotinfo.equipment.work_units: dangling unit id {}",
                           unit_id);
            for (auto item_id : eq.ammo_items)
                if (!df::item::find(item_id))
                    report("plotinfo.equipment.ammo_items: dangling item id {}",
                           item_id);
            for (auto unit_id : eq.ammo_units)
                if (!df::unit::find(unit_id))
                    report("plotinfo.equipment.ammo_units: dangling unit id {}",
                           unit_id);
        }

        // squads
        for (auto squad : world->squads.all)
        {
            if (!squad)
                continue;
            std::string ctx = fmt::format("squad {}", squad->id);
            if (squad->entity_id != -1 &&
                !df::historical_entity::find(squad->entity_id))
                report("{}: dangling entity_id {}", ctx, squad->entity_id);
            size_t i = 0;
            for (auto pos : squad->positions)
            {
                std::string pctx = fmt::format("{}.positions[{}]", ctx, i++);
                if (!pos)
                    continue;
                if (pos->occupant != -1 && !df::historical_figure::find(pos->occupant))
                    report("{}: dangling occupant hfid {}", pctx, pos->occupant);
                // assigned items may legitimately be off-map during raids;
                // only flag type mismatches for items that resolve
                for (auto item_id : pos->equipment.assigned_items)
                {
                    if (auto item = df::item::find(item_id))
                    {
                        auto t = item->getType();
                        switch (t)
                        {
                            case df::item_type::FLASK:
                            case df::item_type::WEAPON:
                            case df::item_type::ARMOR:
                            case df::item_type::SHOES:
                            case df::item_type::SHIELD:
                            case df::item_type::HELM:
                            case df::item_type::GLOVES:
                            case df::item_type::AMMO:
                            case df::item_type::PANTS:
                            case df::item_type::BACKPACK:
                            case df::item_type::QUIVER:
                                break;
                            default:
                                report("{}: assigned item {} has unexpected type {} "
                                       "(bug 11014)", pctx, item_id,
                                       enum_item_key(t));
                        }
                    }
                }
            }
        }

        // historical entity squad lists must resolve
        for (auto ent : world->entities.all)
        {
            if (!ent)
                continue;
            for (auto squad_id : ent->squads)
                if (!df::squad::find(squad_id))
                    report("entity {}: dangling squad id {}", ent->id, squad_id);
        }

        // army controller links (army-controller-sanity)
        for (auto ac : world->army_controllers.all)
            army_controllers.insert(ac);
        for (auto ent : world->entities.all)
        {
            if (!ent)
                continue;
            for (auto ac : ent->army_controllers)
            {
                if (!ac)
                    continue;
                if (!army_controllers.count(ac))
                    report("entity {}: army_controller pointer {} not in "
                           "world.army_controllers.all", ent->id,
                           static_cast<void*>(ac));
                else if (ac->entity_id != ent->id)
                    report("entity {}: army_controller {} has entity_id {}",
                           ent->id, ac->id, ac->entity_id);
            }
        }
        for (auto army : world->armies.all)
        {
            if (!army)
                continue;
            std::string ctx = fmt::format("army {}", army->id);
            auto ac = army->controller;
            if (ac)
            {
                if (!army_controllers.count(ac))
                    report("{}: controller pointer {} not in "
                           "world.army_controllers.all", ctx,
                           static_cast<void*>(ac));
                else if (ac->id != army->controller_id)
                    report("{}: controller id mismatch ({} != {})",
                           ctx, army->controller_id, ac->id);
            }
            else if (army->controller_id != -1 && army->controller_id != 0)
                report("{}: controller_id {} but controller is null "
                       "(fix/stuck-squad)", ctx, army->controller_id);
        }
    }

    // ---- stage 6: map blocks ------------------------------------------------

    void check_map()
    {
        if (!Core::getInstance().isMapLoaded())
            return;

        collect_items();
        collect_units();

        int32_t dim_x, dim_y, dim_z;
        Maps::getTileSize(dim_x, dim_y, dim_z);

        // expected occupancy, like fix-occupancy
        size_t tile_count = size_t(dim_x) * dim_y * dim_z;
        std::vector<df::tile_occupancy> expected_occ(tile_count);
        auto occ_off = [&](int32_t x, int32_t y, int32_t z) {
            return size_t(dim_x * dim_y) * z + dim_x * y + x;
        };

        // buildings claim their covered tiles (marker value only; compared
        // against actual occupancy by None-ness, like fix-occupancy)
        for (auto bld : world->buildings.all)
        {
            if (!bld || !bld->isSettingOccupancy())
                continue;
            for (int y = bld->y1; y <= bld->y2; ++y)
                for (int x = bld->x1; x <= bld->x2; ++x)
                {
                    if (x < 0 || y < 0 || x >= dim_x || y >= dim_y ||
                        bld->z < 0 || bld->z >= dim_z)
                        continue;
                    if (!Buildings::containsTile(bld, df::coord2d(x, y)))
                        continue;
                    auto &occ = expected_occ[occ_off(x, y, bld->z)];
                    if (occ.bits.building != df::tile_building_occ::None)
                        report("map tile ({}, {}, {}): building {} overlaps "
                               "another building", x, y, bld->z, bld->id);
                    occ.bits.building = df::tile_building_occ::Impassable;
                }
        }

        // units claim their tiles (wagons cover a 3x3 area)
        for (auto unit : world->units.active)
        {
            // skip entries not in units.all (already reported in stage 0);
            // they may be dangling
            if (!unit || !units.count(unit) || unit->flags1.bits.caged ||
                unit->flags1.bits.inactive || unit->flags1.bits.rider)
                continue;
            bool wagon = false;
            if (auto craw = df::creature_raw::find(unit->race))
                wagon = craw->flags.is_set(df::creature_raw_flags::EQUIPMENT_WAGON);
            int r = wagon ? 1 : 0;
            for (int y = unit->pos.y - r; y <= unit->pos.y + r; ++y)
                for (int x = unit->pos.x - r; x <= unit->pos.x + r; ++x)
                {
                    if (x < 0 || y < 0 || x >= dim_x || y >= dim_y ||
                        unit->pos.z < 0 || unit->pos.z >= dim_z)
                        continue;
                    auto &occ = expected_occ[occ_off(x, y, unit->pos.z)];
                    if (unit->flags1.bits.on_ground)
                        occ.bits.unit_grounded = true;
                    else
                        occ.bits.unit = true;
                }
        }

        // on-ground items claim their tiles and their map blocks
        std::map<df::map_block*, std::set<int32_t>> expected_block_items;
        for (auto item : world->items.other.IN_PLAY)
        {
            // skip entries not in items.all (already reported in stage 0);
            // they may be dangling
            if (!item || !items.count(item) || !item->flags.bits.on_ground)
                continue;
            auto pos = Items::getPosition(item);
            if (!pos.isValid())
                continue;
            if (auto block = Maps::getTileBlock(pos))
                expected_block_items[block].insert(item->id);
            if (pos.x >= 0 && pos.x < dim_x && pos.y >= 0 && pos.y < dim_y &&
                pos.z >= 0 && pos.z < dim_z)
                expected_occ[occ_off(pos.x, pos.y, pos.z)].bits.item = true;
        }

        // building occupancy is compared by None-ness only (the exact
        // tile_building_occ value depends on the building type)
        const uint32_t occ_mask = df::tile_occupancy::mask_unit |
            df::tile_occupancy::mask_unit_grounded | df::tile_occupancy::mask_item;

        for (auto block : world->map.map_blocks)
        {
            if (!block)
                continue;
            std::string bctx = fmt::format("map block ({}, {}, {})",
                                           block->map_pos.x, block->map_pos.y,
                                           block->map_pos.z);

            // block->items must be sorted (fix-occupancy) and resolve to
            // items actually located inside the block
            int64_t prev_id = -1;
            for (auto item_id : block->items)
            {
                if (item_id < prev_id)
                    report("{}: item list is not sorted (id {} follows {})",
                           bctx, item_id, prev_id);
                prev_id = item_id;
                auto item = df::item::find(item_id);
                if (!item)
                    report("{}: item id {} not in world.items.all", bctx, item_id);
                else
                {
                    auto pos = Items::getPosition(item);
                    if (pos.isValid() &&
                        (pos.x < block->map_pos.x ||
                         pos.x >= block->map_pos.x + 16 ||
                         pos.y < block->map_pos.y ||
                         pos.y >= block->map_pos.y + 16 ||
                         pos.z != block->map_pos.z))
                        report("{}: item {} is at ({}, {}, {}), outside this block",
                               bctx, item_id, pos.x, pos.y, pos.z);
                }
            }

            // block items must match the set of on-ground items in the block
            auto it = expected_block_items.find(block);
            if (it != expected_block_items.end())
            {
                std::vector<int32_t> expected(it->second.begin(), it->second.end());
                if (expected != block->items)
                    report("{}: item list does not match the set of on-ground "
                           "items (fix/occupancy)", bctx);
            }
            else if (!block->items.empty())
                report("{}: {} stale item references", bctx, block->items.size());

            // occupancy bits must match expectations
            int z = block->map_pos.z;
            for (int yoff = 0; yoff < 16; ++yoff)
            {
                int y = block->map_pos.y + yoff;
                if (y < 0 || y >= dim_y)
                    continue;
                for (int xoff = 0; xoff < 16; ++xoff)
                {
                    int x = block->map_pos.x + xoff;
                    if (x < 0 || x >= dim_x)
                        continue;
                    auto &exp = expected_occ[occ_off(x, y, z)];
                    auto &act = block->occupancy[xoff][yoff];
                    bool exp_bld = exp.bits.building != df::tile_building_occ::None;
                    bool act_bld = act.bits.building != df::tile_building_occ::None;
                    if (exp_bld == act_bld &&
                        (exp.whole & occ_mask) == (act.whole & occ_mask))
                        continue;
                    if (!exp.bits.item && act.bits.item)
                        report("{}: tile ({}, {}, {}) has item occupancy but no "
                               "on-ground item", bctx, x, y, z);
                    if (exp.bits.item && !act.bits.item)
                        report("{}: tile ({}, {}, {}) lacks item occupancy",
                               bctx, x, y, z);
                    if (!exp.bits.unit && act.bits.unit)
                        report("{}: tile ({}, {}, {}) has unit occupancy but no "
                               "standing unit", bctx, x, y, z);
                    if (exp.bits.unit && !act.bits.unit)
                        report("{}: tile ({}, {}, {}) lacks unit occupancy",
                               bctx, x, y, z);
                    if (!exp.bits.unit_grounded && act.bits.unit_grounded)
                        report("{}: tile ({}, {}, {}) has grounded unit occupancy "
                               "but no grounded unit", bctx, x, y, z);
                    if (exp.bits.unit_grounded && !act.bits.unit_grounded)
                        report("{}: tile ({}, {}, {}) lacks grounded unit "
                               "occupancy", bctx, x, y, z);
                    if (!exp_bld && act_bld)
                        report("{}: tile ({}, {}, {}) has building occupancy but "
                               "no building covers it", bctx, x, y, z);
                    if (exp_bld && !act_bld)
                        report("{}: tile ({}, {}, {}) lacks building occupancy",
                               bctx, x, y, z);
                }
            }
        }

        // engraving flags must match the tile they are on (fix/engravings)
        for (auto eng : world->event.engravings)
        {
            if (!eng || !eng->pos.isValid())
                continue;
            auto tt = Maps::getTileType(eng->pos);
            if (!tt)
                continue;
            auto special = ENUM_ATTR(tiletype, special, *tt);
            auto shape = ENUM_ATTR(tiletype, shape, *tt);
            if (special != df::tiletype_special::SMOOTH)
            {
                report("engraving at ({}, {}, {}) is not on a smooth tile",
                       eng->pos.x, eng->pos.y, eng->pos.z);
                continue;
            }
            if (shape == df::tiletype_shape::FLOOR && !eng->flags.bits.floor)
                report("engraving at ({}, {}, {}) is on a floor but flags.floor "
                       "is unset", eng->pos.x, eng->pos.y, eng->pos.z);
            else if (shape == df::tiletype_shape::WALL && eng->flags.bits.floor)
                report("engraving at ({}, {}, {}) is on a wall but flags.floor "
                       "is set", eng->pos.x, eng->pos.y, eng->pos.z);
        }
    }

    // ---- driver ------------------------------------------------------------

    int num_stages() { return opt_deep ? 7 : 6; }

    void run_stage(size_t stage)
    {
        switch (stage)
        {
            case 0: check_sorted_vectors(); check_other_vectors(); break;
            case 1: check_units(); break;
            case 2: check_items(); break;
            case 3: check_buildings(); break;
            case 4: check_jobs(); break;
            case 5: check_squads_and_armies(); break;
            case 6: check_map(); break;
        }
    }

    void run_all()
    {
        for (size_t i = 0; i < (size_t)num_stages(); i++)
            run_stage(i);
    }
};

static const char *stage_names[] = {
    "vectors", "units", "items", "buildings", "jobs", "squads/armies", "map"
};

// ---- command handling ------------------------------------------------------

static command_result command(color_ostream &out, std::vector<std::string> &parameters)
{
    std::string sub = parameters.empty() ? "now" : parameters[0];

    if (sub == "now" || sub == "once")
    {
        if (!Core::getInstance().isWorldLoaded())
        {
            out.printerr("consistency-check: no world loaded\n");
            return CR_FAILURE;
        }
        Scanner scanner(out);
        scanner.run_all();
        out.print("consistency-check: done, {} new issue(s), {} total this session\n",
                  scanner.errors, total_report_count);
        return CR_OK;
    }
    else if (sub == "start")
    {
        is_enabled = true;
        out.print("consistency-check: periodic scanning enabled (interval {} ticks)\n",
                  scan_interval);
        return CR_OK;
    }
    else if (sub == "stop")
    {
        is_enabled = false;
        out.print("consistency-check: periodic scanning disabled\n");
        return CR_OK;
    }
    else if (sub == "status")
    {
        out.print("consistency-check: {}, interval {} ticks, deep {}, "
                  "{} issue(s) reported this session, next stage {}\n",
                  is_enabled ? "enabled" : "disabled", scan_interval,
                  opt_deep ? "on" : "off", total_report_count,
                  stage_names[scan_stage]);
        return CR_OK;
    }
    else if (sub == "interval")
    {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;
        try
        {
            scan_interval = std::stoi(parameters[1]);
        }
        catch (std::exception &)
        {
            return CR_WRONG_USAGE;
        }
        if (scan_interval < 1)
            scan_interval = 1;
        out.print("consistency-check: interval set to {} ticks\n", scan_interval);
        return CR_OK;
    }
    else if (sub == "deep")
    {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;
        if (parameters[1] == "on")
            opt_deep = true;
        else if (parameters[1] == "off")
            opt_deep = false;
        else
            return CR_WRONG_USAGE;
        out.print("consistency-check: deep checks {}\n", opt_deep ? "on" : "off");
        return CR_OK;
    }
    else if (sub == "reset")
    {
        seen_reports.clear();
        total_report_count = 0;
        out.print("consistency-check: report history cleared\n");
        return CR_OK;
    }
    else if (sub == "help")
    {
        return CR_WRONG_USAGE; // prints usage text
    }
    return CR_WRONG_USAGE;
}

DFhackCExport command_result plugin_init(color_ostream &, std::vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "devel/consistency-check",
        "periodically check DF data structures for violated invariants",
        command,
        false,
        false,
        "devel/consistency-check [now|once]   run all checks immediately\n"
        "devel/consistency-check start|stop   enable/disable periodic scanning\n"
        "devel/consistency-check status       show scanning state\n"
        "devel/consistency-check interval <n> game ticks between checks "
            "(default 1200)\n"
        "devel/consistency-check deep on|off  include map block/occupancy checks\n"
        "devel/consistency-check reset        clear reported-issue history\n"
        "\n"
        "When enabled ('enable consistency-check' or 'devel/consistency-check "
        "start'),\n"
        "one group of checks runs every interval: vectors, units, items,\n"
        "buildings, jobs, squads/armies, and (with deep on) map blocks.\n"
        "Each distinct issue is only reported once per session.\n"
    ));
    commands.push_back(PluginCommand(
        "consistency-check",
        "periodically check DF data structures for violated invariants",
        command));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &, bool enable)
{
    is_enabled = enable;
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &, state_change_event event)
{
    // a freshly loaded save may carry new corruption; report everything again
    if (event == SC_MAP_LOADED)
    {
        seen_reports.clear();
        total_report_count = 0;
        scan_stage = 0;
        last_scan_tick = -1;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (!is_enabled || !Core::getInstance().isWorldLoaded())
        return CR_OK;

    int64_t now = current_tick();
    if (last_scan_tick >= 0 && now - last_scan_tick < scan_interval)
        return CR_OK;
    last_scan_tick = now;

    Scanner scanner(out);
    if (scan_stage >= (size_t)scanner.num_stages())
        scan_stage = 0;
    scanner.run_stage(scan_stage);
    if (scanner.errors)
    {
        WARN(log, out).print("consistency-check: {} new issue(s) in {} stage "
                             "(see 'consistency-check reset' to re-report)\n",
                             scanner.errors, stage_names[scan_stage]);
    }
    scan_stage = (scan_stage + 1) % scanner.num_stages();
    return CR_OK;
}
