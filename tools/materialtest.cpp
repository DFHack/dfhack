// Attach test
// attachtest - 100x attach/detach, 100x reads, 100x writes

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <ctime>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main (void)
{
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    vector <DFHack::t_matgloss> Plants;
    DF.ReadPlantMatgloss(Plants);
    
    vector <DFHack::t_matgloss> Metals;
    DF.ReadMetalMatgloss(Metals);
    
    vector <DFHack::t_matgloss> Stones;
    DF.ReadStoneMatgloss(Stones);
    
    vector <DFHack::t_matgloss> CreatureTypes;
    DF.ReadCreatureMatgloss(CreatureTypes);
    
    cout << "Plant: " << Plants[0].id << endl;
    cout << "Metal: " << Metals[0].id << endl;
    cout << "Stone: " << Stones[0].id << endl;
    cout << "Creature: " << CreatureTypes[0].id << endl;
    
    DF.Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    
    return 0;
}
