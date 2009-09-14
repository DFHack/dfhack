// Map cleaner. Removes all the snow, mud spills, blood and vomit from map tiles.

#include <iostream>
#include <stdint.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main (void)
{
    uint32_t x_max,y_max,z_max;
    uint32_t num_blocks = 0;
    uint32_t bytes_read = 0;
    t_occupancy occupancies[256];
    
    DFHackAPI DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    DF.InitMap();
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
                    // read block designations
                    DF.ReadOccupancy(x,y,z, (uint32_t *) occupancies);
                    // change the hidden flag to 0
                    for (uint32_t i = 0; i < 256;i++)
                    {
                        occupancies[i].unibits.splatter = 0;
                    }
                    // write the designations back
                    DF.WriteOccupancy(x,y,z, (uint32_t *) occupancies);
                }
            }
        }
    }
    return 0;
}