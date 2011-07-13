#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>
#include <dfhack/modules/Vermin.h>

using std::vector;
using std::string;
using namespace DFHack;
#include <DFHack.h>

DFhackCExport command_result colonies (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "colonies";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("colonies",
               "List or change wild colonies (ants hills and such)\
\n              Options: 'kill' = destroy all colonies\
\n                       'bees' = change all colonies to honey bees",
                colonies));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

void destroyColonies(DFHack::SpawnPoints *points);
void convertColonies(DFHack::SpawnPoints *points, DFHack::Materials *Materials);
void showColonies(DFHack::SpawnPoints *points, DFHack::Materials *Materials);

DFhackCExport command_result colonies (Core * c, vector <string> & parameters)
{
    bool destroy = false;
    bool convert = false;

    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "kill")
            destroy = true;
        else if(parameters[i] == "bees")
            convert = true;
    }

    if (destroy && convert)
    {

        dfout << "Kill or make bees? DECIDE!" << std::endl;
        return CR_FAILURE;
    }
 
    c->Suspend();

    Vermin    * vermin    = c->getVermin();
    Materials * materials = c->getMaterials();

    SpawnPoints *points = vermin->getSpawnPoints();

    if(!points->isValid())
    {
        std::cerr << "vermin not supported for this DF version" << std::endl;
        return CR_FAILURE;
    }

    materials->ReadCreatureTypesEx();

    if (destroy)
        destroyColonies(points);
    else if (convert)
        convertColonies(points, materials);
    else
        showColonies(points, materials);

    delete points;

    vermin->Finish();
    materials->Finish();

    c->Resume();
    return CR_OK;
}

void destroyColonies(DFHack::SpawnPoints *points)
{
    uint32_t numSpawnPoints = points->size();
    for (uint32_t i = 0; i < numSpawnPoints; i++)
    {
        DFHack::t_spawnPoint sp;
        points->Read(i, sp);

        if (sp.in_use && DFHack::SpawnPoints::isWildColony(sp))
        {
            sp.in_use = false;
            points->Write(i, sp);
        }
    }
}

// Convert all colonies to honey bees.
void convertColonies(DFHack::SpawnPoints *points, DFHack::Materials *Materials)
{
    int bee_idx = -1;
    for (size_t i = 0; i < Materials->raceEx.size(); i++)
        if (strcmp(Materials->raceEx[i].rawname, "HONEY_BEE") == 0)
        {
            bee_idx = i;
            break;
        }

    if (bee_idx == -1)
    {
        std::cerr << "Honey bees not present in game." << std::endl;
        return;
    }

    uint32_t numSpawnPoints = points->size();
    for (uint32_t i = 0; i < numSpawnPoints; i++)
    {
        DFHack::t_spawnPoint sp;
        points->Read(i, sp);

        if (sp.in_use && DFHack::SpawnPoints::isWildColony(sp))
        {
            sp.race = bee_idx;
            points->Write(i, sp);
        }
    }
}

void showColonies(DFHack::SpawnPoints *points, DFHack::Materials *Materials)
{
    uint32_t numSpawnPoints = points->size();
    int      numColonies    = 0;
    for (uint32_t i = 0; i < numSpawnPoints; i++)
    {
        DFHack::t_spawnPoint sp;

        points->Read(i, sp);

        if (sp.in_use && DFHack::SpawnPoints::isWildColony(sp))
        {
            numColonies++;
            string race="(no race)";
            if (Materials->raceEx[sp.race].rawname[0])
                race = Materials->raceEx[sp.race].rawname;

             fprintf(dfout_C, "Spawn point %u: %s at %d:%d:%d\n",
                   i, race.c_str(), sp.x, sp.y, sp.z);
        }
    }

    if (numColonies == 0)
        dfout << "No colonies present." << std::endl;
}
