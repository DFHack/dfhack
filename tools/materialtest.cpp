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
    DFHackAPI *pDF = CreateDFHackAPI("Memory.xml");
    DFHackAPI &DF = *pDF;
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    vector <t_matgloss> Plants;
    DF.ReadPlantMatgloss(Plants);
    
    vector <t_matgloss> Metals;
    DF.ReadMetalMatgloss(Metals);
    
    vector <t_matgloss> Stones;
    DF.ReadStoneMatgloss(Stones);
    
    vector <t_matgloss> CreatureTypes;
    DF.ReadCreatureMatgloss(CreatureTypes);
    
    cout << "Plant: " << Plants[0].id << endl;
    cout << "Metal: " << Metals[0].id << endl;
    cout << "Stone: " << Stones[0].id << endl;
    cout << "Creature: " << CreatureTypes[0].id << endl;
    
    DF.Detach();
    delete pDF;
    
    cin.ignore();
    
    return 0;
}