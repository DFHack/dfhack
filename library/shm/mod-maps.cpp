#include <string>
#include <vector>
#include <map>
#include <dfhack/DFIntegers.h>

#include "shms.h"
#include "mod-core.h"
#include "mod-maps.h"
#include <dfhack/DFTypes.h>
#include <dfhack/modules/Maps.h>
using namespace DFHack;
using namespace DFHack::Server::Maps;

#include <string.h>
#ifndef MACOSX_BUILD
#include <malloc.h>
#endif

extern char *shm;

//TODO: circular buffer streaming primitives required
//TODO: commands can fail without the proper offsets. Hot to handle that?

namespace DFHack{
    namespace Server{ // start of namespace
        namespace Maps{ // start of namespace

#define SHMHDR ((shm_maps_hdr *)shm)
#define SHMCMD ((shm_cmd *)shm)->pingpong
#define SHMDATA(type) ((type *)(shm + SHM_HEADER))

void NullCommand (void* data)
{
};

void InitOffsets (void* data)
{
    maps_modulestate * state = (maps_modulestate *) data;
    memcpy((void *) &(state->offsets), SHMDATA(void), sizeof(maps_offsets));
    ((maps_modulestate *) data)->inited = true;
}

void GetMapSize (void *data)
{
    maps_modulestate * state = (maps_modulestate *) data;
    if(state->inited)
    {
        SHMHDR->x = *(uint32_t *) (state->offsets.x_count_offset);
        SHMHDR->y = *(uint32_t *) (state->offsets.y_count_offset);
        SHMHDR->z = *(uint32_t *) (state->offsets.z_count_offset);
        SHMHDR->error = false;
    }
    else
    {
        SHMHDR->error = true;
    }
}

struct mblock
{
    uint32_t * ptr_to_dirty;
};

inline void ReadBlockByAddress (void * data)
{
    maps_modulestate * state = (maps_modulestate *) data;
    maps_offsets & offsets = state->offsets;
    mblock * block = (mblock *) SHMHDR->address;
    if(block)
    {
        memcpy(&(SHMDATA(mapblock40d)->tiletypes), ((char *) block) + offsets.tile_type_offset, sizeof(SHMDATA(mapblock40d)->tiletypes));
        memcpy(&(SHMDATA(mapblock40d)->designation), ((char *) block) + offsets.designation_offset, sizeof(SHMDATA(mapblock40d)->designation));
        memcpy(&(SHMDATA(mapblock40d)->occupancy), ((char *) block) + offsets.occupancy_offset, sizeof(SHMDATA(mapblock40d)->occupancy));
        memcpy(&(SHMDATA(mapblock40d)->biome_indices), ((char *) block) + offsets.biome_stuffs, sizeof(SHMDATA(mapblock40d)->biome_indices));
        SHMDATA(mapblock40d)->blockflags.whole = *block->ptr_to_dirty;
        
        SHMDATA(mapblock40d)->local_feature = *(int16_t *)((char*) block + offsets.local_feature_offset);
        SHMDATA(mapblock40d)->global_feature = *(int16_t *)((char*) block + offsets.global_feature_offset);
        
        SHMDATA(mapblock40d)->origin = (uint32_t)(uint64_t)block; // this is STUPID
        SHMHDR->error = false;
    }
    else
    {
        SHMHDR->error = true;
    }
}

void ReadBlockByCoords (void * data)
{
    maps_modulestate * state = (maps_modulestate *) data;
    maps_offsets & offsets = state->offsets;
    /* map_offset is a pointer to
                  a pointer to
                  an X block of pointers to
                  an Y blocks of pointers to
                  a Z blocks of pointers to
                  map blocks
    only Z blocks can have NULL pointers? TODO: verify
    */
    mblock * *** mapArray = *(mblock * ****)offsets.map_offset;
    SHMHDR->address = (uint32_t) (uint64_t) mapArray[SHMHDR->x][SHMHDR->y][SHMHDR->z];// this is STUPID
    ReadBlockByAddress(data); // I wonder... will this inline properly?
}

DFPP_module Init( void )
{
    DFPP_module maps;
    maps.name = "Maps";
    maps.version = MAPS_VERSION;
    // freed by the core
    maps.modulestate = malloc(sizeof(maps_modulestate)); // we store a flag
    memset(maps.modulestate,0,sizeof(maps_modulestate));
    
    maps.reserve(NUM_MAPS_CMDS);
    
    // client sends a maps_offsets struct -> inited = true;
    maps.set_command(MAP_INIT, FUNCTION, "Supply the module with offsets",InitOffsets,CORE_SUSPENDED);
    maps.set_command(MAP_GET_SIZE, FUNCTION, "Get map size in 16x16x1 tile blocks", GetMapSize, CORE_SUSPENDED);
    maps.set_command(MAP_READ_BLOCK_BY_COORDS, FUNCTION, "Read the whole block with specified coords", ReadBlockByCoords, CORE_SUSPENDED);
    maps.set_command(MAP_READ_BLOCK_BY_ADDRESS, FUNCTION, "Read the whole block from an address", ReadBlockByAddress, CORE_SUSPENDED);
    
    // will it fit into 1MB? We shouldn't assume this is the case
    maps.set_command(MAP_READ_BLOCKTREE, FUNCTION,"Get the tree of block pointers as a single structure", NullCommand, CORE_SUSPENDED);

    // really doesn't fit into 1MB, there should be a streaming variant to better utilize context switches
    maps.set_command(MAP_READ_BLOCKS_3D, FUNCTION, "Read a range of blocks between two sets of coords", NullCommand, CORE_SUSPENDED);
    
    return maps;
}

}}} // end of namespace
