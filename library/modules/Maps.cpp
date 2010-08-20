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
#include <shms.h>
#include <mod-core.h>
#include <mod-maps.h>
#include "ContextShared.h"
#include "dfhack/modules/Maps.h"
#include "dfhack/DFError.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFVector.h"

#define SHMMAPSHDR ((Server::Maps::shm_maps_hdr *)d->d->shm_start)
#define SHMCMD(num) ((shm_cmd *)d->d->shm_start)[num]->pingpong
#define SHMHDR ((shm_core_hdr *)d->d->shm_start)
#define SHMDATA(type) ((type *)(d->d->shm_start + SHM_HEADER))
#define MAPS_GUARD if(!d->Started) throw DFHack::Error::ModuleNotInitialized();
using namespace DFHack;

struct Maps::Private
{
    uint32_t * block;
    uint32_t x_block_count, y_block_count, z_block_count;
    uint32_t regionX, regionY, regionZ;
    uint32_t worldSizeX, worldSizeY;

    uint32_t maps_module;
    Server::Maps::maps_offsets offsets;

    DFContextShared *d;
    Process * owner;
    bool Inited;
    bool Started;

    // map between feature address and the read object
    map <uint32_t, t_feature> local_feature_store;

    vector<uint16_t> v_geology[eBiomeCount];
};

Maps::Maps(DFContextShared* _d)
{
    d = new Private;
    d->d = _d;
    Process *p = d->owner = _d->p;
    d->Inited = d->Started = false;

    DFHack::VersionInfo * mem = p->getDescriptor();
    Server::Maps::maps_offsets &off = d->offsets;

    // get the offsets once here
    off.map_offset = mem->getAddress ("map_data");
    off.x_count_offset = mem->getAddress ("x_count_block");
    off.y_count_offset = mem->getAddress ("y_count_block");
    off.z_count_offset = mem->getAddress ("z_count_block");
    off.tile_type_offset = mem->getOffset ("map_data_type");
    off.designation_offset = mem->getOffset ("map_data_designation");
    off.occupancy_offset = mem->getOffset("map_data_occupancy");
    off.biome_stuffs = mem->getOffset ("map_data_biome_stuffs");
    off.veinvector = mem->getOffset ("map_data_vein_vector");
    off.local_feature_offset = mem->getOffset ("map_data_feature_local");
    off.global_feature_offset = mem->getOffset ("map_data_feature_global");

    off.temperature1_offset = mem->getOffset ("map_data_temperature1_offset");
    off.temperature2_offset = mem->getOffset ("map_data_temperature2_offset");
    off.region_x_offset = mem->getAddress ("region_x");
    off.region_y_offset = mem->getAddress ("region_y");
    off.region_z_offset =  mem->getAddress ("region_z");

    off.world_regions =  mem->getAddress ("ptr2_region_array");
    off.region_size =  mem->getHexValue ("region_size");
    off.region_geo_index_offset =  mem->getOffset ("region_geo_index_off");
    off.geolayer_geoblock_offset = mem->getOffset ("geolayer_geoblock_offset");
    off.world_geoblocks_vector =  mem->getAddress ("geoblock_vector");
    off.type_inside_geolayer = mem->getOffset ("type_inside_geolayer");

    off.world_size_x = mem->getAddress ("world_size_x");
    off.world_size_y = mem->getAddress ("world_size_y");

    // these can fail and will be found when looking at the actual veins later
    // basically a cache
    off.vein_ice_vptr = 0;
    mem->resolveClassnameToVPtr("block_square_event_frozen_liquid", off.vein_ice_vptr);
    off.vein_mineral_vptr = 0;
    mem->resolveClassnameToVPtr("block_square_event_mineral",off.vein_mineral_vptr);

    // upload offsets to SHM server if possible
    d->maps_module = 0;
    if(p->getModuleIndex("Maps2010",1,d->maps_module))
    {
        // supply the module with offsets so it can work with them
        Server::Maps::maps_offsets *off2 = SHMDATA(Server::Maps::maps_offsets);
        memcpy(off2, &(d->offsets), sizeof(Server::Maps::maps_offsets));
        full_barrier
        const uint32_t cmd = Server::Maps::MAP_INIT + (d->maps_module << 16);
        p->SetAndWait(cmd);
    }
    d->Inited = true;
}

Maps::~Maps()
{
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
    Server::Maps::maps_offsets &off = d->offsets;

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

// invalidates local and global features!
bool Maps::Finish()
{
    d->local_feature_store.clear();
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
    if(d->d->shm_start && d->maps_module) // ACCELERATE!
    {
        SHMMAPSHDR->x = x;
        SHMMAPSHDR->y = y;
        SHMMAPSHDR->z = z;
        volatile uint32_t cmd = Server::Maps::MAP_READ_BLOCK_BY_COORDS + (d->maps_module << 16);
        if(!p->SetAndWait(cmd))
            return false;
        memcpy(buffer,SHMDATA(mapblock40d),sizeof(mapblock40d));
        return true;
    }
    else // plain old block read
    {
        uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
        if (addr)
        {
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
        d->owner->write (addr + d->offsets.occupancy_offset, sizeof (tiletypes40d), (uint8_t *) buffer);
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

bool Maps::WriteLocalFeature(uint32_t x, uint32_t y, uint32_t z, int16_t local)
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

bool Maps::WriteGlobalFeature(uint32_t x, uint32_t y, uint32_t z, int16_t global)
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
bool Maps::ReadVeins(uint32_t x, uint32_t y, uint32_t z, vector <t_vein>* veins, vector <t_frozenliquidvein>* ices, vector <t_spattervein> *splatter)
{
    MAPS_GUARD
    t_vein v;
    t_frozenliquidvein fv;
    t_spattervein sv;
    Process* p = d->owner;

    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if(veins) veins->clear();
    if(ices) ices->clear();
    if(splatter) splatter->clear();

    Server::Maps::maps_offsets &off = d->offsets;
    if (addr)
    {
        // veins are stored as a vector of pointers to veins
        /*pointer is 4 bytes! we work with a 32bit program here, no matter what architecture we compile khazad for*/
        DfVector <uint32_t> p_veins (p, addr + off.veinvector);
        uint32_t size = p_veins.size();
        // read all veins
        for (uint32_t i = 0; i < size;i++)
        {
            // read the vein pointer from the vector
            uint32_t temp = p_veins[i];
            uint32_t type = p->readDWord(temp);
try_again:
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
            else
            {
                string cname = p->readClassName(type);
                if(ices && cname == "block_square_event_frozen_liquidst")
                {
                    off.vein_ice_vptr = type;
                    goto try_again;
                }
                else if(veins && cname == "block_square_event_mineralst")
                {
                    off.vein_mineral_vptr = type;
                    goto try_again;
                }
                else if(splatter && cname == "block_square_event_material_spatterst")
                {
                    off.vein_spatter_vptr = type;
                    goto try_again;
                }
                #ifdef DEBUG
                else
                {
                    cerr << "unknown vein " << cname << endl;
                }
                #endif
                // or it was something we don't care about
            }
        }
        return true;
    }
    return false;
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
    VersionInfo * minfo = d->d->offset_descriptor;
    Process *p = d->owner;
    // get needed addresses and offsets. Now this is what I call crazy.
    uint16_t worldSizeX, worldSizeY;
    Server::Maps::maps_offsets &off = d->offsets;
    // get world size
    p->readWord (off.world_size_x, worldSizeX);
    p->readWord (off.world_size_y, worldSizeY);

    // get pointer to first part of 2d array of regions
    uint32_t regions = p->readDWord (off.world_regions);

    // read the geoblock vector
    DfVector <uint32_t> geoblocks (d->d->p, off.world_geoblocks_vector);

    // iterate over 8 surrounding regions + local region
    for (int i = eNorthWest; i < eBiomeCount; i++)
    {
        // check against worldmap boundaries, fix if needed
        int bioRX = d->regionX / 16 + (i % 3) - 1;
        if (bioRX < 0) bioRX = 0;
        if (bioRX >= worldSizeX) bioRX = worldSizeX - 1;
        int bioRY = d->regionY / 16 + (i / 3) - 1;
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

bool Maps::ReadLocalFeatures( std::map <planecoord, std::vector<t_feature *> > & local_features )
{
    MAPS_GUARD
    // can't be used without a map!
    if(!d->block)
        return false;

    Process * p = d->owner;
    VersionInfo * mem = p->getDescriptor();
    // deref pointer to the humongo-structure
    uint32_t base = p->readDWord(mem->getAddress("local_feature_start_ptr"));
    if(!base)
        return false;
    uint32_t sizeof_vec = mem->getHexValue("sizeof_vector");
    const uint32_t sizeof_elem = 16;
    const uint32_t offset_elem = 4;
    const uint32_t main_mat_offset = mem->getOffset("local_feature_mat"); // 0x30
    const uint32_t sub_mat_offset = mem->getOffset("local_feature_submat"); // 0x34

    local_features.clear();

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
        uint32_t wtf = p->readDWord(array_elem + ( sizeof_elem * ( (uint32_t)region_y_local/16)) + offset_elem);
        if(wtf)
        {
            // wtf + sizeof(vector<ptr>) * crap;
            uint32_t feat_vector = wtf + sizeof_vec * (16 * (region_x_local % 16) + (region_y_local % 16));
            DfVector<uint32_t> p_features(p, feat_vector);
            uint32_t size = p_features.size();
            planecoord pc;
            pc.dim.x = blockX;
            pc.dim.y = blockY;
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
                        tftemp.main_material = p->readWord( cur_ptr + main_mat_offset );
                        tftemp.sub_material = p->readDWord( cur_ptr + sub_mat_offset );
                        tftemp.type = feature_Adamantine_Tube;
                    }
                    else if(name == "feature_init_deep_surface_portalst")
                    {
                        tftemp.main_material = p->readWord( cur_ptr + main_mat_offset );
                        tftemp.sub_material = p->readDWord( cur_ptr + sub_mat_offset );
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
            local_features[pc] = tempvec;
        }
    }
    return true;
}

bool Maps::ReadGlobalFeatures( std::vector <t_feature> & features)
{
    MAPS_GUARD
    // can't be used without a map!
    if(!d->block)
        return false;

    Process * p = d->owner;
    VersionInfo * mem = p->getDescriptor();

    uint32_t global_feature_vector = mem->getAddress("global_feature_vector");
    uint32_t global_feature_funcptr = mem->getOffset("global_feature_funcptr_");
    const uint32_t main_mat_offset = mem->getOffset("global_feature_mat"); // 0x34
    const uint32_t sub_mat_offset = mem->getOffset("global_feature_submat"); // 0x38
    DfVector<uint32_t> p_features (p,global_feature_vector);

    features.clear();
    uint32_t size = p_features.size();
    features.reserve(size);
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
            temp.main_material = p->readWord( feat_ptr + main_mat_offset );
            temp.sub_material = p->readDWord( feat_ptr + sub_mat_offset );
            temp.type = feature_Underworld;
        }
        else
        {
            temp.main_material = -1;
            temp.sub_material = -1;
            temp.type = feature_Other;
        }
        features.push_back(temp);
    }
    return true;
}

