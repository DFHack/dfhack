/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

/*******************************************************************************
                                    M A P S
                            Read and write DF's map
*******************************************************************************/
#pragma once
#ifndef CL_MOD_MAPS
#define CL_MOD_MAPS

#include "Export.h"
#include "Module.h"
#include "BitArray.h"

#include "modules/Materials.h"

#include "df/biome_type.h"
#include "df/block_flags.h"
#include "df/feature_type.h"
#include "df/flow_type.h"
#include "df/tile_dig_designation.h"
#include "df/tiletype.h"
#include "df/world_site.h"

namespace df {
    struct block_square_event;
    struct block_square_event_designation_priorityst;
    struct block_square_event_frozen_liquidst;
    struct block_square_event_grassst;
    struct block_square_event_item_spatterst;
    struct block_square_event_material_spatterst;
    struct block_square_event_mineralst;
    struct block_square_event_spoorst;
    struct block_square_event_world_constructionst;
    struct feature_init;
    struct map_block;
    struct map_block_column;
    struct region_map_entry;
    struct plant;
    struct world;
    struct world_data;
    struct world_geo_biome;
    union tile_designation;
    union tile_occupancy;
}

/**
 * \defgroup grp_maps Maps module and its types
 * @ingroup grp_modules
 */

namespace df {
    struct burrow;
    struct world_data;
    struct block_burrow;
}

namespace DFHack {
/***************************************************************************
                                T Y P E S
***************************************************************************/
/**
 * Function for translating feature index to its name
 * \ingroup grp_maps
 */
extern DFHACK_EXPORT const char *sa_feature(df::feature_type index);

typedef df::coord DFCoord;
typedef DFCoord planecoord;

/**
 * A local or global map feature
 * \ingroup grp_maps
 */
struct t_feature {
    df::feature_type type;
    // Main material type - decides between stuff like bodily fluids, inorganics, vomit, amber, etc.
    int16_t main_material;
    // Generally some index to a vector of material types.
    int32_t sub_material;
    // Placeholder
    bool discovered;
    // This is NOT part of the DF feature, but an address of the feature as seen by DFhack.
    df::feature_init *origin;
};

/**
 * \ingroup grp_maps
 */
enum BiomeOffset {
    eNorthWest,
    eNorth,
    eNorthEast,
    eWest,
    eHere,
    eEast,
    eSouthWest,
    eSouth,
    eSouthEast,
    eBiomeCount
};

/**
 * map block flags wrapper
 * \ingroup grp_maps
 */
typedef df::block_flags t_blockflags;

/**
 * 16x16 array of tile types
 * \ingroup grp_maps
 */
typedef df::tiletype tiletypes40d [16][16];
/**
 * 16x16 array used for squashed block materials
 * \ingroup grp_maps
 */
typedef int16_t t_blockmaterials [16][16];
/**
 * 16x16 array of designation flags
 * \ingroup grp_maps
 */
typedef df::tile_designation t_designation;
typedef t_designation designations40d [16][16];
/**
 * 16x16 array of occupancy flags
 * \ingroup grp_maps
 */
typedef df::tile_occupancy t_occupancy;
typedef t_occupancy occupancies40d [16][16];
/**
 * array of 16 biome indexes valid for the block
 * \ingroup grp_maps
 */
typedef uint8_t biome_indices40d [9];
/**
 * 16x16 array of temperatures
 * \ingroup grp_maps
 */
typedef uint16_t t_temperatures [16][16];

/**
 * Index a tile array by a 2D coordinate, clipping it to mod 16.
 */
template<class T> inline auto index_tile(T &v, df::coord2d p)
    -> typename std::add_rvalue_reference<decltype(v[0][0])>::type
{
    return v[p.x&15][p.y&15];
}

/**
 * Check if a 2D coordinate is in the 0-15 range.
 */
inline bool is_valid_tile_coord(df::coord2d p) {
    return (p.x & ~15) == 0 && (p.y & ~15) == 0;
}

/**
 * Utility class representing a cuboid of df::coord.
 * \ingroup grp_maps
 */
class cuboid {
    public:
    // Bounds
    int16_t x_min = -1;
    int16_t x_max = -1;
    int16_t y_min = -1;
    int16_t y_max = -1;
    int16_t z_min = -1;
    int16_t z_max = -1;

    // Default constructor.
    DFHACK_EXPORT cuboid() {}

    // Construct from two corners.
    DFHACK_EXPORT cuboid(int16_t x1, int16_t y1, int16_t z1, int16_t x2, int16_t y2, int16_t z2);
    DFHACK_EXPORT cuboid(const df::coord &p1, const df::coord &p2) : cuboid(p1.x, p1.y, p1.z, p2.x, p2.y, p2.z) {}

    // Construct as single tile.
    DFHACK_EXPORT cuboid(int16_t x, int16_t y, int16_t z);
    DFHACK_EXPORT cuboid(const df::coord &p) : cuboid(p.x, p.y, p.z) {}

    // Construct from map block.
    DFHACK_EXPORT cuboid(const df::map_block *block);

    // Valid cuboid? True if all max >= min >= 0.
    DFHACK_EXPORT bool isValid() const;

    // Clear cuboid dimensions, making it invalid.
    DFHACK_EXPORT void clear() { x_min = x_max = y_min = y_max = z_min = z_max = -1; }

    // Clamp this cuboid within another cuboid. Invalid result implies no intersection or invalid input.
    DFHACK_EXPORT cuboid clamp(const cuboid &other);

    // Return a new cuboid representing overlapping volume. Invalid result implies no intersection or invalid input.
    DFHACK_EXPORT cuboid clampNew(const cuboid &other) const { return cuboid(*this).clamp(other); }

    /// Clamp this cuboid within map area. Invalid result implies no intersection or invalid input (e.g., no map).
    /// Can optionally treat cuboid as map blocks instead of tiles.
    /// Note: A point being in map area isn't sufficient to know that a tile block is allocated there!
    DFHACK_EXPORT cuboid clampMap(bool block = false);

    // Expand cuboid to include point. Returns true if bounds changed. Fails if x/y/z < 0.
    DFHACK_EXPORT bool addPos(int16_t x, int16_t y, int16_t z);
    DFHACK_EXPORT bool addPos(const df::coord &pos) { return addPos(pos.x, pos.y, pos.z); }

    // Return true if point inside cuboid. Make sure cuboid is valid first!
    DFHACK_EXPORT bool containsPos(int16_t x, int16_t y, int16_t z) const;
    DFHACK_EXPORT bool containsPos(const df::coord &pos) const { return containsPos(pos.x, pos.y, pos.z); }

    /// Iterate over every point in the cuboid. Doesn't guarantee valid map tile!
    /// "fn" should return true to keep iterating. Won't iterate if cuboid invalid.
    /// If row_major is false, iterates from top-down (z), N-S (y), then W-E (x).
    /// If row_major is true, iterates from top-down (z), W-E (x), then N-S (y).
    DFHACK_EXPORT void forCoord(std::function<bool(df::coord)> fn, bool row_major = false) const;

    /// Iterate over every non-NULL map block intersecting the tile cuboid from top-down, N-S, then W-E.
    /// Will also supply the intersection of this cuboid and block to your "fn" for use with cuboid::forCoord.
    /// Can optionally attempt to create map blocks if they aren't allocated.
    /// "fn" should return true to keep iterating. Won't iterate if cuboid::clampMap() would fail.
    DFHACK_EXPORT void forBlock(std::function<bool(df::map_block *, cuboid)> fn, bool ensure_block = false) const;
};

/**
 * The Maps module
 * \ingroup grp_modules
 * \ingroup grp_maps
 */
namespace Maps
{
extern DFHACK_EXPORT bool IsValid();

/// Iterate over points in a cuboid from z1:z2, y1:y2, then x1:x2.
/// If row_major is true, iterates from z1:z2, x1:x2, then y1:y2.
/// Doesn't guarantee valid map tile! Can be used to iterate over blocks, etc.
/// "fn" should return true to keep iterating.
DFHACK_EXPORT void forCoord(std::function<bool(df::coord)> fn,
    int16_t x1, int16_t y1, int16_t z1, int16_t x2, int16_t y2, int16_t z2, bool row_major = false);
inline void forCoord(std::function<bool(df::coord)> fn, const df::coord &p1, const df::coord &p2, bool row_major = false) {
    forCoord(fn, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, row_major);
}

/**
 * Method for reading the geological surrounding of the currently loaded region.
 * assign is a reference to an array of nine vectors of unsigned words that are to be filled with the data
 * array is indexed by the BiomeOffset enum
 *
 * I omitted resolving the layer matgloss in this API, because it would
 * introduce overhead by calling some method for each tile. You have to do it
 * yourself.
 *
 * First get the stuff from ReadGeology and then for each block get the RegionOffsets.
 * For each tile get the real region from RegionOffsets and cross-reference it with
 * the geology stuff (region -- array of vectors, depth -- vector).
 * I'm thinking about turning that Geology stuff into a two-dimensional array
 * with static size.
 *
 * This is the algorithm for applying matgloss:
 * @code

void DfMap::applyGeoMatgloss(Block *b)
{   // Load layer matgloss
    for(int x_b = 0; x_b < BLOCK_SIZE; x_b++) {
        for(int y_b = 0; y_b < BLOCK_SIZE; y_b++)
        {
            int geolayer = b->designation[x_b][y_b].bits.geolayer_index;
            int biome = b->designation[x_b][y_b].bits.biome;
            b->material[x_b][y_b].type = Mat_Stone;
            b->material[x_b][y_b].index = v_geology[b->RegionOffsets[biome]][geolayer];
        }
    }
}

 * @endcode
 */
extern DFHACK_EXPORT bool ReadGeology(std::vector<std::vector<int16_t> > *layer_mats, std::vector<df::coord2d> *geoidx);
// Get pointers to features of a block.
extern DFHACK_EXPORT bool ReadFeatures(int32_t x, int32_t y, int32_t z, t_feature *local, t_feature *global);
extern DFHACK_EXPORT bool ReadFeatures(uint32_t x, uint32_t y, uint32_t z, t_feature *local, t_feature *global); // TODO: deprecate me
// Get pointers to features of an already read block.
extern DFHACK_EXPORT bool ReadFeatures(df::map_block *block, t_feature *local, t_feature *global);

// Get a pointer to a specific global feature directly.
DFHACK_EXPORT df::feature_init *getGlobalInitFeature(int32_t index);
// Get a pointer to a specific local feature directly. rgn_coord is in the world region grid.
DFHACK_EXPORT df::feature_init *getLocalInitFeature(df::coord2d rgn_coord, int32_t index);

// Read a specific global or local feature directly.
extern DFHACK_EXPORT bool GetGlobalFeature(t_feature &feature, int32_t index);
//extern DFHACK_EXPORT bool GetLocalFeature(t_feature &feature, df::coord2d rgn_coord, int32_t index);

/*
 * BLOCK DATA
 */

// Get size of the map in blocks.
extern DFHACK_EXPORT void getSize(int32_t &x, int32_t &y, int32_t &z);
extern DFHACK_EXPORT void getSize(uint32_t &x, uint32_t &y, uint32_t &z); // TODO: deprecate me
// Get size of the map in tiles.
extern DFHACK_EXPORT void getTileSize(int32_t &x, int32_t &y, int32_t &z);
extern DFHACK_EXPORT void getTileSize(uint32_t &x, uint32_t &y, uint32_t &z); // TODO: deprecate me
// Get the position of the map on world map.
extern DFHACK_EXPORT void getPosition(int32_t &x, int32_t &y, int32_t &z);

extern DFHACK_EXPORT bool isValidTilePos(int32_t x, int32_t y, int32_t z);
inline bool isValidTilePos(df::coord pos) { return isValidTilePos(pos.x, pos.y, pos.z); }

extern DFHACK_EXPORT bool isTileVisible(int32_t x, int32_t y, int32_t z);
inline bool isTileVisible(df::coord pos) { return isTileVisible(pos.x, pos.y, pos.z); }

// Get the map block or NULL if block is not valid.
extern DFHACK_EXPORT df::map_block *getBlock (int32_t blockx, int32_t blocky, int32_t blockz);
extern DFHACK_EXPORT df::map_block *getTileBlock (int32_t x, int32_t y, int32_t z);
extern DFHACK_EXPORT df::map_block *ensureTileBlock (int32_t x, int32_t y, int32_t z);

extern DFHACK_EXPORT df::map_block_column *getBlockColumn(int32_t blockx, int32_t blocky);

inline df::map_block *getBlock (df::coord pos) { return getBlock(pos.x, pos.y, pos.z); }
inline df::map_block *getTileBlock (df::coord pos) { return getTileBlock(pos.x, pos.y, pos.z); }
inline df::map_block *ensureTileBlock (df::coord pos) { return ensureTileBlock(pos.x, pos.y, pos.z); }

extern DFHACK_EXPORT df::tiletype *getTileType(int32_t x, int32_t y, int32_t z);
extern DFHACK_EXPORT df::tile_designation *getTileDesignation(int32_t x, int32_t y, int32_t z);
extern DFHACK_EXPORT df::tile_occupancy *getTileOccupancy(int32_t x, int32_t y, int32_t z);

inline df::tiletype *getTileType(df::coord pos) { return getTileType(pos.x, pos.y, pos.z); }
inline df::tile_designation *getTileDesignation(df::coord pos) { return getTileDesignation(pos.x, pos.y, pos.z); }
inline df::tile_occupancy *getTileOccupancy(df::coord pos) { return getTileOccupancy(pos.x, pos.y, pos.z); }

// Returns biome info about the specified world region.
DFHACK_EXPORT df::region_map_entry *getRegionBiome(df::coord2d rgn_pos);

// Returns biome world region coordinates for the given tile within given block.
DFHACK_EXPORT df::coord2d getBlockTileBiomeRgn(df::map_block *block, df::coord2d pos);

inline df::coord2d getTileBiomeRgn(df::coord pos) { return getBlockTileBiomeRgn(getTileBlock(pos), pos); }

// Enables per-frame updates for liquid flow and/or temperature.
DFHACK_EXPORT void enableBlockUpdates(df::map_block *blk, bool flow = false, bool temperature = false);

DFHACK_EXPORT df::flow_info *spawnFlow(df::coord pos, df::flow_type type, int mat_type = 0, int mat_index = -1, int density = 100);

/// Sorts the block event vector into multiple vectors by type,
/// mineral veins, what's under ice, blood smears, and mud.
extern DFHACK_EXPORT bool SortBlockEvents(df::map_block *block,
    std::vector<df::block_square_event_mineralst *> *veins,
    std::vector<df::block_square_event_frozen_liquidst *> *ices = 0,
    std::vector<df::block_square_event_material_spatterst *> *materials = 0,
    std::vector<df::block_square_event_grassst *> *grass = 0,
    std::vector<df::block_square_event_world_constructionst *> *constructions = 0,
    std::vector<df::block_square_event_spoorst *> *spoors = 0,
    std::vector<df::block_square_event_item_spatterst *> *items = 0,
    std::vector<df::block_square_event_designation_priorityst *> *priorities = 0
);

// Remove a block event from the block by address.
extern DFHACK_EXPORT bool RemoveBlockEvent(int32_t x, int32_t y, int32_t z, df::block_square_event *which );
extern DFHACK_EXPORT bool RemoveBlockEvent(uint32_t x, uint32_t y, uint32_t z, df::block_square_event *which ); // TODO: deprecate me

DFHACK_EXPORT uint16_t getWalkableGroup(df::coord pos);
DFHACK_EXPORT bool canWalkBetween(df::coord pos1, df::coord pos2);
DFHACK_EXPORT bool canStepBetween(df::coord pos1, df::coord pos2);

// Get the plant that owns the tile at the specified position.
extern DFHACK_EXPORT df::plant *getPlantAtTile(int32_t x, int32_t y, int32_t z);
inline df::plant *getPlantAtTile(df::coord pos) { return getPlantAtTile(pos.x, pos.y, pos.z); }

// Get the biome type at the given region coordinates.
DFHACK_EXPORT df::enums::biome_type::biome_type getBiomeTypeWithRef(int16_t region_x, int16_t region_y, int16_t region_ref_y);
inline df::enums::biome_type::biome_type getBiomeType(int16_t region_x, int16_t region_y) { return getBiomeTypeWithRef(region_x, region_y, region_y); }

DFHACK_EXPORT bool isTileAquifer(int32_t x, int32_t y, int32_t z);
inline bool isTileAquifer(df::coord pos) { return isTileAquifer(pos.x, pos.y, pos.z); }
DFHACK_EXPORT bool isTileHeavyAquifer(int32_t x, int32_t y, int32_t z);
inline bool isTileHeavyAquifer(df::coord pos) { return isTileHeavyAquifer(pos.x, pos.y, pos.z); }
DFHACK_EXPORT bool setTileAquifer(int32_t x, int32_t y, int32_t z, bool heavy = false);
inline bool setTileAquifer(df::coord pos, bool heavy = false) { return setTileAquifer(pos.x, pos.y, pos.z, heavy); }
DFHACK_EXPORT int setAreaAquifer(df::coord pos1, df::coord pos2, bool heavy = false,
    std::function<bool(df::coord, df::map_block *)> filter = [](df::coord pos, df::map_block *block) { return true; });
DFHACK_EXPORT bool removeTileAquifer(int32_t x, int32_t y, int32_t z);
inline bool removeTileAquifer(df::coord pos) { return removeTileAquifer(pos.x, pos.y, pos.z); }
DFHACK_EXPORT int removeAreaAquifer(df::coord pos1, df::coord pos2,
    std::function<bool(df::coord, df::map_block *)> filter = [](df::coord pos, df::map_block *block) { return true; });


/**
 * A single function does not merit a "Sites" module, hence we collect site functions here in the meantime.
 */

// Get the classification string (e.g. "town", "hillocs", "tower", etc.) for a site
DFHACK_EXPORT const char* getSiteTypeName(df::world_site *site);
}
}
#endif
