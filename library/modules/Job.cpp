/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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
#include "PluginManager.h"

#include "modules/Job.h"
#include "modules/Materials.h"

#include "DataDefs.h"
#include <df/world.h>
#include <df/ui.h>
#include <df/job.h>
#include <df/job_item.h>
#include <df/job_list_link.h>
#include <df/general_ref.h>
#include <df/general_ref_unit_workerst.h>

using namespace DFHack;
using namespace df::enums;

df::job *DFHack::cloneJobStruct(df::job *job)
{
    df::job *pnew = new df::job(*job);

    // Clean out transient fields
    pnew->flags.whole = 0;
    pnew->flags.bits.repeat = job->flags.bits.repeat;
    pnew->flags.bits.suspend = job->flags.bits.suspend;

    pnew->list_link = NULL;
    pnew->completion_timer = -1;
    pnew->items.clear();
    pnew->misc_links.clear();

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

void DFHack::deleteJobStruct(df::job *job)
{
    if (!job)
        return;

    // Only allow free-floating job structs
    assert(!job->list_link && job->items.empty() && job->misc_links.empty());

    for (int i = job->references.size()-1; i >= 0; i--)
        delete job->references[i];

    for (int i = job->job_items.size()-1; i >= 0; i--)
        delete job->job_items[i];

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
    if (!(CMP(job_type) && CMP(unk2) &&
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

static void print_job_item_details(Core *c, df::job *job, unsigned idx, df::job_item *item)
{
    c->con << "  Input Item " << (idx+1) << ": " << ENUM_KEY_STR(item_type,item->item_type);
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

void DFHack::printJobDetails(Core *c, df::job *job)
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
        print_job_item_details(c, job, i, job->job_items[i]);
}
