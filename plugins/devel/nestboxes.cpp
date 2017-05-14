#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/building_nest_boxst.h"
#include "df/building_type.h"
#include "df/global_objects.h"
#include "df/item.h"
#include "df/unit.h"
#include "df/building.h"
#include "df/items_other_id.h"
#include "df/creature_raw.h"
#include "modules/MapCache.h"
#include "modules/Items.h"


using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;

static command_result nestboxes(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("nestboxes");

DFHACK_PLUGIN_IS_ENABLED(enabled);

static void eggscan(color_ostream &out)
{
    CoreSuspender suspend;

    for (int i = 0; i < world->buildings.all.size(); ++i)
    {
        df::building *build = world->buildings.all[i];
        auto type = build->getType();
        if (df::enums::building_type::NestBox == type)
        {
            bool fertile = false;
            df::building_nest_boxst *nb = virtual_cast<df::building_nest_boxst>(build);
            if (nb->claimed_by != -1)
            {
                df::unit* u = df::unit::find(nb->claimed_by);
                if (u && u->pregnancy_timer > 0)
                    fertile = true;
            }
            for (int j = 1; j < nb->contained_items.size(); j++)
            {
                df::item* item = nb->contained_items[j]->item;
                if (item->flags.bits.forbid != fertile)
                {
                    item->flags.bits.forbid = fertile;
                    out << item->getStackSize() << " eggs " << (fertile ? "forbidden" : "unforbidden.") << endl;
                }
            }
        }
    }
}


DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (world && ui) {
        commands.push_back(
            PluginCommand("nestboxes", "Derp.",
                nestboxes, false,
                "Derp.\n"
            )
        );
    }
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
    if ((++cnt % 5) != 0)
        return CR_OK;

    eggscan(out);

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    enabled = enable;
    return CR_OK;
}

static command_result nestboxes(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;
    bool clean = false;
    int dump_count = 0;
    int good_egg = 0;

    if (parameters.size() == 1) {
        if (parameters[0] == "enable")
            enabled = true;
        else if (parameters[0] == "disable")
            enabled = false;
        else
            return CR_WRONG_USAGE;
    } else {
        out << "Plugin " << (enabled ? "enabled" : "disabled") << "." << endl;
    }
    return CR_OK;
}

