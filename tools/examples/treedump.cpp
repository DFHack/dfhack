// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#define DFHACK_WANT_MISCUTILS
#include <DFHack.h>
/*
        uint16_t material; // +0x3E
        uint16_t x; // +0x40
        uint16_t y; // +0x42
        uint16_t z; // +0x44
        uint16_t padding; // +0x46
        uint32_t unknown_1; // +0x48
        uint16_t temperature_1; // +0x4C
        uint16_t temperature_2; // +0x4E - maybe fraction?
        uint32_t mystery_flag; // 0x50: yes, just one
        uint32_t unknown_2; // 0x54
        uint32_t unknown_3; // 0x58
        // a vector is here
        uint32_t address;
        */
void print_tree( DFHack::Context * DF , DFHack::dfh_plant & tree)
{
    DFHack::Materials * mat = DF->getMaterials();
    DFHack::t_plant & tdata = tree.sdata;
    printf("%d:%d = ",tdata.type,tdata.material);
    if(tdata.watery)
    {
        cout << "near-water ";
    }
    cout << mat->organic[tdata.material].id << " ";
    if(!tdata.is_shrub)
    {
        cout << "tree";
    }
    else
    {
        cout << "shrub";
    }
    cout << endl;
    printf("Grow counter: 0x%08x\n", tdata.grow_counter);
    printf("temperature 1: %d\n", tdata.temperature_1);
    printf("temperature 2: %d\n", tdata.temperature_2);
    printf("On fire: %d\n", tdata.is_burning);
    printf("hitpoints: 0x%08x\n", tdata.hitpoints);
    printf("update order: %d\n", tdata.update_order);
    printf("Address: 0x%x\n", tree.address);
    hexdump(DF,tree.address,13*16);
}

int main (int numargs, const char ** args)
{
    uint32_t addr;
    if (numargs == 2)
    {
        istringstream input (args[1],istringstream::in);
        input >> std::hex >> addr;
    }
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
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

    DFHack::Process* p = DF->getProcess();
    DFHack::VersionInfo* mem = DF->getMemoryInfo();
    DFHack::Gui * Gui = DF->getGui();
    DFHack::Vegetation * v = DF->getVegetation();
    DFHack::Maps * mps = DF->getMaps();
    DFHack::Materials * mat = DF->getMaterials();
    mat->ReadOrganicMaterials();

    int32_t x,y,z;
    Gui->getCursorCoords(x,y,z);

    uint32_t numVegs = 0;
    v->Start(numVegs);
    if(x == -30000)
    {
        cout << "----==== Trees ====----" << endl;
        for(uint32_t i =0; i < numVegs; i++)
        {
            DFHack::dfh_plant tree;
            v->Read(i,tree);
            printf("%d/%d/%d, %d:%d\n",tree.sdata.x,tree.sdata.y,tree.sdata.z,tree.sdata.type,tree.sdata.material);
        }
    }
    else
    {
        // new method, gets the vector of trees in a block. can show farm plants
        if(mps->Start())
        {
            vector<DFHack::dfh_plant> alltrees;
            if(mps->ReadVegetation(x/16,y/16,z,&alltrees))
            {
                for(int i = 0 ; i < alltrees.size(); i++)
                {
                    DFHack::dfh_plant & tree = alltrees[i];
                    // you could take the tree coords from the struct and % them with 16 for use in loops over the whole block
                    if(tree.sdata.x == x && tree.sdata.y == y && tree.sdata.z == z)
                    {
                        cout << "----==== Tree at "<< x << "/" << y << "/" << z << " ====----" << endl;
                        print_tree(DF, tree);
                        break;
                    }
                }
            }
        }
        // old method, gets the tree from the global vegetation vector. can't show farm plants
        for(uint32_t i =0; i < numVegs; i++)
        {
            DFHack::dfh_plant tree;
            v->Read(i,tree);
            if(tree.sdata.x == x && tree.sdata.y == y && tree.sdata.z == z)
            {
                cout << "----==== Tree at "<< dec << x << "/" << y << "/" << z << " ====----" << endl;
                print_tree(DF, tree);
                break;
            }
        }
    }
    v->Finish();

    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
