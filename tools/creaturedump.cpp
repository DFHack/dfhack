// Creature dump

#include <iostream>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main (void)
{
    vector<t_matgloss> creaturestypes;
    
    DFHackAPI *pDF = CreateDFHackAPI("Memory.xml");
    DFHackAPI &DF = *pDF;
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    
    // get stone matgloss mapping
    if(!DF.ReadCreatureMatgloss(creaturestypes))
    {
        cerr << "Can't get the creature types." << endl;
        return 1; 
    }
    
    uint32_t numCreatures = DF.InitReadCreatures();
    for(uint32_t i = 0; i < numCreatures; i++)
    {
        t_creature temp;
        DF.ReadCreature(i, temp);
        cout << "creature type " << creaturestypes[temp.type].id << ", position:" << temp.x << " " << temp.y << " "<< temp.z << endl;
    }
    DF.FinishReadCreatures();
    delete pDF;
    return 0;
}