#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include "modules/Materials.h"
#include "modules/Items.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/item.h"
#include "df/general_ref.h"
#include "df/items_other_id.h"
#include "df/item_quality.h"

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;

static command_result automelt(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("automelt");

DFHACK_PLUGIN_IS_ENABLED(enabled);

bool is_metal(df::item* item)
{
    MaterialInfo mat(item);
    return (mat.getCraftClass() == craft_material_class::Metal);
}

bool is_meltable(df::item* item)
{

    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
    F(dump); F(forbid); F(garbage_collect); F(in_job);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction); F(artifact); F(melt);
#undef F

    if (item->flags.whole & bad_flags.whole)
        return false;

    df::item_type t = item->getType();

    if (t == df::enums::item_type::BOX || t == df::enums::item_type::BAR)
        return false;

    if (! is_metal(item)) return false;

    for (auto g = item->general_refs.begin(); g != item->general_refs.end(); g++) 
    {
        switch ((*g)->getType()) 
        {
        case general_ref_type::CONTAINS_ITEM:
        case general_ref_type::UNIT_HOLDER:
        case general_ref_type::CONTAINS_UNIT:
            return false;
        case general_ref_type::CONTAINED_IN_ITEM:
            {
                df::item* c = (*g)->getItem();
                for (auto gg = c->general_refs.begin(); gg != c->general_refs.end(); gg++)
                {
                    if ((*gg)->getType() == general_ref_type::UNIT_HOLDER)
                        return false;
                }
            }
            break;
        }
    }

    if (item->getQuality() >= item_quality::Masterful) return false;

    return true;
}

void melt (df::item* item)
{
    item->flags.bits.melt = true;

    bool found = false;
    insert_into_vector(world->items.other[items_other_id::ANY_MELT_DESIGNATED], &df::item::id, item);
}

static void process(color_ostream &out)
{
    CoreSuspender suspend;

    std::vector<df::item*> to_melt;

    for (auto i = world->items.other[items_other_id::IN_PLAY].begin(); i != world->items.other[items_other_id::IN_PLAY].end(); i++)
    {
        if (is_meltable(*i))
            to_melt.push_back(*i);
    }

    for (auto m = to_melt.begin(); m != to_melt.end(); m++)
    {
        std::string d = Items::getDescription(*m, 0, true);
        MaterialInfo mat(*m);
        std::string ms = mat.toString();
        out.print("Marking %s to melt (material %s)\n", d.c_str(), ms.c_str());
        melt(*m);
    }

    out.print("%d items marked for melting.\n", to_melt.size());
}

static void clear (color_ostream &out)
{
    CoreSuspender suspend;

    int count = 0;
    auto l = &(world->items.other[items_other_id::ANY_MELT_DESIGNATED]);
    while (! (l->empty())) 
    {
        df::item* i = l->back();
        i->flags.bits.melt = false;
        count++;
        l->pop_back();
    }
    out.print("%d items unmarked for melting.\n", count);
}

static int last_run_year = 0;
static int last_run_tick = 0;

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (world) {
        commands.push_back(
            PluginCommand("automelt", "Derp.",
            automelt, false,
            "Derp.\n"
            )
            );
    }
    last_run_year = last_run_tick = 0;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (!enabled)
        return CR_OK;

    static unsigned cnt = 0;

    if (last_run_year != *df::global::cur_year ||
        last_run_tick + 1200 < *df::global::cur_year_tick)
    {
        process(out);
        last_run_year = *df::global::cur_year;
        last_run_tick = *df::global::cur_year_tick;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    enabled = enable;
    return CR_OK;
}

static command_result automelt(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    if (parameters.size() == 1) {
        if (parameters[0] == "enable")
            enabled = true;
        else if (parameters[0] == "disable")
            enabled = false;
        else if (parameters[0] == "clear") 
            clear(out);
        else if (parameters[0] == "once")
            process(out);
        else 
            return CR_WRONG_USAGE;
    } else {
        out << "Plugin " << (enabled ? "enabled" : "disabled") << "." << endl;
    }
    return CR_OK;
}

