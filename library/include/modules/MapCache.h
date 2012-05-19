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

#pragma once
#ifndef MAPEXTRAS_H
#define MAPEXTRAS_H

#include "modules/Maps.h"
#include "TileTypes.h"
#include <stdint.h>
#include <cstring>
#include "df/map_block.h"
#include "df/tile_bitmask.h"
#include "df/block_square_event_mineralst.h"
#include "df/construction.h"
#include "df/item.h"

using namespace DFHack;

namespace df {
    struct world_region_details;
}

namespace MapExtras
{

class DFHACK_EXPORT MapCache;

template<class R, class T> inline R index_tile(T &v, df::coord2d p) {
    return v[p.x&15][p.y&15];
}

inline bool is_valid_tile_coord(df::coord2d p) {
    return (p.x & ~15) == 0 && (p.y & ~15) == 0;
}

class Block;

class BlockInfo
{
    Block *mblock;
    MapCache *parent;
    df::map_block *block;

public:
    t_blockmaterials veinmats;
    t_blockmaterials basemats;
    t_blockmaterials grass;
    std::map<df::coord,df::plant*> plants;

    df::feature_init *global_feature;
    df::feature_init *local_feature;

    BlockInfo()
        : mblock(NULL), parent(NULL), block(NULL),
        global_feature(NULL), local_feature(NULL) {}

    void prepare(Block *mblock);

    t_matpair getBaseMaterial(df::tiletype tt, df::coord2d pos);

    static void SquashVeins(df::map_block *mb, t_blockmaterials & materials);
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

    //Arbitrary tag field for flood fills etc.
    int16_t &tag(df::coord2d p) {
        if (!tags) init_tags();
        return index_tile<int16_t&>(tags, p);
    }

    // Base layer
    df::tiletype baseTiletypeAt(df::coord2d p)
    {
        if (!tiles) init_tiles();
        return index_tile<df::tiletype>(tiles->base_tiles,p);
    }
    t_matpair baseMaterialAt(df::coord2d p)
    {
        if (!basemats) init_tiles(true);
        return t_matpair(
            index_tile<int16_t>(basemats->mattype,p),
            index_tile<int16_t>(basemats->matindex,p)
        );
    }
    bool isVeinAt(df::coord2d p)
    {
        using namespace df::enums::tiletype_material;
        auto tm = tileMaterial(baseTiletypeAt(p));
        return tm == MINERAL;
    }
    bool isLayerAt(df::coord2d p)
    {
        using namespace df::enums::tiletype_material;
        auto tm = tileMaterial(baseTiletypeAt(p));
        return tm == STONE || tm == SOIL;
    }

    int16_t veinMaterialAt(df::coord2d p)
    {
        return isVeinAt(p) ? baseMaterialAt(p).mat_index : -1;
    }
    int16_t layerMaterialAt(df::coord2d p)
    {
        if (!basemats) init_tiles(true);
        return index_tile<int16_t>(basemats->layermat,p);
    }

    // Static layer (base + constructions)
    df::tiletype staticTiletypeAt(df::coord2d p)
    {
        if (!tiles) init_tiles();
        if (tiles->con_info)
            return index_tile<df::tiletype>(tiles->con_info->tiles,p);
        return baseTiletypeAt(p);
    }
    t_matpair staticMaterialAt(df::coord2d p)
    {
        if (!basemats) init_tiles(true);
        if (tiles->con_info)
            return t_matpair(
                index_tile<int16_t>(tiles->con_info->mattype,p),
                index_tile<int16_t>(tiles->con_info->matindex,p)
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

    df::coord2d biomeRegionAt(df::coord2d p);
    int16_t GeoIndexAt(df::coord2d p);

    bool GetGlobalFeature(t_feature *out);
    bool GetLocalFeature(t_feature *out);

    bool is_valid() { return valid; }
    df::map_block *getRaw() { return block; }

    MapCache *getParent() { return parent; }

private:
    friend class MapCache;
    friend class BlockInfo;

    MapCache *parent;
    df::map_block *block;

    int biomeIndexAt(df::coord2d p);

    bool valid;
    bool dirty_designations:1;
    bool dirty_tiles:1;
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

    struct ConInfo {
        df::tile_bitmask constructed;
        df::tiletype tiles[16][16];
        t_blockmaterials mattype;
        t_blockmaterials matindex;
    };
    struct TileInfo {
        df::tile_bitmask frozen;
        df::tile_bitmask dirty_raw;
        df::tiletype raw_tiles[16][16];

        ConInfo *con_info;

        df::tile_bitmask dirty_base;
        df::tiletype base_tiles[16][16];

        TileInfo();
        ~TileInfo();

        void init_coninfo();
    };
    struct BasematInfo {
        df::tile_bitmask dirty;
        t_blockmaterials mattype;
        t_blockmaterials matindex;
        t_blockmaterials layermat;

        BasematInfo();
    };
    TileInfo *tiles;
    BasematInfo *basemats;
    void init_tiles(bool basemat = false);
    void ParseTiles(TileInfo *tiles);
    void ParseBasemats(TileInfo *tiles, BasematInfo *bmats);

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
        return b ? b->DesignationAt(tilecoord) : df::tile_designation(0);
    }
    bool setDesignationAt (DFCoord tilecoord, df::tile_designation des)
    {
        if(Block * b= BlockAtTile(tilecoord))
            return b->setDesignationAt(tilecoord, des);
        return false;
    }

    df::tile_occupancy occupancyAt (DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return b ? b->OccupancyAt(tilecoord) : df::tile_occupancy(0);
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

private:
    friend class Block;
    friend class BlockInfo;

    bool valid;
    bool validgeo;
    uint32_t x_bmax;
    uint32_t y_bmax;
    uint32_t x_tmax;
    uint32_t y_tmax;
    uint32_t z_max;
    std::vector<df::coord2d> geoidx;
    std::vector<int> default_soil;
    std::vector<int> default_stone;
    std::vector< std::vector <int16_t> > layer_mats;
    std::map<df::coord2d, df::world_region_details*> region_details;
    std::map<DFCoord, Block *> blocks;
};
}
#endif