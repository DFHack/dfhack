/* 
DFBauxite - Converts all your mechanisms to bauxite (for use in magma).
Author: Alex Legg

Based on code from and uses DFHack - www.sourceforge.net/projects/dfhack
*/

#include "DFCommonInternal.h"
#include <sstream>
using namespace std;

int main ()
{
    DFHack::Process *proc;
    DFHack::memory_info *meminfo;
    DFHack::DfVector *items_vector;
    DFHack::t_item_df40d item_40d;
    DFHack::t_matglossPair item_40d_material;
    vector<DFHack::t_matgloss> stoneMat;
    uint32_t item_material_offset;
    uint32_t temp;
    int32_t type;
    int items;
    int found = 0, converted = 0;

    DFHack::API DF ("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }

    DF.Suspend();
    
    /*
     * Find out which material is bauxite
     */
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
    
    //FIXME: very counter-intuitive, fix the API. This returns a mapping from the internal ID to the object name
    //       internal IDs can be obtained by using resolveClassId
    //FIXME: Proper error reporting/exceptions support is sorely needed
    /*
     * Get the object name/ID mapping
     */
    vector <string> buildingtypes;
    uint32_t numBuildings = DF.InitReadBuildings(buildingtypes);
    
    proc = DF.getProcess();
    meminfo = proc->getDescriptor();
    
    //FIXME: work on the 'supported features' system required
    /*
     * Check availability of required addresses and offsets (doing custom stuff here)
     */
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
    
    items_vector = new DFHack::DfVector (proc->getDataModel()->readVector (items, 4));
    for(uint32_t i = 0; i < items_vector->getSize(); i++)
    {
        // get pointer to object
        temp = * (uint32_t *) items_vector->at (i);
        // read object
        proc->read (temp, sizeof (DFHack::t_item_df40d), (uint8_t *) &item_40d);

        // resolve object type
        type = -1;
        
        // skip things we can't identify
        if(!meminfo->resolveClassId (temp, type))
            continue;
       
        if(buildingtypes[type] == "item_trapparts")
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
