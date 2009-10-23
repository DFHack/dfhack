// Creature dump

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

template <typename T>
void print_bits ( T val, std::ostream& out )
{
    T n_bits = sizeof ( val ) * CHAR_BIT;
    
    for ( unsigned i = 0; i < n_bits; ++i ) {
        out<< !!( val & 1 ) << " ";
        val >>= 1;
    }
}


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
        cout << "flags1: ";
        print_bits(temp.flags1, cout);
        cout << endl << "flags2: ";
        print_bits(temp.flags2, cout);
        cout << endl << endl;
    }
    DF.FinishReadCreatures();
    delete pDF;
    return 0;
}