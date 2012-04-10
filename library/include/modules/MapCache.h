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
#include "df/block_square_event_mineralst.h"
#include "df/construction.h"
#include "df/item.h"

using namespace DFHack;

namespace MapExtras
{

class DFHACK_EXPORT MapCache;

template<class R, class T> inline R index_tile(T &v, df::coord2d p) {
    return v[p.x&15][p.y&15];
}

inline bool is_valid_tile_coord(df::coord2d p) {
    return (p.x & ~15) == 0 && (p.y & ~15) == 0;
}

class DFHACK_EXPORT Block
{
public:
    Block(MapCache *parent, DFCoord _bcoord);
    ~Block();

    /*
     * All coordinates are taken mod 16.
     */

    //Arbitrary tag field for flood fills etc.
    int16_t &tag(df::coord2d p) {
        return index_tile<int16_t&>(tags, p);
    }

    int16_t veinMaterialAt(df::coord2d p)
    {
        return index_tile<int16_t>(veinmats,p);
    }
    int16_t baseMaterialAt(df::coord2d p)
    {
        return index_tile<int16_t>(basemats,p);
    }

    df::tiletype BaseTileTypeAt(df::coord2d p)
    {
        auto tt = index_tile<df::tiletype>(contiles,p);
        if (tt != tiletype::Void) return tt;
        tt = index_tile<df::tiletype>(icetiles,p);
        if (tt != tiletype::Void) return tt;
        return index_tile<df::tiletype>(rawtiles,p);
    }
    df::tiletype TileTypeAt(df::coord2d p)
    {
        return index_tile<df::tiletype>(rawtiles,p);
    }
    bool setTiletypeAt(df::coord2d p, df::tiletype tiletype)
    {
        if(!valid) return false;
        dirty_tiletypes = true;
        //printf("setting block %d/%d/%d , %d %d\n",x,y,z, p.x, p.y);
        index_tile<df::tiletype&>(rawtiles,p) = tiletype;
        return true;
    }

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
        if(des.bits.dig)
        {
            dirty_blockflags = true;
            blockflags.bits.designated = true;
        }
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
        return blockflags;
    }
    bool setBlockFlags(t_blockflags des)
    {
        if(!valid) return false;
        dirty_blockflags = true;
        //printf("setting block %d/%d/%d , %d %d\n",x,y,z, p.x, p.y);
        blockflags = des;
        return true;
    }

    bool Write();

    int16_t GeoIndexAt(df::coord2d p);

    bool GetGlobalFeature(t_feature *out);
    bool GetLocalFeature(t_feature *out);

    bool is_valid() { return valid; }
    df::map_block *getRaw() { return block; }

private:
    friend class MapCache;

    MapCache *parent;
    df::map_block *block;

    static void SquashVeins(df::map_block *mb, t_blockmaterials & materials);
    static void SquashFrozenLiquids (df::map_block *mb, tiletypes40d & frozen);
    static void SquashConstructions (df::map_block *mb, tiletypes40d & constructions);
    static void SquashRocks (df::map_block *mb, t_blockmaterials & materials,
                             std::vector< std::vector <int16_t> > * layerassign);

    bool valid;
    bool dirty_designations:1;
    bool dirty_tiletypes:1;
    bool dirty_temperatures:1;
    bool dirty_blockflags:1;
    bool dirty_occupancies:1;

    DFCoord bcoord;

    int16_t tags[16][16];

    typedef int T_item_counts[16];
    T_item_counts *item_counts;
    void init_item_counts();

    bool addItemOnGround(df::item *item);
    bool removeItemOnGround(df::item *item);

    tiletypes40d rawtiles;
    designations40d designation;
    occupancies40d occupancy;
    t_blockflags blockflags;

    t_blockmaterials veinmats;
    t_blockmaterials basemats;
    t_temperatures temp1;
    t_temperatures temp2;
    tiletypes40d contiles; // what's underneath constructions
    tiletypes40d icetiles; // what's underneath ice
};

class DFHACK_EXPORT MapCache
{
    public:
    MapCache()
    {
        valid = 0;
        Maps::getSize(x_bmax, y_bmax, z_max);
        x_tmax = x_bmax*16; y_tmax = y_bmax*16;
        validgeo = Maps::ReadGeology(&layer_mats, &geoidx);
        valid = true;
    };
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
        Block * b= BlockAtTile(tilecoord);
        return b ? b->BaseTileTypeAt(tilecoord) : tiletype::Void;
    }
    df::tiletype tiletypeAt (DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return b ? b->TileTypeAt(tilecoord) : tiletype::Void;
    }
    bool setTiletypeAt(DFCoord tilecoord, df::tiletype tiletype)
    {
        if (Block * b= BlockAtTile(tilecoord))
            return b->setTiletypeAt(tilecoord, tiletype);
        return false;
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

    int16_t veinMaterialAt (DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return b ? b->veinMaterialAt(tilecoord) : -1;
    }
    int16_t baseMaterialAt (DFCoord tilecoord)
    {
        Block * b= BlockAtTile(tilecoord);
        return b ? b->baseMaterialAt(tilecoord) : -1;
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

    bool valid;
    bool validgeo;
    uint32_t x_bmax;
    uint32_t y_bmax;
    uint32_t x_tmax;
    uint32_t y_tmax;
    uint32_t z_max;
    std::vector<int16_t> geoidx;
    std::vector< std::vector <int16_t> > layer_mats;
    std::map<DFCoord, Block *> blocks;
};
}
#endif