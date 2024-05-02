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

#include "Pragma.h"
#include "Export.h"
#include "DataDefs.h"

#include "df/tiletype.h"

namespace DFHack
{
    //Mainly walls and rivers
    //Byte values are used because walls can have either 1 or 2 in any given direction.
    const int TileDirectionCount = 4;
    union TileDirection
    {
        uint32_t whole;
        unsigned char b[TileDirectionCount];
        struct
        {
            //Maybe should add 'up' and 'down' for Z-levels?
            unsigned char north,south,west,east;
        };

        inline TileDirection()
        {
            whole = 0;
        }
        TileDirection( uint32_t whole_bits)
        {
            whole = whole_bits;
        }
        bool operator== (const TileDirection &other) const
        {
            return whole == other.whole;
        }
        bool operator!= (const TileDirection &other) const
        {
            return whole != other.whole;
        }
        operator bool() const
        {
            return whole != 0;
        }
        TileDirection( unsigned char North, unsigned char South, unsigned char West, unsigned char East )
        {
            north=North; south=South; east=East; west=West;
        }
        TileDirection( const char *dir )
        {
            //This one just made for fun.
            //Supports N S E W
            const char *p = dir;
            unsigned char *l=0;
            north=south=east=west=0;
            if(!dir) return;

            for( ;*p;++p)
            {
                switch(*p)
                {
                    case 'N': //North / Up
                    case 'n':
                        ++north; l=&north; break;
                    case 'S': //South / Down
                    case 's':
                        ++south; l=&south; break;
                    case 'E': //East / Right
                    case 'e':
                        ++east; l=&east; break;
                    case 'W': //West / Left
                    case 'w':
                        ++west; l=&west; break;
                    case '-':
                    case ' ':
                        //Explicitly ensure dash and space are ignored.
                        //Other characters/symbols may be assigned in the future.
                        break;
                    default:
                        if( l && '0' <= *p && '9' >= *p )
                            *l += *p - '0';
                        break;
                }
            }
        }

        //may be useful for some situations
        inline uint32_t sum() const
        {
            return 0L + north + south + east + west;
        }

        //Gives a string that represents the direction.
        //This is a static string, overwritten with every call!
        //Support values > 2 even though they should never happen.
        //Copy string if it will be used.
        inline const char * getStr() const
        {
            static char str[16];

            str[8]=0;
    #define DIRECTION(x,i,c) \
            str[i] = str[i+1] = '-'; \
            if(x){ \
                str[i]=c; \
                if(1==x) ; \
                else if(2==x) str[i+1]=c; \
                else str[i+1]='0'+x; \
            }

            DIRECTION(north,0,'N')
            DIRECTION(south,2,'S')
            DIRECTION(west,4,'W')
            DIRECTION(east,6,'E')
    #undef DIRECTION
            return str;
        }
    };

    using namespace df::enums;

    inline
        const char * tileName(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype, caption, tiletype);
    }

    inline
    df::tiletype_shape tileShape(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype, shape, tiletype);
    }

    inline
    df::tiletype_shape_basic tileShapeBasic(df::tiletype_shape tileshape)
    {
        return ENUM_ATTR(tiletype_shape, basic_shape, tileshape);
    }

    inline
    df::tiletype_special tileSpecial(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype, special, tiletype);
    }

    inline
    df::tiletype_variant tileVariant(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype, variant, tiletype);
    }

    inline
    df::tiletype_material tileMaterial(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype, material, tiletype);
    }

    inline
    TileDirection tileDirection(df::tiletype tiletype)
    {
        return TileDirection(ENUM_ATTR(tiletype, direction, tiletype));
    }

    // Air
    inline bool isAirMaterial(df::tiletype_material mat) { return mat == tiletype_material::AIR; }
    inline bool isAirMaterial(df::tiletype tt) { return isAirMaterial(tileMaterial(tt)); }

    // Soil
    inline bool isSoilMaterial(df::tiletype_material mat) { return mat == tiletype_material::SOIL; }
    inline bool isSoilMaterial(df::tiletype tt) { return isSoilMaterial(tileMaterial(tt)); }

    // Stone materials - their tiles are completely interchangable
    inline bool isStoneMaterial(df::tiletype_material mat)
    {
        using namespace df::enums::tiletype_material;
        switch (mat) {
            case STONE: case LAVA_STONE: case MINERAL: case FEATURE:
                return true;
            default:
                return false;
        }
    }
    inline bool isStoneMaterial(df::tiletype tt) { return isStoneMaterial(tileMaterial(tt)); }

    // Regular ground materials = stone + soil
    inline bool isGroundMaterial(df::tiletype_material mat)
    {
        using namespace df::enums::tiletype_material;
        switch (mat) {
            case SOIL:
            case STONE: case LAVA_STONE: case MINERAL: case FEATURE:
                return true;
            default:
                return false;
        }
    }
    inline bool isGroundMaterial(df::tiletype tt) { return isGroundMaterial(tileMaterial(tt)); }

    // Core materials - their tile sets are sufficiently close to stone
    inline bool isCoreMaterial(df::tiletype_material mat)
    {
        using namespace df::enums::tiletype_material;
        switch (mat) {
            case SOIL:
            case STONE: case LAVA_STONE: case MINERAL: case FEATURE:
            case FROZEN_LIQUID: case CONSTRUCTION:
                return true;
            default:
                return false;
        }
    }
    inline bool isCoreMaterial(df::tiletype tt) { return isCoreMaterial(tileMaterial(tt)); }

    // tile is missing a floor
    inline
    bool LowPassable(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, passable_low, tileShape(tiletype));
    }

    // tile is missing a roof
    inline
    bool HighPassable(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, passable_high, tileShape(tiletype));
    }

    inline
    bool FlowPassable(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, passable_flow, tileShape(tiletype));
    }

    inline
    bool FlowPassableDown(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, passable_flow_down, tileShape(tiletype));
    }

    inline
    bool isWalkable(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, walkable, tileShape(tiletype));
    }

    inline
    bool isWalkableUp(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, walkable_up, tileShape(tiletype));
    }

    inline
    bool isWallTerrain(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, basic_shape, tileShape(tiletype)) == tiletype_shape_basic::Wall;
    }

    inline
    bool isFloorTerrain(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, basic_shape, tileShape(tiletype)) == tiletype_shape_basic::Floor;
    }

    inline
    bool isRampTerrain(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, basic_shape, tileShape(tiletype)) == tiletype_shape_basic::Ramp;
    }

    inline
    bool isOpenTerrain(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, basic_shape, tileShape(tiletype)) == tiletype_shape_basic::Open;
    }

    inline
    bool isStairTerrain(df::tiletype tiletype)
    {
        return ENUM_ATTR(tiletype_shape, basic_shape, tileShape(tiletype)) == tiletype_shape_basic::Stair;
    }

    /**
     * zilpin: Find the first tile entry which matches the given search criteria.
     * All parameters are optional.
     * To omit, specify NONE for that type
     * For tile directions, pass nullptr to omit.
     * @return matching index in tileTypeTable, or 0 if none found.
     */
    inline
    df::tiletype findTileType(const df::tiletype_shape tshape, const df::tiletype_material tmat, const df::tiletype_variant tvar, const df::tiletype_special tspecial, const TileDirection tdir)
    {
        FOR_ENUM_ITEMS(tiletype, tt)
        {
            if (tshape != tiletype_shape::NONE && tshape != tileShape(tt))
                continue;
            if (tmat != tiletype_material::NONE && tmat != tileMaterial(tt))
                continue;
            // Don't require variant to match if the destination tile doesn't even have one
            if (tvar != tiletype_variant::NONE && tvar != tileVariant(tt) && tileVariant(tt) != tiletype_variant::NONE)
                continue;
            // Same for special
            if (tspecial != tiletype_special::NONE && tspecial != tileSpecial(tt) && tileSpecial(tt) != tiletype_special::NONE)
                continue;
            if (tdir && tdir != tileDirection(tt))
                continue;
            // Match!
            return tt;
        }
        return tiletype::Void;
    }

    /**
     * zilpin: Find a tile type similar to the one given, but with a different class.
     * Useful for tile-editing operations.
     * If no match found, returns the sourceType
     *
     * @todo Definitely needs improvement for wall directions, etc.
     */
    DFHACK_EXPORT df::tiletype findSimilarTileType( const df::tiletype sourceTileType, const df::tiletype_shape tshape );

    /**
     * Finds a random variant of the selected tile
     * If there are no variants, returns the same tile
     */
    DFHACK_EXPORT df::tiletype findRandomVariant(const df::tiletype tile);

    /**
     * Map a tile type to a different core material (see above for the list).
     * Returns Void (0) in case of failure.
     */
    DFHACK_EXPORT df::tiletype matchTileMaterial(df::tiletype source, df::tiletype_material tmat);
}
