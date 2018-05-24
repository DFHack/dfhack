/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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

#pragma once
#ifndef MAPEXTRAS_H
#define MAPEXTRAS_H

#include "modules/Maps.h"
#include "TileTypes.h"
#include <stdint.h>
#include <cstring>
#include "df/map_block.h"
#include "df/map_block_column.h"
#include "df/tile_bitmask.h"
#include "df/block_square_event_mineralst.h"
#include "df/construction.h"
#include "df/item.h"
#include "df/inclusion_type.h"

namespace df {
    struct world_region_details;
}

namespace MapExtras
{

using namespace DFHack;

class DFHACK_EXPORT MapCache;

class Block;

struct BiomeInfo {
    // Determined by the 4-bit index in the designation bitfield
    static const unsigned MAX_LAYERS = 16;

    df::coord2d pos;
    int default_soil, default_stone, lava_stone;
    int geo_index;
    df::region_map_entry *biome;
    df::world_geo_biome *geobiome;
    df::world_region_details *details;
    int16_t layer_stone[MAX_LAYERS];
};

typedef uint8_t t_veintype[16][16];
typedef df::tiletype t_tilearr[16][16];

class BlockInfo
{
    Block *mblock;
    MapCache *parent;
    df::map_block *block;
    df::map_block_column *column; //for plants

public:
    enum GroundType {
        G_UNKNOWN = 0, G_STONE, G_SOIL
    };
    static GroundType getGroundType(int material);

    typedef df::block_square_event_mineralst::T_flags DFVeinFlags;

    t_veintype veintype;
    t_blockmaterials veinmats;
    t_blockmaterials grass;
    std::map<df::coord,df::plant*> plants;

    df::feature_init *global_feature;
    df::feature_init *local_feature;

    BlockInfo()
        : mblock(NULL), parent(NULL), block(NULL),
        global_feature(NULL), local_feature(NULL) {}

    void prepare(Block *mblock);

    t_matpair getBaseMaterial(df::tiletype tt, df::coord2d pos);

    static df::inclusion_type getVeinType(DFVeinFlags &flags);
    static void setVeinType(DFVeinFlags &flags, df::inclusion_type type);

    static void SquashVeins(df::map_block *mb, t_blockmaterials & materials, t_veintype &veintype);
    static void SquashFrozenLiquids (df::map_block *mb, tiletypes40d & frozen);
    static void SquashRocks (df::map_block *mb, t_blockmaterials & materials,
                             std::vector< std::vector <int16_t> > * layerassign);
    static void SquashGrass(df::map_block *mb, t_blockmaterials &materials);
};

class DFHACK_EXPORT Block
{
public:
    Block(MapCache *parent, DFCoord _bcoord);
    ~Block();

    DFCoord getCoord() { return bcoord; }

    void enableBlockUpdates(bool flow = false, bool temp = false) {
        Maps::enableBlockUpdates(block, flow, temp);
    }

    /*
     * All coordinates are taken mod 16.
     */

    /// Arbitrary tag field for flood fills etc.
    int16_t &tag(df::coord2d p) {
        if (!tags) init_tags();
        return index_tile<int16_t&>(tags, p);
    }

    /// Base layer tile type (i.e. layer stone, veins, feature stone)
    df::tiletype baseTiletypeAt(df::coord2d p)
    {
        if (!tiles) init_tiles();
        return index_tile<df::tiletype>(tiles->base_tiles,p);
    }
    /// Base layer material (i.e. layer stone, veins, feature stone)
    t_matpair baseMaterialAt(df::coord2d p)
    {
        if (!basemats) init_tiles(true);
        return t_matpair(
            index_tile<int16_t>(basemats->mat_type,p),
            index_tile<int16_t>(basemats->mat_index,p)
        );
    }
    /// Check if the base layer tile is a vein
    bool isVeinAt(df::coord2d p)
    {
        using namespace df::enums::tiletype_material;
        auto tm = tileMaterial(baseTiletypeAt(p));
        return tm == MINERAL;
    }
    /// Check if the base layer tile is layer stone or soil
    bool isLayerAt(df::coord2d p)
    {
        using namespace df::enums::tiletype_material;
        auto tm = tileMaterial(baseTiletypeAt(p));
        return tm == STONE || tm == SOIL;
    }

    /// Vein material at pos (even if there is no vein tile), or -1 if none
    int16_t veinMaterialAt(df::coord2d p)
    {
        if (!basemats) init_tiles(true);
        return index_tile<int16_t>(basemats->veinmat,p);
    }
    /// Vein type at pos (even if there is no vein tile)
    df::inclusion_type veinTypeAt(df::coord2d p)
    {
        if (!basemats) init_tiles(true);
        return (df::inclusion_type)index_tile<uint8_t>(basemats->veintype,p);
    }

    /** Sets the vein material at the specified tile position.
     *  Use -1 to clear the tile from all veins. Does not update tile types.
     *  Returns false in case of some error, e.g. non-stone mat.
     */
    bool setVeinMaterialAt(df::coord2d p, int16_t mat, df::inclusion_type type = df::enums::inclusion_type::CLUSTER);

    /// Geological layer soil or stone material at pos
    int16_t layerMaterialAt(df::coord2d p) {
        return biomeInfoAt(p).layer_stone[layerIndexAt(p)];
    }

    /// Biome-specific lava stone at pos
    int16_t lavaStoneAt(df::coord2d p) { return biomeInfoAt(p).lava_stone; }

    /**
     * Sets the stone tile and material at specified position, automatically
     * choosing between layer, lava or vein stone.
     * The force_type flags ensures the correct inclusion type, even forcing
     * a vein format if necessary. If kill_veins is true and the chosen mode
     * isn't vein, it will clear any old veins from the tile.
     */
    bool setStoneAt(df::coord2d p, df::tiletype tile, int16_t mat, df::inclusion_type type = df::enums::inclusion_type::CLUSTER, bool force_type = false, bool kill_veins = false);

    /**
     * Sets the tile at the position to SOIL material. The actual material
     * is completely determined by geological layers and cannot be set.
     */
    bool setSoilAt(df::coord2d p, df::tiletype tile, bool kill_veins = false);

    /// Static layer tile (i.e. base + constructions)
    df::tiletype staticTiletypeAt(df::coord2d p)
    {
        if (!tiles) init_tiles();
        if (tiles->con_info)
            return index_tile<df::tiletype>(tiles->con_info->tiles,p);
        return baseTiletypeAt(p);
    }
    /// Static layer material (i.e. base + constructions)
    t_matpair staticMaterialAt(df::coord2d p)
    {
        if (!basemats) init_tiles(true);
        if (tiles->con_info)
            return t_matpair(
                index_tile<int16_t>(tiles->con_info->mat_type,p),
                index_tile<int16_t>(tiles->con_info->mat_index,p)
            );
        return baseMaterialAt(p);
    }
    bool hasConstructionAt(df::coord2d p)
    {
        if (!tiles) init_tiles();
        return tiles->con_info &&
               tiles->con_info->constructed.getassignment(p);
    }

    df::tiletype tiletypeAt(df::coord2d p)
    {
        if (!block) return tiletype::Void;
        if (tiles)
            return index_tile<df::tiletype>(tiles->raw_tiles,p);
        return index_tile<df::tiletype>(block->tiletype,p);
    }
    bool setTiletypeAt(df::coord2d, df::tiletype tt, bool force = false);

    uint16_t temperature1At(df::coord2d p)
    {
        return index_tile<uint16_t>(temp1,p);
    }
    bool setTemp1At(df::coord2d p, uint16_t temp)
    {
        if(!valid) return false;
        dirty_temperatures = true;
        index_tile<uint16_t&>(temp1,p) = temp;
        return true;
    }

    uint16_t temperature2At(df::coord2d p)
    {
        return index_tile<uint16_t>(temp2,p);
    }
    bool setTemp2At(df::coord2d p, uint16_t temp)
    {
        if(!valid) return false;
        dirty_temperatures = true;
        index_tile<uint16_t&>(temp2,p) = temp;
        return true;
    }

    df::tile_designation DesignationAt(df::coord2d p)
    {
        return index_tile<df::tile_designation>(designation,p);
    }
    bool setDesignationAt(df::coord2d p, df::tile_designation des)
    {
        if(!valid) return false;
        dirty_designations = true;
        //printf("setting block %d/%d/%d , %d %d\n",x,y,z, p.x, p.y);
        index_tile<df::tile_designation&>(designation,p) = des;
        if(des.bits.dig && block)
            block->flags.bits.designated = true;
        return true;
    }

    int32_t priorityAt(df::coord2d p);
    bool setPriorityAt(df::coord2d p, int32_t priority);

    df::tile_occupancy OccupancyAt(df::coord2d p)
    {
        return index_tile<df::tile_occupancy>(occupancy,p);
    }
    bool setOccupancyAt(df::coord2d p, df::tile_occupancy des)
    {
        if(!valid) return false;
        dirty_occupancies = true;
        index_tile<df::tile_occupancy&>(occupancy,p) = des;
        return true;
    }

    bool getFlagAt(df::coord2d p, df::tile_designation::Mask mask) {
        return (index_tile<df::tile_designation&>(designation,p).whole & mask) != 0;
    }
    bool getFlagAt(df::coord2d p, df::tile_occupancy::Mask mask) {
        return (index_tile<df::tile_occupancy&>(occupancy,p).whole & mask) != 0;
    }
    bool setFlagAt(df::coord2d p, df::tile_designation::Mask mask, bool set);
    bool setFlagAt(df::coord2d p, df::tile_occupancy::Mask mask, bool set);

    int itemCountAt(df::coord2d p)
    {
        if (!item_counts) init_item_counts();
        return index_tile<int>(item_counts,p);
    }

    t_blockflags BlockFlags()
    {
        return block ? block->flags : t_blockflags();
    }

    bool Write();
    bool isDirty();

    int biomeIndexAt(df::coord2d p);
    int layerIndexAt(df::coord2d p) {
        return index_tile<df::tile_designation&>(designation,p).bits.geolayer_index;
    }

    df::coord2d biomeRegionAt(df::coord2d p);
    const BiomeInfo &biomeInfoAt(df::coord2d p);
    int16_t GeoIndexAt(df::coord2d p) { return biomeInfoAt(p).geo_index; }

    bool GetGlobalFeature(t_feature *out);
    bool GetLocalFeature(t_feature *out);

    bool is_valid() { return valid; }
    df::map_block *getRaw() { return block; }

    bool Allocate();

    MapCache *getParent() { return parent; }

private:
    friend class MapCache;
    friend class BlockInfo;

    MapCache *parent;
    df::map_block *block;

    void init();

    bool valid;
    bool dirty_designations:1;
    bool dirty_tiles:1;
    bool dirty_veins:1;
    bool dirty_temperatures:1;
    bool dirty_occupancies:1;

    DFCoord bcoord;

    // Custom tags for floodfill
    typedef int16_t T_tags[16];
    T_tags *tags;
    void init_tags();

    // On-ground item count info
    typedef int T_item_counts[16];
    T_item_counts *item_counts;
    void init_item_counts();

    bool addItemOnGround(df::item *item);
    bool removeItemOnGround(df::item *item);

    struct IceInfo {
        df::tile_bitmask frozen;
        df::tile_bitmask dirty;
    };
    struct ConInfo {
        df::tile_bitmask constructed;
        df::tile_bitmask dirty;
        t_tilearr tiles;
        t_blockmaterials mat_type;
        t_blockmaterials mat_index;
    };
    struct TileInfo {
        df::tile_bitmask dirty_raw;
        t_tilearr raw_tiles;

        IceInfo *ice_info;
        ConInfo *con_info;

        t_tilearr base_tiles;

        TileInfo();
        ~TileInfo();

        void init_iceinfo();
        void init_coninfo();

        void set_base_tile(df::coord2d pos, df::tiletype tile);
    };
    struct BasematInfo {
        t_blockmaterials mat_type;
        t_blockmaterials mat_index;

        df::tile_bitmask vein_dirty;
        t_veintype       veintype;
        t_blockmaterials veinmat;

        BasematInfo();

        void set_base_mat(TileInfo *tiles, df::coord2d pos, int16_t type, int16_t idx);
    };
    TileInfo *tiles;
    BasematInfo *basemats;
    void init_tiles(bool basemat = false);
    void ParseTiles(TileInfo *tiles);
    void WriteTiles(TileInfo*);
    void ParseBasemats(TileInfo *tiles, BasematInfo *bmats);
    void WriteVeins(TileInfo *tiles, BasematInfo *bmats);

    designations40d designation;
    occupancies40d occupancy;

    t_temperatures temp1;
    t_temperatures temp2;
};

class DFHACK_EXPORT MapCache
{
    public:
    MapCache();
    ~MapCache()
    {
        trash();
    }
    bool isValid ()
    {
        return valid;
    }

    /// get the map block at a *block* coord. Block coord = tile coord / 16
    Block *BlockAt(DFCoord blockcoord);
    /// get the map block at a tile coord.
    Block *BlockAtTile(DFCoord coord) {
        return BlockAt(df::coord(coord.x>>4,coord.y>>4,coord.z));
    }

    bool ensureBlockAt(df::coord coord)
    {
        Block *b = BlockAtTile(coord);
        return b ? b->Allocate() : false;
    }

    /// delete the block from memory
    void discardBlock(Block *block);

    df::tiletype baseTiletypeAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b ? b->baseTiletypeAt(tilecoord) : tiletype::Void;
    }
    t_matpair baseMaterialAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b ? b->baseMaterialAt(tilecoord) : t_matpair();
    }
    int16_t veinMaterialAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b ? b->veinMaterialAt(tilecoord) : -1;
    }
    int16_t layerMaterialAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b ? b->layerMaterialAt(tilecoord) : -1;
    }
    bool isVeinAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b && b->isVeinAt(tilecoord);
    }
    bool isLayerAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b && b->isLayerAt(tilecoord);
    }

    df::tiletype staticTiletypeAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b ? b->staticTiletypeAt(tilecoord) : tiletype::Void;
    }
    t_matpair staticMaterialAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b ? b->staticMaterialAt(tilecoord) : t_matpair();
    }
    bool hasConstructionAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b && b->hasConstructionAt(tilecoord);
    }

    df::tiletype tiletypeAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b ? b->tiletypeAt(tilecoord) : tiletype::Void;
    }
    bool setTiletypeAt (DFCoord tilecoord, df::tiletype tt, bool force = false)
    {
        Block *b = BlockAtTile(tilecoord);
        return b && b->setTiletypeAt(tilecoord, tt, force);
    }

    uint16_t temperature1At (DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return b ? b->temperature1At(tilecoord) : 0;
    }
    bool setTemp1At(DFCoord tilecoord, uint16_t temperature)
    {
        if (Block * b= BlockAtTile(tilecoord))
            return b->setTemp1At(tilecoord, temperature);
        return false;
    }

    uint16_t temperature2At (DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return b ? b->temperature2At(tilecoord) : 0;
    }
    bool setTemp2At(DFCoord tilecoord, uint16_t temperature)
    {
        if (Block * b= BlockAtTile(tilecoord))
            return b->setTemp2At(tilecoord, temperature);
        return false;
    }

    int16_t tagAt(DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return b ? b->tag(tilecoord) : 0;
    }
    void setTagAt(DFCoord tilecoord, int16_t val)
    {
        Block * b= BlockAtTile(tilecoord);
        if (b) b->tag(tilecoord) = val;
    }
    void resetTags();

    df::tile_designation designationAt (DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return b ? b->DesignationAt(tilecoord) : df::tile_designation();
    }
    // priority is optional, only set if >= 0
    bool setDesignationAt (DFCoord tilecoord, df::tile_designation des, int32_t priority = -1)
    {
        if (Block *b = BlockAtTile(tilecoord))
        {
            if (!b->setDesignationAt(tilecoord, des))
                return false;
            if (priority >= 0 && b->setPriorityAt(tilecoord, priority))
                return false;
            return true;
        }
        return false;
    }

    int32_t priorityAt (DFCoord tilecoord)
    {
        Block *b = BlockAtTile(tilecoord);
        return b ? b->priorityAt(tilecoord) : -1;
    }
    bool setPriorityAt (DFCoord tilecoord, int32_t priority)
    {
        Block *b = BlockAtTile(tilecoord);
        return b ? b->setPriorityAt(tilecoord, priority) : false;
    }

    df::tile_occupancy occupancyAt (DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return b ? b->OccupancyAt(tilecoord) : df::tile_occupancy();
    }
    bool setOccupancyAt (DFCoord tilecoord, df::tile_occupancy occ)
    {
        if (Block * b= BlockAtTile(tilecoord))
            return b->setOccupancyAt(tilecoord, occ);
        return false;
    }

    bool testCoord (DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return (b && b->valid);
    }

    bool addItemOnGround(df::item *item) {
        Block * b= BlockAtTile(item->pos);
        return b ? b->addItemOnGround(item) : false;
    }
    bool removeItemOnGround(df::item *item) {
        Block * b= BlockAtTile(item->pos);
        return b ? b->removeItemOnGround(item) : false;
    }

    bool WriteAll()
    {
        std::map<DFCoord, Block *>::iterator p;
        for(p = blocks.begin(); p != blocks.end(); p++)
        {
            p->second->Write();
        }
        return true;
    }
    void trash()
    {
        std::map<DFCoord, Block *>::iterator p;
        for(p = blocks.begin(); p != blocks.end(); p++)
        {
            delete p->second;
        }
        blocks.clear();
    }

    uint32_t maxBlockX() { return x_bmax; }
    uint32_t maxBlockY() { return y_bmax; }
    uint32_t maxTileX() { return x_tmax; }
    uint32_t maxTileY() { return y_tmax; }
    uint32_t maxZ() { return z_max; }

    size_t getBiomeCount() { return biomes.size(); }
    const BiomeInfo &getBiomeByIndex(unsigned idx) {
        return (idx < biomes.size()) ? biomes[idx] : biome_stub;
    }

private:
    friend class Block;
    friend class BlockInfo;

    static const BiomeInfo biome_stub;

    bool valid;
    bool validgeo;
    uint32_t x_bmax;
    uint32_t y_bmax;
    uint32_t x_tmax;
    uint32_t y_tmax;
    uint32_t z_max;
    std::vector<BiomeInfo> biomes;
    std::map<df::coord2d, df::world_region_details*> region_details;
    std::map<DFCoord, Block *> blocks;
};
}
#endif
