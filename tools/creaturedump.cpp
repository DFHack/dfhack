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
        bool addendl = false;
        if(!temp.first_name.empty())
        {
            cout << "first name: " << temp.first_name;
            addendl = true;
        }
        if(!temp.nick_name.empty())
        {
            cout << ", nick name: " << temp.nick_name;
            addendl = true;
        }
        if(addendl)
        {
            cout << endl;
        }
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
        if(temp.flags1.bits.on_ground)
        {
            cout << "on the ground, ";
        }
        if(temp.flags1.bits.skeleton)
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
        if(temp.flags2.bits.killed)
        {
            cout << "killed by kill function, ";
        }
        if(temp.flags2.bits.resident)
        {
            cout << "resident ";
        }
        if(temp.flags2.bits.gutted)
        {
            cout << "gutted ";
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
    DF.Detach();
    delete pDF;
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}