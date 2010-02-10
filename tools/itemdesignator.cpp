// Item dump

#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

struct matGlosses 
{
    vector<DFHack::t_matgloss> plantMat;
    vector<DFHack::t_matgloss> woodMat;
    vector<DFHack::t_matgloss> stoneMat;
    vector<DFHack::t_matgloss> metalMat;
    vector<DFHack::t_matgloss> creatureMat;
};

const char * getMaterialType(DFHack::t_item item, const vector<string> & buildingTypes,const matGlosses & mat)
{
    string itemtype = buildingTypes[item.type];
    // plant thread seeds
    if(itemtype == "item_plant" || itemtype == "item_thread" || itemtype == "item_seeds" || itemtype == "item_leaves")
    {
        return mat.plantMat[item.material.type].id;
    }
    // item_skin_raw item_bones item_skull item_fish_raw item_pet item_skin_tanned item_shell
    else if(itemtype == "item_skin_raw" ||
            itemtype == "item_skin_tanned" ||
            itemtype == "item_fish_raw" ||
            itemtype == "item_pet" ||
            itemtype == "item_shell" ||
            itemtype == "item_horn"||
            itemtype == "item_skull" ||
            itemtype == "item_bones" ||
            itemtype == "item_corpse"
            )
    {
        return mat.creatureMat[item.material.type].id;
    }
    else if(itemtype == "item_wood")
    {
        return mat.woodMat[item.material.type].id;
    }
    else
    {
        switch (item.material.type)
        {
        case 0:
            return mat.woodMat[item.material.index].id;
            break;
        case 1:
            return mat.stoneMat[item.material.index].id;
            break;
        case 2:
            return mat.metalMat[item.material.index].id;
            break;
        case 12: // don't ask me why this has such a large jump, maybe this is not actually the matType for plants, but they all have this set to 12
            return mat.plantMat[item.material.index].id;
            break;
        case 3:
        case 9:
        case 10:
        case 11:
        case 121:
            return mat.creatureMat[item.material.index].id;
            break;
        default:
            return 0;
        }
    }   
}
void printItem(DFHack::t_item item, const vector<string> & buildingTypes,const matGlosses & mat)
{
    cout << dec << "Item at x:" << item.x << " y:" << item.y << " z:" << item.z << endl;
    cout << "Type: " << (int) item.type << " " << buildingTypes[item.type] << " Address: " << hex << item.origin << endl;
    cout << "Material: ";

    string itemType = getMaterialType(item,buildingTypes,mat);
    cout << itemType << endl;
}
int main ()
{

    DFHack::API DF ("Memory.xml");
    cout << "This utility lets you mass-designate items by type and material." << endl <<"Like set on fire all MICROCLINE item_stone..." << endl
         << "Some unusual combinations might be untested and cause the program to crash..."<< endl << "so, watch your step and backup your fort" << endl;
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    DF.Suspend();
    DF.InitViewAndCursor();
    matGlosses mat;
    DF.ReadPlantMatgloss(mat.plantMat);
    DF.ReadWoodMatgloss(mat.woodMat);
    DF.ReadStoneMatgloss(mat.stoneMat);
    DF.ReadMetalMatgloss(mat.metalMat);
    DF.ReadCreatureMatgloss(mat.creatureMat);
    DF.ForceResume();

    vector <string> buildingtypes;
    DF.InitReadBuildings(buildingtypes);
    uint32_t numItems = DF.InitReadItems();
    map< string, map<string,vector<uint32_t> > > count;
    int failedItems = 0;
    for(uint32_t i=0; i< numItems; i++)
    {
        DFHack::t_item temp;
        DF.ReadItem(i,temp);
        if(temp.type != -1)
        {
            const char * material = getMaterialType(temp,buildingtypes,mat);
            if (material != 0)
            {
                count[buildingtypes[temp.type]][material].push_back(i);
            }
            else
            {
                cerr << "bad material string for item type: " << buildingtypes[temp.type] << "!" << endl;
            }
        }
    }
    
    map< string, map<string,vector<uint32_t> > >::iterator it1;
    int i =0;
    for(it1 = count.begin(); it1!=count.end();it1++)
    {
        cout << i << ": " << it1->first << "\n";
        i++;
    }
    if(i == 0)
    {
        cout << "No items found" << endl;
        DF.FinishReadBuildings();
        DF.Detach();
        return 0;
    }
    cout << endl << "Select an item type from the list:";
    int number;
    string in;
    stringstream ss;
    getline(cin, in);
    ss.str(in);
    ss >> number;
    int j = 0;
    it1 = count.begin();
    while(j < number && it1!=count.end())
    {
        it1++;
        j++;
    }
    cout << it1->first << "\n";
    map<string,vector<uint32_t> >::iterator it2;
    i=0;
    for(it2 = it1->second.begin();it2!=it1->second.end();it2++){
          cout << i << ":\t" << it2->first << " [" << it2->second.size() << "]" << endl;
            i++;
    }
    cout << endl << "Select a material type: ";
    int number2;
    ss.clear();
    getline(cin, in);
    ss.str(in);
    ss >> number2;
    
    decideAgain:
    cout << "Select a designation - (d)ump, (f)orbid, (m)melt, on fi(r)e :" << flush;
    string designationType;
    getline(cin,designationType);
    DFHack::t_itemflags changeFlag = {0};
    if(designationType == "d" || designationType == "dump")
    {
        changeFlag.bits.dump = 1;
    }
    else if(designationType == "f" || designationType == "forbid")
    {
        changeFlag.bits.forbid = 1;
    }
    else if(designationType == "m" || designationType == "melt")
    {
        changeFlag.bits.melt = 1;
    }
    else if(designationType == "r" || designationType == "flame")
    {
        changeFlag.bits.on_fire = 1;
    }
    else
    {
        goto decideAgain;
    }
    j=0;
    it2= it1->second.begin();
    while(j < number2 && it2!=it1->second.end())
    {
        it2++;
        j++;
    }
    for(uint32_t k = 0;k< it2->second.size();k++)
    {
        DFHack::t_item temp;
        DF.ReadItem(it2->second[k],temp);
        temp.flags.whole |= changeFlag.whole;
        DF.WriteRaw(temp.origin+12,sizeof(uint32_t),(uint8_t *)&temp.flags.whole);
    }

    DF.FinishReadBuildings();
    DF.Detach();
#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
