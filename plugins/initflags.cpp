#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <DataDefs.h>
#include <df/d_init.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::d_init;

DFhackCExport command_result twaterlvl(Core * c, vector <string> & parameters);
DFhackCExport command_result tidlers(Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "initflags";
}

DFhackCExport command_result plugin_init (Core *c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    if (d_init) {
        commands.push_back(PluginCommand("twaterlvl", "Toggle display of water/magma depth.", twaterlvl));
        commands.push_back(PluginCommand("tidlers", "Toggle display of idlers.", tidlers));
    }
    std::cerr << "d_init: " << sizeof(df::d_init) << endl;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result twaterlvl(Core * c, vector <string> & parameters)
{
    c->Suspend();
    df::global::d_init->flags1.toggle(d_init_flags1::SHOW_FLOW_AMOUNTS);
    c->con << "Toggled the display of water/magma depth." << endl;
    c->Resume();
    return CR_OK;
}

DFhackCExport command_result tidlers(Core * c, vector <string> & parameters)
{
    c->Suspend();
    df::d_init_idlers iv = df::d_init_idlers(int(d_init->idlers) + 1);
    if (!d_init_idlers::is_valid(iv))
        iv = ENUM_FIRST_ITEM(d_init_idlers);
    d_init->idlers = iv;
    c->con << "Toggled the display of idlers to " << ENUM_KEY_STR(d_init_idlers, iv) << endl;
    c->Resume();
    return CR_OK;
}
