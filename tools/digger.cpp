// digger.cpp

// Usage: Call with a list of TileClass ids separated by a space,
// every (visible) tile on the map with that id will be designated for digging.

#include <iostream>
#include <integers.h>
#include <vector>
#include <list>
#include <cstdlib>
using namespace std;

#include <DFTypes.h>
#include <DFTileTypes.h>
#include <DFHackAPI.h>

int vec_count(vector<uint16_t>& vec, uint16_t t)
{
    int count = 0;
    for (uint32_t i = 0; i < vec.size(); ++i)
    {
        if (vec[i] == t)
            ++count;
    }
    return count;
}

int dig(DFHack::API& DF, vector<uint16_t>& targets)
{
    uint32_t x_max,y_max,z_max;
    DFHack::t_designation designations[256];
    uint16_t tiles[256];
    uint32_t count = 0;
    DF.getSize(x_max,y_max,z_max);
    
    // walk the map
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(DF.isValidBlock(x,y,z))
                {
                    // read block designations and tiletype
                    DF.ReadDesignations(x,y,z, (uint32_t *) designations);
                    DF.ReadTileTypes(x,y,z, (uint16_t *) tiles);

                    // check all tiles, if type is in target list and its visible: designate for dig
                    for (uint32_t i = 0; i < 256; i++)
                    {
                        if (designations[i].bits.hidden == 0 && 
                            vec_count(targets, DFHack::tileTypeTable[tiles[i]].c) > 0)
                        {
                            //cout << "target found at: ";
                            //cout << x << "," << y << "," << z << "," << i << endl;
                            designations[i].bits.dig = DFHack::designation_default;
                            ++count;
                        }
                    }

                    // write the designations back
                    // could probably be optimized in the cases where we dont changed anything
                    DF.WriteDesignations(x,y,z, (uint32_t *) designations);
                }
            }
        }
    }
    return count;
}

int main (int argc, const char* argv[])
{
    vector<uint16_t> targets;
    for (int i = 1; i < argc; ++i)
    {
        targets.push_back(atoi(argv[i]));
    }
    if (targets.size() == 0)
    {
        cout << "Usage: Call with a list of TileClass ids separated by a space,\n";
        cout << "every (visible) tile on the map with that id will be designated for digging.\n\n";
    }
    else
    {
        DFHack::API DF("Memory.xml");
        if(!DF.Attach())
        {
            cerr << "DF not found" << endl;
            return 1;
        }
        DF.InitMap();
        
        int count = dig(DF, targets); // <-- important part
        cout << count << " targets designated" << endl;
        
        DF.Detach();
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}