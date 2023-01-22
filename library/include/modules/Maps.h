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
#include <vector>
#include "BitArray.h"
#include "modules/Materials.h"

#include "df/biome_type.h"
#include "df/block_flags.h"
#include "df/feature_type.h"
#include "df/flow_type.h"
#include "df/plant.h"
#include "df/tile_dig_designation.h"
#include "df/tile_liquid.h"
#include "df/tile_traffic.h"
#include "df/tiletype.h"

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

namespace df
{
    struct burrow;
    struct world_data;
    struct block_burrow;
}

namespace DFHack
{
/***************************************************************************
                                T Y P E S
***************************************************************************/
/**
    * Function for translating feature index to its name
    * \ingroup grp_maps
    */
extern DFHACK_EXPORT const char * sa_feature(df::feature_type index);

typedef df::coord DFCoord;
typedef DFCoord planecoord;

/**
 * A local or global map feature
 * \ingroup grp_maps
 */
struct t_feature
{
    df::feature_type type;
    /// main material type - decides between stuff like bodily fluids, inorganics, vomit, amber, etc.
    int16_t main_material;
    /// generally some index to a vector of material types.
    int32_t sub_material;
    /// placeholder
    bool discovered;
    /// this is NOT part of the DF feature, but an address of the feature as seen by DFhack.
    df::feature_init * origin;
};

/**
 * \ingroup grp_maps
 */
enum BiomeOffset
{
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
 * Index a tile array by a 2D coordinate, clipping it to mod 16
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
 * The Maps module
 * \ingroup grp_modules
 * \ingroup grp_maps
 */
namespace Maps
{

extern DFHACK_EXPORT bool IsValid();

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
 * this is the algorithm for applying matgloss:
 * @code

void DfMap::applyGeoMatgloss(Block * b)
{
    // load layer matgloss
    for(int x_b = 0; x_b < BLOCK_SIZE; x_b++)
    {
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
extern DFHACK_EXPORT bool ReadGeology(std::vector<std::vector<int16_t> > *layer_mats,
                                      std::vector<df::coord2d> *geoidx);
/**
 * Get pointers to features of a block
 */
extern DFHACK_EXPORT bool ReadFeatures(int32_t x, int32_t y, int32_t z, t_feature * local, t_feature * global);
extern DFHACK_EXPORT bool ReadFeatures(uint32_t x, uint32_t y, uint32_t z, t_feature * local, t_feature * global); // todo: deprecate me
/**
 * Get pointers to features of an already read block
 */
extern DFHACK_EXPORT bool ReadFeatures(df::map_block * block,t_feature * local, t_feature * global);


/**
 * Get a pointer to a specific global feature directly.
*/
DFHACK_EXPORT df::feature_init *getGlobalInitFeature(int32_t index);
/**
 * Get a pointer to a specific local feature directly. rgn_coord is in the world region grid.
 */
DFHACK_EXPORT df::feature_init *getLocalInitFeature(df::coord2d rgn_coord, int32_t index);

/**
 * Read a specific global or local feature directly
 */
extern DFHACK_EXPORT bool GetGlobalFeature(t_feature &feature, int32_t index);
//extern DFHACK_EXPORT bool GetLocalFeature(t_feature &feature, df::coord2d rgn_coord, int32_t index);


/*
 * BLOCK DATA
 */

/// get size of the map in blocks
extern DFHACK_EXPORT void getSize(int32_t& x, int32_t& y, int32_t& z);
extern DFHACK_EXPORT void getSize(uint32_t& x, uint32_t& y, uint32_t& z); // todo: deprecate me
/// get size of the map in tiles
extern DFHACK_EXPORT void getTileSize(int32_t& x, int32_t& y, int32_t& z);
extern DFHACK_EXPORT void getTileSize(uint32_t& x, uint32_t& y, uint32_t& z); // todo: deprecate me
/// get the position of the map on world map
extern DFHACK_EXPORT void getPosition(int32_t& x, int32_t& y, int32_t& z);

extern DFHACK_EXPORT bool isValidTilePos(int32_t x, int32_t y, int32_t z);
inline bool isValidTilePos(df::coord pos) { return isValidTilePos(pos.x, pos.y, pos.z); }

extern DFHACK_EXPORT bool isTileVisible(int32_t x, int32_t y, int32_t z);
inline bool isTileVisible(df::coord pos) { return isTileVisible(pos.x, pos.y, pos.z); }

/**
 * Get the map block or NULL if block is not valid
 */
extern DFHACK_EXPORT df::map_block * getBlock (int32_t blockx, int32_t blocky, int32_t blockz);
extern DFHACK_EXPORT df::map_block * getTileBlock (int32_t x, int32_t y, int32_t z);
extern DFHACK_EXPORT df::map_block * ensureTileBlock (int32_t x, int32_t y, int32_t z);

extern DFHACK_EXPORT df::map_block_column * getBlockColumn(int32_t blockx, int32_t blocky);

inline df::map_block * getBlock (df::coord pos) { return getBlock(pos.x, pos.y, pos.z); }
inline df::map_block * getTileBlock (df::coord pos) { return getTileBlock(pos.x, pos.y, pos.z); }
inline df::map_block * ensureTileBlock (df::coord pos) { return ensureTileBlock(pos.x, pos.y, pos.z); }

extern DFHACK_EXPORT df::tiletype *getTileType(int32_t x, int32_t y, int32_t z);
extern DFHACK_EXPORT df::tile_designation *getTileDesignation(int32_t x, int32_t y, int32_t z);
extern DFHACK_EXPORT df::tile_occupancy *getTileOccupancy(int32_t x, int32_t y, int32_t z);

inline df::tiletype *getTileType(df::coord pos) {
    return getTileType(pos.x, pos.y, pos.z);
}
inline df::tile_designation *getTileDesignation(df::coord pos) {
    return getTileDesignation(pos.x, pos.y, pos.z);
}
inline df::tile_occupancy *getTileOccupancy(df::coord pos) {
    return getTileOccupancy(pos.x, pos.y, pos.z);
}

/**
 * Returns biome info about the specified world region.
 */
DFHACK_EXPORT df::region_map_entry *getRegionBiome(df::coord2d rgn_pos);

/**
 * Returns biome world region coordinates for the given tile within given block.
 */
DFHACK_EXPORT df::coord2d getBlockTileBiomeRgn(df::map_block *block, df::coord2d pos);

inline df::coord2d getTileBiomeRgn(df::coord pos) {
    return getBlockTileBiomeRgn(getTileBlock(pos), pos);
}

// Enables per-frame updates for liquid flow and/or temperature.
DFHACK_EXPORT void enableBlockUpdates(df::map_block *blk, bool flow = false, bool temperature = false);

DFHACK_EXPORT df::flow_info *spawnFlow(df::coord pos, df::flow_type type, int mat_type = 0, int mat_index = -1, int density = 100);

/// sorts the block event vector into multiple vectors by type
/// mineral veins, what's under ice, blood smears and mud
extern DFHACK_EXPORT bool SortBlockEvents(df::map_block *block,
    std::vector<df::block_square_event_mineralst *>* veins,
    std::vector<df::block_square_event_frozen_liquidst *>* ices = 0,
    std::vector<df::block_square_event_material_spatterst *>* materials = 0,
    std::vector<df::block_square_event_grassst *>* grass = 0,
    std::vector<df::block_square_event_world_constructionst *>* constructions = 0,
    std::vector<df::block_square_event_spoorst *>* spoors = 0,
    std::vector<df::block_square_event_item_spatterst *>* items = 0,
    std::vector<df::block_square_event_designation_priorityst *>* priorities = 0
);

/// remove a block event from the block by address
extern DFHACK_EXPORT bool RemoveBlockEvent(int32_t x, int32_t y, int32_t z, df::block_square_event * which );
extern DFHACK_EXPORT bool RemoveBlockEvent(uint32_t x, uint32_t y, uint32_t z, df::block_square_event * which ); // todo: deprecate me

DFHACK_EXPORT bool canWalkBetween(df::coord pos1, df::coord pos2);
DFHACK_EXPORT bool canStepBetween(df::coord pos1, df::coord pos2);

/**
 * Get the plant that owns the tile at the specified position
 */
extern DFHACK_EXPORT df::plant *getPlantAtTile(int32_t x, int32_t y, int32_t z);

inline df::plant *getPlantAtTile(df::coord pos) { return getPlantAtTile(pos.x, pos.y, pos.z); }

DFHACK_EXPORT df::enums::biome_type::biome_type getBiomeType(int world_coord_x, int world_coord_y);
DFHACK_EXPORT df::enums::biome_type::biome_type getBiomeTypeWithRef(int world_coord_x, int world_coord_y, int world_ref_y_coord);

}


}
#endif
