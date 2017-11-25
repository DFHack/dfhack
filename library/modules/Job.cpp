/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
#include <cassert>
using namespace std;

#include "Core.h"
#include "Error.h"
#include "PluginManager.h"
#include "MiscUtils.h"
#include "Types.h"

#include "modules/Job.h"
#include "modules/Materials.h"
#include "modules/Items.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/building.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/job_list_link.h"
#include "df/specific_ref.h"
#include "df/general_ref.h"
#include "df/general_ref_unit_workerst.h"
#include "df/general_ref_building_holderst.h"
#include "df/interface_button_building_new_jobst.h"

using namespace DFHack;
using namespace df::enums;

df::job *DFHack::Job::cloneJobStruct(df::job *job, bool keepEverything)
{
    CHECK_NULL_POINTER(job);

    df::job *pnew = new df::job(*job);

    if ( !keepEverything ) {
        // Clean out transient fields
        pnew->flags.whole = 0;
        pnew->flags.bits.repeat = job->flags.bits.repeat;
        pnew->flags.bits.suspend = job->flags.bits.suspend;

        pnew->completion_timer = -1;
        pnew->posting_index = -1;
    }
    pnew->list_link = NULL;

    //pnew->items.clear();
    //pnew->specific_refs.clear();
    pnew->general_refs.clear();
    //pnew->job_items.clear();

    if ( keepEverything ) {
        for ( size_t a = 0; a < pnew->items.size(); a++ )
            pnew->items[a] = new df::job_item_ref(*pnew->items[a]);
        for ( size_t a = 0; a < pnew->specific_refs.size(); a++ )
            pnew->specific_refs[a] = new df::specific_ref(*pnew->specific_refs[a]);
    } else {
        pnew->items.clear();
        pnew->specific_refs.clear();
    }

    for ( size_t a = 0; a < pnew->job_items.size(); a++ )
        pnew->job_items[a] = new df::job_item(*pnew->job_items[a]);

    for ( size_t a = 0; a < job->general_refs.size(); a++ )
        if ( keepEverything || job->general_refs[a]->getType() != general_ref_type::UNIT_WORKER )
            pnew->general_refs.push_back(job->general_refs[a]->clone());

    return pnew;
}

void DFHack::Job::deleteJobStruct(df::job *job, bool keptEverything)
{
    if (!job)
        return;

    // Only allow free-floating job structs
    if ( !keptEverything )
        assert(!job->list_link && job->items.empty() && job->specific_refs.empty());
    else
        assert(!job->list_link);

    if ( keptEverything ) {
        for ( size_t a = 0; a < job->items.size(); a++ )
            delete job->items[a];
        for ( size_t a = 0; a < job->specific_refs.size(); a++ )
            delete job->specific_refs[a];
    }
    for ( size_t a = 0; a < job->job_items.size(); a++ )
        delete job->job_items[a];
    for ( size_t a = 0; a < job->general_refs.size(); a++ )
        delete job->general_refs[a];

    delete job;
}

#define CMP(field) (a.field == b.field)

bool DFHack::operator== (const df::job_item &a, const df::job_item &b)
{
    CHECK_NULL_POINTER(&a);
    CHECK_NULL_POINTER(&b);

    if (!(CMP(item_type) && CMP(item_subtype) &&
          CMP(mat_type) && CMP(mat_index) &&
          CMP(flags1.whole) && CMP(quantity) && CMP(vector_id) &&
          CMP(flags2.whole) && CMP(flags3.whole) &&
          CMP(metal_ore) && CMP(reaction_class) &&
          CMP(has_material_reaction_product) &&
          CMP(min_dimension) && CMP(reagent_index) &&
          CMP(reaction_id) && CMP(has_tool_use) &&
          CMP(contains.size())))
        return false;

    for (int i = a.contains.size()-1; i >= 0; i--)
        if (a.contains[i] != b.contains[i])
            return false;

    return true;
}

bool DFHack::operator== (const df::job &a, const df::job &b)
{
    CHECK_NULL_POINTER(&a);
    CHECK_NULL_POINTER(&b);

    if (!(CMP(job_type) && CMP(job_subtype) &&
          CMP(mat_type) && CMP(mat_index) &&
          CMP(item_subtype) && CMP(item_category.whole) &&
          CMP(hist_figure_id) && CMP(material_category.whole) &&
          CMP(reaction_name) && CMP(job_items.size())))
        return false;

    for (int i = a.job_items.size()-1; i >= 0; i--)
        if (!(*a.job_items[i] == *b.job_items[i]))
            return false;

    return true;
}

void DFHack::Job::printItemDetails(color_ostream &out, df::job_item *item, int idx)
{
    CHECK_NULL_POINTER(item);

    ItemTypeInfo info(item);
    out << "  Input Item " << (idx+1) << ": " << info.toString();

    if (item->quantity != 1)
        out << "; quantity=" << item->quantity;
    if (item->min_dimension >= 0)
        out << "; min_dimension=" << item->min_dimension;
    out << endl;

    MaterialInfo mat(item);
    if (mat.isValid() || item->metal_ore >= 0) {
        out << "    material: " << mat.toString();
        if (item->metal_ore >= 0)
            out << "; ore of " << MaterialInfo(0,item->metal_ore).toString();
        out << endl;
    }

    if (item->flags1.whole)
        out << "    flags1: " << bitfield_to_string(item->flags1) << endl;
    if (item->flags2.whole)
        out << "    flags2: " << bitfield_to_string(item->flags2) << endl;
    if (item->flags3.whole)
        out << "    flags3: " << bitfield_to_string(item->flags3) << endl;

    if (!item->reaction_class.empty())
        out << "    reaction class: " << item->reaction_class << endl;
    if (!item->has_material_reaction_product.empty())
        out << "    reaction product: " << item->has_material_reaction_product << endl;
    if (item->has_tool_use >= (df::tool_uses)0)
        out << "    tool use: " << ENUM_KEY_STR(tool_uses, item->has_tool_use) << endl;
}

void DFHack::Job::printJobDetails(color_ostream &out, df::job *job)
{
    CHECK_NULL_POINTER(job);

    out.color(job->flags.bits.suspend ? COLOR_DARKGREY : COLOR_GREY);
    out << "Job " << job->id << ": " << ENUM_KEY_STR(job_type,job->job_type);
    if (job->flags.whole)
        out << " (" << bitfield_to_string(job->flags) << ")";
    out << endl;
    out.reset_color();

    df::item_type itype = ENUM_ATTR(job_type, item, job->job_type);

    MaterialInfo mat(job);
    if (itype == item_type::FOOD)
        mat.decode(-1);

    if (mat.isValid() || job->material_category.whole)
    {
        out << "    material: " << mat.toString();
        if (job->material_category.whole)
            out << " (" << bitfield_to_string(job->material_category) << ")";
        out << endl;
    }

    if (job->item_subtype >= 0 || job->item_category.whole)
    {
        ItemTypeInfo iinfo(itype, job->item_subtype);

        out << "    item: " << iinfo.toString()
               << " (" << bitfield_to_string(job->item_category) << ")" << endl;
    }

    if (job->hist_figure_id >= 0)
        out << "    figure: " << job->hist_figure_id << endl;

    if (!job->reaction_name.empty())
        out << "    reaction: " << job->reaction_name << endl;

    for (size_t i = 0; i < job->job_items.size(); i++)
        printItemDetails(out, job->job_items[i], i);
}

df::general_ref *Job::getGeneralRef(df::job *job, df::general_ref_type type)
{
    CHECK_NULL_POINTER(job);

    return findRef(job->general_refs, type);
}

df::specific_ref *Job::getSpecificRef(df::job *job, df::specific_ref_type type)
{
    CHECK_NULL_POINTER(job);

    return findRef(job->specific_refs, type);
}

df::building *DFHack::Job::getHolder(df::job *job)
{
    CHECK_NULL_POINTER(job);

    auto ref = getGeneralRef(job, general_ref_type::BUILDING_HOLDER);

    return ref ? ref->getBuilding() : NULL;
}

df::unit *DFHack::Job::getWorker(df::job *job)
{
    CHECK_NULL_POINTER(job);

    auto ref = getGeneralRef(job, general_ref_type::UNIT_WORKER);

    return ref ? ref->getUnit() : NULL;
}

void DFHack::Job::setJobCooldown(df::building *workshop, df::unit *worker, int cooldown)
{
    CHECK_NULL_POINTER(workshop);
    CHECK_NULL_POINTER(worker);

    if (cooldown <= 0)
        return;

    int idx = linear_index(workshop->job_claim_suppress, &df::building::T_job_claim_suppress::unit, worker);

    if (idx < 0)
    {
        auto obj = new df::building::T_job_claim_suppress;
        obj->unit = worker;
        obj->timer = cooldown;
        workshop->job_claim_suppress.push_back(obj);
    }
    else
    {
        auto obj = workshop->job_claim_suppress[idx];
        obj->timer = std::max(obj->timer, cooldown);
    }
}

void DFHack::Job::disconnectJobItem(df::job *job, df::job_item_ref *ref) {
    if (!ref) return;

    auto item = ref->item;
    if (!item) return;

    //Work backward through the specific refs & remove/delete all specific refs to this job
    int refCount = item->specific_refs.size();
    bool stillHasJobs = false;
    for(int refIndex = refCount-1; refIndex >= 0; refIndex--) {
        auto ref = item->specific_refs[refIndex];

        if (ref->type == df::specific_ref_type::JOB) {
            if (ref->job == job) {
                vector_erase_at(item->specific_refs, refIndex);
                delete ref;
            } else {
                stillHasJobs = true;
            }
        }
    }

    if (!stillHasJobs) item->flags.bits.in_job = false;
}

bool DFHack::Job::disconnectJobGeneralRef(df::job *job, df::general_ref *ref) {
    if (ref == NULL) return true;

    df::building *building = NULL;
    df::unit *unit = NULL;

    switch (ref->getType()) {
    case general_ref_type::BUILDING_HOLDER:
        building = ref->getBuilding();

        if (building != NULL) {
            int jobIndex = linear_index(building->jobs, job);
            if (jobIndex >= 0) {
                vector_erase_at(building->jobs, jobIndex);
            }
        }
        break;
    case general_ref_type::UNIT_WORKER:
        unit = ref->getUnit();

        if (unit != NULL) {
            if (unit->job.current_job == job) {
                unit->job.current_job = NULL;
            }
        }
        break;
    default:
        return false;
    }

    return true;
}

bool DFHack::Job::removeJob(df::job *job) {
    using df::global::world;
    CHECK_NULL_POINTER(job);

    if (job->flags.bits.special) //I don't think you can cancel these, because DF wasn't build to expect it?
        return false;

    //We actually only know how to handle BUILDING_HOLDER and UNIT_WORKER refs- there's probably a great
    //way to handle them, but until we have a good example, we'll just fail to remove jobs that have other sorts
    //of refs, or any specific refs
    if (job->specific_refs.size() > 0)
        return false;

    for (auto genRefItr = job->general_refs.begin(); genRefItr != job->general_refs.end(); ++genRefItr) {
        auto ref = *genRefItr;
        if (ref != NULL && (ref->getType() != general_ref_type::BUILDING_HOLDER && ref->getType() != general_ref_type::UNIT_WORKER))
            return false;
    }

    //Disconnect, delete, and wipe all general refs
    while (job->general_refs.size() > 0) {
        auto ref = job->general_refs[0];

        //Our code above should have ensured that this won't return false- if it does, there's not
        //a great way of recovering since we can't properly destroy the job & we can't leave it
        //around.  Better to know the moment that becomes a problem.
        bool success = disconnectJobGeneralRef(job, ref);
        assert(success);

        vector_erase_at(job->general_refs, 0);
        if (ref != NULL) delete ref;
    }

    //Detach all items from the job
    while (job->items.size() > 0) {
        auto itemRef = job->items[0];
        disconnectJobItem(job, itemRef);
        vector_erase_at(job->items, 0);
        if (itemRef != NULL) delete itemRef;
    }

    //Remove job from job board
    Job::removePostings(job, true);

    //Clean up job_items
    while (job->job_items.size() > 0) {
        auto jobItem = job->job_items[0];
        vector_erase_at(job->job_items, 0);
        if (jobItem) {
            delete jobItem;
        }
    }

    //Remove job from global list
    if (job->list_link) {
        auto prev = job->list_link->prev;
        auto next = job->list_link->next;

        if (prev)
            prev->next = next;

        if (next)
            next->prev = prev;

        delete job->list_link;
    }

    delete job;
    return true;
}

bool DFHack::Job::removeWorker(df::job *job, int cooldown)
{
    CHECK_NULL_POINTER(job);

    if (job->flags.bits.special)
        return false;

    auto holder = getHolder(job);
    if (!holder || linear_index(holder->jobs,job) < 0)
        return false;

    for (size_t i = 0; i < job->general_refs.size(); i++)
    {
        df::general_ref *ref = job->general_refs[i];
        if (ref->getType() != general_ref_type::UNIT_WORKER)
            continue;

        auto worker = ref->getUnit();
        if (!worker || worker->job.current_job != job)
            return false;

        setJobCooldown(holder, worker, cooldown);

        vector_erase_at(job->general_refs, i);
        worker->job.current_job = NULL;
        delete ref;

        return true;
    }

    return false;
}

void DFHack::Job::checkBuildingsNow()
{
    if (df::global::process_jobs)
        *df::global::process_jobs = true;
}

void DFHack::Job::checkDesignationsNow()
{
    if (df::global::process_dig)
        *df::global::process_dig = true;
}

bool DFHack::Job::linkIntoWorld(df::job *job, bool new_id)
{
    using df::global::world;
    using df::global::job_next_id;

    assert(!job->list_link);

    if (new_id) {
        job->id = (*job_next_id)++;

        job->list_link = new df::job_list_link();
        job->list_link->item = job;
        linked_list_append(&world->jobs.list, job->list_link);
        return true;
    } else {
        df::job_list_link *ins_pos = &world->jobs.list;
        while (ins_pos->next && ins_pos->next->item->id < job->id)
            ins_pos = ins_pos->next;

        if (ins_pos->next && ins_pos->next->item->id == job->id)
            return false;

        job->list_link = new df::job_list_link();
        job->list_link->item = job;
        linked_list_insert_after(ins_pos, job->list_link);
        return true;
    }
}

bool DFHack::Job::removePostings(df::job *job, bool remove_all)
{
    using df::global::world;
    CHECK_NULL_POINTER(job);
    bool removed = false;
    if (!remove_all)
    {
        if (job->posting_index >= 0 && job->posting_index < world->jobs.postings.size())
        {
            world->jobs.postings[job->posting_index]->flags.bits.dead = true;
            removed = true;
        }
    }
    else
    {
        for (auto it = world->jobs.postings.begin(); it != world->jobs.postings.end(); ++it)
        {
            if ((**it).job == job)
            {
                (**it).job = NULL;
                (**it).flags.bits.dead = true;
                removed = true;
            }
        }
    }
    job->posting_index = -1;
    return removed;
}

bool DFHack::Job::listNewlyCreated(std::vector<df::job*> *pvec, int *id_var)
{
    using df::global::world;
    using df::global::job_next_id;

    pvec->clear();

    if (!job_next_id || *job_next_id <= *id_var)
        return false;

    int old_id = *id_var;
    int cur_id = *job_next_id;

    *id_var = cur_id;

    pvec->reserve(std::min(20,cur_id - old_id));

    df::job_list_link *link = world->jobs.list.next;
    for (; link; link = link->next)
    {
        int id = link->item->id;
        if (id >= old_id)
            pvec->push_back(link->item);
    }

    return true;
}

bool DFHack::Job::attachJobItem(df::job *job, df::item *item,
                                df::job_item_ref::T_role role,
                                int filter_idx, int insert_idx)
{
    CHECK_NULL_POINTER(job);
    CHECK_NULL_POINTER(item);

    /*
     * Functionality 100% reverse-engineered from DF code.
     */

    if (role != df::job_item_ref::TargetContainer)
    {
        if (item->flags.bits.in_job)
            return false;

        item->flags.bits.in_job = true;
    }

    auto item_link = new df::specific_ref();
    item_link->type = specific_ref_type::JOB;
    item_link->job = job;
    item->specific_refs.push_back(item_link);

    auto job_link = new df::job_item_ref();
    job_link->item = item;
    job_link->role = role;
    job_link->job_item_idx = filter_idx;

    if (size_t(insert_idx) < job->items.size())
        vector_insert_at(job->items, insert_idx, job_link);
    else
        job->items.push_back(job_link);

    return true;
}

bool Job::isSuitableItem(df::job_item *item, df::item_type itype, int isubtype)
{
    CHECK_NULL_POINTER(item);

    if (itype == item_type::NONE)
        return true;

    ItemTypeInfo iinfo(itype, isubtype);
    MaterialInfo minfo(item);

    return iinfo.isValid() && iinfo.matches(*item, &minfo);
}

bool Job::isSuitableMaterial(df::job_item *item, int mat_type, int mat_index)
{
    CHECK_NULL_POINTER(item);

    if (mat_type == -1 && mat_index == -1)
        return true;

    ItemTypeInfo iinfo(item);
    MaterialInfo minfo(mat_type, mat_index);

    return minfo.isValid() && iinfo.matches(*item, &minfo);
}

std::string Job::getName(df::job *job)
{
    CHECK_NULL_POINTER(job);

    std::string desc;
    auto button = df::allocate<df::interface_button_building_new_jobst>();
    button->reaction_name = job->reaction_name;
    button->hist_figure_id = job->hist_figure_id;
    button->job_type = job->job_type;
    button->item_type = job->item_type;
    button->item_subtype = job->item_subtype;
    button->mat_type = job->mat_type;
    button->mat_index = job->mat_index;
    button->item_category = job->item_category;
    button->material_category = job->material_category;

    button->getLabel(&desc);
    delete button;

    return desc;
}
