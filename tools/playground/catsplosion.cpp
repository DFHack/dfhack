// Catsplosion
// By Zhentar , Further modified by dark_rabite, peterix, belal
// This work of evil makes animals pregnant
// and due within 2 in-game hours...

#include <iostream>
#include <integers.h>
#include <cstdlib>
#include <assert.h>
#include <climits>
#include <stdlib.h> // for rand()
#include <algorithm> // for std::transform
#include <vector>
#include <list>
#include <iterator>
using namespace std;

#include <DFError.h>
#include <DFTypes.h>
#include <DFContextManager.h>
#include <DFContext.h>
#include <DFMemInfo.h>
#include <DFProcess.h>
#include <argstream.h>
#include <modules/Materials.h>
#include <modules/Creatures.h>

using namespace DFHack;

int main ( int argc, char** argv )
{
    DFHack::memory_info *mem;
    DFHack::Process *proc;
    uint32_t creature_pregnancy_offset;
    
    //bool femaleonly = 0;
    bool showcreatures = 0;
    int maxpreg = 1000; // random start value, since its not required and im not sure how to set it to infinity
    list<string> s_creatures;
    
    // parse input, handle this nice and neat before we get to the connecting
    argstream as(argc,argv);
    as // >>option('f',"female",femaleonly,"Impregnate females only")
        >>option('s',"show",showcreatures,"Show creature list (read only)")
        >>parameter('m',"max",maxpreg,"The maximum limit of pregnancies ", false)
        >>values<string>(back_inserter(s_creatures), "any number of creatures")
        >>help();
        
    // make the creature list unique
    s_creatures.unique();
    
    if (!as.isOk())
    {
        cout << as.errorLog();
        return(0);
    }
    else if (as.helpRequested())
    {
        cout<<as.usage()<<endl;
        return(1);
    }
    else if(showcreatures==1)
    {
    }
    else if (s_creatures.size() == 0 && showcreatures != 1)
    {
        cout << as.usage() << endl << "---------------------------------------" << endl;
        cout << "Creature list empty, assuming CATs" << endl;
        s_creatures.push_back("CAT");
    }

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    proc = DF->getProcess();
    mem = DF->getMemoryInfo();
    DFHack::Materials *Mats = DF->getMaterials();
    DFHack::Creatures *Cre = DF->getCreatures();
    creature_pregnancy_offset = mem->getOffset("creature_pregnancy");

    if(!Mats->ReadCreatureTypesEx())
    {
        cerr << "Can't get the creature types." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    uint32_t numCreatures;
    if(!Cre->Start(numCreatures))
    {
        cerr << "Can't get creatures" << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    int totalcount=0;
    int totalchanged=0;
    string sextype;

    // shows all the creatures and returns.

    int maxlength = 0;
    map<string, vector <t_creature> > male_counts;
    map<string, vector <t_creature> > female_counts;
    
    // classify
    for(uint32_t i =0;i < numCreatures;i++)
    {
        DFHack::t_creature creature;
        Cre->ReadCreature(i,creature);
        DFHack::t_creaturetype & crt = Mats->raceEx[creature.race];
        string castename = crt.castes[creature.sex].rawname;
        if(castename == "FEMALE")
        {
            female_counts[Mats->raceEx[creature.race].rawname].push_back(creature);
            male_counts[Mats->raceEx[creature.race].rawname].size();
        }
        else // male, other, etc.
        {
            male_counts[Mats->raceEx[creature.race].rawname].push_back(creature);
            female_counts[Mats->raceEx[creature.race].rawname].size(); //auto initialize the females as well
        }
    }
    
    // print (optional)
    if (showcreatures == 1)
    {
        cout << "Type\t\tMale #\tFemale #" << endl;
        for(map<string, vector <t_creature> >::iterator it1 = male_counts.begin();it1!=male_counts.end();it1++)
        {
            cout << it1->first << "\t\t" << it1->second.size() << "\t" << female_counts[it1->first].size() << endl;
        }
    }
    
    // process
    for (list<string>::iterator it = s_creatures.begin(); it != s_creatures.end(); ++it)
    {
        std::string clinput = *it;
        std::transform(clinput.begin(), clinput.end(), clinput.begin(), ::toupper);
        vector <t_creature> &females = female_counts[clinput];
        uint32_t sz_fem = females.size();
        totalcount += sz_fem;
        for(uint32_t i = 0; i < sz_fem && totalchanged != maxpreg; i++)
        {
            t_creature & female = females[i];
            uint32_t preg_timer = proc->readDWord(female.origin + creature_pregnancy_offset);
            if(preg_timer != 0)
            {
                proc->writeDWord(female.origin + creature_pregnancy_offset, rand() % 100 + 1);
                totalchanged++;
            }
        }
    }

    cout << totalchanged << " pregnancies accelerated. Total creatures checked: " << totalcount << "." << endl;
    Cre->Finish();
    DF->Detach();
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
