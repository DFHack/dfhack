// Item designator

#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
using namespace std;

#include <DFHack.h>
#include <DFVector.h>
using namespace DFHack;

int main ()
{

    DFHack::ContextManager CM ("Memory.xml");
    DFHack::Context * DF;
    DFHack::VersionInfo *mem;
    DFHack::Gui * Gui;
    DFHack::Materials * Mats;
    DFHack::Items * Items;
    cout << "This utility lets you mass-designate items by type and material." << endl
         << "Like set on fire all MICROCLINE item_stone..." << endl
         << "Some unusual combinations might be untested and cause the program to crash..."<< endl
         << "so, watch your step and backup your fort" << endl;
    try
    {
        DF = CM.getSingleContext();
        DF->Attach();
        mem = DF->getMemoryInfo();
        Gui = DF->getGui();
        Mats = DF->getMaterials();
        Mats->ReadAllMaterials();
        Items = DF->getItems();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    DFHack::Process * p = DF->getProcess();
    DFHack::OffsetGroup* itemGroup = mem->getGroup("Items");
    unsigned vector_addr = itemGroup->getAddress("items_vector");
    DFHack::DfVector <uint32_t> p_items (p, vector_addr);
    uint32_t numItems = p_items.size();

    map< string, map<string,vector< dfh_item > > > itemmap;
    map< string, map< string, vector< dfh_item > > >::iterator it1;
    int failedItems = 0;
    map <string, int > bad_mat_items;
    for(uint32_t i=0; i< numItems; i++)
    {
        DFHack::dfh_item temp;
        Items->readItem(p_items[i],temp);
        string typestr = Items->getItemClass(temp);
        string material = Mats->getDescription(temp.matdesc);
        itemmap[typestr][material].push_back(temp);
    }

    int i =0;
    for( it1  = itemmap.begin(); it1!=itemmap.end();it1++)
    {
        cout << i << ": " << it1->first << "\n";
        i++;
    }
    if(i == 0)
    {
        cout << "No items found" << endl;
        DF->Detach();
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
    it1 = itemmap.begin();
    while(j < number && it1!=itemmap.end())
    {
        it1++;
        j++;
    }
    cout << it1->first << "\n";
    map<string,vector<dfh_item> >::iterator it2;
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
        changeFlag.dump = 1;
    }
    else if(designationType == "f" || designationType == "forbid")
    {
        changeFlag.forbid = 1;
    }
    else if(designationType == "m" || designationType == "melt")
    {
        changeFlag.melt = 1;
    }
    else if(designationType == "r" || designationType == "fire")
    {
        changeFlag.on_fire = 1;
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
        DFHack::dfh_item & t = it2->second[k];
        t.base.flags.whole |= changeFlag.whole;
        Items->writeItem(t);
    }
    DF->Detach();
#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
