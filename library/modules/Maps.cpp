/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <cstdlib>
using namespace std;

#include "modules/Maps.h"
#include "Error.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "ModuleFactory.h"
#include "Core.h"

#include "DataDefs.h"
#include "df/world_data.h"
#include "df/world_underground_region.h"
#include "df/feature_init.h"

using df::global::world;

#define MAPS_GUARD if(!d->Started) throw DFHack::Error::ModuleNotInitialized();

using namespace DFHack;

Module* DFHack::createMaps()
{
    return new Maps();
}

const char * DFHack::sa_feature(df::feature_type index)
{
    switch(index)
    {
    case df::feature_type::outdoor_river:
        return "River";
    case df::feature_type::cave:
        return "Cave";
    case df::feature_type::pit:
        return "Pit";
    case df::feature_type::magma_pool:
        return "Magma pool";
    case df::feature_type::volcano:
        return "Volcano";
    case df::feature_type::deep_special_tube:
        return "Adamantine deposit";
    case df::feature_type::deep_surface_portal:
        return "Underworld portal";
    case df::feature_type::subterranean_from_layer:
        return "Cavern";
    case df::feature_type::magma_core_from_layer:
        return "Magma sea";
    case df::feature_type::feature_underworld_from_layer:
        return "Underworld";
    default:
        return "Unknown/Error";
    }
};

struct Maps::Private
{
    uint32_t worldSizeX, worldSizeY;

    uint32_t maps_module;

    Process * owner;
    bool Inited;
    bool FeaturesStarted;
    bool Started;
    bool hasGeology;
    bool hasFeatures;
    bool hasVeggies;

    // map between feature address and the read object
    map <void *, t_feature> local_feature_store;
    map <DFCoord, vector <t_feature *> > m_local_feature;
    vector <t_feature> v_global_feature;

    vector<uint16_t> v_geology[eBiomeCount];
};

Maps::Maps()
{
    Core & c = Core::getInstance();
    d = new Private;
    Process *p = d->owner = c.p;
    d->Inited = d->FeaturesStarted = d->Started = false;

    DFHack::VersionInfo * mem = c.vinfo;
    d->hasFeatures = d->hasGeology = d->hasVeggies = true;

    d->Inited = true;
}

Maps::~Maps()
{
    if(d->FeaturesStarted)
        StopFeatures();
    if(d->Started)
        Finish();
    delete d;
}

/*-----------------------------------*
 *  Init the mapblock pointer array  *
 *-----------------------------------*/
bool Maps::Start()
{
    if(!d->Inited)
        return false;
    if(d->Started)
        Finish();

    Process *p = d->owner;

    // get the size
    int32_t & mx = world->map.x_count_block;
    int32_t & my = world->map.y_count_block;
    int32_t & mz = world->map.z_count_block;

    // test for wrong map dimensions
    if (mx == 0 || mx > 48 || my == 0 || my > 48 || mz == 0)
    {
        cerr << hex << &mx << " " << &my << " " << &mz << endl;
        cerr << dec << mx << " "<< my << " "<< mz << endl;
        return false;
    }

    d->Started = true;
    return true;
}

// getter for map size
void Maps::getSize (uint32_t& x, uint32_t& y, uint32_t& z)
{
    MAPS_GUARD
    x = world->map.x_count_block;
    y = world->map.y_count_block;
    z = world->map.z_count_block;
}

// getter for map position
void Maps::getPosition (int32_t& x, int32_t& y, int32_t& z)
{
    MAPS_GUARD
    x = world->map.region_x;
    y = world->map.region_y;
    z = world->map.region_z;
}

bool Maps::Finish()
{
    return true;
}

/*
 * Block reading
 */

df::map_block* Maps::getBlock (int32_t blockx, int32_t blocky, int32_t blockz)
{
    if ((blockx < 0) || (blocky < 0) || (blockz < 0))
        return NULL;
    if ((blockx >= world->map.x_count_block) || (blocky << 4 >= world->map.y_count_block) || (blockz >= world->map.z_count_block))
        return NULL;
    return world->map.block_index[blockx][blocky][blockz];
}

df::map_block *Maps::getBlockAbs (int32_t x, int32_t y, int32_t z)
{
    if ((x < 0) || (y < 0) || (z < 0))
        return NULL;
    if ((x >= world->map.x_count) || (y >= world->map.y_count) || (z >= world->map.z_count))
        return NULL;
    return world->map.block_index[x >> 4][y >> 4][z];
}

bool Maps::ReadBlock40d(uint32_t x, uint32_t y, uint32_t z, mapblock40d * buffer)
{
    MAPS_GUARD
    Process *p = d->owner;
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        buffer->position = DFCoord(x,y,z);
        memcpy(buffer->tiletypes,block->tiletype, sizeof(tiletypes40d));
        memcpy(buffer->designation,block->designation, sizeof(designations40d));
        memcpy(buffer->occupancy,block->occupancy, sizeof(occupancies40d));
        memcpy(buffer->biome_indices,block->region_offset, sizeof(block->region_offset));
        buffer->global_feature = block->global_feature;
        buffer->local_feature = block->local_feature;
        buffer->mystery = block->unk2;
        // FIXME: not 64-bit safe
        buffer->origin = &block;
        //uint32_t addr_of_struct = p->readDWord(addr);
        // FIXME: maybe truncates
        buffer->blockflags.whole = block->flags;
        return true;
    }
    return false;
}

/*
 * Tiletypes
 */

bool Maps::ReadTileTypes (uint32_t x, uint32_t y, uint32_t z, tiletypes40d *buffer)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        memcpy(buffer, block->tiletype, sizeof(tiletypes40d));
        return true;
    }
    return false;
}

bool Maps::WriteTileTypes (uint32_t x, uint32_t y, uint32_t z, tiletypes40d *buffer)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        memcpy(block->tiletype, buffer, sizeof(tiletypes40d));
        return true;
    }
    return false;
}

/*
 * Dirty bit
 */
bool Maps::ReadDirtyBit(uint32_t x, uint32_t y, uint32_t z, bool &dirtybit)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        dirtybit = block->flags.is_set(df::block_flags::Designated);
        return true;
    }
    return false;
}

bool Maps::WriteDirtyBit(uint32_t x, uint32_t y, uint32_t z, bool dirtybit)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        block->flags.set(df::block_flags::Designated, dirtybit);
        return true;
    }
    return false;
}
/*
/*
 * Block flags
 */
// FIXME: maybe truncates, still bullshit
bool Maps::ReadBlockFlags(uint32_t x, uint32_t y, uint32_t z, t_blockflags &blockflags)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        blockflags.whole = block->flags;
        return true;
    }
    return false;
}
//FIXME: maybe truncated, still bullshit
bool Maps::WriteBlockFlags(uint32_t x, uint32_t y, uint32_t z, t_blockflags blockflags)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        return (block->flags = blockflags.whole);
    }
    return false;
}

/*
 * Designations
 */
bool Maps::ReadDesignations (uint32_t x, uint32_t y, uint32_t z, designations40d *buffer)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        memcpy(buffer, block->designation, sizeof(designations40d));
        return true;
    }
    return false;
}

bool Maps::WriteDesignations (uint32_t x, uint32_t y, uint32_t z, designations40d *buffer)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        memcpy(block->designation, buffer, sizeof(designations40d));
        return true;
    }
    return false;
}

/*
 * Occupancies
 */
bool Maps::ReadOccupancy (uint32_t x, uint32_t y, uint32_t z, occupancies40d *buffer)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        memcpy(buffer, block->occupancy, sizeof(occupancies40d));
        return true;
    }
    return false;
}

bool Maps::WriteOccupancy (uint32_t x, uint32_t y, uint32_t z, occupancies40d *buffer)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        memcpy(block->occupancy, buffer, sizeof(occupancies40d));
        return true;
    }
    return false;
}

/*
 * Temperatures
 */
bool Maps::ReadTemperatures(uint32_t x, uint32_t y, uint32_t z, t_temperatures *temp1, t_temperatures *temp2)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        if(temp1)
            memcpy(temp1, block->temperature_1, sizeof(t_temperatures));
        if(temp2)
            memcpy(temp2, block->temperature_2, sizeof(t_temperatures));
        return true;
    }
    return false;
}
bool Maps::WriteTemperatures (uint32_t x, uint32_t y, uint32_t z, t_temperatures *temp1, t_temperatures *temp2)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        if(temp1)
            memcpy(block->temperature_1, temp1, sizeof(t_temperatures));
        if(temp2)
            memcpy(block->temperature_2, temp2, sizeof(t_temperatures));
        return true;
    }
    return false;
}

/*
 * Region Offsets - used for layer geology
 */
bool Maps::ReadRegionOffsets (uint32_t x, uint32_t y, uint32_t z, biome_indices40d *buffer)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        memcpy(buffer, block->region_offset,sizeof(biome_indices40d));
        return true;
    }
    return false;
}
bool Maps::StopFeatures()
{
    if(d->FeaturesStarted)
    {
        d->local_feature_store.clear();
        d->v_global_feature.clear();
        d->m_local_feature.clear();
        d->FeaturesStarted = false;
        return true;
    }
    return false;
}

bool Maps::StartFeatures()
{
    MAPS_GUARD
    if(d->FeaturesStarted) return true;
    if(!d->hasFeatures) return false;

    // regionX and regionY are in embark squares!
    // we convert to full region tiles
    // this also works in adventure mode
    // region X coord - whole regions

    for(uint32_t blockX = 0; blockX < world->map.x_count_block; blockX ++)
        for(uint32_t blockY = 0; blockY < world->map.y_count_block; blockY ++)
    {
        // regionX and regionY are in embark squares!
        // we convert to full region tiles
        // this also works in adventure mode
        // region X coord - whole regions
        uint32_t region_x = ( (blockX / 3) + world->map.region_x ) / 16;
        // region Y coord - whole regions
        uint32_t region_y = ( (blockY / 3) + world->map.region_y ) / 16;
        uint32_t bigregion_x = region_x / 16;
        uint32_t bigregion_y = region_y / 16;
        uint32_t sub_x = region_x % 16;
        uint32_t sub_y = region_y % 16;
        // megaregions = 16x16 squares of regions = 256x256 squares of embark squares

        // base = pointer to local feature structure (inside world data struct)
        // bigregion is 16x16 regions. for each bigregion in X dimension:

        if(world->world_data->unk_204[bigregion_x][bigregion_y].features)
        {
            vector <df::feature_init *> *features = &world->world_data->unk_204[bigregion_x][bigregion_y].features->feature_init[sub_x][sub_y];
            uint32_t size = features->size();
            DFCoord pc(blockX,blockY);
            std::vector<t_feature *> tempvec;
            for(uint32_t i = 0; i < size; i++)
            {
                df::feature_init * cur_ptr = features->at(i);

                map <void *, t_feature>::iterator it;
                it = d->local_feature_store.find(cur_ptr);
                // do we already have the feature?
                if(it != d->local_feature_store.end())
                {
                    // push pointer to existing feature
                    tempvec.push_back(&((*it).second));
                }
                // no?
                else
                {
                    t_feature tftemp;
                    tftemp.discovered = false;
                    tftemp.origin = cur_ptr;
		    tftemp.type = cur_ptr->getType();
		    cur_ptr->getMaterial(&tftemp.main_material, &tftemp.sub_material);
                    d->local_feature_store[cur_ptr] = tftemp;
                    // push pointer
                    tempvec.push_back(&(d->local_feature_store[cur_ptr]));
                }
            }
            d->m_local_feature[pc] = tempvec;
        }
    }

    // enumerate global features
    uint32_t size = world->world_data->underground_regions.size();
    d->v_global_feature.clear();
    d->v_global_feature.reserve(size);
    for (uint32_t i = 0; i < size; i++)
    {
        t_feature temp;
	df::feature_init * feat_ptr = world->world_data->underground_regions[i]->feature_init;
        temp.origin = feat_ptr;
        temp.discovered = false;
	temp.type = feat_ptr->getType();
	feat_ptr->getMaterial(&temp.main_material, &temp.sub_material);
        d->v_global_feature.push_back(temp);
    }
    d->FeaturesStarted = true;
    return true;
}

t_feature * Maps::GetGlobalFeature(int16_t index)
{
    if(!d->FeaturesStarted) return 0;
    if(index < 0 || uint16_t(index) >= d->v_global_feature.size())
        return 0;
    return &(d->v_global_feature[index]);
}

std::vector <t_feature *> * Maps::GetLocalFeatures(DFCoord coord)
{
    if(!d->FeaturesStarted) return 0;
    coord.z = 0; // just making sure
    map <DFCoord, std::vector <t_feature* > >::iterator iter = d->m_local_feature.find(coord);
    if(iter != d->m_local_feature.end())
    {
        return &((*iter).second);
    }
    return 0;
}

bool Maps::ReadFeatures(uint32_t x, uint32_t y, uint32_t z, int16_t & local, int16_t & global)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        local = block->local_feature;
        global = block->global_feature;
        return true;
    }
    return false;
}

bool Maps::WriteFeatures(uint32_t x, uint32_t y, uint32_t z, const int16_t & local, const int16_t & global)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        block->local_feature = local;
        block->global_feature = global;
        return true;
    }
    return false;
}

bool Maps::ReadFeatures(uint32_t x, uint32_t y, uint32_t z, t_feature ** local, t_feature ** global)
{
    if(!d->FeaturesStarted) return false;
    int16_t loc, glob;
    if(!ReadFeatures(x,y,z,loc,glob)) return false;

    if(global && glob != -1)
        *global = &(d->v_global_feature[glob]);
    else if (global)
        *global = 0;

    if(local && loc != -1)
    {
        DFCoord foo(x,y,0);
        map <DFCoord, std::vector <t_feature* > >::iterator iter = d->m_local_feature.find(foo);
        if(iter != d->m_local_feature.end())
        {
            *local = ((*iter).second)[loc];
        }
        else *local = 0;
    }
    else if(local)
        *local = 0;
    return true;
}

bool Maps::ReadFeatures(mapblock40d * block, t_feature ** local, t_feature ** global)
{
    if(!block) return false;
    if(!d->FeaturesStarted) return false;
    DFCoord c = block->position;
    c.z = 0;
    *global = 0;
    *local = 0;
    if(block->global_feature != -1)
        *global = &(d->v_global_feature[block->global_feature]);
    if(local && block->local_feature != -1)
    {
        map <DFCoord, std::vector <t_feature* > >::iterator iter = d->m_local_feature.find(c);
        if(iter != d->m_local_feature.end())
        {
            *local = ((*iter).second)[block->local_feature];
        }
        else *local = 0;
    }
    return true;
}

bool Maps::SetBlockLocalFeature(uint32_t x, uint32_t y, uint32_t z, int16_t local)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        block->local_feature = local;
        return true;
    }
    return false;
}

bool Maps::SetBlockGlobalFeature(uint32_t x, uint32_t y, uint32_t z, int16_t global)
{
    MAPS_GUARD
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        block->global_feature = global;
        return true;
    }
    return false;
}

/*
 * Block events
 */
bool Maps::SortBlockEvents(uint32_t x, uint32_t y, uint32_t z,
    vector <df::block_square_event_mineralst *>* veins,
    vector <df::block_square_event_frozen_liquidst *>* ices,
    vector <df::block_square_event_material_spatterst *> *splatter,
    vector <df::block_square_event_grassst *> *grass,
    vector <df::block_square_event_world_constructionst *> *constructions)
{
    MAPS_GUARD
    Process* p = d->owner;

    df::map_block * block = getBlock(x,y,z);
    if(veins) veins->clear();
    if(ices) ices->clear();
    if(splatter) splatter->clear();
    if(grass) grass->clear();
    if(constructions) constructions->clear();

    if (!block)
        return false;
    uint32_t size = block->block_events.size();
    // read all veins
    for (uint32_t i = 0; i < size;i++)
    {
        df::block_square_event *evt = block->block_events[i];
        switch (evt->getType())
        {
        case df::block_square_event_type::mineral:
            if (veins)
                veins->push_back((df::block_square_event_mineralst *)evt);
            break;
        case df::block_square_event_type::frozen_liquid:
            if (ices)
                ices->push_back((df::block_square_event_frozen_liquidst *)evt);
            break;
        case df::block_square_event_type::material_spatter:
            if (splatter)
                splatter->push_back((df::block_square_event_material_spatterst *)evt);
            break;
        case df::block_square_event_type::grass:
            if (grass)
                grass->push_back((df::block_square_event_grassst *)evt);
            break;
        case df::block_square_event_type::world_construction:
            if (constructions)
                constructions->push_back((df::block_square_event_world_constructionst *)evt);
            break;
        }
    }
    return true;
}

bool Maps::RemoveBlockEvent(uint32_t x, uint32_t y, uint32_t z, df::block_square_event * which)
{
    MAPS_GUARD
    Process* p = d->owner;

    df::map_block * block = getBlock(x,y,z);
    if(block)
    {
        for(int i = 0; i < block->block_events.size();i++)
        {
            if (block->block_events[i] == which)
            {
                free(which);
                block->block_events.erase(block->block_events.begin() + i);
                return true;
            }
        }
    }
    return false;
}

/*
* Layer geology
*/
bool Maps::ReadGeology (vector < vector <uint16_t> >& assign)
{
    MAPS_GUARD
    if(!d->hasGeology) return false;

    // iterate over 8 surrounding regions + local region
    for (int i = eNorthWest; i < eBiomeCount; i++)
    {
        // check against worldmap boundaries, fix if needed
        // regionX is in embark squares
        // regionX/16 is in 16x16 embark square regions
        // i provides -1 .. +1 offset from the current region
        int bioRX = world->map.region_x / 16 + ((i % 3) - 1);
        if (bioRX < 0) bioRX = 0;
        if (bioRX >= world->world_data->world_width) bioRX = world->world_data->world_width - 1;
        int bioRY = world->map.region_y / 16 + ((i / 3) - 1);
        if (bioRY < 0) bioRY = 0;
        if (bioRY >= world->world_data->world_height) bioRY = world->world_data->world_height - 1;

        // get index into geoblock vector
        uint16_t geoindex = world->world_data->unk_1c0[bioRX][bioRY].geo_index;

        /// geology blocks have a vector of layer descriptors
        // get the vector with pointer to layers
	vector <df::world_data::T_unk_190::T_unk_4 *> *geolayers = &world->world_data->unk_190[geoindex]->unk_4;

        /// layer descriptor has a field that determines the type of stone/soil
        d->v_geology[i].reserve (geolayers->size());
        // finally, read the layer matgloss
        for (uint32_t j = 0;j < geolayers->size();j++)
        {
            d->v_geology[i].push_back (geolayers->at(j)->unk_4);
        }
    }
    assign.clear();
    assign.reserve (eBiomeCount);
    for (int i = 0; i < eBiomeCount;i++)
    {
        assign.push_back (d->v_geology[i]);
    }
    return true;
}

bool Maps::ReadLocalFeatures( std::map <DFCoord, std::vector<t_feature *> > & local_features )
{
    StopFeatures();
    StartFeatures();
    if(d->FeaturesStarted)
    {
        local_features = d->m_local_feature;
        return true;
    }
    return false;
}

bool Maps::ReadGlobalFeatures( std::vector <t_feature> & features )
{
    StopFeatures();
    StartFeatures();
    if(d->FeaturesStarted)
    {
        features = d->v_global_feature;
        return true;
    }
    return false;
}

bool Maps::ReadVegetation(uint32_t x, uint32_t y, uint32_t z, std::vector<df_plant *>*& plants)
{
    if(!d->hasVeggies || !d->Started)
        return false;
    df::map_block * block = getBlock(x,y,z);
    if(!block)
        return false;

    plants = (vector<DFHack::df_plant *> *)&block->plants;
    return true;
}
