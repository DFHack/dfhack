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

void print_tree( DFHack::Context * DF , DFHack::t_tree & tree)
{
    DFHack::Materials * mat = DF->getMaterials();

    printf("%d:%d = ",tree.type,tree.material);
    if(tree.type == 1 || tree.type == 3)
    {
        cout << "near-water ";
    }
    cout << mat->organic[tree.material].id << " ";
    if(tree.type == 0 || tree.type == 1)
    {
        cout << "tree";
    }
    if(tree.type == 2 || tree.type == 3)
    {
        cout << "shrub";
    }
    cout << endl;
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
            DFHack::t_tree tree;
            v->Read(i,tree);
            printf("%d/%d/%d, %d:%d\n",tree.x,tree.y,tree.z,tree.type,tree.material);
        }
    }
    else
    {
        // new method, gets the vector of trees in a block. can show farm plants
        if(mps->Start())
        {
            vector<DFHack::t_tree> alltrees;
            if(mps->ReadVegetation(x/16,y/16,z,&alltrees))
            {
                for(int i = 0 ; i < alltrees.size(); i++)
                {
                    DFHack::t_tree & tree = alltrees[i];
                    // you could take the tree coords from the struct and % them with 16 for use in loops over the whole block
                    if(tree.x == x && tree.y == y && tree.z == z)
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
            DFHack::t_tree tree;
            v->Read(i,tree);
            if(tree.x == x && tree.y == y && tree.z == z)
            {
                cout << "----==== Tree at "<< x << "/" << y << "/" << z << " ====----" << endl;
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
