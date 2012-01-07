#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <MiscUtils.h>

#include <modules/Materials.h>
#include <modules/Gui.h>
#include <modules/Job.h>
#include <modules/World.h>

#include <DataDefs.h>
#include <df/world.h>
#include <df/ui.h>
#include <df/building_workshopst.h>
#include <df/building_furnacest.h>
#include <df/job.h>
#include <df/job_item.h>
#include <df/job_list_link.h>
#include <df/item.h>
#include <df/tool_uses.h>
#include <df/general_ref.h>
#include <df/general_ref_unit_workerst.h>
#include <df/general_ref_building_holderst.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;
using df::global::ui_workshop_job_cursor;
using df::global::job_next_id;

/* Plugin registration */

static command_result protect_job(Core *c, vector <string> & parameters);

static void init_state(Core *c);
static void cleanup_state(Core *c);

DFhackCExport const char * plugin_name ( void )
{
    return "workflow";
}

DFhackCExport command_result plugin_init (Core *c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    if (!world || !ui)
        return CR_FAILURE;

    if (ui_workshop_job_cursor && job_next_id) {
        commands.push_back(
            PluginCommand(
                "protect-job", "Manage protection of workshop jobs from removal.",
                protect_job, false,
                "  protect-job list\n"
                "    List protected jobs. If a workshop is selected, filters by it.\n"
                "  protect-job add [all]\n"
                "    Protect the selected job, or any repeat jobs (possibly in the workshop).\n"
                "  protect-job remove [all]\n"
                "    Unprotect the selected job, or any repeat jobs (possibly in the workshop).\n"
            )
        );
    }

    init_state(c);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    cleanup_state(c);

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(Core* c, state_change_event event)
{
    switch (event) {
    case SC_GAME_LOADED:
        cleanup_state(c);
        init_state(c);
        break;
    case SC_GAME_UNLOADED:
        cleanup_state(c);
        break;
    default:
        break;
    }

    return CR_OK;
}

/*******************************/

static df::building *getJobHolder(df::job *job)
{
    for (unsigned i = 0; i < job->references.size(); i++)
    {
        VIRTUAL_CAST_VAR(ref, df::general_ref_building_holderst, job->references[i]);
        if (ref)
            return ref->getBuilding();
    }

    return NULL;
};

struct ProtectedJob {
    int id;
    int building_id;
    int check_idx;

    bool live;
    df::building *holder;
    df::job *job_copy;

    ProtectedJob(df::job *job) : id(job->id), live(true)
    {
        check_idx = 0;
        holder = getJobHolder(job);
        building_id = holder ? holder->id : -1;
        job_copy = cloneJobStruct(job);
    }

    ~ProtectedJob()
    {
        deleteJobStruct(job_copy);
    }

    void update(df::job *job)
    {
        if (*job == *job_copy)
            return;

        deleteJobStruct(job_copy);
        job_copy = cloneJobStruct(job);
    }
};

PersistentDataItem config;
static std::vector<PersistentDataItem> protected_cfg;

typedef std::map<int, ProtectedJob*> TKnownJobs;
static TKnownJobs known_jobs;

static std::vector<ProtectedJob*> pending_recover;

static ProtectedJob *get_known(int id)
{
    TKnownJobs::iterator it = known_jobs.find(id);
    return (it != known_jobs.end()) ? it->second : NULL;
}

static void enumLiveJobs(std::map<int, df::job*> &rv)
{
    df::job_list_link *p = world->job_list.next;
    for (; p; p = p->next)
        rv[p->item->id] = p->item;
}

static void cleanup_state(Core *)
{
    config = PersistentDataItem();
    protected_cfg.clear();

    pending_recover.clear();

    for (TKnownJobs::iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
        delete it->second;

    known_jobs.clear();
}

static void init_state(Core *c)
{
    config = c->getWorld()->GetPersistentData("workflow/config");
    c->getWorld()->GetPersistentData(&protected_cfg, "workflow/protected-jobs");

    std::map<int, df::job*> jobs;
    enumLiveJobs(jobs);

    for (unsigned i = 0; i < protected_cfg.size(); i++)
    {
        PersistentDataItem &item = protected_cfg[i];
        for (int j = 0; j < PersistentDataItem::NumInts; j++)
        {
            int id = item.ival(j);
            if (id <= 0)
                continue;

            if (get_known(id)) // duplicate
            {
                item.ival(j) = -1;
                continue;
            }

            df::job *job = jobs[id];
            if (!job)
            {
                c->con.printerr("Protected job lost: %d\n", id);
                item.ival(j) = -1;
                continue;
            }

            if (!job->misc_links.empty() || job->job_items.empty())
            {
                c->con.printerr("Protected job unsupported: %d (%s)\n",
                                id, ENUM_KEY_STR(job_type, job->job_type));
                item.ival(j) = -1;
                continue;
            }

            ProtectedJob *pj = new ProtectedJob(job);
            if (!pj->holder)
            {
                c->con.printerr("Protected job not in building: %d (%s)\n",
                                id, ENUM_KEY_STR(job_type, job->job_type));
                delete pj;
                item.ival(j) = -1;
                continue;
            }

            known_jobs[id] = pj;

            if (!job->flags.bits.repeat) {
                c->con.printerr("Protected job not repeating: %d\n", id);
                job->flags.bits.repeat = true;
            }
        }
    }

    if (!known_jobs.empty())
        c->con.print("Protecting %d jobs.\n", known_jobs.size());
}

static int *find_protected_id_slot(Core *c, int key)
{
    for (unsigned i = 0; i < protected_cfg.size(); i++)
    {
        PersistentDataItem &item = protected_cfg[i];
        for (int j = 0; j < PersistentDataItem::NumInts; j++)
        {
            if (item.ival(j) == key)
                return &item.ival(j);
        }
    }

    if (key == -1) {
        protected_cfg.push_back(c->getWorld()->AddPersistentData("workflow/protected-jobs"));
        PersistentDataItem &item = protected_cfg.back();
        for (int j = 0; j < PersistentDataItem::NumInts; j++)
            item.ival(j) = -1;
        return &item.ival(0);
    }

    return NULL;
}

static void forget_job(Core *c, ProtectedJob *pj)
{
    known_jobs.erase(pj->id);

    if (int *p = find_protected_id_slot(c, pj->id))
        *p = -1;

    delete pj;
}

static void remember_job(Core *c, df::job *job)
{
    if (get_known(job->id))
        return;

    if (!job->misc_links.empty() || job->job_items.empty())
    {
        c->con.printerr("Unsupported job type: %d (%s)\n",
                        job->id, ENUM_KEY_STR(job_type, job->job_type));
        return;
    }

    known_jobs[job->id] = new ProtectedJob(job);

    *find_protected_id_slot(c, -1) = job->id;
}

static bool recover_job(Core *c, ProtectedJob *pj)
{
    // Check that the building exists
    pj->holder = df::building::find(pj->building_id);
    if (!pj->holder)
    {
        c->con.printerr("Forgetting job %d (%s): holder building lost.",
                        pj->id, ENUM_KEY_STR(job_type, pj->job_copy->job_type));
        forget_job(c, pj);
        return true;
    }

    // Check its state and postpone or cancel if invalid
    if (pj->holder->jobs.size() >= 10)
    {
        c->con.printerr("Forgetting job %d (%s): holder building has too many jobs.",
                        pj->id, ENUM_KEY_STR(job_type, pj->job_copy->job_type));
        forget_job(c, pj);
        return true;
    }

    if (!pj->holder->jobs.empty())
    {
        df::job_type ftype = pj->holder->jobs[0]->job_type;
        if (ftype == job_type::DestroyBuilding)
            return false;

        if (ENUM_ATTR(job_type,type,ftype) == job_type_class::StrangeMood)
            return false;
    }

    // Find the position in the job list
    df::job_list_link *ins_pos = &world->job_list;
    while (ins_pos->next && ins_pos->next->item->id < pj->id)
        ins_pos = ins_pos->next;

    if (ins_pos->next && ins_pos->next->item->id == pj->id)
    {
        c->con.printerr("Inconsistency: job %d (%s) already in list.",
                        pj->id, ENUM_KEY_STR(job_type, pj->job_copy->job_type));
        pj->live = true;
        return true;
    }

    // Create the actual job structure
    df::job *recovered = cloneJobStruct(pj->job_copy);

    recovered->flags.bits.repeat = true;
    recovered->flags.bits.suspend = true;

    // Link the job into the global list
    df::job_list_link *link = new df::job_list_link();
    recovered->list_link = link;

    link->item = recovered;
    link->next = ins_pos->next;
    if (ins_pos->next)
        ins_pos->next->prev = link;
    link->prev = ins_pos;
    ins_pos->next = link;

    // Add to building jobs
    pj->holder->jobs.push_back(recovered);

    // Done
    pj->live = true;
    return true;
}

static void check_lost_jobs(Core *c)
{
    static int check = 1;
    check++;

    df::job_list_link *p = world->job_list.next;
    for (; p; p = p->next)
    {
        ProtectedJob *pj = get_known(p->item->id);
        if (!pj)
            continue;
        pj->check_idx = check;

        // force repeat
        p->item->flags.bits.repeat = true;
    }

    for (TKnownJobs::const_iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
    {
        if (it->second->check_idx == check || !it->second->live)
            continue;

        it->second->live = false;
        pending_recover.push_back(it->second);
    }
}

static void update_job_data(Core *c)
{
    df::job_list_link *p = world->job_list.next;
    for (; p; p = p->next)
    {
        ProtectedJob *pj = get_known(p->item->id);
        if (!pj)
            continue;
        pj->update(p->item);
    }
}

DFhackCExport command_result plugin_onupdate(Core* c)
{
    if (known_jobs.empty())
        return CR_OK;

    static unsigned cnt = 0;
    cnt++;

    if ((cnt % 10) == 0)
    {
        for (int i = pending_recover.size()-1; i >= 0; i--)
            if (recover_job(c, pending_recover[i]))
                pending_recover.erase(pending_recover.begin()+i);

        check_lost_jobs(c);
    }

    if ((cnt % 500) == 0)
        update_job_data(c);

    return CR_OK;
}

static command_result protect_job(Core *c, vector <string> & parameters)
{
    CoreSuspender suspend(c);

    if (parameters.empty())
        return CR_WRONG_USAGE;

    df::building *workshop = NULL;
    df::job *job = NULL;

    if (dwarfmode_hotkey(c, c->getTopViewscreen()) &&
        ui->main.mode == ui_sidebar_mode::QueryBuilding)
    {
        workshop = world->selected_building;
        job = getSelectedWorkshopJob(c, true);
    }

    std::map<int, df::job*> jobs;
    enumLiveJobs(jobs);
    update_job_data(c);

    std::string cmd = parameters[0];
    if (cmd == "list")
    {
        if (workshop)
        {
            for (unsigned i = 0; i < workshop->jobs.size(); i++)
                if (get_known(workshop->jobs[i]->id))
                    printJobDetails(c, workshop->jobs[i]);
        }
        else
        {
            for (TKnownJobs::iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
                if (df::job *job = jobs[it->first])
                    printJobDetails(c, job);
        }

        bool pending = false;

        for (unsigned i = 0; i < pending_recover.size(); i++)
        {
            if (!workshop || pending_recover[i]->holder == workshop)
            {
                if (!pending)
                {
                    c->con.print("\nPending recovery:\n");
                    pending = true;
                }

                printJobDetails(c, pending_recover[i]->job_copy);
            }
        }
    }
    else if (cmd == "add" || cmd == "remove")
    {
        bool add = (cmd == "add");
        bool all = (parameters.size() >= 2 && parameters[1] == "all");
        if (parameters.size() >= 2 && !all)
            return CR_WRONG_USAGE;

        if (workshop && all)
        {
            for (unsigned i = 0; i < workshop->jobs.size(); i++)
            {
                df::job *job = workshop->jobs[i];
                if (add)
                {
                    if (!job->flags.bits.repeat)
                        continue;
                    remember_job(c, job);
                }
                else
                {
                    if (ProtectedJob *pj = get_known(job->id))
                        forget_job(c, pj);
                }
            }
        }
        else if (workshop)
        {
            if (!job) {
                c->con.printerr("No job is selected in the current building.\n");
                return CR_FAILURE;
            }

            if (add)
                remember_job(c, job);
            else if (ProtectedJob *pj = get_known(job->id))
                forget_job(c, pj);
        }
        else
        {
            if (!all) {
                c->con.printerr("Please either select a job, or specify 'all'.\n");
                return CR_WRONG_USAGE;
            }

            if (add)
            {
                for (std::map<int,df::job*>::iterator it = jobs.begin(); it != jobs.end(); it++)
                    if (it->second->flags.bits.repeat)
                        remember_job(c, it->second);
            }
            else
            {
                pending_recover.clear();

                while (!known_jobs.empty())
                    forget_job(c, known_jobs.begin()->second);
            }
        }
    }
    else
        return CR_WRONG_USAGE;
}
