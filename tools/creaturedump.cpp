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
    
    DFHack::API DF("Memory.xml");
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
        if(!temp.trans_name.empty()){
            cout << ", trans name: " << temp.trans_name;
            addendl =true;
        }
        if(!temp.generic_name.empty()){
            cout << ", generic name: " << temp.generic_name;
            addendl=true;
        }
        if(addendl)
        {
            cout << endl;
            addendl = false;
        }
        if(!temp.profession.empty()){
            cout << ", profession: " << temp.profession;
            addendl = false;
        }
        if(!temp.custom_profession.empty()){
            cout << ", custom profession: " << temp.custom_profession;
            addendl = false;
        }
        if(!temp.current_job.empty()){
            cout << ", current job: " << temp.current_job;
            addendl = false;
        }
        if(addendl)
        {
            cout << endl;
            addendl = false;
        }
        cout << ", happiness: " << temp.happiness << ", strength: " << temp.strength << ", agility: " 
            << temp.agility << ", toughness: " << temp.toughness << ", money: " << temp.money << ", id: " << temp.id;
        if(temp.squad_leader_id != -1){
            cout << ", squad_leader_id: " << temp.squad_leader_id;
        }
        cout << ", sex";
        if(temp.sex == 0){
            cout << ", Female";
        }
        else{
            cout <<", Male";
        }
        cout << endl;

        //skills
        for(unsigned int i = 0; i < temp.skills.size();i++){
            if(i > 0){
                cout << ", ";
            }
            cout << temp.skills[i].name << ": " << temp.skills[i].rating;
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
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}