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

static command_result workflow_cmd(Core *c, vector <string> & parameters);

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
                "workflow", "Manage control of repeat jobs.",
                workflow_cmd, false,
                "  workflow enable\n"
                "  workflow disable\n"
                "    Enable or disable the plugin.\n"
                "  workflow list-jobs\n"
                "    List workflow-controlled jobs (if in a workshop, filtered by it).\n"
                "Function:\n"
                "  When the plugin is enabled, it protects all repeat jobs from removal.\n"
                "  If they do disappear due to any cause, they are immediately re-added\n"
                "  to their workshop and suspended.\n"
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

/*******************************/

static bool enabled = false;
static PersistentDataItem config;

enum ConfigFlags {
    CF_ENABLED = 1
};

typedef std::map<int, ProtectedJob*> TKnownJobs;
static TKnownJobs known_jobs;

static std::vector<ProtectedJob*> pending_recover;

/*******************************/

static ProtectedJob *get_known(int id)
{
    TKnownJobs::iterator it = known_jobs.find(id);
    return (it != known_jobs.end()) ? it->second : NULL;
}

static bool isSupportedJob(df::job *job)
{
    return job->misc_links.empty() &&
           !job->job_items.empty() &&
           getJobHolder(job);
}

static void enumLiveJobs(std::map<int, df::job*> &rv)
{
    df::job_list_link *p = world->job_list.next;
    for (; p; p = p->next)
        rv[p->item->id] = p->item;
}

/*******************************/

static void stop_protect(Core *c)
{
    pending_recover.clear();

    if (!known_jobs.empty())
        c->con.print("Unprotecting %d jobs.\n", known_jobs.size());

    for (TKnownJobs::iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
        delete it->second;

    known_jobs.clear();
}

static void cleanup_state(Core *c)
{
    config = PersistentDataItem();

    stop_protect(c);
}

static bool check_lost_jobs(Core *c);

static void start_protect(Core *c)
{
    check_lost_jobs(c);

    if (!known_jobs.empty())
        c->con.print("Protecting %d jobs.\n", known_jobs.size());
}

static void init_state(Core *c)
{
    config = c->getWorld()->GetPersistentData("workflow/config");

    enabled = config.isValid() && config.ival(0) != -1 &&
              (config.ival(0) & CF_ENABLED);

    if (!enabled)
        return;

    start_protect(c);
}

static void enable_plugin(Core *c)
{
    if (!config.isValid())
    {
        config = c->getWorld()->AddPersistentData("workflow/config");
        config.ival(0) = 0;
    }

    config.ival(0) |= CF_ENABLED;
    enabled = true;
    c->con << "Enabling the plugin." << endl;

    start_protect(c);
}

/*******************************/

static void forget_job(Core *c, ProtectedJob *pj)
{
    known_jobs.erase(pj->id);
    delete pj;
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

    // Create and link in the actual job structure
    df::job *recovered = cloneJobStruct(pj->job_copy);

    recovered->flags.bits.repeat = true;
    recovered->flags.bits.suspend = true;

    if (!linkJobIntoWorld(recovered, false)) // reuse same id
    {
        deleteJobStruct(recovered);

        c->con.printerr("Inconsistency: job %d (%s) already in list.",
                        pj->id, ENUM_KEY_STR(job_type, pj->job_copy->job_type));
        pj->live = true;
        return true;
    }

    pj->holder->jobs.push_back(recovered);

    // Done
    pj->live = true;
    return true;
}

static bool check_lost_jobs(Core *c)
{
    static int check = 1;
    check++;

    df::job_list_link *p = world->job_list.next;
    for (; p; p = p->next)
    {
        df::job *job = p->item;

        ProtectedJob *pj = get_known(job->id);
        if (pj)
        {
            if (!job->flags.bits.repeat)
                forget_job(c, pj);
            else
                pj->check_idx = check;
        }
        else if (job->flags.bits.repeat && isSupportedJob(job))
        {
            pj = new ProtectedJob(job);
            assert(pj->holder);
            known_jobs[pj->id] = pj;
            pj->check_idx = check;
        }
    }

    bool any_lost = false;

    for (TKnownJobs::const_iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
    {
        if (it->second->check_idx == check || !it->second->live)
            continue;

        it->second->live = false;
        pending_recover.push_back(it->second);
        any_lost = true;
    }

    return any_lost;
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

static void recover_jobs(Core *c)
{
    for (int i = pending_recover.size()-1; i >= 0; i--)
        if (recover_job(c, pending_recover[i]))
            vector_erase_at(pending_recover, i);
}

DFhackCExport command_result plugin_onupdate(Core* c)
{
    if (!enabled)
        return CR_OK;

    static unsigned cnt = 0;
    cnt++;

    bool force_recover = false;
    if ((cnt % 5) == 0)
    {
        force_recover = check_lost_jobs(c);
    }

    if (force_recover || (cnt % 50) == 0)
    {
        recover_jobs(c);
    }

    if ((cnt % 500) == 0)
        update_job_data(c);

    return CR_OK;
}

static command_result workflow_cmd(Core *c, vector <string> & parameters)
{
    CoreSuspender suspend(c);

    update_job_data(c);

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

    std::string cmd = parameters[0];

    if (cmd == "enable")
    {
        if (enabled)
        {
            c->con << "The plugin is already enabled." << endl;
            return CR_OK;
        }

        enable_plugin(c);
        return CR_OK;
    }
    else if (cmd == "disable")
    {
        if (!enabled)
        {
            c->con << "The plugin is already disabled." << endl;
            return CR_OK;
        }

        enabled = false;
        config.ival(0) &= ~CF_ENABLED;
        stop_protect(c);
        return CR_OK;
    }

    if (!enabled)
        c->con << "Note: the plugin is not enabled." << endl;

    if (cmd == "list-jobs")
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

        return CR_OK;
    }
    else
        return CR_WRONG_USAGE;
}
