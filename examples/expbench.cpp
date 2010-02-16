// This program exports the entire map from DF. Takes roughly 6.6 seconds for 1000 cycles on my Linux machine. ~px

#include <iostream>
#include <integers.h>
#include <vector>
#include <ctime>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main (void)
{
    time_t start, end;
    double time_diff;
    
    uint32_t x_max,y_max,z_max;
    uint32_t num_blocks = 0;
    uint64_t bytes_read = 0;
    uint16_t tiletypes[16][16];
    DFHack::t_designation designations[16][16];
    DFHack::t_occupancy occupancies[16][16];
    
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    
    time(&start);
    for(uint32_t i = 0; i< 1000;i++)
    {
        if(!DF.InitMap())
            break;
        DF.getSize(x_max,y_max,z_max);
        if((i % 10) == 0)
        {
            int percentage = i / 10;
            cout << percentage << endl;
        }
        for(uint32_t x = 0; x< x_max;x++)
        {
            for(uint32_t y = 0; y< y_max;y++)
            {
                for(uint32_t z = 0; z< z_max;z++)
                {
                    if(DF.isValidBlock(x,y,z))
                    {
                        DF.ReadTileTypes(x,y,z, (uint16_t *) tiletypes);
                        DF.ReadDesignations(x,y,z, (uint32_t *) designations);
                        DF.ReadOccupancy(x,y,z, (uint32_t *) occupancies);
                        num_blocks ++;
                        bytes_read += 256 * (4 + 4 + 2);
                    }
                }
            }
        }
        DF.DestroyMap();
    }
    DF.Detach();
    time(&end);
    time_diff = difftime(end, start);
    cout << num_blocks << " blocks read" << endl;
    cout << bytes_read / (1024 * 1024) << " MB" << endl;
    cout << "map export tests done in " << time_diff << " seconds." << endl;
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
