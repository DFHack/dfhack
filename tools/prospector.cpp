// produces a list of vein materials available on the map. can be run with '-a' modifier to show even unrevealed minerals deep underground
// with -b modifier, it will show base layer materials too

// TODO: use material colors to make the output prettier
// TODO: needs the tiletype filter!
// TODO: tile override materials
// TODO: material types, trees, ice, constructions
// TODO: GUI

#include <iostream>
#include <stdint.h>
#include <string.h> // for memset
#include <vector>
#include <map>
using namespace std;

#include <DFTypes.h>
#include <DFTileTypes.h>
#include <DFHackAPI.h>

int main (int argc, const char* argv[])
{
    
    bool showhidden = false;
    bool showbaselayers = false;
    for(int i = 0; i < argc; i++)
    {
        string test = argv[i];
        if(test == "-a")
        {
            showhidden = true;
        }
        else if(test == "-b")
        {
            showbaselayers = true;
        }
        else if(test == "-ab" || test == "-ba")
        {
            showhidden = true;
            showbaselayers = true;
        }
    }
    // let's be more useful when double-clicked on windows
    #ifndef LINUX_BUILD
    showhidden = true;
    #endif
    uint32_t x_max,y_max,z_max;
    uint16_t tiletypes[16][16];
    t_designation designations[16][16];
    uint8_t regionoffsets[16];
    map <int16_t, uint32_t> materials;
    materials.clear();
    vector<t_matgloss> stonetypes;
    vector< vector <uint16_t> > layerassign;
    
    // init the API
    DFHackAPI DF("Memory.xml");
    
    // attach
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    
    // init the map
    DF.InitMap();
    DF.getSize(x_max,y_max,z_max);
    
    // get stone matgloss mapping
    if(!DF.ReadStoneMatgloss(stonetypes))
    {
        //DF.DestroyMap();
        cerr << "Can't get the materials." << endl;
        return 1; 
    }
    
    // get region geology
    if(!DF.ReadGeology( layerassign ))
    {
        cerr << "Can't get region geology." << endl;
        return 1; 
    }
    
    int16_t tempvein [16][16];
    vector <t_vein> veins;
    // walk the map!
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(!DF.isValidBlock(x,y,z))
                    continue;
                
                // read data
                DF.ReadTileTypes(x,y,z, (uint16_t *) tiletypes);
                DF.ReadDesignations(x,y,z, (uint32_t *) designations);
                
                memset(tempvein, -1, sizeof(tempvein));
                veins.clear();
                DF.ReadVeins(x,y,z,veins);
                
                if(showbaselayers)
                {
                    DF.ReadRegionOffsets(x,y,z, regionoffsets);
                    // get the layer materials
                    for(uint32_t xx = 0;xx<16;xx++)
                    {
                        for (uint32_t yy = 0; yy< 16;yy++)
                        {
                            tempvein[xx][yy] =
                            layerassign
                            [regionoffsets[designations[xx][yy].bits.biome]]
                            [designations[xx][yy].bits.geolayer_index];
                        }
                    }
                }
                
                // for each vein
                for(int i = 0; i < veins.size();i++)
                {
                    //iterate through vein rows
                    for(uint32_t j = 0;j<16;j++)
                    {
                        //iterate through the bits
                        for (uint32_t k = 0; k< 16;k++)
                        {
                            // and the bit array with a one-bit mask, check if the bit is set
                            bool set = ((1 << k) & veins[i].assignment[j]) >> k;
                            if(set)
                            {
                                // store matgloss
                                tempvein[k][j] = veins[i].type;
                            }
                        }
                    }
                }
                // count the material types
                for(uint32_t xi = 0 ; xi< 16 ; xi++)
                {
                    for(uint32_t yi = 0 ; yi< 16 ; yi++)
                    {
                        // hidden tiles are ignored unless '-a' is provided on the command line
                        // non-wall tiles are ignored
                        if(designations[xi][yi].bits.hidden && !showhidden || !isWallTerrain(tiletypes[xi][yi]))
                            continue;
                        if(tempvein[xi][yi] < 0)
                            continue;
                        
                        if(materials.count(tempvein[xi][yi]))
                        {
                            materials[tempvein[xi][yi]] += 1;
                        }
                        else
                        {
                            materials[tempvein[xi][yi]] = 1;
                        }
                    }    
                }
            }
        }
    }
    // print report
    map<int16_t, uint32_t>::iterator p;
    for(p = materials.begin(); p != materials.end(); p++)
    {
        cout << stonetypes[p->first].id << " : " << p->second << endl;
    }
    // wait for input on windows so the tool is still usable to some extent
    #ifndef LINUX_BUILD
    uint32_t junk = 0;
    cin >> junk;
    #endif
    return 0;
}