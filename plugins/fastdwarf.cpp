// foo
// vi:expandtab:sw=4

#include <iostream>
#include <vector>
#include <map>
#include <stddef.h>
#include <assert.h>
#include <string.h>
using namespace std;
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <VersionInfo.h>
#include <modules/Units.h>
using namespace DFHack;

// dfhack interface
DFhackCExport const char * plugin_name ( void )
{
    return "fastdwarf";
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

static int enable_fastdwarf;

DFhackCExport command_result plugin_onupdate ( Core * c )
{
    if (!enable_fastdwarf)
        return CR_OK;
    df_unit *cre;
    DFHack::Units * cr = c->getUnits();
    static vector <df_unit*> *v = cr->creatures;
    uint32_t race = cr->GetDwarfRaceIndex();
    uint32_t civ = cr->GetDwarfCivId();
    if (!v)
    {
        c->con.printerr("Unable to locate creature vector. Fastdwarf cancelled.\n");
    }

    for (unsigned i=0 ; i<v->size() ; ++i)
    {
        cre = v->at(i);
        if (cre->race == race && cre->civ == civ && cre->job_counter > 0)
            cre->job_counter = 0;
        // could also patch the cre->current_job->counter
    }
    return CR_OK;
}

static command_result fastdwarf (Core * c, vector <string> & parameters)
{
    if (parameters.size() == 1 && (parameters[0] == "0" || parameters[0] == "1"))
    {
        if (parameters[0] == "0")
            enable_fastdwarf = 0;
        else
            enable_fastdwarf = 1;
        c->con.print("fastdwarf %sactivated.\n", (enable_fastdwarf ? "" : "de"));
    }
    else
    {
        c->con.print("Makes your minions move at ludicrous speeds.\nActivate with 'fastdwarf 1', deactivate with 'fastdwarf 0'.\nCurrent state: %d.\n", enable_fastdwarf);
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();

    commands.push_back(PluginCommand("fastdwarf",
                "enable/disable fastdwarf (parameter=0/1)",
                fastdwarf));

    return CR_OK;
}
