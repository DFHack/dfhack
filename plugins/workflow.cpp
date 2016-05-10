#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "DataFuncs.h"
#include "Export.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Materials.h"
#include "modules/Persistent.h"
#include "modules/World.h"

#include "df/building_furnacest.h"
#include "df/building_workshopst.h"
#include "df/builtin_mats.h"
#include "df/dfhack_material_category.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/general_ref_contains_itemst.h"
#include "df/general_ref_contains_unitst.h"
#include "df/general_ref_unit_holderst.h"
#include "df/general_ref_unit_workerst.h"
#include "df/item.h"
#include "df/itemdef_foodst.h"
#include "df/item_quality.h"
#include "df/items_other_id.h"
#include "df/inorganic_raw.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/job_list_link.h"
#include "df/plant_raw.h"
#include "df/reaction.h"
#include "df/reaction_product_itemst.h"
#include "df/reaction_reagent_itemst.h"
#include "df/tool_uses.h"
#include "df/ui.h"
#include "df/vehicle.h"
#include "df/world.h"

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using std::vector;
using std::string;
using std::endl;
using std::cerr;
using std::flush;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("workflow");

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_workshop_job_cursor);
REQUIRE_GLOBAL(job_next_id);

static const int32_t persist_version=1;
static void save_config(color_ostream& out);
static void load_config(color_ostream& out);
static void on_presave_callback(color_ostream& out, void* nothing) {
    save_config(out);
}

/* Plugin registration */

static command_result workflow_cmd(color_ostream &out, vector <string> & parameters);
static command_result fix_job_postings_cmd(color_ostream &out, vector<string> &parameters);

static void init_state(color_ostream &out);
static void cleanup_state(color_ostream &out);

static int fix_job_postings(color_ostream *out = NULL, bool dry_run = false);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!world || !ui)
        return CR_FAILURE;

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
                "Constraint format:\n"
                "  The contstraint spec consists of 4 parts, separated with '/' characters:\n"
                "    ITEM[:SUBTYPE]/[GENERIC_MAT,...]/[SPECIFIC_MAT:...]/[LOCAL,<quality>]\n"
                "  The first part is mandatory and specifies the item type and subtype,\n"
                "  using the raw tokens for items, in the same syntax you would e.g. use\n"
                "  for a custom reaction input. The subsequent parts are optional:\n"
                "  - A generic material spec constrains the item material to one of\n"
                "    the hard-coded generic classes, like WOOD, METAL, YARN or MILK.\n"
                "  - A specific material spec chooses the material exactly, using the\n"
                "    raw syntax for reaction input materials, e.g. INORGANIC:IRON,\n"
                "    although for convenience it also allows just IRON, or ACACIA:WOOD.\n"
                "  - A comma-separated list of miscellaneous flags, which currently can\n"
                "    be used to ignore imported items or items below a certain quality.\n"
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
                "  workflow count CRAFTS///LOCAL,EXCEPTIONAL 100 90\n"
                "    Maintain 10-100 locally-made crafts of exceptional quality.\n"
            )
        );
        commands.push_back(PluginCommand(
            "fix-job-postings",
            "Fix broken job postings caused by certain versions of workflow",
            fix_job_postings_cmd, false,
            "fix-job-postings: Fix job postings\n"
            "fix-job-postings dry|[any argument]: Dry run only (avoid making changes)\n"
        ));
    }

    EventManager::EventHandler handler(on_presave_callback, 1);
    EventManager::registerListener(EventManager::EventType::PRESAVE, handler, plugin_self);

    if ( DFHack::Core::getInstance().isWorldLoaded() ) {
        load_config(out);
    }

    init_state(out);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    if ( DFHack::Core::getInstance().isWorldLoaded() ) {
        save_config(out);
    }
    cleanup_state(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        cleanup_state(out);
        load_config(out);
        init_state(out);
        break;
    case SC_MAP_UNLOADED:
        break;
    default:
        break;
    }

    return CR_OK;
}

command_result fix_job_postings_cmd(color_ostream &out, vector<string> &parameters)
{
    bool dry = parameters.size();
    int fixed = fix_job_postings(&out, dry);
    out << fixed << " job issue(s) " << (dry ? "detected but not fixed" : "fixed") << endl;
    return CR_OK;
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
                set_resumed(true);
        }
    }

    void recover(df::job *job)
    {
        actual_job = job;
        job->flags.bits.repeat = true;
        set_resumed(false);

        resume_delay = std::min(DAY_TICKS*MONTH_DAYS, 5*resume_delay/3);
        resume_time = world->frame_counter + resume_delay;
    }

    void set_resumed(bool resume)
    {
        if (resume)
        {
            if (world->frame_counter >= resume_time && actual_job->flags.bits.suspend)
            {
                Job::removePostings(actual_job, true);
                actual_job->flags.bits.suspend = false;
            }
        }
        else
        {
            resume_time = 0;
            if (isActuallyResumed())
                resume_delay = DAY_TICKS;

            if (!actual_job->flags.bits.suspend)
            {
                actual_job->flags.bits.suspend = true;
                Job::removePostings(actual_job, true);
            }
        }

        want_resumed = resume;
    }
};

int ProtectedJob::cur_tick_idx = 0;

typedef std::map<std::pair<int,int>, bool> TMaterialCache;

static const size_t MAX_HISTORY_SIZE = 90;

enum HistoryItem {
    HIST_COUNT = 0,
    HIST_AMOUNT,
    HIST_INUSE_COUNT,
    HIST_INUSE_AMOUNT
};
struct Snapshot {
    int32_t count;
    int32_t amount;
    int32_t inuse_count;
    int32_t inuse_amount;
};

struct ItemConstraint {
    int goalCount_;
    int goalGap_;
    bool goalByCount_;

    // Fixed key parsed into fields
    bool is_craft;
    ItemTypeInfo item;

    MaterialInfo material;
    df::dfhack_material_category mat_mask;

    item_quality::item_quality min_quality;
    bool is_local;

    // Tracking data
    int weight;
    std::vector<ProtectedJob*> jobs;

    int item_amount, item_count, item_inuse_amount, item_inuse_count;
    bool request_suspend, request_resume;

    bool is_active, cant_resume_reported;
    int low_stock_reported;

    TMaterialCache material_cache;
    std::string val;

    std::vector<Snapshot> history;
    size_t oldest_snapshot;
public:
    ItemConstraint()
        : goalCount_(0), goalGap_(-1), goalByCount_(0),
          is_craft(false), min_quality(item_quality::Ordinary), is_local(false),
          weight(0), item_amount(0), item_count(0), item_inuse_amount(0), item_inuse_count(0),
          is_active(false), cant_resume_reported(false), low_stock_reported(-1)
    {}

    int goalCount() { return goalCount_; } //return config.ival(0); }
    void setGoalCount(int v) { goalCount_ = v; } //config.ival(0) = v; }

    int goalGap() {
        int cval = goalGap_ <= 0 ? std::min(5,goalCount()/2) : goalGap_;
        return std::max(1, std::min(goalCount()-1,cval));
        //int cval = (config.ival(1) <= 0) ? std::min(5,goalCount()/2) : config.ival(1);
        //return std::max(1, std::min(goalCount()-1, cval));
    }
    void setGoalGap(int v) { goalGap_ = v; } //config.ival(1) = v; }

    bool goalByCount() { return goalByCount_; } //return config.ival(2) & 1; }
    void setGoalByCount(bool v) {
        goalByCount_ = v;
        //if (v)
        //    goalByCount_ |= 1;
        //else
        //    goalByCount_ &= ~1;
        /*if (v)
            config.ival(2) |= 1;
        else
            config.ival(2) &= ~1;*/
    }

    int curItemStock() { return goalByCount() ? item_count : item_amount; }

    void init(const std::string &str)
    {
        //config.val() = str;
        //config.ival(0) = 10;
        //config.ival(2) = 0;
        val = str;
        goalCount_ = 10;
        goalByCount_ = 0;
    }

    void computeRequest()
    {
        int size = curItemStock();
        request_resume = (size <= goalCount()-goalGap());
        request_suspend = (size >= goalCount());
    }

    void updateHistory() {
        Snapshot s;
        s.count = item_count;
        s.amount = item_amount;
        s.inuse_count = item_inuse_count;
        s.inuse_amount = item_inuse_amount;
        size_t buffer_size = history.size();
        if ( buffer_size >= MAX_HISTORY_SIZE ) {
            history[oldest_snapshot] = s;
            oldest_snapshot++;
            if ( oldest_snapshot >= history.size() )
                oldest_snapshot -= history.size();
            return;
        }
        history.push_back(s);
    }
};

static int fix_job_postings (color_ostream *out, bool dry_run)
{
    int count = 0;
    df::job_list_link *link = &world->job_list;
    while (link)
    {
        df::job *job = link->item;
        if (job)
        {
            for (size_t i = 0; i < world->job_postings.size(); ++i)
            {
                df::world::T_job_postings *posting = world->job_postings[i];
                if (posting->job == job && i != job->posting_index && !posting->flags.bits.dead)
                {
                    ++count;
                    if (out)
                        *out << "Found extra job posting: Job " << job->id << ": "
                            << Job::getName(job) << endl;
                    if (!dry_run)
                        posting->flags.bits.dead = true;
                }
            }
        }
        link = link->next;
    }
    return count;
}

/******************************
 *      GLOBAL VARIABLES      *
 ******************************/

DFHACK_PLUGIN_IS_ENABLED(enabled);

static int32_t config_flags;

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
            job->job_type == job_type::CollectSand ||
            job->job_type == job_type::MilkCreature ||
            job->job_type == job_type::ShearCreature);
}

static bool isOptionEnabled(unsigned flag)
{
    return (config_flags & flag) != 0;
}

static void setOptionEnabled(unsigned flag, bool on)
{
    if (on)
        config_flags |= flag;
    else
        config_flags &= ~flag;
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
    config_flags = 0;//PersistentDataItem();

    stop_protect(out);

    for (size_t i = 0; i < constraints.size(); i++)
        delete constraints[i];
    constraints.clear();
}

static void check_lost_jobs(color_ostream &out, int ticks);
static ItemConstraint *get_constraint(color_ostream &out, const std::string &str, bool create = true);
static void print_constraint(color_ostream &out, ItemConstraint *cv, bool no_job = false, std::string prefix = "");
static vector<ItemConstraint*>::iterator maybeAddConstraint(ItemConstraint* cv);

static void load_config(color_ostream& out) {
    for ( uint32_t i = 0; i < constraints.size(); ++i ) {
        delete constraints[i];
    }
    constraints.clear();
    Json::Value& p = Persistent::get("workflow");
    int32_t version = p["version"].isInt() ? p["version"].asInt() : 0;
    if ( version == 0 ) {
        PersistentDataItem conf = World::GetPersistentData("workflow/config");
        if ( conf.isValid() && conf.ival(0) == -1 )
            conf.ival(0) = 0;
        config_flags = conf.ival(0);
        if ( conf.isValid() )
            World::DeletePersistentData(conf);

        std::vector<PersistentDataItem> items;
        World::GetPersistentData(&items,"workflow/constraints");
        //no idea why this is in reverse order
        for ( int32_t i = items.size()-1; i >= 0; --i ) {
            ItemConstraint* constr = get_constraint(out,items[i].val());
            if ( !constr )
                out.printerr("Lost constraint %s\n", items[i].val().c_str());
            World::DeletePersistentData(items[i]);
        }
    } else if ( version == 1 ) {
        //out << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": " << "constraints.size() = " << constraints.size() << endl;
        std::map<std::string,df::item_quality> qualityMap;
        FOR_ENUM_ITEMS(item_quality,qual) {
            qualityMap[ENUM_KEY_STR(item_quality,qual)] = qual;
        }
        config_flags = p["flags"].asInt();
        Json::Value& constraintsTable = p["constraints"];
        for ( int32_t i = 0; i < constraintsTable.size(); i++ ) {
            Json::Value& c = constraintsTable[i];
            ItemConstraint* constr = new ItemConstraint;
            constr->goalCount_ = c["goalCount"].asInt();
            constr->goalGap_ = c["goalGap"].asInt();
            constr->goalByCount_ = c["goalByCount"].asBool();
            constr->is_craft = c["is_craft"].asBool();
            constr->item.find(c["item"].asString());
            constr->material.find(c["material"].asString());
            constr->mat_mask.whole = c["mat_mask"].asInt();
            constr->min_quality = qualityMap[c["min_quality"].asString()];
            constr->is_local = c["is_local"].asBool();
            constr->weight = c["weight"].asInt();
            Json::Value& v = c["history"];
            for ( int32_t j = 0; j < v.size(); j++ ) {
                Json::Value& snap = v[j];
                constr->history.push_back(Snapshot());
                Snapshot& s = constr->history[constr->history.size()-1];
                s.count = snap["count"].asInt();
                s.amount = snap["amount"].asInt();
                s.inuse_count = snap["inuse_count"].asInt();
                s.inuse_amount = snap["inuse_amount"].asInt();
            }
            //if later versions decide to use less history, delete old entries
            if ( constr->history.size() > MAX_HISTORY_SIZE )
                constr->history.erase(constr->history.begin(), constr->history.begin()+(constr->history.size()-MAX_HISTORY_SIZE));
            constr->val = c["val"].asString();
            auto where = maybeAddConstraint(constr);
            if ( where != constraints.end() ) {
                out << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": " << "redundant constraint in persistent data for workflow" << endl;
            } else {
                //out << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": " << "added new constraint" << endl;
            }
        }
        p.clear();
    } else {
        cerr << __FILE__ << ":" << __LINE__ << ": Unrecognized version: " << version << endl;
        exit(1);
    }
    //out << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": " << "constraints.size() = " << constraints.size() << endl;
    if ( isOptionEnabled((unsigned)CF_ENABLED) ) {
        plugin_self->plugin_enable(out,true);
    }
}
static void save_config(color_ostream& out) {
    //global std::vector<ItemConstraint*> constraints
    Json::Value& p = Persistent::get("workflow");
    p.clear();
    p["version"] = persist_version;
    p["flags"] = config_flags;
    Json::Value& constrs = p["constraints"];
    for ( size_t a = 0; a < constraints.size(); a++ ) {
        constrs.append(Json::Value());
        Json::Value& constrTable = constrs[constrs.size()-1];
        ItemConstraint* constraint = constraints[a];
        constrTable["goalCount"] = constraint->goalCount_;
        constrTable["goalGap"] = constraint->goalGap_;
        constrTable["goalByCount"] = constraint->goalByCount_;
        constrTable["is_craft"] = constraint->is_craft;
        constrTable["item"] = constraint->item.getToken();
        constrTable["material"] = constraint->material.getToken();
        constrTable["mat_mask"] = constraint->mat_mask.whole;
        constrTable["min_quality"] = ENUM_KEY_STR(item_quality,constraint->min_quality);
        constrTable["is_local"] = constraint->is_local;
        constrTable["weight"] = constraint->weight;
        Json::Value& histrTable = constrTable["history"];
        for ( size_t a = 0; a < constraint->history.size(); ++a ) {
            size_t i = (a+constraint->oldest_snapshot)%constraint->history.size(); //rotate it for readability in the serialization
            const Snapshot& s = constraint->history[i];
            Json::Value& hist_snapshot = histrTable[a];
            hist_snapshot["count"] = s.count;
            hist_snapshot["amount"] = s.amount;
            hist_snapshot["inuse_count"] = s.inuse_count;
            hist_snapshot["inuse_amount"] = s.inuse_amount;
        }
        //don't save jobs
        constrTable["val"] = constraint->val;
    }
#if 0
        out << "goalCount_=" << constraint->goalCount_ << endl;
        out << "goalGap_=" << constraint->goalGap_ << endl;
        out << "goalByCount_=" << constraint->goalByCount_ << endl;
        out << "is_craft=" << constraint->is_craft << endl;
        out << "material=" << constraint->material.toString() << endl;
        out << "mat_mask=" << constraint->mat_mask.whole << endl;
        out << "min_quality=" << ENUM_KEY_STR(item_quality,constraint->min_quality) << endl;
        out << "is_local=" << constraint->is_local << endl;
        out << "is_active=" << constraint->is_active << endl;
        out << "cant_resume_reported=" << constraint->cant_resume_reported << endl;
        out << "low_stock_reported=" << constraint->low_stock_reported << endl;
        print_constraint(out,constraint);
        //static void print_constraint(color_ostream &out, ItemConstraint *cv, bool no_job = false, std::string prefix = "")
        //shortJobDescription(df::job *job)
        //out << "material_cache=" << static_cast<std::map<std::pair<int,int>,bool>>(constraint->material_cache) <<endl;
        out << "material_cache=" << endl;
        for ( auto i = constraint->material_cache.begin(); i != constraint->material_cache.end(); ++i ) {
            const std::pair<int,int> x = i->first;
            const bool y = i->second;
            out << "  " << x.first << "," << x.second << ": " << y << endl;
        }
        out << endl;
    }
#endif
}

static void start_protect(color_ostream &out)
{
    out << "workflow: checking for existing job issues" << endl;
    int count = fix_job_postings(&out);
    if (count)
        out << "workflow: fixed " << count << " job issues" << endl;

    check_lost_jobs(out, 0);

    if (!known_jobs.empty())
        out.print("Protecting %d jobs.\n", known_jobs.size());
}

static void init_state(color_ostream &out)
{
    last_tick_frame_count = world->frame_counter;
    last_frame_count = world->frame_counter;

    if (!enabled)
        return;

    start_protect(out);
}

static void enable_plugin(color_ostream &out)
{
    setOptionEnabled(CF_ENABLED, true);
    enabled = true;
    out << "Enabling the plugin." << endl;

    start_protect(out);
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
            pj = new ProtectedJob(job);
            assert(pj->holder);
            known_jobs[pj->id] = pj;
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
    if ((++cnt % 5) != 0)
        return CR_OK;

    check_lost_jobs(out, world->frame_counter - last_tick_frame_count);
    last_tick_frame_count = world->frame_counter;

    // Proceed every in-game day, or when jobs to recover changed
    static unsigned last_rlen = 0;
    bool check_time = (world->frame_counter - last_frame_count) >= DAY_TICKS;

    if (pending_recover.size() != last_rlen || check_time)
    {
        recover_jobs(out);
        last_rlen = pending_recover.size();

        // If the day passed, proceed to update
        if (check_time)
        {
            last_frame_count = world->frame_counter;

            update_job_data(out);
            process_constraints(out);

            for (size_t i = 0; i < constraints.size(); i++)
                constraints[i]->updateHistory();
        }
    }

    return CR_OK;
}

/******************************
 *   ITEM COUNT CONSTRAINT    *
 ******************************/

static ItemConstraint *get_constraint(color_ostream &out, const std::string &str, bool create)
{
    std::vector<std::string> tokens;
    split_string(&tokens, str, "/");

    if (tokens.size() > 4) {
        out << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": " << "tokens.size() = " << tokens.size() << ", str = " << str << endl;
        return NULL;
    }

    int weight = 0;

    bool is_craft = false;
    ItemTypeInfo item;

    if (tokens[0] == "ANY_CRAFT" || tokens[0] == "CRAFTS") {
        is_craft = true;
    } else if (!item.find(tokens[0]) || !item.isValid()) {
        out.printerr("Cannot find item type: %s\n", tokens[0].c_str());
        return NULL;
    }

    if (item.subtype >= 0)
        weight += 10000;

    df::dfhack_material_category mat_mask(0);
    std::string maskstr = vector_get(tokens,1);
    if (!maskstr.empty() && !parseJobMaterialCategory(&mat_mask, maskstr)) {
        out.printerr("Cannot decode material mask: %s\n", maskstr.c_str());
        return NULL;
    }

    if (mat_mask.whole != 0)
        weight += 100;

    MaterialInfo material;
    std::string matstr = vector_get(tokens,2);
    if (!matstr.empty() && (!material.find(matstr) || !material.isValid())) {
        out.printerr("Cannot find material: %s\n", matstr.c_str());
        return NULL;
    }

    if (material.type >= 0)
        weight += (material.index >= 0 ? 5000 : 1000);

    if (mat_mask.whole && material.isValid() && !material.matches(mat_mask)) {
        out.printerr("Material %s doesn't match mask %s\n", matstr.c_str(), maskstr.c_str());
        return NULL;
    }

    item_quality::item_quality minqual = item_quality::Ordinary;
    bool is_local = false;
    std::string qualstr = vector_get(tokens, 3);

    if(!qualstr.empty())
    {
        std::vector<std::string> qtokens;
        split_string(&qtokens, qualstr, ",");

        for (size_t i = 0; i < qtokens.size(); i++)
        {
            auto token = toLower(qtokens[i]);

            if (token == "local")
                is_local = true;
            else
            {
                bool found = false;
                FOR_ENUM_ITEMS(item_quality, qv)
                {
                    if (toLower(ENUM_KEY_STR(item_quality, qv)) != token)
                        continue;
                    minqual = qv;
                    found = true;
                }

                if (!found)
                {
                    out.printerr("Cannot parse token: %s\n", token.c_str());
                    return NULL;
                }
            }
        }
    }

    if (is_local || minqual > item_quality::Ordinary)
        weight += 10;

    ItemConstraint *nct = new ItemConstraint;
    nct->is_craft = is_craft;
    nct->item = item;
    nct->material = material;
    nct->mat_mask = mat_mask;
    nct->min_quality = minqual;
    nct->is_local = is_local;
    nct->weight = weight;
    nct->init(str);
    auto where = maybeAddConstraint(nct);
    if ( where != constraints.end() ) {
        delete nct;
        return *where;
    }
    return nct;
}

static vector<ItemConstraint*>::iterator maybeAddConstraint(ItemConstraint* cv) {
    for ( auto i = constraints.begin(); i != constraints.end(); ++i ) {
        ItemConstraint* ct = *i;
        if ( cv->is_craft == ct->is_craft &&
                cv->item == ct->item &&
                cv->mat_mask.whole == ct->mat_mask.whole &&
                cv->min_quality == ct->min_quality &&
                cv->is_local == ct->is_local ) {
            return i;
        }
    }
    constraints.push_back(cv);
    return constraints.end();
}

static void delete_constraint(ItemConstraint *cv)
{
    int idx = linear_index(constraints, cv);
    if (idx >= 0)
        vector_erase_at(constraints, idx);

    delete cv;
}

static bool deleteConstraint(std::string name)
{
    for (size_t i = 0; i < constraints.size(); i++)
    {
        if (constraints[i]->val != name)
            continue;

        delete_constraint(constraints[i]);
        return true;
    }

    return false;
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
                                bool is_craft = false)
{
    MaterialInfo mat(mat_type, mat_index);

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

        if (!ct->is_active && pj->isResumed())
            ct->is_active = true;
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

static int cbEnumJobOutputs(lua_State *L)
{
    auto pj = (ProtectedJob*)lua_touserdata(L, lua_upvalueindex(1));

    lua_settop(L, 6);

    df::dfhack_material_category mat_mask(0);
    if (!lua_isnil(L, 3))
        Lua::CheckDFAssign(L, &mat_mask, 3);

    link_job_constraint(
        pj,
        (df::item_type)luaL_optint(L, 1, -1), luaL_optint(L, 2, -1),
        mat_mask, luaL_optint(L, 4, -1), luaL_optint(L, 5, -1),
        lua_toboolean(L, 6)
    );

    return 0;
}

static void map_job_constraints(color_ostream &out)
{
    melt_active = false;

    for (size_t i = 0; i < constraints.size(); i++)
    {
        constraints[i]->jobs.clear();
        constraints[i]->is_active = false;
    }

    auto L = Lua::Core::State;
    Lua::StackUnwinder frame(L);

    bool ok = Lua::PushModulePublic(out, L, "plugins.workflow", "doEnumJobOutputs");
    if (!ok)
        out.printerr("The workflow lua module is not available.\n");

    for (TKnownJobs::const_iterator it = known_jobs.begin(); it != known_jobs.end(); ++it)
    {
        ProtectedJob *pj = it->second;

        pj->constraints.clear();

        if (!ok || !pj->isLive())
            continue;

        if (!melt_active && pj->actual_job->job_type == job_type::MeltMetalObject)
            melt_active = pj->isResumed();

        // Call the lua module
        lua_pushvalue(L, -1);
        lua_pushlightuserdata(L, pj);
        lua_pushcclosure(L, cbEnumJobOutputs, 1);
        Lua::PushDFObject(L, pj->job_copy);

        Lua::SafeCall(out, L, 2, 0);
    }
}

/******************************
 *  ITEM-CONSTRAINT MAPPING   *
 ******************************/

static void dryBucket(df::item *item)
{
    for (size_t i = 0; i < item->general_refs.size(); i++)
    {
        df::general_ref *ref = item->general_refs[i];
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

    for (size_t i = 0; i < item->general_refs.size(); i++)
    {
        df::general_ref *ref = item->general_refs[i];
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

static bool isRouteVehicle(df::item *item)
{
    int id = item->getVehicleID();
    if (id < 0) return false;

    auto vehicle = df::vehicle::find(id);
    return vehicle && vehicle->route_id >= 0;
}

static bool isAssignedSquad(df::item *item)
{
    auto &vec = ui->equipment.items_assigned[item->getType()];
    return binsearch_index(vec, &df::item::id, item->id) >= 0;
}

static void map_job_items(color_ostream &out)
{
    for (size_t i = 0; i < constraints.size(); i++)
    {
        constraints[i]->item_amount = 0;
        constraints[i]->item_count = 0;
        constraints[i]->item_inuse_amount = 0;
        constraints[i]->item_inuse_count = 0;
    }

    meltable_count = 0;

    // Precompute a bitmask with the bad flags
    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
    F(dump); F(forbid); F(garbage_collect);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction); F(artifact);
#undef F

    bool dry_buckets = isOptionEnabled(CF_DRYBUCKETS);

    std::vector<df::item*> &items = world->items.other[items_other_id::IN_PLAY];

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
            if (item->flags.bits.spider_web)
                continue;
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

            if (cv->is_local && item->flags.bits.foreign)
                continue;
            if (item->getQuality() < cv->min_quality)
                continue;

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
                isRouteVehicle(item) ||
                itemInRealJob(item) ||
                itemBusy(item) ||
                isAssignedSquad(item))
            {
                is_invalid = true;
                cv->item_inuse_count++;
                cv->item_inuse_amount += item->getStackSize();
            }
            else
            {
                cv->item_count++;
                cv->item_amount += item->getStackSize();
            }
        }
    }

    for (size_t i = 0; i < constraints.size(); i++)
        constraints[i]->computeRequest();
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

        if (ct->low_stock_reported != DF_GLOBAL_VALUE(cur_season,-1))
        {
            int count = ct->goalCount(), gap = ct->goalGap();

            if (count >= gap*3 && ct->curItemStock() < std::min(gap*2, (count-gap)/2))
            {
                ct->low_stock_reported = DF_GLOBAL_VALUE(cur_season,-1);

                Gui::showAnnouncement("Stock level is low: " + info, COLOR_BROWN, true);
            }
            else
                ct->low_stock_reported = -1;
        }

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

static void update_data_structures(color_ostream &out)
{
    if (enabled) {
        check_lost_jobs(out, 0);
        recover_jobs(out);
        update_job_data(out);
        map_job_constraints(out);
        map_job_items(out);
    }
}

/*************
 *  LUA API  *
 *************/

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (enable && !enabled)
    {
        enable_plugin(out);
    }
    else if (!enable && enabled)
    {
        enabled = false;
        setOptionEnabled(CF_ENABLED, false);
        stop_protect(out);
    }

    return CR_OK;
}

static void push_count_history(lua_State *L, ItemConstraint *icv)
{
    size_t hsize = icv->history.size();

    lua_createtable(L, hsize, 0);

    for (size_t i = 0; i < hsize; i++)
    {
        size_t index = (i+icv->oldest_snapshot)%hsize;
        const Snapshot& s = icv->history[i];
        lua_createtable(L, 0, 4);

        Lua::SetField(L, s.amount, -1, "cur_amount");
        Lua::SetField(L, s.count, -1, "cur_count");
        Lua::SetField(L, s.inuse_amount, -1, "cur_in_use_amount");
        Lua::SetField(L, s.inuse_count, -1, "cur_in_use_count");

        lua_rawseti(L, -2, i+1);
    }
}

static void push_constraint(lua_State *L, ItemConstraint *cv)
{
    lua_newtable(L);
    int ctable = lua_gettop(L);

    //Lua::SetField(L, cv->config.entry_id(), ctable, "id");
    Lua::SetField(L, cv->val, ctable, "token");

    // Constraint key

    Lua::SetField(L, cv->item.type, ctable, "item_type");
    Lua::SetField(L, cv->item.subtype, ctable, "item_subtype");

    Lua::SetField(L, cv->is_craft, ctable, "is_craft");

    lua_getglobal(L, "copyall");
    Lua::PushDFObject(L, &cv->mat_mask);
    lua_call(L, 1, 1);
    lua_setfield(L, -2, "mat_mask");

    Lua::SetField(L, cv->material.type, ctable, "mat_type");
    Lua::SetField(L, cv->material.index, ctable, "mat_index");

    Lua::SetField(L, (int)cv->min_quality, ctable, "min_quality");
    Lua::SetField(L, (bool)cv->is_local, ctable, "is_local");

    // Constraint value

    Lua::SetField(L, cv->goalByCount(), ctable, "goal_by_count");
    Lua::SetField(L, cv->goalCount(), ctable, "goal_value");
    Lua::SetField(L, cv->goalGap(), ctable, "goal_gap");

    Lua::SetField(L, cv->item_amount, ctable, "cur_amount");
    Lua::SetField(L, cv->item_count, ctable, "cur_count");
    Lua::SetField(L, cv->item_inuse_amount, ctable, "cur_in_use_amount");
    Lua::SetField(L, cv->item_inuse_count, ctable, "cur_in_use_count");

    // Current state value

    if (cv->request_resume)
        Lua::SetField(L, "resume", ctable, "request");
    else if (cv->request_suspend)
        Lua::SetField(L, "suspend", ctable, "request");

    lua_newtable(L);

    bool resumed = false, want_resumed = false;

    for (size_t i = 0, j = 0; i < cv->jobs.size(); i++)
    {
        if (!cv->jobs[i]->isLive()) continue;
        Lua::PushDFObject(L, cv->jobs[i]->actual_job);
        lua_rawseti(L, -2, ++j);

        if (cv->jobs[i]->want_resumed) {
            want_resumed = true;
            resumed = resumed || cv->jobs[i]->isActuallyResumed();
        }
    }

    lua_setfield(L, ctable, "jobs");

    if (want_resumed && !resumed)
        Lua::SetField(L, true, ctable, "is_delayed");
}

static int listConstraints(lua_State *L)
{
    lua_settop(L, 2);
    auto job = Lua::CheckDFObject<df::job>(L, 1);
    bool with_history = lua_toboolean(L, 2);

    lua_pushnil(L);

    if (!enabled || (job && !isSupportedJob(job)))
        return 1;

    color_ostream &out = *Lua::GetOutput(L);
    update_data_structures(out);

    ProtectedJob *pj = NULL;
    if (job)
    {
        pj = get_known(job->id);
        if (!pj)
            return 1;
    }

    lua_newtable(L);

    auto &vec = (pj ? pj->constraints : constraints);

    for (size_t i = 0; i < vec.size(); i++)
    {
        push_constraint(L, vec[i]);

        if (with_history)
        {
            push_count_history(L, vec[i]);
            lua_setfield(L, -2, "history");
        }

        lua_rawseti(L, -2, i+1);
    }

    return 1;
}

static int findConstraint(lua_State *L)
{
    auto token = luaL_checkstring(L, 1);

    color_ostream &out = *Lua::GetOutput(L);
    update_data_structures(out);

    ItemConstraint *icv = get_constraint(out, token, false);

    if (icv)
        push_constraint(L, icv);
    else
        lua_pushnil(L);
    return 1;
}

static int setConstraint(lua_State *L)
{
    auto token = luaL_checkstring(L, 1);
    bool by_count = lua_toboolean(L, 2);
    int count = luaL_optint(L, 3, -1);
    int gap = luaL_optint(L, 4, -1);

    color_ostream &out = *Lua::GetOutput(L);
    update_data_structures(out);

    ItemConstraint *icv = get_constraint(out, token);
    if (!icv)
        luaL_error(L, "invalid constraint: %s", token);

    if (!lua_isnil(L, 2))
        icv->setGoalByCount(by_count);
    if (!lua_isnil(L, 3))
        icv->setGoalCount(count);
    if (!lua_isnil(L, 4))
        icv->setGoalGap(gap);

    process_constraints(out);
    push_constraint(L, icv);
    return 1;
}

static int getCountHistory(lua_State *L)
{
    auto token = luaL_checkstring(L, 1);

    color_ostream &out = *Lua::GetOutput(L);
    update_data_structures(out);

    ItemConstraint *icv = get_constraint(out, token, false);

    if (icv)
        push_count_history(L, icv);
    else
        lua_pushnil(L);

    return 1;
}

static int fixJobPostings(lua_State *L)
{
    bool dry = lua_toboolean(L, 1);
    lua_pushinteger(L, fix_job_postings(NULL, dry));
    return 1;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(deleteConstraint),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(listConstraints),
    DFHACK_LUA_COMMAND(findConstraint),
    DFHACK_LUA_COMMAND(setConstraint),
    DFHACK_LUA_COMMAND(getCountHistory),
    DFHACK_LUA_COMMAND(fixJobPostings),
    DFHACK_LUA_END
};

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

static void print_constraint(color_ostream &out, ItemConstraint *cv, bool no_job, std::string prefix)
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
    out << cv->val << " " << flush;
    out.color(color);
    out << (cv->goalByCount() ? "count " : "amount ")
           << cv->goalCount() << " (gap " << cv->goalGap() << ")" << endl;
    out.reset_color();

    if (cv->item_count || cv->item_inuse_count)
        out << prefix << "  items: amount " << cv->item_amount << "; "
                         << cv->item_count << " stacks available, "
                         << cv->item_inuse_count << " in use." << endl;

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

static command_result workflow_cmd(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    update_data_structures(out);

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
        if (enable)
            plugin_enable(out, true);
        else if (parameters.size() == 1)
        {
            plugin_enable(out, false);

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
    {
        out.printerr("Error: the plugin is not enabled.\n");
        return CR_WRONG_USAGE;
    }

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
                << cv->val << " " << cv->goalCount() << " " << cv->goalGap() << endl;
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

        if (deleteConstraint(parameters[1]))
            return CR_OK;

        out.printerr("Constraint not found: %s\n", parameters[1].c_str());
        return CR_FAILURE;
    }
    else if (cmd == "unlimit-all")
    {
        if (parameters.size() != 1)
            return CR_WRONG_USAGE;

        while (!constraints.empty())
            delete_constraint(constraints[0]);

        out.print("Removed all constraints.\n");
        return CR_OK;
    }
    else
        return CR_WRONG_USAGE;
}
