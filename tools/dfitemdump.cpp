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

void printItem(DFHack::t_item item, const vector<string> & buildingTypes,const matGlosses & mat){
    cout << dec << "Item at x:" << item.x << " y:" << item.y << " z:" << item.z << endl;
    cout << "Type: " << (int) item.type << " " << buildingTypes[item.type] << " Address: " << hex << item.origin << endl;
    cout << "Material: ";

    //If the item is a thread, seed, or creature based, there is no MatType, so the MatType is actually the material
    //This should probably be checked in the item function
    if(item.type == 113 || item.type == 117) // item_thread or item_seeds
    {
        cout << mat.plantMat[item.material.type].id;
    }
    else if(item.type == 114 || item.type == 115 || item.type == 116 || item.type==128 || item.type == 129|| item.type == 130|| item.type == 131) // item_skin_raw item_bones item_skill item_fish_raw item_pet item_skin_tanned item_shell
    {
        cout << mat.creatureMat[item.material.type].id;
    }
    else{
        switch (item.material.type)
        {
        case 0:
            cout << mat.woodMat[item.material.index].id;
            break;
        case 1:
            cout << mat.stoneMat[item.material.index].id;
            break;
        case 2:
            cout << mat.metalMat[item.material.index].id;
            break;
        case 12: // don't ask me why this has such a large jump, maybe this is not actually the matType for plants, but they all have this set to 12
            cout << mat.plantMat[item.material.index].id;
            break;
        case 121:
            cout << mat.creatureMat[item.material.index].id;
            break;
        default:
            cerr << "invalid mat" << (int) item.material.type << " " << (int) item.material.index << endl;
        }
    }   
    cout << endl;
}

int main ()
{
    DFHack::API DF ("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    
 
    matGlosses mat;
    DF.ReadPlantMatgloss(mat.plantMat);
    DF.ReadWoodMatgloss(mat.woodMat);
    DF.ReadStoneMatgloss(mat.stoneMat);
    DF.ReadMetalMatgloss(mat.metalMat);
    DF.ReadCreatureMatgloss(mat.creatureMat);

    vector <string> buildingtypes;
    DF.InitViewAndCursor();
    cout << "q to quit, anything else to look up items at that location\n";
    while(1)
    {
        string input;
        DF.Resume();
        getline (cin, input);
        DF.Suspend();
        uint32_t numItems = DF.InitReadItems();
        if(input == "q")
        {
            break;
        }
 //       else if(next == 'c'){
 //           cerr << "clearing similarity" << endl;
 //           similarity.clear();
 //           continue;
 //       }

 //       else if(next == 'p'){
 //           vector<bool> same(similarity[0].size(),true);
 //           for(int k =0; k < similarity.size(); k++){
 //               for(int j =0; j < similarity.size(); j++){
 //                   if(k != j){
 //                       for(int l =0; l < similarity[k].size(); l++){
 //                           if(similarity[k][l] != similarity[j][l]){
 //                               same[l] = false;
 //                           }
 //                       }
 //                   }
 //               }
 //           }
 //           for(int itr =0; itr < same.size(); itr++){
 //               if(same[itr] == true && similarity[0][itr] != 0){
 //                   cout << hex << itr << " " << hex << (int)similarity[0][itr] << endl;
 //               }
 //           }
 //           continue;
 //       }
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
        else if(foundItems.size() == 1){
            printItem(foundItems[0], buildingtypes,mat);
  //          similarity.push_back(foundItems[0].bytes);
        }
        else{
            cerr << "Please Select which item you want to display\n";
            for(uint32_t j = 0; j < foundItems.size(); ++j)
            {
                cerr << j << " " << buildingtypes[foundItems[j].type] << endl;
            }
            uint32_t value;
            string input2;
            stringstream ss;
            getline(cin, input2);
            ss.str(input2);
            ss >> value;
            while(value >= foundItems.size()){
                cerr << "invalid choice, please try again" << endl;
                input2.clear();
                ss.clear();
                getline (cin, input2);
                ss.str(input2);
                ss >> value;
            }
            printItem(foundItems[value], buildingtypes,mat);
  //          similarity.push_back(foundItems[value].bytes);
        }
        DF.FinishReadItems();
    }
    DF.FinishReadBuildings();
    DF.Detach();
#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
