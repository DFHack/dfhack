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

#include "dfhack/modules/Maps.h"
#include "dfhack/Error.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/Process.h"
#include "dfhack/Vector.h"
#include "ModuleFactory.h"
#include <dfhack/Core.h>

#define MAPS_GUARD if(!d->Started) throw DFHack::Error::ModuleNotInitialized();

using namespace DFHack;

Module* DFHack::createMaps()
{
    return new Maps();
}

const char * DFHack::sa_feature(e_feature index)
{
    switch(index)
    {
        case feature_Other:
            return "Other";
        case feature_Adamantine_Tube:
            return "Adamantine Tube";
        case feature_Underworld:
            return "Underworld";
        case feature_Hell_Temple:
            return "Hell Temple";
        default:
            return "Unknown/Error";
    }
};

struct Maps::Private
{
    uint32_t worldSizeX, worldSizeY;

    uint32_t maps_module;
    struct t_offsets
    {
        // FIXME: those need a rework really. Why did Toady use virtual inheritance for such vastly different types anyway?
        void * vein_mineral_vptr;
        void * vein_ice_vptr;
        void * vein_spatter_vptr;
        void * vein_grass_vptr;
        void * vein_worldconstruction_vptr;
        /*
        GEOLOGY
        */
        uint32_t world_regions;// mem->getAddress ("ptr2_region_array");
        uint32_t region_size;// =  minfo->getHexValue ("region_size");
        uint32_t region_geo_index_offset;// =  minfo->getOffset ("region_geo_index_off");
        uint32_t world_geoblocks_vector;// =  minfo->getOffset ("geoblock_vector");
        uint32_t world_size_x;// = minfo->getOffset ("world_size_x");
        uint32_t world_size_y;// = minfo->getOffset ("world_size_y");
        uint32_t geolayer_geoblock_offset;// = minfo->getOffset ("geolayer_geoblock_offset");
        uint32_t type_inside_geolayer;// = mem->getOffset ("type_inside_geolayer");
        
        /*
        FEATURES
         */
        // FIXME: replace with a struct pointer, eventually. needs to be mapped out first
        uint32_t world_data;
        uint32_t local_f_start; // offset from world_data
        // FIXME: replace by virtual function call
        uint32_t local_material;
        // FIXME: replace by virtual function call
        uint32_t local_submaterial;
        uint32_t global_vector; // offset from world_data
        uint32_t global_funcptr;
        // FIXME: replace by virtual function call
        uint32_t global_material;
        // FIXME: replace by virtual function call
        uint32_t global_submaterial;
    } offsets;

    Process * owner;
    OffsetGroup *OG_vector;
    bool Inited;
    bool FeaturesStarted;
    bool Started;
    bool hasGeology;
    bool hasFeatures;
    bool hasVeggies;

    set <uint32_t> unknown_veins;

    // map between feature address and the read object
    map <uint32_t, t_feature> local_feature_store;
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
    Private::t_offsets &off = d->offsets;
    d->hasFeatures = d->hasGeology = d->hasVeggies = true;

    // get the offsets once here
    OffsetGroup *OG_Maps = mem->getGroup("Maps");
    off.world_data = OG_Maps->getAddress("world_data");
    {
        mdata = (map_data *) OG_Maps->getAddress ("map_data");
        off.world_size_x = OG_Maps->getOffset ("world_size_x_from_wdata");
        off.world_size_y = OG_Maps->getOffset ("world_size_y_from_wdata");
        try
        {
            OffsetGroup *OG_Geology = OG_Maps->getGroup("geology");
            off.world_regions =  OG_Geology->getOffset ("ptr2_region_array_from_wdata");
            off.world_geoblocks_vector =  OG_Geology->getOffset ("geoblock_vector_from_wdata");
            off.region_size =  OG_Geology->getHexValue ("region_size");
            off.region_geo_index_offset =  OG_Geology->getOffset ("region_geo_index_off");
            off.geolayer_geoblock_offset = OG_Geology->getOffset ("geolayer_geoblock_offset");
            off.type_inside_geolayer = OG_Geology->getOffset ("type_inside_geolayer");
        }
        catch(Error::AllMemdef &)
        {
            d->hasGeology = false;
        }
        OffsetGroup *OG_global_features = OG_Maps->getGroup("features")->getGroup("global");
        OffsetGroup *OG_local_features = OG_Maps->getGroup("features")->getGroup("local");
        try
        {
            off.local_f_start = OG_local_features->getOffset("start_ptr_from_wdata");
            off.global_vector = OG_global_features->getOffset("vector_from_wdata");
            off.local_material = OG_local_features->getOffset("material");
            off.local_submaterial = OG_local_features->getOffset("submaterial");

            off.global_funcptr =  OG_global_features->getOffset("funcptr");
            off.global_material =  OG_global_features->getOffset("material");
            off.global_submaterial =  OG_global_features->getOffset("submaterial");
        }
        catch(Error::AllMemdef &)
        {
            d->hasFeatures = false;
        }
    }
    d->OG_vector = mem->getGroup("vector");

    // these can (will) fail and will be found when looking at the actual veins later
    // basically a cache
    off.vein_ice_vptr = 0;
    mem->resolveClassnameToVPtr("block_square_event_frozen_liquid", off.vein_ice_vptr);
    off.vein_mineral_vptr = 0;
    mem->resolveClassnameToVPtr("block_square_event_mineral",off.vein_mineral_vptr);
    off.vein_spatter_vptr = 0;
    mem->resolveClassnameToVPtr("block_square_event_material_spatterst",off.vein_spatter_vptr);
    off.vein_grass_vptr = 0;
    mem->resolveClassnameToVPtr("block_square_event_grassst",off.vein_grass_vptr);
    off.vein_worldconstruction_vptr = 0;
    mem->resolveClassnameToVPtr("block_square_event_world_constructionst",off.vein_worldconstruction_vptr);
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
    Private::t_offsets &off = d->offsets;

    // is there a map?
    //uint32_t x_array_loc = p->readDWord (off.map_offset);
    if (!mdata->map)
    {
        return false;
    }

    // get the size
    uint32_t & mx = mdata->x_size_blocks;
    uint32_t & my = mdata->y_size_blocks;
    uint32_t & mz = mdata->z_size_blocks;

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
    x = mdata->x_size_blocks;
    y = mdata->y_size_blocks;
    z = mdata->z_size_blocks;
}

// getter for map position
void Maps::getPosition (int32_t& x, int32_t& y, int32_t& z)
{
    MAPS_GUARD
    x = mdata->x_area_offset;
    y = mdata->y_area_offset;
    z = mdata->z_area_offset;
}

bool Maps::Finish()
{
    return true;
}

/*
 * Block reading
 */

df_block* Maps::getBlock (uint32_t x, uint32_t y, uint32_t z)
{
    MAPS_GUARD
    if(x >= mdata->x_size_blocks || y >= mdata->y_size_blocks || z >= mdata->z_size_blocks)
        return 0;
    return mdata->map[x][y][z];
}

bool Maps::ReadBlock40d(uint32_t x, uint32_t y, uint32_t z, mapblock40d * buffer)
{
    MAPS_GUARD
    Process *p = d->owner;
    df_block * block = getBlock(x,y,z);
    if (block)
    {
        buffer->position = DFCoord(x,y,z);
        memcpy(buffer->tiletypes,block->tiletype, sizeof(tiletypes40d));
        memcpy(buffer->designation,block->designation, sizeof(designations40d));
        memcpy(buffer->occupancy,block->occupancy, sizeof(occupancies40d));
        memcpy(buffer->biome_indices,block->region_offset, sizeof(block->region_offset));
        buffer->global_feature = block->global_feature;
        buffer->local_feature = block->local_feature;
        buffer->mystery = block->mystery;
        // FIXME: not 64-bit safe
        buffer->origin = (uint32_t) &block;
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
    if (block)
    {
        dirtybit = block->flags.is_set(1);
        return true;
    }
    return false;
}

bool Maps::WriteDirtyBit(uint32_t x, uint32_t y, uint32_t z, bool dirtybit)
{
    MAPS_GUARD
    df_block * block = getBlock(x,y,z);
    if (block)
    {
        block->flags.set(1,dirtybit);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    // can't be used without a map!
    if(!mdata->map)
        return false;

    Process * p = d->owner;
    Private::t_offsets &off = d->offsets;
    uint32_t base = 0;
    uint32_t global_feature_vector = 0;

    uint32_t world = p->readDWord(off.world_data);
    if(!world) return false;
    base = p->readDWord(world + off.local_f_start);
    global_feature_vector = p->readDWord(off.world_data) + off.global_vector;

    // deref pointer to the humongo-structure
    if(!base)
        return false;

    // regionX and regionY are in embark squares!
    // we convert to full region tiles
    // this also works in adventure mode
    // region X coord - whole regions
    const uint32_t sizeof_vec = d->OG_vector->getHexValue("sizeof");
    const uint32_t sizeof_elem = 16;
    const uint32_t offset_elem = 4;
    const uint32_t loc_main_mat_offset = off.local_material; 
    const uint32_t loc_sub_mat_offset = off.local_submaterial;
    const uint32_t sizeof_16vec = 16* sizeof_vec;

    for(uint32_t blockX = 0; blockX < mdata->x_size_blocks; blockX ++)
        for(uint32_t blockY = 0; blockY < mdata->y_size_blocks; blockY ++)
    {
        // regionX and regionY are in embark squares!
        // we convert to full region tiles
        // this also works in adventure mode
        // region X coord - whole regions
        uint32_t region_x = ( (blockX / 3) + mdata->x_area_offset ) / 16;
        // region Y coord - whole regions
        uint32_t region_y = ( (blockY / 3) + mdata->y_area_offset ) / 16;
        uint32_t bigregion_x = region_x / 16;
        uint32_t bigregion_y = region_y / 16;
        uint32_t sub_x = region_x % 16;
        uint32_t sub_y = region_y % 16;
        // megaregions = 16x16 squares of regions = 256x256 squares of embark squares

        // base = pointer to local feature structure (inside world data struct)
        // bigregion is 16x16 regions. for each bigregion in X dimension:
        uint32_t mega_column = p->readDWord(base + bigregion_x * 4);

        // 16B structs, second DWORD of the struct is a pointer
        uint32_t loc_f_array16x16 = p->readDWord(mega_column + offset_elem + (sizeof_elem * bigregion_y));
        if(loc_f_array16x16)
        {
            uint32_t feat_vector = loc_f_array16x16 + sizeof_16vec * sub_x + sizeof_vec * sub_y;
            DfVector<uint32_t> p_features(feat_vector);
            uint32_t size = p_features.size();
            DFCoord pc(blockX,blockY);
            std::vector<t_feature *> tempvec;
            for(uint32_t i = 0; i < size; i++)
            {
                uint32_t cur_ptr = p_features[i];

                map <uint32_t, t_feature>::iterator it;
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
                    //FIXME: replace with accessors
                    // create, add to store
                    t_feature tftemp;
                    tftemp.discovered = false; //= p->readDWord(cur_ptr + 4);
                    tftemp.origin = cur_ptr;
                    string name = p->readClassName((void *)p->readDWord( cur_ptr ));
                    if(name == "feature_init_deep_special_tubest")
                    {
                        tftemp.main_material = p->readWord( cur_ptr + loc_main_mat_offset );
                        tftemp.sub_material = p->readDWord( cur_ptr + loc_sub_mat_offset );
                        tftemp.type = feature_Adamantine_Tube;
                    }
                    else if(name == "feature_init_deep_surface_portalst")
                    {
                        tftemp.main_material = p->readWord( cur_ptr + loc_main_mat_offset );
                        tftemp.sub_material = p->readDWord( cur_ptr + loc_sub_mat_offset );
                        tftemp.type = feature_Hell_Temple;
                    }
                    else
                    {
                        tftemp.main_material = -1;
                        tftemp.sub_material = -1;
                        tftemp.type = feature_Other;
                    }
                    d->local_feature_store[cur_ptr] = tftemp;
                    // push pointer
                    tempvec.push_back(&(d->local_feature_store[cur_ptr]));
                }
            }
            d->m_local_feature[pc] = tempvec;
        }
    }
    // deref pointer to the humongo-structure
    const uint32_t global_feature_funcptr = off.global_funcptr;
    const uint32_t glob_main_mat_offset = off.global_material;
    const uint32_t glob_sub_mat_offset = off.global_submaterial;
    DfVector<uint32_t> p_features (global_feature_vector);

    d->v_global_feature.clear();
    uint32_t size = p_features.size();
    d->v_global_feature.reserve(size);
    for(uint32_t i = 0; i < size; i++)
    {
        t_feature temp;
        uint32_t feat_ptr = p->readDWord(p_features[i] + global_feature_funcptr );
        temp.origin = feat_ptr;
        //temp.discovered = p->readDWord( feat_ptr + 4 ); // maybe, placeholder
        temp.discovered = false;

        // FIXME: use the memory_info cache mechanisms
        string name = p->readClassName((void *)p->readDWord( feat_ptr));
        if(name == "feature_init_underworld_from_layerst")
        {
            temp.main_material = p->readWord( feat_ptr + glob_main_mat_offset );
            temp.sub_material = p->readDWord( feat_ptr + glob_sub_mat_offset );
            temp.type = feature_Underworld;
        }
        else
        {
            temp.main_material = -1;
            temp.sub_material = -1;
            temp.type = feature_Other;
        }
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
    df_block * block = getBlock(x,y,z);
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
bool Maps::SortBlockEvents(uint32_t x, uint32_t y, uint32_t z, vector <t_vein *>* veins, vector <t_frozenliquidvein *>* ices, vector <t_spattervein *> *splatter, vector <t_grassvein *> *grass, vector <t_worldconstruction *> *constructions)
{
    MAPS_GUARD
    Process* p = d->owner;

    df_block * block = getBlock(x,y,z);
    if(veins) veins->clear();
    if(ices) ices->clear();
    if(splatter) splatter->clear();
    if(grass) grass->clear();
    if(constructions) constructions->clear();

    Private::t_offsets &off = d->offsets;
    if (!block)
        return false;
    uint32_t size = block->block_events.size();
    // read all veins
    for (uint32_t i = 0; i < size;i++)
    {
        retry:
        // read the vein pointer from the vector
        t_virtual * temp = block->block_events[i];
        void * type = temp->vptr;
        if(type == (void *) off.vein_mineral_vptr)
        {
            if(!veins) continue;
            // store it in the vector
            veins->push_back ((t_vein *) temp);
        }
        else if(type == (void *) off.vein_ice_vptr)
        {
            if(!ices) continue;
            ices->push_back ((t_frozenliquidvein *) temp);
        }
        else if(type == (void *) off.vein_spatter_vptr)
        {
            if(!splatter) continue;
            splatter->push_back ( (t_spattervein *)temp);
        }
        else if(type == (void *) off.vein_grass_vptr)
        {
            if(!grass) continue;
            grass->push_back ((t_grassvein *) temp);
        }
        else if(type == (void *) off.vein_worldconstruction_vptr)
        {
            if(!constructions) continue;
            constructions->push_back ((t_worldconstruction *) temp);
        }
        // previously unseen type of vein
        else
        {
            string cname = p->readClassName(type);
            //string cname = typeid(*(blockevent *)temp).name();
            //Core::getInstance().con.printerr("%s\n",cname.c_str());
            if(!off.vein_ice_vptr && cname == "block_square_event_frozen_liquidst")
            {
                off.vein_ice_vptr = type;
                //Core::getInstance().con.printerr("%s %x\n",cname.c_str(), type);
                goto retry;
            }
            else if(!off.vein_mineral_vptr &&cname == "block_square_event_mineralst")
            {
                off.vein_mineral_vptr = type;
                //Core::getInstance().con.printerr("%s %x\n",cname.c_str(), type);
                goto retry;
            }
            else if(!off.vein_spatter_vptr && cname == "block_square_event_material_spatterst")
            {
                off.vein_spatter_vptr = type;
                //Core::getInstance().con.printerr("%s %x\n",cname.c_str(), type);
                goto retry;
            }
            else if(!off.vein_grass_vptr && cname=="block_square_event_grassst")
            {
                off.vein_grass_vptr = type;
                //Core::getInstance().con.printerr("%s %x\n",cname.c_str(), type);
                goto retry;
            }
            else if(!off.vein_worldconstruction_vptr && cname=="block_square_event_world_constructionst")
            {
                off.vein_worldconstruction_vptr = type;
                //Core::getInstance().con.printerr("%s %x\n",cname.c_str(), type);
                goto retry;
            }
#ifdef DEBUG
            else // this is something we've never seen before
            {
                if(!d->unknown_veins.count(type))
                {
                    cerr << "unknown vein " << cname << hex << " 0x" << temp << " block: 0x" << addr << dec << endl;
                    d->unknown_veins.insert(type);
                }
            }
#endif
        }
    }
    return true;
}

bool Maps::RemoveBlockEvent(uint32_t x, uint32_t y, uint32_t z, t_virtual * which)
{
    MAPS_GUARD
    Process* p = d->owner;

    df_block * block = getBlock(x,y,z);
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
    Process *p = d->owner;
    // get needed addresses and offsets. Now this is what I call crazy.
    uint16_t worldSizeX, worldSizeY;
    uint32_t regions, geoblocks_vector_addr;
    Private::t_offsets &off = d->offsets;
    // get world size
    uint32_t world = p->readDWord(off.world_data);
    p->readWord (world + off.world_size_x, worldSizeX);
    p->readWord (world + off.world_size_y, worldSizeY);
    regions = p->readDWord ( world + off.world_regions); // ptr2_region_array
    geoblocks_vector_addr = world + off.world_geoblocks_vector;

    // read the geoblock vector
    DfVector <uint32_t> geoblocks (geoblocks_vector_addr);

    // iterate over 8 surrounding regions + local region
    for (int i = eNorthWest; i < eBiomeCount; i++)
    {
        // check against worldmap boundaries, fix if needed
        // regionX is in embark squares
        // regionX/16 is in 16x16 embark square regions
        // i provides -1 .. +1 offset from the current region
        int bioRX = mdata->x_area_offset / 16 + ((i % 3) - 1);
        if (bioRX < 0) bioRX = 0;
        if (bioRX >= worldSizeX) bioRX = worldSizeX - 1;
        int bioRY = mdata->y_area_offset / 16 + ((i / 3) - 1);
        if (bioRY < 0) bioRY = 0;
        if (bioRY >= worldSizeY) bioRY = worldSizeY - 1;

        /// regions are a 2d array. consists of pointers to arrays of regions
        /// regions are of region_size size
        // get pointer to column of regions
        uint32_t geoX;
        p->readDWord (regions + bioRX*4, geoX);

        // get index into geoblock vector
        uint16_t geoindex;
        p->readWord (geoX + bioRY*off.region_size + off.region_geo_index_offset, geoindex);

        /// geology blocks are assigned to regions from a vector
        // get the geoblock from the geoblock vector using the geoindex
        // read the matgloss pointer from the vector into temp
        uint32_t geoblock_off = geoblocks[geoindex];

        /// geology blocks have a vector of layer descriptors
        // get the vector with pointer to layers
        DfVector <uint32_t> geolayers (geoblock_off + off.geolayer_geoblock_offset); // let's hope
        // make sure we don't load crap
        assert (geolayers.size() > 0 && geolayers.size() <= 16);

        /// layer descriptor has a field that determines the type of stone/soil
        d->v_geology[i].reserve (geolayers.size());
        // finally, read the layer matgloss
        for (uint32_t j = 0;j < geolayers.size();j++)
        {
            // read pointer to a layer
            uint32_t geol_offset = geolayers[j];
            // read word at pointer + 2, store in our geology vectors
            d->v_geology[i].push_back (p->readWord (geol_offset + off.type_inside_geolayer));
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
    df_block * block = getBlock(x,y,z);
    if(!block)
        return false;
    Private::t_offsets &off = d->offsets;
    plants = &block->plants;
    return true;
}
