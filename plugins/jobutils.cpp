#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <MiscUtils.h>

#include <DataDefs.h>
#include <df/world.h>
#include <df/ui.h>
#include <df/ui_build_selector.h>
#include <df/ui_build_item_req.h>
#include <df/build_req_choice_genst.h>
#include <df/build_req_choice_specst.h>
#include <df/building_workshopst.h>
#include <df/building_furnacest.h>
#include <df/material.h>
#include <df/inorganic_raw.h>
#include <df/plant_raw.h>
#include <df/creature_raw.h>
#include <df/historical_figure.h>
#include <df/job.h>
#include <df/job_item.h>
#include <df/item.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

/*
 * TODO: This should be in core:
 */

struct MaterialInfo {
    int16_t type;
    int32_t index;

    df::material *material;

    enum Mode {
        Builtin,
        Inorganic,
        Creature,
        Plant
    };
    Mode mode;

    int16_t subtype;
    df::inorganic_raw *inorganic;
    df::creature_raw *creature;
    df::plant_raw *plant;

    df::historical_figure *figure;

    MaterialInfo(int16_t type = -1, int32_t index = -1) {
        decode(type, index);
    }

    bool isValid() const { return material != NULL; }
    bool decode(int16_t type, int32_t index = -1);

    bool findInorganic(const std::string &token);

    std::string toString(uint16_t temp = 10015, bool named = true);
};

inline bool operator== (const MaterialInfo &a, const MaterialInfo &b) {
    return a.type == b.type && a.index == b.index;
}
inline bool operator!= (const MaterialInfo &a, const MaterialInfo &b) {
    return a.type != b.type || a.index != b.index;
}

bool MaterialInfo::decode(int16_t type, int32_t index)
{
    this->type = type;
    this->index = index;

    material = NULL;
    mode = Builtin; subtype = 0;
    inorganic = NULL; plant = NULL; creature = NULL;
    figure = NULL;

    if (type < 0 || type >= 659)
        return false;

    df::world_raws &raws = df::global::world->raws;

    if (index < 0)
    {
        material = raws.mat_table.builtin[type];
    }
    else if (type == 0)
    {
        mode = Inorganic;
        inorganic = df::inorganic_raw::find(index);
        if (!inorganic)
            return false;
        material = &inorganic->material;
    }
    else if (type < 19)
    {
        material = raws.mat_table.builtin[type];
    }
    else if (type < 219)
    {
        mode = Creature;
        subtype = type-19;
        creature = df::creature_raw::find(index);
        if (!creature || subtype >= creature->material.size())
            return false;
        material = creature->material[subtype];
    }
    else if (type < 419)
    {
        mode = Creature;
        subtype = type-219;
        figure = df::historical_figure::find(index);
        if (!figure)
            return false;
        creature = df::creature_raw::find(figure->race);
        if (!creature || subtype >= creature->material.size())
            return false;
        material = creature->material[subtype];
    }
    else if (type < 619)
    {
        mode = Plant;
        subtype = type-419;
        plant = df::plant_raw::find(index);
        if (!plant || subtype >= plant->material.size())
            return false;
        material = plant->material[subtype];
    }
    else
    {
        material = raws.mat_table.builtin[type];
    }

    return (material != NULL);
}

bool MaterialInfo::findInorganic(const std::string &token)
{
    df::world_raws &raws = df::global::world->raws;
    for (unsigned i = 0; i < raws.inorganics.size(); i++)
    {
        df::inorganic_raw *p = raws.inorganics[i];
        if (p->id == token)
            return decode(0, i);
    }
    return decode(-1);
}

std::string MaterialInfo::toString(uint16_t temp, bool named)
{
    if (type == -1)
        return "NONE";
    if (!material)
        return stl_sprintf("INVALID %d:%d", type, index);

    int idx = 0;
    if (temp >= material->heat.melting_point)
        idx = 1;
    if (temp >= material->heat.boiling_point)
        idx = 2;
    std::string name = material->state_name[idx];
    if (!material->prefix.empty())
        name = material->prefix + " " + name;

    if (named && figure)
        name += stl_sprintf(" of HF %d", index);
    return name;
}

/*
 * Plugin-specific code starts here.
 */

using df::global::world;
using df::global::ui;
using df::global::ui_build_selector;
using df::global::ui_workshop_job_cursor;

static bool job_material_hotkey(Core *c, df::viewscreen *top);
static command_result job_material(Core *c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "jobutils";
}

DFhackCExport command_result plugin_init (Core *c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    if (world && ui && (ui_workshop_job_cursor || ui_build_selector)) {
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
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

static bool job_material_hotkey(Core *c, df::viewscreen *top)
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
                selected->jobs[0]->job_id == job_type::DestroyBuilding)
                return false;

            // Add job gui activated?
            if (df::global::ui_workshop_in_add && // allow missing
                *df::global::ui_workshop_in_add)
                return false;

            return true;
        };
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

static command_result job_material_in_job(Core *c, MaterialInfo &new_mat)
{
    df::building *selected = world->selected_building;
    int idx = *ui_workshop_job_cursor;

    if (idx < 0 || idx >= selected->jobs.size())
    {
        c->con.printerr("Invalid job cursor index: %d\n", idx);
        return CR_FAILURE;
    }

    df::job *job = selected->jobs[idx];
    MaterialInfo cur_mat(job->matType, job->matIndex);

    if (!cur_mat.isValid() || cur_mat.type != 0)
    {
        c->con.printerr("Current job material isn't inorganic: %s\n",
                        cur_mat.toString().c_str());
        return CR_FAILURE;
    }

    // Verify that the new material matches the old one
    df::material_flags rq_flag = material_flags::IS_STONE;
    if (cur_mat.mode != MaterialInfo::Builtin)
    {
        if (cur_mat.material->flags.is_set(material_flags::IS_GEM))
            rq_flag = material_flags::IS_GEM;
        else if (cur_mat.material->flags.is_set(material_flags::IS_METAL))
            rq_flag = material_flags::IS_METAL;
        else if (!cur_mat.material->flags.is_set(rq_flag))
        {
            c->con.printerr("Unexpected current material type: %s\n",
                            cur_mat.toString().c_str());
            return CR_FAILURE;
        }
    }

    if (!new_mat.material->flags.is_set(rq_flag))
    {
        c->con.printerr("New material %s does not satisfy requirement: %s\n",
                        new_mat.toString().c_str(), ENUM_KEY_STR(material_flags, rq_flag));
        return CR_FAILURE;
    }

    for (unsigned i = 0; i < job->job_items.size(); i++)
    {
        df::job_item *item = job->job_items[i];
        MaterialInfo item_mat(item->matType, item->matIndex);

        if (item_mat != cur_mat)
        {
            c->con.printerr("Job item %d has different material: %s\n",
                            i, item_mat.toString().c_str());
            return CR_FAILURE;
        }
    }

    // Apply the substitution
    job->matType = new_mat.type;
    job->matIndex = new_mat.index;

    for (unsigned i = 0; i < job->job_items.size(); i++)
    {
        df::job_item *item = job->job_items[i];
        item->matType = new_mat.type;
        item->matIndex = new_mat.index;
    }

    c->con << "Applied material '" << new_mat.toString()
           << " jo job " << ENUM_KEY_STR(job_type,job->job_id) << endl;
    return CR_OK;
}

static command_result job_material_in_build(Core *c, MaterialInfo &new_mat)
{
    df::ui_build_item_req *req = ui_build_selector->requirements[ui_build_selector->req_index];

    for (unsigned i = 0; i < ui_build_selector->choices.size(); i++)
    {
        df::build_req_choicest *choice = ui_build_selector->choices[i];
        bool found = false;

        if (VIRTUAL_CAST_VAR(gen, df::build_req_choice_genst, choice))
        {
            if (gen->matType == new_mat.type &&
                gen->matIndex == new_mat.index &&
                gen->used_count < gen->candidates.size())
            {
                found = true;
            }
        }
        else if (VIRTUAL_CAST_VAR(spec, df::build_req_choice_specst, choice))
        {
            if (spec->candidate &&
                spec->candidate->getActualMaterial() == new_mat.type &&
                spec->candidate->getActualMaterialIndex() == new_mat.index &&
                !req->candidate_selected[spec->candidate_id])
            {
                found = true;
            }
        }

        if (found)
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
