// produces a list of vein materials available on the map. can be run with '-a' modifier to show even unrevealed minerals deep underground
// with -b modifier, it will show base layer materials too

// TODO: use material colors to make the output prettier
// TODO: needs the tiletype filter!
// TODO: tile override materials
// TODO: material types, trees, ice, constructions
// TODO: GUI

#include <iostream>
#include <string.h> // for memset
#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <algorithm>
using namespace std;

#include <DFHack.h>
#include <dfhack/DFTileTypes.h>

template<template <typename> class P = std::less >
struct compare_pair_first
{
    template<class T1, class T2>
    bool operator()(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right)
    {
        return P<T1>()(left.first, right.first);
    }
};

template<template <typename> class P = std::less >
struct compare_pair_second
{
    template<class T1, class T2>
    bool operator()(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right)
    {
        return P<T2>()(left.second, right.second);
    }
};

int main (int argc, const char* argv[])
{
    bool showhidden = false;
    bool showbaselayers = false;
    for(int i = 1; i < argc; i++)
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

    DFHack::mapblock40d Block;
    map <int16_t, uint32_t> hardcoded_m;
    map <int16_t, uint32_t> layer_m;
    map <int16_t, uint32_t> vein_m;

    vector<DFHack::t_feature> global_features;
    std::map <DFHack::DFCoord, std::vector<DFHack::t_feature *> > local_features;

    vector< vector <uint16_t> > layerassign;

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF;
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

    DFHack::Maps * Maps = DF->getMaps();
    DFHack::Materials * Mats = DF->getMaterials();

    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    Maps->getSize(x_max,y_max,z_max);

    if(!Maps->ReadGlobalFeatures(global_features))
    {
        cerr << "Can't get global features." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1; 
    }

    if(!Maps->ReadLocalFeatures(local_features))
    {
        cerr << "Can't get local features." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1; 
    }
    // get stone matgloss mapping
    if(!Mats->ReadInorganicMaterials())
    {
        //DF.DestroyMap();
        cerr << "Can't get the materials." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1; 
    }

    // get region geology
    if(showbaselayers && !Maps->ReadGeology( layerassign ))
    {
        cerr << "Can't get region geology." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1; 
    }

    int16_t tempvein [16][16];
    vector <DFHack::t_vein> veins;
    uint32_t maximum_regionoffset = 0;
    uint32_t num_overflows = 0;
    // walk the map!
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(!Maps->isValidBlock(x,y,z))
                    continue;

                // read data
                Maps->ReadBlock40d(x,y,z, &Block);
                DFHack::tiletypes40d & tt = Block.tiletypes;

                // hardcoded materials
                for(uint32_t xx = 0;xx<16;xx++)
                {
                    for (uint32_t yy = 0; yy< 16;yy++)
                    {
                        DFHack::TileMaterial mat = DFHack::tileMaterial(tt[xx][yy]);
                        if(!DFHack::isWallTerrain(tt[xx][yy]))
                            continue;
                        if(hardcoded_m.count(mat))
                        {
                            hardcoded_m[mat] += 1;
                        }
                        else
                        {
                            hardcoded_m[mat] = 1;
                        }
                    }
                }

                // layer stone and soil
                if(showbaselayers)
                {
                    // get the layer materials
                    for(uint32_t xx = 0;xx<16;xx++)
                    {
                        for (uint32_t yy = 0; yy< 16;yy++)
                        {
                            DFHack::TileMaterial mat = DFHack::tileMaterial(tt[xx][yy]);
                            DFHack::TileShape shape = DFHack::TileShape(tt[xx][yy]);
                            if(mat != DFHack::SOIL && mat != DFHack::STONE)
                                continue;
                            if(!DFHack::isWallTerrain(tt[xx][yy]))
                                continue;
                            uint8_t test = Block.designation[xx][yy].bits.biome;
                            if(test > maximum_regionoffset)
                                maximum_regionoffset = test;
                            if( test >= sizeof(Block.biome_indices))
                            {
                                num_overflows++;
                                continue;
                            }
                            uint16_t mat2 = layerassign [Block.biome_indices[test]] [Block.designation[xx][yy].bits.geolayer_index];
                            if(layer_m.count(mat2))
                            {
                                layer_m[mat2] += 1;
                            }
                            else
                            {
                                layer_m[mat2] = 1;
                            }
                        }
                    }
                }
                // global feature overrides
                int16_t idx = Block.global_feature;
                if( idx != -1 && uint16_t(idx) < global_features.size() && global_features[idx].type == DFHack::feature_Underworld)
                {
                    for(uint32_t xi = 0 ; xi< 16 ; xi++) for(uint32_t yi = 0 ; yi< 16 ; yi++)
                    {
                        if(!DFHack::isWallTerrain(tt[xi][yi]))
                            continue;
                        DFHack::TileMaterial mat = DFHack::tileMaterial(tt[xi][yi]);
                        if(Block.designation[xi][yi].bits.feature_global && mat == DFHack::FEATSTONE)
                        {
                            if(global_features[idx].main_material == 0) // stone
                            {
                                int32_t mat2 = global_features[idx].sub_material;
                                if(layer_m.count(mat2))
                                {
                                    layer_m[mat2] += 1;
                                }
                                else
                                {
                                    layer_m[mat2] = 1;
                                }
                            }
                        }
                    }
                }

                // vein stones
                memset(tempvein, 0xff, sizeof(tempvein));
                veins.clear();
                Maps->ReadVeins(x,y,z,&veins);
                // for each vein
                for(int i = 0; i < (int)veins.size();i++)
                {
                    //iterate through vein rows
                    for(uint32_t j = 0;j<16;j++)
                    {
                        //iterate through the bits
                        for (uint32_t k = 0; k< 16;k++)
                        {
                            DFHack::TileMaterial mat = DFHack::tileMaterial(tt[k][j]);
                            if(mat != DFHack::VEIN)
                                continue;
                            // and the bit array with a one-bit mask, check if the bit is set
                            bool set = !!(((1 << k) & veins[i].assignment[j]) >> k);
                            if(set)
                            {
                                // store matgloss
                                tempvein[k][j] = veins[i].type;
                            }
                        }
                    }
                }

                idx = Block.local_feature;
                if( idx != -1 )
                {
                    DFHack::DFCoord pc(x,y);
                    std::map <DFHack::DFCoord, std::vector<DFHack::t_feature *> >::iterator it;
                    it = local_features.find(pc);
                    if(it != local_features.end())
                    {
                        std::vector<DFHack::t_feature *>& vectr = (*it).second;
                        if(uint16_t(idx) < vectr.size() && vectr[idx]->type == DFHack::feature_Adamantine_Tube)
                        {
                            for(uint32_t xi = 0 ; xi< 16 ; xi++) for(uint32_t yi = 0 ; yi< 16 ; yi++)
                            {
                                DFHack::TileMaterial mat = DFHack::tileMaterial(tt[xi][yi]);
                                if(Block.designation[xi][yi].bits.feature_local && mat == DFHack::FEATSTONE)
                                {
                                    if(vectr[idx]->main_material == 0) // stone
                                    {
                                        tempvein[xi][yi] = vectr[idx]->sub_material;
                                    }
                                    else
                                    {
                                        tempvein[xi][yi] = -1;
                                    }
                                }
                            }
                        }
                    }
                }

                // count the vein material types
                for(uint32_t xi = 0 ; xi< 16 ; xi++)
                {
                    for(uint32_t yi = 0 ; yi< 16 ; yi++)
                    {
                        // hidden tiles are ignored unless '-a' is provided on the command line
                        // non-wall tiles are ignored
                        if( (Block.designation[xi][yi].bits.hidden && !showhidden)
                            || !DFHack::isWallTerrain(Block.tiletypes[xi][yi])
                          )
                            continue;
                        // ignore stuff that isn't a vein
                        if(tempvein[xi][yi] < 0)
                            continue;

                        if(vein_m.count(tempvein[xi][yi]))
                        {
                            vein_m[tempvein[xi][yi]] += 1;
                        }
                        else
                        {
                            vein_m[tempvein[xi][yi]] = 1;
                        }
                    }
                }
            }
        }
    }
    // print report

    // some layer/geology debug stuff
    if(maximum_regionoffset >= sizeof(Block.biome_indices) )
    {
        cerr << "Maximal regionoffset seen: " << maximum_regionoffset << ".";
        cerr << " This is above the regionoffsets array size!" << endl;
        cerr << "Number of overflows: " << num_overflows;
        cerr << endl;
    }
    
    vector <pair <int16_t, uint32_t> > veins_sort;
    vector <pair <int16_t, uint32_t> > layers_sort;
    vector <pair <int16_t, uint32_t> > hardcoded_sort;
    map<int16_t, uint32_t>::iterator p;
    // HARDCODED
    cout << endl << "Base materials:" << endl;
    for(p = hardcoded_m.begin(); p != hardcoded_m.end(); p++)
    {
        hardcoded_sort.push_back( pair<int16_t,uint32_t>(p->first, p->second) );
    }
    std::sort(hardcoded_sort.begin(), hardcoded_sort.end(), compare_pair_second<>());
    for(size_t i = 0; i < hardcoded_sort.size();i++)
    {
        cout << DFHack::TileMaterialString[hardcoded_sort[i].first] << " : " << hardcoded_sort[i].second << endl;
    }
    // LAYERS
    cout << endl << "Layer materials:" << endl;
    for(p = layer_m.begin(); p != layer_m.end(); p++)
    {
        if(p->first == -1)
        {
            cout << "Non-stone" << " : " << p->second << endl;
        }
        else
        {
            layers_sort.push_back( pair<int16_t,uint32_t>(p->first, p->second) );
        }
    }
    std::sort(layers_sort.begin(), layers_sort.end(), compare_pair_second<>());
    for(size_t i = 0; i < layers_sort.size();i++)
    {
        if(layers_sort[i].first >= Mats->inorganic.size())
        {
            cerr << "Error, material out of bounds: " << layers_sort[i].first << endl;
            continue;
        }
        cout << Mats->inorganic[layers_sort[i].first].id << " : " << layers_sort[i].second << endl;
    }
    // VEINS
    cout << endl << "Vein materials:" << endl;
    for(p = vein_m.begin(); p != vein_m.end(); p++)
    {
        if(p->first == -1)
        {
            cout << "Non-stone" << " : " << p->second << endl;
        }
        else
        {
            veins_sort.push_back( pair<int16_t,uint32_t>(p->first, p->second) );
        }
    }
    std::sort(veins_sort.begin(), veins_sort.end(), compare_pair_second<>());
    for(size_t i = 0; i < veins_sort.size();i++)
    {
        if(veins_sort[i].first >= Mats->inorganic.size())
        {
            cerr << "Error, material out of bounds: " << veins_sort[i].first << endl;
            continue;
        }
        cout << Mats->inorganic[veins_sort[i].first].id << " : " << veins_sort[i].second << endl;
    }

    DF->Detach();
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
