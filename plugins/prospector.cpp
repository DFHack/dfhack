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
#include <functional>
#include <vector>

using namespace std;
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Gui.h"
#include "modules/MapCache.h"

#include "MiscUtils.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_region_details.h"
#include "df/world_region_feature.h"
#include "df/world_geo_biome.h"
#include "df/world_geo_layer.h"
#include "df/world_underground_region.h"
#include "df/feature_init.h"
#include "df/region_map_entry.h"
#include "df/inclusion_type.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/plant.h"

using namespace DFHack;
using namespace df::enums;
using df::coord2d;

DFHACK_PLUGIN("prospector");
REQUIRE_GLOBAL(world);

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
    unsigned int add( int z_level = invalid_z, int delta = 1 )
    {
        count += delta;
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

template<typename _Tp = matdata >
struct shallower : public binary_function<_Tp, _Tp, bool>
{
    bool operator()(const _Tp& top, const _Tp& bottom) const
    {
        float topavg = (top.lower_z + top.upper_z)/2.0f;
        float btmavg = (bottom.lower_z + bottom.upper_z)/2.0f;
        return topavg > btmavg;
    }
};


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

static void printMatdata(color_ostream &con, const matdata &data, bool only_z = false)
{
    if (!only_z)
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

template <typename T, template <typename> class P>
void printMats(color_ostream &con, MatMap &mat, std::vector<T*> &materials, bool show_value)
{
    unsigned int total = 0;
    MatSorter sorting_vector;
    for (MatMap::const_iterator it = mat.begin(); it != mat.end(); ++it)
    {
        sorting_vector.push_back(*it);
    }
    std::sort(sorting_vector.begin(), sorting_vector.end(),
              compare_pair_second<P>());
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

void printVeins(color_ostream &con, MatMap &mat_map,
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
    printMats<df::inorganic_raw, std::greater>(con, ores, world->raws.inorganics, show_value);

    con << "Gems:" << std::endl;
    printMats<df::inorganic_raw, std::greater>(con, gems, world->raws.inorganics, show_value);

    con << "Other vein stone:" << std::endl;
    printMats<df::inorganic_raw, std::greater>(con, rest, world->raws.inorganics, show_value);
}

command_result prospector (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "prospect", "Show stats of available raw resources.",
        prospector, false,
        "  Prints a big list of all the present minerals.\n"
        "  By default, only the visible part of the map is scanned.\n"
        "Options:\n"
        "  all   - Scan the whole map, as if it was revealed.\n"
        "  value - Show material value in the output. Most useful for gems.\n"
        "  hell  - Show the Z range of HFS tubes. Implies 'all'.\n"
        "Pre-embark estimate:\n"
        "  If called during the embark selection screen, displays\n"
        "  an estimate of layer stone availability. If the 'all'\n"
        "  option is specified, also estimates veins.\n"
        "  The estimate is computed either for 1 embark tile of the\n"
        "  blinking biome, or for all tiles of the embark rectangle.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

static coord2d biome_delta[] = {
    coord2d(-1,1), coord2d(0,1), coord2d(1,1),
    coord2d(-1,0), coord2d(0,0), coord2d(1,0),
    coord2d(-1,-1), coord2d(0,-1), coord2d(1,-1)
};

struct EmbarkTileLayout {
    coord2d biome_off, biome_pos;
    df::region_map_entry *biome;
    df::world_geo_biome *geo_biome;
    int elevation, max_soil_depth;
    int min_z, base_z;
    std::map<int, float> penalty;
};

static df::world_region_details *get_details(df::world_data *data, df::coord2d pos)
{
    int d_idx = linear_index(data->region_details, &df::world_region_details::pos, pos);
    return vector_get(data->region_details, d_idx);
}

bool estimate_underground(color_ostream &out, EmbarkTileLayout &tile, df::world_region_details *details, int x, int y)
{
    // Find actual biome
    int bv = clip_range(details->biome[x][y] & 15, 1, 9);
    tile.biome_off = biome_delta[bv-1];

    df::world_data *data = world->world_data;
    int bx = clip_range(details->pos.x + tile.biome_off.x, 0, data->world_width-1);
    int by = clip_range(details->pos.y + tile.biome_off.y, 0, data->world_height-1);
    tile.biome_pos = coord2d(bx, by);
    tile.biome = &data->region_map[bx][by];

    tile.geo_biome = df::world_geo_biome::find(tile.biome->geo_index);

    // Compute surface elevation
    tile.elevation = details->elevation[x][y];
    tile.max_soil_depth = std::max((154-tile.elevation)/5,1);
    tile.penalty.clear();

    // Special biome adjustments
    if (!tile.biome->flags.is_set(region_map_entry_flags::is_lake))
    {
        // Mountain biome
        if (tile.biome->elevation >= 150)
            tile.max_soil_depth = 0;
        // Ocean biome
        else if (tile.biome->elevation < 100)
        {
            if (tile.elevation == 99)
                tile.elevation = 98;

            if (tile.geo_biome && (tile.geo_biome->unk1 == 4 || tile.geo_biome->unk1 == 5))
            {
                auto b_details = get_details(data, tile.biome_pos);

                if (b_details && b_details->unk12e8 < 500)
                    tile.max_soil_depth = 0;
            }
        }
    }

    tile.base_z = tile.elevation-1;

    auto &features = details->features[x][y];

    // Collect global feature layer depths and apply penalties
    std::map<int, int> layer_bottom, layer_top;
    bool sea_found = false;

    for (size_t i = 0; i < features.size(); i++)
    {
        auto feature = features[i];
        auto layer = df::world_underground_region::find(feature->layer);
        if (!layer || feature->min_z == -30000) continue;

        layer_bottom[layer->layer_depth] = feature->min_z;
        layer_top[layer->layer_depth] = feature->max_z;
        tile.base_z = std::min(tile.base_z, (int)feature->min_z);

        float penalty = 1.0f;
        switch (layer->type) {
        case df::world_underground_region::Cavern:
            penalty = 0.75f;
            break;
        case df::world_underground_region::MagmaSea:
            sea_found = true;
            tile.min_z = feature->min_z;
            for (int i = feature->min_z; i <= feature->max_z; i++)
                tile.penalty[i] = 0.2 + 0.6f*(i-feature->min_z)/(feature->max_z-feature->min_z+1);
            break;
        case df::world_underground_region::Underworld:
            penalty = 0.0f;
            break;
        }

        if (penalty != 1.0f)
        {
            for (int i = feature->min_z; i <= feature->max_z; i++)
                tile.penalty[i] = penalty;
        }
    }

    if (!sea_found)
    {
        out.printerr("Could not find magma sea; depth may be incorrect.\n");
        tile.min_z = tile.base_z;
    }

    // Scan for big local features and apply their penalties
    for (size_t i = 0; i < features.size(); i++)
    {
        auto feature = features[i];
        auto lfeature = Maps::getLocalInitFeature(details->pos, feature->feature_idx);
        if (!lfeature)
            continue;

        switch (lfeature->getType())
        {
        case feature_type::pit:
        case feature_type::magma_pool:
        case feature_type::volcano:
            for (int i = layer_bottom[lfeature->end_depth];
                 i <= layer_top[lfeature->start_depth]; i++)
                tile.penalty[i] = std::min(0.4f, map_find(tile.penalty, i, 1.0f));
            break;
        default:
            break;
        }
    }

    return true;
}

void add_materials(EmbarkTileLayout &tile, matdata &data, float amount, int min_z, int max_z)
{
    for (int z = min_z; z <= max_z; z++)
        data.add(z, int(map_find(tile.penalty, z, 1)*amount));
}

bool estimate_materials(color_ostream &out, EmbarkTileLayout &tile, MatMap &layerMats, MatMap &veinMats)
{
    using namespace geo_layer_type;

    df::world_geo_biome *geo_biome = tile.geo_biome;

    if (!geo_biome)
    {
        out.printerr("Region geo-biome not found: (%d,%d)\n",
                     tile.biome_pos.x, tile.biome_pos.y);
        return false;
    }

    // soil depth increases by 1 every 5 levels below 150
    unsigned nlayers = std::min<unsigned>(16, geo_biome->layers.size());
    int soil_size = 0;

    for (unsigned i = 0; i < nlayers; i++)
    {
        auto layer = geo_biome->layers[i];
        if (layer->type == SOIL || layer->type == SOIL_SAND)
            soil_size += layer->top_height - layer->bottom_height + 1;
    }

    // Compute shifts for layers in the stack
    int soil_erosion = soil_size - std::min(soil_size,tile.max_soil_depth);
    int layer_shift[16];
    int cur_shift = tile.elevation+soil_erosion-1;

    for (unsigned i = 0; i < nlayers; i++)
    {
        auto layer = geo_biome->layers[i];
        layer_shift[i] = cur_shift;

        if (layer->type == SOIL || layer->type == SOIL_SAND)
        {
            int size = layer->top_height - layer->bottom_height + 1;

            // This is to replicate the behavior of a probable bug in the
            // map generation code: if a layer is partially eroded, the
            // removed levels are in fact transferred to the layer below,
            // because unlike the case of removing the whole layer, the code
            // does not execute a loop to shift the lower part of the stack up.
            if (size > soil_erosion)
                cur_shift -= soil_erosion;

            soil_erosion -= std::min(soil_erosion, size);
        }
    }

    // Estimate amounts
    int last_bottom = tile.elevation;

    for (unsigned i = 0; i < nlayers; i++)
    {
        auto layer = geo_biome->layers[i];

        int top_z = last_bottom-1;
        int bottom_z = std::max(layer->bottom_height + layer_shift[i], tile.min_z);
        if (i+1 == nlayers) // stretch layer if needed
            bottom_z = tile.min_z;
        if (top_z < bottom_z)
            continue;

        last_bottom = bottom_z;

        float layer_size = 48*48;

        int sums[ENUM_LAST_ITEM(inclusion_type)+1] = { 0 };

        for (unsigned j = 0; j < layer->vein_mat.size(); j++)
            if (is_valid_enum_item<df::inclusion_type>(layer->vein_type[j]))
                sums[layer->vein_type[j]] += layer->vein_unk_38[j];

        for (unsigned j = 0; j < layer->vein_mat.size(); j++)
        {
            // TODO: find out how to estimate the real density
            // this code assumes that vein_unk_38 is the weight
            // used when choosing the vein material
            float size = float(layer->vein_unk_38[j]);
            df::inclusion_type type = layer->vein_type[j];

            switch (type)
            {
            case inclusion_type::VEIN:
                // 3 veins of 80 tiles avg
                size = size * 80 * 3 / sums[type];
                break;
            case inclusion_type::CLUSTER:
                // 1 cluster of 700 tiles avg
                size = size * 700 * 1 / sums[type];
                break;
            case inclusion_type::CLUSTER_SMALL:
                size = size * 6 * 7 / sums[type];
                break;
            case inclusion_type::CLUSTER_ONE:
                size = size * 1 * 5 / sums[type];
                break;
            default:
                // shouldn't actually happen
                size = 1;
            }

            layer_size -= size;

            add_materials(tile, veinMats[layer->vein_mat[j]], size, bottom_z, top_z);
        }

        add_materials(tile, layerMats[layer->mat_index], layer_size, bottom_z, top_z);
    }

    return true;
}

static command_result embark_prospector(color_ostream &out, df::viewscreen_choose_start_sitest *screen,
                                        bool showHidden, bool showValue)
{
    if (!world || !world->world_data)
    {
        out.printerr("World data is not available.\n");
        return CR_FAILURE;
    }

    df::world_data *data = world->world_data;
    coord2d cur_region = screen->location.region_pos;
    auto cur_details = get_details(data, cur_region);

    if (!cur_details)
    {
        out.printerr("Current region details are not available.\n");
        return CR_FAILURE;
    }

    // Compute material maps
    MatMap layerMats;
    MatMap veinMats;
    matdata world_bottom;

    // Compute biomes
    std::map<coord2d, int> biomes;

    /*if (screen->biome_highlighted)
    {
        out.print("Processing one embark tile of biome F%d.\n\n", screen->biome_idx+1);
        biomes[screen->biome_rgn[screen->biome_idx]]++;
    }*/

    for (int x = screen->location.embark_pos_min.x; x <= screen->location.embark_pos_max.x; x++)
    {
        for (int y = screen->location.embark_pos_min.y; y <= screen->location.embark_pos_max.y; y++)
        {
            EmbarkTileLayout tile;
            if (!estimate_underground(out, tile, cur_details, x, y) ||
                !estimate_materials(out, tile, layerMats, veinMats))
                return CR_FAILURE;

            world_bottom.add(tile.base_z, 0);
            world_bottom.add(tile.elevation-1, 0);
        }
    }

    // Print the report
    out << "Layer materials:" << std::endl;
    printMats<df::inorganic_raw, shallower>(out, layerMats, world->raws.inorganics, showValue);

    if (showHidden) {
        DFHack::Materials *mats = Core::getInstance().getMaterials();
        printVeins(out, veinMats, mats, showValue);
        mats->Finish();
    }

    out << "Embark depth: " << (world_bottom.upper_z-world_bottom.lower_z+1) << " ";
    printMatdata(out, world_bottom, true);

    out << std::endl << "Warning: the above data is only a very rough estimate." << std::endl;

    return CR_OK;
}

command_result prospector (color_ostream &con, vector <string> & parameters)
{
    bool showHidden = false;
    bool showPlants = true;
    bool showSlade = true;
    bool showTemple = true;
    bool showValue = false;
    bool showTube = false;

    for(size_t i = 0; i < parameters.size();i++)
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
        else
            return CR_WRONG_USAGE;
    }

    CoreSuspender suspend;

    // Embark screen active: estimate using world geology data
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    if (screen)
        return embark_prospector(con, screen, showHidden, showValue);

    if (!Maps::IsValid())
    {
        con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    uint32_t x_max = 0, y_max = 0, z_max = 0;
    Maps::getSize(x_max, y_max, z_max);
    MapExtras::MapCache map;

    DFHack::Materials *mats = Core::getInstance().getMaterials();

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
                if (!b || !b->is_valid())
                {
                    continue;
                }

                // Find features
                b->GetGlobalFeature(&blockFeatureGlobal);
                b->GetLocalFeature(&blockFeatureLocal);

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

                        df::tiletype type = b->tiletypeAt(coord);
                        df::tiletype_shape tileshape = tileShape(type);
                        df::tiletype_material tilemat = tileMaterial(type);

                        // We only care about these types
                        switch (tileshape)
                        {
                        case tiletype_shape::WALL:
                        case tiletype_shape::FORTIFICATION:
                            break;
                        case tiletype_shape::EMPTY:
                            /* A heuristic: tubes inside adamantine have EMPTY:AIR tiles which
                               still have feature_local set. Also check the unrevealed status,
                               so as to exclude any holes mined by the player. */
                            if (tilemat == tiletype_material::AIR &&
                                des.bits.feature_local && des.bits.hidden &&
                                blockFeatureLocal.type == feature_type::deep_special_tube)
                            {
                                tubeTiles.add(global_z);
                            }
                        default:
                            continue;
                        }

                        // Count the material type
                        baseMats[tilemat].add(global_z);

                        // Find the type of the tile
                        switch (tilemat)
                        {
                        case tiletype_material::SOIL:
                        case tiletype_material::STONE:
                            layerMats[b->layerMaterialAt(coord)].add(global_z);
                            break;
                        case tiletype_material::MINERAL:
                            veinMats[b->veinMaterialAt(coord)].add(global_z);
                            break;
                        case tiletype_material::FEATURE:
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
                        case tiletype_material::LAVA_STONE:
                            // TODO ?
                            break;
                        default:
                            break;
                        }
                    }
                }

                // Check plants this way, as the other way wasn't getting them all
                // and we can check visibility more easily here
                if (showPlants)
                {
                    auto block = Maps::getBlockColumn(b_x,b_y);
                    vector<df::plant *> *plants = block ? &block->plants : NULL;
                    if(plants)
                    {
                        for (PlantList::const_iterator it = plants->begin(); it != plants->end(); it++)
                        {
                            const df::plant & plant = *(*it);
                            if (plant.pos.z != z)
                                continue;
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
        con << std::setw(25) << ENUM_KEY_STR(tiletype_material,(df::tiletype_material)it->first) << " : " << it->second.count << std::endl;
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
    printMats<df::inorganic_raw, shallower>(con, layerMats, world->raws.inorganics, showValue);

    printVeins(con, veinMats, mats, showValue);

    if (showPlants)
    {
        con << "Shrubs:" << std::endl;
        printMats<df::plant_raw, std::greater>(con, plantMats, world->raws.plants.all, showValue);
        con << "Wood in trees:" << std::endl;
        printMats<df::plant_raw, std::greater>(con, treeMats, world->raws.plants.all, showValue);
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
