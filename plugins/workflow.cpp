#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include "modules/Materials.h"
#include "modules/Items.h"
#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/World.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/building_workshopst.h"
#include "df/building_furnacest.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/job_list_link.h"
#include "df/dfhack_material_category.h"
#include "df/item.h"
#include "df/item_quality.h"
#include "df/items_other_id.h"
#include "df/tool_uses.h"
#include "df/general_ref.h"
#include "df/general_ref_unit_workerst.h"
#include "df/general_ref_unit_holderst.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_contains_itemst.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/general_ref_contains_unitst.h"
#include "df/itemdef_foodst.h"
#include "df/reaction.h"
#include "df/reaction_reagent_itemst.h"
#include "df/reaction_product_itemst.h"
#include "df/plant_raw.h"
#include "df/inorganic_raw.h"
#include "df/builtin_mats.h"

#include "df/viewscreen_dwarfmodest.h"
#include "df/itemdef_weaponst.h"
#include "df/itemdef_trapcompst.h"
#include "df/itemdef_toyst.h"
#include "df/itemdef_toolst.h"
#include "df/itemdef_instrumentst.h"
#include "df/itemdef_armorst.h"
#include "df/itemdef_ammost.h"
#include "df/itemdef_siegeammost.h"
#include "df/itemdef_glovesst.h"
#include "df/itemdef_shoesst.h"
#include "df/itemdef_shieldst.h"
#include "df/itemdef_helmst.h"
#include "df/itemdef_pantsst.h"
#include "df/itemdef_foodst.h"
#include "df/trapcomp_flags.h"

#include <deque>
#include <algorithm>
#include <VTableInterpose.h>
#include <modules/Screen.h>
#include "df/creature_raw.h"
#include "df/enabler.h"

using std::deque;
using df::global::gps;
using df::global::enabler;


using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;
using df::global::ui_workshop_job_cursor;
using df::global::ui_workshop_in_add;
using df::global::job_next_id;

/* Plugin registration */

static command_result workflow_cmd(color_ostream &out, vector <string> & parameters);

static void init_state(color_ostream &out);
static void cleanup_state(color_ostream &out);

DFHACK_PLUGIN("workflow");


void OutputString(int8_t color, int &x, int &y, const std::string &text, bool newline = false, int left_margin = 0)
{
    Screen::paintString(Screen::Pen(' ', color, 0), x, y, text);
    if (newline)
    {
        ++y;
        x = left_margin;
    }
    else
        x += text.length();
}

void OutputHotkeyString(int &x, int &y, const char *text, const char *hotkey, bool newline = false, int left_margin = 0)
{
    OutputString(10, x, y, hotkey);
    string display(": ");
    display.append(text);
    OutputString(15, x, y, display, newline, left_margin);
}



/******************************
 * JOB STATE TRACKING STRUCTS *
 ******************************/

const int DAY_TICKS = 1200;
const int MONTH_DAYS = 28;
const int YEAR_MONTHS = 12;

struct ItemConstraint;

struct ProtectedJob {
    int id;
    int building_id;
    int tick_idx;

    static int cur_tick_idx;

    df::building *holder;
    df::job *job_copy;
    int reaction_id;

    df::job *actual_job;

    bool want_resumed;
    int resume_time, resume_delay;

    std::vector<ItemConstraint*> constraints;

public:
    ProtectedJob(df::job *job) : id(job->id)
    {
        tick_idx = cur_tick_idx;
        holder = Job::getHolder(job);
        building_id = holder ? holder->id : -1;
        job_copy = Job::cloneJobStruct(job);
        actual_job = job;
        reaction_id = -1;

        want_resumed = false;
        resume_time = 0; resume_delay = DAY_TICKS;
    }

    ~ProtectedJob()
    {
        Job::deleteJobStruct(job_copy);
    }

    bool isActuallyResumed() {
        return actual_job && !actual_job->flags.bits.suspend;
    }
    bool isResumed() {
        return want_resumed || isActuallyResumed();
    }
    bool isLive() { return actual_job != NULL; }

    void update(df::job *job)
    {
        actual_job = job;
        if (*job == *job_copy)
            return;

        reaction_id = -1;
        Job::deleteJobStruct(job_copy);
        job_copy = Job::cloneJobStruct(job);
    }

    void tick_job(df::job *job, int ticks)
    {
        tick_idx = cur_tick_idx;
        actual_job = job;

        if (isActuallyResumed())
        {
            resume_time = 0;
            resume_delay = std::max(DAY_TICKS, resume_delay - ticks);
        }
        else if (want_resumed)
        {
            if (!resume_time)
                want_resumed = false;
            else if (world->frame_counter >= resume_time)
                actual_job->flags.bits.suspend = false;
        }
    }

    void recover(df::job *job)
    {
        actual_job = job;
        job->flags.bits.repeat = true;
        job->flags.bits.suspend = true;

        resume_delay = std::min(DAY_TICKS*MONTH_DAYS, 5*resume_delay/3);
        resume_time = world->frame_counter + resume_delay;
    }

    void set_resumed(bool resume)
    {
        if (resume)
        {
            if (world->frame_counter >= resume_time)
                actual_job->flags.bits.suspend = false;
        }
        else
        {
            resume_time = 0;
            if (isActuallyResumed())
                resume_delay = DAY_TICKS;

            actual_job->flags.bits.suspend = true;
        }

        want_resumed = resume;
    }
};

int ProtectedJob::cur_tick_idx = 0;

typedef std::map<std::pair<int,int>, bool> TMaterialCache;

static int max_history_days = 14;

struct ItemConstraint {
    PersistentDataItem config;

    bool is_craft;
    ItemTypeInfo item;

    MaterialInfo material;
    df::dfhack_material_category mat_mask;

    int weight;
    std::vector<ProtectedJob*> jobs;

    item_quality::item_quality min_quality;

    int item_amount, item_count, item_inuse;
    bool request_suspend, request_resume;

    bool is_active, cant_resume_reported;

    TMaterialCache material_cache;

    deque<int> history;

public:
    ItemConstraint()
        : is_craft(false), weight(0), min_quality(item_quality::Ordinary),item_amount(0),
          item_count(0), item_inuse(0), is_active(false), cant_resume_reported(false)
    {}

    int goalCount() { return config.ival(0); }
    void setGoalCount(int v) { config.ival(0) = v; }

    int goalGap() {
        int gcnt = std::max(1, goalCount()/2);
        return std::min(gcnt, config.ival(1) <= 0 ? 5 : config.ival(1));
    }
    void setGoalGap(int v) { config.ival(1) = v; }

    bool goalByCount() { return config.ival(2) & 1; }
    void setGoalByCount(bool v) {
        if (v)
            config.ival(2) |= 1;
        else
            config.ival(2) &= ~1;
    }

    void init(const std::string &str)
    {
        config.val() = str;
        config.ival(2) = 0;
    }

    void computeRequest()
    {
        int size = goalByCount() ? item_count : item_amount;
        request_resume = (size <= goalCount()-goalGap());
        request_suspend = (size >= goalCount());

        if (max_history_days > 0)
        {
            history.push_back(size);
            if (history.size() > max_history_days * 2)
                history.pop_front();
        }

    }
};

/******************************
 *      GLOBAL VARIABLES      *
 ******************************/

static bool enabled = false;
static PersistentDataItem config;

static int last_tick_frame_count = 0;
static int last_frame_count = 0;

enum ConfigFlags {
    CF_ENABLED = 1,
    CF_DRYBUCKETS = 2,
    CF_AUTOMELT = 4
};

typedef std::map<int, ProtectedJob*> TKnownJobs;
static TKnownJobs known_jobs;

static std::vector<ProtectedJob*> pending_recover;
static std::vector<ItemConstraint*> constraints;

static int meltable_count = 0;
static bool melt_active = false;

/******************************
 *       MISC FUNCTIONS       *
 ******************************/

static ProtectedJob *get_known(int id)
{
    TKnownJobs::iterator it = known_jobs.find(id);
    return (it != known_jobs.end()) ? it->second : NULL;
}

static bool isSupportedJob(df::job *job)
{
    return job->specific_refs.empty() &&
           Job::getHolder(job) &&
           (!job->job_items.empty() ||
            job->job_type == job_type::CollectClay ||
            job->job_type == job_type::CollectSand);
}

static void enumLiveJobs(std::map<int, df::job*> &rv)
{
    df::job_list_link *p = world->job_list.next;
    for (; p; p = p->next)
        rv[p->item->id] = p->item;
}

static bool isOptionEnabled(unsigned flag)
{
    return config.isValid() && (config.ival(0) & flag) != 0;
}

static void setOptionEnabled(ConfigFlags flag, bool on)
{
    if (!config.isValid())
        return;

    if (on)
        config.ival(0) |= flag;
    else
        config.ival(0) &= ~flag;
}

/******************************
 *    STATE INITIALIZATION    *
 ******************************/

static void stop_protect(color_ostream &out)
{
    pending_recover.clear();

    if (!known_jobs.empty())
        out.print("Unprotecting %d jobs.\n", known_jobs.size());

    for (TKnownJobs::iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
        delete it->second;

    known_jobs.clear();
}

static void cleanup_state(color_ostream &out)
{
    config = PersistentDataItem();

    stop_protect(out);

    for (size_t i = 0; i < constraints.size(); i++)
        delete constraints[i];
    constraints.clear();
}

static void check_lost_jobs(color_ostream &out, int ticks);
static ItemConstraint *get_constraint(color_ostream &out, const std::string &str, PersistentDataItem *cfg = NULL);

static void start_protect(color_ostream &out)
{
    check_lost_jobs(out, 0);

    if (!known_jobs.empty())
        out.print("Protecting %d jobs.\n", known_jobs.size());
}

static void init_state(color_ostream &out)
{
    auto pworld = Core::getInstance().getWorld();

    config = pworld->GetPersistentData("workflow/config");
    if (config.isValid() && config.ival(0) == -1)
        config.ival(0) = 0;

    enabled = isOptionEnabled(CF_ENABLED);

    // Parse constraints
    std::vector<PersistentDataItem> items;
    pworld->GetPersistentData(&items, "workflow/constraints");

    for (int i = items.size()-1; i >= 0; i--) {
        if (get_constraint(out, items[i].val(), &items[i]))
            continue;

        out.printerr("Lost constraint %s\n", items[i].val().c_str());
        pworld->DeletePersistentData(items[i]);
    }

    last_tick_frame_count = world->frame_counter;
    last_frame_count = world->frame_counter;

    if (!enabled)
        return;

    start_protect(out);
}

static bool first_update_done = false;
static void enable_plugin(color_ostream &out)
{
    auto pworld = Core::getInstance().getWorld();

    if (!config.isValid())
    {
        config = pworld->AddPersistentData("workflow/config");
        config.ival(0) = 0;
    }

    setOptionEnabled(CF_ENABLED, true);
    enabled = true;
    out << "Enabling workflow plugin." << endl;

    for (vector<ItemConstraint *>::iterator it = constraints.begin(); it != constraints.end(); it++)
    {
        (*it)->history.clear();
    }

    start_protect(out);
    first_update_done = false;
}

/******************************
 *     JOB AUTO-RECOVERY      *
 ******************************/

static void forget_job(color_ostream &out, ProtectedJob *pj)
{
    known_jobs.erase(pj->id);
    delete pj;
}

static bool recover_job(color_ostream &out, ProtectedJob *pj)
{
    if (pj->isLive())
        return true;

    // Check that the building exists
    pj->holder = df::building::find(pj->building_id);
    if (!pj->holder)
    {
        out.printerr("Forgetting job %d (%s): holder building lost.\n",
                        pj->id, ENUM_KEY_STR(job_type, pj->job_copy->job_type).c_str());
        forget_job(out, pj);
        return true;
    }

    // Check its state and postpone or cancel if invalid
    if (pj->holder->jobs.size() >= 10)
    {
        out.printerr("Forgetting job %d (%s): holder building has too many jobs.\n",
                        pj->id, ENUM_KEY_STR(job_type, pj->job_copy->job_type).c_str());
        forget_job(out, pj);
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
    df::job *recovered = Job::cloneJobStruct(pj->job_copy);

    if (!Job::linkIntoWorld(recovered, false)) // reuse same id
    {
        Job::deleteJobStruct(recovered);

        out.printerr("Inconsistency: job %d (%s) already in list.\n",
                        pj->id, ENUM_KEY_STR(job_type, pj->job_copy->job_type).c_str());
        return true;
    }

    pj->holder->jobs.push_back(recovered);

    // Done
    pj->recover(recovered);
    return true;
}

static ProtectedJob *add_known_job(df::job *job)
{
    ProtectedJob *pj = new ProtectedJob(job);
    assert(pj->holder);
    known_jobs[pj->id] = pj;

    return pj;
}

static void check_lost_jobs(color_ostream &out, int ticks)
{
    ProtectedJob::cur_tick_idx++;
    if (ticks < 0) ticks = 0;

    df::job_list_link *p = world->job_list.next;
    for (; p; p = p->next)
    {
        df::job *job = p->item;

        ProtectedJob *pj = get_known(job->id);
        if (pj)
        {
            if (!job->flags.bits.repeat)
                forget_job(out, pj);
            else
                pj->tick_job(job, ticks);
        }
        else if (job->flags.bits.repeat && isSupportedJob(job))
        {
            add_known_job(job);
        }
    }

    for (TKnownJobs::const_iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
    {
        if (it->second->tick_idx == ProtectedJob::cur_tick_idx ||
            !it->second->isLive())
            continue;

        it->second->actual_job = NULL;
        pending_recover.push_back(it->second);
    }
}

static void update_job_data(color_ostream &out)
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

static void recover_jobs(color_ostream &out)
{
    for (int i = pending_recover.size()-1; i >= 0; i--)
        if (recover_job(out, pending_recover[i]))
            vector_erase_at(pending_recover, i);
}

static void process_constraints(color_ostream &out);

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (!enabled)
        return CR_OK;

    // Every 5 frames check the jobs for disappearance
    static unsigned cnt = 0;
    if ((++cnt % 5) != 0 && first_update_done)
        return CR_OK;

    check_lost_jobs(out, world->frame_counter - last_tick_frame_count);
    last_tick_frame_count = world->frame_counter;

    // Proceed every in-game half-day, or when jobs to recover changed
    static unsigned last_rlen = 0;
    bool check_time = (world->frame_counter - last_frame_count) >= DAY_TICKS/2;

    if (pending_recover.size() != last_rlen || check_time || !first_update_done)
    {
        recover_jobs(out);
        last_rlen = pending_recover.size();

        // If the half-day passed, proceed to update
        if (check_time || !first_update_done)
        {
            last_frame_count = world->frame_counter;

            update_job_data(out);
            process_constraints(out);
            first_update_done = true;
        }
    }


    return CR_OK;
}


/******************************
 *   ITEM COUNT CONSTRAINT    *
 ******************************/

static string get_constraint_material(ItemConstraint *cv)
{
    string text;
    if (!cv->material.isNone())
    {
        text.append(cv->material.toString());
        text.append(" ");
    }

    text.append(bitfield_to_string(cv->mat_mask));

    return text;
}

static ItemConstraint * create_new_constraint(bool is_craft, ItemTypeInfo item, MaterialInfo material, 
                                              df::dfhack_material_category mat_mask, item_quality::item_quality minqual, 
                                              PersistentDataItem * cfg, std::string str) 
{
    for (size_t i = 0; i < constraints.size(); i++)
    {
        ItemConstraint *ct = constraints[i];
        if (ct->is_craft == is_craft &&
            ct->item == item && ct->material == material &&
            ct->mat_mask.whole == mat_mask.whole &&
            ct->min_quality == minqual)
            return ct;
    }

    int weight = 0;
    if (item.subtype >= 0)
        weight += 10000;
    if (mat_mask.whole != 0)
        weight += 100;
    if (material.type >= 0)
        weight += (material.index >= 0 ? 5000 : 1000);

    ItemConstraint *nct = new ItemConstraint;
    nct->is_craft = is_craft;
    nct->item = item;
    nct->material = material;
    nct->mat_mask = mat_mask;
    nct->min_quality = minqual;
    nct->weight = weight;

    if (cfg)
        nct->config = *cfg;
    else
    {
        if (str.empty())
        {
            str = get_constraint_material(nct);
            str.append(item.toString());
        }

        nct->config = Core::getInstance().getWorld()->AddPersistentData("workflow/constraints");
        nct->init(str);
    }

    constraints.push_back(nct);
    return nct;
}

static ItemConstraint *get_constraint(color_ostream &out, const std::string &str, PersistentDataItem *cfg)
{
    std::vector<std::string> tokens;
    split_string(&tokens, str, "/");

    if (tokens.size() > 4)
        return NULL;

    bool is_craft = false;
    ItemTypeInfo item;

    if (tokens[0] == "ANY_CRAFT" || tokens[0] == "CRAFTS") {
        is_craft = true;
    } else if (!item.find(tokens[0]) || !item.isValid()) {
        out.printerr("Cannot find item type: %s\n", tokens[0].c_str());
        return NULL;
    }

    df::dfhack_material_category mat_mask;
    std::string maskstr = vector_get(tokens,1);
    if (!maskstr.empty() && !parseJobMaterialCategory(&mat_mask, maskstr)) {
        out.printerr("Cannot decode material mask: %s\n", maskstr.c_str());
        return NULL;
    }
    
    MaterialInfo material;
    std::string matstr = vector_get(tokens,2);
    if (!matstr.empty() && (!material.find(matstr) || !material.isValid())) {
        out.printerr("Cannot find material: %s\n", matstr.c_str());
        return NULL;
    }

    item_quality::item_quality minqual = item_quality::Ordinary;
    std::string qualstr = vector_get(tokens, 3);
    if(!qualstr.empty()) {
	    if(qualstr == "ordinary") minqual = item_quality::Ordinary;
	    else if(qualstr == "wellcrafted") minqual = item_quality::WellCrafted;
	    else if(qualstr == "finelycrafted") minqual = item_quality::FinelyCrafted;
	    else if(qualstr == "superior") minqual = item_quality::Superior;
	    else if(qualstr == "exceptional") minqual = item_quality::Exceptional;
	    else if(qualstr == "masterful") minqual = item_quality::Masterful;
	    else {
		    out.printerr("Cannot find quality: %s\nKnown qualities: ordinary, wellcrafted, finelycrafted, superior, exceptional, masterful\n", qualstr.c_str());
		    return NULL;
	    }
    }

    if (mat_mask.whole && material.isValid() && !material.matches(mat_mask)) {
        out.printerr("Material %s doesn't match mask %s\n", matstr.c_str(), maskstr.c_str());
        return NULL;
    }

    ItemConstraint *nct = create_new_constraint(is_craft, item, material, mat_mask, minqual, cfg, str);

    return nct;
}

static void delete_constraint(ItemConstraint *cv)
{
    int idx = linear_index(constraints, cv);
    if (idx >= 0)
    {
        for (vector<ProtectedJob *>::iterator it = constraints[idx]->jobs.begin(); it != constraints[idx]->jobs.end(); it++)
        {
            int idpj = linear_index((*it)->constraints, cv);
            if (idpj >= 0)
                vector_erase_at((*it)->constraints, idpj);
        }
        vector_erase_at(constraints, idx);
    }

    Core::getInstance().getWorld()->DeletePersistentData(cv->config);
    delete cv;
}

/******************************
 *   JOB-CONSTRAINT MAPPING   *
 ******************************/

static bool isCraftItem(df::item_type type)
{
    using namespace df::enums::job_type;

    auto lst = ENUM_ATTR(job_type, possible_item, MakeCrafts);
    for (size_t i = 0; i < lst.size; i++)
        if (lst.items[i] == type)
            return true;

    return false;
}

static void link_job_constraint(ProtectedJob *pj, df::item_type itype, int16_t isubtype,
                                df::dfhack_material_category mat_mask,
                                int16_t mat_type, int32_t mat_index,
                                bool is_craft = false, bool create_constraint = false)
{
    MaterialInfo mat(mat_type, mat_index);

    bool constraint_found = false;
    for (size_t i = 0; i < constraints.size(); i++)
    {
        ItemConstraint *ct = constraints[i];

        if (is_craft)
        {
            if (!ct->is_craft && !isCraftItem(ct->item.type))
                continue;
        }
        else
        {
            if (ct->item.type != itype ||
                (ct->item.subtype != -1 && ct->item.subtype != isubtype))
                continue;
        }

        if (!mat.matches(ct->material))
            continue;
        if (ct->mat_mask.whole)
        {
            if (mat.isValid())
            {
                if (!mat.matches(ct->mat_mask))
                    continue;
            }
            else
            {
                if (!(mat_mask.whole & ct->mat_mask.whole))
                    continue;
            }
        }

        if (linear_index(pj->constraints, ct) >= 0)
            continue;

        ct->jobs.push_back(pj);
        pj->constraints.push_back(ct);
        constraint_found = true;

        if (!ct->is_active && pj->isResumed())
            ct->is_active = true;
    }

    if (create_constraint && !constraint_found)
    {
        ItemTypeInfo item;
        item.type = itype;
        item.subtype = isubtype;

        ItemConstraint *nct = create_new_constraint(is_craft, item, mat, mat_mask, item_quality::Ordinary, NULL, "");
        nct->jobs.push_back(pj);
        pj->constraints.push_back(nct);
    }
}

static void compute_custom_job(ProtectedJob *pj, df::job *job)
{
    if (pj->reaction_id < 0)
        pj->reaction_id = linear_index(df::reaction::get_vector(),
                                       &df::reaction::code, job->reaction_name);

    df::reaction *r = df::reaction::find(pj->reaction_id);
    if (!r)
        return;

    for (size_t i = 0; i < r->products.size(); i++)
    {
        using namespace df::enums::reaction_product_item_flags;

        VIRTUAL_CAST_VAR(prod, df::reaction_product_itemst, r->products[i]);
        if (!prod || (prod->item_type < (df::item_type)0 && !prod->flags.is_set(CRAFTS)))
            continue;

        MaterialInfo mat(prod);
        df::dfhack_material_category mat_mask(0);

        bool get_mat_prod = prod->flags.is_set(GET_MATERIAL_PRODUCT);
        if (get_mat_prod || prod->flags.is_set(GET_MATERIAL_SAME))
        {
            int reagent_idx = linear_index(r->reagents, &df::reaction_reagent::code,
                                           prod->get_material.reagent_code);
            if (reagent_idx < 0)
                continue;

            int item_idx = linear_index(job->job_items, &df::job_item::reagent_index, reagent_idx);
            if (item_idx >= 0)
                mat.decode(job->job_items[item_idx]);
            else
            {
                VIRTUAL_CAST_VAR(src, df::reaction_reagent_itemst, r->reagents[reagent_idx]);
                if (!src)
                    continue;
                mat.decode(src);
            }

            if (get_mat_prod)
            {
                std::string code = prod->get_material.product_code;

                if (mat.isValid())
                {
                    int idx = linear_index(mat.material->reaction_product.id, code);
                    if (idx < 0)
                        continue;

                    mat.decode(mat.material->reaction_product.material, idx);
                }
                else
                {
                    if (code == "SOAP_MAT")
                        mat_mask.bits.soap = true;
                }
            }
        }

        link_job_constraint(pj, prod->item_type, prod->item_subtype,
                            mat_mask, mat.type, mat.index, prod->flags.is_set(CRAFTS)); 
    }
}

static void guess_job_material(df::job *job, MaterialInfo &mat, df::dfhack_material_category &mat_mask)
{
    using namespace df::enums::job_type;

    if (job->job_type == PrepareMeal)
        mat.decode(-1);
    else
        mat.decode(job);

    mat_mask.whole = job->material_category.whole;

    // Material from the job enum
    const char *job_material = ENUM_ATTR(job_type, material, job->job_type);
    if (job_material)
    {
        MaterialInfo info;
        if (info.findBuiltin(job_material))
            mat = info;
        else
            parseJobMaterialCategory(&mat_mask, job_material);
    }

    // Material from the job reagent
    if (!mat.isValid() && !job->job_items.empty() &&
        (job->job_items.size() == 1 ||
         job->job_items[0]->item_type == item_type::PLANT))
    {
        mat.decode(job->job_items[0]);

        switch (job->job_items[0]->item_type)
        {
        case item_type::WOOD:
            mat_mask.bits.wood = mat_mask.bits.wood2 = true;
            break;

        default:
            break;
        }
    }
}

static void compute_job_outputs(color_ostream &out, ProtectedJob *pj, bool create_constraint = false)
{
    using namespace df::enums::job_type;

    // Custom reactions handled in another function
    df::job *job = pj->job_copy;

    if (job->job_type == CustomReaction)
    {
        compute_custom_job(pj, job);
        return;
    }

    // Item type & subtype
    df::item_type itype = ENUM_ATTR(job_type, item, job->job_type);
    int16_t isubtype = job->item_subtype;

    if (itype == item_type::NONE && job->job_type != MakeCrafts)
        return;

    // Item material & material category
    MaterialInfo mat;
    df::dfhack_material_category mat_mask;
    guess_job_material(job, mat, mat_mask);

    // Job-specific code
    switch (job->job_type)
    {
    case SmeltOre:
        if (mat.inorganic)
        {
            std::vector<int16_t> &ores = mat.inorganic->metal_ore.mat_index;
            for (size_t i = 0; i < ores.size(); i++)
                link_job_constraint(pj, item_type::BAR, -1, 0, 0, ores[i]);
        }
        return;

    case ExtractMetalStrands:
        if (mat.inorganic)
        {
            std::vector<int16_t> &threads = mat.inorganic->thread_metal.mat_index;
            for (size_t i = 0; i < threads.size(); i++)
                link_job_constraint(pj, item_type::THREAD, -1, 0, 0, threads[i]);
        }
        return;

    case PrepareMeal:
        if (job->mat_type != -1)
        {
            std::vector<df::itemdef_foodst*> &food = df::itemdef_foodst::get_vector();
            for (size_t i = 0; i < food.size(); i++)
                if (food[i]->level == job->mat_type)
                    link_job_constraint(pj, item_type::FOOD, i, 0, -1, -1);
            return;
        }
        break;

    case MakeCrafts:
        link_job_constraint(pj, item_type::NONE, -1, mat_mask, mat.type, mat.index, true);
        return;

#define PLANT_PROCESS_MAT(flag, tag) \
        if (mat.plant && mat.plant->flags.is_set(plant_raw_flags::flag)) \
            mat.decode(mat.plant->material_defs.type_##tag, \
                       mat.plant->material_defs.idx_##tag); \
        else mat.decode(-1);
    case BrewDrink:
        PLANT_PROCESS_MAT(DRINK, drink);
        break;
    case MillPlants:
        PLANT_PROCESS_MAT(MILL, mill);
        break;
    case ProcessPlants:
        PLANT_PROCESS_MAT(THREAD, thread);
        break;
    case ProcessPlantsBag:
        PLANT_PROCESS_MAT(LEAVES, leaves);
        break;
    case ProcessPlantsBarrel:
        PLANT_PROCESS_MAT(EXTRACT_BARREL, extract_barrel);
        break;
    case ProcessPlantsVial:
        PLANT_PROCESS_MAT(EXTRACT_VIAL, extract_vial);
        break;
    case ExtractFromPlants:
        PLANT_PROCESS_MAT(EXTRACT_STILL_VIAL, extract_still_vial);
        break;
#undef PLANT_PROCESS_MAT

    default:
        break;
    }

    link_job_constraint(pj, itype, isubtype, mat_mask, mat.type, mat.index, false, create_constraint);
}

static void map_job_constraints(color_ostream &out)
{
    melt_active = false;

    for (size_t i = 0; i < constraints.size(); i++)
    {
        constraints[i]->jobs.clear();
        constraints[i]->is_active = false;
    }

    for (TKnownJobs::const_iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
    {
        ProtectedJob *pj = it->second;

        pj->constraints.clear();

        if (!pj->isLive())
            continue;

        if (!melt_active && pj->actual_job->job_type == job_type::MeltMetalObject)
            melt_active = pj->isResumed();

        compute_job_outputs(out, pj);
    }
}

/******************************
 *  ITEM-CONSTRAINT MAPPING   *
 ******************************/

static void dryBucket(df::item *item)
{
    for (size_t i = 0; i < item->itemrefs.size(); i++)
    {
        df::general_ref *ref = item->itemrefs[i];
        if (ref->getType() == general_ref_type::CONTAINS_ITEM)
        {
            df::item *obj = ref->getItem();

            if (obj &&
                obj->getType() == item_type::LIQUID_MISC &&
                obj->getMaterial() == builtin_mats::WATER)
            {
                obj->flags.bits.garbage_collect = true;
                obj->flags.bits.hidden = true;
            }
        }
    }
}

static bool itemBusy(df::item *item)
{
    using namespace df::enums::item_type;

    for (size_t i = 0; i < item->itemrefs.size(); i++)
    {
        df::general_ref *ref = item->itemrefs[i];
        if (ref->getType() == general_ref_type::CONTAINS_ITEM)
        {
            df::item *obj = ref->getItem();
            if (obj && !obj->flags.bits.garbage_collect)
                return true;
        }
        else if (ref->getType() == general_ref_type::CONTAINS_UNIT)
            return true;
        else if (ref->getType() == general_ref_type::UNIT_HOLDER)
        {
            if (!item->flags.bits.in_job)
                return true;
        }
        else if (ref->getType() == general_ref_type::CONTAINED_IN_ITEM)
        {
            df::item *obj = ref->getItem();
            if (!obj)
                continue;

            // Stuff in flasks and backpacks is busy
            df::item_type type = obj->getType();
            if ((type == FLASK && item->getType() == DRINK) || type == BACKPACK)
                return true;
        }
    }

    return false;
}

static bool itemInRealJob(df::item *item)
{
    if (!item->flags.bits.in_job)
        return false;

    auto ref = Items::getSpecificRef(item, specific_ref_type::JOB);
    if (!ref || !ref->job)
        return true;

    return ENUM_ATTR(job_type, type, ref->job->job_type)
               != job_type_class::Hauling;
}

static void map_job_items(color_ostream &out)
{
    for (size_t i = 0; i < constraints.size(); i++)
    {
        constraints[i]->item_amount = 0;
        constraints[i]->item_count = 0;
        constraints[i]->item_inuse = 0;
    }

    meltable_count = 0;

    // Precompute a bitmask with the bad flags
    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(left_margin) bad_flags.bits.left_margin = true;
    F(dump); F(forbid); F(garbage_collect);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction); F(artifact1);
#undef F

    bool dry_buckets = isOptionEnabled(CF_DRYBUCKETS);

    std::vector<df::item*> &items = world->items.other[items_other_id::ANY_FREE];

    for (size_t i = 0; i < items.size(); i++)
    {
        df::item *item = items[i];

        if (item->flags.whole & bad_flags.whole)
            continue;

        df::item_type itype = item->getType();
        int16_t isubtype = item->getSubtype();
        int16_t imattype = item->getActualMaterial();
        int32_t imatindex = item->getActualMaterialIndex();

        bool is_invalid = false;

		// don't count worn items
		if (item->getWear() >= 1) 
			is_invalid = true;

        // Special handling
        switch (itype) {
        case item_type::BUCKET:
            if (dry_buckets && !item->flags.bits.in_job)
                dryBucket(item);
            break;

        case item_type::THREAD:
            if (item->getTotalDimension() < 15000)
                is_invalid = true;
            break;

        case item_type::CLOTH:
            if (item->getTotalDimension() < 10000)
                is_invalid = true;
            break;

        default:
            break;
        }

        if (item->flags.bits.melt && !item->flags.bits.owned && !itemBusy(item))
            meltable_count++;

        // Match to constraints
        TMaterialCache::key_type matkey(imattype, imatindex);

        for (size_t i = 0; i < constraints.size(); i++)
        {
            ItemConstraint *cv = constraints[i];

            if (cv->is_craft)
            {
                if (!isCraftItem(itype))
                    continue;
            }
            else
            {
                if (cv->item.type != itype ||
                    (cv->item.subtype != -1 && cv->item.subtype != isubtype))
                    continue;
            }
	    if(item->getQuality() < cv->min_quality) {
		    continue;
	    }

            TMaterialCache::iterator it = cv->material_cache.find(matkey);

            bool ok = true;
            if (it != cv->material_cache.end())
                ok = it->second;
            else
            {
                MaterialInfo mat(imattype, imatindex);
                ok = mat.matches(cv->material) &&
                     (cv->mat_mask.whole == 0 || mat.matches(cv->mat_mask));
                cv->material_cache[matkey] = ok;
            }

            if (!ok)
                continue;

            if (is_invalid ||
                item->flags.bits.owned ||
                item->flags.bits.in_chest ||
                item->isAssignedToStockpile() ||
                itemInRealJob(item) ||
                itemBusy(item))
            {
                cv->item_inuse++;
            }
            else
            {
                cv->item_count++;
                cv->item_amount += item->getStackSize();
            }
        }
    }

    for (size_t i = 0; i < constraints.size(); i++)
    {
        constraints[i]->computeRequest();
    }
}

/******************************
 *   ITEM COUNT CONSTRAINT    *
 ******************************/

static std::string shortJobDescription(df::job *job);

static void setJobResumed(color_ostream &out, ProtectedJob *pj, bool goal)
{
    bool current = pj->isResumed();

    pj->set_resumed(goal);

    if (goal != current)
    {
        out.print("%s %s%s\n",
                     (goal ? "Resuming" : "Suspending"),
                     shortJobDescription(pj->actual_job).c_str(),
                     (!goal || pj->isActuallyResumed() ? "" : " (delayed)"));
    }
}

static void update_jobs_by_constraints(color_ostream &out)
{
    for (TKnownJobs::const_iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
    {
        ProtectedJob *pj = it->second;
        if (!pj->isLive())
            continue;

        if (pj->actual_job->job_type == job_type::MeltMetalObject &&
            isOptionEnabled(CF_AUTOMELT))
        {
            setJobResumed(out, pj, meltable_count > 0);
            continue;
        }

        if (pj->constraints.empty())
            continue;

        int resume_weight = -1;
        int suspend_weight = -1;

        for (size_t i = 0; i < pj->constraints.size(); i++)
        {
            if (pj->constraints[i]->request_resume)
                resume_weight = std::max(resume_weight, pj->constraints[i]->weight);
            if (pj->constraints[i]->request_suspend)
                suspend_weight = std::max(suspend_weight, pj->constraints[i]->weight);
        }

        bool goal = pj->isResumed();

        if (resume_weight >= 0 && resume_weight >= suspend_weight)
            goal = true;
        else if (suspend_weight >= 0 && suspend_weight >= resume_weight)
            goal = false;

        setJobResumed(out, pj, goal);
    }

    for (size_t i = 0; i < constraints.size(); i++)
    {
        ItemConstraint *ct = constraints[i];

        bool is_running = false;
        for (size_t j = 0; j < ct->jobs.size(); j++)
            if (!!(is_running = ct->jobs[j]->isResumed()))
                break;

        std::string info = ct->item.toString();

        if (ct->is_craft)
            info = "crafts";

        if (ct->material.isValid())
            info = ct->material.toString() + " " + info;
        else if (ct->mat_mask.whole)
            info = bitfield_to_string(ct->mat_mask) + " " + info;

        if (is_running != ct->is_active)
        {
            if (is_running && ct->request_resume)
                Gui::showAnnouncement("Resuming production: " + info, 2, false);
            else if (!is_running && !ct->request_resume)
                Gui::showAnnouncement("Stopping production: " + info, 3, false);
        }

        if (ct->request_resume && !is_running)
        {
            if (!ct->cant_resume_reported)
                Gui::showAnnouncement("Cannot produce: " + info, 6, true);
            ct->cant_resume_reported = true;
        }
        else
        {
            ct->cant_resume_reported = false;
        }
    }
}

static void process_constraints(color_ostream &out)
{
    if (constraints.empty() &&
        !isOptionEnabled(CF_DRYBUCKETS | CF_AUTOMELT))
        return;

    map_job_constraints(out);
    map_job_items(out);
    update_jobs_by_constraints(out);
}

/******************************
 *  PRINTING AND THE COMMAND  *
 ******************************/

static std::string shortJobDescription(df::job *job)
{
    std::string rv = stl_sprintf("job %d: ", job->id);

    if (job->job_type != job_type::CustomReaction)
        rv += ENUM_KEY_STR(job_type, job->job_type);
    else
        rv += job->reaction_name;

    MaterialInfo mat;
    df::dfhack_material_category mat_mask;
    guess_job_material(job, mat, mat_mask);

    if (mat.isValid())
        rv += " [" + mat.toString() + "]";
    else if (mat_mask.whole)
        rv += " [" + bitfield_to_string(mat_mask) + "]";

    return rv;
}

static void print_constraint(color_ostream &out, ItemConstraint *cv, bool no_job = false, std::string prefix = "")
{
    Console::color_value color;
    if (cv->request_resume)
        color = COLOR_GREEN;
    else if (cv->request_suspend)
        color = COLOR_CYAN;
    else
        color = COLOR_DARKGREY;

    out.color(color);
    out << prefix << "Constraint " << flush;
    out.color(COLOR_GREY);
    out << cv->config.val() << " " << flush;
    out.color(color);
    out << (cv->goalByCount() ? "count " : "amount ")
           << cv->goalCount() << " (gap " << cv->goalGap() << ")" << endl;
    out.reset_color();

    if (cv->item_count || cv->item_inuse)
        out << prefix << "  items: amount " << cv->item_amount << "; "
                         << cv->item_count << " stacks available, "
                         << cv->item_inuse << " in use." << endl;

    if (no_job) return;

    if (cv->jobs.empty())
        out.printerr("  (no jobs)\n");

    std::vector<ProtectedJob*> unique_jobs;
    std::vector<int> unique_counts;

    for (size_t i = 0; i < cv->jobs.size(); i++)
    {
        ProtectedJob *pj = cv->jobs[i];
        for (size_t j = 0; j < unique_jobs.size(); j++)
        {
            if (unique_jobs[j]->building_id == pj->building_id &&
                *unique_jobs[j]->actual_job == *pj->actual_job)
            {
                unique_counts[j]++;
                goto next_job;
            }
        }

        unique_jobs.push_back(pj);
        unique_counts.push_back(1);
    next_job:;
    }

    for (size_t i = 0; i < unique_jobs.size(); i++)
    {
        ProtectedJob *pj = unique_jobs[i];
        df::job *job = pj->actual_job;

        std::string start = prefix + "  " + shortJobDescription(job);

        if (!pj->isActuallyResumed())
        {
            if (pj->want_resumed)
            {
                out.color(COLOR_YELLOW);
                out << start << " (delayed)" << endl;
            }
            else
            {
                out.color(COLOR_BLUE);
                out << start << " (suspended)" << endl;
            }
        }
        else
        {
            out.color(COLOR_GREEN);
            out << start << endl;
        }

        out.reset_color();

        if (unique_counts[i] > 1)
            out << prefix << "    (" << unique_counts[i] << " copies)" << endl;
    }
}

static void print_job(color_ostream &out, ProtectedJob *pj)
{
    if (!pj)
        return;

    df::job *job = pj->isLive() ? pj->actual_job : pj->job_copy;

    Job::printJobDetails(out, job);

    if (job->job_type == job_type::MeltMetalObject &&
        isOptionEnabled(CF_AUTOMELT))
    {
        if (meltable_count <= 0)
            out.color(COLOR_CYAN);
        else if (pj->want_resumed && !pj->isActuallyResumed())
            out.color(COLOR_YELLOW);
        else
            out.color(COLOR_GREEN);
        out << "  Meltable: " << meltable_count << " objects." << endl;
        out.reset_color();
    }

    for (size_t i = 0; i < pj->constraints.size(); i++)
        print_constraint(out, pj->constraints[i], true, "  ");
}

/******************************
 *  Inventory Monitor         *
 ******************************/
#define MAX_ITEM_NAME 17
#define MAX_MASK 10
#define MAX_MATERIAL 21

#define SIDEBAR_WIDTH 30
#define COLOR_TITLE COLOR_BLUE
#define COLOR_UNSELECTED COLOR_GREY
#define COLOR_SELECTED COLOR_WHITE
#define COLOR_HIGHLIGHTED COLOR_GREEN

#define ITEMDEF_VECTORS \
    ITEM(WEAPON, weapons, itemdef_weaponst) \
    ITEM(TRAPCOMP, trapcomps, itemdef_trapcompst) \
    ITEM(TOY, toys, itemdef_toyst) \
    ITEM(TOOL, tools, itemdef_toolst) \
    ITEM(INSTRUMENT, instruments, itemdef_instrumentst) \
    ITEM(ARMOR, armor, itemdef_armorst) \
    ITEM(AMMO, ammo, itemdef_ammost) \
    ITEM(SIEGEAMMO, siege_ammo, itemdef_siegeammost) \
    ITEM(GLOVES, gloves, itemdef_glovesst) \
    ITEM(SHOES, shoes, itemdef_shoesst) \
    ITEM(SHIELD, shields, itemdef_shieldst) \
    ITEM(HELM, helms, itemdef_helmst) \
    ITEM(PANTS, pants, itemdef_pantsst) \
    ITEM(FOOD, food, itemdef_foodst)

namespace wf_ui
{
    /*
     * Utility Functions
     */
    typedef int8_t UIColor;

    const int ascii_to_enum_offset = interface_key::STRING_A048 - '0';

    inline string int_to_string(const int n)
    {
        return static_cast<ostringstream*>( &(ostringstream() << n) )->str();
    }

    static void set_to_limit(int &value, const int maximum, const int min = 0)
    {
        if (value < min)
            value = min;
        else if (value > maximum)
            value = maximum;
    }

    inline void paint_text(const UIColor color, const int &x, const int &y, const std::string &text, const UIColor background = 0)
    {
        Screen::paintString(Screen::Pen(' ', color, background), x, y, text);
    }

    static string pad_string(string text, const int size, const bool front = true, const bool trim = false)
    {
        if (text.length() > size)
        {
            if (trim && size > 10)
            {
                text = text.substr(0, size-3);
                text.append("...");
            }
            return text;
        }

        string aligned(size - text.length(), ' ');
        if (front)
        {
            aligned.append(text);
            return aligned;
        }
        else
        {
            text.append(aligned);
            return text;
        }
    }

    /*
     * Adjustment Dialog
     */

    class AdjustmentScreen
    {
    public:
        int32_t x, y, left_margin;

        AdjustmentScreen();
        void reset();
        bool feed(set<df::interface_key> *input, ItemConstraint *cv, ProtectedJob *pj = NULL);
        void render(ItemConstraint *cv);

    protected:
        int32_t adjustment_ui_display_start;

        virtual void onConstraintChanged() {}
        virtual void onModeChanged() {}

    private:
        bool edit_limit, edit_gap;
        string edit_string;

    };

    AdjustmentScreen::AdjustmentScreen()
    {
        reset();
    }

    void AdjustmentScreen::reset()
    {
        edit_gap = false;
        edit_limit = false;
        adjustment_ui_display_start = 24;
    }

    bool AdjustmentScreen::feed(set<df::interface_key> *input, ItemConstraint *cv, ProtectedJob *pj /* = NULL */)
    {
        if ((edit_limit || edit_gap))
        {
            df::interface_key last_token = *input->rbegin();
            if (last_token == interface_key::STRING_A000)
            {
                // Backspace
                if (edit_string.length() > 0)
                {
                    edit_string.erase(edit_string.length()-1);
                }

                return true;
            }

            if (edit_string.length() >= 6)
                return true;

            if (last_token >= interface_key::STRING_A048 && last_token <= interface_key::STRING_A057)
            {
                // Numeric character
                edit_string += last_token - ascii_to_enum_offset;
            }
            else if (input->count(interface_key::SELECT) || input->count(interface_key::LEAVESCREEN))
            {
                if (input->count(interface_key::SELECT) && edit_string.length() > 0)
                {
                    if (edit_limit)
                        cv->setGoalCount(atoi(edit_string.c_str()));
                    else
                        cv->setGoalGap(atoi(edit_string.c_str()));

                    onConstraintChanged();
                }
                edit_string.clear();
                edit_limit = false;
                edit_gap = false;
            }
            else if (last_token == interface_key::STRING_A000)
            {
                // Backspace
                if (edit_string.length() > 0)
                {
                    edit_string.erase(edit_string.length()-1);
                }
            }

            return true;
        }
        else if (input->count(interface_key::CUSTOM_L))
        {
            edit_string = int_to_string(cv->goalCount());
            edit_limit = true;
        }
        else if (input->count(interface_key::CUSTOM_G))
        {
            edit_string = int_to_string(cv->goalGap());
            edit_gap = true;
        }
        else if (input->count(interface_key::CUSTOM_N))
        {
            cv->setGoalByCount(!cv->goalByCount());
            onModeChanged();
        }
        else if (input->count(interface_key::CUSTOM_T))
        {
            if (cv)
            {
                // Remove tracking
                if (pj)
                {
                    for (vector<ItemConstraint*>::iterator it = pj->constraints.begin(); it < pj->constraints.end(); it++)
                        delete_constraint(*it);

                    forget_job(color_ostream_proxy(Core::getInstance().getConsole()), pj);
                }
                else
                    delete_constraint(cv);
            }
            else
            {
                // Add tracking
                return false;
            }

            onConstraintChanged();
        }
        else
        {
            return false;
        }

        return true;
    }

    void AdjustmentScreen::render(ItemConstraint *cv)
    {
        left_margin = gps->dimx - 30;
        x = left_margin;
        y = adjustment_ui_display_start;
        OutputString(COLOR_BROWN, x, y, "Workflow Settings", true, left_margin);
        if (cv != NULL)
        {
            string text;
            text.reserve(20);

            text.append("Available: ");
            text.append(int_to_string((cv->goalByCount()) ? cv->item_count : cv->item_amount));
            text.append(" ");
            text.append((cv->goalByCount()) ? "Stacks" : "Items");

            OutputString(15, x, y, text, true, left_margin);

            text.clear();
            text.append("In use   : ");
            text.append(int_to_string(cv->item_inuse));
            OutputString(15, x, y, text, true, left_margin);

            ++y;
            text.clear();
            text.append("Limit : ");
            text.append((edit_limit) ? edit_string : int_to_string(cv->goalCount()));
            OutputHotkeyString(x, y, text.c_str(), "l");
            if (edit_limit)
                OutputString(10, x, y, "_");

            ++y;
            x = left_margin;
            text.clear();
            text.append("Gap   : ");
            text.append((edit_gap) ? edit_string : int_to_string(cv->goalGap()));
            OutputHotkeyString(x, y, text.c_str(), "g");
            if (edit_gap)
                OutputString(10, x, y, "_");

            ++y;
            x = left_margin;
            OutputHotkeyString(x, y, "Disable Tracking", "t", true, left_margin);
            OutputHotkeyString(x, y, (cv->goalByCount()) ? "Count Items" : "Count Stacks", "n");
        }
        else
            OutputHotkeyString(x, y, "Enable Tracking", "t", true, left_margin);
    }


    /*
     * List classes
     */
    template <typename T>
    class ListEntry
    {
    public:
        T elem;
        string text;
        bool selected;

        ListEntry(string text, T elem)
        {
            this->text = text;
            this->elem = elem;
            selected = false;
        }
    };

    template <typename T>
    class ListColumn
    {
    public:
        string title;
        int highlighted_index;
        int display_start_offset;
        int32_t bottom_margin, search_margin, left_margin;
        bool search_entry_mode;
        bool multiselect;
        bool allow_null;
        bool auto_select;
        bool force_sort;

        ListColumn()
        {
            clear();
            left_margin = 2;
            bottom_margin = 3;
            search_margin = 38;
            highlighted_index = 0;
            multiselect = false;
            allow_null = true;
            auto_select = false;
            search_entry_mode = false;
            force_sort = false;
        }

        void clear()
        {
            list.clear();
            display_list.clear();
            display_start_offset = 0;
            max_item_width = 0;
            resize();
        }

        void resize()
        {
            display_max_rows = gps->dimy - 4 - bottom_margin;
        }

        void add(ListEntry<T> &entry)
        {
            list.push_back(entry);
            if (entry.text.length() > max_item_width)
                max_item_width = entry.text.length();
        }

        void add(string &text, T &elem)
        {
            list.push_back(ListEntry<T>(text, elem));
            if (text.length() > max_item_width)
                max_item_width = text.length();
        }

        virtual void display_extras(const T &elem, int32_t &x, int32_t &y) const {}

        void display(const bool is_selected_column) const
        {
            int32_t y = 2;
            paint_text(COLOR_TITLE, left_margin, y, title);

            int last_index_able_to_display = display_start_offset + display_max_rows;
            for (int i = display_start_offset; i < display_list.size() && i < last_index_able_to_display; i++)
            {
                ++y;
                UIColor fg_color = (display_list[i]->selected) ? COLOR_SELECTED : COLOR_UNSELECTED;
                UIColor bg_color = (is_selected_column && i == highlighted_index) ? COLOR_HIGHLIGHTED : COLOR_BLACK;
                paint_text(fg_color, left_margin, y, display_list[i]->text, bg_color);
                int x = left_margin + display_list[i]->text.length() + 1;
                display_extras(display_list[i]->elem, x, y);
            }

            if (is_selected_column)
            {
                y = gps->dimy - bottom_margin;
                int32_t x = search_margin;
                OutputHotkeyString(x, y, "Search" ,"S");
                if (!search_string.empty() || search_entry_mode)
                {
                    OutputString(COLOR_WHITE, x, y, ": ");
                    OutputString(COLOR_WHITE, x, y, search_string);
                    if (search_entry_mode)
                        OutputString(COLOR_LIGHTGREEN, x, y, "_");
                }
            }
        }

        void filter_display()
        {
            ListEntry<T> *prev_selected = (getDisplayListSize() > 0) ? display_list[highlighted_index] : NULL;
            display_list.clear();
            for (size_t i = 0; i < list.size(); i++)
            {
                if (search_string.empty() || list[i].text.find(search_string) != string::npos)
                {
                    ListEntry<T> *entry = &list[i];
                    display_list.push_back(entry);
                    if (entry == prev_selected)
                        highlighted_index = display_list.size() - 1;
                }
            }
            changeHighlight(0);
        }

        void selectDefaultEntry()
        {
            for (size_t i = 0; i < display_list.size(); i++)
            {
                if (display_list[i]->selected)
                {
                    highlighted_index = i;
                    break;
                }
            }
        }

        void validateHighlight()
        {
            set_to_limit(highlighted_index, display_list.size() - 1);

            if (highlighted_index < display_start_offset)
                display_start_offset = highlighted_index;
            else if (highlighted_index >= display_start_offset + display_max_rows)
                display_start_offset = highlighted_index - display_max_rows + 1;

            if (auto_select || (!allow_null && list.size() == 1))
                display_list[highlighted_index]->selected = true;
        }

        void changeHighlight(const int highlight_change, const int offset_shift = 0)
        {
            if (!initHighlightChange())
                return;

            highlighted_index += highlight_change + offset_shift * display_max_rows;

            display_start_offset += offset_shift * display_max_rows;
            set_to_limit(display_start_offset, max(0, (int)(display_list.size())-display_max_rows));
            validateHighlight();
        }

        void setHighlight(const int index)
        {
            if (!initHighlightChange())
                return;

            highlighted_index = index;
            validateHighlight();
        }

        bool initHighlightChange()
        {
            if (display_list.size() == 0)
                return false;

            if (auto_select && !multiselect)
            {
                for (vector< ListEntry<T> >::iterator it = list.begin(); it != list.end(); it++)
                {
                    it->selected = false;
                }
            }

            return true;
        }

        void toggleHighlighted()
        {
            if (auto_select)
                return;

            ListEntry<T> *entry = display_list[highlighted_index];
            if (!multiselect || !allow_null)
            {
                int selected_count = 0;
                for (size_t i = 0; i < list.size(); i++)
                {
                    if (!multiselect && !entry->selected)
                        list[i].selected = false;
                    if (!allow_null && list[i].selected)
                        selected_count++;
                }

                if (!allow_null && entry->selected && selected_count == 1)
                    return;
            }

            entry->selected = !entry->selected;
        }

        vector<T*> getSelectedElems(bool only_one = false)
        {
            vector<T*> results;
            for (vector< ListEntry<T> >::iterator it = list.begin(); it != list.end(); it++)
            {
                if ((*it).selected)
                {
                    results.push_back(&(*it).elem);
                    if (only_one)
                        break;
                }
            }

            return results;
        }

        T* getFirstSelectedElem()
        {
            vector<T*> results = getSelectedElems(true);
            if (results.size() == 0)
                return NULL;
            else
                return results[0];
        }

        size_t getDisplayListSize()
        {
            return display_list.size();
        }

        size_t getBaseListSize()
        {
            return list.size();
        }

        bool feed(set<df::interface_key> *input)
        {
            if  (input->count(interface_key::CURSOR_UP))
            {
                search_entry_mode = false;
                changeHighlight(-1);
            }
            else if  (input->count(interface_key::CURSOR_DOWN))
            {
                search_entry_mode = false;
                changeHighlight(1);
            }
            else if  (input->count(interface_key::STANDARDSCROLL_PAGEUP))
            {
                search_entry_mode = false;
                changeHighlight(0, -1);
            }
            else if  (input->count(interface_key::STANDARDSCROLL_PAGEDOWN))
            {
                search_entry_mode = false;
                changeHighlight(0, 1);
            }
            else if (search_entry_mode)
            {
                // Search query typing mode

                df::interface_key last_token = *input->rbegin();
                if (last_token >= interface_key::STRING_A032 && last_token <= interface_key::STRING_A126)
                {
                    // Standard character
                    search_string += last_token - ascii_to_enum_offset;
                    filter_display();
                }
                else if (last_token == interface_key::STRING_A000)
                {
                    // Backspace
                    if (search_string.length() > 0)
                    {
                        search_string.erase(search_string.length()-1);
                        filter_display();
                    }
                }
                else if (input->count(interface_key::SELECT) || input->count(interface_key::LEAVESCREEN))
                {
                    // ENTER or ESC: leave typing mode
                    search_entry_mode = false;
                }
                else if  (input->count(interface_key::CURSOR_LEFT) || input->count(interface_key::CURSOR_RIGHT))
                {
                    // Arrow key pressed. Leave entry mode and allow screen to process key
                    search_entry_mode = false;
                    return false;
                }

                return true;
            }

            // Not in search query typing mode
            else if  (input->count(interface_key::SELECT) && !auto_select)
            {
                toggleHighlighted();
            }
            else if  (input->count(interface_key::CUSTOM_S))
            {
                search_entry_mode = true;
            }
            else if  (input->count(interface_key::CUSTOM_SHIFT_S))
            {
                search_string.clear();
                filter_display();
            }
            else if (enabler->tracking_on && gps->mouse_x != -1 && gps->mouse_y != -1 && enabler->mouse_lbut)
            {
                return setHighlightByMouse();
            }
            else
                return false;

            return true;
        }

        bool setHighlightByMouse()
        {
            if (gps->mouse_y >= 3 && gps->mouse_y < display_max_rows + 3 &&
                gps->mouse_x >= left_margin && gps->mouse_x < left_margin + max_item_width)
            {
                int new_index = display_start_offset + gps->mouse_y - 3;
                if (new_index < display_list.size())
                    setHighlight(new_index);

                enabler->mouse_lbut = enabler->mouse_rbut = 0;

                return true;
            }

            return false;
        }

        static bool compareText(ListEntry<T> const& a, ListEntry<T> const& b)
        {
            return a.text.compare(b.text) < 0;
        }

        void doSort(bool (*function)(ListEntry<T> const&, ListEntry<T> const&))
        {
            if (force_sort || list.size() < 100)
                std::sort(list.begin(), list.end(), function);

            filter_display();
        }

        virtual void sort()
        {
            doSort(&compareText);
        }


        private:
            vector< ListEntry<T> > list;
            vector< ListEntry<T>* > display_list;
            string search_string;
            int display_max_rows;
            int max_item_width;
    };

    class viewscreenChooseMaterial : public dfhack_viewscreen
    {
    public:
        static bool reset_list;

        viewscreenChooseMaterial(ItemConstraint *cv = NULL);
        void feed(set<df::interface_key> *input);
        void render();

        std::string getFocusString() { return "wfchoosemat"; }

    private:
        ItemConstraint *cv;

        ListColumn<ItemTypeInfo> items_column;
        ListColumn<df::dfhack_material_category> masks_column;
        ListColumn<MaterialInfo> materials_column;
        vector< ListEntry<df::dfhack_material_category> > all_masks;

        int selected_column;

        void populateItems();
        void populateMasks(const bool set_defaults = false);
        void populateMaterials(const bool set_defaults = false);

        bool addMaterialEntry(df::dfhack_material_category &selected_category, 
                                MaterialInfo &material, string name, const bool set_defaults);

        virtual void resize(int32_t x, int32_t y);

        void validateColumn();
    };

    bool viewscreenChooseMaterial::reset_list = false;


    viewscreenChooseMaterial::viewscreenChooseMaterial(ItemConstraint *cv /*= NULL*/)
    {
        this->cv = cv;
        selected_column = 0;
        items_column.title = "Item";
        items_column.allow_null = false;
        items_column.multiselect = false;
        items_column.auto_select = true;
        items_column.force_sort = true;
        masks_column.title = "Type";
        masks_column.multiselect = true;
        masks_column.left_margin = MAX_ITEM_NAME + 3;
        materials_column.left_margin = MAX_ITEM_NAME + MAX_MASK + 4;
        materials_column.title = "Material";


        populateItems();
        items_column.changeHighlight(0);

        vector<string> raw_masks;
        df::dfhack_material_category full_mat_mask, curr_mat_mask;
        full_mat_mask.whole = -1;
        curr_mat_mask.whole = 1;
        bitfield_to_string(&raw_masks, full_mat_mask);
        for (int i = 0; i < raw_masks.size(); i++)
        {
            if (raw_masks[i][0] == '?')
                break;

            all_masks.push_back(ListEntry<df::dfhack_material_category>(pad_string(raw_masks[i], MAX_MASK, false), curr_mat_mask));
            curr_mat_mask.whole <<= 1;
        }
        populateMasks(cv != NULL);
        populateMaterials(cv != NULL);

        masks_column.selectDefaultEntry();
        materials_column.selectDefaultEntry();
        materials_column.changeHighlight(0);
    }

    void viewscreenChooseMaterial::populateItems()
    {
        items_column.clear();
        if (cv != NULL)
        {
            items_column.add(cv->item.toString(), cv->item);
        }
        else
        {
            typedef df::enum_traits<df::item_type> traits;
            int size = traits::last_item_value-traits::first_item_value+1;
            for (size_t i = 0; i < size; i++)
            {
                //string item_name = traits::key_table[i];
                df::world_raws::T_itemdefs &defs = df::global::world->raws.itemdefs;
                df::item_type val = df::item_type(i);
                switch (val)
                {
#define ITEM(type_string,vec,tclass) \
                    case df::item_type::type_string: \
                        for (size_t j = 0; j < defs.vec.size(); j++) \
                        { \
                            if (defs.vec[j]->name[0] == '?') \
                                continue; \
                            DFHack::ItemTypeInfo item; \
                            item.type = (df::item_type) i; \
                            item.subtype = j; \
                            item.custom = defs.vec[j]; \
                            items_column.add(defs.vec[j]->name, item); \
                        } \
                        break;

                    ITEMDEF_VECTORS
#undef ITEM
                default:
                    DFHack::ItemTypeInfo item;
                    item.type = (df::item_type) i;
                    string name = item.toString();
                    if (name[0] != '?')
                        items_column.add(name, item);
                    break;
                }
            }
        }
        items_column.sort();
        items_column.changeHighlight(0);
    }

    void viewscreenChooseMaterial::populateMasks(const bool set_defaults /*= false */)
    {
        masks_column.clear();
        for (vector< ListEntry<df::dfhack_material_category> >::iterator it = all_masks.begin(); it != all_masks.end(); it++)
        {
            auto entry = *it;
            if (set_defaults)
            {
                if (cv->mat_mask.whole & entry.elem.whole)
                    entry.selected = true;
            }
            masks_column.add(entry);
        }
        masks_column.sort();
    }
    
    void viewscreenChooseMaterial::populateMaterials(const bool set_defaults /*= false */)
    {
        materials_column.clear();
        df::dfhack_material_category selected_category;
        vector<df::dfhack_material_category *> selected_materials = masks_column.getSelectedElems();
        if (selected_materials.size() == 1)
            selected_category = *selected_materials[0];
        else if (selected_materials.size() > 1)
            return;

        df::world_raws &raws = world->raws;
        for (int i = 1; i < DFHack::MaterialInfo::NUM_BUILTIN; i++)
        {
            auto obj = raws.mat_table.builtin[i];
            if (obj)
            {
                MaterialInfo material;
                material.decode(i, -1);
                addMaterialEntry(selected_category, material, material.toString(), set_defaults);
            }
        }

        for (size_t i = 0; i < raws.inorganics.size(); i++)
        {
            df::inorganic_raw *p = raws.inorganics[i];
            MaterialInfo material;
            material.decode(0, i);
            addMaterialEntry(selected_category, material, material.toString(), set_defaults);
        }

        for (size_t i = 0; i < raws.plants.all.size(); i++)
        {
            df::plant_raw *p = raws.plants.all[i];
            string basename = p->name;

            MaterialInfo material;
            material.decode(p->material_defs.type_basic_mat, p->material_defs.idx_basic_mat);
            if (!selected_category.whole || material.matches(selected_category))
            {
                if (p->material.size() > 1)
                    basename.append(" (all)");

                ListEntry<MaterialInfo> entry(pad_string(basename, MAX_MATERIAL, false), material);
                if (set_defaults)
                {
                    if (cv->material.matches(material))
                        entry.selected = true;
                }
                materials_column.add(entry);

                for (size_t j = 0; p->material.size() > 1 && j < p->material.size(); j++)
                {
                    MaterialInfo material;
                    material.decode(DFHack::MaterialInfo::PLANT_BASE+j, i);
                    //if (addMaterialEntry(selected_category, material, basename+" (" + p->material[j]->id + "): " + material.toString(), set_defaults))
                    if (addMaterialEntry(selected_category, material, material.toString(), set_defaults))
                        entry.selected = false;
                }
            }
        }

        for (size_t i = 0; i < raws.creatures.all.size(); i++)
        {
            df::creature_raw *p = raws.creatures.all[i];
            string basename = p->name[0];

            for (size_t j = 0; j < p->material.size(); j++)
            {
                MaterialInfo material;
                material.decode(DFHack::MaterialInfo::CREATURE_BASE+j, i);
                //addMaterialEntry(selected_category, material, basename+" (" + p->material[j]->id + "): " + material.toString(), set_defaults);
                addMaterialEntry(selected_category, material, material.toString(), set_defaults);
            }
        }

        materials_column.sort();
    }


    bool viewscreenChooseMaterial::addMaterialEntry(df::dfhack_material_category &selected_category, MaterialInfo &material, 
                                                           string name, const bool set_defaults)
    {
        bool selected = false;
        if (!selected_category.whole || material.matches(selected_category))
        {
            ListEntry<MaterialInfo> entry(pad_string(name, MAX_MATERIAL, false), material);
            if (set_defaults)
            {
                if (cv->material.matches(material))
                {
                    entry.selected = true;
                    selected = true;
                    /*if (materials_column.highlighted_index == 0)
                        materials_column.highlighted_index = materials_column.getBaseListSize();*/
                }
            }
            materials_column.add(entry);
        }

        return selected;
    }

    void viewscreenChooseMaterial::feed(set<df::interface_key> *input)
    {
        bool key_processed;
        switch (selected_column)
        {
        case 0:
            key_processed = items_column.feed(input);
            break;
        case 1:
            key_processed = masks_column.feed(input);
            if (input->count(interface_key::SELECT))
                populateMaterials(false);
            break;
        case 2:
            key_processed = materials_column.feed(input);
            break;
        }

        if (key_processed)
            return;

        if (input->count(interface_key::LEAVESCREEN))
        {
            input->clear();
            Screen::dismiss(this);
            return;
        }
        else if  (input->count(interface_key::SEC_SELECT))
        {
            df::dfhack_material_category mat_mask;
            vector<df::dfhack_material_category *> selected_masks = masks_column.getSelectedElems();
            for (vector<df::dfhack_material_category *>::iterator it = selected_masks.begin(); it != selected_masks.end(); it++)
            {
                mat_mask.whole |= (*it)->whole;
            
            }

            MaterialInfo *selected_material = materials_column.getFirstSelectedElem();
            MaterialInfo material = (selected_material) ? *selected_material : MaterialInfo();

            if (cv == NULL)
            {
                ItemConstraint *nct = create_new_constraint(false, *items_column.getFirstSelectedElem(), material, mat_mask, item_quality::Ordinary, NULL, "");
                nct->setGoalByCount(false);
                nct->setGoalCount(10);
                nct->setGoalGap(1);
            }
            else
            {
                cv->mat_mask = mat_mask;
                cv->material = (selected_material) ? *selected_material : MaterialInfo();
            }

            reset_list = true;
            Screen::dismiss(this);
        }
        else if  (input->count(interface_key::CURSOR_LEFT))
        {
            --selected_column;
        }
        else if  (input->count(interface_key::CURSOR_RIGHT))
        {
            selected_column++;
        }
        else if (enabler->tracking_on && enabler->mouse_lbut)
        {
            if (items_column.setHighlightByMouse())
                selected_column = 0;
            else if (masks_column.setHighlightByMouse())
                selected_column = 1;
            else if (materials_column.setHighlightByMouse())
                selected_column = 2;

            enabler->mouse_lbut = enabler->mouse_rbut = 0;
        }
    }

    void viewscreenChooseMaterial::render()
    {
        if (Screen::isDismissed(this))
            return;

        dfhack_viewscreen::render();

        Screen::clear();
        Screen::drawBorder("  Workflow Material  ");

        items_column.display(selected_column == 0);
        masks_column.display(selected_column == 1);
        materials_column.display(selected_column == 2);

        int32_t y = gps->dimy - 3;
        int32_t x = 2;
        OutputHotkeyString(x, y, "Save", "Shift-Enter");
        x += 3;
        OutputHotkeyString(x, y, "Cancel", "Esc");
    }

    void viewscreenChooseMaterial::validateColumn()
    {
        set_to_limit(selected_column, 2);
    }

    void viewscreenChooseMaterial::resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        items_column.resize();
        masks_column.resize();
        materials_column.resize();
    }

    
    /*
     * Inventory Monitor
     */
    class viewscreenInventoryMonitor : public dfhack_viewscreen, public AdjustmentScreen
    {
    private:
        struct TableRow
        {
            vector< pair<int32_t, int32_t> > history_plot;
            int32_t limit_y, gap_y;
            ItemConstraint *cv;
            UIColor value_color;
            UIColor limit_color;
            int severity;
        };

        class MonitorListColumn : public ListColumn<TableRow>
        {
        public:
            bool order_by_severity;

            virtual void sort()
            {
                if (!order_by_severity)
                    ListColumn::sort();
                else
                    doSort(&compareSeverity);
            }

        private:
            void display_extras(const TableRow &row, int32_t &x, int32_t &y) const
            {
                string text;
                text.append(int_to_string((row.cv->goalByCount()) ? row.cv->item_count : row.cv->item_amount));
                text = pad_string(text, 6);
                OutputString(row.value_color, x, y, text);

                text = (row.cv->goalByCount()) ? " S " : " I ";
                OutputString(COLOR_GREY, x, y, text);

                text = int_to_string(row.cv->goalCount());
                OutputString(row.limit_color, x, y, text);
            }

            static bool compareSeverity(ListEntry<TableRow> const& a, ListEntry<TableRow> const& b)
            {
                return a.elem.severity > b.elem.severity;
            }
        };

    public:
        viewscreenInventoryMonitor();

        void feed(set<df::interface_key> *input);

        void render();

        std::string getFocusString() { return "invmonitor"; }

        virtual void resize(int32_t x, int32_t y) 
        {
            dfhack_viewscreen::resize(x, y);
            init();
        }


    private:
        MonitorListColumn rows;
        int32_t bottom_controls_row;
        int32_t divider_x;
        int chart_width, chart_height;
        int32_t axis_y_end, axis_y_start, axis_x_start, axis_x_end;

        void init();

        virtual void onConstraintChanged();

        virtual void onModeChanged();


    };

    viewscreenInventoryMonitor::viewscreenInventoryMonitor()
    {
        adjustment_ui_display_start = 2;
        chart_width = SIDEBAR_WIDTH - 2;
        rows.order_by_severity = false;

        rows.multiselect = false;
        rows.allow_null = false;
        rows.auto_select = true;
        rows.title = pad_string("Item", MAX_ITEM_NAME, false);
        rows.title += " ";
        rows.title += pad_string("Material", MAX_MATERIAL, false);
        
        init();
    }
    
    void viewscreenInventoryMonitor::init()
    {
        bottom_controls_row = gps->dimy - 3;

        divider_x = gps->dimx - SIDEBAR_WIDTH - 2;
        chart_height = min(SIDEBAR_WIDTH, gps->dimy - 20);
        axis_y_end = gps->dimy - 3;
        axis_y_start = axis_y_end - chart_height;
        axis_x_start = divider_x + 2;
        axis_x_end = axis_x_start + chart_width - 1;

        rows.clear();
        for (vector<ItemConstraint *>::iterator it = constraints.begin(); it < constraints.end(); it++)
        {
            TableRow row;
            row.cv = *it;

            if (row.cv->jobs.size() > 0)
            {
                ProtectedJob *pj = row.cv->jobs[0];
                if (!pj->isActuallyResumed() && pj->want_resumed)
                    row.limit_color = COLOR_YELLOW;
                else
                    row.limit_color = COLOR_GREY;
            }
            else
                row.limit_color = COLOR_RED;


            int curr_value = (row.cv->goalByCount()) ? row.cv->item_count : row.cv->item_amount;
            row.value_color = COLOR_GREY;
            if (curr_value >= row.cv->goalCount() - row.cv->goalGap())
                row.value_color = COLOR_LIGHTGREEN;
            else if (curr_value >= (row.cv->goalCount() - row.cv->goalGap() * 2))
                row.value_color = COLOR_GREEN;
            else if (curr_value <= row.cv->goalGap())
                row.value_color = COLOR_LIGHTRED;
            else if (curr_value <= (row.cv->goalGap() * 2))
                row.value_color = COLOR_RED;

            int max_val = row.cv->goalCount();
            float scale_y;
            if (max_history_days > 0 && row.cv->history.size() > 0)
            {
                row.history_plot.clear();
                max_val = max(max_val, *max_element(row.cv->history.begin(), row.cv->history.end()));
                scale_y = (float) chart_height / (float) max_val;
                float scale_x = (float) chart_width / (float) (max_history_days * 2);

                int sumX, sumY, sumXY, sumXX;
                sumX = sumY = sumXY = sumXX = 0;

                for (size_t i = 0; i < row.cv->history.size(); i++)
                {
                    pair<int32_t, int32_t> point(axis_x_start + (int) (scale_x * (float) i), 
                                                 axis_y_end - (int) (scale_y * (float) row.cv->history[i]));

                    row.history_plot.push_back(point);

                    if (row.value_color == COLOR_GREY)
                    {
                        sumX += i;
                        sumY += row.cv->history[i];
                        sumXY += i * row.cv->history[i];
                        sumXX += i*i;
                    }
                }

                if (row.value_color == COLOR_GREY)
                {
                    int count = row.cv->history.size();
                    float slope = (float) (count * sumXY - sumX * sumY) / 
                        (float) (count * sumXX - sumX * sumX);

                    if (slope > 0.3f)
                        row.value_color = COLOR_GREEN;
                    else if (slope < -0.3f)
                        row.value_color = COLOR_RED;
                }

            }
            else
            {
                scale_y = (float) chart_height / (float) max_val;
            }

            row.limit_y = axis_y_end - (int) (scale_y * (float) row.cv->goalCount());
            row.gap_y = axis_y_end - (int) (scale_y * (float) (row.cv->goalCount() - row.cv->goalGap()));

            string text = pad_string(row.cv->item.toString(), MAX_ITEM_NAME, false);
            text += " ";
            text += pad_string(get_constraint_material(row.cv), MAX_MATERIAL, false, true);

            switch (row.value_color)
            {
            case COLOR_LIGHTGREEN:
                row.severity = 0;
                break;
            case COLOR_GREEN:
                row.severity = 1;
                break;
            case COLOR_GREY:
                row.severity = 2;
                break;
            case COLOR_RED:
                row.severity = 3;
                break;
            case COLOR_LIGHTRED:
                row.severity = 4;
                break;
            default:
                break;
            }

            rows.add(text, row);
        }
        rows.sort();
        rows.changeHighlight(0);
    }
    
    void viewscreenInventoryMonitor::onConstraintChanged()
    {
        init();
    }
    
    void viewscreenInventoryMonitor::onModeChanged()
    {
        TableRow *row = rows.getFirstSelectedElem();
        if (!row)
            return;
        row->cv->history.clear();
        init();
    }
        
    void viewscreenInventoryMonitor::render()
    {
        if (Screen::isDismissed(this))
            return;

        if (viewscreenChooseMaterial::reset_list)
        {
            viewscreenChooseMaterial::reset_list = false;
            init();
            return;
        }

        dfhack_viewscreen::render();

        Screen::clear();
        Screen::drawBorder("  Inventory Monitor  ");

        Screen::Pen border('\xDB', 8);
        for (int32_t y = 1; y < gps->dimy - 1; y++)
        {
            paintTile(border, divider_x, y);
        }

        int32_t x = MAX_ITEM_NAME + MAX_MATERIAL + 3;
        int32_t y = 2;
        paint_text(COLOR_TITLE, x, y, "Stock / Limit");

        rows.display(true);

        y = bottom_controls_row;
        x = 2;
        OutputHotkeyString(x, y, "Add", "a");
        x += 2;
        OutputHotkeyString(x, y, "Edit", "e");
        
        x += 2;
        OutputHotkeyString(x, y, (rows.order_by_severity) ? "Name Order" : "Severity Order", "o");

        if (rows.getDisplayListSize() > 0)
        {
            TableRow *row = rows.getFirstSelectedElem();
            AdjustmentScreen::render(row->cv);

            if (max_history_days > 0)
            {
                paint_text(COLOR_BROWN, axis_x_start, axis_y_start-2, "      Stock History Chart");
                Screen::Pen y_axis('\xB3', COLOR_BROWN);
                x = axis_x_start;
                for (y = axis_y_start; y <= axis_y_end; y++)
                {
                    paintTile(y_axis, x, y);
                }

                Screen::Pen up_arrow('\xCF', COLOR_BROWN);
                paintTile(up_arrow, axis_x_start, axis_y_start-1);

                Screen::Pen x_axis('\xC4', COLOR_BROWN);
                y = axis_y_end;
                for (x = axis_x_start; x <= axis_x_end; x++)
                {
                    paintTile(x_axis, x, y);
                    paint_text(COLOR_LIGHTGREEN, x, row->limit_y, "-");
                    paint_text(COLOR_GREEN, x, row->gap_y, "-");
                }

                Screen::Pen right_arrow('\xAF', COLOR_BROWN);
                paintTile(right_arrow, axis_x_end+1, axis_y_end);

                Screen::Pen zero_axis('\x9E', COLOR_BROWN);
                paintTile(zero_axis, axis_x_start, axis_y_end);

                for (size_t i = 0; i < row->history_plot.size(); i++)
	            {
                    int x = row->history_plot[i].first;
                    int y = row->history_plot[i].second;
                    paint_text(COLOR_CYAN, x, y, "*");
	            }
            }
        }


    }
    
    void viewscreenInventoryMonitor::feed(set<df::interface_key> *input)
    {
        if (!rows.search_entry_mode && rows.getDisplayListSize() > 0 && AdjustmentScreen::feed(input, rows.getFirstSelectedElem()->cv))
            return;

        if (rows.feed(input))
            return;

        if (input->count(interface_key::LEAVESCREEN))
        {
            input->clear();
            Screen::dismiss(this);
            return;
        }
        else if  (input->count(interface_key::CUSTOM_O))
        {
            rows.order_by_severity = !rows.order_by_severity;
            rows.sort();
        }
        else if  ((input->count(interface_key::CUSTOM_E) || input->count(interface_key::SELECT)) && rows.getDisplayListSize() > 0)
        {
            TableRow *row = rows.getFirstSelectedElem();
            if (row)
            {
                Screen::show(new viewscreenChooseMaterial(row->cv));
            }
        }
        else if  (input->count(interface_key::CUSTOM_A))
        {
            Screen::show(new viewscreenChooseMaterial());
        }
        else if (enabler->tracking_on)
        {
            enabler->mouse_lbut = enabler->mouse_rbut = 0;
        }
    }

    /******************************
    *   Hook for workshop view   *
    ******************************/
    struct wf_workshop_hook : public df::viewscreen_dwarfmodest
    {
        typedef df::viewscreen_dwarfmodest interpose_base;

        static color_ostream_proxy console_out;
        static df::job *last_job;
        static df::job *job;
        static AdjustmentScreen dialog;
        
        bool checkJobSelection()
        {
            if (!enabled)
                return false;

            if (!first_update_done)
            {
                plugin_onupdate(console_out);
            }

            job = Gui::getSelectedWorkshopJob(console_out, true);
            if (job != last_job)
            {
                dialog.reset();
                last_job = job;
            }


            return job != NULL;
        }

        bool can_enable_plugin()
        {
            return (!enabled && 
                Gui::dwarfmode_hotkey(Core::getTopViewscreen()) &&
                ui->main.mode == ui_sidebar_mode::QueryBuilding &&
                Gui::getSelectedWorkshopJob(console_out, true));
        }

        bool handleInput(set<df::interface_key> *input)
        {
            bool key_processed = true;
            if (checkJobSelection())
            {
                ProtectedJob *pj = get_known(job->id);
                if (!pj)
                    return false;

                ItemConstraint *cv = NULL;
                if (pj->constraints.size() > 0)
                    cv = pj->constraints[0];

                if (!dialog.feed(input, cv, pj))
                {
                    if (input->count(interface_key::CUSTOM_T))
                    {
                        if (!cv)
                        {
                            // Add tracking
                            compute_job_outputs(console_out, pj, true);
                            if (pj->constraints.size() > 0)
                            {
                                cv = pj->constraints[0];
                                cv->setGoalByCount(false);
                                cv->setGoalCount(10);
                                cv->setGoalGap(1);
                                job->flags.bits.repeat = true;
                            }
                            else
                            {
                                Gui::showAnnouncement("Job type not currently supported", 6, true);
                            }
                        }
                    }
                    else if (input->count(interface_key::CUSTOM_M))
                    {
                        Screen::show(new viewscreenInventoryMonitor());
                    }
                    else
                        key_processed = false;
                }
            }
            else if (can_enable_plugin() && input->count(interface_key::CUSTOM_W))
            {
                enable_plugin(console_out);
                plugin_onupdate(console_out);
            }
            else
                key_processed = false;

            return key_processed;
        }

        DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
        {
            if (!handleInput(input))
                INTERPOSE_NEXT(feed)(input);
            else
                input->clear();
        }

        DEFINE_VMETHOD_INTERPOSE(void, render, ())
        {
            INTERPOSE_NEXT(render)();
            if (checkJobSelection())
            {
                ProtectedJob *pj = get_known(job->id);
                if (!pj)
                {
                    pj = add_known_job(job);
                    compute_job_outputs(console_out, pj);
                }

                ItemConstraint *cv = NULL;
                if (pj->constraints.size() > 0)
                {
                    cv = pj->constraints[0];
                }
                dialog.render(cv);
                if (cv)
                    ++dialog.y;
                OutputHotkeyString(dialog.left_margin, dialog.y, "Inventory Monitor", "m");
            }
            else if (can_enable_plugin())
            {
                int x = gps->dimx - 30;
                int y = 24;
                OutputHotkeyString(x, y, "Enable Workflow", "w");
            }
        }
    };

    color_ostream_proxy wf_workshop_hook::console_out(Core::getInstance().getConsole());
    df::job *wf_workshop_hook::job = NULL;
    df::job *wf_workshop_hook::last_job = NULL;
    AdjustmentScreen wf_workshop_hook::dialog;


    IMPLEMENT_VMETHOD_INTERPOSE(wf_workshop_hook, feed);
    IMPLEMENT_VMETHOD_INTERPOSE(wf_workshop_hook, render);
}

#undef INV_MONITOR_COL_COUNT
#undef MAX_ITEM_NAME


static command_result workflow_cmd(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (enabled) {
        check_lost_jobs(out, 0);
        recover_jobs(out);
        update_job_data(out);
        map_job_constraints(out);
        map_job_items(out);
    }

    df::building *workshop = NULL;
    //FIXME: unused variable!
    //df::job *job = NULL;

    if (Gui::dwarfmode_hotkey(Core::getTopViewscreen()) &&
        ui->main.mode == ui_sidebar_mode::QueryBuilding)
    {
        workshop = world->selected_building;
        //job = Gui::getSelectedWorkshopJob(out, true);
    }

    std::string cmd = parameters.empty() ? "list" : parameters[0];

    if (cmd == "enable" || cmd == "disable")
    {
        bool enable = (cmd == "enable");
        if (enable && !enabled)
        {
            enable_plugin(out);
        }
        else if (!enable && parameters.size() == 1)
        {
            if (enabled)
            {
                enabled = false;
                setOptionEnabled(CF_ENABLED, false);
                stop_protect(out);
            }

            out << "The plugin is disabled." << endl;
            return CR_OK;
        }

        for (size_t i = 1; i < parameters.size(); i++)
        {
            if (parameters[i] == "drybuckets")
                setOptionEnabled(CF_DRYBUCKETS, enable);
            else if (parameters[i] == "auto-melt")
                setOptionEnabled(CF_AUTOMELT, enable);
            else
                return CR_WRONG_USAGE;
        }

        if (enabled)
            out << "The plugin is enabled." << endl;
        else
            out << "The plugin is disabled." << endl;

        if (isOptionEnabled(CF_DRYBUCKETS))
            out << "Option drybuckets is enabled." << endl;
        if (isOptionEnabled(CF_AUTOMELT))
            out << "Option auto-melt is enabled." << endl;

        return CR_OK;
    }
    else if (cmd == "count" || cmd == "amount")
    {
        if (!enabled)
            enable_plugin(out);
    }

    if (!enabled)
        out << "Note: the plugin is not enabled." << endl;

    if (cmd == "jobs")
    {
        if (workshop)
        {
            for (size_t i = 0; i < workshop->jobs.size(); i++)
                print_job(out, get_known(workshop->jobs[i]->id));
        }
        else
        {
            for (TKnownJobs::iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
                if (it->second->isLive())
                    print_job(out, it->second);
        }

        bool pending = false;

        for (size_t i = 0; i < pending_recover.size(); i++)
        {
            if (!workshop || pending_recover[i]->holder == workshop)
            {
                if (!pending)
                {
                    out.print("\nPending recovery:\n");
                    pending = true;
                }

                Job::printJobDetails(out, pending_recover[i]->job_copy);
            }
        }

        return CR_OK;
    }
    else if (cmd == "list")
    {
        for (size_t i = 0; i < constraints.size(); i++)
            print_constraint(out, constraints[i]);

        return CR_OK;
    }
    else if (cmd == "list-commands")
    {
        for (size_t i = 0; i < constraints.size(); i++)
        {
            auto cv = constraints[i];
            out << "workflow " << (cv->goalByCount() ? "count " : "amount ")
                << cv->config.val() << " " << cv->goalCount() << " " << cv->goalGap() << endl;
        }

        return CR_OK;
    }
    else if (cmd == "count" || cmd == "amount")
    {
        if (parameters.size() < 3)
            return CR_WRONG_USAGE;

        int limit = atoi(parameters[2].c_str());
        if (limit <= 0) {
            out.printerr("Invalid limit value.\n");
            return CR_FAILURE;
        }

        ItemConstraint *icv = get_constraint(out, parameters[1]);
        if (!icv)
            return CR_FAILURE;

        icv->setGoalByCount(cmd == "count");
        icv->setGoalCount(limit);
        if (parameters.size() >= 4)
            icv->setGoalGap(atoi(parameters[3].c_str()));
        else
            icv->setGoalGap(-1);

        process_constraints(out);
        print_constraint(out, icv);
        return CR_OK;
    }
    else if (cmd == "unlimit")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;

        for (size_t i = 0; i < constraints.size(); i++)
        {
            if (constraints[i]->config.val() != parameters[1])
                continue;

            delete_constraint(constraints[i]);
            //process_constraints(out);
            return CR_OK;
        }

        out.printerr("Constraint not found: %s\n", parameters[1].c_str());
        return CR_FAILURE;
    }
    else if (cmd == "unlimit-all")
    {
        if (parameters.size() != 1)
            return CR_WRONG_USAGE;

        while (!constraints.empty())
            delete_constraint(constraints[0]);

        //process_constraints(out);

        out.print("Removed all constraints.\n");
        return CR_OK;
    }
    else if (cmd == "monitor")
    {
        if (!enabled)
        {
            enable_plugin(out);
            plugin_onupdate(out);
        }

        Screen::show(new wf_ui::viewscreenInventoryMonitor());
        return CR_OK;
    }
    else
        return CR_WRONG_USAGE;
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!world || !ui)
        return CR_FAILURE;

    if (!gps || !INTERPOSE_HOOK(wf_ui::wf_workshop_hook, feed).apply() || !INTERPOSE_HOOK(wf_ui::wf_workshop_hook, render).apply())
        out.printerr("Could not insert Workflow hooks!\n");

    if (ui_workshop_job_cursor && job_next_id) {
        commands.push_back(
            PluginCommand(
            "workflow", "Manage control of repeat jobs.",
            workflow_cmd, false,
            "  workflow enable [option...]\n"
            "  workflow disable [option...]\n"
            "    If no options are specified, enables or disables the plugin.\n"
            "    Otherwise, enables or disables any of the following options:\n"
            "     - drybuckets: Automatically empty abandoned water buckets.\n"
            "     - auto-melt: Resume melt jobs when there are objects to melt.\n"
            "  workflow jobs\n"
            "    List workflow-controlled jobs (if in a workshop, filtered by it).\n"
            "  workflow list\n"
            "    List active constraints, and their job counts.\n"
            "  workflow list-commands\n"
            "    List workflow commands that re-create existing constraints.\n"
            "  workflow count <constraint-spec> <cnt-limit> [cnt-gap]\n"
            "  workflow amount <constraint-spec> <cnt-limit> [cnt-gap]\n"
            "    Set a constraint. The first form counts each stack as only 1 item.\n"
            "  workflow unlimit <constraint-spec>\n"
            "    Delete a constraint.\n"
            "  workflow unlimit-all\n"
            "    Delete all constraints.\n"
            "Function:\n"
            "  - When the plugin is enabled, it protects all repeat jobs from removal.\n"
            "    If they do disappear due to any cause, they are immediately re-added\n"
            "    to their workshop and suspended.\n"
            "  - In addition, when any constraints on item amounts are set, repeat jobs\n"
            "    that produce that kind of item are automatically suspended and resumed\n"
            "    as the item amount goes above or below the limit. The gap specifies how\n"
            "    much below the limit the amount has to drop before jobs are resumed;\n"
            "    this is intended to reduce the frequency of jobs being toggled.\n"
            "Constraint examples:\n"
            "  workflow amount AMMO:ITEM_AMMO_BOLTS/METAL 1000 100\n"
            "  workflow amount AMMO:ITEM_AMMO_BOLTS/WOOD,BONE 200 50\n"
            "    Keep metal bolts within 900-1000, and wood/bone within 150-200.\n"
            "  workflow count FOOD 120 30\n"
            "  workflow count DRINK 120 30\n"
            "    Keep the number of prepared food & drink stacks between 90 and 120\n"
            "  workflow count BIN 30\n"
            "  workflow count BARREL 30\n"
            "  workflow count BOX/CLOTH,SILK,YARN 30\n"
            "    Make sure there are always 25-30 empty bins/barrels/bags.\n"
            "  workflow count BAR//COAL 20\n"
            "  workflow count BAR//COPPER 30\n"
            "    Make sure there are always 15-20 coal and 25-30 copper bars.\n"
            "  workflow count CRAFTS//GOLD 20\n"
            "    Produce 15-20 gold crafts.\n"
            "  workflow count POWDER_MISC/SAND 20\n"
            "  workflow count BOULDER/CLAY 20\n"
            "    Collect 15-20 sand bags and clay boulders.\n"
            "  workflow amount POWDER_MISC//MUSHROOM_CUP_DIMPLE:MILL 100 20\n"
            "    Make sure there are always 80-100 units of dimple dye.\n"
            "    In order for this to work, you have to set the material of\n"
            "    the PLANT input on the Mill Plants job to MUSHROOM_CUP_DIMPLE\n"
            "    using the 'job item-material' command.\n"
            )
            );
    }

    init_state(out);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    INTERPOSE_HOOK(wf_ui::wf_workshop_hook, feed).remove();
    INTERPOSE_HOOK(wf_ui::wf_workshop_hook, render).remove();

    cleanup_state(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        cleanup_state(out);
        init_state(out);
        break;
    case SC_MAP_UNLOADED:
        cleanup_state(out);
        break;
    default:
        break;
    }

    return CR_OK;
}

