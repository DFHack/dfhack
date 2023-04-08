#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>
#include <math.h>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/MapCache.h"
#include "modules/Random.h"
#include "modules/World.h"

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

#ifdef LINUX_BUILD
#include <tr1/memory>
using std::tr1::shared_ptr;
#else
#include <memory>
using std::shared_ptr;
#endif

using namespace df::enums;

using namespace DFHack;
using namespace MapExtras;
using namespace DFHack::Random;

DFHACK_PLUGIN("3dveins");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(gametype);

command_result cmd_3dveins(color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "3dveins",
        "Rewrites the veins to make them extend in 3D space.",
        cmd_3dveins));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

/*
 * Vein density fields
 */

typedef std::pair<int,df::inclusion_type> t_veinkey;

struct NoiseFunction
{
    typedef shared_ptr<NoiseFunction> Ptr;
    typedef std::pair<float,float> t_range;

    virtual ~NoiseFunction() {};

    /*
     * Veins are placed by clipping the computed value
     * against a floating threshold, with values above
     * the threshold causing placement of a vein tile.
     */
    virtual float eval(float x, float y, float z) = 0;
    virtual t_range range() = 0;
    virtual void displace(float &x, float &y, float &z) = 0;
};

inline float apow(float a, float b) { return powf(fabsf(a), b); }

struct Distribution : NoiseFunction
{
    float bx, by, bz;

    Distribution(MersenneRNG &rng, float scale)
    {
        bx = rng.drandom() * scale;
        by = rng.drandom() * scale;
        bz = rng.drandom() * scale;
    }

    void displace(float &x, float &y, float &z) {
        x += bx; y += by; z += bz;
    }
};

struct DistributionVein : Distribution
{
    PerlinNoise3D<float> density1, density2;
    PerlinNoise3D<float> strand1a, strand1b;

    DistributionVein(MersenneRNG &rng) : Distribution(rng, 96) {
        density1.init(rng);
        density2.init(rng);
        strand1a.init(rng);
        strand1b.init(rng);
    }

    float eval(float x, float y, float z) {
        return 0.1f * density1(x/96, y/96, z/48)
             + 0.2f * density2(x/48, y/48, z/24)
             - apow(      strand1a(x/24,y/24,z/12)
                    +0.6f*strand1b(x/16,y/16,z/8), 0.6f);
    }

    t_range range() { return t_range(-0.3f-1.33f,0.3f); }
};

struct DistributionCluster : Distribution
{
    PerlinNoise3D<float> density1, density2, shape;

    DistributionCluster(MersenneRNG &rng) : Distribution(rng, 96) {
        density1.init(rng);
        density2.init(rng);
        shape.init(rng);
    }

    float eval(float x, float y, float z) {
        return 0.2f * density1(x/96, y/96, z/32)
             + 0.6f * density2(x/48, y/48, z/16)
             + shape(x/24, y/24, z/8);
    }

    t_range range() { return t_range(-1.8f,1.8f); }
};

struct DistributionClusterSmall : Distribution
{
    PerlinNoise3D<float> density1, density2, shape;

    DistributionClusterSmall(MersenneRNG &rng) : Distribution(rng, 96) {
        density1.init(rng);
        density2.init(rng);
        shape.init(rng);
    }

    float eval(float x, float y, float z) {
        const float scale = 1.0f/4.3f;
        return 0.06f * density1(x/96, y/96, z/48)
             + 0.12f * density2(x/24, y/24, z/12)
             + apow(shape(x*scale, y*scale, z*scale), 0.1f);
    }

    t_range range() { return t_range(-0.18f,1.18f); }
};

struct DistributionClusterOne : Distribution
{
    PerlinNoise3D<float> density1, density2, shape;

    DistributionClusterOne(MersenneRNG &rng) : Distribution(rng, 96) {
        density1.init(rng);
        density2.init(rng);
        shape.init(rng);
    }

    float eval(float x, float y, float z) {
        return 0.05f * density1(x/96, y/96, z/48)
             + 0.1f * density2(x/48, y/48, z/24)
             + shape(x-bx, y-by, z-bz);
    }

    t_range range() { return t_range(-1.15f,1.15f); }
};

static NoiseFunction *makeVeinDistribution(t_veinkey vein, MersenneRNG &rng)
{
    using namespace df::enums::inclusion_type;

    switch (vein.second)
    {
        case VEIN:
            return new DistributionVein(rng);
        case CLUSTER_SMALL:
            return new DistributionClusterSmall(rng);
        case CLUSTER_ONE:
            return new DistributionClusterOne(rng);
        case CLUSTER:
        default:
            return new DistributionCluster(rng);
    }
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

    uint16_t arena_mask, arena_unmined;
    int16_t arena_material;

    df::tile_bitmask unmined;
    int16_t material[16][16];
    uint8_t veintype[16][16];
    float weight[16][16];

    GeoBlock(GeoLayer *parent, df::coord pos) : layer(parent), pos(pos) {
        memset(material, -1, sizeof(material));
    }

    bool prepare_arena(int16_t env_material, NoiseFunction::Ptr fn);
    int measure_placement(float threshold);
    void place_tiles(float threshold, int16_t new_material, df::inclusion_type itype);
};

/*
 * A set of layers that have the same vein type, distribution
 * and approximate location, and thus should be placed using
 * one binsearch pass for more uniform appearance.
 */
struct VeinExtent
{
    typedef shared_ptr<VeinExtent> Ptr;
    typedef std::vector<Ptr> PVec;

    t_veinkey vein;
    int probability, num_tiles;
    Ptr parent;
    int parent_depth;

    bool placed;
    int placed_tiles;
    NoiseFunction::Ptr distribution;

    int num_unmined, num_layer, min_z, max_z;
    std::vector<GeoLayer*> layers;

    VeinExtent(t_veinkey vein) : vein(vein) {
        probability = num_tiles = placed_tiles = 0;
        num_unmined = num_layer = 0;
        min_z = max_z = 0;
        parent_depth = 0;
        placed = false;
    }

    float density() { return float(num_tiles) / num_unmined; }

    void set_parent(Ptr pp) {
        if (parent)
            parent->add_tiles(-num_tiles);
        parent = pp;
        if (parent)
            parent->add_tiles(num_tiles);

        parent_depth = (pp ? pp->parent_depth+1 : 0);
    }

    int parent_mat() {
        return parent ? parent->vein.first : SMC_LAYER;
    }

    void add_tiles(int tiles) {
        num_tiles += tiles;
        if (parent)
            parent->add_tiles(tiles);
    }

    bool is_similar(Ptr ext2) {
        return probability == ext2->probability &&
               parent_mat() == ext2->parent_mat();
    }

    void link(GeoLayer *layer);
    void merge_into(VeinExtent::Ptr ext2);

    void place_tiles();
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
    int aquifer_depth;

    int16_t material;
    bool is_soil;
    bool is_soil_layer;

    // World-global origin coordinates in blocks
    df::coord world_pos;

    int min_z() { return world_pos.z - z_bias; }
    int max_z() { return world_pos.z + thickness - 1; }

    df::coord2d size;
    BlockGrid<GeoBlock*> blocks;
    std::vector<GeoBlock*> block_list;

    int tiles, unmined_tiles, mineral_tiles;
    std::map<t_veinkey,int> mineral_count;
    std::map<t_veinkey, VeinExtent::Ptr> veins;

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

    bool form_veins(color_ostream &out);
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
    aquifer_depth = 0;
    tiles = unmined_tiles = mineral_tiles = 0;
    material = info->mat_index;
    is_soil = isSoilInorganic(material);
    is_soil_layer = (info->type == geo_layer_type::SOIL || info->type == geo_layer_type::SOIL_SAND);
}

const unsigned NUM_INCLUSIONS = 1+(int)ENUM_LAST_ITEM(inclusion_type);

struct VeinGenerator
{
    color_ostream &out;
    MapCache map;

    df::coord2d size;
    df::coord2d base;

    std::map<int, GeoBiome*> biomes;
    std::vector<GeoBiome*> biome_by_idx;

    struct VMats {
        bool can_support_aquifer;
        int8_t default_type;

        int8_t valid_type[NUM_INCLUSIONS];
        uint32_t seeds[NUM_INCLUSIONS];
        NoiseFunction::Ptr funcs[NUM_INCLUSIONS];

        VMats()
        {
            default_type = -2;
            memset(valid_type, -1, sizeof(valid_type));
        }
    };
    std::vector<VMats> materials;

    std::map<t_veinkey, VeinExtent::PVec> veins;

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

    bool form_veins();
    bool place_orphan(t_veinkey vein, int size, GeoLayer *from);

    void init_seeds();
    NoiseFunction::Ptr get_noise(t_veinkey vein);

    bool place_veins(bool verbose);

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
    auto &mats = df::inorganic_raw::get_vector();
    materials.resize(world->raws.inorganics.size());

    for (size_t i = 0; i < mats.size(); i++)
    {
        materials[i].can_support_aquifer = mats[i]->flags.is_set(inorganic_flags::AQUIFER);

        // Be silent about slade veins, which can happen below hell
        if (mats[i]->flags.is_set(inorganic_flags::DEEP_SURFACE))
            materials[i].valid_type[0] = -2;
    }

    biome_by_idx.resize(map.getBiomeCount());

    size = df::coord2d(map.maxBlockX()+1, map.maxBlockY()+1);
    base = df::coord2d(world->map.region_x*3, world->map.region_y*3);

    for (size_t i = 0; i < biome_by_idx.size(); i++)
    {
        const BiomeInfo &info = map.getBiomeByIndex(i);

        if (info.geo_index < 0 || !info.geobiome)
        {
            out.printerr("Biome %zd is not defined.\n", i);
            return false;
        }

        GeoBiome *&biome = biomes[info.geo_index];
        if (!biome)
        {
            biome = new GeoBiome(info, base, size);

            if (!biome->init_layers())
                return false;

            // Mark valid vein types
            auto &layers = info.geobiome->layers;

            for (size_t i = 0; i < layers.size(); i++)
            {
                auto layer = layers[i];

                for (size_t j = 0; j < layer->vein_mat.size(); j++)
                {
                    if (unsigned(layer->vein_mat[j]) >= materials.size() ||
                        unsigned(layer->vein_type[j]) >= NUM_INCLUSIONS)
                        continue;

                    auto &minfo = materials[layer->vein_mat[j]];
                    int type = layer->vein_type[j];
                    minfo.valid_type[type] = type;
                    if (minfo.default_type < 0 || type == inclusion_type::CLUSTER)
                        minfo.default_type = type;
                }
            }
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

static bool isTransientMaterial(df::tiletype tile)
{
    using namespace df::enums::tiletype_material;

    switch (tileMaterial(tile))
    {
        case AIR:
        case LAVA_STONE:
        case PLANT:
        case ROOT:
        case TREE:
        case MUSHROOM:
            return true;

        default:
            return false;
    }
}

static bool isSkyBlock(Block *b)
{
    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            df::coord2d tile(x,y);
            auto dsgn = b->DesignationAt(tile);
            auto ttype = b->baseTiletypeAt(tile);

            if (dsgn.bits.subterranean || !dsgn.bits.light || !isTransientMaterial(ttype))
                return false;
        }
    }

    return true;
}

static int findTopBlock(MapCache &map, int x, int y)
{
    for (int z = map.maxZ(); z >= 0; z--)
    {
        Block *b = map.BlockAt(df::coord(x,y,z));
        if (b && b->is_valid() && !isSkyBlock(b))
            return z;
    }

    return -1;
}

bool VeinGenerator::scan_tiles()
{
    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            df::coord2d column(x,y);

            int top = findTopBlock(map, x, y);

            // First find where layers start and end
            for (int z = top; z >= 0; z--)
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
            for (int z = top; z >= 0; z--)
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

            auto ttype = b->baseTiletypeAt(tile);
            bool obsidian = isTransientMaterial(ttype);

            if (top_solid < 0 && !obsidian && isWallTerrain(ttype))
                top_solid = z;

            if (max_level[idx] < 0)
            {
                // Do not start the layer stack in open air.
                // Those tiles can be very weird.
                if (bottom < 0 && (isOpenTerrain(ttype) || obsidian))
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

                int last_top = min_defined;

                // Verify assumptions
                for (int i = min_defined; i < max_defined; i++)
                {
                    if (max_level[i] >= top_solid)
                        last_top = i;

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

                    // If below a thick soil layer, allow thickness to pass from prev to current.
                    // This accounts for a probable bug in worldgen soil placement code.
                    if (i > min_defined && i-1 <= last_top)
                    {
                        auto prev = biome->layers[i-1];

                        if (size > layer->thickness &&
                            prev->is_soil_layer && prev->thickness > 1 &&
                            size <= layer->thickness+prev->thickness-1)
                        {
                            max_level[i] += layer->thickness - size;
                            layer->setZBias(size - layer->thickness);
                            continue;
                        }
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
    bool aquifer = b->getRaw()->flags.bits.has_aquifer;

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
                {
                    t_veinkey key(b->veinMaterialAt(tile),b->veinTypeAt(tile));

                    // Check if the vein material and type is reasonable
                    if (unsigned(key.first) >= materials.size() ||
                        unsigned(key.second) >= NUM_INCLUSIONS)
                    {
                        out.printerr("Invalid vein code: %d %d - aborting.\n",key.first,key.second);
                        return false;
                    }

                    int8_t &status = materials[key.first].valid_type[key.second];

                    if (status == -1)
                    {
                        // Report first occurence of unreasonable vein spec
                        out.printerr(
                            "Unexpected vein %s %s - ",
                            MaterialInfo(0,key.first).getToken().c_str(),
                            ENUM_KEY_STR(inclusion_type, key.second).c_str()
                        );

                        status = materials[key.first].default_type;
                        if (status < 0)
                            out.printerr("will be left in place.\n");
                        else
                            out.printerr(
                                "correcting to %s.\n",
                                ENUM_KEY_STR(inclusion_type, df::inclusion_type(status)).c_str()
                            );
                    }

                    if (status >= 0)
                    {
                        key.second = df::inclusion_type(status);
                        matches = true;

                        // Count minerals
                        if (wall)
                        {
                            layer->mineral_tiles++;
                            layer->mineral_count[key]++;
                        }
                    }
                    break;
                }

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

                if (aquifer && b->getFlagAt(tile, df::tile_designation::mask_water_table))
                {
                    int depth = z - col_info.min_level[x][y][layer->index] + 1;
                    layer->aquifer_depth = std::max(layer->aquifer_depth, depth);
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

            int top = findTopBlock(map, x, y);

            for (int z = top; z >= 0; z--)
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
    bool aquifer = b->getRaw()->flags.bits.has_aquifer;

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

                bool aquifer_tile = aquifer
                                 && (z-col_info.min_level[x][y][layer->index]) < layer->aquifer_depth;

                if (mat < 0)
                {
                    if (layer->is_soil)
                        ok = b->setSoilAt(tile, tt, true);
                    else
                        ok = b->setStoneAt(tile, tt, layer->material, vein, true, true);
                }
                else
                {
                    aquifer_tile = aquifer_tile && materials[mat].can_support_aquifer;

                    vein = (df::inclusion_type)block->veintype[x][y];
                    ok = b->setStoneAt(tile, tt, mat, vein, true, true);
                }

                if (aquifer)
                    b->setFlagAt(tile, df::tile_designation::mask_water_table, aquifer_tile);

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

/*
 * Vein placement code
 */

bool GeoBlock::prepare_arena(int16_t basemat, NoiseFunction::Ptr fn)
{
    arena_mask = arena_unmined = 0;
    arena_material = basemat;

    df::coord origin = pos + layer->world_pos;
    float x0 = float(origin.x)*16 + 0.5f, y0 = float(origin.y)*16 + 0.5f;
    float z = origin.z - layer->z_bias + 0.5f;

    fn->displace(x0, y0, z);

    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            if (material[x][y] != arena_material)
                continue;

            weight[x][y] = fn->eval(x0+x, y0+y, z);

            arena_mask |= (1<<x);
            if (unmined.getassignment(x,y))
                arena_unmined |= (1<<x);
        }
    }

    return arena_mask != 0;
}

int GeoBlock::measure_placement(float threshold)
{
    if (!arena_unmined)
        return 0;

    int count = 0;

    for (int x = 0; x < 16; x++)
    {
        if ((arena_unmined & (1<<x)) == 0)
            continue;

        for (int y = 0; y < 16; y++)
        {
            if (material[x][y] != arena_material || weight[x][y] < threshold)
                continue;

            if (unmined.getassignment(x,y))
                count++;
        }
    }

    return count;
}

void GeoBlock::place_tiles(float threshold, int16_t new_material, df::inclusion_type itype)
{
    for (int x = 0; x < 16; x++)
    {
        if ((arena_mask & (1<<x)) == 0)
            continue;

        for (int y = 0; y < 16; y++)
        {
            if (material[x][y] != arena_material || weight[x][y] < threshold)
                continue;

            material[x][y] = new_material;
            veintype[x][y] = itype;
        }
    }
}

static int measure(const std::vector<GeoBlock*> &arena, float threshold)
{
    int count = 0;
    for (size_t i = 0; i < arena.size(); i++)
        count += arena[i]->measure_placement(threshold);
    return count;
}

void VeinExtent::link(GeoLayer *layer)
{
    layers.push_back(layer);

    num_unmined += layer->unmined_tiles;
    num_layer += layer->tiles;

    if (layers.size() == 1)
    {
        min_z = layer->min_z();
        max_z = layer->max_z();
    }
    else
    {
        min_z = std::min(min_z, layer->min_z());
        max_z = std::max(max_z, layer->max_z());
    }
}

void VeinExtent::merge_into(Ptr target)
{
    assert(target->vein == vein);

    target->num_tiles += num_tiles;

    for (size_t i = 0; i < layers.size(); i++)
    {
        target->link(layers[i]);

        layers[i]->veins[vein] = target;
    }

    num_tiles = 0;
    layers.clear();
}

void VeinExtent::place_tiles()
{
    std::vector<GeoBlock*> arena;

    int env_material = parent_mat();

    for (size_t i = 0; i < layers.size(); i++)
    {
        auto layer = layers[i];

        for (auto bit = layer->block_list.begin(); bit != layer->block_list.end(); ++bit)
        {
            if ((*bit)->prepare_arena(env_material, distribution))
                arena.push_back(*bit);
        }
    }

    // Binary search to meet the required number
    auto range = distribution->range();
    float mid;

    for (int i = 0; i < 32; i++) // iteration limit
    {
        mid = (range.first + range.second) / 2;
        int count = placed_tiles = measure(arena, mid);

        if (count == num_tiles)
            break;
        else if (count > num_tiles)
            range.first = mid;
        else
            range.second = mid;
    }

    // Write the tiles out
    for (size_t i = 0; i < arena.size(); i++)
        arena[i]->place_tiles(mid, vein.first, vein.second);

    placed = true;
}

bool GeoLayer::form_veins(color_ostream &out)
{
    std::vector<VeinExtent::Ptr> refs;

    // Defunct layers cannot have veins
    if (tiles <= 0)
        return true;

    for (size_t i = 0; i < info->vein_mat.size(); i++)
    {
        int parent_id = info->vein_nested_in[i];

        if (parent_id >= (int)refs.size())
        {
            out.printerr("Forward vein reference in biome %d.\n", biome->info.geo_index);
            return false;
        }

        t_veinkey key(info->vein_mat[i], info->vein_type[i]);
        VeinExtent::Ptr &vptr = veins[key];

        if (vptr)
        {
            int tgt_pmat = parent_id < 0 ? SMC_LAYER : info->vein_mat[parent_id];
            int cur_pmat = vptr->parent_mat();
            if (parent_id < 0)
                vptr->set_parent(VeinExtent::Ptr());

            if (cur_pmat != tgt_pmat)
            {
                std::string ctx = "be anywhere";
                if (vptr->parent)
                    ctx = "only be in "+MaterialInfo(0,vptr->parent_mat()).getToken();

                out.printerr(
                    "Duplicate vein %s %s in biome %d layer %d - will %s.\n",
                    MaterialInfo(0,key.first).getToken().c_str(),
                    ENUM_KEY_STR(inclusion_type, key.second).c_str(),
                    biome->info.geo_index, index, ctx.c_str()
                );
            }

            vptr->probability = std::max<int>(vptr->probability, info->vein_unk_38[i]);
        }
        else
        {
            vptr = VeinExtent::Ptr(new VeinExtent(key));
            vptr->probability = info->vein_unk_38[i];
            if (parent_id >= 0)
                vptr->set_parent(refs[parent_id]);

            vptr->add_tiles(mineral_count[key]);
            mineral_count.erase(key);

            vptr->link(this);
            refs.push_back(vptr);
        }
    }

    return true;
}

bool VeinGenerator::place_orphan(t_veinkey key, int size, GeoLayer *from)
{
    std::map<int, VeinExtent::PVec> best;

    for (auto it = biomes.begin(); it != biomes.end(); ++it)
    {
        auto &layers = it->second->layers;

        for (size_t i = 0; i < layers.size(); i++)
        {
            if (layers[i]->mineral_tiles >= layers[i]->unmined_tiles)
                continue;

            VeinExtent::Ptr vein = map_find(layers[i]->veins, key);
            if (!vein)
                continue;

            int dist = std::min(
                layers[i]->min_z() - from->max_z(),
                from->min_z() - layers[i]->max_z()
            );

            best[std::max(0, dist)].push_back(vein);
        }
    }

    if (best.empty())
    {
        out.printerr(
            "Could not place orphaned vein %s %s anywhere.\n",
            MaterialInfo(0,key.first).getToken().c_str(),
            ENUM_KEY_STR(inclusion_type, key.second).c_str()
        );

        return true;
    }

    for (auto it = best.begin(); size > 0 && it != best.end(); ++it)
    {
        auto &vec = it->second;

        int free = 0;
        for (size_t i = 0; i < vec.size(); i++)
        {
            auto layer = vec[i]->layers[0];
            free += layer->unmined_tiles - layer->mineral_tiles;
        }
        float coeff = float(size)/free;

        for (size_t i = 0; i < vec.size(); i++)
        {
            auto layer = vec[i]->layers[0];
            int cfree = std::max(0, layer->unmined_tiles - layer->mineral_tiles);
            int cnt = std::min(size, std::min(cfree, int(ceilf(cfree*coeff))));
            vec[i]->add_tiles(cnt);
            layer->mineral_tiles += cnt;
            size -= cnt;
        }
    }

    if (size > 0)
    {
         out.printerr(
            "Could not place all of orphaned vein %s %s: %d left.\n",
            MaterialInfo(0,key.first).getToken().c_str(),
            ENUM_KEY_STR(inclusion_type, key.second).c_str(),
            size
        );
    }

    return true;
}

bool VeinGenerator::form_veins()
{
    // Form veins in layers
    for (auto it = biomes.begin(); it != biomes.end(); ++it)
    {
        auto &layers = it->second->layers;

        for (size_t i = 0; i < layers.size(); i++)
            if (layers[i] && !layers[i]->form_veins(out))
                return false;
    }

    // Place orphaned minerals
    for (auto it = biomes.begin(); it != biomes.end(); ++it)
    {
        auto &layers = it->second->layers;

        for (size_t i = 0; i < layers.size(); i++)
        {
            auto &mins = layers[i]->mineral_count;

            for (auto mit = mins.begin(); mit != mins.end(); ++mit)
            {
                if (mit->second <= 0) continue;

                if (!place_orphan(mit->first, mit->second, layers[i]))
                    return false;
            }
        }
    }

    // Join adjacent extents with the same density
    for (auto it = biomes.begin(); it != biomes.end(); ++it)
    {
        auto &layers = it->second->layers;

        for (size_t i = 0; i < layers.size(); i++)
        {
            auto &mins = layers[i]->veins;

            for (auto mit = mins.begin(); mit != mins.end(); ++mit)
            {
                auto cur = mit->second;
                if (!cur) continue;

                // Merge in this biome
                if (i > 0)
                {
                    if (auto prev = map_find(layers[i-1]->veins, mit->first))
                    {
                        if (cur->is_similar(prev))
                        {
                            cur->merge_into(prev);
                            continue;
                        }
                    }
                }

                // Merge across biomes
                auto &vec = veins[cur->vein];

                for (size_t j = 0; j < vec.size(); j++)
                {
                    if (vec[j]->min_z <= cur->max_z &&
                        vec[j]->max_z >= cur->min_z &&
                        cur->is_similar(vec[j]))
                    {
                        cur->merge_into(vec[j]);
                        cur.reset();
                        break;
                    }
                }

                if (cur)
                    vec.push_back(cur);
            }
        }
    }

    return true;
}

void VeinGenerator::init_seeds()
{
    MersenneRNG rng;

    std::string seed = world->worldgen.worldgen_parms.seed;
    seed.resize((seed.size()+3)&~3);
    rng.init((uint32_t*)seed.data(), seed.size()/4, 10);

    for (size_t i = 0; i < materials.size(); i++)
    {
        for (unsigned j = 0; j < NUM_INCLUSIONS; j++)
            materials[i].seeds[j] = rng.random();
    }
}

NoiseFunction::Ptr VeinGenerator::get_noise(t_veinkey vein)
{
    auto &seed = materials[vein.first];
    auto &func = seed.funcs[vein.second];

    if (!func)
    {
        MersenneRNG rng;
        rng.init(seed.seeds[vein.second], 10);

        func = NoiseFunction::Ptr(makeVeinDistribution(vein, rng));
    }

    return func;
}

static bool vein_cmp(const VeinExtent::Ptr &a, const VeinExtent::Ptr &b)
{
    return (a->parent_depth < b->parent_depth)
        || (a->parent_depth == b->parent_depth && a->density() < b->density());
}

bool VeinGenerator::place_veins(bool verbose)
{
    VeinExtent::PVec queue;

    init_seeds();

    // Compute the placement queue
    for (auto it = veins.begin(); it != veins.end(); ++it)
    {
        auto &vec = it->second;

        for (size_t i = 0; i < vec.size(); i++)
        {
            auto key = vec[i]->vein;

            if (vec[i]->num_tiles <= 0)
                continue;

            if (!isStoneInorganic(key.first))
            {
                out.printerr(
                    "Invalid vein material: %s\n",
                    MaterialInfo(0, key.first).getToken().c_str()
                );

                return false;
            }

            if (!is_valid_enum_item(key.second))
            {
                out.printerr("Invalid vein type: %d\n", key.second);
                return false;
            }

            vec[i]->distribution = get_noise(key);

            queue.push_back(vec[i]);
        }
    }

    sort(queue.begin(), queue.end(), vein_cmp);

    // Place tiles
    out.print("Processing... (%zu)", queue.size());

    for (size_t j = 0; j < queue.size(); j++)
    {
        if (queue[j]->parent && !queue[j]->parent->placed)
        {
            out.printerr(
                "\nParent vein not placed for %s %s.\n",
                MaterialInfo(0,queue[j]->vein.first).getToken().c_str(),
                ENUM_KEY_STR(inclusion_type, queue[j]->vein.second).c_str()
            );

            return false;
        }

        if (verbose)
        {
            if (j > 0)
                out.print("done.");

            out.print(
                "\nVein layer %zu of %zu: %s %s (%.2f%%)... ",
                j+1, queue.size(),
                MaterialInfo(0,queue[j]->vein.first).getToken().c_str(),
                ENUM_KEY_STR(inclusion_type, queue[j]->vein.second).c_str(),
                queue[j]->density() * 100
            );
        }
        else
        {
            out.print("\rVein layer %zu of %zu... ", j+1, queue.size());
            out.flush();
        }

        queue[j]->place_tiles();
    }

    out.print("done.\n");
    return true;
}

command_result cmd_3dveins(color_ostream &con, std::vector<std::string> & parameters)
{
    bool verbose = false;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        if (parameters[i] == "verbose")
            verbose = true;
        else
            return CR_WRONG_USAGE;
    }

    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    if (!World::isFortressMode())
    {
        con.printerr("Must be used in fortress mode!\n");
        return CR_FAILURE;
    }

    VeinGenerator generator(con);

    con.print("Collecting statistics...\n");

    if (!generator.init_biomes())
        return CR_FAILURE;
    if (!generator.scan_tiles())
        return CR_FAILURE;

    con.print("Generating veins...\n");

    if (!generator.form_veins())
        return CR_FAILURE;
    if (!generator.place_veins(verbose))
        return CR_FAILURE;

    con.print("Writing tiles...\n");

    generator.write_tiles();

    return CR_OK;
}
