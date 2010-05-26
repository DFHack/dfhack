/* 
DFBauxite - Converts all your mechanisms to bauxite (for use in magma).
Author: Alex Legg

Based on code from and uses DFHack - www.sourceforge.net/projects/dfhack
*/

#include <sstream>
#include <iostream>
#include <string.h>
#include <cstdlib>
#include <assert.h>
#include <string>
#include <vector>
using namespace std;


#include <DFIntegers.h>
#include <DFExport.h>
#include <DFError.h>
#include <DFVector.h>
#include <DFMemInfo.h>
#include <DFProcess.h>
#include <DFTypes.h>
using namespace DFHack;


int main ()
{
    DFHack::Process *proc;
    DFHack::memory_info *meminfo;
    DFHack::DfVector <uint32_t> *items_vector;
    DFHack::t_item_df40d item_40d;
    DFHack::t_matglossPair item_40d_material;
    vector<DFHack::t_matgloss> stoneMat;
    uint32_t item_material_offset;
    uint32_t temp;
    int32_t type;
    int items;
    int found = 0, converted = 0;

    DFHack::ContextManager DF("Memory.xml");
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
    
    // Find out which material is bauxite
    if(!DF.ReadStoneMatgloss(stoneMat))
    {
        cout << "Materials not supported for this version of DF, exiting." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        DF.Detach();
        return EXIT_FAILURE;
    }
    int bauxiteIndex = -1;
    for (int i = 0; i < stoneMat.size();i++)
    {
        if(strcmp(stoneMat[i].id, "BAUXITE") == 0)
        {
            bauxiteIndex = i;
            break;
        }
    }
    if(bauxiteIndex == -1)
    {
        cout << "Cannot locate bauxite in the DF raws, exiting" << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        DF.Detach();
        return EXIT_FAILURE;
    }
    
    // Get some basics needed for full access
    proc = DF.getProcess();
    meminfo = proc->getDescriptor();
    
    // Get the object name/ID mapping
    //FIXME: work on the 'supported features' system required

    // Check availability of required addresses and offsets (doing custom stuff here)

    items = meminfo->getAddress("items");
    item_material_offset = meminfo->getOffset("item_materials");
    if( !items || ! item_material_offset)
    {
        cout << "Items not supported for this DF version, exiting" << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        DF.Detach();
        return EXIT_FAILURE;
    }
    
    items_vector = new DFHack::DfVector <uint32_t> (proc, items);
    for(uint32_t i = 0; i < items_vector->size(); i++)
    {
        // get pointer to object
        temp = items_vector->at (i);
        // read object
        proc->read (temp, sizeof (DFHack::t_item_df40d), (uint8_t *) &item_40d);

        // resolve object type
        type = -1;
        
        // skip things we can't identify
        if(!meminfo->resolveObjectToClassID (temp, type))
            continue;
        string classname;
        if(!meminfo->resolveClassIDToClassname (type, classname))
            continue;
       
        if(classname == "item_trapparts")
        {
            proc->read (temp + item_material_offset, sizeof (DFHack::t_matglossPair), (uint8_t *) &item_40d_material);

            cout << dec << "Mechanism at x:" << item_40d.x << " y:" << item_40d.y << " z:" << item_40d.z << " ID:" << item_40d.ID << endl;
            
            if (item_40d_material.index != bauxiteIndex)
            {
                item_40d_material.index = bauxiteIndex;
                proc->write (temp + item_material_offset, sizeof (DFHack::t_matglossPair), (uint8_t *) &item_40d_material);
                converted++;
            }

            found++;
        }
    }


    if (found == 0)
    {
        cout << "No mechanisms to convert" << endl;
    } else {
        cout << found << " mechanisms found" << endl;
        cout << converted << " mechanisms converted" << endl;
    }

    DF.Resume();
    DF.Detach();

    delete items_vector;

#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif

    return 0;
}
