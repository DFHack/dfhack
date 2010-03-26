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
#include <DFMemInfo.h>

struct matGlosses 
{
    vector<DFHack::t_matgloss> plantMat;
    vector<DFHack::t_matgloss> woodMat;
    vector<DFHack::t_matgloss> stoneMat;
    vector<DFHack::t_matgloss> metalMat;
    vector<DFHack::t_matgloss> creatureMat;
};

string getMaterialType(DFHack::t_item item, const string & itemtype,const matGlosses & mat)
{
    // plant thread seeds
    if(itemtype == "item_plant" || itemtype == "item_thread" || itemtype == "item_seeds" || itemtype == "item_leaves" )
    {
        if(item.material.type >= 0 && item.material.type < mat.plantMat.size())
        {
            return mat.plantMat[item.material.type].id;
        }
        else
        {
            return "some invalid plant material";
        }
    }
    else if (itemtype == "item_drink") // drinks must have different offset for materials
    {
        return "Booze or something";
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
            itemtype == "item_corpse" ||
            itemtype == "item_meat"
            )
    {
        if(item.material.type >= 0 && item.material.type < mat.creatureMat.size())
            return mat.creatureMat[item.material.type].id;
        return "bad material";
    }
    else if(itemtype == "item_wood")
    {
        if(item.material.type >= 0 && item.material.type < mat.woodMat.size())
            return mat.woodMat[item.material.type].id;
        return "bad material";
    }
    else if(itemtype == "item_bar")
    {
        if(item.material.type >= 0 && item.material.type < mat.metalMat.size())
            return mat.metalMat[item.material.type].id;
        return "bad material";
    }
    else
    {
        /*
    Mat_Wood,
    Mat_Stone,
    Mat_Metal,
    Mat_Plant,
    Mat_Leather = 10,
    Mat_SilkCloth = 11,
    Mat_PlantCloth = 12,
    Mat_GreenGlass = 13,
    Mat_ClearGlass = 14,
    Mat_CrystalGlass = 15,
    Mat_Ice = 17,
    Mat_Charcoal =18,
    Mat_Potash = 19,
    Mat_Ashes = 20,
    Mat_PearlAsh = 21,
    Mat_Soap = 24,
    */
        switch (item.material.type)
        {
        case DFHack::Mat_Wood:
            if(item.material.index >= 0 && item.material.index < mat.woodMat.size())
                return mat.woodMat[item.material.index].id;
            return "bad material";
            
        case DFHack::Mat_Stone:
            if(item.material.index >= 0 && item.material.index < mat.stoneMat.size())
                return mat.stoneMat[item.material.index].id;
            return "bad material";
            
        case DFHack::Mat_Metal:
            if(item.material.index >= 0 && item.material.index < mat.metalMat.size())
                return mat.metalMat[item.material.index].id;
            return "bad material";
            
        //case DFHack::Mat_Plant:
        case DFHack::Mat_PlantCloth:
            //return mat.plantMat[item.material.index].id;
            if(item.material.index >= 0 && item.material.index < mat.plantMat.size())
                return string(mat.plantMat[item.material.index].id) + " plant";
            return "bad material";
            
        case 3: // bone
            if(item.material.index >= 0 && item.material.index < mat.creatureMat.size())
                return string(mat.creatureMat[item.material.index].id) + " bone";
            return "bad material";
            
        case 25: // fat
            if(item.material.index >= 0 && item.material.index < mat.creatureMat.size())
                return string(mat.creatureMat[item.material.index].id) + " fat";
            return "bad material";
            
        case 23: // tallow
            if(item.material.index >= 0 && item.material.index < mat.creatureMat.size())
                return string(mat.creatureMat[item.material.index].id) + " tallow";
            return "bad material";
        case 9: // shell
            if(item.material.index >= 0 && item.material.index < mat.creatureMat.size())
                return string(mat.creatureMat[item.material.index].id) + " shell";
            return "bad material";
            
        case DFHack::Mat_Leather: // really a generic creature material. meat for item_food, leather for item_box...
            if(item.material.index >= 0 && item.material.index < mat.creatureMat.size())
                return string(mat.creatureMat[item.material.index].id);
            return "bad material";
            
        case DFHack::Mat_SilkCloth:
            if(item.material.index >= 0 && item.material.index < mat.creatureMat.size())
                return string(mat.creatureMat[item.material.index].id) + " silk";
            return "bad material";
            
        case DFHack::Mat_Soap:
            if(item.material.index >= 0 && item.material.index < mat.creatureMat.size())
                return string(mat.creatureMat[item.material.index].id) + " soap";
            return "bad material";
            
        case DFHack::Mat_GreenGlass:
            return "Green Glass";
            
        case DFHack::Mat_ClearGlass:
            return "Clear Glass";
            
        case DFHack::Mat_CrystalGlass:
            return "Crystal Glass";
            
        case DFHack::Mat_Ice:
            return "Ice";
            
        case DFHack::Mat_Charcoal:
            return "Charcoal";
            
        /*case DFHack::Mat_Potash:
            return "Potash";*/
        
        case DFHack::Mat_Ashes:
            return "Ashes";
            
        case DFHack::Mat_PearlAsh:
            return "Pearlash";
            
        default:
            cout << "unknown material hit: " << item.material.type << " " << item.material.index  << " " << itemtype << endl;
            return "Invalid";
        }
    }   
    return "Invalid";
}
void printItem(DFHack::t_item item, const string & typeString,const matGlosses & mat)
{
    cout << dec << "Item at x:" << item.x << " y:" << item.y << " z:" << item.z << endl;
    cout << "Type: " << (int) item.type << " " << typeString << " Address: " << hex << item.origin << endl;
    cout << "Material: ";

    string itemType = getMaterialType(item,typeString,mat);
    cout << itemType << endl;
}
int main ()
{

    DFHack::API DF ("Memory.xml");
    cout << "This utility lets you mass-designate items by type and material." << endl
         << "Like set on fire all MICROCLINE item_stone..." << endl
         << "Some unusual combinations might be untested and cause the program to crash..."<< endl
         << "so, watch your step and backup your fort" << endl;
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
    DFHack::memory_info *mem = DF.getMemoryInfo();
    DF.Suspend();
    DF.InitViewAndCursor();
    matGlosses mat;
    DF.ReadPlantMatgloss(mat.plantMat);
    DF.ReadWoodMatgloss(mat.woodMat);
    DF.ReadStoneMatgloss(mat.stoneMat);
    DF.ReadMetalMatgloss(mat.metalMat);
    DF.ReadCreatureMatgloss(mat.creatureMat);

//    vector <string> objecttypes;
//    DF.getClassIDMapping(objecttypes);
    uint32_t numItems;
    DF.InitReadItems(numItems);
    map< string, map<string,vector<uint32_t> > > count;
    int failedItems = 0;
    map <string, int > bad_mat_items;
    for(uint32_t i=0; i< numItems; i++)
    {
        DFHack::t_item temp;
        DF.ReadItem(i,temp);
        if(temp.type != -1) // this should be the case pretty much always
        {
            string typestr;
            mem->resolveClassIDToClassname(temp.type,typestr);
            string material = getMaterialType(temp,typestr,mat);
            if (material != "Invalid")
            {
                count[typestr][material].push_back(i);
            }
            else
            {
                if(bad_mat_items.count(typestr))
                {
                    int tmp = bad_mat_items[typestr];
                    tmp ++;
                    bad_mat_items[typestr] = tmp;
                }
                else
                {
                    bad_mat_items[typestr] = 1;
                }
            }
        }
    }
    
    map< string, int >::iterator it_bad;
    if(! bad_mat_items.empty())
    {
        cout << "Items with badly assigned materials:" << endl;
        for(it_bad = bad_mat_items.begin(); it_bad!=bad_mat_items.end();it_bad++)
        {
            cout << it_bad->first << " : " << it_bad->second << endl;
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
    cout << "Select a designation - (d)ump, (f)orbid, (m)melt, set on fi(r)e :" << flush;
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
    else if(designationType == "r" || designationType == "fire")
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

    DF.FinishReadItems();
    DF.Detach();
#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
