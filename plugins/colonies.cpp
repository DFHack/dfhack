#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include <vector>
#include <string>
#include "modules/Vermin.h"
#include "modules/Materials.h"

using std::vector;
using std::string;
using namespace DFHack;

command_result colonies (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("colonies");
REQUIRE_GLOBAL(world);  // used by Materials

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "colonies", "List or change wild colonies (ants hills and such)",
        colonies, false,
        "  Without any options, this command lists all the vermin colonies present.\n"
        "Options:\n"
        //"  kill   - destroy colonies\n" // unlisted because it's likely broken anyway
        "  bees   - turn colonies into honey bee hives\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

void destroyColonies();
void convertColonies(Materials *Materials);
void showColonies(color_ostream &out, Materials *Materials);

command_result colonies (color_ostream &out, vector <string> & parameters)
{
    bool destroy = false;
    bool convert = false;

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "kill")
            destroy = true;
        else if(parameters[i] == "bees")
            convert = true;
        else
            return CR_WRONG_USAGE;
    }
    if (destroy && convert)
    {
        out.printerr("Kill or make bees? DECIDE!\n");
        return CR_FAILURE;
    }

    CoreSuspender suspend;

    Materials * materials = Core::getInstance().getMaterials();

    materials->ReadCreatureTypesEx();

    if (destroy)
        destroyColonies();
    else if (convert)
        convertColonies(materials);
    else
        showColonies(out, materials);

    materials->Finish();

    return CR_OK;
}

//FIXME: this is probably bullshit
void destroyColonies()
{
    uint32_t numSpawnPoints = Vermin::getNumVermin();
    for (uint32_t i = 0; i < numSpawnPoints; i++)
    {
        Vermin::t_vermin sp;
        Vermin::Read(i, sp);

        if (sp.visible && sp.is_colony)
        {
            sp.visible = false;
            Vermin::Write(i, sp);
        }
    }
}

// Convert all colonies to honey bees.
void convertColonies(Materials *Materials)
{
    int bee_idx = -1;
    for (size_t i = 0; i < Materials->raceEx.size(); i++)
    {
        if (Materials->raceEx[i].id == "HONEY_BEE")
        {
            bee_idx = i;
            break;
        }
    }

    if (bee_idx == -1)
    {
        std::cerr << "Honey bees not present in game." << std::endl;
        return;
    }

    uint32_t numSpawnPoints = Vermin::getNumVermin();
    for (uint32_t i = 0; i < numSpawnPoints; i++)
    {
        Vermin::t_vermin sp;
        Vermin::Read(i, sp);

        if (sp.visible && sp.is_colony)
        {
            sp.race = bee_idx;
            Vermin::Write(i, sp);
        }
    }
}

void showColonies(color_ostream &out, Materials *Materials)
{
    uint32_t numSpawnPoints = Vermin::getNumVermin();
    int      numColonies    = 0;
    for (uint32_t i = 0; i < numSpawnPoints; i++)
    {
        Vermin::t_vermin sp;

        Vermin::Read(i, sp);

        if (sp.visible && sp.is_colony)
        {
            numColonies++;
            string race="(no race)";
            if(sp.race != -1)
                race = Materials->raceEx[sp.race].id;

            out.print("Colony %u: %s at %d:%d:%d\n", i,
                      race.c_str(), sp.x, sp.y, sp.z);
        }
    }

    if (numColonies == 0)
        out << "No colonies present." << std::endl;
}
