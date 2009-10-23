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

#ifndef BUILD_DFHACK_LIB
#   define BUILD_DFHACK_LIB
#endif

#include "DFCommon.h"
#include "DFVector.h"
#include "DFHackAPI.h"
#include "DFMemInfo.h"

#if defined(_MSC_VER) && _MSC_VER >= 1400
#	define fill_char_buf(buf, str) strcpy_s((buf), sizeof(buf) / sizeof((buf)[0]), (str).c_str())
#else
#	define fill_char_buf(buf, str) strncpy((buf), (str).c_str(), sizeof(buf) / sizeof((buf)[0]))
#endif

/* TODO: Test these
    matgloss other than stone/soil
*/

DFHACKAPI DFHackAPI *CreateDFHackAPI0(const char *path_to_xml)
{
	return new DFHackAPIImpl(path_to_xml);
}

// TODO: templating for vectors, simple copy constructor for stl vectors
// TODO: encapsulate access to multidimensional arrays.

DFHackAPIImpl::DFHackAPIImpl(const string path_to_xml)
: block(NULL)
, pm(NULL), p(NULL), dm(NULL), offset_descriptor(NULL)
, p_cons(NULL), p_bld(NULL), p_veg(NULL)
{
    xml = path_to_xml;
    constructionsInited = false;
    creaturesInited = false;
    buildingsInited = false;
    vegetationInited = false;
    pm = NULL;
}


/*-----------------------------------*
 *  Init the mapblock pointer array   *
 *-----------------------------------*/
bool DFHackAPIImpl::InitMap()
{
    uint32_t    map_loc,   // location of the X array
                temp_loc,  // block location
                temp_locx, // iterator for the X array
                temp_locy, // iterator for the Y array
                temp_locz; // iterator for the Z array
    uint32_t map_offset = offset_descriptor->getAddress("map_data");
    uint32_t x_count_offset = offset_descriptor->getAddress("x_count");
    uint32_t y_count_offset = offset_descriptor->getAddress("y_count");
    uint32_t z_count_offset = offset_descriptor->getAddress("z_count");

    // get the offsets once here
    tile_type_offset = offset_descriptor->getOffset("type");
    designation_offset = offset_descriptor->getOffset("designation");
    occupancy_offset = offset_descriptor->getOffset("occupancy");

    // get the map pointer
    map_loc = MreadDWord(map_offset);
    if (!map_loc)
    {
        // bad stuffz happend
        return false;
    }

    // get the size
    x_block_count = MreadDWord(x_count_offset);
    y_block_count = MreadDWord(y_count_offset);
    z_block_count = MreadDWord(z_count_offset);

    // alloc array for pinters to all blocks
    block = new uint32_t[x_block_count*y_block_count*z_block_count];

    //read the memory from the map blocks - x -> map slice
    for(uint32_t x = 0; x < x_block_count; x++)
    {
        temp_locx = map_loc + ( 4 * x );
        temp_locy = MreadDWord(temp_locx);

        // y -> map column
        for(uint32_t y = 0; y < y_block_count; y++)
        {
            temp_locz = MreadDWord(temp_locy);
            temp_locy += 4;

            // z -> map block (16x16)
            for(uint32_t z = 0; z < z_block_count; z++)
            {
                temp_loc = MreadDWord(temp_locz);
                temp_locz += 4;
                block[x*y_block_count*z_block_count + y*z_block_count + z] = temp_loc;
            }
        }
    }
    return true;
}

bool DFHackAPIImpl::DestroyMap()
{
	if (block != NULL)
	{
		delete [] block;
		block = NULL;
	}

	return true;
}

bool DFHackAPIImpl::isValidBlock(uint32_t x, uint32_t y, uint32_t z)
{
    return block[x*y_block_count*z_block_count + y*z_block_count + z] != NULL;
}

// 256 * sizeof(uint16_t)
bool DFHackAPIImpl::ReadTileTypes(uint32_t x, uint32_t y, uint32_t z, uint16_t *buffer)
{
    uint32_t addr = block[x*y_block_count*z_block_count + y*z_block_count + z];
    if (addr!=NULL)
    {
        Mread(addr+tile_type_offset, 256 * sizeof(uint16_t), (uint8_t *)buffer);
        return true;
    }
    return false;
}


// 256 * sizeof(uint32_t)
bool DFHackAPIImpl::ReadDesignations(uint32_t x, uint32_t y, uint32_t z, uint32_t *buffer)
{
    uint32_t addr = block[x*y_block_count*z_block_count + y*z_block_count + z];
    if (addr!=NULL)
    {
        Mread(addr+designation_offset, 256 * sizeof(uint32_t), (uint8_t *)buffer);
        return true;
    }
    return false;
}


// 256 * sizeof(uint32_t)
bool DFHackAPIImpl::ReadOccupancy(uint32_t x, uint32_t y, uint32_t z, uint32_t *buffer)
{
    uint32_t addr = block[x*y_block_count*z_block_count + y*z_block_count + z];
    if (addr!=NULL)
    {
        Mread(addr+occupancy_offset, 256 * sizeof(uint32_t), (uint8_t *)buffer);
        return true;
    }
    return false;
}


// 256 * sizeof(uint16_t)
bool DFHackAPIImpl::WriteTileTypes(uint32_t x, uint32_t y, uint32_t z, uint16_t *buffer)
{
    uint32_t addr = block[x*y_block_count*z_block_count + y*z_block_count + z];
    if (addr!=NULL)
    {
        Mwrite(addr+tile_type_offset, 256 * sizeof(uint16_t), (uint8_t *)buffer);
        return true;
    }
    return false;
}


// 256 * sizeof(uint32_t)
bool DFHackAPIImpl::WriteDesignations(uint32_t x, uint32_t y, uint32_t z, uint32_t *buffer)
{
    uint32_t addr = block[x*y_block_count*z_block_count + y*z_block_count + z];
    if (addr!=NULL)
    {
        Mwrite(addr+designation_offset, 256 * sizeof(uint32_t), (uint8_t *)buffer);
        return true;
    }
    return false;
}


// 256 * sizeof(uint32_t)
bool DFHackAPIImpl::WriteOccupancy(uint32_t x, uint32_t y, uint32_t z, uint32_t *buffer)
{
    uint32_t addr = block[x*y_block_count*z_block_count + y*z_block_count + z];
    if (addr!=NULL)
    {
        Mwrite(addr+occupancy_offset, 256 * sizeof(uint32_t), (uint8_t *)buffer);
        return true;
    }
    return false;
}


//16 of them? IDK... there's probably just 7. Reading more doesn't cause errors as it's an array nested inside a block
// 16 * sizeof(uint8_t)
bool DFHackAPIImpl::ReadRegionOffsets(uint32_t x, uint32_t y, uint32_t z, uint8_t *buffer)
{
    uint32_t biome_stuffs = offset_descriptor->getOffset("biome_stuffs");
    uint32_t addr = block[x*y_block_count*z_block_count + y*z_block_count + z];
    if (addr!=NULL)
    {
        Mread(addr+biome_stuffs, 16 * sizeof(uint8_t), buffer);
        return true;
    }
    return false;
}


// veins of a block, expects empty vein vector
bool DFHackAPIImpl::ReadVeins(uint32_t x, uint32_t y, uint32_t z, vector <t_vein> & veins)
{
    uint32_t addr = block[x*y_block_count*z_block_count + y*z_block_count + z];
    int veinvector = offset_descriptor->getOffset("v_vein");
    int veinsize = offset_descriptor->getHexValue("v_vein_size");
    veins.clear();
    if(addr!=NULL && veinvector && veinsize)
    {
        assert(sizeof(t_vein) == veinsize);
        // veins are stored as a vector of pointers to veins
        /*pointer is 4 bytes! we work with a 32bit program here, no matter what architecture we compile khazad for*/
        DfVector p_veins = dm->readVector(addr + veinvector, 4);

        // read all veins
        for (uint32_t i = 0; i< p_veins.getSize();i++)
        {
            t_vein v;
            uint32_t temp;
            // read the vein pointer from the vector
            p_veins.read((uint32_t)i,(uint8_t *)&temp);
            // read the vein data (dereference pointer)
            Mread(temp, veinsize, (uint8_t *)&v);
            // store it in the vector
            veins.push_back(v);
        }
        return true;
    }
    return false;
}


// getter for map size
void DFHackAPIImpl::getSize(uint32_t& x, uint32_t& y, uint32_t& z)
{
    x = x_block_count;
    y = y_block_count;
    z = z_block_count;
}

bool DFHackAPIImpl::ReadWoodMatgloss(vector<t_matgloss> & woods)
{
    int matgloss_address = offset_descriptor->getAddress("matgloss");
    // TODO: find flag for autumnal coloring?
    DfVector p_matgloss = dm->readVector(matgloss_address, 4);
    
    woods.clear();
    
    t_matgloss mat;
    // TODO: use brown?
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    for (uint32_t i = 0; i< p_matgloss.getSize();i++)
    {
        uint32_t temp;
        
        // read the matgloss pointer from the vector into temp
        p_matgloss.read((uint32_t)i,(uint8_t *)&temp);
        
        // read the string pointed at by
        fill_char_buf(mat.id, dm->readSTLString(temp)); // reads a C string given an address
        woods.push_back(mat);
    }
    return true;
}

bool DFHackAPIImpl::ReadStoneMatgloss(vector<t_matgloss> & stones)
{
    int matgloss_address = offset_descriptor->getAddress("matgloss");
    int matgloss_offset = offset_descriptor->getHexValue("matgloss_skip");
    int matgloss_colors = offset_descriptor->getOffset("matgloss_stone_color");
    DfVector p_matgloss = dm->readVector(matgloss_address + matgloss_offset, 4);

    stones.clear();

    for (uint32_t i = 0; i< p_matgloss.getSize();i++)
    {
        uint32_t temp;
        // read the matgloss pointer from the vector into temp
        p_matgloss.read((uint32_t)i,(uint8_t *)&temp);
        // read the string pointed at by
        t_matgloss mat;
        fill_char_buf(mat.id, dm->readSTLString(temp)); // reads a C string given an address
        mat.fore = (uint8_t)MreadWord(temp + matgloss_colors);
        mat.back = (uint8_t)MreadWord(temp + matgloss_colors + 2);
        mat.bright = (uint8_t)MreadWord(temp + matgloss_colors + 4);
        stones.push_back(mat);
    }
    return true;
}


bool DFHackAPIImpl::ReadMetalMatgloss(vector<t_matgloss> & metals)
{
    int matgloss_address = offset_descriptor->getAddress("matgloss");
    int matgloss_offset = offset_descriptor->getHexValue("matgloss_skip");
    int matgloss_colors = offset_descriptor->getOffset("matgloss_metal_color");
    DfVector p_matgloss = dm->readVector(matgloss_address + matgloss_offset*2, 4);

    metals.clear();

    for (uint32_t i = 0; i< p_matgloss.getSize();i++)
    {
        uint32_t temp;

        // read the matgloss pointer from the vector into temp
        p_matgloss.read((uint32_t)i,(uint8_t *)&temp);

        // read the string pointed at by
        t_matgloss mat;
        fill_char_buf(mat.id, dm->readSTLString(temp)); // reads a C string given an address
        mat.fore = (uint8_t)MreadWord(temp + matgloss_colors);
        mat.back = (uint8_t)MreadWord(temp + matgloss_colors + 2);
        mat.bright = (uint8_t)MreadWord(temp + matgloss_colors + 4);
        metals.push_back(mat);
    }
    return true;
}

bool DFHackAPIImpl::ReadPlantMatgloss(vector<t_matgloss> & plants)
{
    int matgloss_address = offset_descriptor->getAddress("matgloss");
    int matgloss_offset = offset_descriptor->getHexValue("matgloss_skip");
    DfVector p_matgloss = dm->readVector(matgloss_address + matgloss_offset*3, 4);

    plants.clear();

    // TODO: use green?
    t_matgloss mat;
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    for (uint32_t i = 0; i< p_matgloss.getSize();i++)
    {
        uint32_t temp;

        // read the matgloss pointer from the vector into temp
        p_matgloss.read((uint32_t)i,(uint8_t *)&temp);

        // read the string pointed at by
        fill_char_buf(mat.id, dm->readSTLString(temp)); // reads a C string given an address
        plants.push_back(mat);
    }
    return true;
}

bool DFHackAPIImpl::ReadCreatureMatgloss(vector<t_matgloss> & creatures)
{
    int matgloss_address = offset_descriptor->getAddress("matgloss");
    int matgloss_offset = offset_descriptor->getHexValue("matgloss_skip");
    DfVector p_matgloss = dm->readVector(matgloss_address + matgloss_offset*6, 4);
    
    creatures.clear();
    
    // TODO: use green?
    t_matgloss mat;
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    for (uint32_t i = 0; i< p_matgloss.getSize();i++)
    {
        uint32_t temp;
        
        // read the matgloss pointer from the vector into temp
        p_matgloss.read((uint32_t)i,(uint8_t *)&temp);
        
        // read the string pointed at by
        fill_char_buf(mat.id, dm->readSTLString(temp)); // reads a C string given an address
        creatures.push_back(mat);
    }
    return true;
}


//vector<uint16_t> v_geology[eBiomeCount];
bool DFHackAPIImpl::ReadGeology( vector < vector <uint16_t> >& assign )
{
    // get needed addresses and offsets
    int region_x_offset = offset_descriptor->getAddress("region_x");
    int region_y_offset = offset_descriptor->getAddress("region_y");
    int region_z_offset =  offset_descriptor->getAddress("region_z");
    int world_offset =  offset_descriptor->getAddress("world");
    int world_regions_offset =  offset_descriptor->getOffset("w_regions_arr");
    int region_size =  offset_descriptor->getHexValue("region_size");
    int region_geo_index_offset =  offset_descriptor->getOffset("region_geo_index_off");
    int world_geoblocks_offset =  offset_descriptor->getOffset("w_geoblocks");
    int world_size_x = offset_descriptor->getOffset("world_size_x");
    int world_size_y = offset_descriptor->getOffset("world_size_y");
    int geolayer_geoblock_offset = offset_descriptor->getOffset("geolayer_geoblock_offset");

    uint32_t regionX, regionY, regionZ;
    uint16_t worldSizeX, worldSizeY;

    // check if we have 'em all
    if(
        !(
        region_x_offset && region_y_offset && region_z_offset && world_size_x && world_size_y
        && world_offset && world_regions_offset && world_geoblocks_offset && region_size
        && region_geo_index_offset && geolayer_geoblock_offset
        )
    )
    {
        // fail if we don't have them
        return false;
    }

    // read position of the region inside DF world
    MreadDWord(region_x_offset, regionX);
    MreadDWord(region_y_offset, regionY);
    MreadDWord(region_z_offset, regionZ);

    // get world size
    MreadWord(world_offset + world_size_x, worldSizeX);
    MreadWord(world_offset + world_size_y, worldSizeY);

    // get pointer to first part of 2d array of regions
    uint32_t regions = MreadDWord(world_offset + world_regions_offset);

    // read the geoblock vector
    DfVector geoblocks = dm->readVector(world_offset + world_geoblocks_offset,4);

    // iterate over 8 surrounding regions + local region
    for(int i = eNorthWest; i< eBiomeCount; i++)
    {
        // check bounds, fix them if needed
        int bioRX = regionX / 16 + (i%3) - 1;
        if( bioRX < 0) bioRX = 0;
        if( bioRX >= worldSizeX) bioRX = worldSizeX - 1;
        int bioRY = regionY / 16 + (i/3) - 1;
        if( bioRY < 0) bioRY = 0;
        if( bioRY >= worldSizeY) bioRY = worldSizeY - 1;

        // get pointer to column of regions
        uint32_t geoX;
        MreadDWord(regions + bioRX*4, geoX);

        // get index into geoblock vector
        uint16_t geoindex;
        MreadWord(geoX + bioRY*region_size + region_geo_index_offset, geoindex);

        // get the geoblock from the geoblock vector using the geoindex
        uint32_t geoblock_off;
        geoblocks.read(geoindex,(uint8_t *) &geoblock_off);

        // get the vector with pointer to layers
        DfVector geolayers = dm->readVector(geoblock_off + geolayer_geoblock_offset , 4); // let's hope
        // make sure we don't load crap
        assert(geolayers.getSize() > 0 && geolayers.getSize() <= 16);

        // finally, read the layer matgloss
        for(uint32_t j = 0;j< geolayers.getSize();j++)
        {
            int geol_offset;
            // read pointer to a layer
            geolayers.read(j, (uint8_t *) & geol_offset);
            // read word at pointer + 2, store in our geology vectors
            v_geology[i].push_back(MreadWord(geol_offset + 2));
        }
    }
    assign.clear();
    // TODO: clean this up
    for(int i = 0; i< eBiomeCount;i++)
    {
        assign.push_back(v_geology[i]);
    }
    return true;
}


// returns number of buildings, expects v_buildingtypes that will later map t_building.type to its name
uint32_t DFHackAPIImpl::InitReadBuildings(vector <string> &v_buildingtypes)
{
    buildingsInited = true;
    int buildings = offset_descriptor->getAddress("buildings");
    assert(buildings);
    p_bld = new DfVector( dm->readVector(buildings,4));
    offset_descriptor->copyBuildings(v_buildingtypes);
    return p_bld->getSize();
}


// read one building
bool DFHackAPIImpl::ReadBuilding(const uint32_t &index, t_building & building)
{
    assert(buildingsInited);
    uint32_t temp;
    t_building_df40d bld_40d;

    // read pointer from vector at position
    p_bld->read(index,(uint8_t *)&temp);

    //read building from memory
    Mread(temp, sizeof(t_building_df40d), (uint8_t *)&bld_40d);

    // transform
    int32_t type = -1;
    offset_descriptor->resolveClassId(temp, type);
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


void DFHackAPIImpl::FinishReadBuildings()
{
    buildingsInited = false;
}


//TODO: maybe do construction reading differently - this could go slow with many of them.
// returns number of constructions, prepares a vector, returns total number of constructions
uint32_t DFHackAPIImpl::InitReadConstructions()
{
    constructionsInited = true;
    int constructions = offset_descriptor->getAddress("constructions");
    assert(constructions);

    p_cons = new DfVector(dm->readVector(constructions,4));
    return p_cons->getSize();
}


bool DFHackAPIImpl::ReadConstruction(const uint32_t &index, t_construction & construction)
{
    assert(constructionsInited);
    t_construction_df40d c_40d;
    uint32_t temp;

    // read pointer from vector at position
    p_cons->read((uint32_t)index,(uint8_t *)&temp);

    //read construction from memory
    Mread(temp, sizeof(t_construction_df40d), (uint8_t *)&c_40d);

    // transform
    construction.x = c_40d.x;
    construction.y = c_40d.y;
    construction.z = c_40d.z;
    construction.material = c_40d.material;

	return true;
}


void DFHackAPIImpl::FinishReadConstructions()
{
    constructionsInited = false;
}


uint32_t DFHackAPIImpl::InitReadVegetation()
{
    vegetationInited = true;
    int vegetation = offset_descriptor->getAddress("vegetation");
    tree_offset = offset_descriptor->getOffset("tree_desc_offset");
    assert(vegetation && tree_offset);
    p_veg = new DfVector(dm->readVector(vegetation,4));
    return p_veg->getSize();
}


bool DFHackAPIImpl::ReadVegetation(const uint32_t &index, t_tree_desc & shrubbery)
{
    assert(vegetationInited);
    uint32_t temp;
    // read pointer from vector at position
    p_veg->read(index,(uint8_t *)&temp);
    //read construction from memory
    Mread(temp + tree_offset, sizeof(t_tree_desc), (uint8_t *) &shrubbery);
    // FIXME: this is completely wrong. type isn't just tree/shrub but also different kinds of trees. stuff that grows around ponds has its own type ID
    if(shrubbery.material.type == 3) shrubbery.material.type = 2;
    return true;
}


void DFHackAPIImpl::FinishReadVegetation()
{
    vegetationInited = false;
}


uint32_t DFHackAPIImpl::InitReadCreatures()
{
    creaturesInited = true;
    int creatures = offset_descriptor->getAddress("creatures");
    creature_pos_offset = offset_descriptor->getOffset("creature_position");
    creature_type_offset = offset_descriptor->getOffset("creature_type");
    creature_flags1_offset = offset_descriptor->getOffset("creature_flags1");
    creature_flags2_offset = offset_descriptor->getOffset("creature_flags2");
    assert(creatures && creature_pos_offset && creature_type_offset && creature_flags1_offset && creature_flags2_offset);
    p_cre = new DfVector(dm->readVector(creatures, 4));
    return p_cre->getSize();
}


bool DFHackAPIImpl::ReadCreature(const uint32_t &index, t_creature & furball)
{
    assert(creaturesInited);
    uint32_t temp;
    // read pointer from vector at position
    p_cre->read(index,(uint8_t *)&temp);
    //read creature from memory
    Mread(temp + creature_pos_offset, 3 * sizeof(uint16_t), (uint8_t *) &(furball.x)); // xyz really
    Mread(temp + creature_type_offset, sizeof(uint32_t), (uint8_t *) &furball.type);
    Mread(temp + creature_flags1_offset, sizeof(uint32_t), (uint8_t *) &furball.flags1);
    Mread(temp + creature_flags2_offset, sizeof(uint32_t), (uint8_t *) &furball.flags2);
    return true;
}


void DFHackAPIImpl::FinishReadCreatures()
{
    creaturesInited = false;
}

bool DFHackAPIImpl::Attach()
{
    // detach all processes, destroy manager
    if(pm != NULL)
        delete pm;
    // find a process (ProcessManager can find multiple when used properly)
    pm = new ProcessManager(xml); // FIXME: handle bad XML better
    if(!pm->findProcessess())
        return false; // FIXME: throw exceptions to distinguish errors? provide error code?
    p = (*pm)[0]; // we just use the first found process
    if(!p->attach())
        return false; // couldn't attach to process, no go
    offset_descriptor = p->getDescriptor();
    dm = p->getDataModel();
    // process is attached, everything went just fine... hopefully
    return true;
}


// TODO: clean inited stuff here
bool DFHackAPIImpl::Detach()
{
    p->detach();
    if(pm != NULL)
        delete pm;
    pm = NULL;
    p = NULL;
    offset_descriptor = NULL;
    dm = NULL;
    return true;
}

bool DFHackAPIImpl::isAttached()
{
	return dm != NULL;
}
