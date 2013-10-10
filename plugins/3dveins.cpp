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

using namespace df::enums;

using namespace DFHack;
using namespace MapExtras;

using df::global::world;

command_result cmd_3dveins(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("3dveins");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "3dveins", "Rewrites the veins to make them extend in 3D space.",
        cmd_3dveins, false,
        "  Run this after embark to change all veins on the map to a shape\n"
        "  that consistently spans Z levels. The operation preserves the\n"
        "  mineral counts reported by prospect.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

/*
 * Data structures.
 */

template<class T>
class BlockGrid
{
    df::coord dim;
    std::vector<T> buf;
public:
    BlockGrid(df::coord size) : dim(size) {
        buf.resize(dim.x * dim.y * dim.z);
    }
    BlockGrid(df::coord2d size, int zdepth = 1)
        : dim(df::coord(size.x, size.y, zdepth))
    {
        buf.resize(dim.x * dim.y * dim.z);
    }

    const df::coord &size() { return dim; }

    void resize_depth(int16_t zdepth) {
        dim.z = zdepth;
        buf.resize(dim.x * dim.y * dim.z);
    }
    void shift_depth(int16_t to_add) {
        dim.z += to_add;
        buf.insert(buf.begin(), dim.x*dim.y*to_add, T());
    }

    T &operator() (int x, int y, int z = 0) {
        return buf[x + dim.x*(y + dim.y*z)];
    }
    T &operator() (df::coord pos) {
        return buf[pos.x + dim.x*(pos.y + dim.y*pos.z)];
    }
    T &operator() (df::coord2d pos, int z = 0) {
        return buf[pos.x + dim.x*(pos.y + dim.y*z)];
    }
};

enum SpecialMatCodes {
    // Tile not mapped to an actual tile
    SMC_NO_MAPPING = -1,
    // Tile not assigned to a vein yet
    SMC_LAYER = -2
};

struct GeoLayer;
struct GeoColumn;
struct GeoBiome;

typedef std::pair<int,df::inclusion_type> t_veinkey;

/* Representation of a block in geolayer coordinate space.
 * That is, it represents a block that would have happened
 * if geological layers weren't shifted in Z direction to
 * conform to terrain height. Computing veins in this system
 * of coordinates should make them nicely follow the layers.
 */
struct GeoBlock
{
    GeoLayer *layer;
    GeoColumn *column;
    df::coord pos;

    df::tile_bitmask unmined;
    int16_t material[16][16];
    uint8_t veintype[16][16];
    float weight[16][16];

    GeoBlock(GeoLayer *parent, df::coord pos) : layer(parent), pos(pos) {
        memset(material, -1, sizeof(material));
    }
};

struct GeoColumn
{
    // Original Z level values for each layer
    int16_t min_level[16][16][BiomeInfo::MAX_LAYERS];
    int16_t max_level[16][16][BiomeInfo::MAX_LAYERS];
    int8_t top_layer[16][16];
    int8_t bottom_layer[16][16];
    int8_t top_solid_z[16][16];

    GeoColumn()
    {
        memset(min_level, -1, sizeof(min_level));
        memset(max_level, -1, sizeof(max_level));
        memset(top_layer, -1, sizeof(top_layer));
        memset(bottom_layer, -1, sizeof(bottom_layer));
        memset(top_solid_z, -1, sizeof(top_solid_z));
    }
};

struct GeoLayer
{
    GeoBiome *biome;
    int index;

    df::world_geo_layer *info;
    int thickness, z_bias;

    int16_t material;
    bool is_soil;

    // World-global origin coordinates in blocks
    df::coord world_pos;

    df::coord2d size;
    BlockGrid<GeoBlock*> blocks;
    std::vector<GeoBlock*> block_list;

    int tiles, unmined_tiles, mineral_tiles;
    std::map<t_veinkey,int> mineral_count;

    GeoLayer(GeoBiome *parent, int index, df::world_geo_layer *info);

    ~GeoLayer() {
        for (size_t i = 0; i < block_list.size(); i++)
            delete block_list[i];
    }

    GeoBlock *blockAt(df::coord pos)
    {
        assert(pos.z >= 0);
        if (pos.z >= blocks.size().z)
            blocks.resize_depth(pos.z+1);

        GeoBlock *&blk = blocks(pos);
        if (!blk) {
            blk = new GeoBlock(this, pos);
            block_list.push_back(blk);
        }
        return blk;
    }

    GeoBlock *getBlockAt(df::coord pos)
    {
        if (pos.z < 0 || pos.z >= blocks.size().z)
            return NULL;

        return blocks(pos);
    }

    void setZBias(int bias)
    {
        if (bias <= z_bias) return;
        blocks.shift_depth(bias - z_bias);
        z_bias = bias;
    }

    void print_mineral_stats(color_ostream &out)
    {
        for (auto it = mineral_count.begin(); it != mineral_count.end(); ++it)
            out << "    " << MaterialInfo(0,it->first.first).getToken()
                << " " << ENUM_KEY_STR(inclusion_type,it->first.second)
                << ": \t\t" << it->second << " (" << (float(it->second)/unmined_tiles) << ")" << std::endl;

        out.print("    Total tiles: %d (%d unmined)\n", tiles, unmined_tiles);
    }
};

struct GeoBiome
{
    const BiomeInfo &info;

    df::coord2d world_pos;

    df::coord2d size;
    BlockGrid<GeoColumn> columns;

    std::vector<GeoLayer*> layers;

    GeoBiome(const BiomeInfo &biome, df::coord2d base, df::coord2d mapsize)
        : info(biome), world_pos(base), size(mapsize), columns(mapsize)
    {
    }

    ~GeoBiome()
    {
        for (size_t i = 0; i < layers.size(); i++)
            delete layers[i];
    }

    bool init_layers();

    void print_mineral_stats(color_ostream &out)
    {
        out.print("Geological biome %d:\n", info.geo_index);

        for (size_t i = 0; i < layers.size(); i++)
            if (layers[i])
            {
                out << "  Layer " << i << std::endl;
                layers[i]->print_mineral_stats(out);
            }
    }
};

GeoLayer::GeoLayer(GeoBiome *parent, int index, df::world_geo_layer *info)
    : biome(parent), index(index), info(info),
      world_pos(parent->world_pos.x, parent->world_pos.y, -info->top_height),
      size(parent->size), blocks(parent->size)
{
    thickness = info->top_height - info->bottom_height + 1;
    z_bias = 0;
    tiles = unmined_tiles = mineral_tiles = 0;
    material = info->mat_index;
    is_soil = isSoilInorganic(material);
}

struct VeinGenerator
{
    color_ostream &out;
    MapCache map;

    df::coord2d size;
    df::coord2d base;

    std::map<int, GeoBiome*> biomes;
    std::vector<GeoBiome*> biome_by_idx;

    VeinGenerator(color_ostream &out) : out(out) {}

    ~VeinGenerator() {
        for (auto it = biomes.begin(); it != biomes.end(); ++it)
            delete it->second;
    }

    GeoLayer *mapLayer(Block *pb, df::coord2d tile);

    bool init_biomes();

    bool scan_tiles();
    bool scan_layer_depth(Block *b, df::coord2d column, int z);
    bool adjust_layer_depth(df::coord2d column);
    bool scan_block_tiles(Block *b, df::coord2d column, int z);

    void write_tiles();
    void write_block_tiles(Block *b, df::coord2d column, int z);

    void print_mineral_stats()
    {
        for (auto it = biomes.begin(); it != biomes.end(); ++it)
            it->second->print_mineral_stats(out);
    }
};

/*
 * General initialization
 */

bool VeinGenerator::init_biomes()
{
    biome_by_idx.resize(map.getBiomeCount());

    size = df::coord2d(map.maxBlockX()+1, map.maxBlockY()+1);
    base = df::coord2d(world->map.region_x*3, world->map.region_y*3);

    for (size_t i = 0; i < biome_by_idx.size(); i++)
    {
        const BiomeInfo &info = map.getBiomeByIndex(i);

        if (info.geo_index < 0 || !info.geobiome)
        {
            out.printerr("Biome %d is not defined.\n", i);
            return false;
        }

        GeoBiome *&biome = biomes[info.geo_index];
        if (!biome)
        {
            biome = new GeoBiome(info, base, size);

            if (!biome->init_layers())
                return false;
        }

        biome_by_idx[i] = biome;
    }

    return true;
}

bool GeoBiome::init_layers()
{
    auto &info_layers = info.geobiome->layers;

    layers.resize(info_layers.size());

    for (size_t i = 0; i < layers.size(); i++)
    {
        layers[i] = new GeoLayer(this, i, info_layers[i]);
    }

    return true;
}

/*
 * Initial statistics collection scan. It has to:
 *
 * 1) Find out how the layers are shifted in Z direction for each tile column.
 * 2) Record which tiles are available, which are unmined, and how much minerals are there.
 */

GeoLayer *VeinGenerator::mapLayer(Block *pb, df::coord2d tile)
{
    int idx = pb->biomeIndexAt(tile);
    GeoBiome *biome = biome_by_idx.at(idx);

    int lidx = pb->layerIndexAt(tile);
    if (unsigned(lidx) >= biome->layers.size())
        return NULL;

    return biome->layers[lidx];
}

bool VeinGenerator::scan_tiles()
{
    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            df::coord2d column(x,y);

            // First find where layers start and end
            for (int z = map.maxZ(); z >= 0; z--)
            {
                Block *b = map.BlockAt(df::coord(x,y,z));
                if (!b || !b->is_valid())
                    continue;

                if (!scan_layer_depth(b, column, z))
                    return false;
            }

            if (!adjust_layer_depth(column))
                return false;

            // Collect tile data
            for (int z = map.maxZ(); z >= 0; z--)
            {
                Block *b = map.BlockAt(df::coord(x,y,z));
                if (!b || !b->is_valid())
                    continue;

                if (!scan_block_tiles(b, column, z))
                    return false;

                map.discardBlock(b);
            }

            // Discard this column of parsed blocks
            map.trash();
        }
    }

    return true;
}

bool VeinGenerator::scan_layer_depth(Block *b, df::coord2d column, int z)
{
    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            df::coord2d tile(x,y);
            GeoLayer *layer = mapLayer(b, tile);
            if (!layer)
                continue;

            int idx = layer->index;

            auto &col_info = layer->biome->columns(column);
            auto &max_level = col_info.max_level[x][y];
            auto &min_level = col_info.min_level[x][y];
            auto &top = col_info.top_layer[x][y];
            auto &top_solid = col_info.top_solid_z[x][y];
            auto &bottom = col_info.bottom_layer[x][y];

            if (top_solid < 0 && isWallTerrain(b->baseTiletypeAt(tile)))
                top_solid = z;

            if (max_level[idx] < 0)
            {
                // Do not start the layer stack in open air.
                // Those tiles can be very weird.
                if (bottom < 0 && isOpenTerrain(b->baseTiletypeAt(tile)))
                    continue;

                max_level[idx] = min_level[idx] = z;

                if (top < 0 || idx < top)
                    top = idx;

                bottom = std::max<int8_t>(idx, bottom);
            }
            else
            {
                if (z != min_level[idx]-1 && min_level[idx] <= top_solid)
                {
                    out.printerr(
                        "Discontinuous layer %d at (%d,%d,%d).\n",
                        layer->index, x+column.x*16, y+column.y*16, z
                    );
                    return false;
                }

                min_level[idx] = z;
            }
        }
    }

    return true;
}

bool VeinGenerator::adjust_layer_depth(df::coord2d column)
{
    for (auto it = biomes.begin(); it != biomes.end(); ++it)
    {
        GeoBiome *biome = it->second;
        auto &col_info = biome->columns(column);

        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                auto &max_level = col_info.max_level[x][y];
                auto &min_level = col_info.min_level[x][y];
                auto top_solid = col_info.top_solid_z[x][y];

                int min_defined = col_info.top_layer[x][y];
                int max_defined = col_info.bottom_layer[x][y];
                if (max_defined < 0)
                    continue;

                // Verify assumptions
                for (int i = min_defined; i < max_defined; i++)
                {
                    if (max_level[i+1] < 0 && min_level[i] > top_solid)
                        max_level[i+1] = min_level[i+1] = min_level[i];

                    if (max_level[i+1] > top_solid)
                        continue;

                    if (max_level[i+1] != min_level[i]-1)
                    {
                        out.printerr(
                            "Gap or overlap with next layer %d at (%d,%d,%d-%d).\n",
                            i+1, x+column.x*16, y+column.y*16, max_level[i+1], min_level[i]
                        );
                        return false;
                    }
                }

                for (int i = min_defined; i < max_defined; i++)
                {
                    auto layer = biome->layers[i];
                    int size = max_level[i]-min_level[i]+1;

                    if (size == layer->thickness)
                        continue;

                    // Adjust the top layers so that the bottom of the layer maps to the correct place
                    if (max_level[i] >= top_solid)
                    {
                        max_level[i] += layer->thickness - size;

                        if (size > layer->thickness)
                            layer->setZBias(size - layer->thickness);

                        continue;
                    }

                    out.printerr(
                        "Layer height change in layer %d at (%d,%d,%d): %d instead of %d.\n",
                        i, x+column.x*16, y+column.y*16, max_level[i],
                        size, biome->layers[i]->thickness
                    );
                    return false;
                }
            }
        }
    }

    return true;
}

bool VeinGenerator::scan_block_tiles(Block *b, df::coord2d column, int z)
{
    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            df::coord2d tile(x,y);
            GeoLayer *layer = mapLayer(b, tile);
            if (!layer)
                continue;

            auto tt = b->baseTiletypeAt(tile);
            bool wall = isWallTerrain(tt);
            bool matches = false;

            switch (tileMaterial(tt))
            {
                case tiletype_material::MINERAL:
                    matches = true;
                    // Count minerals
                    if (wall)
                    {
                        layer->mineral_tiles++;
                        layer->mineral_count[t_veinkey(b->veinMaterialAt(tile),b->veinTypeAt(tile))]++;
                    }
                    break;

                case tiletype_material::SOIL:
                    matches = layer->is_soil;
                    break;

                case tiletype_material::STONE:
                    matches = !layer->is_soil;
                    break;

                default:;
            }

            // This tile should be overwritten
            if (matches)
            {
                auto &col_info = layer->biome->columns(column);
                int max_level = col_info.max_level[x][y][layer->index] + layer->z_bias;
                GeoBlock *block = layer->blockAt(df::coord(column, max_level-z));

                block->material[x][y] = SMC_LAYER;
                layer->tiles++;

                if (wall)
                {
                    layer->unmined_tiles++;
                    block->unmined.setassignment(x,y,true);
                }
            }
        }
    }

    return true;
}

/*
 * Vein writing pass. Without running generation it just deletes all veins.
 */

void VeinGenerator::write_tiles()
{
    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            df::coord2d column(x,y);

            for (int z = map.maxZ(); z >= 0; z--)
            {
                Block *b = map.BlockAt(df::coord(x,y,z));
                if (!b || !b->is_valid())
                    continue;

                write_block_tiles(b, column, z);

                b->Write();
                map.discardBlock(b);
            }

            map.trash();
        }
    }
}

void VeinGenerator::write_block_tiles(Block *b, df::coord2d column, int z)
{
    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            df::coord2d tile(x,y);
            GeoLayer *layer = mapLayer(b, tile);
            if (!layer)
                continue;

            auto &col_info = layer->biome->columns(column);
            int max_level = col_info.max_level[x][y][layer->index] + layer->z_bias;
            GeoBlock *block = layer->getBlockAt(df::coord(column, max_level-z));

            auto tt = b->baseTiletypeAt(tile);

            if (block && block->material[x][y] != SMC_NO_MAPPING)
            {
                bool ok;
                int mat = block->material[x][y];
                df::inclusion_type vein = inclusion_type::CLUSTER;

                if (mat < 0)
                {
                    if (layer->is_soil)
                        ok = b->setSoilAt(tile, tt, true);
                    else
                        ok = b->setStoneAt(tile, tt, layer->material, vein, true, true);
                }
                else
                {
                    vein = (df::inclusion_type)block->veintype[x][y];
                    ok = b->setStoneAt(tile, tt, mat, vein, true, true);
                }

                if (!ok)
                {
                    out.printerr(
                        "Couldn't write %d vein at (%d,%d,%d)\n",
                        mat, x+column.x*16, y+column.y*16, z
                    );
                }
            }
            else
            {
                // Otherwise just kill latent veins if they happen to be there
                if (tileMaterial(tt) != tiletype_material::MINERAL)
                    b->setVeinMaterialAt(tile, -1);
            }
        }
    }
}

command_result cmd_3dveins(color_ostream &con, vector <string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    VeinGenerator generator(con);

    con.print("Collecting statistics...\n");

    if (!generator.init_biomes())
        return CR_FAILURE;
    if (!generator.scan_tiles())
        return CR_FAILURE;

    generator.print_mineral_stats();

    con.print("Writing tiles...\n");

    generator.write_tiles();

    return CR_OK;
}
