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
#include <DFHackAPI.h>
#include <DFMemInfo.h>
#include <DFProcess.h>
#include <argstream/argstream.h>


vector<DFHack::t_matgloss> creaturestypes;
DFHack::memory_info *mem;
DFHack::Process *proc;
uint32_t creature_pregnancy_offset;

bool femaleonly = 0;
bool showcreatures = 0;
int maxpreg = 1000; // random start value, since its not required and im not sure how to set it to infinity
list<string> s_creatures;

int main ( int argc, char** argv )
{
    // parse input, handle this nice and neat before we get to the connecting
    argstream as(argc,argv);
    as  >>option('f',"female",femaleonly,"Impregnate females only")
        >>option('s',"show",showcreatures,"Show creature list (read only)")
        >>parameter('m',"max",maxpreg,"The maximum limit of pregnancies ", false)
        >>values<string>(back_inserter(s_creatures), "any number of creatures")
        >>help();

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
        cout << as.usage();
        return(1);
    }

    DFHack::API DF("Memory.xml");
    try
    {
        DF.Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    proc = DF.getProcess();
    mem = DF.getMemoryInfo();
    creature_pregnancy_offset = mem->getOffset("creature_pregnancy");

    if(!DF.ReadCreatureMatgloss(creaturestypes))
    {
        cerr << "Can't get the creature types." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    uint32_t numCreatures;
    if(!DF.InitReadCreatures(numCreatures))
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
    if (showcreatures == 1)
    {
        int maxlength = 0;
        map<string,uint32_t> male_counts;
        map<string,uint32_t> female_counts;
        for(uint32_t i =0;i < numCreatures;i++)
        {
            DFHack::t_creature creature;
            DF.ReadCreature(i,creature);
            if(creature.sex == 1){
                male_counts[creaturestypes[creature.type].id]++;
                female_counts[creaturestypes[creature.type].id]+=0; //auto initialize the females as well
            }
            else{
                female_counts[creaturestypes[creature.type].id]++;
                male_counts[creaturestypes[creature.type].id]+=0;
            }
        }
        cout << "Type\t\t\tMale #\tFemale #" << endl;
        for(map<string, uint32_t>::iterator it1 = male_counts.begin();it1!=male_counts.end();it1++)
        {
            cout << it1->first << "\t\t" << it1->second << "\t" << female_counts[it1->first] << endl;
        }
        return(1);
    }

    for(uint32_t i = 0; i < numCreatures && totalchanged != maxpreg; i++)
    {
        DFHack::t_creature creature;
        DF.ReadCreature(i,creature);
        if (showcreatures == 1)
        {
            if (creature.sex == 0) { sextype = "Female"; } else { sextype = "Male";}
            cout << string(creaturestypes[creature.type].id) << ":" << sextype << "" << endl;
        }
        else
        {
            s_creatures.unique();
            for (list<string>::iterator it = s_creatures.begin(); it != s_creatures.end(); ++it)
            {
                std::string clinput = *it;
                std::transform(clinput.begin(), clinput.end(), clinput.begin(), ::toupper);
                if(string(creaturestypes[creature.type].id) == clinput)
                {
                    if((femaleonly == 1 && creature.sex == 0) || (femaleonly != 1))
                    {
                        proc->writeDWord(creature.origin + creature_pregnancy_offset, rand() % 100 + 1);
                        totalchanged+=1;
                        totalcount+=1;
                    }
                    else
                    {
                        totalcount+=1;
                    }
                }
            }
        }
    }

    cout << totalchanged << " animals impregnated out of a possible " << totalcount << "." << endl;

    DF.FinishReadCreatures();
    DF.Detach();
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}