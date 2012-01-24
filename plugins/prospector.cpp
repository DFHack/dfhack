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
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/MapCache.h"

#include "DataDefs.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;

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
        return count;
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

typedef std::vector<df::plant *> PlantList;

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

static void printMatdata(DFHack::Console & con, const matdata &data)
{
    con << std::setw(9) << data.count;

    if(data.lower_z != data.upper_z)
        con <<" Z:" << std::setw(4) << data.lower_z << ".." <<  data.upper_z << std::endl;
    else
        con <<" Z:" << std::setw(4) << data.lower_z << std::endl;
}

static int getValue(const df::inorganic_raw &info)
{
    return info.material.material_value;
}

static int getValue(const df::plant_raw &info)
{
    return info.value;
}

template <typename T>
void printMats(DFHack::Console & con, MatMap &mat, std::vector<T*> &materials, bool show_value)
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
        T* mat = materials[it->first];
        // Somewhat of a hack, but it works because df::inorganic_raw and df::plant_raw both have a field named "id"
        con << std::setw(25) << mat->id << " : ";
        if (show_value)
            con << std::setw(3) << getValue(*mat) << " : ";
        printMatdata(con, it->second);
        total += it->second.count;
    }

    con << ">>> TOTAL = " << total << std::endl << std::endl;
}

void printVeins(DFHack::Console & con, MatMap &mat_map,
                DFHack::Materials* mats, bool show_value)
{
    MatMap ores;
    MatMap gems;
    MatMap rest;

    for (MatMap::const_iterator it = mat_map.begin(); it != mat_map.end(); ++it)
    {
        df::inorganic_raw *gloss = world->raws.inorganics[it->first];

        if (gloss->material.isGem())
            gems[it->first] = it->second;
        else if (gloss->isOre())
            ores[it->first] = it->second;
        else
            rest[it->first] = it->second;
    }

    con << "Ores:" << std::endl;
    printMats(con, ores, world->raws.inorganics, show_value);

    con << "Gems:" << std::endl;
    printMats(con, gems, world->raws.inorganics, show_value);

    con << "Other vein stone:" << std::endl;
    printMats(con, rest, world->raws.inorganics, show_value);
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
    bool showValue = false;
    bool showTube = false;
    Console & con = c->con;
    for(int i = 0; i < parameters.size();i++)
    {
        if (parameters[i] == "all")
        {
            showHidden = true;
        }
        else if (parameters[i] == "value")
        {
            showValue = true;
        }
        else if (parameters[i] == "hell")
        {
            showHidden = showTube = true;
        }
        else if(parameters[i] == "help" || parameters[i] == "?")
        {
            c->con.print("Prints a big list of all the present minerals.\n"
                         "By default, only the visible part of the map is scanned.\n"
                         "\n"
                         "Options:\n"
                         "all   - Scan the whole map, as if it was revealed.\n"
                         "value - Show material value in the output.\n"
                         "hell  - Show the Z range of HFS tubes.\n"
            );
            return CR_OK;
        }
    }
    uint32_t x_max = 0, y_max = 0, z_max = 0;
    CoreSuspender suspend(c);
    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    Maps::getSize(x_max, y_max, z_max);
    MapExtras::MapCache map;

    DFHack::Materials *mats = c->getMaterials();

    DFHack::t_feature blockFeatureGlobal;
    DFHack::t_feature blockFeatureLocal;

    bool hasAquifer = false;
    bool hasDemonTemple = false;
    bool hasLair = false;
    MatMap baseMats;
    MatMap layerMats;
    MatMap veinMats;
    MatMap plantMats;
    MatMap treeMats;

    matdata liquidWater;
    matdata liquidMagma;
    matdata aquiferTiles;
    matdata tubeTiles;

    uint32_t vegCount = 0;

    for(uint32_t z = 0; z < z_max; z++)
    {
        for(uint32_t b_y = 0; b_y < y_max; b_y++)
        {
            for(uint32_t b_x = 0; b_x < x_max; b_x++)
            {
                // Get the map block
                df::coord2d blockCoord(b_x, b_y);
                MapExtras::Block *b = map.BlockAt(DFHack::DFCoord(b_x, b_y, z));
                if (!b || !b->valid)
                {
                    continue;
                }

                { // Find features
                    uint32_t index = b->raw.global_feature;
                    if (index != -1)
                        Maps::GetGlobalFeature(blockFeatureGlobal, index);

                    index = b->raw.local_feature;
                    if (index != -1)
                        Maps::GetLocalFeature(blockFeatureLocal, blockCoord, index);
                }

                int global_z = world->map.region_z + z;

                // Iterate over all the tiles in the block
                for(uint32_t y = 0; y < 16; y++)
                {
                    for(uint32_t x = 0; x < 16; x++)
                    {
                        df::coord2d coord(x, y);
                        df::tile_designation des = b->DesignationAt(coord);
                        df::tile_occupancy occ = b->OccupancyAt(coord);

                        // Skip hidden tiles
                        if (!showHidden && des.bits.hidden)
                        {
                            continue;
                        }

                        // Check for aquifer
                        if (des.bits.water_table)
                        {
                            hasAquifer = true;
                            aquiferTiles.add(global_z);
                        }

                        // Check for lairs
                        if (occ.bits.monster_lair)
                        {
                            hasLair = true;
                        }

                        // Check for liquid
                        if (des.bits.flow_size)
                        {
                            if (des.bits.liquid_type == tile_liquid::Magma)
                                liquidMagma.add(global_z);
                            else
                                liquidWater.add(global_z);
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
                        case DFHack::EMPTY:
                            /* A heuristic: tubes inside adamantine have EMPTY:AIR tiles which
                               still have feature_local set. Also check the unrevealed status,
                               so as to exclude any holes mined by the player. */
                            if (info->material == DFHack::AIR &&
                                des.bits.feature_local && des.bits.hidden &&
                                blockFeatureLocal.type == feature_type::deep_special_tube)
                            {
                                tubeTiles.add(global_z);
                            }
                        default:
                            continue;
                        }

                        // Count the material type
                        baseMats[info->material].add(global_z);

                        // Find the type of the tile
                        switch (info->material)
                        {
                        case DFHack::SOIL:
                        case DFHack::STONE:
                            layerMats[b->baseMaterialAt(coord)].add(global_z);
                            break;
                        case DFHack::VEIN:
                            veinMats[b->veinMaterialAt(coord)].add(global_z);
                            break;
                        case DFHack::FEATSTONE:
                            if (blockFeatureLocal.type != -1 && des.bits.feature_local)
                            {
                                if (blockFeatureLocal.type == feature_type::deep_special_tube
                                        && blockFeatureLocal.main_material == 0) // stone
                                {
                                    veinMats[blockFeatureLocal.sub_material].add(global_z);
                                }
                                else if (showTemple
                                         && blockFeatureLocal.type == feature_type::deep_surface_portal)
                                {
                                    hasDemonTemple = true;
                                }
                            }

                            if (showSlade && blockFeatureGlobal.type != -1 && des.bits.feature_global
                                    && blockFeatureGlobal.type == feature_type::feature_underworld_from_layer
                                    && blockFeatureGlobal.main_material == 0) // stone
                            {
                                layerMats[blockFeatureGlobal.sub_material].add(global_z);
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
                    if (Maps::ReadVegetation(b_x, b_y, z, plants))
                    {
                        for (PlantList::const_iterator it = plants->begin(); it != plants->end(); it++)
                        {
                            const df::plant & plant = *(*it);
                            df::coord2d loc(plant.pos.x, plant.pos.y);
                            loc = loc % 16;
                            if (showHidden || !b->DesignationAt(loc).bits.hidden)
                            {
                                if(plant.flags.bits.is_shrub)
                                    plantMats[plant.material].add(global_z);
                                else
                                    treeMats[plant.material].add(global_z);
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

    if (liquidWater.count || liquidMagma.count)
    {
        con << std::endl << "Liquids:" << std::endl;
        if (liquidWater.count)
        {
            con << std::setw(25) << "WATER" << " : ";
            printMatdata(con, liquidWater);
        }
        if (liquidWater.count)
        {
            con << std::setw(25) << "MAGMA" << " : ";
            printMatdata(con, liquidMagma);
        }
    }

    con << std::endl << "Layer materials:" << std::endl;
    printMats(con, layerMats, world->raws.inorganics, showValue);

    printVeins(con, veinMats, mats, showValue);

    if (showPlants)
    {
        con << "Shrubs:" << std::endl;
        printMats(con, plantMats, world->raws.plants.all, showValue);
        con << "Wood in trees:" << std::endl;
        printMats(con, treeMats, world->raws.plants.all, showValue);
    }

    if (hasAquifer)
    {
        con << "Has aquifer";
        if (aquiferTiles.count)
        {
            con << "               : ";
            printMatdata(con, aquiferTiles);
        }
        else
            con << std::endl;
    }

    if (showTube && tubeTiles.count)
    {
        con << "Has HFS tubes             : ";
        printMatdata(con, tubeTiles);
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
    mats->Finish();
    con << std::endl;
    return CR_OK;
}
