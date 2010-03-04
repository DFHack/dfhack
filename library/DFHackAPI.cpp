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

#include "DFCommonInternal.h"
#include "../shmserver/shms.h"
#include "../shmserver/mod-core.h"
#include "../shmserver/mod-maps.h"
using namespace DFHack;

/*
FIXME: memset to 0?
*/
class API::Private
{
public:
    Private()
            : block (NULL)
            , pm (NULL), p (NULL), offset_descriptor (NULL)
            , p_cons (NULL), p_bld (NULL), p_veg (NULL)
    {}
    uint32_t * block;
    uint32_t x_block_count, y_block_count, z_block_count;
    uint32_t regionX, regionY, regionZ;
    uint32_t worldSizeX, worldSizeY;

    uint32_t tile_type_offset;
    uint32_t designation_offset;
    uint32_t occupancy_offset;
    uint32_t block_flags_offset;
    uint32_t biome_stuffs;
    uint32_t veinvector;
    uint32_t veinsize;
    uint32_t vein_mineral_vptr;
    uint32_t vein_ice_vptr;

    uint32_t window_x_offset;
    uint32_t window_y_offset;
    uint32_t window_z_offset;
    uint32_t cursor_xyz_offset;
    uint32_t window_dims_offset;
    uint32_t current_cursor_creature_offset;
    uint32_t pause_state_offset;
    uint32_t view_screen_offset;
    uint32_t current_menu_state_offset;

    uint32_t creature_pos_offset;
    uint32_t creature_type_offset;
    uint32_t creature_flags1_offset;
    uint32_t creature_flags2_offset;
    uint32_t creature_first_name_offset;
    uint32_t creature_nick_name_offset;
    uint32_t creature_last_name_offset;

    uint32_t creature_custom_profession_offset;
    uint32_t creature_profession_offset;
    uint32_t creature_sex_offset;
    uint32_t creature_id_offset;
    uint32_t creature_squad_name_offset;
    uint32_t creature_squad_leader_id_offset;
    uint32_t creature_money_offset;
    uint32_t creature_current_job_offset;
    uint32_t creature_current_job_id_offset;
    uint32_t creature_strength_offset;
    uint32_t creature_agility_offset;
    uint32_t creature_toughness_offset;
    uint32_t creature_skills_offset;
    uint32_t creature_labors_offset;
    uint32_t creature_happiness_offset;
    uint32_t creature_traits_offset;
    uint32_t creature_likes_offset;

    uint32_t item_material_offset;

    uint32_t note_foreground_offset;
    uint32_t note_background_offset;
    uint32_t note_name_offset;
    uint32_t note_xyz_offset;
    uint32_t hotkey_start;
    uint32_t hotkey_mode_offset;
    uint32_t hotkey_xyz_offset;
    uint32_t hotkey_size;

    uint32_t settlement_name_offset;
    uint32_t settlement_world_xy_offset;
    uint32_t settlement_local_xy_offset;
    
    uint32_t dwarf_lang_table_offset;

    ProcessEnumerator* pm;
    Process* p;
    char * shm_start;
    memory_info* offset_descriptor;
    vector<uint16_t> v_geology[eBiomeCount];
    string xml;

    bool constructionsInited;
    bool buildingsInited;
    bool vegetationInited;
    bool creaturesInited;
    bool cursorWindowInited;
    bool viewSizeInited;
    bool itemsInited;
    bool notesInited;
    bool hotkeyInited;
    bool settlementsInited;
    bool nameTablesInited;
    
    uint32_t maps_module;

    uint32_t tree_offset;
    DfVector *p_cre;
    DfVector *p_cons;
    DfVector *p_bld;
    DfVector *p_veg;
    DfVector *p_itm;
    DfVector *p_notes;
    DfVector *p_settlements;
    DfVector *p_current_settlement;
};

API::API (const string path_to_xml)
        : d (new Private())
{
    d->xml = QUOT (MEMXML_DATA_PATH);
    d->xml += "/";
    d->xml += path_to_xml;
    d->constructionsInited = false;
    d->creaturesInited = false;
    d->buildingsInited = false;
    d->vegetationInited = false;
    d->cursorWindowInited = false;
    d->viewSizeInited = false;
    d->itemsInited = false;
    d->notesInited = false;
    d->hotkeyInited = false;
    d->pm = NULL;
    d->shm_start = 0;
    d->maps_module = 0;
}

API::~API()
{
    delete d;
}

#define SHMCMD ((shm_cmd *)d->shm_start)->pingpong
#define SHMHDR ((shm_core_hdr *)d->shm_start)
#define SHMMAPSHDR ((Maps::shm_maps_hdr *)d->shm_start)
#define SHMDATA(type) ((type *)(d->shm_start + SHM_HEADER))

/*-----------------------------------*
 *  Init the mapblock pointer array   *
 *-----------------------------------*/
bool API::InitMap()
{
    uint32_t map_offset = d->offset_descriptor->getAddress ("map_data");
    uint32_t x_count_offset = d->offset_descriptor->getAddress ("x_count");
    uint32_t y_count_offset = d->offset_descriptor->getAddress ("y_count");
    uint32_t z_count_offset = d->offset_descriptor->getAddress ("z_count");

    // get the offsets once here
    d->tile_type_offset = d->offset_descriptor->getOffset ("type");
    d->designation_offset = d->offset_descriptor->getOffset ("designation");
    d->occupancy_offset = d->offset_descriptor->getOffset ("occupancy");
    d->biome_stuffs = d->offset_descriptor->getOffset ("biome_stuffs");

    d->veinvector = d->offset_descriptor->getOffset ("v_vein");
    d->veinsize = d->offset_descriptor->getHexValue ("v_vein_size");
    
    // these can fail and will be found when looking at the actual veins later
    // basically a cache
    d->vein_ice_vptr = 0;
    d->offset_descriptor->resolveClassnameToVPtr("block_square_event_frozen_liquid", d->vein_ice_vptr);
    d->vein_mineral_vptr = 0;
    d->offset_descriptor->resolveClassnameToVPtr("block_square_event_mineral",d->vein_mineral_vptr);
    
    /*
     * --> SHM initialization (if possible) <--
     */
    g_pProcess->getModuleIndex("Maps",1,d->maps_module);
    
    if(d->maps_module)
    {
        // supply the module with offsets so it can work with them
        Maps::maps_offsets *off = SHMDATA(Maps::maps_offsets);
        off->biome_stuffs = d->biome_stuffs;
        off->designation_offset = d->designation_offset;
        off->map_offset = map_offset;
        off->occupancy_offset = d->occupancy_offset;
        off->tile_type_offset = d->tile_type_offset;
        off->vein_ice_vptr = d->vein_ice_vptr; // FIXME: not necessarily true, the shm server side needs a class lookup >_<
        off->vein_mineral_vptr = d->vein_mineral_vptr; // FIXME: not necessarily true, the shm server side needs a class lookup >_<
        off->veinvector = d->veinvector;
        off->x_count_offset = x_count_offset;
        off->y_count_offset = y_count_offset;
        off->z_count_offset = z_count_offset;
        full_barrier
        const uint32_t cmd = Maps::MAP_INIT + d->maps_module << 16;
        SHMCMD = cmd;
        g_pProcess->waitWhile(cmd);
        //cerr << "Map acceleration enabled!" << endl;
    }
    
    // get the map pointer
    uint32_t x_array_loc = g_pProcess->readDWord (map_offset);
    if (!x_array_loc)
    {
        throw Error::NoMapLoaded();
    }
    
    // get the size
    uint32_t mx, my, mz;
    mx = d->x_block_count = g_pProcess->readDWord (x_count_offset);
    my = d->y_block_count = g_pProcess->readDWord (y_count_offset);
    mz = d->z_block_count = g_pProcess->readDWord (z_count_offset);

    // test for wrong map dimensions
    if (mx == 0 || mx > 48 || my == 0 || my > 48 || mz == 0)
    {
        throw Error::BadMapDimensions(mx, my);
        //return false;
    }

    // alloc array for pointers to all blocks
    d->block = new uint32_t[mx*my*mz];
    uint32_t *temp_x = new uint32_t[mx];
    uint32_t *temp_y = new uint32_t[my];
    uint32_t *temp_z = new uint32_t[mz];

    g_pProcess->read (x_array_loc, mx * sizeof (uint32_t), (uint8_t *) temp_x);
    for (uint32_t x = 0; x < mx; x++)
    {
        g_pProcess->read (temp_x[x], my * sizeof (uint32_t), (uint8_t *) temp_y);
        // y -> map column
        for (uint32_t y = 0; y < my; y++)
        {
            g_pProcess->read (temp_y[y],
                   mz * sizeof (uint32_t),
                   (uint8_t *) (d->block + x*my*mz + y*mz));
        }
    }
    delete [] temp_x;
    delete [] temp_y;
    delete [] temp_z;
    return true;
}

bool API::DestroyMap()
{
    if (d->block != NULL)
    {
        delete [] d->block;
        d->block = NULL;
    }
    return true;
}

bool API::isValidBlock (uint32_t x, uint32_t y, uint32_t z)
{
    if (x < 0 || x >= d->x_block_count || y < 0 || y >= d->y_block_count || z < 0 || z >= d->z_block_count)
        return false;
    return d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z] != 0;
}

uint32_t API::getBlockPtr (uint32_t x, uint32_t y, uint32_t z)
{
    if (x < 0 || x >= d->x_block_count || y < 0 || y >= d->y_block_count || z < 0 || z >= d->z_block_count)
        return 0;
    return d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
}

bool API::ReadBlock40d(uint32_t x, uint32_t y, uint32_t z, mapblock40d * buffer)
{
    if(d->shm_start && d->maps_module) // ACCELERATE!
    {
        SHMMAPSHDR->x = x;
        SHMMAPSHDR->y = y;
        SHMMAPSHDR->z = z;
        const uint32_t cmd = Maps::MAP_READ_BLOCK_BY_COORDS + (d->maps_module << 16);
        full_barrier
        SHMCMD = cmd;
        if(!g_pProcess->waitWhile(cmd))
        {
            return false;
        }
        memcpy(buffer,SHMDATA(mapblock40d),sizeof(mapblock40d));
        return true;
    }
    else // plain old block read
    {
        uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
        if (addr)
        {
            g_pProcess->read (addr + d->tile_type_offset, sizeof (buffer->tiletypes), (uint8_t *) buffer->tiletypes);
            g_pProcess->read (addr + d->occupancy_offset, sizeof (buffer->occupancy), (uint8_t *) buffer->occupancy);
            g_pProcess->read (addr + d->designation_offset, sizeof (buffer->designaton), (uint8_t *) buffer->designaton);
            g_pProcess->read (addr + d->biome_stuffs, sizeof (buffer->biome_indices), (uint8_t *) buffer->biome_indices);
            uint32_t addr_of_struct = g_pProcess->readDWord(addr);
            buffer->dirty_dword = g_pProcess->readDWord(addr_of_struct);
            return true;
        }
        return false;
    }
}


// 256 * sizeof(uint16_t)
bool API::ReadTileTypes (uint32_t x, uint32_t y, uint32_t z, uint16_t *buffer)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        g_pProcess->read (addr + d->tile_type_offset, 256 * sizeof (uint16_t), (uint8_t *) buffer);
        return true;
    }
    return false;
}

bool API::ReadDirtyBit(uint32_t x, uint32_t y, uint32_t z, bool &dirtybit)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if(addr)
    {
        uint32_t addr_of_struct = g_pProcess->readDWord(addr);
        dirtybit = g_pProcess->readDWord(addr_of_struct) & 1;
        return true;
    }
    return false;
}

bool API::WriteDirtyBit(uint32_t x, uint32_t y, uint32_t z, bool dirtybit)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        uint32_t addr_of_struct = g_pProcess->readDWord(addr);
        uint32_t dirtydword = g_pProcess->readDWord(addr_of_struct);
        dirtydword &= 0xFFFFFFFE;
        dirtydword |= (uint32_t) dirtybit;
        g_pProcess->writeDWord (addr_of_struct, dirtydword);
        return true;
    }
    return false;
}


// 256 * sizeof(uint32_t)
bool API::ReadDesignations (uint32_t x, uint32_t y, uint32_t z, uint32_t *buffer)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        g_pProcess->read (addr + d->designation_offset, 256 * sizeof (uint32_t), (uint8_t *) buffer);
        return true;
    }
    return false;
}


// 256 * sizeof(uint32_t)
bool API::ReadOccupancy (uint32_t x, uint32_t y, uint32_t z, uint32_t *buffer)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        g_pProcess->read (addr + d->occupancy_offset, 256 * sizeof (uint32_t), (uint8_t *) buffer);
        return true;
    }
    return false;
}


// 256 * sizeof(uint16_t)
bool API::WriteTileTypes (uint32_t x, uint32_t y, uint32_t z, uint16_t *buffer)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        g_pProcess->write (addr + d->tile_type_offset, 256 * sizeof (uint16_t), (uint8_t *) buffer);
        return true;
    }
    return false;
}

bool API::getCurrentCursorCreatures (vector<uint32_t> &addresses)
{
    if(d->cursorWindowInited) return false;
    DfVector creUnderCursor = d->p->readVector (d->current_cursor_creature_offset, 4);
    if (creUnderCursor.getSize() == 0)
    {
        return false;
    }
    addresses.clear();
    for (uint32_t i = 0;i < creUnderCursor.getSize();i++)
    {
        uint32_t temp = * (uint32_t *) creUnderCursor.at (i);
        addresses.push_back (temp);
    }
    return true;
}

// 256 * sizeof(uint32_t)
bool API::WriteDesignations (uint32_t x, uint32_t y, uint32_t z, uint32_t *buffer)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        g_pProcess->write (addr + d->designation_offset, 256 * sizeof (uint32_t), (uint8_t *) buffer);
        return true;
    }
    return false;
}

// 256 * sizeof(uint32_t)
bool API::WriteOccupancy (uint32_t x, uint32_t y, uint32_t z, uint32_t *buffer)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        g_pProcess->write (addr + d->occupancy_offset, 256 * sizeof (uint32_t), (uint8_t *) buffer);
        return true;
    }
    return false;
}

// FIXME: this is bad. determine the real size!
//16 of them? IDK... there's probably just 7. Reading more doesn't cause errors as it's an array nested inside a block
// 16 * sizeof(uint8_t)
bool API::ReadRegionOffsets (uint32_t x, uint32_t y, uint32_t z, uint8_t *buffer)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    if (addr)
    {
        g_pProcess->read (addr + d->biome_stuffs, 16 * sizeof (uint8_t), buffer);
        return true;
    }
    return false;
}

// veins of a block, expects empty vein vectors
bool API::ReadVeins(uint32_t x, uint32_t y, uint32_t z, vector <t_vein> & veins, vector <t_frozenliquidvein>& ices)
{
    uint32_t addr = d->block[x*d->y_block_count*d->z_block_count + y*d->z_block_count + z];
    veins.clear();
    ices.clear();
    if (addr && d->veinvector && d->veinsize)
    {
        // veins are stored as a vector of pointers to veins
        /*pointer is 4 bytes! we work with a 32bit program here, no matter what architecture we compile khazad for*/
        DfVector p_veins = d->p->readVector (addr + d->veinvector, 4);
        uint32_t size = p_veins.getSize();
        veins.reserve (size);

        // read all veins
        for (uint32_t i = 0; i < size;i++)
        {
            t_vein v;
            t_frozenliquidvein fv;

            // read the vein pointer from the vector
            uint32_t temp = * (uint32_t *) p_veins[i];
            uint32_t type = g_pProcess->readDWord(temp);
try_again:
            if(type == d->vein_mineral_vptr)
            {
                // read the vein data (dereference pointer)
                g_pProcess->read (temp, sizeof(t_vein), (uint8_t *) &v);
                v.address_of = temp;
                // store it in the vector
                veins.push_back (v);
            }
            else if(type == d->vein_ice_vptr)
            {
                // read the ice vein data (dereference pointer)
                g_pProcess->read (temp, sizeof(t_frozenliquidvein), (uint8_t *) &fv);
                // store it in the vector
                ices.push_back (fv);
            }
            //#define ___FIND_THEM
            #ifdef ___FIND_THEM
            else if(g_pProcess->readClassName(type) == "block_square_event_frozen_liquid")
            {
                d->vein_ice_vptr = type;
                cout << "block_square_event_frozen_liquid : 0x" << hex << type << endl;
                goto try_again;
            }
            else if(g_pProcess->readClassName(type) == "block_square_event_mineral")
            {
                d->vein_mineral_vptr = type;
                cout << "block_square_event_mineral : 0x" << hex << type << endl;
                goto try_again;
            }
            #endif
        }
        return true;
    }
    return false;
}


// getter for map size
void API::getSize (uint32_t& x, uint32_t& y, uint32_t& z)
{
    x = d->x_block_count;
    y = d->y_block_count;
    z = d->z_block_count;
}

bool API::ReadWoodMatgloss (vector<t_matgloss> & woods)
{

    int matgloss_address = d->offset_descriptor->getAddress ("matgloss");
    int matgloss_wood_name_offset = d->offset_descriptor->getOffset("matgloss_wood_name");
    // TODO: find flag for autumnal coloring?
    DfVector p_matgloss = d->p->readVector (matgloss_address, 4);

    woods.clear();

    t_matgloss mat;
    // TODO: use brown?
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    uint32_t size = p_matgloss.getSize();
    for (uint32_t i = 0; i < size ;i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        /*
        fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        */
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_wood_name_offset, mat.name, 128);
        woods.push_back (mat);
    }
    return true;
}

bool API::ReadStoneMatgloss (vector<t_matgloss> & stones)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("matgloss");
    int matgloss_offset = minfo->getHexValue ("matgloss_skip");
    int matgloss_colors = minfo->getOffset ("matgloss_stone_color");
    int matgloss_stone_name_offset = minfo->getOffset("matgloss_stone_name");

    DfVector p_matgloss = d->p->readVector (matgloss_address + matgloss_offset, 4);

    uint32_t size = p_matgloss.getSize();
    stones.resize (0);
    stones.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        t_matgloss mat;
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_stone_name_offset, mat.name, 128);
        mat.fore = (uint8_t) g_pProcess->readWord (temp + matgloss_colors);
        mat.back = (uint8_t) g_pProcess->readWord (temp + matgloss_colors + 2);
        mat.bright = (uint8_t) g_pProcess->readWord (temp + matgloss_colors + 4);
        stones.push_back (mat);
    }
    return true;
}


bool API::ReadMetalMatgloss (vector<t_matgloss> & metals)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("matgloss");
    int matgloss_offset = minfo->getHexValue ("matgloss_skip");
    int matgloss_colors = minfo->getOffset ("matgloss_metal_color");
    int matgloss_metal_name_offset = minfo->getOffset("matgloss_metal_name");
    DfVector p_matgloss = d->p->readVector (matgloss_address + matgloss_offset * 3, 4);

    metals.clear();

    for (uint32_t i = 0; i < p_matgloss.getSize();i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        t_matgloss mat;
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_metal_name_offset, mat.name, 128);
        mat.fore = (uint8_t) g_pProcess->readWord (temp + matgloss_colors);
        mat.back = (uint8_t) g_pProcess->readWord (temp + matgloss_colors + 2);
        mat.bright = (uint8_t) g_pProcess->readWord (temp + matgloss_colors + 4);
        metals.push_back (mat);
    }
    return true;
}

bool API::ReadPlantMatgloss (vector<t_matgloss> & plants)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("matgloss");
    int matgloss_offset = minfo->getHexValue ("matgloss_skip");
    int matgloss_plant_name_offset = minfo->getOffset("matgloss_plant_name");
    DfVector p_matgloss = d->p->readVector (matgloss_address + matgloss_offset * 2, 4);

    plants.clear();

    // TODO: use green?
    t_matgloss mat;
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    for (uint32_t i = 0; i < p_matgloss.getSize();i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_plant_name_offset, mat.name, 128);
        plants.push_back (mat);
    }
    return true;
}

bool API::ReadPlantMatgloss (vector<t_matglossPlant> & plants)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("matgloss");
    int matgloss_offset = minfo->getHexValue ("matgloss_skip");
    int matgloss_plant_name_offset = minfo->getOffset("matgloss_plant_name");
    int matgloss_plant_drink_offset = minfo->getOffset("matgloss_plant_drink");
    int matgloss_plant_food_offset = minfo->getOffset("matgloss_plant_food");
    int matgloss_plant_extract_offset = minfo->getOffset("matgloss_plant_extract");
    DfVector p_matgloss = d->p->readVector (matgloss_address + matgloss_offset * 2, 4);

    plants.clear();

    // TODO: use green?
    t_matglossPlant mat;
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    for (uint32_t i = 0; i < p_matgloss.getSize();i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_plant_name_offset, mat.name, 128);
        d->p->readSTLString (temp+matgloss_plant_drink_offset, mat.drink_name, 128);
        d->p->readSTLString (temp+matgloss_plant_food_offset, mat.food_name, 128);
        d->p->readSTLString (temp+matgloss_plant_extract_offset, mat.extract_name, 128);
        
        //d->p->readSTLString (temp
        plants.push_back (mat);
    }
    return true;
}

bool API::ReadCreatureMatgloss (vector<t_matgloss> & creatures)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("matgloss");
    int matgloss_offset = minfo->getHexValue ("matgloss_skip");
    int matgloss_creature_name_offset = minfo->getOffset("matgloss_creature_name");
    DfVector p_matgloss = d->p->readVector (matgloss_address + matgloss_offset * 6, 4);

    creatures.clear();

    // TODO: use green?
    t_matgloss mat;
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    for (uint32_t i = 0; i < p_matgloss.getSize();i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_creature_name_offset, mat.name, 128);
        creatures.push_back (mat);
    }
    return true;
}


//vector<uint16_t> v_geology[eBiomeCount];
bool API::ReadGeology (vector < vector <uint16_t> >& assign)
{
    memory_info * minfo = d->offset_descriptor;
    // get needed addresses and offsets. Now this is what I call crazy.
    int region_x_offset = minfo->getAddress ("region_x");
    int region_y_offset = minfo->getAddress ("region_y");
    int region_z_offset =  minfo->getAddress ("region_z");
    int world_offset =  minfo->getAddress ("world");
    int world_regions_offset =  minfo->getOffset ("w_regions_arr");
    int region_size =  minfo->getHexValue ("region_size");
    int region_geo_index_offset =  minfo->getOffset ("region_geo_index_off");
    int world_geoblocks_offset =  minfo->getOffset ("w_geoblocks");
    int world_size_x = minfo->getOffset ("world_size_x");
    int world_size_y = minfo->getOffset ("world_size_y");
    int geolayer_geoblock_offset = minfo->getOffset ("geolayer_geoblock_offset");

    uint32_t regionX, regionY, regionZ;
    uint16_t worldSizeX, worldSizeY;

    // read position of the region inside DF world
    g_pProcess->readDWord (region_x_offset, regionX);
    g_pProcess->readDWord (region_y_offset, regionY);
    g_pProcess->readDWord (region_z_offset, regionZ);

    // get world size
    g_pProcess->readWord (world_offset + world_size_x, worldSizeX);
    g_pProcess->readWord (world_offset + world_size_y, worldSizeY);

    // get pointer to first part of 2d array of regions
    uint32_t regions = g_pProcess->readDWord (world_offset + world_regions_offset);

    // read the geoblock vector
    DfVector geoblocks = d->p->readVector (world_offset + world_geoblocks_offset, 4);

    // iterate over 8 surrounding regions + local region
    for (int i = eNorthWest; i < eBiomeCount; i++)
    {
        // check bounds, fix them if needed
        int bioRX = regionX / 16 + (i % 3) - 1;
        if (bioRX < 0) bioRX = 0;
        if (bioRX >= worldSizeX) bioRX = worldSizeX - 1;
        int bioRY = regionY / 16 + (i / 3) - 1;
        if (bioRY < 0) bioRY = 0;
        if (bioRY >= worldSizeY) bioRY = worldSizeY - 1;

        // get pointer to column of regions
        uint32_t geoX;
        g_pProcess->readDWord (regions + bioRX*4, geoX);

        // get index into geoblock vector
        uint16_t geoindex;
        g_pProcess->readWord (geoX + bioRY*region_size + region_geo_index_offset, geoindex);

        // get the geoblock from the geoblock vector using the geoindex
        // read the matgloss pointer from the vector into temp
        uint32_t geoblock_off = * (uint32_t *) geoblocks[geoindex];

        // get the vector with pointer to layers
        DfVector geolayers = d->p->readVector (geoblock_off + geolayer_geoblock_offset , 4); // let's hope
        // make sure we don't load crap
        assert (geolayers.getSize() > 0 && geolayers.getSize() <= 16);

        d->v_geology[i].reserve (geolayers.getSize());
        // finally, read the layer matgloss
        for (uint32_t j = 0;j < geolayers.getSize();j++)
        {
            // read pointer to a layer
            uint32_t geol_offset = * (uint32_t *) geolayers[j];
            // read word at pointer + 2, store in our geology vectors
            d->v_geology[i].push_back (g_pProcess->readWord (geol_offset + 2));
        }
    }
    assign.clear();
    assign.reserve (eBiomeCount);
//     // TODO: clean this up
    for (int i = 0; i < eBiomeCount;i++)
    {
        assign.push_back (d->v_geology[i]);
    }
    return true;
}


// returns number of buildings, expects v_buildingtypes that will later map t_building.type to its name
bool API::InitReadBuildings ( uint32_t& numbuildings )
{
    int buildings = d->offset_descriptor->getAddress ("buildings");
    d->buildingsInited = true;
    d->p_bld = new DfVector (d->p->readVector (buildings, 4));
    numbuildings = d->p_bld->getSize();
    return true;
}


// read one building
bool API::ReadBuilding (const int32_t index, t_building & building)
{
    if(!d->buildingsInited) return false;

    t_building_df40d bld_40d;

    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_bld->at (index);
    //d->p_bld->read(index,(uint8_t *)&temp);

    //read building from memory
    g_pProcess->read (temp, sizeof (t_building_df40d), (uint8_t *) &bld_40d);

    // transform
    int32_t type = -1;
    d->offset_descriptor->resolveObjectToClassID (temp, type);
    building.origin = temp;
    building.vtable = bld_40d.vtable;
    building.x1 = bld_40d.x1;
    building.x2 = bld_40d.x2;
    building.y1 = bld_40d.y1;
    building.y2 = bld_40d.y2;
    building.z = bld_40d.z;
    building.material = bld_40d.material;
    building.type = type;

    return true;
}


void API::FinishReadBuildings()
{
    if(d->p_bld)
    {
        delete d->p_bld;
        d->p_bld = NULL;
    }
    d->buildingsInited = false;
}


//TODO: maybe do construction reading differently - this could go slow with many of them.
// returns number of constructions, prepares a vector, returns total number of constructions
bool API::InitReadConstructions(uint32_t & numconstructions)
{
    int constructions = d->offset_descriptor->getAddress ("constructions");
    d->p_cons = new DfVector (d->p->readVector (constructions, 4));
    d->constructionsInited = true;
    numconstructions = d->p_cons->getSize();
    return true;
}


bool API::ReadConstruction (const int32_t index, t_construction & construction)
{
    if(!d->constructionsInited) return false;
    t_construction_df40d c_40d;

    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_cons->at (index);

    //read construction from memory
    g_pProcess->read (temp, sizeof (t_construction_df40d), (uint8_t *) &c_40d);

    // transform
    construction.x = c_40d.x;
    construction.y = c_40d.y;
    construction.z = c_40d.z;
    construction.material = c_40d.material;

    return true;
}


void API::FinishReadConstructions()
{
    if(d->p_cons)
    {
        delete d->p_cons;
        d->p_cons = NULL;
    }
    d->constructionsInited = false;
}


bool API::InitReadVegetation(uint32_t & numplants)
{
    try 
    {
        int vegetation = d->offset_descriptor->getAddress ("vegetation");
        d->tree_offset = d->offset_descriptor->getOffset ("tree_desc_offset");

        d->vegetationInited = true;
        d->p_veg = new DfVector (d->p->readVector (vegetation, 4));
        numplants = d->p_veg->getSize();
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->vegetationInited = false;
        numplants = 0;
        throw;
    }
}


bool API::ReadVegetation (const int32_t index, t_tree_desc & shrubbery)
{
    if(!d->vegetationInited)
        return false;
    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_veg->at (index);
    //read construction from memory
    g_pProcess->read (temp + d->tree_offset, sizeof (t_tree_desc), (uint8_t *) &shrubbery);
    // FIXME: this is completely wrong. type isn't just tree/shrub but also different kinds of trees. stuff that grows around ponds has its own type ID
    if (shrubbery.material.type == 3) shrubbery.material.type = 2;
    return true;
}


void API::FinishReadVegetation()
{
    if(d->p_veg)
    {
        delete d->p_veg;
        d->p_veg = 0;
    }
    d->vegetationInited = false;
}


bool API::InitReadCreatures( uint32_t &numcreatures )
{
    try
    {
        memory_info * minfo = d->offset_descriptor;
        int creatures = d->offset_descriptor->getAddress ("creatures");
        d->creature_pos_offset = minfo->getOffset ("creature_position");
        d->creature_type_offset = minfo->getOffset ("creature_race");
        d->creature_flags1_offset = minfo->getOffset ("creature_flags1");
        d->creature_flags2_offset = minfo->getOffset ("creature_flags2");
        d->creature_first_name_offset = minfo->getOffset ("creature_first_name");
        d->creature_nick_name_offset = minfo->getOffset ("creature_nick_name");
        d->creature_last_name_offset = minfo->getOffset ("creature_last_name");
        d->creature_custom_profession_offset = minfo->getOffset ("creature_custom_profession");
        d->creature_profession_offset = minfo->getOffset ("creature_profession");
        d->creature_sex_offset = minfo->getOffset ("creature_sex");
        d->creature_id_offset = minfo->getOffset ("creature_id");
        d->creature_squad_name_offset = minfo->getOffset ("creature_squad_name");
        d->creature_squad_leader_id_offset = minfo->getOffset ("creature_squad_leader_id");
        d->creature_money_offset = minfo->getOffset ("creature_money");
        d->creature_current_job_offset = minfo->getOffset ("creature_current_job");
        d->creature_current_job_id_offset = minfo->getOffset ("current_job_id");
        d->creature_strength_offset = minfo->getOffset ("creature_strength");
        d->creature_agility_offset = minfo->getOffset ("creature_agility");
        d->creature_toughness_offset = minfo->getOffset ("creature_toughness");
        d->creature_skills_offset = minfo->getOffset ("creature_skills");
        d->creature_labors_offset = minfo->getOffset ("creature_labors");
        d->creature_happiness_offset = minfo->getOffset ("creature_happiness");
        d->creature_traits_offset = minfo->getOffset ("creature_traits");
        d->creature_likes_offset = minfo->getOffset("creature_likes");

        d->p_cre = new DfVector (d->p->readVector (creatures, 4));
        //InitReadNameTables();
        d->creaturesInited = true;
        numcreatures =  d->p_cre->getSize();
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->creaturesInited = false;
        numcreatures = 0;
        throw;
    }
}
bool API::InitReadNotes( uint32_t &numnotes )
{
    try
    {
        memory_info * minfo = d->offset_descriptor;
        int notes = minfo->getAddress ("notes");
        d->note_foreground_offset = minfo->getOffset ("note_foreground");
        d->note_background_offset = minfo->getOffset ("note_background");
        d->note_name_offset = minfo->getOffset ("note_name");
        d->note_xyz_offset = minfo->getOffset ("note_xyz");

        d->p_notes = new DfVector (d->p->readVector (notes, 4));
        d->notesInited = true;
        numnotes =  d->p_notes->getSize();
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->notesInited = false;
        numnotes = 0;
        throw;
    }
}
bool API::ReadNote (const int32_t index, t_note & note)
{
    if(!d->notesInited) return false;
    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_notes->at (index);
    note.symbol = g_pProcess->readByte(temp);
    note.foreground = g_pProcess->readWord(temp + d->note_foreground_offset);
    note.background = g_pProcess->readWord(temp + d->note_background_offset);
    d->p->readSTLString (temp + d->note_name_offset, note.name, 128);
    g_pProcess->read (temp + d->note_xyz_offset, 3*sizeof (uint16_t), (uint8_t *) &note.x);
    return true;
}
bool API::InitReadSettlements( uint32_t & numsettlements )
{
    try
    {
        memory_info * minfo = d->offset_descriptor;
        int allSettlements = minfo->getAddress ("settlements");
        int currentSettlement = minfo->getAddress("settlement_current");
        d->settlement_name_offset = minfo->getOffset ("settlement_name");
        d->settlement_world_xy_offset = minfo->getOffset ("settlement_world_xy");
        d->settlement_local_xy_offset = minfo->getOffset ("settlement_local_xy");

        d->p_settlements = new DfVector (d->p->readVector (allSettlements, 4));
        d->p_current_settlement = new DfVector(d->p->readVector(currentSettlement,4));
        d->settlementsInited = true;
        numsettlements =  d->p_settlements->getSize();
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->settlementsInited = false;
        numsettlements = 0;
        throw;
    }
}
bool API::ReadSettlement(const int32_t index, t_settlement & settlement)
{
    if(!d->settlementsInited) return false;
    if(!d->p_settlements->getSize()) return false;
    
    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_settlements->at (index);
    settlement.origin = temp;
    g_pProcess->read(temp + d->settlement_name_offset, 2 * sizeof(int32_t), (uint8_t *) &settlement.name);
    g_pProcess->read(temp + d->settlement_world_xy_offset, 2 * sizeof(int16_t), (uint8_t *) &settlement.world_x);
    g_pProcess->read(temp + d->settlement_local_xy_offset, 4 * sizeof(int16_t), (uint8_t *) &settlement.local_x1);
    return true;
}

bool API::ReadCurrentSettlement(t_settlement & settlement)
{
    if(!d->settlementsInited) return false;
    if(!d->p_current_settlement->getSize()) return false;
    
    uint32_t temp = * (uint32_t *) d->p_current_settlement->at(0);
    settlement.origin = temp;
    g_pProcess->read(temp + d->settlement_name_offset, 2 * sizeof(int32_t), (uint8_t *) &settlement.name);
    g_pProcess->read(temp + d->settlement_world_xy_offset, 2 * sizeof(int16_t), (uint8_t *) &settlement.world_x);
    g_pProcess->read(temp + d->settlement_local_xy_offset, 4 * sizeof(int16_t), (uint8_t *) &settlement.local_x1);
    return true;
}

void API::FinishReadSettlements()
{
    if(d->p_settlements)
    {
        delete d->p_settlements;
        d->p_settlements = NULL;
    }
    if(d->p_current_settlement)
    {
        delete d->p_current_settlement;
        d->p_current_settlement = NULL;
    }
    d->settlementsInited = false;
}


bool API::InitReadHotkeys( )
{
    try
    {
        memory_info * minfo = d->offset_descriptor;
        d->hotkey_start = minfo->getAddress("hotkey_start");
        d->hotkey_mode_offset = minfo->getOffset ("hotkey_mode");
        d->hotkey_xyz_offset = minfo->getOffset("hotkey_xyz");
        d->hotkey_size = minfo->getHexValue("hotkey_size");

        d->hotkeyInited = true;
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->hotkeyInited = false;
        throw;
    }
}
bool API::ReadHotkeys(t_hotkey hotkeys[])
{
    if (!d->hotkeyInited) return false;
    uint32_t currHotkey = d->hotkey_start;
    for(uint32_t i = 0 ; i < NUM_HOTKEYS ;i++)
    {
        d->p->readSTLString(currHotkey,hotkeys[i].name,10);
        hotkeys[i].mode = g_pProcess->readWord(currHotkey+d->hotkey_mode_offset);
        g_pProcess->read (currHotkey + d->hotkey_xyz_offset, 3*sizeof (int32_t), (uint8_t *) &hotkeys[i].x);
        currHotkey+=d->hotkey_size;
    }
    return true;
}
// returns index of creature actually read or -1 if no creature can be found
int32_t API::ReadCreatureInBox (int32_t index, t_creature & furball,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
    if (!d->creaturesInited) return -1;
    uint16_t coords[3];
    uint32_t size = d->p_cre->getSize();
    while (uint32_t(index) < size)
    {
        // read pointer from vector at position
        uint32_t temp = * (uint32_t *) d->p_cre->at (index);
        g_pProcess->read (temp + d->creature_pos_offset, 3 * sizeof (uint16_t), (uint8_t *) &coords);
        if (coords[0] >= x1 && coords[0] < x2)
        {
            if (coords[1] >= y1 && coords[1] < y2)
            {
                if (coords[2] >= z1 && coords[2] < z2)
                {
                    ReadCreature (index, furball);
                    return index;
                }
            }
        }
        index++;
    }
    return -1;
}

bool API::getItemIndexesInBox(vector<uint32_t> &indexes,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
    if(!d->itemsInited) return false;
    indexes.clear();
    uint32_t size = d->p_itm->getSize();
    struct temp2{
        uint16_t coords[3];
        uint32_t flags;
    };
    temp2 temp2;
    for(uint32_t i =0;i<size;i++){
        uint32_t temp = *(uint32_t *) d->p_itm->at(i);
        g_pProcess->read(temp+sizeof(uint32_t),5 * sizeof(uint16_t), (uint8_t *) &temp2);
        if(temp2.flags & (1 << 0)){
            if (temp2.coords[0] >= x1 && temp2.coords[0] < x2)
            {
                if (temp2.coords[1] >= y1 && temp2.coords[1] < y2)
                {
                    if (temp2.coords[2] >= z1 && temp2.coords[2] < z2)
                    {
                        indexes.push_back(i);
                    }
                }
            }
        }
    }
    return true;
}

bool API::ReadCreature (const int32_t index, t_creature & furball)
{
    if(!d->creaturesInited) return false;
    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_cre->at (index);
    furball.origin = temp;
    //read creature from memory
    g_pProcess->read (temp + d->creature_pos_offset, 3 * sizeof (uint16_t), (uint8_t *) & (furball.x)); // xyz really
    g_pProcess->readDWord (temp + d->creature_type_offset, furball.type);
    g_pProcess->readDWord (temp + d->creature_flags1_offset, furball.flags1.whole);
    g_pProcess->readDWord (temp + d->creature_flags2_offset, furball.flags2.whole);
    // normal names
    d->p->readSTLString (temp + d->creature_first_name_offset, furball.first_name, 128);
    d->p->readSTLString (temp + d->creature_nick_name_offset, furball.nick_name, 128);
    // custom profession
    d->p->readSTLString (temp + d->creature_nick_name_offset, furball.nick_name, 128);
    fill_char_buf (furball.custom_profession, d->p->readSTLString (temp + d->creature_custom_profession_offset));
    // crazy composited names
    g_pProcess->read (temp + d->creature_last_name_offset, sizeof (t_lastname), (uint8_t *) &furball.last_name);
    g_pProcess->read (temp + d->creature_squad_name_offset, sizeof (t_squadname), (uint8_t *) &furball.squad_name);



    // labors
    g_pProcess->read (temp + d->creature_labors_offset, NUM_CREATURE_LABORS, furball.labors);
    // traits
    g_pProcess->read (temp + d->creature_traits_offset, sizeof (uint16_t) * NUM_CREATURE_TRAITS, (uint8_t *) &furball.traits);
    // learned skills
    DfVector skills (d->p->readVector (temp + d->creature_skills_offset, 4));
    furball.numSkills = skills.getSize();
    for (uint32_t i = 0; i < furball.numSkills;i++)
    {
        uint32_t temp2 = * (uint32_t *) skills[i];
        //skills.read(i, (uint8_t *) &temp2);
        // a byte: this gives us 256 skills maximum.
        furball.skills[i].id = g_pProcess->readByte (temp2);
        furball.skills[i].rating = g_pProcess->readByte (temp2 + 4);
        furball.skills[i].experience = g_pProcess->readWord (temp2 + 8);
    }
    // profession
    furball.profession = g_pProcess->readByte (temp + d->creature_profession_offset);
    // current job HACK: the job object isn't cleanly represented here
    uint32_t jobIdAddr = g_pProcess->readDWord (temp + d->creature_current_job_offset);
    
    if (jobIdAddr)
    {
        furball.current_job.active = true;
        furball.current_job.jobId = g_pProcess->readByte (jobIdAddr + d->creature_current_job_id_offset);
    }
    else
    {
        furball.current_job.active = false;
    }
    
    //likes
    DfVector likes(d->p->readVector(temp+d->creature_likes_offset,4));
    furball.numLikes = likes.getSize();
    for(uint32_t i = 0;i<furball.numLikes;i++)
    {
        uint32_t temp2 = *(uint32_t *) likes[i];
        g_pProcess->read(temp2,sizeof(t_like),(uint8_t *) &furball.likes[i]);
    }
    
    g_pProcess->readDWord (temp + d->creature_happiness_offset, furball.happiness);
    g_pProcess->readDWord (temp + d->creature_id_offset, furball.id);
    g_pProcess->readDWord (temp + d->creature_agility_offset, furball.agility);
    g_pProcess->readDWord (temp + d->creature_strength_offset, furball.strength);
    g_pProcess->readDWord (temp + d->creature_toughness_offset, furball.toughness);
    g_pProcess->readDWord (temp + d->creature_money_offset, furball.money);
    furball.squad_leader_id = (int32_t) g_pProcess->readDWord (temp + d->creature_squad_leader_id_offset);
    g_pProcess->readByte (temp + d->creature_sex_offset, furball.sex);
    return true;
}

void API::WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS])
{
    uint32_t temp = * (uint32_t *) d->p_cre->at (index);
    WriteRaw(temp + d->creature_labors_offset, NUM_CREATURE_LABORS, labors);
}

bool API::InitReadNameTables (map< string, vector<string> > & nameTable)
{
    try
    {
        int genericAddress = d->offset_descriptor->getAddress ("language_vector");
        int transAddress = d->offset_descriptor->getAddress ("translation_vector");
        int word_table_offset = d->offset_descriptor->getOffset ("word_table");

        DfVector genericVec (d->p->readVector (genericAddress, 4));
        DfVector transVec (d->p->readVector (transAddress, 4));

        for (uint32_t i = 0;i < genericVec.getSize();i++)
        {
            uint32_t genericNamePtr = * (uint32_t *) genericVec.at (i);
            string genericName = d->p->readSTLString (genericNamePtr);
            nameTable["GENERIC"].push_back (genericName);
        }

        for (uint32_t i = 0; i < transVec.getSize();i++)
        {
            uint32_t transPtr = * (uint32_t *) transVec.at (i);
            string transName = d->p->readSTLString (transPtr);
            DfVector trans_names_vec (d->p->readVector (transPtr + word_table_offset, 4));
            for (uint32_t j = 0;j < trans_names_vec.getSize();j++)
            {
                uint32_t transNamePtr = * (uint32_t *) trans_names_vec.at (j);
                string name = d->p->readSTLString (transNamePtr);
                nameTable[transName].push_back (name);
            }
        }
        d->nameTablesInited = true;
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->nameTablesInited = false;
        throw;
    }
}

string API::TranslateName (const int names[], int size, const map<string, vector<string> > & nameTable, const string & language)
{
    string trans;
    assert (d->nameTablesInited);
    map<string, vector<string> >::const_iterator it;
    it = nameTable.find (language);
    if (it != nameTable.end())
    {
        for (int i = 0;i < size;i++)
        {
            if (names[i] == -1)
            {
                break;
            }
            trans.append (it->second[names[i]]);
        }
    }
    return (trans);
}

string API::TranslateName (const t_lastname & last, const map<string, vector<string> > & nameTable, const string & language)
{
    string trans_last;
    assert (d->nameTablesInited);
    map<string, vector<string> >::const_iterator it;
    it = nameTable.find (language);
    if (it != nameTable.end())
    {
        for (int i = 0;i < 7;i++)
        {
            if (last.names[i] == -1)
            {
                break;
            }
            trans_last.append (it->second[last.names[i]]);
        }
    }
    return (trans_last);
}
string API::TranslateName (const t_squadname & squad, const map<string, vector<string> > & nameTable, const string & language)
{
    string trans_squad;
    assert (d->nameTablesInited);
    map<string, vector<string> >::const_iterator it;
    it = nameTable.find (language);
    if (it != nameTable.end())
    {
        for (int i = 0;i < 7;i++)
        {
            if (squad.names[i] == -1)
            {
                continue;
            }
            if (squad.names[i] == 0)
            {
                break;
            }
            if (i == 4)
            {
                trans_squad.append (" ");
            }
            trans_squad.append (it->second[squad.names[i]]);
        }
    }
    return (trans_squad);
}

void API::FinishReadNameTables()
{
    d->nameTablesInited = false;
}

void API::FinishReadCreatures()
{
    if(d->p_cre)
    {
        delete d->p_cre;
        d->p_cre = 0;
    }
    d->creaturesInited = false;
    //FinishReadNameTables();
}
void API::FinishReadNotes()
{
    if(d->p_notes)
    {
        delete d->p_notes;
        d->p_notes = 0;
    }
    d->notesInited = false;
}

bool API::Attach()
{
    // detach all processes, destroy manager
    if (d->pm == 0)
    {
        d->pm = new ProcessEnumerator (d->xml); // FIXME: handle bad XML better
    }
    else
    {
        d->pm->purge();
    }

    // find a process (ProcessManager can find multiple when used properly)
    if (!d->pm->findProcessess())
    {
        throw Error::NoProcess();
        //cerr << "couldn't find a suitable process" << endl;
        //return false;
    }
    d->p = (*d->pm) [0];
    if (!d->p->attach())
    {
        throw Error::CantAttach();
        //cerr << "couldn't attach to process" << endl;
        //return false; // couldn't attach to process, no go
    }
    d->shm_start = d->p->getSHMStart();
    d->offset_descriptor = d->p->getDescriptor();
    // process is attached, everything went just fine... hopefully
    return true;
}


bool API::Detach()
{
    if (!d->p->detach())
    {
        return false;
    }
    if (d->pm != NULL)
    {
        delete d->pm;
    }
    d->pm = NULL;
    d->p = NULL;
    d->shm_start = 0;
    d->offset_descriptor = NULL;
    return true;
}

bool API::isAttached()
{
    return d->p != NULL;
}

bool API::Suspend()
{
    return d->p->suspend();
}
bool API::AsyncSuspend()
{
    return d->p->asyncSuspend();
}

bool API::Resume()
{
    return d->p->resume();
}
bool API::ForceResume()
{
    return d->p->forceresume();
}
bool API::isSuspended()
{
    return d->p->isSuspended();
}

void API::ReadRaw (const uint32_t offset, const uint32_t size, uint8_t *target)
{
    g_pProcess->read (offset, size, target);
}

void API::WriteRaw (const uint32_t offset, const uint32_t size, uint8_t *source)
{
    g_pProcess->write (offset, size, source);
}

bool API::InitViewAndCursor()
{
    try
    {
        d->window_x_offset = d->offset_descriptor->getAddress ("window_x");
        d->window_y_offset = d->offset_descriptor->getAddress ("window_y");
        d->window_z_offset = d->offset_descriptor->getAddress ("window_z");
        d->cursor_xyz_offset = d->offset_descriptor->getAddress ("cursor_xyz");
        d->current_cursor_creature_offset = d->offset_descriptor->getAddress ("current_cursor_creature");

        d->current_menu_state_offset = d->offset_descriptor->getAddress("current_menu_state");
        d->pause_state_offset = d->offset_descriptor->getAddress ("pause_state");
        d->view_screen_offset = d->offset_descriptor->getAddress ("view_screen");

        d->cursorWindowInited = true;
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->cursorWindowInited = false;
        throw;
    }
}

bool API::InitViewSize()
{
    try
    {
        d->window_dims_offset = d->offset_descriptor->getAddress ("window_dims");

        d->viewSizeInited = true;
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->viewSizeInited = false;
        throw;
    }
}

bool API::getViewCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if (!d->cursorWindowInited) return false;
    g_pProcess->readDWord (d->window_x_offset, (uint32_t &) x);
    g_pProcess->readDWord (d->window_y_offset, (uint32_t &) y);
    g_pProcess->readDWord (d->window_z_offset, (uint32_t &) z);
    return true;
}
//FIXME: confine writing of coords to map bounds?
bool API::setViewCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->cursorWindowInited) return false;
    g_pProcess->writeDWord (d->window_x_offset, (uint32_t) x);
    g_pProcess->writeDWord (d->window_y_offset, (uint32_t) y);
    g_pProcess->writeDWord (d->window_z_offset, (uint32_t) z);
    return true;
}

bool API::getCursorCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if(!d->cursorWindowInited) return false;
    int32_t coords[3];
    g_pProcess->read (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    x = coords[0];
    y = coords[1];
    z = coords[2];
    if (x == -30000) return false;
    return true;
}
//FIXME: confine writing of coords to map bounds?
bool API::setCursorCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->cursorWindowInited) return false;
    int32_t coords[3] = {x, y, z};
    g_pProcess->write (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    return true;
}
bool API::getWindowSize (int32_t &width, int32_t &height)
{
    if(! d->viewSizeInited) return false;
    
    int32_t coords[2];
    g_pProcess->read (d->window_dims_offset, 2*sizeof (int32_t), (uint8_t *) coords);
    width = coords[0];
    height = coords[1];
    return true;
}
/*
bool API::getClassIDMapping (vector <string>& objecttypes)
{
    if(isAttached())
    {
        d->offset_descriptor->getClassIDMapping(objecttypes);
        return true;
    }
    return false;
}
*/
memory_info *API::getMemoryInfo()
{
    return d->offset_descriptor;
}
Process * API::getProcess()
{
    return d->p;
}

DFWindow * API::getWindow()
{
    return d->p->getWindow();
}

bool API::InitReadItems(uint32_t & numitems)
{
    try
    {
        int items = d->offset_descriptor->getAddress ("items");
        d->item_material_offset = d->offset_descriptor->getOffset ("item_materials");

        d->p_itm = new DfVector (d->p->readVector (items, 4));
        d->itemsInited = true;
        numitems = d->p_itm->getSize();
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->itemsInited = false;
        numitems = 0;
        throw;
    }
}
bool API::ReadItem (const uint32_t index, t_item & item)
{
    if (!d->itemsInited) return false;
    
    t_item_df40d item_40d;

    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_itm->at (index);

    //read building from memory
    g_pProcess->read (temp, sizeof (t_item_df40d), (uint8_t *) &item_40d);

    // transform
    int32_t type = -1;
    d->offset_descriptor->resolveObjectToClassID (temp, type);
    item.origin = temp;
    item.vtable = item_40d.vtable;
    item.x = item_40d.x;
    item.y = item_40d.y;
    item.z = item_40d.z;
    item.type = type;
    item.ID = item_40d.ID;
    item.flags.whole = item_40d.flags;

    //TODO  certain item types (creature based, threads, seeds, bags do not have the first matType byte, instead they have the material index only located at 0x68
    g_pProcess->read (temp + d->item_material_offset, sizeof (t_matglossPair), (uint8_t *) &item.material);
    //for(int i = 0; i < 0xCC; i++){  // used for item research
    //    uint8_t byte = MreadByte(temp+i);
    //    item.bytes.push_back(byte);
    //}
    return true;
}
void API::FinishReadItems()
{
    if(d->p_itm)
    {
        delete d->p_itm;
        d->p_itm = NULL;
    }
    d->itemsInited = false;
}

bool API::ReadPauseState()
{
    // replace with an exception
    if(!d->cursorWindowInited) return false;

    uint32_t pauseState = g_pProcess->readDWord (d->pause_state_offset);
    return pauseState & 1;
}

uint32_t API::ReadMenuState()
{
    if(d->cursorWindowInited)
        return(g_pProcess->readDWord(d->current_menu_state_offset));
    return false;
}

bool API::ReadViewScreen (t_viewscreen &screen)
{
    if (!d->cursorWindowInited) return false;
    
    uint32_t last = g_pProcess->readDWord (d->view_screen_offset);
    uint32_t screenAddr = g_pProcess->readDWord (last);
    uint32_t nextScreenPtr = g_pProcess->readDWord (last + 4);
    while (nextScreenPtr != 0)
    {
        last = nextScreenPtr;
        screenAddr = g_pProcess->readDWord (nextScreenPtr);
        nextScreenPtr = g_pProcess->readDWord (nextScreenPtr + 4);
    }
    return d->offset_descriptor->resolveObjectToClassID (last, screen.type);
}

bool API::ReadItemTypes(vector< vector< t_itemType > > & itemTypes)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress("matgloss");
    int matgloss_skip = minfo->getHexValue("matgloss_skip");
    int item_type_name_offset = minfo->getOffset("item_type_name");
    for(int i = 8;i<20;i++)
    {
        DfVector p_temp = d->p->readVector(matgloss_address + i*matgloss_skip,4);
        vector< t_itemType > typesForVec;
        for(uint32_t j =0; j<p_temp.getSize();j++)
        {
            t_itemType currType;
            uint32_t temp = *(uint32_t *) p_temp[j];
           // Mread(temp+40,sizeof(name),(uint8_t *) name);
            d->p->readSTLString(temp+4,currType.id,128);
            d->p->readSTLString(temp+item_type_name_offset,currType.name,128);
            //stringsForVec.push_back(string(name));
            typesForVec.push_back(currType);
        }
        itemTypes.push_back(typesForVec);
    }
    return true;
}