#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/unit.h"

using std::string;
using std::vector;
using namespace DFHack;

using df::global::world;
using df::global::ui;

// dfhack interface
DFHACK_PLUGIN("fastdwarf");

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

static int enable_fastdwarf = false;

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    // check run conditions
    if(!world || !world->map.block_index || !enable_fastdwarf)
    {
        // give up if we shouldn't be running'
        return CR_OK;
    }
    int32_t race = ui->race_id;
    int32_t civ = ui->civ_id;

    for (size_t i = 0; i < world->units.all.size(); i++)
    {
        df::unit *unit = world->units.all[i];

        if (unit->race == race && unit->civ_id == civ && unit->counters.job_counter > 0)
            unit->counters.job_counter = 0;
        // could also patch the unit->job.current_job->completion_timer
    }
    return CR_OK;
}

static command_result fastdwarf (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() == 1 && (parameters[0] == "0" || parameters[0] == "1"))
    {
        if (parameters[0] == "0")
            enable_fastdwarf = 0;
        else
            enable_fastdwarf = 1;
        out.print("fastdwarf %sactivated.\n", (enable_fastdwarf ? "" : "de"));
    }
    else
    {
        out.print("Makes your minions move at ludicrous speeds.\n"
            "Activate with 'fastdwarf 1', deactivate with 'fastdwarf 0'.\n"
            "Current state: %d.\n", enable_fastdwarf);
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("fastdwarf",
        "enable/disable fastdwarf (parameter=0/1)",
        fastdwarf));

    return CR_OK;
}
