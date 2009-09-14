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

#include "DFCommon.h"
// zlib helper functions for de/compressing files
#include "ZlibHelper.h"
#include "DfMapHeader.h"

// some bounds checking in debug mode. used in asserts
#define CheckBounds x < x_cell_count && x >= 0 && y < y_cell_count && y >= 0 && z < z_block_count && z >= 0
#define CheckBoundsXY x < x_cell_count && x >= 0 && y < y_cell_count && y >= 0
#define CheckBlockBounds x < x_block_count && x >= 0 && y < y_block_count && y >= 0 && z < z_block_count && z >= 0

// this expands into lots of ugly switch statement functions. some of them unused?, but kept for reference
#include "DFTileTypes.h"

// process vein vector into matgloss values...
void Block::collapseVeins()
{
    // iterate through assigned veins
    for( uint32_t i = 0; i < veins.size(); i++)
    {
        t_vein v = veins[i];
        //iterate through vein assignment bit arrays - one for every row
        for(uint32_t j = 0;j<16;j++)
        {
            //iterate through the bits
            for (uint32_t k = 0; k< 16;k++)
            {
                // and the bit array with a one-bit mask, check if the bit is set
                bool set = ((1 << k) & v.assignment[j]) >> k;
                if(set)
                {
                    material[k][j].type = Mat_Stone;
                    material[k][j].index = v.type;
                }
            }
        }
    }
}


DfMap::~DfMap()
{
    clear();
}


/// TODO: make this sync
void DfMap::clear()
{
    if(valid)
    {
        valid = false;
        for(uint32_t i = 0; i < x_block_count*y_block_count*z_block_count;i++)
        {
            Block * b = block[i];
            if(b!=NULL)
            {
                delete b;
            }
        }
        delete[] block;
    }
    for (uint32_t i = eNorthWest; i< eBiomeCount;i++)
    {
        v_geology[i].clear();
        //geodebug[i].clear();
        //geoblockadresses[i] = 0;
        //regionadresses[i] = 0;
    }
    for(uint32_t counter = Mat_Wood; counter < NUM_MATGLOSS_TYPES; counter++)
    {
        v_matgloss[counter].clear();
    }
    // delete buildings, clear vector
    for(uint32_t i = 0; i < v_buildings.size(); i++)
    {
        delete v_buildings[i];
    }
    v_buildings.clear();

    // delete vegetation, clear vector
    for(uint32_t i = 0; i < v_trees.size(); i++)
    {
        delete v_trees[i];
    }
    v_trees.clear();
    // clear construction vector
    v_constructions.clear();
    blocks_allocated = 0;
    ///FIXME: destroy all the extracted data here
}


void DfMap::getRegionCoords (uint32_t &x,uint32_t &y,uint32_t &z)
{
    x= regionX;
    y= regionY;
    z= regionZ;
}


void DfMap::setRegionCoords (uint32_t x,uint32_t y,uint32_t z)
{
    regionX = x;
    regionY = y;
    regionZ = z;
}


void DfMap::allocBlockArray(uint32_t x,uint32_t y, uint32_t z)
{
    clear();
    x_block_count = x;
    y_block_count = y;
    z_block_count = z;
    updateCellCount();
    block = new Block*[x_block_count*y_block_count*z_block_count];
    for (uint32_t i = 0; i < x_block_count*y_block_count*z_block_count; i++ )
    {
        block[i] = NULL;
    }
    blocks_allocated = 0;
    valid = true;
}


DfMap::DfMap(uint32_t x, uint32_t y, uint32_t z)
{
    valid = false;
    allocBlockArray(x,y,z);
}


DfMap::DfMap(string FileName)
{
    valid = false;
    valid = load( FileName);
}


bool DfMap::isValid ()
{
    return valid;
}


Block * DfMap::getBlock (uint32_t x,uint32_t y,uint32_t z)
{
    if(isValid())
    {
        return block[x*y_block_count*z_block_count + y*z_block_count + z];
    }
    return NULL;
}


vector<t_building *> * DfMap::getBlockBuildingsVector(uint32_t x,uint32_t y,uint32_t z)
{
    Block * b = getBlock(x,y,z);
    if(b)
    {
        return &b->v_buildings;
    }
    return NULL;
}


vector<t_tree_desc *> *  DfMap::getBlockVegetationVector(uint32_t x,uint32_t y,uint32_t z)
{
    Block * b = getBlock(x,y,z);
    if(b)
    {
        return &b->v_trees;
    }
    return NULL;
}


t_tree_desc *DfMap::getTree (uint32_t x, uint32_t y, uint32_t z)
{
    for(uint32_t i = 0; i< v_trees.size();i++)
    {
        if(x == v_trees[i]->x
        && y == v_trees[i]->y
        && z == v_trees[i]->z)
        {
            return v_trees[i];
        }
    }
    return 0;
}


t_building *DfMap::getBuilding (uint32_t x, uint32_t y, uint32_t z)
{
    for(uint32_t i = 0; i< v_buildings.size();i++)
    {
        if(x >= v_buildings[i]->x1 && x <= v_buildings[i]->x2
        && y >= v_buildings[i]->y1 && y <= v_buildings[i]->y2
        && z == v_buildings[i]->z)
        {
            return v_buildings[i];
        }
    }
    return 0;
}


Block * DfMap::allocBlock (uint32_t x,uint32_t y,uint32_t z)
{
    if(isValid())
    {
        if(block[x*y_block_count*z_block_count + y*z_block_count + z])
        {
            return block[x*y_block_count*z_block_count + y*z_block_count + z];
        }
        Block *b  = new Block;
        block[x*y_block_count*z_block_count + y*z_block_count + z] = b;
        blocks_allocated++;
        return b;
    }
    return NULL;
}


void DfMap::updateCellCount()
{
    x_cell_count = x_block_count * BLOCK_SIZE;
    y_cell_count = y_block_count * BLOCK_SIZE;
    z_cell_count = z_block_count;
}


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


uint8_t DfMap::getLiquidLevel(uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);

    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return b->designation[x2][y2].bits.flow_size;
    }
    return 0;
}


uint16_t DfMap::getTileType(uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);

    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return b->tile_type[x2][y2];
    }
    if(isTileSky(x,y,z,x2,y2))
    {
        return 32;
    }
    return -1;
}


uint16_t DfMap::getTileType(uint32_t x, uint32_t y, uint32_t z, uint32_t blockX, uint32_t blockY)
{
    assert(CheckBlockBounds);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return b->tile_type[blockX][blockY];
    }
    if(isTileSky(x,y,z,blockX,blockY))
    {
        return 32;
    }
    return -1;
}


uint32_t DfMap::getDesignations(uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);
    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return b->designation[x2][y2].whole;
    }
    return -1;
}


bool DfMap::isBlockInitialized(uint32_t x, uint32_t y, uint32_t z)
{
    // because of the way DfMap is done, more than one check must be made.
    return getBlock(x,y,z) != NULL;
}


uint32_t DfMap::getOccupancies(uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);

    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return b->occupancy[x2][y2].whole;
    }
    return -1;
}


void DfMap::getGeoRegion (uint32_t x, uint32_t y, uint32_t z, int32_t& geoX, int32_t& geoY)
{
    assert(CheckBoundsXY);
    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        int biome = b->designation[x2][y2].bits.biome;
        int BiomeOffset = b->RegionOffsets[biome];
        int16_t X_biomeB = (regionX / 16) + (BiomeOffset % 3) - 1;
        int16_t Y_biomeB = (regionY / 16) + (BiomeOffset / 3) - 1;
        if(X_biomeB < 0) X_biomeB = 0;
        if(Y_biomeB < 0) Y_biomeB = 0;
        if( (uint32_t)X_biomeB >= worldSizeX)
        {
            X_biomeB = worldSizeX - 1;
        }
        if( (uint32_t)Y_biomeB >= worldSizeY)
        {
            Y_biomeB = worldSizeY - 1;
        }
        geoX = X_biomeB;
        geoY = Y_biomeB;
    }
    else
    {
        geoX = regionX / 16;
        geoY = regionY / 16;
    }
}


t_matglossPair DfMap::getMaterialPair (uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);

    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return b->material[x2][y2];
    }
    t_matglossPair fail = {-1,-1};
    return fail;
};


// this is what the vein structures say it is
string DfMap::getGeoMaterialString (uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);

    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return getMaterialString(b->material[x2][y2].type, b->material[x2][y2].index);
    }
    string fallback = "UNKNOWN";
    return fallback;
}


string DfMap::getMaterialTypeString (uint32_t type)
{
    string ret = "";
    switch (type)
    {
        case 0:
        ret += "wood";
        break;
        case 1:
        ret += "stone/soil";
        break;
        case 2:
        ret += "metal";
        break;
        case 3:
        ret += "plant";
        break;
        case 10:
        ret += "leather";
        break;
        case 11:
        ret += "silk cloth";
        break;
        case 12:
        ret += "plant thread cloth";
        break;
        case 13: // green glass
        ret += "green glass";
        break;
        case 14: // clear glass
        ret += "clear glass";
        break;
        case 15: // crystal glass
        ret += "crystal glass";
        break;
        case 17:
        ret += "ice";
        break;
        case 18:
        ret += "charcoal";
        break;
        case 19:
        ret += "potash";
        break;
        case 20:
        ret += "ashes";
        break;
        case 21:
        ret += "pearlash";
        break;
        case 24:
        ret += "soap";
        break;
        default:
        ret += "unknown";
        break;
    }
    return ret;
}


string DfMap::getMaterialString (uint32_t type, uint32_t index)
{
    if(index != 65535 && type >= 0 && type < NUM_MATGLOSS_TYPES)
    {
        if(index < v_matgloss[type].size())
        {
            return v_matgloss[type][index];
        }
        else
        {
            string fallback = "ERROR";
            return fallback;
        }
    }
    string fallback = "UNKNOWN";
    return fallback;
}


uint16_t DfMap::getNumMatGloss(uint16_t type)
{
    return v_matgloss[type].size();
}


string DfMap::getBuildingTypeName(uint32_t index)
{
    if(index < v_buildingtypes.size())
    {
        return v_buildingtypes[index];
    }
    return string("error");
}


string DfMap::getMatGlossString(uint16_t type,uint16_t index)
{
    if(index < v_matgloss[type].size())
    {
        return v_matgloss[type][index];
    }
    return string("error");
}


// matgloss part of the designation
unsigned int DfMap::getGeolayerIndex (uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);

    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return b->designation[x2][y2].bits.geolayer_index;
    }
    return -1;
}


// matgloss part of the designation
unsigned int DfMap::getBiome (uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);

    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return b->designation[x2][y2].bits.biome;
    }
    return -1;
}


bool DfMap::isHidden (uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);
    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return (b->designation[x2][y2].bits.hidden);
    }
    return false;
}


bool DfMap::isSubterranean (uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);
    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return (b->designation[x2][y2].bits.subterranean);
    }
    if(isTileSky( x, y, z, x2, y2))
    {
        return false;
    }
    return true;
}


// x,y,z - coords of block
// blockX,blockY - coords of tile inside block
bool DfMap::isTileSky(uint32_t x, uint32_t y, uint32_t z, uint32_t blockX, uint32_t blockY)
{
    assert(CheckBounds);
    Block *b;
    // trace down through blocks until we hit an inited one or the base
    for (int i = z; i>= 0;i--)
    {
        b = getBlock(x,y,i);
        if(b)
        {
            // is the inited block open to the sky?
            return b->designation[blockX][blockY].bits.skyview;
        }
    }
    // we hit base
    return false;
}


// is the sky above this tile visible?
bool DfMap::isSkyView (uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);
    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return (b->designation[x2][y2].bits.skyview);
    }
    if(isTileSky(x,y,z,x2,y2))
    {
        return true;
    }
    return false;
}


// is there light in this tile?
bool DfMap::isSunLit (uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);
    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return (b->designation[x2][y2].bits.light);
    }
    return false;
}


bool DfMap::isMagma (uint32_t x, uint32_t y, uint32_t z)
{
    assert(CheckBounds);
    uint32_t x2, y2;
    convertToDfMapCoords(x, y, x, y, x2, y2);
    Block *b = getBlock(x,y,z);
    if(b != NULL)
    {
        return (b->designation[x2][y2].bits.liquid_type);
    }
    return false;
}
