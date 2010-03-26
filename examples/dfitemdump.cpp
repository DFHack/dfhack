// Item dump

#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFError.h>
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
    
    DFHack::memory_info * mem = DF.getMemoryInfo();
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
    DF.InitViewAndCursor();
    
    cout << "q to quit, anything else to look up items at that location\n";
    while(1)
    {
        string input;
        DF.ForceResume();
        getline (cin, input);
        DF.Suspend();
        //FIXME: why twice?
        uint32_t numItems;
        DF.InitReadItems(numItems);
        if(input == "q")
        {
            break;
        }
        
        int32_t x,y,z;
        DF.getCursorCoords(x,y,z);
        vector <DFHack::t_item> foundItems;
        for(uint32_t i = 0; i < numItems; i++)
        {
            DFHack::t_item temp;
            DF.ReadItem(i, temp);
            if(temp.x == x && temp.y == y && temp.z == z)
            {
                foundItems.push_back(temp);
                //cout << buildingtypes[temp.type] << " 0x" << hex << temp.origin << endl;
            }
        }
        if(foundItems.size() == 0){
            cerr << "No Items at x:" << x << " y:" << y << " z:" << z << endl;
        }
        else if(foundItems.size() == 1)
        {
            string itemtype;
            mem->resolveClassIDToClassname(foundItems[0].type,itemtype);
            printItem(foundItems[0], itemtype ,mat);
        }
        else
        {
            cerr << "Please Select which item you want to display\n";
            string itemtype;
            for(uint32_t j = 0; j < foundItems.size(); ++j)
            {
                mem->resolveClassIDToClassname(foundItems[j].type,itemtype);
                cerr << j << " " << itemtype << endl;
            }
            uint32_t value;
            string input2;
            stringstream ss;
            getline(cin, input2);
            ss.str(input2);
            ss >> value;
            while(value >= foundItems.size())
            {
                cerr << "invalid choice, please try again" << endl;
                input2.clear();
                ss.clear();
                getline (cin, input2);
                ss.str(input2);
                ss >> value;
            }
            mem->resolveClassIDToClassname(foundItems[value].type,itemtype);
            printItem(foundItems[value], itemtype ,mat);
        }
        DF.FinishReadItems();
    }    
    DF.Detach();
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
