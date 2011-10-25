// Produces a list of materials available on the map.
// Options:
//  -a : show unrevealed tiles
//  -p : don't show plants
//  -s : don't show slade
//  -t : don't show demon temple

//#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>

using namespace std;
#include <DFHack.h>
#include <dfhack/extra/MapExtras.h>
#include <dfhack/extra/termutil.h>
#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/PluginManager.h>

using namespace DFHack;

struct matdata
{
    const static int invalid_z = -30000;
    matdata()
    {
        count = 0;
        lower_z = invalid_z;
        upper_z = invalid_z;
    }
    matdata (const matdata & copyme)
    {
        count = copyme.count;
        lower_z = copyme.lower_z;
        upper_z = copyme.upper_z;
    }
    unsigned int add( int z_level = invalid_z )
    {
        count ++;
        if(z_level != invalid_z)
        {
            if(lower_z == invalid_z || z_level < lower_z)
            {
                lower_z = z_level;
            }
            if(upper_z == invalid_z || z_level > upper_z)
            {
                upper_z = z_level;
            }
        }
    }
    unsigned int count;
    int lower_z;
    int upper_z;
};

bool operator>(const matdata & q1, const matdata & q2)
{
    return q1.count > q2.count;
}

typedef std::map<int16_t, matdata> MatMap;
typedef std::vector< pair<int16_t, matdata> > MatSorter;

typedef std::vector<DFHack::t_feature> FeatureList;
typedef std::vector<DFHack::t_feature*> FeatureListPointer;
typedef std::map<DFHack::DFCoord, FeatureListPointer> FeatureMap;
typedef std::vector<DFHack::df_plant *> PlantList;

#define TO_PTR_VEC(obj_vec, ptr_vec) \
    ptr_vec.clear(); \
    for (size_t i = 0; i < obj_vec.size(); i++) \
        ptr_vec.push_back(&obj_vec[i])

template<template <typename> class P = std::greater >
struct compare_pair_second
{
    template<class T1, class T2>
        bool operator()(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right)
        {
            return P<T2>()(left.second, right.second);
        }
};

// printMats() accepts a vector of pointers to t_matgloss so that it can
// deal t_matgloss and all subclasses.
void printMats(DFHack::Console & con, MatMap &mat,
               std::vector<DFHack::t_matgloss*> &materials)
{
    unsigned int total = 0;
    MatSorter sorting_vector;
    for (MatMap::const_iterator it = mat.begin(); it != mat.end(); ++it)
    {
        sorting_vector.push_back(*it);
    }
    std::sort(sorting_vector.begin(), sorting_vector.end(),
              compare_pair_second<>());
    for (MatSorter::const_iterator it = sorting_vector.begin();
         it != sorting_vector.end(); ++it)
    {
        if(it->first >= materials.size())
        {
            con << "Bad index: " << it->first << " out of " 
                <<  materials.size() << endl;
            continue;
        }
        DFHack::t_matgloss* mat = materials[it->first];
        con << std::setw(25) << mat->id << " : "
        << std::setw(9) << it->second.count;
        if(it->second.lower_z != it->second.upper_z)
            con <<" Z:" << std::setw(4) << it->second.lower_z << ".." <<  it->second.upper_z << std::endl;
        else
            con <<" Z:" << std::setw(4) << it->second.lower_z << std::endl;
        total += it->second.count;
    }

    con << ">>> TOTAL = " << total << std::endl << std::endl;
}

void printMats(DFHack::Console & con, MatMap &mat,
               std::vector<DFHack::t_matgloss> &materials)
{
    std::vector<DFHack::t_matgloss*> ptr_vec;
    TO_PTR_VEC(materials, ptr_vec);
    printMats(con, mat, ptr_vec);
}

void printVeins(DFHack::Console & con, MatMap &mat_map,
                DFHack::Materials* mats)
{
    MatMap ores;
    MatMap gems;
    MatMap rest;

    for (MatMap::const_iterator it = mat_map.begin(); it != mat_map.end(); ++it)
    {
        DFHack::t_matglossInorganic &gloss = mats->inorganic[it->first];

        if (gloss.isGem())
            gems[it->first] = it->second;
        else if (gloss.isOre())
            ores[it->first] = it->second;
        else
            rest[it->first] = it->second;
    }

    std::vector<DFHack::t_matgloss*> ptr_vec;
    TO_PTR_VEC(mats->inorganic, ptr_vec);

    con << "Ores:" << std::endl;
    printMats(con, ores, ptr_vec);

    con << "Gems:" << std::endl;
    printMats(con, gems, ptr_vec);

    con << "Other vein stone:" << std::endl;
    printMats(con, rest, ptr_vec);
}

DFhackCExport command_result prospector (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "prospector";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("prospect","Show stats of available raw resources. Use option 'all' to show hidden resources.",prospector));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result prospector (DFHack::Core * c, vector <string> & parameters)
{
    bool showHidden = false;
    bool showPlants = true;
    bool showSlade = true;
    bool showTemple = true;
    Console & con = c->con;
    for(int i = 0; i < parameters.size();i++)
    {
        if (parameters[0] == "all")
        {
            showHidden = true;
        }
        else if(parameters[i] == "help" || parameters[i] == "?")
        {
            c->con.print("Prints a big list of all the present minerals.\n"
                         "By default, only the visible part of the map is scanned.\n"
                         "\n"
                         "Options:\n"
                         "all   - Scan the whole map, as if it was revealed.\n"
            );
            return CR_OK;
        }
    }
    uint32_t x_max = 0, y_max = 0, z_max = 0;
    c->Suspend();
    DFHack::Maps *maps = c->getMaps();
    if (!maps->Start())
    {
        con << "Cannot get map info!" << std::endl;
        c->Resume();
        return CR_FAILURE;
    }
    maps->getSize(x_max, y_max, z_max);
    MapExtras::MapCache map(maps);

    DFHack::Materials *mats = c->getMaterials();
    if (!mats->ReadInorganicMaterials())
    {
        con << "Unable to read inorganic material definitons!" << std::endl;
        c->Resume();
        return CR_FAILURE;
    }
    if (showPlants && !mats->ReadOrganicMaterials())
    {
        con << "Unable to read organic material definitons; plants won't be listed!" << std::endl;
        showPlants = false;
    }

    FeatureList globalFeatures;
    FeatureMap localFeatures;
    DFHack::t_feature *blockFeatureGlobal = 0;
    DFHack::t_feature *blockFeatureLocal = 0;

    bool hasAquifer = false;
    bool hasDemonTemple = false;
    bool hasLair = false;
    MatMap baseMats;
    MatMap layerMats;
    MatMap veinMats;
    MatMap plantMats;
    MatMap treeMats;

    if (!(showSlade && maps->ReadGlobalFeatures(globalFeatures)))
    {
        con << "Unable to read global features; slade won't be listed!" << std::endl;
    }

    if (!maps->ReadLocalFeatures(localFeatures))
    {
        con << "Unable to read local features; adamantine "
            << (showTemple ? "and demon temples " : "")
            << "won't be listed!" << std::endl;
    }

    uint32_t vegCount = 0;
    DFHack::Vegetation *veg = c->getVegetation();
    if (showPlants && !veg->Start())
    {
        con << "Unable to read vegetation; plants won't be listed!" << std::endl;
    }

    for(uint32_t z = 0; z < z_max; z++)
    {
        for(uint32_t b_y = 0; b_y < y_max; b_y++)
        {
            for(uint32_t b_x = 0; b_x < x_max; b_x++)
            {
                // Get the map block
                DFHack::DFCoord blockCoord(b_x, b_y);
                MapExtras::Block *b = map.BlockAt(DFHack::DFCoord(b_x, b_y, z));
                if (!b || !b->valid)
                {
                    continue;
                }

                { // Find features
                    uint16_t index = b->raw.global_feature;
                    if (index != -1 && index < globalFeatures.size())
                    {
                        blockFeatureGlobal = &globalFeatures[index];
                    }

                    index = b->raw.local_feature;
                    FeatureMap::const_iterator it = localFeatures.find(blockCoord);
                    if (it != localFeatures.end())
                    {
                        FeatureListPointer features = it->second;

                        if (index != -1 && index < features.size())
                        {
                            blockFeatureLocal = features[index];
                        }
                    }
                }

                // Iterate over all the tiles in the block
                for(uint32_t y = 0; y < 16; y++)
                {
                    for(uint32_t x = 0; x < 16; x++)
                    {
                        DFHack::DFCoord coord(x, y);
                        DFHack::t_designation des = b->DesignationAt(coord);
                        DFHack::t_occupancy occ = b->OccupancyAt(coord);

                        // Skip hidden tiles
                        if (!showHidden && des.bits.hidden)
                        {
                            continue;
                        }

                        // Check for aquifer
                        if (des.bits.water_table)
                        {
                            hasAquifer = true;
                        }

                        // Check for lairs
                        if (occ.bits.monster_lair)
                        {
                            hasLair = true;
                        }

                        uint16_t type = b->TileTypeAt(coord);
                        const DFHack::TileRow *info = DFHack::getTileRow(type);

                        if (!info)
                        {
                            con << "Bad type: " << type << std::endl;
                            continue;
                        }

                        // We only care about these types
                        switch (info->shape)
                        {
                        case DFHack::WALL:
                        case DFHack::PILLAR:
                        case DFHack::FORTIFICATION:
                            break;
                        default:
                            continue;
                        }

                        // Count the material type
                        baseMats[info->material].add(z);

                        // Find the type of the tile
                        switch (info->material)
                        {
                        case DFHack::SOIL:
                        case DFHack::STONE:
                            layerMats[b->baseMaterialAt(coord)].add(z);
                            break;
                        case DFHack::VEIN:
                            veinMats[b->veinMaterialAt(coord)].add(z);
                            break;
                        case DFHack::FEATSTONE:
                            if (blockFeatureLocal && des.bits.feature_local)
                            {
                                if (blockFeatureLocal->type == DFHack::feature_Adamantine_Tube
                                        && blockFeatureLocal->main_material == 0) // stone
                                {
                                    veinMats[blockFeatureLocal->sub_material].add(z);
                                }
                                else if (showTemple
                                         && blockFeatureLocal->type == DFHack::feature_Hell_Temple)
                                {
                                    hasDemonTemple = true;
                                }
                            }

                            if (showSlade && blockFeatureGlobal && des.bits.feature_global
                                    && blockFeatureGlobal->type == DFHack::feature_Underworld
                                    && blockFeatureGlobal->main_material == 0) // stone
                            {
                                layerMats[blockFeatureGlobal->sub_material].add(z);
                            }
                            break;
                        case DFHack::OBSIDIAN:
                            // TODO ?
                            break;
                        }
                    }
                }

                // Check plants this way, as the other way wasn't getting them all
                // and we can check visibility more easily here
                if (showPlants)
                {
                    PlantList * plants;
                    if (maps->ReadVegetation(b_x, b_y, z, plants))
                    {
                        for (PlantList::const_iterator it = plants->begin(); it != plants->end(); it++)
                        {
                            const DFHack::df_plant & plant = *(*it);
                            DFHack::DFCoord loc(plant.x, plant.y);
                            loc = loc % 16;
                            if (showHidden || !b->DesignationAt(loc).bits.hidden)
                            {
                                if(plant.is_shrub)
                                    plantMats[plant.material].add(z);
                                else
                                    treeMats[plant.material].add(z);
                            }
                        }
                    }
                }
                // Block end
            } // block x

            // Clean uneeded memory
            map.trash();
        } // block y
    } // z

    MatMap::const_iterator it;

    con << "Base materials:" << std::endl;
    for (it = baseMats.begin(); it != baseMats.end(); ++it)
    {
        con << std::setw(25) << DFHack::TileMaterialString[it->first] << " : " << it->second.count << std::endl;
    }

    std::vector<t_matgloss*> ptr_vec;
    TO_PTR_VEC(mats->inorganic, ptr_vec);

    con << std::endl << "Layer materials:" << std::endl;
    printMats(con, layerMats, ptr_vec);

    printVeins(con, veinMats, mats);

    if (showPlants)
    {
        con << "Shrubs:" << std::endl;
        printMats(con, plantMats, mats->organic);
        con << "Wood in trees:" << std::endl;
        printMats(con, treeMats, mats->organic);
    }

    if (hasAquifer)
    {
        con << "Has aquifer" << std::endl;
    }

    if (hasDemonTemple)
    {
        con << "Has demon temple" << std::endl;
    }

    if (hasLair)
    {
        con << "Has lair" << std::endl;
    }

    // Cleanup
    if (showPlants)
    {
        veg->Finish();
    }
    mats->Finish();
    maps->Finish();
    c->Resume();
    con << std::endl;
    return CR_OK;
}
