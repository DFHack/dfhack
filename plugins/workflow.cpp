#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <MiscUtils.h>

#include <modules/Materials.h>
#include <modules/Items.h>
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
                "  workflow jobs\n"
                "    List workflow-controlled jobs (if in a workshop, filtered by it).\n"
                "  workflow list\n"
                "    List active constraints, and their job counts.\n"
                "  workflow limit <constraint-spec> <cnt-limit> [cnt-gap]\n"
                "    Set a constraint.\n"
                "  workflow unlimit <constraint-spec>\n"
                "    Delete a constraint.\n"
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

struct ItemConstraint;

struct ProtectedJob {
    int id;
    int building_id;
    int check_idx;

    bool live;
    df::building *holder;
    df::job *job_copy;

    df::job *actual_job;

    std::vector<ItemConstraint*> constraints;

    ProtectedJob(df::job *job) : id(job->id), live(true)
    {
        check_idx = 0;
        holder = getJobHolder(job);
        building_id = holder ? holder->id : -1;
        job_copy = cloneJobStruct(job);
        actual_job = job;
    }

    ~ProtectedJob()
    {
        deleteJobStruct(job_copy);
    }

    void update(df::job *job)
    {
        actual_job = job;
        if (*job == *job_copy)
            return;

        deleteJobStruct(job_copy);
        job_copy = cloneJobStruct(job);
    }
};

struct ItemConstraint {
    PersistentDataItem config;

    ItemTypeInfo item;

    MaterialInfo material;
    df::job_material_category mat_mask;

    int weight;
    std::vector<ProtectedJob*> jobs;

    std::map<std::pair<int,int>, bool> material_cache;

    int goalCount() { return config.ival(0); }
    void setGoalCount(int v) { config.ival(0) = v; }

    int goalGap() {
        int gcnt = std::max(1, goalCount()/2);
        return std::min(gcnt, config.ival(1) <= 0 ? 5 : config.ival(1));
    }
    void setGoalGap(int v) { config.ival(1) = v; }
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
static std::vector<ItemConstraint*> constraints;

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

    for (unsigned i = 0; i < constraints.size(); i++)
        delete constraints[i];
    constraints.clear();
}

static bool check_lost_jobs(Core *c);
static ItemConstraint *get_constraint(Core *c, const std::string &str, PersistentDataItem *cfg = NULL);

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

    // Parse constraints
    std::vector<PersistentDataItem> items;
    c->getWorld()->GetPersistentData(&items, "workflow/constraints");

    for (int i = items.size()-1; i >= 0; i--) {
        if (get_constraint(c, items[i].val(), &items[i]))
            continue;

        c->con.printerr("Lost constraint %s\n", items[i].val().c_str());
        c->getWorld()->DeletePersistentData(items[i]);
    }

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
    pj->actual_job = recovered;
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
        it->second->actual_job = NULL;
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
    static unsigned last_rlen = 0;
    cnt++;

    if ((cnt % 5) == 0) {
        check_lost_jobs(c);

        if (pending_recover.size() != last_rlen || (cnt % 50) == 0)
        {
            recover_jobs(c);
            last_rlen = pending_recover.size();

            if ((cnt % 500) == 0)
                update_job_data(c);
        }
    }

    return CR_OK;
}

/*******************************/

static ItemConstraint *get_constraint(Core *c, const std::string &str, PersistentDataItem *cfg)
{
    std::vector<std::string> tokens;
    split_string(&tokens, str, "/");

    if (tokens.size() > 3)
        return NULL;

    int weight = 0;

    ItemTypeInfo item;
    if (!item.find(tokens[0]) || !item.isValid()) {
        c->con.printerr("Cannot find item type: %s\n", tokens[0].c_str());
        return NULL;
    }

    if (item.subtype >= 0)
        weight += 10000;

    MaterialInfo material;
    std::string matstr = vector_get(tokens,1);
    if (!matstr.empty() && (!material.find(matstr) || !material.isValid())) {
        c->con.printerr("Cannot find material: %s\n", matstr.c_str());
        return NULL;
    }

    if (material.type >= 0)
        weight += (material.index >= 0 ? 5000 : 1000);

    df::job_material_category mat_mask;
    std::string maskstr = vector_get(tokens,2);
    if (!maskstr.empty() && !parseJobMaterialCategory(&mat_mask, maskstr)) {
        c->con.printerr("Cannot decode material mask: %s\n", maskstr.c_str());
        return NULL;
    }

    if (mat_mask.whole && material.isValid() && !material.matches(mat_mask)) {
        c->con.printerr("Material %s doesn't match mask %s\n", matstr.c_str(), maskstr.c_str());
        return NULL;
    }

    if (mat_mask.whole != 0)
        weight += 100;

    for (unsigned i = 0; i < constraints.size(); i++)
    {
        ItemConstraint *ct = constraints[i];
        if (ct->item == item && ct->material == material &&
            ct->mat_mask.whole == mat_mask.whole)
            return ct;
    }

    ItemConstraint *nct = new ItemConstraint;
    nct->item = item;
    nct->material = material;
    nct->mat_mask = mat_mask;
    nct->weight = weight;

    if (cfg)
        nct->config = *cfg;
    else
    {
        nct->config = c->getWorld()->AddPersistentData("workflow/constraints");
        nct->config.val() = str;
    }

    constraints.push_back(nct);
    return nct;
}

static void delete_constraint(Core *c, ItemConstraint *cv)
{
    int idx = linear_index(constraints, cv);
    if (idx >= 0)
        vector_erase_at(constraints, idx);

    c->getWorld()->DeletePersistentData(cv->config);
    delete cv;
}

static void print_constraint(Core *c, ItemConstraint *cv, bool no_job = false, std::string prefix = "")
{
    c->con << prefix << "Constraint " << cv->config.val() << ": "
           << cv->goalCount() << " (gap " << cv->goalGap() << ")" << endl;

    if (no_job) return;

    if (cv->jobs.empty())
        c->con.printerr("  (no jobs)\n");

    for (int i = 0; i < cv->jobs.size(); i++)
    {
        ProtectedJob *pj = cv->jobs[i];
        df::job *job = pj->actual_job;

        c->con << prefix << "  job " << job->id << ": "
               << ENUM_KEY_STR(job_type, job->job_type);
        if (job->flags.bits.suspend)
            c->con << " (suspended)";
        c->con << endl;
    }
}

static void print_job(Core *c, ProtectedJob *pj)
{
    if (!pj)
        return;

    printJobDetails(c, pj->job_copy);

    for (int i = 0; i < pj->constraints.size(); i++)
        print_constraint(c, pj->constraints[i], true, "  ");
}

static void map_job_constraints(Core *c)
{
    for (unsigned i = 0; i < constraints.size(); i++)
        constraints[i]->jobs.clear();

    for (TKnownJobs::const_iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
    {
        it->second->constraints.clear();

        if (!it->second->live)
            continue;

        df::job *job = it->second->job_copy;

        df::item_type itype = ENUM_ATTR(job_type, item, job->job_type);
        if (itype == item_type::NONE)
            continue;

        int16_t isubtype = job->item_subtype;

        int16_t mat_type = job->mat_type;
        int32_t mat_index = job->mat_index;

        if (itype == item_type::FOOD)
            mat_type = -1;

        if (mat_type == -1 && job->job_items.size() == 1) {
            mat_type = job->job_items[0]->mat_type;
            mat_index = job->job_items[0]->mat_index;
        }

        MaterialInfo mat(mat_type, mat_index);

        for (unsigned i = 0; i < constraints.size(); i++)
        {
            ItemConstraint *ct = constraints[i];

            if (ct->item.type != itype ||
                (ct->item.subtype != -1 && ct->item.subtype != isubtype))
                continue;
            if (ct->material.isValid() && ct->material != mat)
                continue;
            if (ct->mat_mask.whole)
            {
                if (mat.isValid() && !mat.matches(ct->mat_mask))
                    continue;
                else if (!(job->material_category.whole & ct->mat_mask.whole))
                    continue;
            }

            ct->jobs.push_back(it->second);
            it->second->constraints.push_back(ct);
        }
    }
}

static command_result workflow_cmd(Core *c, vector <string> & parameters)
{
    CoreSuspender suspend(c);

    if (enabled) {
        check_lost_jobs(c);
        recover_jobs(c);
        update_job_data(c);
        map_job_constraints(c);
    }

    df::building *workshop = NULL;
    df::job *job = NULL;

    if (dwarfmode_hotkey(c, c->getTopViewscreen()) &&
        ui->main.mode == ui_sidebar_mode::QueryBuilding)
    {
        workshop = world->selected_building;
        job = getSelectedWorkshopJob(c, true);
    }

    std::string cmd = parameters.empty() ? "list" : parameters[0];

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

    if (cmd == "jobs")
    {
        if (workshop)
        {
            for (unsigned i = 0; i < workshop->jobs.size(); i++)
                print_job(c, get_known(workshop->jobs[i]->id));
        }
        else
        {
            for (TKnownJobs::iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
                if (it->second->live)
                    print_job(c, it->second);
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
    else if (cmd == "list")
    {
        for (int i = 0; i < constraints.size(); i++)
            print_constraint(c, constraints[i]);

        return CR_OK;
    }
    else if (cmd == "limit")
    {
        if (parameters.size() < 3)
            return CR_WRONG_USAGE;

        int limit = atoi(parameters[2].c_str());
        if (limit <= 0) {
            c->con.printerr("Invalid limit value.\n");
            return CR_FAILURE;
        }

        ItemConstraint *icv = get_constraint(c, parameters[1]);
        if (!icv)
            return CR_FAILURE;

        icv->setGoalCount(limit);
        if (parameters.size() >= 4)
            icv->setGoalGap(atoi(parameters[3].c_str()));

        map_job_constraints(c);
        print_constraint(c, icv);
        return CR_OK;
    }
    else if (cmd == "unlimit")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;

        for (int i = 0; i < constraints.size(); i++)
        {
            if (constraints[i]->config.val() != parameters[1])
                continue;

            delete_constraint(c, constraints[i]);
            return CR_OK;
        }

        c->con.printerr("Constraint not found: %s\n", parameters[1].c_str());
        return CR_FAILURE;
    }
    else
        return CR_WRONG_USAGE;
}
