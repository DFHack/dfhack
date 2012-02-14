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

using namespace DFHack;
using namespace DFHack::Simple;

command_result catsplosion (Core * c, std::vector <std::string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "catsplosion";
}

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.clear();
    commands.push_back(PluginCommand(
        "catsplosion", "Do nothing, look pretty.",
        catsplosion, false,
        "  This command does nothing at all. For now.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
   return CR_OK;
}

typedef df::unit::T_relations::T_pregnancy_ptr pregstruct;
command_result catsplosion (Core * c, std::vector <std::string> & parameters)
{
    list<string> s_creatures;
    // only cats for now.
    s_creatures.push_back("CAT");
    // make the creature list unique ... with cats. they are always unique
    s_creatures.unique();
    // SUSPEND THE CORE! ::Evil laugh::
    CoreSuspender susp(c);

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
        c->con.print("Type                 Male # Female #\n");
        for(auto it1 = male_counts.begin();it1!=male_counts.end();it1++)
        {
            c->con.print("%20s %6d %8d\n", it1->first.c_str(), it1->second.size(), female_counts[it1->first].size());
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
            else if(!female->relations.pregnancy_ptr)
            {
                pregstruct * preg = new pregstruct;
                preg->anon_1 = female->appearance.unk_51c;
                preg->anon_2 = female->appearance.unk_524;
                female->relations.pregnancy_ptr = preg;
                female->relations.pregnancy_timer = rand() % 100 + 1;
                female->relations.pregnancy_mystery = 1; // WTF is this?
                totalcreated ++;
            }
        }
    }
    if(totalchanged)
        c->con.print("%d pregnancies accelerated.\n", totalchanged);
    if(totalcreated)
        c->con.print("%d pregnancies created.\n", totalcreated);
    c->con.print("Total creatures checked: %d\n", totalcount);
    return CR_OK;
}
