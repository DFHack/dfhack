// Creature dump

#include <iostream>
#include <climits>
#include <string.h>
#include <vector>
#include <stdio.h>
using namespace std;

#define DFHACK_WANT_MISCUTILS
#include <DFHack.h>

enum likeType
{
    FAIL = 0,
    MATERIAL = 1,
    ITEM = 2,
    FOOD = 3
};

DFHack::Materials * Materials;
DFHack::VersionInfo *mem;
vector< vector<string> > englishWords;
vector< vector<string> > foreignWords;
DFHack::Creatures * Creatures = NULL;
uint32_t current_year;
uint32_t current_tick;
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

void printCreature(DFHack::Context * DF, const DFHack::t_creature & creature)
{
    uint32_t dayoflife;
    cout << "address: " << hex <<  creature.origin << dec << ", creature race: " << creature.race << "/" << Materials->raceEx[creature.race].rawname 
                << "[" << Materials->raceEx[creature.race].tile_character
                << "," << Materials->raceEx[creature.race].tilecolor.fore
                << "," << Materials->raceEx[creature.race].tilecolor.back
                << "," << Materials->raceEx[creature.race].tilecolor.bright
                << "]"
                << ", position: " << creature.x << "x " << creature.y << "y "<< creature.z << "z" << endl;
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
        
        DFHack::Translation *Tran = DF->getTranslation();
        DFHack::VersionInfo *mem = DF->getMemoryInfo();
        
        string transName = Tran->TranslateName(creature.name,false);
        if(!transName.empty())
        {
            cout << ", trans name: " << transName;
            addendl=true;
        }
        
        transName = Tran->TranslateName(creature.name,true);
        if(!transName.empty())
        {
            cout << ", last name: " << transName;
            addendl=true;
        }

        if(creature.civ)
        {
            cout << ", civilization: " << creature.civ;
            addendl = true;
        }

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
        cout << ", profession: " << mem->getProfession(creature.profession) << "(" << (int) creature.profession << ")";
        
        if(creature.custom_profession[0])
        {
            cout << ", custom profession: " << creature.custom_profession;
        }
        
        if(creature.current_job.active)
        {
            try{
                cout << ", current job: " << mem->getJob(creature.current_job.jobId);
            }
            catch(exception & e)
            {
                cout << e.what() << endl;
            }
        }
        
        cout << endl;
        dayoflife = creature.birth_year*12*28 + creature.birth_time/1200;
        cout << "Born on the year " << creature.birth_year << ", month " << (creature.birth_time/1200/28) << ", day " << ((creature.birth_time/1200) % 28 + 1) << ", " << dayoflife << " days lived." << endl;
        cout << "Appearance : ";
        for(unsigned int i = 0; i<creature.nbcolors ; i++)
        {
            cout << Materials->raceEx[creature.race].castes[creature.caste].ColorModifier[i].part << " ";
            uint32_t color = Materials->raceEx[creature.race].castes[creature.caste].ColorModifier[i].colorlist[creature.color[i]];
            if(color<Materials->color.size())
            {
                cout << Materials->color[color].name << "[" 
                     << (unsigned int) (Materials->color[color].red*255) << ":"
                     << (unsigned int) (Materials->color[color].green*255) << ":"
                     << (unsigned int) (Materials->color[color].blue*255) << "]";
            }
            else if (color < Materials->alldesc.size())
            {
                cout << Materials->alldesc[color].id;
            }
            else
            {
                cout << "Unknown color " << color << endl;
            }
            if( Materials->raceEx[creature.race].castes[creature.caste].ColorModifier[i].startdate > 0 )
            {
                if( (Materials->raceEx[creature.race].castes[creature.caste].ColorModifier[i].startdate <= dayoflife) &&
                    (Materials->raceEx[creature.race].castes[creature.caste].ColorModifier[i].enddate > dayoflife) )
                    cout << "[active]";
                else
                    cout << "[inactive]";
            }
            cout << " - ";

        }
        cout << endl;
        cout << "happiness: "   << creature.happiness
             << ", strength: "  << creature.strength.level 
             << ", agility: "   << creature.agility.level
             << ", toughness: " << creature.toughness.level
             << ", endurance: " << creature.endurance.level
             << ", recuperation: " << creature.recuperation.level
             << ", disease resistance: " << creature.disease_resistance.level
             //<< ", money: " << creature.money
             << ", id: " << creature.id;
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

        if((creature.mood != -1) && (creature.mood<5))
        {
            cout << "mood: " << creature.mood << ", skill: " << mem->getSkill(creature.mood_skill) << endl;
            vector<DFHack::t_material> mymat;
            if(Creatures->ReadJob(&creature, mymat))
            {
                for(unsigned int i = 0; i < mymat.size(); i++)
                {
                    printf("\t%s(%d)\t%d %d %d - %.8x\n", Materials->getDescription(mymat[i]).c_str(), mymat[i].itemType, mymat[i].subType, mymat[i].subIndex, mymat[i].index, mymat[i].flags);
                }
            }
        }

        //std::vector<uint32_t> inventory;
        // FIXME: TOO BAD...
        /*
        if( Creatures->ReadInventoryPtr(creature.origin, inventory) )
        {
            DFHack::Items * Items = DF->getItems();
            printf("\tInventory:\n");
            for(unsigned int i = 0; i < inventory.size(); i++)
            {
                printf("\t\t%s\n", Items->getItemDescription(inventory[i], Materials).c_str());
            }
        }
        */

        /*
        if(creature.pregnancy_timer > 0)
            cout << "gives birth in " << creature.pregnancy_timer/1200 << " days. ";
        cout << "Blood: " << creature.blood_current << "/" << creature.blood_max << " bleeding: " << creature.bleed_rate;
        */
        cout << endl;

        if(creature.has_default_soul)
        {
            //skills
            cout << "Skills" << endl;
            for(unsigned int i = 0; i < creature.defaultSoul.numSkills;i++)
            {
                if(i > 0)
                {
                    cout << ", ";
                }
                try
                {
                    cout << mem->getSkill(creature.defaultSoul.skills[i].id) << ": " << creature.defaultSoul.skills[i].rating;
                }
                catch(DFHack::Error::AllMemdef &e)
                {
                    cout << "Unknown skill! : " << creature.defaultSoul.skills[i].id <<", rating: "  << creature.defaultSoul.skills[i].rating << endl;
                    cout << e.what() << endl;
                }
            }
            cout << endl;
            cout << "Traits" << endl;
            for(uint32_t i = 0; i < 30;i++)
            {
                string trait = mem->getTrait (i, creature.defaultSoul.traits[i]);
                if(!trait.empty()) cout << trait << ", ";
            }
            cout << endl;
                    
            // labors
            cout << "Labors" << endl;
            for(unsigned int i = 0; i < NUM_CREATURE_LABORS;i++)
            {
                if(!creature.labors[i])
                    continue;
                string laborname;
                try
                {
                    laborname = mem->getLabor(i);
                }
                catch(exception &)
                {
                    break;
                }
                cout << laborname << ", ";
            }
            cout << endl;
        }
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
        
        if(creature.flags1.bits.had_mood && (creature.mood == -1 || creature.mood == 8 ) )
        {
            string artifact_name = Tran->TranslateName(creature.artifact_name,false);
            cout << "artifact: " << artifact_name << endl;
        }


    cout << endl;
}


int main (int numargs, char ** args)
{
    DFHack::World * World;
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context* DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    string check = "";
    if(numargs == 2)
        check = args[1];
    
    Creatures = DF->getCreatures();
    Materials = DF->getMaterials();
    World = DF->getWorld();
    current_year = World->ReadCurrentYear();
    current_tick = World->ReadCurrentTick();
    DFHack::Translation * Tran = DF->getTranslation();
    
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

    mem = DF->getMemoryInfo();
    Materials->ReadInorganicMaterials();
    Materials->ReadOrganicMaterials();
    Materials->ReadWoodMaterials();
    Materials->ReadPlantMaterials();
    Materials->ReadCreatureTypes();
    Materials->ReadCreatureTypesEx();
    //Materials->ReadDescriptorColors();

    if(!Tran->Start())
    {
        cerr << "Can't get name tables" << endl;
        return 1;
    }
    vector<uint32_t> addrs;
    for(uint32_t i = 0; i < numCreatures; i++)
    {
        DFHack::t_creature temp;
        Creatures->ReadCreature(i,temp);
        if(check.empty() || string(Materials->raceEx[temp.race].rawname) == check)
        {
            printCreature(DF,temp);
            addrs.push_back(temp.origin);
        }
    }
    if(addrs.size() <= 10)
    {
        interleave_hex(DF,addrs,200);
    }
    Creatures->Finish();
    DF->Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
