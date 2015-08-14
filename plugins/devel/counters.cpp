// Show creature counter values

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Gui.h"
#include "df/unit.h"
#include "df/unit_misc_trait.h"

using std::vector;
using std::string;

using namespace DFHack;
using namespace df::enums;

command_result df_counters (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    df::unit *unit = Gui::getSelectedUnit(out);
    if (!unit)
        return CR_WRONG_USAGE;
    auto &counters = unit->status.misc_traits;
    for (size_t i = 0; i < counters.size(); i++)
    {
        auto counter = counters[i];
        out.print("%i (%s): %i\n", counter->id, ENUM_KEY_STR(misc_trait_type, counter->id).c_str(), counter->value);
    }

    return CR_OK;
}

DFHACK_PLUGIN("counters");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("counters",
                                     "Display counters for currently selected creature",
                                     df_counters));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
