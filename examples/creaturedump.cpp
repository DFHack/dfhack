// Creature dump

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFMemInfo.h>

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
    
likeType printLike(DFHack::t_like like, const matGlosses & mat,const vector< vector <DFHack::t_itemType> > & itemTypes)
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


int main (void)
{
    vector<DFHack::t_matgloss> creaturestypes;
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    
    vector< vector <DFHack::t_itemType> > itemTypes;
    DF.ReadItemTypes(itemTypes);
    
    matGlosses mat;
    DF.ReadPlantMatgloss(mat.plantMat);
    DF.ReadWoodMatgloss(mat.woodMat);
    DF.ReadStoneMatgloss(mat.stoneMat);
    DF.ReadMetalMatgloss(mat.metalMat);
    DF.ReadCreatureMatgloss(mat.creatureMat);
    
    DFHack::memory_info *mem = DF.getMemoryInfo();
    // get stone matgloss mapping
    if(!DF.ReadCreatureMatgloss(creaturestypes))
    {
        cerr << "Can't get the creature types." << endl;
        return 1; 
    }
    
    map<string, vector<string> > names;
    if(!DF.InitReadNameTables(names))
    {
        cerr << "Can't get name tables" << endl;
        return 1;
    }
    uint32_t numCreatures;
    if(!DF.InitReadCreatures(numCreatures))
    {
        cerr << "Can't get creatures" << endl;
        return 1;
    }
    for(uint32_t i = 0; i < numCreatures; i++)
    {
        DFHack::t_creature temp;
        DF.ReadCreature(i, temp);
        if(string(creaturestypes[temp.type].id) == "DWARF")
        {
            cout << "address: " << temp.origin << " creature type: " << creaturestypes[temp.type].id << ", position: " << temp.x << "x " << temp.y << "y "<< temp.z << "z" << endl;
            bool addendl = false;
            if(temp.first_name[0])
            {
                cout << "first name: " << temp.first_name;
                addendl = true;
            }
            if(temp.nick_name[0])
            {
                cout << ", nick name: " << temp.nick_name;
                addendl = true;
            }
            string transName = DF.TranslateName(temp.last_name,names,creaturestypes[temp.type].id);
            if(!transName.empty())
            {
                cout << ", trans name: " << transName;
                addendl=true;
            }
            //cout << ", generic name: " << DF.TranslateName(temp.last_name,names,"GENERIC");
            /*
            if(!temp.trans_name.empty()){
                cout << ", trans name: " << temp.trans_name;
                addendl =true;
            }
            if(!temp.generic_name.empty()){
                cout << ", generic name: " << temp.generic_name;
                addendl=true;
            }
            */
            cout << ", likes: ";
            for(uint32_t i = 0;i<temp.numLikes; i++)
            {
                if(printLike(temp.likes[i],mat,itemTypes))
                {
                    cout << ", ";
                }
            }   
            if(addendl)
            {
                cout << endl;
                addendl = false;
            }
            cout << "profession: " << mem->getProfession(temp.profession) << "(" << (int) temp.profession << ")";
            if(temp.custom_profession[0])
            {
                cout << ", custom profession: " << temp.custom_profession;
            }
            if(temp.current_job.active)
            {
                cout << ", current job: " << mem->getJob(temp.current_job.jobId);
            }
            cout << endl;
            cout << "happiness: " << temp.happiness << ", strength: " << temp.strength << ", agility: " 
                 << temp.agility << ", toughness: " << temp.toughness << ", money: " << temp.money << ", id: " << temp.id;
            if(temp.squad_leader_id != -1)
            {
                cout << ", squad_leader_id: " << temp.squad_leader_id;
            }
            cout << ", sex: ";
            if(temp.sex == 0)
            {
                cout << "Female";
            }
            else
            {
                cout <<"Male";
            }
            cout << endl;
        /*
            //skills
            for(unsigned int i = 0; i < temp.skills.size();i++){
                if(i > 0){
                    cout << ", ";
                }
                cout << temp.skills[i].name << ": " << temp.skills[i].rating;
            }
        */
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
                cout << "resident, ";
            }
            if(temp.flags2.bits.gutted)
            {
                cout << "gutted, ";
            }
            if(temp.flags2.bits.slaughter)
            {
                cout << "marked for slaughter, ";
            }
            if(temp.flags2.bits.underworld)
            {
                cout << "from the underworld, ";
            }
            cout << endl << endl;
        }
    }
    DF.FinishReadCreatures();
    DF.Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
