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

#pragma once

#ifndef TILETYPES_H_INCLUDED
#define TILETYPES_H_INCLUDED

#include "DFPragma.h"
#include "DFExport.h"

namespace DFHack
{

    // tile class -- determines the general shape of the tile
    // enum and lookup table for string names created using X macros
    #define TILESHAPE_MACRO \
        X(EMPTY,			"") \
        X(WALL,				"") \
        X(PILLAR,			"") \
        X(FORTIFICATION,	"") \
        X(STAIR_UP,			"") \
        X(STAIR_DOWN,		"") \
        X(STAIR_UPDOWN,		"") \
        X(RAMP,				"ramps have no direction" ) \
        X(RAMP_TOP,			"used for pathing?" ) \
        X(FLOOR,			"") \
        X(TREE_DEAD,		"") \
        X(TREE_OK,			"") \
        X(SAPLING_DEAD,		"") \
        X(SAPLING_OK,		"") \
        X(SHRUB_DEAD,		"") \
        X(SHRUB_OK,			"") \
        X(BOULDER,			"") \
        X(PEBBLES,			"") 
    //end TILESHAPE_MACRO

    //define tile class enum
    #define X(name,comment) name,
    enum TileShape {
        tileshape_invalid=-1,
        TILESHAPE_MACRO
        tileshape_count,
    };
    #undef X

    DFHACK_EXPORT extern const char *TileShapeString[];

    #define TILEMATERIAL_MACRO \
        X(AIR,        "empty" ) \
        X(SOIL,       "ordinary soil. material depends on geology" ) \
        X(STONE,      "ordinary layer stone. material depends on geology" ) \
        X(FEATSTONE,  "map special stone. used for things like hell, the hell temple or adamantine tubes. material depends on local/global special" ) \
        X(OBSIDIAN,   "cast obsidian" ) \
        X(VEIN,       "vein stone. material depends on mineral veins present" ) \
        X(ICE,        "frozen water... not much to say. you can determine what was on the tile before it froze by looking into the 'ice vein' objects" ) \
        X(GRASS,      "grass (has 4 variants)" ) \
        X(GRASS2,     "grass (has 4 variants)" ) \
        X(GRASS_DEAD, "dead grass (has 4 variants)" ) \
        X(GRASS_DRY,  "dry grass (has 4 variants)" ) \
        X(DRIFTWOOD,  "non-specified wood - normally on top of the local layer stone/soil." ) \
        X(HFS,        "the stuff demon pits are made of - this makes them different from ordinary pits." ) \
        X(MAGMA,      "material for semi-molten rock and 'magma flow' tiles" ) \
        X(CAMPFIRE,   "human armies make them when they siege. The original tile may be lost?" ) \
        X(FIRE,       "burning grass" ) \
        X(ASHES,      "what remains from a FIRE" ) \
        X(CONSTRUCTED,"tile material depends on the construction present" ) \
        X(CYAN_GLOW,  "the glowy stuff that disappears from the demon temple when you take the sword." ) 
    //end TILEMATERIAL_MACRO

    // material enum
    #define X(name,comment) name,
    enum TileMaterial {
        tilematerial_invalid=-1,
        TILEMATERIAL_MACRO
        tilematerial_count,
    };
    #undef X


    DFHACK_EXPORT extern const char *TileMaterialString[];

    // Special specials of the tile.
    // Not the best way to do this, but compatible with existing code.
    // When the TileType class gets created, everything should be re-thought.
    #define TILESPECIAL_MACRO \
        X(NORMAL,            "Default for all type, nothing present" ) \
        X(SPECIAL,           "General purpose, for any unique tile which can not otherwise be differenciated" ) \
        X(POOL,              "Murky Pool, will gather water from rain" ) \
        X(STREAM,            "Streams (and brooks too? maybe?)" ) \
        X(STREAM_TOP,        "The walkable surface of a stream/brook" ) \
        X(RIVER_SOURCE,      "Rivers Source, when it exists on a map" ) \
        X(RIVER,             "Rivers, and their entering and exiting tiles" ) \
        X(WATERFALL,         "Special case for Waterfall Landing. How's this used?" ) \
        X(ENDLESS,           "Eerie Pit and Old Chasm/Endless Pit" ) \
        X(CRACKED,           "Walls being dug" ) \
        X(DAMAGED,           "Walls being dug" ) \
        X(WORN,              "Walls being dug ??" ) \
        X(SMOOTH,            "Walls and floors." ) 
    //end TILESPECIAL_MACRO

    //special enum
    #define X(name,comment) TILE_##name,
    enum TileSpecial {
        tilespecial_invalid=-1,
        TILESPECIAL_MACRO
        tilespecial_count,
    };
    #undef X

    DFHACK_EXPORT extern const char *TileSpecialString[];

    // variants are used for tiles, where there are multiple variants of the same - like grass floors
    enum TileVariant
    {
        tilevariant_invalid=-1,
        VAR_1, //Yes, the value of VAR_1 is 0.  It's legacy.  Deal with it.
        VAR_2,
        VAR_3,
        VAR_4,
    };


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
            unsigned char	north,south,west,east;
        };

        inline TileDirection()
        {
            whole = 0;
        }
        TileDirection( uint32_t whole_bits)
        {
            whole = whole_bits;
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
        inline char * getStr() const
        {
            static char str[16];
            //type punning trick
            *( (uint64_t *)str ) = *( (uint64_t *)"--------" );
            str[8]=0;
    #define DIRECTION(x,i,c) \
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

    struct TileRow
    {
        const char * name;
        TileShape shape;
        TileMaterial material;
        TileVariant variant;
        TileSpecial special;
        TileDirection direction;
    };

    #define TILE_TYPE_ARRAY_LENGTH 520

    extern DFHACK_EXPORT const TileRow tileTypeTable[];

    // tile is missing a floor
    inline
    bool LowPassable(uint16_t tiletype)
    {
        switch(DFHack::tileTypeTable[tiletype].shape)
        {
            case DFHack::EMPTY:
            case DFHack::STAIR_DOWN:
            case DFHack::STAIR_UPDOWN:
            case DFHack::RAMP_TOP:
                return true;
            default:
                return false;
        }
    };

    // tile is missing a roof
    inline
    bool HighPassable(uint16_t tiletype)
    {
        switch(DFHack::tileTypeTable[tiletype].shape)
        {
            case DFHack::EMPTY:
            case DFHack::STAIR_DOWN:
            case DFHack::STAIR_UPDOWN:
            case DFHack::STAIR_UP:
            case DFHack::RAMP:
            case DFHack::RAMP_TOP:
            case DFHack::FLOOR:
            case DFHack::SAPLING_DEAD:
            case DFHack::SAPLING_OK:
            case DFHack::SHRUB_DEAD:
            case DFHack::SHRUB_OK:
            case DFHack::BOULDER:
            case DFHack::PEBBLES:
                return true;
            default:
                return false;
        }
    };

    inline
    bool FlowPassable(uint16_t tiletype)
    {
        TileShape tc = tileTypeTable[tiletype].shape;
        return tc != WALL && tc != PILLAR && tc != TREE_DEAD && tc != TREE_OK;
    };

    inline
    bool isWallTerrain(int tiletype)
    {
        return tileTypeTable[tiletype].shape >= WALL && tileTypeTable[tiletype].shape <= FORTIFICATION ;
    }

    inline
    bool isFloorTerrain(int tiletype)
    {
        return tileTypeTable[tiletype].shape >= FLOOR && tileTypeTable[tiletype].shape <= PEBBLES;
    }

    inline
    bool isRampTerrain(int tiletype)
    {
        return tileTypeTable[tiletype].shape == RAMP;
    }

    inline
    bool isStairTerrain(int tiletype)
    {
        return tileTypeTable[tiletype].shape >= STAIR_UP && tileTypeTable[tiletype].shape <= STAIR_UPDOWN;
    }

    inline
    bool isOpenTerrain(int tiletype)
    {
        return tileTypeTable[tiletype].shape == EMPTY;
    }

    inline
    const char * tileName(int tiletype)
    {
        return tileTypeTable[tiletype].name;
    }

    inline
    TileShape tileShape(int tiletype)
    {
        return tileTypeTable[tiletype].shape;
    }

    inline
    TileSpecial tileSpecial(int tiletype)
    {
        return tileTypeTable[tiletype].special;
    }

    inline
    TileVariant tileVariant(int tiletype)
    {
        return tileTypeTable[tiletype].variant;
    }

    inline
    TileMaterial tileMaterial(int tiletype)
    {
        return tileTypeTable[tiletype].material;
    }

    inline
    TileDirection tileDirection(int tiletype)
    {
        return tileTypeTable[tiletype].direction;
    }

    /// Safely access the tile type array.
    inline const
    TileRow * getTileRow(int tiletype)
    {
        if( tiletype<0 || tiletype>=TILE_TYPE_ARRAY_LENGTH ) return 0;
        return ( const TileRow * ) &tileTypeTable[tiletype];
    }

    /**
     * zilpin: Find the first tile entry which matches the given search criteria.
     * All parameters are optional.
     * To omit, use the 'invalid' enum for that type (e.g. tileclass_invalid, tilematerial_invalid, etc)
     * For tile directions, pass NULL to omit.
     * @return matching index in tileTypeTable, or -1 if none found.
     */
    inline
    int32_t findTileType( const TileShape tshape, const TileMaterial tmat, const TileVariant tvar, const TileSpecial tspecial, const TileDirection tdir )
    {
        int32_t tt;
        for(tt=0;tt<TILE_TYPE_ARRAY_LENGTH; ++tt){
            if( tshape>-1	) if( tshape != tileTypeTable[tt].shape ) continue;
            if( tmat>-1		) if( tmat != tileTypeTable[tt].material ) continue;
            if( tvar>-1		) if( tvar != tileTypeTable[tt].variant ) continue;
            if( tspecial>-1 ) if( tspecial != tileTypeTable[tt].special ) continue;
            if( tdir.whole  ) if( tdir.whole != tileTypeTable[tt].direction.whole ) continue;
            //Match!
            return tt;
        }
        return -1;
    }

    /**
     * zilpin: Find a tile type similar to the one given, but with a different class.
     * Useful for tile-editing operations.
     * If no match found, returns the sourceType
     * 
     * @todo Definitely needs improvement for wall directions, etc.
     */
    DFHACK_EXPORT int32_t findSimilarTileType( const int32_t sourceTileType, const TileShape tshape );
}



#endif // TILETYPES_H_INCLUDED
