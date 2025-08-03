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

#include "Core.h"
#include "Error.h"
#include "PluginManager.h"
#include "MiscUtils.h"
#include "Types.h"
#include "DataDefs.h"

#include "modules/Job.h"
#include "modules/Materials.h"
#include "modules/Items.h"
#include "modules/References.h"

#include "df/building.h"
#include "df/building_workshopst.h"
#include "df/general_ref.h"
#include "df/general_ref_unit_workerst.h"
#include "df/general_ref_building_holderst.h"
#include "df/interface_button_building_new_jobst.h"
#include "df/item.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/job_list_link.h"
#include "df/job_postingst.h"
#include "df/job_restrictionst.h"
#include "df/plotinfost.h"
#include "df/specific_ref.h"
#include "df/unit.h"
#include "df/world.h"

#include <string>
#include <vector>
#include <map>
#include <cassert>

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
    //pnew->job_items.elements.clear();

    if ( keepEverything ) {
        for ( size_t a = 0; a < pnew->items.size(); a++ )
            pnew->items[a] = new df::job_item_ref(*pnew->items[a]);
        for ( size_t a = 0; a < pnew->specific_refs.size(); a++ )
            pnew->specific_refs[a] = new df::specific_ref(*pnew->specific_refs[a]);
    } else {
        pnew->items.clear();
        pnew->specific_refs.clear();
    }

    for ( size_t a = 0; a < pnew->job_items.elements.size(); a++ )
        pnew->job_items.elements[a] = new df::job_item(*pnew->job_items.elements[a]);

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
    for ( size_t a = 0; a < job->job_items.elements.size(); a++ )
        delete job->job_items.elements[a];
    for ( size_t a = 0; a < job->general_refs.size(); a++ )
        delete job->general_refs[a];

    delete job;
}

#define CMP(field) (a.field == b.field)

bool DFHack::operator== (const df::job_item &a, const df::job_item &b)
{
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
    if (!(CMP(job_type) && CMP(job_subtype) &&
          CMP(mat_type) && CMP(mat_index) &&
          CMP(item_subtype) && CMP(specflag.whole) &&
          CMP(specdata.hist_figure_id) && CMP(material_category.whole) &&
          CMP(reaction_name) && CMP(job_items.elements.size())))
        return false;

    for (int i = a.job_items.elements.size()-1; i >= 0; i--)
        if (!(*a.job_items.elements[i] == *b.job_items.elements[i]))
            return false;

    return true;
}

void DFHack::Job::printItemDetails(color_ostream &out, df::job_item *item, int idx)
{
    using std::endl;

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

template <typename T>
static void print_bitfield(color_ostream &out, T jsf) {
    out << " (" << bitfield_to_string(jsf) << ")";
}

void DFHack::Job::printJobDetails(color_ostream &out, df::job *job)
{
    using std::endl;

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

    if (job->item_subtype >= 0 || job->specflag.whole)
    {
        ItemTypeInfo iinfo(itype, job->item_subtype);

        out << "    item: " << iinfo.toString();
        switch (job->job_type) {
        case df::job_type::ConstructBuilding:  print_bitfield(out, job->specflag.construct_building_flags); break;
        case df::job_type::CleanPatient:       print_bitfield(out, job->specflag.clean_patient_flags); break;
        case df::job_type::CleanSelf:          print_bitfield(out, job->specflag.clean_self_flags); break;
        case df::job_type::PlaceTrackVehicle:  print_bitfield(out, job->specflag.place_track_vehicle_flags); break;
        case df::job_type::GatherPlants:       print_bitfield(out, job->specflag.gather_flags); break;
        case df::job_type::Drink:              print_bitfield(out, job->specflag.drink_item_flags); break;
        case df::job_type::InterrogateSubject: print_bitfield(out, job->specflag.interrogation_flags); break;
        case df::job_type::WeaveCloth:         print_bitfield(out, job->specflag.weave_cloth_flags); break;
        case df::job_type::CarveTrack:         print_bitfield(out, job->specflag.carve_track_flags); break;
        case df::job_type::LinkBuildingToTrigger: print_bitfield(out, job->specflag.link_building_to_trigger_flags); break;
        case df::job_type::EncrustWithGems:
        case df::job_type::EncrustWithGlass:
        case df::job_type::EncrustWithStones:
            print_bitfield(out, job->specflag.encrust_flags);
            break;
        default:
            break;
        }
        out << endl;
    }

    if (job->specdata.hist_figure_id >= 0)
        out << "    figure: " << job->specdata.hist_figure_id << endl;

    if (!job->reaction_name.empty())
        out << "    reaction: " << job->reaction_name << endl;

    for (size_t i = 0; i < job->job_items.elements.size(); i++)
        printItemDetails(out, job->job_items.elements[i], i);
}

bool Job::addGeneralRef(df::job *job, df::general_ref_type type, int32_t id)
{
    CHECK_NULL_POINTER(job)
    auto ref = References::createGeneralRef(type);
    if (!ref)
        return false;
    ref->setID(id);
    job->general_refs.push_back(ref);
    return true;
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

    int idx = linear_index(workshop->job_claim_suppress, &df::job_restrictionst::unit, worker);

    if (idx < 0)
    {
        auto obj = new df::job_restrictionst;
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

void DFHack::Job::disconnectJobItem(df::job *job, df::job_item_ref *item_ref) {
    if (!item_ref) return;

    auto item = item_ref->item;
    if (!item) return;

    //Work backward through the specific refs & remove/delete all specific refs to this job
    int refCount = item->specific_refs.size();
    bool stillHasJobs = false;
    for(int refIndex = refCount-1; refIndex >= 0; refIndex--) {
        auto ref = item->specific_refs[refIndex];

        if (ref->type == df::specific_ref_type::JOB) {
            if (ref->data.job == job) {
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

bool DFHack::Job::removeJob(df::job* job) {
    using df::global::world;
    CHECK_NULL_POINTER(job);

    // cancel_job below does not clean up all refs, so we have to do some work
    for (auto &item_ref : job->items) {
        disconnectJobItem(job, item_ref);
        if (item_ref) delete item_ref;
    }
    job->items.resize(0);

    // call the job cancel vmethod graciously provided by The Toady One.
    // job_handler::cancel_job calls job::~job, and then deletes job (this has
    // been confirmed by disassembly).

    // HACK: GCC (starting around GCC 10 targeting C++20 as of v50.09) optimizes
    // out the vmethod call here regardless of optimization level, so we need to
    // invoke the vmethod manually through a pointer, as the Lua wrapper does.
    // `volatile` does not seem to be necessary but is included for good
    // measure.
    volatile auto cancel_job_method = &df::job_handler::cancel_job;
    (world->jobs.*cancel_job_method)(job);

    return true;
}

bool DFHack::Job::addWorker(df::job *job, df::unit *unit){
    CHECK_NULL_POINTER(job);
    CHECK_NULL_POINTER(unit);

    if (unit->job.current_job)
        return false;

    if (job->posting_index != -1){
        removePostings(job);
    }

    if (!addGeneralRef(job, general_ref_type::UNIT_WORKER, unit->id))
        return false;
    unit->job.current_job = job;
    job->recheck_cntdn = 0;
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

df::job* DFHack::Job::createLinked()
{
    auto job = new df::job();
    DFHack::Job::linkIntoWorld(job, true);
    return job;
}

bool assignToWorkshop(df::job *job, df::building_workshopst *workshop)
{
    CHECK_NULL_POINTER(job);
    CHECK_NULL_POINTER(workshop);

    if (workshop->jobs.size() >= 10) {
        return false;
    }
    job->pos = df::coord(workshop->centerx, workshop->centery, workshop->z);
    DFHack::Job::addGeneralRef(job, df::general_ref_type::BUILDING_HOLDER, workshop->id);
    workshop->jobs.push_back(job);
    return true;
}

bool DFHack::Job::removePostings(df::job *job, bool remove_all)
{
    using df::global::world;
    CHECK_NULL_POINTER(job);
    bool removed = false;
    if (!remove_all)
    {
        if (job->posting_index >= 0 && size_t(job->posting_index) < world->jobs.postings.size())
        {
            auto posting = world->jobs.postings[job->posting_index];
            posting->flags.bits.dead = true;
            posting->job = nullptr;
            removed = true;
        }
    }
    else
    {
        for (auto posting : world->jobs.postings)
        {
            if (posting->job == job)
            {
                posting->flags.bits.dead = true;
                posting->job = nullptr;
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
                                df::job_role_type role,
                                int filter_idx, int insert_idx)
{
    CHECK_NULL_POINTER(job);
    CHECK_NULL_POINTER(item);

    /*
     * Functionality 100% reverse-engineered from DF code.
     */

    if (role != df::job_role_type::TargetContainer)
    {
        if (item->flags.bits.in_job)
            return false;

        item->flags.bits.in_job = true;
    }

    auto item_link = new df::specific_ref();
    item_link->type = specific_ref_type::JOB;
    item_link->data.job = job;
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

bool Job::isSuitableItem(const df::job_item *jitem, df::item_type itype, int isubtype)
{
    CHECK_NULL_POINTER(jitem);

    if (itype == item_type::NONE)
        return true;

    ItemTypeInfo iinfo(itype, isubtype);
    MaterialInfo minfo(jitem);

    return iinfo.isValid() && iinfo.matches(*jitem, &minfo, false, itype);
}

bool Job::isSuitableMaterial(
    const df::job_item *jitem, int mat_type, int mat_index, df::item_type itype)
{
    CHECK_NULL_POINTER(jitem);

    if (mat_type == -1 && mat_index == -1)
        return true;

    ItemTypeInfo iinfo(jitem);
    MaterialInfo minfo(mat_type, mat_index);

    return minfo.isValid() && iinfo.matches(*jitem, &minfo, false, itype);
}

std::string Job::getName(df::job *job)
{
    CHECK_NULL_POINTER(job);

    std::string desc;
    auto button = df::allocate<df::interface_button_building_new_jobst>();
    button->mstring = job->reaction_name;
    button->specdata.hist_figure_id = job->specdata.hist_figure_id;
    button->jobtype = job->job_type;
    button->itemtype = job->item_type;
    button->subtype = job->item_subtype;
    button->material = job->mat_type;
    button->matgloss = job->mat_index;
    button->specflag = job->specflag;
    button->job_item_flag = job->material_category;

    button->text(&desc);
    delete button;

    return desc;
}
