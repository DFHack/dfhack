// foo
// vi:expandtab:sw=4

#include <iostream>
#include <vector>
#include <map>
#include <stddef.h>
#include <assert.h>
#include <string.h>
using namespace std;
#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/VersionInfo.h>
#include <dfhack/modules/Creatures.h>
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

// remove that if struct df_creature is updated
#define job_counter unk_540

DFhackCExport command_result plugin_onupdate ( Core * c )
{
    if (!enable_fastdwarf)
        return CR_OK;

    static vector <df_creature*> *v;
    df_creature *cre;

    if (!v) {
        OffsetGroup *ogc = c->vinfo->getGroup("Creatures");
        v = (vector<df_creature*>*)ogc->getAddress("vector");
    }

    //c->Suspend(); // will deadlock in onupdate
    for (unsigned i=0 ; i<v->size() ; ++i) {
        cre = v->at(i);
        if (cre->race == 241 && cre->job_counter > 0)
            cre->job_counter = 0;
	// could also patch the cre->current_job->counter
    }
    //c->Resume();

    return CR_OK;
}

static command_result fastdwarf (Core * c, vector <string> & parameters)
{
    if (parameters.size() == 1 && (parameters[0] == "0" || parameters[0] == "1")) {
        if (parameters[0] == "0")
            enable_fastdwarf = 0;
        else
            enable_fastdwarf = 1;
        c->con.print("fastdwarf %sactivated.\n", (enable_fastdwarf ? "" : "de"));
    } else {
        c->con.print("Activate fastdwarf with 'fastdwarf 1', deactivate with 'fastdwarf 0'.\nCurrent state: %d.\n", enable_fastdwarf);
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
