// Creature dump

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFError.h>
#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFMemInfo.h>
#include <modules/Materials.h>
#include <modules/Creatures.h>

template <typename T>
void print_bits ( T val, std::ostream& out )
{
    T n_bits = sizeof ( val ) * CHAR_BIT;
    
    for ( unsigned i = 0; i < n_bits; ++i ) {
        out<< !!( val & 1 ) << " ";
        val >>= 1;
    }
}
struct matGlosses 
{
    vector<DFHack::t_matglossPlant> plantMat;
    vector<DFHack::t_matgloss> woodMat;
    vector<DFHack::t_matgloss> stoneMat;
    vector<DFHack::t_matgloss> metalMat;
    vector<DFHack::t_matgloss> creatureMat;
};
enum likeType
{
    FAIL = 0,
    MATERIAL = 1,
    ITEM = 2,
    FOOD = 3
};

vector<DFHack::t_matgloss> creaturestypes;
matGlosses mat;
vector< vector <DFHack::t_itemType> > itemTypes;
DFHack::memory_info *mem;
vector< vector<string> > englishWords;
vector< vector<string> > foreignWords;
/*
likeType printLike40d(DFHack::t_like like, const matGlosses & mat,const vector< vector <DFHack::t_itemType> > & itemTypes)
{ // The function in DF which prints out the likes is a monster, it is a huge switch statement with tons of options and calls a ton of other functions as well, 
    //so I am not going to try and put all the possibilites here, only the low hanging fruit, with stones and metals, as well as items,
    //you can easily find good canidates for military duty for instance
    //The ideal thing to do would be to call the df function directly with the desired likes, the df function modifies a string, so it should be possible to do...
    if(like.active){
        if(like.type ==0){
            switch (like.material.type)
            {
            case 0:
                cout << mat.woodMat[like.material.index].name;
                return(MATERIAL);
            case 1:
                cout << mat.stoneMat[like.material.index].name;
                return(MATERIAL);
            case 2:
                cout << mat.metalMat[like.material.index].name;
                return(MATERIAL);
            case 12: // don't ask me why this has such a large jump, maybe this is not actually the matType for plants, but they all have this set to 12
                cout << mat.plantMat[like.material.index].name;
                return(MATERIAL);
            case 32:
                cout << mat.plantMat[like.material.index].name;
                return(MATERIAL);
            case 121:
                cout << mat.creatureMat[like.material.index].name;
                return(MATERIAL);
            default:
                return(FAIL);
            }
        }
        else if(like.type == 4 && like.itemIndex != -1){
            switch(like.itemClass)
            {
            case 24:
                cout << itemTypes[0][like.itemIndex].name;
                return(ITEM);
            case 25:
                cout << itemTypes[4][like.itemIndex].name;
                return(ITEM);
            case 26:
                cout << itemTypes[8][like.itemIndex].name;
                return(ITEM);
            case 27:
                cout << itemTypes[9][like.itemIndex].name;
                return(ITEM);
            case 28:
                cout << itemTypes[10][like.itemIndex].name;
                return(ITEM);
            case 29:
                cout << itemTypes[7][like.itemIndex].name;
                return(ITEM);
            case 38:
                cout << itemTypes[5][like.itemIndex].name;
                return(ITEM);
            case 63:
                cout << itemTypes[11][like.itemIndex].name;
                return(ITEM);
            case 68:
            case 69:
                cout << itemTypes[6][like.itemIndex].name;
                return(ITEM);
            case 70:
                cout << itemTypes[1][like.itemIndex].name;
                return(ITEM);
            default:
          //      cout << like.itemClass << ":" << like.itemIndex;
                return(FAIL);
            }
        }
        else if(like.material.type != -1){// && like.material.index == -1){
            if(like.type == 2){
                switch(like.itemClass)
                {
                case 52:
                case 53:
                case 58:
                    cout << mat.plantMat[like.material.type].name;
                    return(FOOD);
                case 72:
                    if(like.material.type =! 10){ // 10 is for milk stuff, which I don't know how to do
                        cout << mat.plantMat[like.material.index].extract_name;
                        return(FOOD);
                    }
                    return(FAIL);
                case 74:
                    cout << mat.plantMat[like.material.index].drink_name;
                    return(FOOD);
                case 75:
                    cout << mat.plantMat[like.material.index].food_name;
                    return(FOOD);
                case 47:
                case 48:
                    cout << mat.creatureMat[like.material.type].name;
                    return(FOOD);
                default:
                    return(FAIL);
                }
            }
        }
    }
    return(FAIL);
}
*/

void printCreature(DFHack::API & DF, const DFHack::t_creature & creature)
{
        cout << "address: " << hex <<  creature.origin << dec << " creature type: " << creaturestypes[creature.type].id << ", position: " << creature.x << "x " << creature.y << "y "<< creature.z << "z" << endl;
        bool addendl = false;
        if(creature.name.first_name[0])
        {
            cout << "first name: " << creature.name.first_name;
            addendl = true;
        }
        if(creature.name.nickname[0])
        {
            cout << ", nick name: " << creature.name.nickname;
            addendl = true;
        }
        /*
        string transName = DF.TranslateName(creature.name,englishWords,foreignWords,false);
        if(!transName.empty())
        {
            cout << ", trans name: " << transName;
            addendl=true;
        }
*/
        /*
        cout << ", likes: ";
        for(uint32_t i = 0;i<creature.numLikes; i++)
        {
            if(printLike(creature.likes[i],mat,itemTypes))
            {
                cout << ", ";
            }
        } 
        */  
        if(addendl)
        {
            cout << endl;
            addendl = false;
        }
        cout << "profession: " << mem->getProfession(creature.profession) << "(" << (int) creature.profession << ")";
        /*
        if(creature.custom_profession[0])
        {
            cout << ", custom profession: " << creature.custom_profession;
        }
        if(creature.current_job.active)
        {
            cout << ", current job: " << mem->getJob(creature.current_job.jobId);
        }
        */
        cout << endl;
        cout << "happiness: " << creature.happiness /*<< ", strength: " << creature.strength << ", agility: " 
             << creature.agility << ", toughness: " << creature.toughness << ", money: " << creature.money*/ << ", id: " << creature.id;
        /*
        if(creature.squad_leader_id != -1)
        {
            cout << ", squad_leader_id: " << creature.squad_leader_id;
        }
        if(creature.mood != -1){
            cout << ", mood: " << creature.mood << " ";
        }*/
        cout << ", sex: ";
        if(creature.sex == 0)
        {
            cout << "Female";
        }
        else
        {
            cout <<"Male";
        }
        cout << endl;
        /*
        if(creature.pregnancy_timer > 0)
            cout << "gives birth in " << creature.pregnancy_timer/1200 << " days. ";
        cout << "Blood: " << creature.blood_current << "/" << creature.blood_max << " bleeding: " << creature.bleed_rate;
        */
        cout << endl;

    /*
        //skills
        for(unsigned int i = 0; i < creature.skills.size();i++){
            if(i > 0){
                cout << ", ";
            }
            cout << creature.skills[i].name << ": " << creature.skills[i].rating;
        }
    */
        /*
         * FLAGS 1
         */
        cout << "flags1: ";
        print_bits(creature.flags1.whole, cout);
        cout << endl;
        if(creature.flags1.bits.dead)
        {
            cout << "dead ";
        }
        if(creature.flags1.bits.on_ground)
        {
            cout << "on the ground, ";
        }
        if(creature.flags1.bits.skeleton)
        {
            cout << "skeletal ";
        }
        if(creature.flags1.bits.zombie)
        {
            cout << "zombie ";
        }
        if(creature.flags1.bits.tame)
        {
            cout << "tame ";
        }
        if(creature.flags1.bits.royal_guard)
        {
            cout << "royal_guard ";
        }
        if(creature.flags1.bits.fortress_guard)
        {
            cout << "fortress_guard ";
        }
        /*
        * FLAGS 2
        */
        cout << endl << "flags2: ";
        print_bits(creature.flags2.whole, cout);
        cout << endl;
        if(creature.flags2.bits.killed)
        {
            cout << "killed by kill function, ";
        }
        if(creature.flags2.bits.resident)
        {
            cout << "resident, ";
        }
        if(creature.flags2.bits.gutted)
        {
            cout << "gutted, ";
        }
        if(creature.flags2.bits.slaughter)
        {
            cout << "marked for slaughter, ";
        }
        if(creature.flags2.bits.underworld)
        {
            cout << "from the underworld, ";
        }
        cout << endl;
        /*
        if(creature.flags1.bits.had_mood && (creature.mood == -1 || creature.mood == 8 ) )
        {
            string artifact_name = DF.TranslateName(creature.artifact_name,englishWords,foreignWords,false);
            cout << "artifact: " << artifact_name << endl;
        }*/
    cout << endl;
}


int main (void)
{
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
    
    DFHack::Creatures * Creatures = DF.getCreatures();
    DFHack::Materials * Materials = DF.getMaterials();
    
    uint32_t numCreatures;
    if(!Creatures->Start(numCreatures))
    {
        cerr << "Can't get creatures" << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    if(!numCreatures)
    {
        cerr << "No creatures to print" << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    /*
    DF.ReadItemTypes(itemTypes);
    DF.ReadPlantMatgloss(mat.plantMat);
    DF.ReadWoodMatgloss(mat.woodMat);
    DF.ReadStoneMatgloss(mat.stoneMat);
    DF.ReadMetalMatgloss(mat.metalMat);
    DF.ReadCreatureMatgloss(mat.creatureMat);
    */
    mem = DF.getMemoryInfo();
    // get stone matgloss mapping
    if(!Materials->ReadCreatureTypes(creaturestypes))
    {
        cerr << "Can't get the creature types." << endl;
        return 1; 
    }
	/*
    if(!DF.InitReadNameTables(englishWords,foreignWords))
    {
        cerr << "Can't get name tables" << endl;
        return 1;
    }
    */
    //DF.InitViewAndCursor();
    for(uint32_t i = 0; i < numCreatures; i++)
    {
        DFHack::t_creature temp;
        Creatures->ReadCreature(i,temp);
        //if(string(creaturestypes[temp.type].id) == "DWARF")
        {
            cout << "index " << i << " ";
            printCreature(DF,temp);
        }
    }
    /*
    uint32_t currentIdx;
    DFHack::t_creature currentCreature;
    DF.getCurrentCursorCreature(currentIdx);
    cout << "current creature at index " << currentIdx << endl;

    DF.ReadCreature(currentIdx, currentCreature);
    printCreature(DF,currentCreature);
    */
    Creatures->Finish();
    DF.Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
