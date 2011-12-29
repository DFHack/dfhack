#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>

#include <dfhack/DataDefs.h>
#include <dfhack/df/ui.h>
#include <dfhack/df/world.h>
#include <dfhack/df/squad.h>

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::ui;
using df::global::world;

static command_result rename(Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "rename";
}

DFhackCExport command_result plugin_init (Core *c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    if (world && ui) {
        commands.push_back(PluginCommand("rename", "Rename various things.", rename));
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

static command_result usage(Core *c)
{
    c->con << "Usage:" << endl
           << "  rename squad <index> \"name\"" << endl
           << "  rename hotkey <index> \"name\"" << endl;
    return CR_OK;
}

static command_result rename(Core * c, vector <string> &parameters)
{
    CoreSuspender suspend(c);

    string cmd;
    if (!parameters.empty())
        cmd = parameters[0];

    if (cmd == "squad") {
        if (parameters.size() != 3)
            return usage(c);
        
        std::vector<df::squad*> &squads = world->squads.all;

        int id = atoi(parameters[1].c_str());
        if (id < 1 || id > squads.size()) {
            c->con.printerr("Invalid squad index\n");
            return usage(c);
        }

        squads[id-1]->alias = parameters[2];
    } else if (cmd == "hotkey") {
        if (parameters.size() != 3)
            return usage(c);

        int id = atoi(parameters[1].c_str());
        if (id < 1 || id > 16) {
            c->con.printerr("Invalid hotkey index\n");
            return usage(c);
        }

        ui->main.hotkeys[id-1].name = parameters[2];
    } else {
        if (!parameters.empty() && cmd != "?")
            c->con.printerr("Invalid command: %s\n", cmd.c_str());
        return usage(c);
    }

    return CR_OK;
}
