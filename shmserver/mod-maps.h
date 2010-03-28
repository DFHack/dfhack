/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix)

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

#ifndef MOD_MAPS_H
#define MOD_MAPS_H

// increment on every change
#include <DFTypes.h>

namespace DFHack
{
    namespace Maps
    {

#define MAPS_VERSION 2
typedef struct
{
    uint32_t map_offset;// = d->offset_descriptor->getAddress ("map_data");
    uint32_t x_count_offset;// = d->offset_descriptor->getAddress ("x_count");
    uint32_t y_count_offset;// = d->offset_descriptor->getAddress ("y_count");
    uint32_t z_count_offset;// = d->offset_descriptor->getAddress ("z_count");
    uint32_t tile_type_offset;// = d->offset_descriptor->getOffset ("type");
    uint32_t designation_offset;// = d->offset_descriptor->getOffset ("designation");
    uint32_t occupancy_offset;// = d->offset_descriptor->getOffset ("occupancy");
    uint32_t biome_stuffs;// = d->offset_descriptor->getOffset ("biome_stuffs");
    uint32_t veinvector;// = d->offset_descriptor->getOffset ("v_vein");
    uint32_t vein_mineral_vptr;
    uint32_t vein_ice_vptr;
    /*
    GEOLOGY
    uint32_t region_x_offset;// = minfo->getAddress ("region_x");
    uint32_t region_y_offset;// = minfo->getAddress ("region_y");
    uint32_t region_z_offset;// =  minfo->getAddress ("region_z");
    uint32_t world_offset;// =  minfo->getAddress ("world");
    uint32_t world_regions_offset;// =  minfo->getOffset ("w_regions_arr");
    uint32_t region_size;// =  minfo->getHexValue ("region_size");
    uint32_t region_geo_index_offset;// =  minfo->getOffset ("region_geo_index_off");
    uint32_t world_geoblocks_offset;// =  minfo->getOffset ("w_geoblocks");
    uint32_t world_size_x;// = minfo->getOffset ("world_size_x");
    uint32_t world_size_y;// = minfo->getOffset ("world_size_y");
    uint32_t geolayer_geoblock_offset;// = minfo->getOffset ("geolayer_geoblock_offset");
    */
} maps_offsets;

typedef struct
{
    bool inited;
    maps_offsets offsets;
} maps_modulestate;

typedef struct
{
    shm_cmd cmd[SHM_MAX_CLIENTS]; // MANDATORY!
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t x2;
    uint32_t y2;
    uint32_t z2;
    uint32_t address;
    uint32_t error;
} shm_maps_hdr;

enum MAPS_COMMAND
{
    MAP_INIT = 0, // initialization
    MAP_PROBE, // check if the map is still there
    MAP_GET_SIZE, // get the map size in 16x16x1 blocks
    MAP_READ_BLOCKTREE, // read the structure of pointers to blocks
    MAP_READ_BLOCK_BY_COORDS, // read block by cords
    MAP_READ_BLOCK_BY_ADDRESS, // read block by address
    MAP_WRITE_BLOCK,
    MAP_READ_BLOCKS_3D, // read blocks between two coords (volumetric)
    MAP_READ_ALL_BLOCKS, // read the entire map
    MAP_REVEAL, // reveal the whole map
    NUM_MAPS_CMDS,
};
DFPP_module Init(void);

    }
}

#endif