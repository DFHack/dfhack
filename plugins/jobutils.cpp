#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <MiscUtils.h>

#include <modules/Materials.h>

#include <DataDefs.h>
#include <df/world.h>
#include <df/ui.h>
#include <df/ui_build_selector.h>
#include <df/ui_build_item_req.h>
#include <df/build_req_choice_genst.h>
#include <df/build_req_choice_specst.h>
#include <df/building_workshopst.h>
#include <df/building_furnacest.h>
#include <df/job.h>
#include <df/job_item.h>
#include <df/job_list_link.h>
#include <df/item.h>
#include <df/tool_uses.h>
#include <df/general_ref.h>
#include <df/general_ref_unit_workerst.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;
using df::global::ui_build_selector;
using df::global::ui_workshop_job_cursor;
using df::global::job_next_id;

/* Plugin registration */

static bool workshop_job_hotkey(Core *c, df::viewscreen *top);
static bool build_selector_hotkey(Core *c, df::viewscreen *top);
static bool job_material_hotkey(Core *c, df::viewscreen *top);

static command_result job_material(Core *c, vector <string> & parameters);
static command_result job_duplicate(Core *c, vector <string> & parameters);
static command_result job_cmd(Core *c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "jobutils";
}

DFhackCExport command_result plugin_init (Core *c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    if (!world || !ui)
        return CR_FAILURE;

    commands.push_back(
        PluginCommand(
            "job", "General job query and manipulation.",
            job_cmd, false,
            "  job query   - Print details of the current job.\n"
            "  job list    - Print details of all jobs in the workshop.\n"
        )
    );

    if (ui_workshop_job_cursor || ui_build_selector) {
        commands.push_back(
            PluginCommand(
                "job-material", "Alter the material of the selected job.",
                job_material, job_material_hotkey,
                "  job-material <inorganic-token>\n"
                "Intended to be used as a keybinding:\n"
                "  - In 'q' mode, when a job is highlighted within a workshop\n"
                "    or furnace, changes the material of the job.\n"
                "  - In 'b' mode, during selection of building components\n"
                "    positions the cursor over the first available choice\n"
                "    with the matching material.\n"
            )
        );
    }

    if (ui_workshop_job_cursor && job_next_id) {
        commands.push_back(
            PluginCommand(
                "job-duplicate", "Duplicate the selected job in a workshop.",
                job_duplicate, workshop_job_hotkey,
                "  - In 'q' mode, when a job is highlighted within a workshop\n"
                "    or furnace building, instantly duplicates the job.\n"
            )
        );
    }

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

/* UI state guards */

static bool workshop_job_hotkey(Core *c, df::viewscreen *top)
{
    using namespace ui_sidebar_mode;

    if (!dwarfmode_hotkey(c,top))
        return false;

    switch (ui->main.mode) {
    case QueryBuilding:
        {
            if (!ui_workshop_job_cursor) // allow missing
                return false;

            df::building *selected = world->selected_building;
            if (!virtual_cast<df::building_workshopst>(selected) &&
                !virtual_cast<df::building_furnacest>(selected))
                return false;

            // No jobs?
            if (selected->jobs.empty() ||
                selected->jobs[0]->job_type == job_type::DestroyBuilding)
                return false;

            // Add job gui activated?
            if (df::global::ui_workshop_in_add && // allow missing
                *df::global::ui_workshop_in_add)
                return false;

            return true;
        };
    default:
        return false;
    }
}

static bool build_selector_hotkey(Core *c, df::viewscreen *top)
{
    using namespace ui_sidebar_mode;

    if (!dwarfmode_hotkey(c,top))
        return false;

    switch (ui->main.mode) {
    case Build:
        {
            if (!ui_build_selector) // allow missing
                return false;

            // Not selecting, or no choices?
            if (ui_build_selector->building_type < 0 ||
                ui_build_selector->stage != 2 ||
                ui_build_selector->choices.empty())
                return false;

            return true;
        };
    default:
        return false;
    }
}

static bool job_material_hotkey(Core *c, df::viewscreen *top)
{
    return workshop_job_hotkey(c, top) ||
           build_selector_hotkey(c, top);
}

/* job-material implementation */

static df::job *getWorkshopJob(Core *c)
{
    df::building *selected = world->selected_building;
    int idx = *ui_workshop_job_cursor;

    if (idx < 0 || idx >= selected->jobs.size())
    {
        c->con.printerr("Invalid job cursor index: %d\n", idx);
        return NULL;
    }

    return selected->jobs[idx];
}

static command_result job_material_in_job(Core *c, MaterialInfo &new_mat)
{
    df::job *job = getWorkshopJob(c);
    if (!job)
        return CR_FAILURE;

    MaterialInfo cur_mat(job);

    if (!cur_mat.isValid() || cur_mat.type != 0)
    {
        c->con.printerr("Current job material isn't inorganic: %s\n",
                        cur_mat.toString().c_str());
        return CR_FAILURE;
    }

    df::craft_material_class old_class = cur_mat.getCraftClass();
    if (old_class == craft_material_class::None)
    {
        c->con.printerr("Unexpected current material type: %s\n",
                        cur_mat.toString().c_str());
        return CR_FAILURE;
    }
    if (new_mat.getCraftClass() != old_class)
    {
        c->con.printerr("New material %s does not satisfy requirement: %s\n",
                        new_mat.toString().c_str(), ENUM_KEY_STR(craft_material_class, old_class));
        return CR_FAILURE;
    }

    for (unsigned i = 0; i < job->job_items.size(); i++)
    {
        df::job_item *item = job->job_items[i];
        MaterialInfo item_mat(item);

        if (item_mat != cur_mat)
        {
            c->con.printerr("Job item %d has different material: %s\n",
                            i, item_mat.toString().c_str());
            return CR_FAILURE;
        }
    }

    // Apply the substitution
    job->mat_type = new_mat.type;
    job->mat_index = new_mat.index;

    for (unsigned i = 0; i < job->job_items.size(); i++)
    {
        df::job_item *item = job->job_items[i];
        item->mat_type = new_mat.type;
        item->mat_index = new_mat.index;
    }

    c->con << "Applied material '" << new_mat.toString()
           << "' to job " << ENUM_KEY_STR(job_type,job->job_type) << endl;
    return CR_OK;
}

static bool build_choice_matches(df::ui_build_item_req *req, df::build_req_choicest *choice,
                                 MaterialInfo &new_mat)
{
    if (VIRTUAL_CAST_VAR(gen, df::build_req_choice_genst, choice))
    {
        if (gen->mat_type == new_mat.type &&
            gen->mat_index == new_mat.index &&
            gen->used_count < gen->candidates.size())
        {
            return true;
        }
    }
    else if (VIRTUAL_CAST_VAR(spec, df::build_req_choice_specst, choice))
    {
        if (spec->candidate &&
            spec->candidate->getActualMaterial() == new_mat.type &&
            spec->candidate->getActualMaterialIndex() == new_mat.index &&
            !req->candidate_selected[spec->candidate_id])
        {
            return true;
        }
    }

    return false;
}

static command_result job_material_in_build(Core *c, MaterialInfo &new_mat)
{
    df::ui_build_item_req *req = ui_build_selector->requirements[ui_build_selector->req_index];

    for (unsigned i = 0; i < ui_build_selector->choices.size(); i++)
    {
        if (build_choice_matches(req, ui_build_selector->choices[i], new_mat))
        {
            ui_build_selector->sel_index = i;
            return CR_OK;
        }
    }

    c->con.printerr("Could not find material in list: %s\n", new_mat.toString().c_str());
    return CR_FAILURE;
}

static command_result job_material(Core * c, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED

    MaterialInfo new_mat;
    if (parameters.size() == 1)
    {
        if (!new_mat.findInorganic(parameters[0])) {
            c->con.printerr("Could not find inorganic material: %s\n", parameters[0].c_str());
            return CR_WRONG_USAGE;
        }
    }
    else
        return CR_WRONG_USAGE;

    if (ui->main.mode == ui_sidebar_mode::QueryBuilding)
        return job_material_in_job(c, new_mat);
    if (ui->main.mode == ui_sidebar_mode::Build)
        return job_material_in_build(c, new_mat);

    return CR_WRONG_USAGE;
}

/* job-duplicate implementation */

static df::job *clone_job(df::job *job)
{
    df::job *pnew = new df::job(*job);

    pnew->id = (*job_next_id)++;

    // Clean out transient fields
    pnew->flags.whole = 0;
    pnew->flags.bits.repeat = job->flags.bits.repeat;

    pnew->completion_timer = -1;
    pnew->items.clear();
    pnew->misc_links.clear();

    // Link the job into the global list
    pnew->list_link = new df::job_list_link();
    pnew->list_link->item = pnew;

    linked_list_append(&world->job_list, pnew->list_link);

    // Clone refs
    for (int i = pnew->references.size()-1; i >= 0; i--)
    {
        df::general_ref *ref = pnew->references[i];

        if (virtual_cast<df::general_ref_unit_workerst>(ref))
            pnew->references.erase(pnew->references.begin()+i);
        else
            pnew->references[i] = ref->clone();
    }

    // Clone items
    for (int i = pnew->job_items.size()-1; i >= 0; i--)
        pnew->job_items[i] = new df::job_item(*pnew->job_items[i]);

    return pnew;
}

static command_result job_duplicate(Core * c, vector <string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    df::job *job = getWorkshopJob(c);
    if (!job)
        return CR_FAILURE;

    if (!job->misc_links.empty() || job->job_items.empty())
    {
        c->con.printerr("Cannot duplicate job %s\n", ENUM_KEY_STR(job_type,job->job_type));
        return CR_FAILURE;
    }

    df::building *building = world->selected_building;
    if (building->jobs.size() >= 10)
    {
        c->con.printerr("Job list is already full.\n");
        return CR_FAILURE;
    }

    // Actually clone
    df::job *pnew = clone_job(job);

    int pos = ++*ui_workshop_job_cursor;
    building->jobs.insert(building->jobs.begin()+pos, pnew);

    return CR_OK;
}

/* Main job command implementation */

static void print_job_item_details(Core *c, df::job *job, df::job_item *item)
{
    c->con << "  Input Item: " << ENUM_KEY_STR(item_type,item->item_type);
    if (item->item_subtype != -1)
        c->con << " [" << item->item_subtype << "]";
    if (item->quantity != 1)
        c->con << "; quantity=" << item->quantity;
    if (item->min_dimension >= 0)
        c->con << "; min_dimension=" << item->min_dimension;
    c->con << endl;

    MaterialInfo mat(item);
    if (mat.isValid() || item->metal_ore >= 0) {
        c->con << "    material: " << mat.toString();
        if (item->metal_ore >= 0)
            c->con << "; ore of " << MaterialInfo(0,item->metal_ore).toString();
        c->con << endl;
    }

    if (item->flags1.whole)
        c->con << "    flags1: " << bitfieldToString(item->flags1) << endl;
    if (item->flags2.whole)
        c->con << "    flags2: " << bitfieldToString(item->flags2) << endl;
    if (item->flags3.whole)
        c->con << "    flags3: " << bitfieldToString(item->flags3) << endl;

    if (!item->reaction_class.empty())
        c->con << "    reaction class: " << item->reaction_class << endl;
    if (!item->has_material_reaction_product.empty())
        c->con << "    reaction product: " << item->has_material_reaction_product << endl;
    if (item->has_tool_use >= 0)
        c->con << "    tool use: " << ENUM_KEY_STR(tool_uses, item->has_tool_use) << endl;
}

static void print_job_details(Core *c, df::job *job)
{
    c->con << "Job " << job->id << ": " << ENUM_KEY_STR(job_type,job->job_type);
    if (job->flags.whole)
           c->con << " (" << bitfieldToString(job->flags) << ")";
    c->con << endl;

    MaterialInfo mat(job);
    if (mat.isValid() || job->material_category.whole)
    {
        c->con << "    material: " << mat.toString();
        if (job->material_category.whole)
            c->con << " (" << bitfieldToString(job->material_category) << ")";
        c->con << endl;
    }

    if (job->item_subtype >= 0 || job->item_category.whole)
        c->con << "    item: " << job->item_subtype
               << " (" << bitfieldToString(job->item_category) << ")" << endl;

    if (job->hist_figure_id >= 0)
        c->con << "    figure: " << job->hist_figure_id << endl;

    if (!job->reaction_name.empty())
        c->con << "    reaction: " << job->reaction_name << endl;

    for (unsigned i = 0; i < job->job_items.size(); i++)
        print_job_item_details(c, job, job->job_items[i]);
}

static command_result job_cmd(Core * c, vector <string> & parameters)
{
    CoreSuspender suspend(c);

    if (parameters.empty())
        return CR_WRONG_USAGE;

    std::string cmd = parameters[0];
    if (cmd == "query" || cmd == "list")
    {
        if (!workshop_job_hotkey(c, c->getTopViewscreen())) {
            c->con.printerr("No job is highlighted.\n");
            return CR_WRONG_USAGE;
        }

        if (cmd == "query") {
            df::job *job = getWorkshopJob(c);
            if (!job)
                return CR_FAILURE;
            print_job_details(c, job);
        } else {
            df::building *selected = world->selected_building;
            for (unsigned i = 0; i < selected->jobs.size(); i++)
                print_job_details(c, selected->jobs[i]);
        }
    }
    else
        return CR_WRONG_USAGE;

    return CR_OK;
}
