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

#ifndef DFMAP_H_INCLUDED
#define DFMAP_H_INCLUDED

#define BLOCK_SIZE 16

class DfMapHeader;

class Block
{
public:
    // where does the Block come from?
    uint32_t origin;
    // generic tile type. determines how the tile behaves ingame
    uint16_t tile_type[BLOCK_SIZE][BLOCK_SIZE];
    t_designation designation[BLOCK_SIZE][BLOCK_SIZE];
    t_occupancy occupancy[BLOCK_SIZE][BLOCK_SIZE];
    // veins
    vector <t_vein> veins;
    t_matglossPair material[BLOCK_SIZE][BLOCK_SIZE];
    vector<t_building*> v_buildings;
    vector<t_tree_desc*> v_trees;
    void collapseVeins();
    /**
    // region offset modifiers... what a hack.
    // here we have double indexed offset into regions.
    // once inside t_designation, pointing into this, second time from here as a index modifier into region array (2d)
    // disassembled code where it's used follows. biome is biome from t_designation
    biome_stuffs = *(_BYTE *)((char)biome + offset_Block + 0x1D84);
    biome_stuffs_mod3 = biome_stuffs % 3;
    biome_stuffs_div3 = biome_stuffs / 3;
    biome_stuffs_mod3_ = biome_stuffs_mod3;
    if ( !biome_stuffs_mod3_ )
      --*(_WORD *)X_stuff;
    if ( biome_stuffs_mod3_ == 2 )
      ++*(_WORD *)X_stuff;
    if ( !biome_stuffs_div3 )
      --*(_WORD *)Y_stuff_;
    if ( biome_stuffs_div3 == 2 )
      ++*(_WORD *)Y_stuff_;
    */
    uint8_t RegionOffsets[16];// idk if the length is right here
};
/**
 * This class can load and save DF maps
 */
class DfMap
{
private:
    // allow extractor direct access to our data, avoid call lag and lots of self-serving methods
    friend class Extractor;

    Block **block;
    uint32_t blocks_allocated;
    bool valid;

    // converts the (x,y,z) cell coords to internal coords
    // out_y, out_x - block coords
    // out_y2, out_x2 - cell coords in that block
    inline void convertToDfMapCoords(uint32_t x, uint32_t y, uint32_t &out_x, uint32_t &out_y, uint32_t &out_x2, uint32_t &out_y2)
    {
        out_x = x / BLOCK_SIZE;
        out_x2 = x % BLOCK_SIZE;
        out_y = y / BLOCK_SIZE;
        out_y2 = y % BLOCK_SIZE;
    };

    void allocBlockArray(uint32_t x,uint32_t y, uint32_t z);
    void updateCellCount();

    bool loadVersion1(FILE * Decompressed,DfMapHeader & h);
    bool writeVersion1(FILE * SaveFile);

    bool loadMatgloss2(FILE * Decompressed);
    bool loadBlocks2(FILE * Decompressed,DfMapHeader & h);
    bool loadRegion2(FILE * Decompressed);
    bool loadVersion2(FILE * Decompressed,DfMapHeader & h);

    void writeMatgloss2(FILE * SaveFile);
    void writeBlocks2(FILE * SaveFile);
    void writeRegion2(FILE * SaveFile);
    bool writeVersion2(FILE * SaveFile);

    uint32_t regionX;
    uint32_t regionY;
    uint32_t regionZ;

    ///FIXME: these belong to some world structure
    uint32_t worldSizeX;
    uint32_t worldSizeY;

    vector<uint16_t> v_geology[eBiomeCount];
    vector<string> v_matgloss[NUM_MATGLOSS_TYPES];
    vector<string> v_buildingtypes;
    vector<t_construction> v_constructions;
    vector<t_building*> v_buildings;
    vector<t_tree_desc*> v_trees;
    unsigned x_block_count, y_block_count, z_block_count; // block count
    unsigned x_cell_count, y_cell_count, z_cell_count;    // cell count

public:
    DfMap();
    DfMap(uint32_t x, uint32_t y, uint32_t z);
    DfMap(string file_name);
    ~DfMap();

    /// TODO: rework matgloss
    void applyGeoMatgloss(Block * b);
    // accessing vectors of materials
    uint16_t getNumMatGloss(uint16_t type);
    string getMaterialTypeString (uint32_t type);
    string getMatGlossString(uint16_t type, uint16_t index);
    // accessing vectors of building types
    uint32_t getNumBuildingTypes();
    string getBuildingTypeName(uint32_t index);

    bool isValid();
    bool load(string FilePath);
    bool write(string FilePath);
    void clear();

    Block* getBlock(uint32_t x, uint32_t y, uint32_t z);
    Block* allocBlock(uint32_t x, uint32_t y, uint32_t z);
    bool   deallocBlock(uint32_t x, uint32_t y, uint32_t z);

    vector<t_building *> * getBlockBuildingsVector(uint32_t x,uint32_t y,uint32_t z);
    vector<t_tree_desc *> * getBlockVegetationVector(uint32_t x,uint32_t y,uint32_t z);

    inline unsigned int getXBlocks()        { return x_block_count; }
    inline unsigned int getYBlocks()        { return y_block_count; }
    inline unsigned int getZBlocks()        { return z_block_count; }

    bool isTileSky(uint32_t x, uint32_t y, uint32_t z, uint32_t blockX, uint32_t blockY);
    uint16_t getTileType(uint32_t x, uint32_t y, uint32_t z);
    uint16_t getTileType(uint32_t x, uint32_t y, uint32_t z, uint32_t blockX, uint32_t blockY);

    uint32_t getDesignations(uint32_t x, uint32_t y, uint32_t z);
    uint32_t getOccupancies(uint32_t x, uint32_t y, uint32_t z);

    // get tile material
    t_matglossPair getMaterialPair (uint32_t x, uint32_t y, uint32_t z);
    string getGeoMaterialString (uint32_t x, uint32_t y, uint32_t z);
    string getMaterialString (uint32_t type, uint32_t index);

    // get coords of region used for materials
    void getGeoRegion (uint32_t x, uint32_t y, uint32_t z, int32_t& geoX, int32_t& geoY);

    // matgloss part of the designation
    uint32_t getGeolayerIndex (uint32_t x, uint32_t y, uint32_t z);

    void getRegionCoords (uint32_t &x,uint32_t &y,uint32_t &z);
    void setRegionCoords (uint32_t x,uint32_t y,uint32_t z);

    // what kind of building is here?
    //uint16_t getBuilding (uint32_t x, uint32_t y, uint32_t z);
    t_building *getBuilding (uint32_t x, uint32_t y, uint32_t z);
    t_tree_desc *getTree (uint32_t x, uint32_t y, uint32_t z);

    unsigned int getBiome (uint32_t x, uint32_t y, uint32_t z);

    int picktexture(int);
/*
    bool isOpenTerrain(int);
    bool isStairTerrain(int);
    bool isRampTerrain(int);
    bool isFloorTerrain(int);
    bool isWallTerrain(int);
*/
    bool isBlockInitialized(uint32_t x, uint32_t y, uint32_t z);

    bool isHidden (uint32_t x, uint32_t y, uint32_t z);
    bool isSubterranean (uint32_t x, uint32_t y, uint32_t z);
    bool isSkyView (uint32_t x, uint32_t y, uint32_t z);
    bool isSunLit (uint32_t x, uint32_t y, uint32_t z);
    bool isMagma (uint32_t x, uint32_t y, uint32_t z);

    uint8_t getLiquidLevel(uint32_t x, uint32_t y, uint32_t z);
};


#endif // DFMAP_H_INCLUDED
