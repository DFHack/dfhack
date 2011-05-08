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
#include <xgetopt.h>

typedef std::map<int16_t, unsigned int> MatMap;
typedef std::vector< pair<int16_t, unsigned int> > MatSorter;

typedef std::vector<DFHack::t_feature> FeatureList;
typedef std::vector<DFHack::t_feature*> FeatureListPointer;
typedef std::map<DFHack::DFCoord, FeatureListPointer> FeatureMap;
typedef std::vector<DFHack::t_tree> PlantList;

bool parseOptions(int argc, char **argv, bool &showHidden, bool &showPlants,
                  bool &showSlade, bool &showTemple)
{
    char c;
    xgetopt opt(argc, argv, "apst");
    opt.opterr = 0;
    while ((c = opt()) != -1)
    {
        switch (c)
        {
        case 'a':
            showHidden = true;
            break;
        case 'p':
            showPlants = false;
            break;
        case 's':
            showSlade = false;
            break;
        case 't':
            showTemple = false;
            break;
        case '?':
            switch (opt.optopt)
            {
            // For when we take arguments
            default:
                if (isprint(opt.optopt))
                    std::cerr << "Unknown option -" << opt.optopt << "!"
                            << std::endl;
                else
                    std::cerr << "Unknown option character " << (int) opt.optopt << "!"
                            << std::endl;
            }
        default:
            // Um.....
            return false;
        }
    }
    return true;
}

template<template <typename> class P = std::greater >
struct compare_pair_second
{
    template<class T1, class T2>
        bool operator()(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right)
        {
            return P<T2>()(left.second, right.second);
        }
};


void printMats(MatMap &mat, std::vector<DFHack::t_matgloss> &materials)
{
    unsigned int total = 0;
    MatSorter sorting_vector;
    for (MatMap::const_iterator it = mat.begin(); it != mat.end(); ++it)
    {
        sorting_vector.push_back(*it);
    }
    std::sort(sorting_vector.begin(), sorting_vector.end(), compare_pair_second<>());
    for (MatSorter::const_iterator it = sorting_vector.begin(); it != sorting_vector.end(); ++it)
    {
        DFHack::t_matgloss mat = materials[it->first];
        std::cout << std::setw(25) << mat.id << " : " << it->second << std::endl;
        total += it->second;
    }

    std::cout << ">>> TOTAL = " << total << std::endl << std::endl;
}

int main(int argc, char *argv[])
{
    bool showHidden = false;
    bool showPlants = true;
    bool showSlade = true;
    bool showTemple = true;

    if (!parseOptions(argc, argv, showHidden, showPlants, showSlade, showTemple))
    {
        return -1;
    }

    uint32_t x_max = 0, y_max = 0, z_max = 0;
    DFHack::ContextManager manager("Memory.xml");

    DFHack::Context *context = manager.getSingleContext();
    if (!context->Attach())
    {
        std::cerr << "Unable to attach to DF!" << std::endl;
        #ifndef LINUX_BUILD
        std::cin.ignore();
        #endif
        return 1;
    }

    DFHack::Maps *maps = context->getMaps();
    if (!maps->Start())
    {
        std::cerr << "Cannot get map info!" << std::endl;
        context->Detach();
        #ifndef LINUX_BUILD
        std::cin.ignore();
        #endif
        return 1;
    }
    maps->getSize(x_max, y_max, z_max);
    MapExtras::MapCache map(maps);

    DFHack::Materials *mats = context->getMaterials();
    if (!mats->ReadInorganicMaterials())
    {
        std::cerr << "Unable to read inorganic material definitons!" << std::endl;
        context->Detach();
        #ifndef LINUX_BUILD
        std::cin.ignore();
        #endif
        return 1;
    }
    if (showPlants && !mats->ReadOrganicMaterials())
    {
        std::cerr << "Unable to read organic material definitons; plants won't be listed!" << std::endl;
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
        std::cerr << "Unable to read global features; slade won't be listed!" << std::endl;
    }

    if (!maps->ReadLocalFeatures(localFeatures))
    {
        std::cerr << "Unable to read local features; adamantine "
                  << (showTemple ? "and demon temples " : "")
                  << "won't be listed!" << std::endl;
    }

    uint32_t vegCount = 0;
    DFHack::Vegetation *veg = context->getVegetation();
    if (showPlants && !veg->Start(vegCount))
    {
        std::cerr << "Unable to read vegetation; plants won't be listed!" << std::endl;
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
                            std::cerr << "Bad type: " << type << std::endl;
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
                        baseMats[info->material]++;

                        // Find the type of the tile
                        switch (info->material)
                        {
                        case DFHack::SOIL:
                        case DFHack::STONE:
                            layerMats[b->baseMaterialAt(coord)]++;
                            break;
                        case DFHack::VEIN:
                            veinMats[b->veinMaterialAt(coord)]++;
                            break;
                        case DFHack::FEATSTONE:
                            if (blockFeatureLocal && des.bits.feature_local)
                            {
                                if (blockFeatureLocal->type == DFHack::feature_Adamantine_Tube
                                        && blockFeatureLocal->main_material == 0) // stone
                                {
                                    veinMats[blockFeatureLocal->sub_material]++;
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
                                layerMats[blockFeatureGlobal->sub_material]++;
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
                    PlantList plants;
                    if (maps->ReadVegetation(b_x, b_y, z, &plants))
                    {
                        for (PlantList::const_iterator it = plants.begin(); it != plants.end(); it++)
                        {
                            DFHack::t_tree plant = *it;
                            DFHack::DFCoord loc(plant.x, plant.y);
                            loc = loc % 16;
                            if (showHidden || !b->DesignationAt(loc).bits.hidden)
                            {
                                if(plant.is_shrub)
                                    plantMats[it->material]++;
                                else
                                    treeMats[it->material]++;
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

    std::cout << "Base materials:" << std::endl;
    for (it = baseMats.begin(); it != baseMats.end(); ++it)
    {
        std::cout << std::setw(25) << DFHack::TileMaterialString[it->first] << " : " << it->second << std::endl;
    }

    std::cout << std::endl << "Layer materials:" << std::endl;
    printMats(layerMats, mats->inorganic);

    std::cout << "Vein materials:" << std::endl;
    printMats(veinMats, mats->inorganic);

    if (showPlants)
    {
        std::cout << "Shrubs:" << std::endl;
        printMats(plantMats, mats->organic);
        std::cout << "Wood in trees:" << std::endl;
        printMats(treeMats, mats->organic);
    }

    if (hasAquifer)
    {
        std::cout << "Has aquifer" << std::endl;
    }

    if (hasDemonTemple)
    {
        std::cout << "Has demon temple" << std::endl;
    }

    if (hasLair)
    {
        std::cout << "Has lair" << std::endl;
    }

    // Cleanup
    if (showPlants)
    {
        veg->Finish();
    }
    mats->Finish();
    maps->Finish();
    context->Detach();
    #ifndef LINUX_BUILD
    std::cout << " Press any key to finish.";
    std::cin.ignore();
    #endif
    std::cout << std::endl;
    return 0;
}
