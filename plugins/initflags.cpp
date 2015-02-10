#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/d_init.h"

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("initflags");
REQUIRE_GLOBAL(d_init);

command_result twaterlvl(color_ostream &out, vector <string> & parameters);
command_result tidlers(color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (d_init) {
        commands.push_back(PluginCommand("twaterlvl", "Toggle display of water/magma depth.",
                                         twaterlvl, Gui::dwarfmode_hotkey));
        commands.push_back(PluginCommand("tidlers", "Toggle display of idlers.",
                                         tidlers, Gui::dwarfmode_hotkey));
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

command_result twaterlvl(color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    d_init->flags1.toggle(d_init_flags1::SHOW_FLOW_AMOUNTS);
    out << "Toggled the display of water/magma depth." << endl;
    return CR_OK;
}

command_result tidlers(color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    d_init->idlers = ENUM_NEXT_ITEM(d_init_idlers, d_init->idlers);
    out << "Toggled the display of idlers to " << ENUM_KEY_STR(d_init_idlers, d_init->idlers) << endl;
    return CR_OK;
}
