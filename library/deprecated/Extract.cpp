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

// Extractor
#include "DFCommon.h"
using namespace std;

#include "Extract.h"
#include "DFDataModel.h"
#include "DFMemInfo.h"

Extractor::Extractor()
{
    df_map = NULL; // important, null pointer means we don't have a map loaded
}


Extractor::~Extractor()
{
    if(df_map !=NULL )
    {
        delete df_map;
    }
}


bool Extractor::dumpMemory( string path_to_xml)
{
    // create process manager, get first process
    ProcessManager pm(path_to_xml);
    if(!pm.findProcessess())
    {
        fprintf(stderr,"Can't find any suitable DF process\n");
        return false;
    }
    // attach to process
    printf("Attempting to Attach Process\n");
    ///FIXME: this won't do.
    Process * p = pm[0];
    DataModel * dm = p->getDataModel();
    if(!p->attach())
    {
        printf("Could not Attach Process, Aborting\n");
        return false; // couldn't attach to process, no go
    }
    printf("Process succesfully Attached\n");
    memory_info* offset_descriptor = p->getDescriptor();

    uint32_t map_loc, // location of the X array
            temp_loc, // block location
            temp_locx, // iterator for the X array
            temp_locy, // iterator for the Y array
            temp_locz; // iterator for the Z array
    unsigned blocks_read = 0U;

    // Read Map Data Blocks
    int map_offset = offset_descriptor->getAddress("map_data");;
    int x_count_offset = offset_descriptor->getAddress("x_count");
    int y_count_offset = offset_descriptor->getAddress("y_count");
    int z_count_offset = offset_descriptor->getAddress("z_count");
    int tile_type_offset = offset_descriptor->getOffset("type");
    int designation_offset = offset_descriptor->getOffset("designation");
    int occupancy_offset = offset_descriptor->getOffset("occupancy");
    int biome_stuffs = offset_descriptor->getOffset("biome_stuffs");

    // layers
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
    // veins
    int veinvector = offset_descriptor->getOffset("v_vein");
    int veinsize = offset_descriptor->getHexValue("v_vein_size");

    int vegetation = offset_descriptor->getAddress("vegetation");
    int tree_desc_offset = offset_descriptor->getOffset("tree_desc_offset");
    // constructions
    int constructions = offset_descriptor->getAddress("constructions");
    // buildings
    int buildings = offset_descriptor->getAddress("buildings");
    /// TODO: what about building shape and orientation?

    // matgloss
    int matgloss_address = offset_descriptor->getAddress("matgloss");
    int matgloss_skip = offset_descriptor->getHexValue("matgloss_skip");

    bool have_geology = false;

    printf("Map offset: 0x%.8X\n", map_offset);
    map_loc = MreadDWord(map_offset);

    if (!map_loc)
    {
        printf("Could not find DF map information in memory, Aborting\n");
        return false;
    }
    printf("Map data Found at: 0x%.8X\n", map_loc);

    if(df_map != NULL)
    {
        delete df_map;
    }
    df_map = new DfMap(MreadDWord(x_count_offset),MreadDWord(y_count_offset),MreadByte(z_count_offset));

    // read matgloss data from df if we can
    RawType matglossRawMapping[] = {Mat_Wood, Mat_Stone, Mat_Metal, Mat_Plant};
    if(matgloss_address && matgloss_skip)
    {
        uint32_t addr = matgloss_address;
        uint32_t counter = Mat_Wood;

        for(; counter < NUM_MATGLOSS_TYPES; addr += matgloss_skip, counter++)
        {
            // get vector of matgloss pointers
            DfVector p_matgloss = dm->readVector(addr, 4);

            // iterate over it
            for (uint32_t i = 0; i< p_matgloss.getSize();i++)
            {
                uint32_t temp;
                string tmpstr;

                // read the matgloss pointer from the vector into temp
                p_matgloss.read((uint32_t)i,(uint8_t *)&temp);

                // read the string pointed at by temp
                tmpstr = dm->readSTLString(temp);

                // store it in the block
                df_map->v_matgloss[matglossRawMapping[counter]].push_back(tmpstr);
                printf("%d = %s\n",i,tmpstr.c_str());
            }
        }
    }

    if(region_x_offset && region_y_offset && region_z_offset)
    {
        // we have offsets for region coordinates, get them.
        df_map->setRegionCoords(MreadDWord(region_x_offset),MreadDWord(region_y_offset),MreadDWord(region_z_offset));

        // extract layer geology data. we need all these to do that
        if(world_size_x && world_size_y && world_offset && world_regions_offset && world_geoblocks_offset && region_size && region_geo_index_offset && geolayer_geoblock_offset)
        {
            // get world size
            int worldSizeX = MreadWord(world_offset + world_size_x);
            int worldSizeY = MreadWord(world_offset + world_size_y);
            df_map->worldSizeX = worldSizeX;
            df_map->worldSizeY = worldSizeY;
            printf("World size. X=%d Y=%d\n",worldSizeX,worldSizeY);

            // get pointer to first part of 2d array of regions
            uint32_t regions = MreadDWord(world_offset + world_regions_offset);
            printf("regions. Offset=%d\n",regions);

            // read the 9 surrounding regions
            DfVector geoblocks = dm->readVector(world_offset + world_geoblocks_offset,4);

            // iterate over surrounding biomes. make sure we don't fall off the world
            for(int i = eNorthWest; i< eBiomeCount; i++)
            {
                // check bounds, fix them if needed
                int bioRX = df_map->regionX / 16 + (i%3) - 1;
                if( bioRX < 0) bioRX = 0;
                if( bioRX >= worldSizeX) bioRX = worldSizeX - 1;
                int bioRY = df_map->regionY / 16 + (i/3) - 1;
                if( bioRY < 0) bioRY = 0;
                if( bioRY >= worldSizeY) bioRY = worldSizeY - 1;

                /// TODO: encapsulate access to multidimensional arrays.
                // load region stuff here
                uint32_t geoX = MreadDWord(regions + bioRX*4);// get pointer to column of regions

                // geoX = base
                // bioRY = index
                // region_size = size of array objects
                // region_geo_index_off = offset into the array object
                uint16_t geoindex = MreadWord(geoX + bioRY*region_size + region_geo_index_offset);
                uint32_t geoblock_off;

                // get the geoblock from the geoblock vector using the geoindex
                geoblocks.read(geoindex,(uint8_t *) &geoblock_off);

                // get the layer pointer vector :D
                DfVector geolayers = dm->readVector(geoblock_off + geolayer_geoblock_offset , 4); // let's hope

                // make sure we don't load crap
                assert(geolayers.getSize() > 0 && geolayers.getSize() <= 16);
                for(uint32_t j = 0;j< geolayers.getSize();j++)
                {
                    int geol_offset;

                    // read pointer to a layer
                    geolayers.read(j, (uint8_t *) & geol_offset);

                    // read word at pointer + 2, store in our geology vectors
                    df_map->v_geology[i].push_back(MreadWord(geol_offset + 2));
                }
            }
            have_geology = true;
        }
    }
    else
    {
        // crap, can't get the real layer materials
        df_map->setRegionCoords(0,0,0);
    }

    //read the memory from the map blocks
    for(uint32_t x = 0; x < df_map->x_block_count; x++)
    {
        temp_locx = map_loc + ( 4 * x );
        temp_locy = MreadDWord(temp_locx);
        for(uint32_t y = 0; y < df_map->y_block_count; y++)
        {
            temp_locz = MreadDWord(temp_locy);
            temp_locy += 4;
            for(uint32_t z = 0; z < df_map->z_block_count; z++)
            {
                temp_loc = MreadDWord(temp_locz);
                temp_locz += 4;
                if (temp_loc)
                {
                    Block * b = df_map->allocBlock(x,y,z);
                    b->origin = temp_loc; // save place of block in DF's memory for later

                    Mread(
                    /*Uint32 offset*/ temp_loc + tile_type_offset,
                    /*Uint32 size*/   sizeof(uint16_t)*BLOCK_SIZE*BLOCK_SIZE,
                    /*void *target*/  (uint8_t *)&b->tile_type
                           );
                    Mread(
                    /*Uint32 offset*/ temp_loc + designation_offset,
                    /*Uint32 size*/   sizeof(uint32_t)*BLOCK_SIZE*BLOCK_SIZE,
                    /*void *target*/  (uint8_t *)&b->designation
                           );
                    Mread(
                    /*Uint32 offset*/ temp_loc + occupancy_offset,
                    /*Uint32 size*/   sizeof(uint32_t)*BLOCK_SIZE*BLOCK_SIZE,
                    /*void *target*/  (uint8_t *)&b->occupancy
                           );

                    // set all materials to -1.
                    memset(b->material, -1, sizeof(int16_t) * 256);
                    if(biome_stuffs) // we got biome stuffs! we can try loading matgloss from here
                    {
                        Mread(
                        /*Uint32 offset*/ temp_loc + biome_stuffs,
                        /*Uint32 size*/   sizeof(uint8_t)*16,
                        /*void *target*/  (uint8_t *)&b->RegionOffsets
                               );
                        // if we have geology, we can use the geolayers to determine materials
                        if(have_geology)
                        {
                            df_map->applyGeoMatgloss(b);
                        }
                    }
                    else
                    {
                        // can't load offsets, substitute local biome everywhere
                        memset(b->RegionOffsets,eHere,sizeof(b->RegionOffsets));
                    }
                    // load veins from the game
                    if(veinvector && veinsize)
                    {
                        assert(sizeof(t_vein) == veinsize);
                        // veins are stored as a vector of pointers to veins .. at least in df 40d11 on linux
                        /*pointer is 4 bytes! we work with a 32bit program here, no matter what architecture we compile khazad for*/
                        DfVector p_veins = dm->readVector(temp_loc + veinvector, 4);
                        // read all veins
                        for (uint32_t i = 0; i< p_veins.getSize();i++)
                        {
                            t_vein v;
                            uint32_t temp;
                            // read the vein pointer from the vector
                            p_veins.read((uint32_t)i,(uint8_t *)&temp);
                            // read the vein data (dereference pointer)
                            Mread(temp, veinsize, (uint8_t *)&v);
                            // store it in the block
                            b->veins.push_back(v);
                        }
                        b->collapseVeins(); // collapse *our* vein vector into vein matgloss data
                    }
                }
            }
        }
    }
    // read constructions, apply immediately.
    if(constructions)
    {
        // read the constructions vector
        DfVector p_cons = dm->readVector(constructions,4);
        // iterate
        for (uint32_t i = 0; i< p_cons.getSize();i++)
        {
            uint32_t temp;
            t_construction c;
            t_construction_df40d c_40d;
            // read pointer from vector at position
            p_cons.read((uint32_t)i,(uint8_t *)&temp);
            //read construction from memory
            Mread(temp, sizeof(t_construction_df40d), (uint8_t *)&c_40d);
            // stupid apply. this will probably be removed later
            Block * b = df_map->getBlock(c_40d.x/16,c_40d.y/16,c_40d.z);
            b->material[c_40d.x%16][c_40d.y%16] = c_40d.material;
            //b->material[c_40d.x%16][c_40d.y%16].index = c_40d.material.index;
            // transform
            c.x = c_40d.x;
            c.y = c_40d.y;
            c.z = c_40d.z;
            c.material = c_40d.material;
            // store for save/load
            df_map->v_constructions.push_back(c);
        }
    }
    if(buildings)
    {
        // fill the building type vector first
        offset_descriptor->copyBuildings(df_map->v_buildingtypes);
        DfVector p_bld = dm->readVector(buildings,4);
        for (uint32_t i = 0; i< p_bld.getSize();i++)
        {
            uint32_t temp;
            t_building * bld = new t_building;
            t_building_df40d bld_40d;
            // read pointer from vector at position
            p_bld.read((uint32_t)i,(uint8_t *)&temp);
            //read construction from memory
            Mread(temp, sizeof(t_building_df40d), (uint8_t *)&bld_40d);
            // transform
            int32_t type = -1;
            offset_descriptor->resolveClassId(temp, type);
            bld->vtable = bld_40d.vtable;
            bld->type = type;
            bld->x1 = bld_40d.x1;
            bld->x2 = bld_40d.x2;
            bld->y1 = bld_40d.y1;
            bld->y2 = bld_40d.y2;
            bld->z = bld_40d.z;
            bld->material = bld_40d.material;
            // store for save/load. will need more processing.
            df_map->v_buildings.push_back(bld);
            ///FIXME: delete created building structs
            // save buildings in a block for later display
            Block * b = df_map->getBlock(bld->x1/16,bld->y1/16,bld->z);
            b->v_buildings.push_back(bld);
        }
    }
    if(vegetation && tree_desc_offset)
    {
        DfVector p_tree = dm->readVector(vegetation,4);
        for (uint32_t i = 0; i< p_tree.getSize();i++)
        {
            uint32_t temp;
            t_tree_desc * tree = new t_tree_desc;
            // read pointer from vector at position
            p_tree.read((uint32_t)i,(uint8_t *)&temp);
            //read construction from memory
            Mread(temp + tree_desc_offset, sizeof(t_tree_desc), (uint8_t *)tree);
            // fix bad stuff
            if(tree->material.type == 2) tree->material.type = 3;
            // store for save/load. will need more processing.
            df_map->v_trees.push_back(tree);
            // save buildings in a block for later display
            Block * b = df_map->getBlock(tree->x/16,tree->y/16,tree->z);
            b->v_trees.push_back(tree);
        }
    }
    printf("Blocks read into memory: %d\n", blocks_read);
    p->detach();
    return true;
}
// wrappers!
bool Extractor::loadMap(string FileName)
{
    if(df_map == NULL)
    {
        df_map = new DfMap(FileName);
    }
    else
    {
        df_map->load(FileName);
    }
    return df_map->isValid();
}

bool Extractor::writeMap(string FileName)
{
    if(df_map == NULL)
    {
        return false;
    }
    return df_map->write(FileName);
}

bool Extractor::isMapLoaded()
{
    if(df_map != NULL)
    {
        if(df_map->isValid())
        {
            return true;
        }
    }
    return false;
}
