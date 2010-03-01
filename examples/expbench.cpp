// This program exports the entire map from DF. Takes roughly 6.6 seconds for 1000 cycles on my Linux machine. ~px

#include <iostream>
#include <integers.h>
#include <vector>
#include <ctime>
#include <sstream>
#include <string>

using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

void print_progress (int current, int total)
{
    if(total < 100)
    {
        cout << "\b" << current << " / " <<  total << " %\xd" << std::flush;
    }
    else
    {
        if( current % (total/100) == 0 )
        {
            int percentage = current / (total/100);
            // carridge return, not line feed, so percentage, less console spam :)
            cout << "\b" << percentage << " %\xd" << std::flush; // and a flush for good measure
        }
    }
}

int main (int numargs, char** args)
{
    time_t start, end;
    double time_diff;
    
    
    unsigned int iterations = 0;
    if (numargs == 2)
    {
        istringstream input (args[1],istringstream::in);
        input >> iterations;
    }
    if(iterations == 0)
        iterations = 1000;
    
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
    
    cout << "doing " << iterations << " iterations" << endl;
    for(uint32_t i = 0; i< iterations;i++)
    {
        print_progress (i, iterations);
        
        if(!DF.InitMap())
            break;
        DF.getSize(x_max,y_max,z_max);
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
                        bytes_read += sizeof(tiletypes) + sizeof(designations) + sizeof(occupancies);
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
