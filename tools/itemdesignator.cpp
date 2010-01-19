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

string getMaterialType(DFHack::t_item item, const vector<string> & buildingTypes,const matGlosses & mat){
    if(item.type == 85 || item.type == 113 || item.type == 117) // item_plant or item_thread or item_seeds
    {
        return(string(mat.plantMat[item.material.type].id));
    }
    else if(item.type == 109 || item.type == 114 || item.type == 115 || item.type == 116 || item.type==128 || item.type == 129|| item.type == 130|| item.type == 131) // item_skin_raw item_bones item_skill item_fish_raw item_pet item_skin_tanned item_shell
    {
        return(string(mat.creatureMat[item.material.type].id));
    }
    else if(item.type == 124){ //wood
        return(string(mat.woodMat[item.material.type].id));
    }
    else if(item.type == 118){ //blocks
        return(string(mat.metalMat[item.material.index].id));
    }
    else if(item.type == 86){ // item_glob I don't know what those are in game, just ignore them
        return(string(""));
    }
    else{
        switch (item.material.type)
        {
        case 0:
            return(string(mat.woodMat[item.material.index].id));
            break;
        case 1:
            return(string(mat.stoneMat[item.material.index].id));
            break;
        case 2:
            return(string(mat.metalMat[item.material.index].id));
            break;
        case 12: // don't ask me why this has such a large jump, maybe this is not actually the matType for plants, but they all have this set to 12
            return(string(mat.plantMat[item.material.index].id));
            break;
        case 3:
        case 9:
        case 10:
        case 11:
        case 121:
            return(string(mat.creatureMat[item.material.index].id));
            break;
        default:
            //DF.setCursorCoords(item.x,item.y,item.z);
            return(string(""));
        }
    }   
}
void printItem(DFHack::t_item item, const vector<string> & buildingTypes,const matGlosses & mat){
    cout << dec << "Item at x:" << item.x << " y:" << item.y << " z:" << item.z << endl;
    cout << "Type: " << (int) item.type << " " << buildingTypes[item.type] << " Address: " << hex << item.origin << endl;
    cout << "Material: ";

    string itemType = getMaterialType(item,buildingTypes,mat);
    cout << itemType << endl;
}
int main ()
{

    DFHack::API DF ("Memory.xml");

    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
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
            count[buildingtypes[temp.type]][getMaterialType(temp,buildingtypes,mat)].push_back(i);
    }
    
    map< string, map<string,vector<uint32_t> > >::iterator it1;
    int i =0;
    for(it1 = count.begin(); it1!=count.end();it1++){
        cout << i << ": " << it1->first << "\n";
        i++;
    }
    cout << endl << "Select A Item Type:";
    int number;
    string in;
    stringstream ss;
    getline(cin, in);
    ss.str(in);
    ss >> number;
    int j = 0;
    it1 = count.begin();
    while(j < number && it1!=count.end()){
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
    cout << endl << "Select A Material Type: ";
    int number2;
    ss.clear();
    getline(cin, in);
    ss.str(in);
    ss >> number2;
    cout << "Select A Designation: " << flush;
    string designationType;
    getline(cin,designationType);
    //uint32_t changeFlag;
    DFHack::t_itemflags changeFlag = {0};
    if(designationType == "d"){
        changeFlag.bits.dump = 1;
        //changeFlag = (1 << 21);
    }
    else if(designationType == "f"){
        changeFlag.bits.forbid = 1;
        //changeFlag = (1 << 19);
    }
    else if(designationType == "m"){
        changeFlag.bits.melt = 1;
        //changeFlag = (1 << 23);
    }
    else if(designationType == "r"){
        changeFlag.bits.on_fire = 1;
        //changeFlag = (1 << 22);
    }
    j=0;
    it2= it1->second.begin();
    while(j < number2 && it2!=it1->second.end()){
        it2++;
        j++;
    }
    for(uint32_t k = 0;k< it2->second.size();k++){
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
