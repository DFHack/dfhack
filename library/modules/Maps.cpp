/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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
#include "ContextShared.h"
#include "dfhack/modules/Maps.h"
#include "dfhack/DFError.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFVector.h"
#include <set>
#include "ModuleFactory.h"

#define MAPS_GUARD if(!d->Started) throw DFHack::Error::ModuleNotInitialized();

using namespace DFHack;

Module* DFHack::createMaps(DFContextShared * d)
{
    return new Maps(d);
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
    uint32_t * block;
    uint32_t x_block_count, y_block_count, z_block_count;
    uint32_t regionX, regionY, regionZ;
    uint32_t worldSizeX, worldSizeY;

    uint32_t maps_module;
    struct t_offsets
    {
        uint32_t map_offset;// = d->offset_descriptor->getAddress ("map_data");
        uint32_t x_count_offset;// = d->offset_descriptor->getAddress ("x_count");
        uint32_t y_count_offset;// = d->offset_descriptor->getAddress ("y_count");
        uint32_t z_count_offset;// = d->offset_descriptor->getAddress ("z_count");
        /*
         Block
        */
        uint32_t tile_type_offset;// = d->offset_descriptor->getOffset ("type");
        uint32_t designation_offset;// = d->offset_descriptor->getOffset ("designation");
        uint32_t occupancy_offset;// = d->offset_descriptor->getOffset ("occupancy");
        uint32_t biome_stuffs;// = d->offset_descriptor->getOffset ("biome_stuffs");
        uint32_t veinvector;// = d->offset_descriptor->getOffset ("v_vein");
        uint32_t vegvector;
        uint32_t temperature1_offset;
        uint32_t temperature2_offset;
        uint32_t global_feature_offset;
        uint32_t local_feature_offset;
        
        uint32_t vein_mineral_vptr;
        uint32_t vein_ice_vptr;
        uint32_t vein_spatter_vptr;
        uint32_t vein_grass_vptr;
        uint32_t vein_worldconstruction_vptr;
        /*
        GEOLOGY
        */
        uint32_t region_x_offset;// = minfo->getAddress ("region_x");
        uint32_t region_y_offset;// = minfo->getAddress ("region_y");
        uint32_t region_z_offset;// =  minfo->getAddress ("region_z");
        
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
        uint32_t world_data;
        uint32_t local_f_start; // offset from world_data or absolute address.
        uint32_t local_material;
        uint32_t local_submaterial;
        uint32_t global_vector; // offset from world_data or absolute address.
        uint32_t global_funcptr;
        uint32_t global_material;
        uint32_t global_submaterial;
        /*
         * Vegetation
         */
        uint32_t tree_desc_offset;
    } offsets;

    DFContextShared *d;
    Process * owner;
    OffsetGroup *OG_vector;
    bool Inited;
    bool FeaturesStarted;
    bool Started;
    bool hasGeology;
    bool hasFeatures;
    bool hasVeggies;

    bool usesWorldDataPtr;

    set <uint32_t> unknown_veins;

    // map between feature address and the read object
    map <uint32_t, t_feature> local_feature_store;
    map <DFCoord, vector <t_feature *> > m_local_feature;
    vector <t_feature> v_global_feature;

    vector<uint16_t> v_geology[eBiomeCount];
};

Maps::Maps(DFContextShared* _d)
{
    d = new Private;
    d->d = _d;
    Process *p = d->owner = _d->p;
    d->Inited = d->FeaturesStarted = d->Started = false;
    d->block = NULL;
    d->usesWorldDataPtr = false;

    DFHack::VersionInfo * mem = p->getDescriptor();
    Private::t_offsets &off = d->offsets;
    d->hasFeatures = d->hasGeology = d->hasVeggies = true;

    // get the offsets once here
    OffsetGroup *OG_Maps = mem->getGroup("Maps");
    try
    {
        off.world_data = OG_Maps->getAddress("world_data");
        d->usesWorldDataPtr = true;
        cout << "uses world ptr" << endl;
    }catch(Error::AllMemdef &){}

    {
        off.map_offset = OG_Maps->getAddress ("map_data");
        off.x_count_offset = OG_Maps->getAddress ("x_count_block");
        off.y_count_offset = OG_Maps->getAddress ("y_count_block");
        off.z_count_offset = OG_Maps->getAddress ("z_count_block");
        off.region_x_offset = OG_Maps->getAddress ("region_x");
        off.region_y_offset = OG_Maps->getAddress ("region_y");
        off.region_z_offset =  OG_Maps->getAddress ("region_z");
        if(d->usesWorldDataPtr)
        {
            off.world_size_x = OG_Maps->getOffset ("world_size_x_from_wdata");
            off.world_size_y = OG_Maps->getOffset ("world_size_y_from_wdata");
        }
        else
        {
            off.world_size_x = OG_Maps->getAddress ("world_size_x");
            off.world_size_y = OG_Maps->getAddress ("world_size_y");
        }
        OffsetGroup *OG_MapBlock = OG_Maps->getGroup("block");
        {
            off.tile_type_offset = OG_MapBlock->getOffset ("type");
            off.designation_offset = OG_MapBlock->getOffset ("designation");
            off.occupancy_offset = OG_MapBlock->getOffset("occupancy");
            off.biome_stuffs = OG_MapBlock->getOffset ("biome_stuffs");
            off.veinvector = OG_MapBlock->getOffset ("vein_vector");
            off.local_feature_offset = OG_MapBlock->getOffset ("feature_local");
            off.global_feature_offset = OG_MapBlock->getOffset ("feature_global");
            off.temperature1_offset = OG_MapBlock->getOffset ("temperature1");
            off.temperature2_offset = OG_MapBlock->getOffset ("temperature2");
        }

        try
        {
            OffsetGroup *OG_Geology = OG_Maps->getGroup("geology");
            if(d->usesWorldDataPtr)
            {
                off.world_regions =  OG_Geology->getOffset ("ptr2_region_array_from_wdata");
                off.world_geoblocks_vector =  OG_Geology->getOffset ("geoblock_vector_from_wdata");
            }
            else
            {
                off.world_regions =  OG_Geology->getAddress ("ptr2_region_array");
                off.world_geoblocks_vector =  OG_Geology->getAddress ("geoblock_vector");
            }
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
            if(d->usesWorldDataPtr)
            {
                off.local_f_start = OG_local_features->getOffset("start_ptr_from_wdata");
                off.global_vector = OG_global_features->getOffset("vector_from_wdata");
            }
            else
            {
                off.local_f_start = OG_local_features->getAddress("start_ptr");
                off.global_vector = OG_global_features->getAddress("vector");
            }
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

        try
        {
            OffsetGroup * OG_Veg = d->d->offset_descriptor->getGroup("Vegetation");
            off.vegvector = OG_MapBlock->getOffset ("vegetation_vector");
            off.tree_desc_offset = OG_Veg->getOffset ("tree_desc_offset");
        }
        catch(Error::AllMemdef &)
        {
            d->hasVeggies = false;
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

    // get the map pointer
    uint32_t x_array_loc = p->readDWord (off.map_offset);
    if (!x_array_loc)
    {
        return false;
    }

    // get the size
    uint32_t mx, my, mz;
    mx = d->x_block_count = p->readDWord (off.x_count_offset);
    my = d->y_block_count = p->readDWord (off.y_count_offset);
    mz = d->z_block_count = p->readDWord (off.z_count_offset);

    // test for wrong map dimensions
    if (mx == 0 || mx > 48 || my == 0 || my > 48 || mz == 0)
    {
        throw Error::BadMapDimensions(mx, my);
        //return false;
    }

    // read position of the region inside DF world
    p->readDWord (off.region_x_offset, d->regionX);
    p->readDWord (off.region_y_offset, d->regionY);
    p->readDWord (off.region_z_offset, d->regionZ);

    // alloc array for pointers to all blocks
    d->block = new uint32_t[mx*my*mz];
    uint32_t *temp_x = new uint32_t[mx];
    uint32_t *temp_y = new uint32_t[my];

    p->read (x_array_loc, mx * sizeof (uint32_t), (uint8_t *) temp_x);
    for (uint32_t x = 0; x < mx; x++)
    {
        p->read (temp_x[x], my * sizeof (uint32_t), (uint8_t *) temp_y);
        // y -> map column
        for (uint32_t y = 0; y < my; y++)
        {
            p->read (temp_y[y],
                   mz * sizeof (uint32_t),
                   (uint8_t *) (d->block + x*my*mz + y*mz));
        }
    }
    delete [] temp_x;
    delete [] temp_y;
    d->Started = true;
    return true;
}

// getter for map size
void Maps::getSize (uint32_t& x, uint32_t& y, uint32_t& z)
{
    MAPS_GUARD
    x = d->x_block_count;
    y = d->y_block_count;
    z = d->z_block_count;
}

bool Maps::Finish()
{
    if (d->block != NULL)
    {
        delete [] d->block;
        d->block = NULL;
    }
    return true;
}

/*
 * Block reading
 */

bool Maps::isValidBlock (uint32_t x, uint32_t y, uint32_t z)
{
    MAPS_GUARD
    if ( x >= d->x_block_count || y >= d->y_block_count || z >= d->z_block_count)
        return false;
    return d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z] != 0;
}

uint32_t Maps::getBlockPtr (uint32_t x, uint32_t y, uint32_t z)
{
    MAPS_GUARD
    if ( x >= d->x_block_count || y >= d->y_block_count || z >= d->z_block_count)
        return 0;
    return d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
}

bool Maps::ReadBlock40d(uint32_t x, uint32_t y, uint32_t z, mapblock40d * buffer)
{
    MAPS_GUARD
    Process *p = d->owner;
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        buffer->position = DFCoord(x,y,z);
        p->read (addr + d->offsets.tile_type_offset, sizeof (buffer->tiletypes), (uint8_t *) buffer->tiletypes);
        p->read (addr + d->offsets.designation_offset, sizeof (buffer->designation), (uint8_t *) buffer->designation);
        p->read (addr + d->offsets.occupancy_offset, sizeof (buffer->occupancy), (uint8_t *) buffer->occupancy);
        p->read (addr + d->offsets.biome_stuffs, sizeof (biome_indices40d), (uint8_t *) buffer->biome_indices);
        p->readWord(addr + d->offsets.global_feature_offset, (uint16_t&) buffer->global_feature);
        p->readWord(addr + d->offsets.local_feature_offset, (uint16_t&)buffer->local_feature);
        buffer->origin = addr;
        uint32_t addr_of_struct = p->readDWord(addr);
        buffer->blockflags.whole = p->readDWord(addr_of_struct);
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
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        d->owner->read (addr + d->offsets.tile_type_offset, sizeof (tiletypes40d), (uint8_t *) buffer);
        return true;
    }
    return false;
}

bool Maps::WriteTileTypes (uint32_t x, uint32_t y, uint32_t z, tiletypes40d *buffer)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        d->owner->write (addr + d->offsets.tile_type_offset, sizeof (tiletypes40d), (uint8_t *) buffer);
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
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if(addr)
    {
        Process * p = d->owner;
        uint32_t addr_of_struct = p->readDWord(addr);
        dirtybit = p->readDWord(addr_of_struct) & 1;
        return true;
    }
    return false;
}

bool Maps::WriteDirtyBit(uint32_t x, uint32_t y, uint32_t z, bool dirtybit)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        Process * p = d->owner;
        uint32_t addr_of_struct = p->readDWord(addr);
        uint32_t dirtydword = p->readDWord(addr_of_struct);
        dirtydword &= 0xFFFFFFFE;
        dirtydword |= (uint32_t) dirtybit;
        p->writeDWord (addr_of_struct, dirtydword);
        return true;
    }
    return false;
}

/*
 * Block flags
 */
bool Maps::ReadBlockFlags(uint32_t x, uint32_t y, uint32_t z, t_blockflags &blockflags)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if(addr)
    {
        Process * p = d->owner;
        uint32_t addr_of_struct = p->readDWord(addr);
        blockflags.whole = p->readDWord(addr_of_struct);
        return true;
    }
    return false;
}
bool Maps::WriteBlockFlags(uint32_t x, uint32_t y, uint32_t z, t_blockflags blockflags)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        Process * p = d->owner;
        uint32_t addr_of_struct = p->readDWord(addr);
        p->writeDWord (addr_of_struct, blockflags.whole);
        return true;
    }
    return false;
}

/*
 * Designations
 */
bool Maps::ReadDesignations (uint32_t x, uint32_t y, uint32_t z, designations40d *buffer)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        d->owner->read (addr + d->offsets.designation_offset, sizeof (designations40d), (uint8_t *) buffer);
        return true;
    }
    return false;
}

bool Maps::WriteDesignations (uint32_t x, uint32_t y, uint32_t z, designations40d *buffer)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        d->owner->write (addr + d->offsets.designation_offset, sizeof (designations40d), (uint8_t *) buffer);
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
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        d->owner->read (addr + d->offsets.occupancy_offset, sizeof (occupancies40d), (uint8_t *) buffer);
        return true;
    }
    return false;
}

bool Maps::WriteOccupancy (uint32_t x, uint32_t y, uint32_t z, occupancies40d *buffer)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        d->owner->write (addr + d->offsets.occupancy_offset, sizeof (occupancies40d), (uint8_t *) buffer);
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
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        if(temp1)
            d->owner->read (addr + d->offsets.temperature1_offset, sizeof (t_temperatures), (uint8_t *) temp1);
        if(temp2)
            d->owner->read (addr + d->offsets.temperature2_offset, sizeof (t_temperatures), (uint8_t *) temp2);
        return true;
    }
    return false;
}
bool Maps::WriteTemperatures (uint32_t x, uint32_t y, uint32_t z, t_temperatures *temp1, t_temperatures *temp2)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        if(temp1)
            d->owner->write (addr + d->offsets.temperature1_offset, sizeof (t_temperatures), (uint8_t *) temp1);
        if(temp2)
            d->owner->write (addr + d->offsets.temperature2_offset, sizeof (t_temperatures), (uint8_t *) temp2);
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
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        d->owner->read (addr + d->offsets.biome_stuffs, sizeof (biome_indices40d), (uint8_t *) buffer);
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
    if(!d->block)
        return false;

    Process * p = d->owner;
    Private::t_offsets &off = d->offsets;
    uint32_t base = 0;
    uint32_t global_feature_vector = 0;

    if(d->usesWorldDataPtr)
    {
        uint32_t world = p->readDWord(off.world_data);
        if(!world) return false;
        base = p->readDWord(world + off.local_f_start);
        global_feature_vector = p->readDWord(off.world_data) + off.global_vector;
    }
    else
    {
        base = p->readDWord(off.local_f_start);
        global_feature_vector = off.global_vector;
    }

    // deref pointer to the humongo-structure
    if(!base)
        return false;
    const uint32_t sizeof_vec = d->OG_vector->getHexValue("sizeof");
    const uint32_t sizeof_elem = 16;
    const uint32_t offset_elem = 4;
    const uint32_t loc_main_mat_offset = off.local_material; 
    const uint32_t loc_sub_mat_offset = off.local_submaterial;

    for(uint32_t blockX = 0; blockX < d->x_block_count; blockX ++)
        for(uint32_t blockY = 0; blockY < d->x_block_count; blockY ++)
    {
        // region X coord (48x48 tiles)
        uint16_t region_x_local = ( (blockX / 3) + d->regionX ) / 16;
        // region Y coord (48x48 tiles)
        uint64_t region_y_local = ( (blockY / 3) + d->regionY ) / 16;

        // this is just a few pointers to arrays of 16B (4 DWORD) structs
        uint32_t array_elem = p->readDWord(base + (region_x_local / 16) * 4);

        // 16B structs, second DWORD of the struct is a pointer
        uint32_t loc_f_array16x16 = p->readDWord(array_elem + ( sizeof_elem * ( (uint32_t)region_y_local/16)) + offset_elem);
        if(loc_f_array16x16)
        {
            // wtf + sizeof(vector<ptr>) * crap;
            uint32_t feat_vector = loc_f_array16x16 + sizeof_vec * (16 * (region_x_local % 16) + (region_y_local % 16));
            DfVector<uint32_t> p_features(p, feat_vector);
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
                    // create, add to store
                    t_feature tftemp;
                    tftemp.discovered = false; //= p->readDWord(cur_ptr + 4);
                    tftemp.origin = cur_ptr;
                    string name = p->readClassName(p->readDWord( cur_ptr ));
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
    DfVector<uint32_t> p_features (p,global_feature_vector);

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
        string name = p->readClassName(p->readDWord( feat_ptr));
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
    if(index < 0 || index >= d->v_global_feature.size())
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
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        Process * p = d->owner;
        p->readWord(addr + d->offsets.global_feature_offset, (uint16_t&) global);
        p->readWord(addr + d->offsets.local_feature_offset, (uint16_t&)local);
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
    if(global && block->global_feature != -1)
        *global = &(d->v_global_feature[block->global_feature]);
    else if (global)
        *global = 0;

    if(local && block->local_feature != -1)
    {
        map <DFCoord, std::vector <t_feature* > >::iterator iter = d->m_local_feature.find(c);
        if(iter != d->m_local_feature.end())
        {
            *local = ((*iter).second)[block->local_feature];
        }
        else *local = 0;
    }
    else if(local)
        *local = 0;
    return true;
}

bool Maps::SetBlockLocalFeature(uint32_t x, uint32_t y, uint32_t z, int16_t local)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        Process * p = d->owner;
        p->writeWord(addr + d->offsets.local_feature_offset, (uint16_t&)local);
        return true;
    }
    return false;
}

bool Maps::SetBlockGlobalFeature(uint32_t x, uint32_t y, uint32_t z, int16_t global)
{
    MAPS_GUARD
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        Process * p = d->owner;
        p->writeWord(addr + d->offsets.global_feature_offset, (uint16_t&)global);
        return true;
    }
    return false;
}

/*
 * Block events
 */
bool Maps::ReadVeins(uint32_t x, uint32_t y, uint32_t z, vector <t_vein>* veins, vector <t_frozenliquidvein>* ices, vector <t_spattervein> *splatter, vector <t_grassvein> *grass, vector <t_worldconstruction> *constructions)
{
    MAPS_GUARD
    t_vein v;
    t_frozenliquidvein fv;
    t_spattervein sv;
    t_grassvein gv;
    t_worldconstruction wcv;
    Process* p = d->owner;

    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if(veins) veins->clear();
    if(ices) ices->clear();
    if(splatter) splatter->clear();
    if(grass) splatter->clear();
    if(constructions) constructions->clear();

    Private::t_offsets &off = d->offsets;
    if (!addr) return false;
    // veins are stored as a vector of pointers to veins
    /*pointer is 4 bytes! we work with a 32bit program here, no matter what architecture we compile khazad for*/
    DfVector <uint32_t> p_veins (p, addr + off.veinvector);
    uint32_t size = p_veins.size();
    // read all veins
    for (uint32_t i = 0; i < size;i++)
    {
        retry:
        // read the vein pointer from the vector
        uint32_t temp = p_veins[i];
        uint32_t type = p->readDWord(temp);
        if(veins && type == off.vein_mineral_vptr)
        {
            // read the vein data (dereference pointer)
            p->read (temp, sizeof(t_vein), (uint8_t *) &v);
            v.address_of = temp;
            // store it in the vector
            veins->push_back (v);
        }
        else if(ices && type == off.vein_ice_vptr)
        {
            // read the ice vein data (dereference pointer)
            p->read (temp, sizeof(t_frozenliquidvein), (uint8_t *) &fv);
            fv.address_of = temp;
            // store it in the vector
            ices->push_back (fv);
        }
        else if(splatter && type == off.vein_spatter_vptr)
        {
            // read the splatter vein data (dereference pointer)
            p->read (temp, sizeof(t_spattervein), (uint8_t *) &sv);
            sv.address_of = temp;
            // store it in the vector
            splatter->push_back (sv);
        }
        else if(grass && type == off.vein_grass_vptr)
        {
            // read the splatter vein data (dereference pointer)
            p->read (temp, sizeof(t_grassvein), (uint8_t *) &gv);
            gv.address_of = temp;
            // store it in the vector
            grass->push_back (gv);
        }
        else if(constructions && type == off.vein_worldconstruction_vptr)
        {
            // read the splatter vein data (dereference pointer)
            p->read (temp, sizeof(t_worldconstruction), (uint8_t *) &wcv);
            wcv.address_of = temp;
            // store it in the vector
            constructions->push_back (wcv);
        }
        else
        {
            string cname = p->readClassName(type);
            if(!off.vein_ice_vptr && cname == "block_square_event_frozen_liquidst")
            {
                off.vein_ice_vptr = type;
                goto retry;
            }
            else if(!off.vein_mineral_vptr &&cname == "block_square_event_mineralst")
            {
                off.vein_mineral_vptr = type;
                goto retry;
            }
            else if(!off.vein_spatter_vptr && cname == "block_square_event_material_spatterst")
            {
                off.vein_spatter_vptr = type;
                goto retry;
            }
            else if(!off.vein_grass_vptr && cname=="block_square_event_grassst")
            {
                off.vein_grass_vptr = type;
                goto retry;
            }
            else if(!off.vein_worldconstruction_vptr && cname=="block_square_event_world_constructionst")
            {
                off.vein_worldconstruction_vptr = type;
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

/*
__int16 __userpurge GetGeologicalRegion<ax>(__int16 block_X<cx>, int X<ebx>, __int16 block_Y<di>, int block_addr<esi>, int Y)
{
  char bio_off; // al@1
  int tile_designation; // ecx@1
  __int64 corrected_x; // qax@1
  __int64 corrected_y; // qax@1
  int difY; // eax@9
  int difX; // edx@9
  signed __int64 bio_off_2; // qax@9
  signed __int64 bio_off_2_; // qtt@9
  __int16 result; // ax@23

  corrected_x = reg_off_x + (block_X + (signed int)*(_WORD *)(block_addr + 0x90)) / 48;
  *(_WORD *)X = ((BYTE4(corrected_x) & 0xF) + (_DWORD)corrected_x) >> 4;
  corrected_y = reg_off_y + (block_Y + (signed int)*(_WORD *)(block_addr + 0x92)) / 48;
  *(_WORD *)Y = ((BYTE4(corrected_y) & 0xF) + (_DWORD)corrected_y) >> 4;
  tile_designation = *(_DWORD *)(block_addr + 4 * (block_Y + 16 * block_X) + 0x29C);
  bio_off = 0;
  if ( tile_designation & 0x20000 )
    bio_off = 1;
  if ( tile_designation & 0x40000 )
    bio_off |= 2u;
  if ( tile_designation & 0x80000 )
    bio_off |= 4u;
  if ( tile_designation & 0x100000 )
    bio_off |= 8u;
  bio_off_2 = *(_BYTE *)(bio_off + block_addr + 0x1D9C);
  bio_off_2_ = bio_off_2;
  difY = bio_off_2 / 3;
  difX = bio_off_2_ % 3;
  if ( !difX )
    --*(_WORD *)X;
  if ( difX == 2 )
    ++*(_WORD *)X;
  if ( !difY )
    --*(_WORD *)Y;
  if ( difY == 2 )
    ++*(_WORD *)Y;
  if ( *(_WORD *)X < 0 )
    *(_WORD *)X = 0;
  if ( *(_WORD *)Y < 0 )
    *(_WORD *)Y = 0;
  if ( *(_WORD *)X >= (_WORD)World_size )
    *(_WORD *)X = World_size - 1;
  result = HIWORD(World_size);
  if ( *(_WORD *)Y >= HIWORD(World_size) )
  {
    result = HIWORD(World_size) - 1;
    *(_WORD *)Y = HIWORD(World_size) - 1;
  }
  return result;
}
*/

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
    if(d->usesWorldDataPtr)
    {
        uint32_t world = p->readDWord(off.world_data);
        p->readWord (world + off.world_size_x, worldSizeX);
        p->readWord (world + off.world_size_y, worldSizeY);
        regions = p->readDWord ( world + off.world_regions); // ptr2_region_array
        geoblocks_vector_addr = world + off.world_geoblocks_vector;
    }
    else
    {
        p->readWord (off.world_size_x, worldSizeX);
        p->readWord (off.world_size_y, worldSizeY);
        // get pointer to first part of 2d array of regions
        regions = p->readDWord (off.world_regions); // ptr2_region_array
        geoblocks_vector_addr = off.world_geoblocks_vector;
    }

    // read the geoblock vector
    DfVector <uint32_t> geoblocks (d->d->p, geoblocks_vector_addr);

    // iterate over 8 surrounding regions + local region
    for (int i = eNorthWest; i < eBiomeCount; i++)
    {
        // check against worldmap boundaries, fix if needed
        // regionX is in embark squares
        // regionX/16 is in 16x16 embark square regions
        // i provides -1 .. +1 offset from the current region
        int bioRX = d->regionX / 16 + ((i % 3) - 1);
        if (bioRX < 0) bioRX = 0;
        if (bioRX >= worldSizeX) bioRX = worldSizeX - 1;
        int bioRY = d->regionY / 16 + ((i / 3) - 1);
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
        DfVector <uint32_t> geolayers (p, geoblock_off + off.geolayer_geoblock_offset); // let's hope
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

bool Maps::ReadGlobalFeatures( std::vector <t_feature> & features)
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

bool Maps::ReadVegetation(uint32_t x, uint32_t y, uint32_t z, std::vector<t_tree>* plants)
{
    if(!d->hasVeggies || !d->Started)
        return false;
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if(!addr)
        return false;

    t_tree shrubbery;
    plants->clear();

    Private::t_offsets &off = d->offsets;
    DfVector<uint32_t> vegptrs(d->owner, addr + off.vegvector);
    for(size_t i = 0; i < vegptrs.size(); i++)
    {
        d->owner->read (vegptrs[i] + off.tree_desc_offset, sizeof (t_tree), (uint8_t *) &shrubbery);
        shrubbery.address = vegptrs[i];
        plants->push_back(shrubbery);
    }
    if(plants->empty()) return false;
    return true;
}
