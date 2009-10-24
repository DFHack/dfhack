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
        
        /*
         * FLAGS 1
         */
        cout << "flags1: ";
        print_bits(temp.flags1.whole, cout);
        cout << endl;
        if(temp.flags1.bits.dead)
        {
            cout << "dead ";
        }
        if(temp.flags1.bits.unconscious)
        {
            cout << "unconscious ";
        }
        if(temp.flags1.bits.skeletal)
        {
            cout << "skeletal ";
        }
        if(temp.flags1.bits.zombie)
        {
            cout << "zombie ";
        }
        if(temp.flags1.bits.tame)
        {
            cout << "tame ";
        }
        if(temp.flags1.bits.royal_guard)
        {
            cout << "royal_guard ";
        }
        if(temp.flags1.bits.fortress_guard)
        {
            cout << "fortress_guard ";
        }
        /*
        * FLAGS 2
        */
        cout << endl << "flags2: ";
        print_bits(temp.flags2.whole, cout);
        cout << endl;
        if(temp.flags2.bits.dead)
        {
            cout << "dead! ";
        }
        if(temp.flags2.bits.flying)
        {
            cout << "flying ";
        }
        if(temp.flags2.bits.ground)
        {
            cout << "grounded ";
        }
        if(temp.flags2.bits.slaughter)
        {
            cout << "slaughter ";
        }
        if(temp.flags2.bits.underworld)
        {
            cout << "from the underworld ";
        }
        cout << endl << endl;
    }
    DF.FinishReadCreatures();
    delete pDF;
    return 0;
}