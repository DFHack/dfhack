// Catsplosion
// By Zhentar , Further modified by dark_rabite, peterix, belal
// This work of evil makes animals pregnant
// and due within 2 in-game hours...

#include <iostream>
#include <cstdlib>
#include <assert.h>
#include <climits>
#include <stdlib.h> // for rand()
#include <algorithm> // for std::transform
#include <vector>
#include <list>
#include <iterator>
using namespace std;

#include "DFHack.h"
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"
#include <df/caste_raw.h>
#include <df/creature_raw.h>
#include <df/unit_genes.h>

using namespace DFHack;

command_result catsplosion (color_ostream &out, std::vector <std::string> & parameters);

DFHACK_PLUGIN("catsplosion");

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "catsplosion", "Make cats just /multiply/.",
        catsplosion, false,
        "  Makes cats abnormally abundant, if you provide some base population ;)\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
   return CR_OK;
}

command_result catsplosion (color_ostream &out, std::vector <std::string> & parameters)
{
    list<string> s_creatures;
    // only cats for now.
    s_creatures.push_back("CAT");
    // make the creature list unique ... with cats. they are always unique
    s_creatures.unique();
    // SUSPEND THE CORE! ::Evil laugh::
    CoreSuspender susp;

    uint32_t numCreatures;
    if(!(numCreatures = Units::getNumCreatures()))
    {
        cerr << "Can't get any creatures." << endl;
        return CR_FAILURE;
    }

    int totalcount=0;
    int totalchanged=0;
    int totalcreated=0;
    string sextype;

    // shows all the creatures and returns.

    int maxlength = 0;
    map<string, vector <df::unit *> > male_counts;
    map<string, vector <df::unit *> > female_counts;

    // classify
    for(uint32_t i =0;i < numCreatures;i++)
    {
        df::unit * creature = Units::GetCreature(i);
        df::creature_raw *raw = df::global::world->raws.creatures.all[creature->race];
        if(creature->sex == 0) // female
        {
            female_counts[raw->creature_id].push_back(creature);
            male_counts[raw->creature_id].size();
        }
        else // male, other, etc.
        {
            male_counts[raw->creature_id].push_back(creature);
            female_counts[raw->creature_id].size(); //auto initialize the females as well
        }
    }
    
    // print (optional)
    //if (showcreatures == 1)
    {
        out.print("Type                 Male # Female #\n");
        for(auto it1 = male_counts.begin();it1!=male_counts.end();it1++)
        {
            out.print("%20s %6d %8d\n", it1->first.c_str(), it1->second.size(), female_counts[it1->first].size());
        }
    }


    // process
    for (list<string>::iterator it = s_creatures.begin(); it != s_creatures.end(); ++it)
    {
        std::string clinput = *it;
        std::transform(clinput.begin(), clinput.end(), clinput.begin(), ::toupper);
        vector <df::unit *> &females = female_counts[clinput];
        uint32_t sz_fem = females.size();
        totalcount += sz_fem;
        for(uint32_t i = 0; i < sz_fem; i++)// max 1 pregnancy
        {
            df::unit * female = females[i];
            // accelerate
            if(female->relations.pregnancy_timer != 0)
            {
                female->relations.pregnancy_timer = rand() % 100 + 1;
                totalchanged++;
            }
            else if(!female->relations.pregnancy_genes)
            {
                df::unit_genes *preg = new df::unit_genes;
                preg->appearance = female->appearance.genes.appearance;
                preg->colors = female->appearance.genes.colors;
                female->relations.pregnancy_genes = preg;
                female->relations.pregnancy_timer = rand() % 100 + 1;
                female->relations.pregnancy_caste = 1;
                totalcreated ++;
            }
        }
    }
    if(totalchanged)
        out.print("%d pregnancies accelerated.\n", totalchanged);
    if(totalcreated)
        out.print("%d pregnancies created.\n", totalcreated);
    out.print("Total creatures checked: %d\n", totalcount);
    return CR_OK;
}
