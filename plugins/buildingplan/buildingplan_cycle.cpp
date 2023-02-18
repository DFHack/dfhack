#include "plannedbuilding.h"
#include "buildingplan.h"

#include "Debug.h"

#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Materials.h"

#include "df/building_design.h"
#include "df/item.h"
#include "df/job.h"
#include "df/world.h"

#include <unordered_map>

using std::map;
using std::string;
using std::unordered_map;

namespace DFHack {
    DBG_EXTERN(buildingplan, cycle);
}

using namespace DFHack;

struct BadFlags {
    uint32_t whole;

    BadFlags() {
        df::item_flags flags;
        #define F(x) flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(in_job);
        F(owned); F(in_chest); F(removed); F(encased);
        F(spider_web);
        #undef F
        whole = flags.whole;
    }
};

bool itemPassesScreen(df::item * item) {
    static const BadFlags bad_flags;
    return !(item->flags.whole & bad_flags.whole)
        && !item->isAssignedToStockpile();
}

bool matchesFilters(df::item * item, df::job_item * job_item) {
    // check the properties that are not checked by Job::isSuitableItem()
    if (job_item->item_type > -1 && job_item->item_type != item->getType())
        return false;

    if (job_item->item_subtype > -1 &&
        job_item->item_subtype != item->getSubtype())
        return false;

    if (job_item->flags2.bits.building_material && !item->isBuildMat())
        return false;

    if (job_item->metal_ore > -1 && !item->isMetalOre(job_item->metal_ore))
        return false;

    if (job_item->has_tool_use > df::tool_uses::NONE
        && !item->hasToolUse(job_item->has_tool_use))
        return false;

    return Job::isSuitableItem(
            job_item, item->getType(), item->getSubtype())
        && Job::isSuitableMaterial(
            job_item, item->getMaterial(), item->getMaterialIndex(),
            item->getType());
}

bool isJobReady(color_ostream &out, const std::vector<df::job_item *> &jitems) {
    int needed_items = 0;
    for (auto job_item : jitems) { needed_items += job_item->quantity; }
    if (needed_items) {
        DEBUG(cycle,out).print("building needs %d more item(s)\n", needed_items);
        return false;
    }
    return true;
}

static bool job_item_idx_lt(df::job_item_ref *a, df::job_item_ref *b) {
    // we want the items in the opposite order of the filters
    return a->job_item_idx > b->job_item_idx;
}

// this function does not remove the job_items since their quantity fields are
// now all at 0, so there is no risk of having extra items attached. we don't
// remove them to keep the "finalize with buildingplan active" path as similar
// as possible to the "finalize with buildingplan disabled" path.
void finalizeBuilding(color_ostream &out, df::building *bld) {
    DEBUG(cycle,out).print("finalizing building %d\n", bld->id);
    auto job = bld->jobs[0];

    // sort the items so they get added to the structure in the correct order
    std::sort(job->items.begin(), job->items.end(), job_item_idx_lt);

    // derive the material properties of the building and job from the first
    // applicable item. if any boulders are involved, it makes the whole
    // structure "rough".
    bool rough = false;
    for (auto attached_item : job->items) {
        df::item *item = attached_item->item;
        rough = rough || item->getType() == df::item_type::BOULDER;
        if (bld->mat_type == -1) {
            bld->mat_type = item->getMaterial();
            job->mat_type = bld->mat_type;
        }
        if (bld->mat_index == -1) {
            bld->mat_index = item->getMaterialIndex();
            job->mat_index = bld->mat_index;
        }
    }

    if (bld->needsDesign()) {
        auto act = (df::building_actual *)bld;
        if (!act->design)
            act->design = new df::building_design();
        act->design->flags.bits.rough = rough;
    }

    // we're good to go!
    job->flags.bits.suspend = false;
    Job::checkBuildingsNow();
}

static df::building * popInvalidTasks(color_ostream &out, Bucket &task_queue,
        unordered_map<int32_t, PlannedBuilding> &planned_buildings) {
    while (!task_queue.empty()) {
        auto & task = task_queue.front();
        auto id = task.first;
        if (planned_buildings.count(id) > 0) {
            auto bld = planned_buildings.at(id).getBuildingIfValidOrRemoveIfNot(out);
            if (bld && bld->jobs[0]->job_items[task.second]->quantity)
                return bld;
        }
        DEBUG(cycle,out).print("discarding invalid task: bld=%d, job_item_idx=%d\n", id, task.second);
        task_queue.pop_front();
    }
    return NULL;
}

static void doVector(color_ostream &out, df::job_item_vector_id vector_id,
        map<string, Bucket> &buckets,
        unordered_map<int32_t, PlannedBuilding> &planned_buildings) {
    auto other_id = ENUM_ATTR(job_item_vector_id, other, vector_id);
    auto item_vector = df::global::world->items.other[other_id];
    DEBUG(cycle,out).print("matching %zu item(s) in vector %s against %zu filter bucket(s)\n",
          item_vector.size(),
          ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
          buckets.size());
    for (auto item_it = item_vector.rbegin();
            item_it != item_vector.rend();
            ++item_it) {
        auto item = *item_it;
        if (!itemPassesScreen(item))
            continue;
        for (auto bucket_it = buckets.begin(); bucket_it != buckets.end(); ) {
            auto & task_queue = bucket_it->second;
            auto bld = popInvalidTasks(out, task_queue, planned_buildings);
            if (!bld) {
                DEBUG(cycle,out).print("removing empty bucket: %s/%s; %zu bucket(s) left\n",
                      ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                      bucket_it->first.c_str(),
                      buckets.size() - 1);
                bucket_it = buckets.erase(bucket_it);
                continue;
            }
            auto & task = task_queue.front();
            auto id = task.first;
            auto job = bld->jobs[0];
            auto filter_idx = task.second;
            if (matchesFilters(item, job->job_items[filter_idx])
               && Job::attachJobItem(job, item,
                        df::job_item_ref::Hauled, filter_idx))
            {
                MaterialInfo material;
                material.decode(item);
                ItemTypeInfo item_type;
                item_type.decode(item);
                DEBUG(cycle,out).print("attached %s %s to filter %d for %s(%d): %s/%s\n",
                      material.toString().c_str(),
                      item_type.toString().c_str(),
                      filter_idx,
                      ENUM_KEY_STR(building_type, bld->getType()).c_str(),
                      id,
                      ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                      bucket_it->first.c_str());
                // keep quantity aligned with the actual number of remaining
                // items so if buildingplan is turned off, the building will
                // be completed with the correct number of items.
                --job->job_items[filter_idx]->quantity;
                task_queue.pop_front();
                if (isJobReady(out, job->job_items)) {
                    finalizeBuilding(out, bld);
                    planned_buildings.at(id).remove(out);
                }
                if (task_queue.empty()) {
                    DEBUG(cycle,out).print(
                        "removing empty item bucket: %s/%s; %zu left\n",
                        ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                        bucket_it->first.c_str(),
                        buckets.size() - 1);
                    buckets.erase(bucket_it);
                }
                // we found a home for this item; no need to look further
                break;
            }
            ++bucket_it;
        }
        if (buckets.empty())
            break;
    }
}

struct VectorsToScanLast {
    std::vector<df::job_item_vector_id> vectors;
    VectorsToScanLast() {
        // order is important here. we want to match boulders before wood and
        // everything before bars. blocks are not listed here since we'll have
        // already scanned them when we did the first pass through the buckets.
        vectors.push_back(df::job_item_vector_id::BOULDER);
        vectors.push_back(df::job_item_vector_id::WOOD);
        vectors.push_back(df::job_item_vector_id::BAR);
    }
};

void buildingplan_cycle(color_ostream &out, Tasks &tasks,
        unordered_map<int32_t, PlannedBuilding> &planned_buildings) {
    static const VectorsToScanLast vectors_to_scan_last;

    DEBUG(cycle,out).print(
            "running buildingplan cycle for %zu registered buildings\n",
            planned_buildings.size());

    for (auto it = tasks.begin(); it != tasks.end(); ) {
        auto vector_id = it->first;
        // we could make this a set, but it's only three elements
        if (std::find(vectors_to_scan_last.vectors.begin(),
                      vectors_to_scan_last.vectors.end(),
                      vector_id) != vectors_to_scan_last.vectors.end()) {
            ++it;
            continue;
        }

        auto & buckets = it->second;
        doVector(out, vector_id, buckets, planned_buildings);
        if (buckets.empty()) {
            DEBUG(cycle,out).print("removing empty vector: %s; %zu vector(s) left\n",
                  ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                  tasks.size() - 1);
            it = tasks.erase(it);
        }
        else
            ++it;
    }
    for (auto vector_id : vectors_to_scan_last.vectors) {
        if (tasks.count(vector_id) == 0)
            continue;
        auto & buckets = tasks[vector_id];
        doVector(out, vector_id, buckets, planned_buildings);
        if (buckets.empty()) {
            DEBUG(cycle,out).print("removing empty vector: %s; %zu vector(s) left\n",
                  ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                  tasks.size() - 1);
            tasks.erase(vector_id);
        }
    }
    DEBUG(cycle,out).print("cycle done; %zu registered building(s) left\n",
          planned_buildings.size());
}
