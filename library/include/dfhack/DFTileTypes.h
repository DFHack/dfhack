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

#ifndef TILETYPES_H_INCLUDED
#define TILETYPES_H_INCLUDED

#include "DFPragma.h"

namespace DFHack
{

	// tile class -- determines the general shape of the tile
	// enum and lookup table for string names created using X macros
#define TILECLASS_MACRO \
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
//end TILECLASS_MACRO

	//define tile class enum
	#define X(name,comment) name,
	enum TileClass {
		tileclass_invalid=-1,
		TILECLASS_MACRO
		tileclass_count,
	};
	#undef X

	//Visual Studio screams if you don't do this for the const char* arrays
	#ifndef char_p
	typedef char * char_p;
	#endif

	//set tile class string lookup table (e.g. for printing to user)
	#define X(name,comment) #name,
	const char_p TileClassString[tileclass_count+1] = {
		TILECLASS_MACRO
		NULL
	};
	#undef X


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

	//string lookup table (e.g. for printing to user)
	#define X(name,comment) #name,
	const char_p TileMaterialString[tilematerial_count+1] = {
		TILEMATERIAL_MACRO
		NULL
	};
	#undef X


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

	//string lookup table (e.g. for printing to user)
	#define X(name,comment) #name,
	const char_p TileSpecialString[tilespecial_count+1] = {
		TILESPECIAL_MACRO
		NULL
	};
	#undef X


    // variants are used for tiles, where there are multiple variants of the same - like grass floors
    enum TileVariant
    {
		tilevariant_invalid=-1,
        VAR_1,	//Yes, the value of VAR_1 is 0.  It's legacy.  Deal with it.
        VAR_2,
        VAR_3,
        VAR_4,
    };


	//Mainly walls and rivers
	//Byte values are used because walls can have either 1 or 2 in any given direction.
	const int TileDirectionCount = 4;
	union TileDirection
	{
		uint32_t	whole;
		unsigned char b[TileDirectionCount];
		struct {
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
			unsigned char *l=NULL;
			north=south=east=west=0;
			if(!dir) return;

			for( ;*p;++p){
				switch(*p){
					case 'N':	//North / Up
					case 'n':
						++north; l=&north; break;
					case 'S':	//South / Down
					case 's':
						++south; l=&south; break;
					case 'E':	//East / Right
					case 'e':
						++east; l=&east; break;
					case 'W':	//West / Left
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
		inline uint32_t sum() const {
			return 0L + north + south + east + west;
		}

		//Gives a string that represents the direction.
		//This is a static string, overwritten with every call!
		//Support values > 2 even though they should never happen.
		//Copy string if it will be used.
		inline char * getStr() const {
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
        TileClass c;
        TileMaterial m;
        TileVariant v;
		TileSpecial s;
		TileDirection d;
    };
	
	#define TILE_TYPE_ARRAY_LENGTH 520

    const TileRow tileTypeTable[TILE_TYPE_ARRAY_LENGTH] =
    {
        // 0
        {"void",EMPTY, AIR, VAR_1},
        {"ramp top",RAMP_TOP, AIR, VAR_1},
        {"pool",FLOOR, SOIL, VAR_1,			TILE_POOL},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        
        // 10
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {0, EMPTY, AIR, VAR_1},
        {"driftwood stack",FLOOR, DRIFTWOOD, VAR_1},
        
        // 20
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"tree",TREE_OK, SOIL, VAR_1},
        {"ice stair up/down",STAIR_UPDOWN, ICE, VAR_1},
        {"ice stair down",STAIR_DOWN, ICE, VAR_1},
        {"ice stair up",STAIR_UP, ICE, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 30
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"empty space",EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"shrub",SHRUB_OK, SOIL, VAR_1},
        {"chasm",FLOOR, AIR, VAR_1,	TILE_ENDLESS },
        {"obsidian stair up/down",STAIR_UPDOWN, OBSIDIAN, VAR_1},
        {"obsidian stair down",STAIR_DOWN, OBSIDIAN, VAR_1},
        {"obsidian stair up",STAIR_UP, OBSIDIAN, VAR_1},
        {"soil stair up/down",STAIR_UPDOWN, SOIL, VAR_1},
        
        // 40
        {"soil stair down",STAIR_DOWN, SOIL, VAR_1},
        {"soil stair up",STAIR_UP, SOIL, VAR_1},
        {"eerie pit",FLOOR, HFS, VAR_1, TILE_ENDLESS},
        {"smooth stone floor",FLOOR, STONE, VAR_1 , TILE_SMOOTH },
        {"smooth obsidian floor",FLOOR, OBSIDIAN, VAR_1 , TILE_SMOOTH },
        {"smooth featstone? floor",FLOOR, FEATSTONE, VAR_1 , TILE_SMOOTH },
        {"smooth vein floor",FLOOR, VEIN, VAR_1 , TILE_SMOOTH },
        {"smooth ice floor",FLOOR, ICE, VAR_1 , TILE_SMOOTH },
        {0 ,EMPTY, AIR, VAR_1},
        {"grass stair up/down",STAIR_UPDOWN, GRASS, VAR_1},
        
        // 50
        {"grass stair down",STAIR_DOWN, GRASS, VAR_1},
        {"grass stair up",STAIR_UP, GRASS, VAR_1},
        {"grass2 stair up/down",STAIR_UPDOWN, GRASS2, VAR_1},
        {"grass2 stair down",STAIR_DOWN, GRASS2, VAR_1},
        {"grass2 stair up",STAIR_UP, GRASS2, VAR_1},
        {"stone stair up/down",STAIR_UPDOWN, STONE, VAR_1},
        {"stone stair down",STAIR_DOWN, STONE, VAR_1},
        {"stone stair up",STAIR_UP, STONE, VAR_1},
        {"vein stair up/down",STAIR_UPDOWN, VEIN, VAR_1},
        {"vein stair down",STAIR_DOWN, VEIN, VAR_1},
        
        // 60
        {"vein stair up",STAIR_UP, VEIN, VAR_1},
        {"featstone? stair up/down",STAIR_UPDOWN, FEATSTONE, VAR_1},
        {"featstone? stair down",STAIR_DOWN, FEATSTONE, VAR_1},
        {"featstone? stair up",STAIR_UP, FEATSTONE, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"stone fortification",FORTIFICATION, STONE, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"campfire",FLOOR, CAMPFIRE, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 70
        {"fire",FLOOR, FIRE, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"stone pillar",PILLAR, STONE, VAR_1},
        
        //80
        {"obsidian pillar",PILLAR, OBSIDIAN, VAR_1},
        {"featstone? pillar",PILLAR, FEATSTONE, VAR_1},
        {"vein pillar",PILLAR, VEIN, VAR_1},
        {"ice pillar",PILLAR, ICE, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"waterfall landing",FLOOR, SOIL, VAR_1,  TILE_WATERFALL }, // verify material
        
        // 90
        {"river source",FLOOR, SOIL, VAR_1,  TILE_RIVER_SOURCE }, // verify material
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 100
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 110
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 120
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 130
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 140
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 150
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 160
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 170
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"cracked stone wall" ,WALL, STONE, VAR_1, TILE_CRACKED },
        {"damaged stone wall" ,WALL, STONE, VAR_1, TILE_DAMAGED },
        {"worn stone wall" ,WALL, STONE, VAR_1,    TILE_WORN },
        {0 ,EMPTY, AIR, VAR_1},
        
        // 180
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 190
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 200
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 210
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"stone wall" ,WALL, STONE, VAR_1},
        
        // 220
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 230
        {0 ,EMPTY, AIR, VAR_1},
        {"sapling" ,SAPLING_OK, SOIL, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"dry grass ramp" ,RAMP, GRASS_DRY, VAR_1},
        {"dead grass ramp" ,RAMP, GRASS_DEAD, VAR_1},
        {"grass ramp" ,RAMP, GRASS, VAR_1},
        {"grass ramp" ,RAMP, GRASS2, VAR_1},
        {"stone ramp" ,RAMP, STONE, VAR_1},
        {"obsidian ramp" ,RAMP, OBSIDIAN, VAR_1},
        {"featstone? ramp" ,RAMP, FEATSTONE, VAR_1},
        
        // 240
        {"vein ramp" ,RAMP, VEIN, VAR_1},
        {"soil ramp" ,RAMP, SOIL, VAR_1},
        {"ashes" ,FLOOR, ASHES, VAR_1},
        {"ashes" ,FLOOR, ASHES, VAR_2},
        {"ashes" ,FLOOR, ASHES, VAR_3},
        {"ice ramp" ,RAMP, ICE, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 250
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"ice floor" ,FLOOR, ICE, VAR_2},
        {"ice floor" ,FLOOR, ICE, VAR_3},
        
        // 260
        {"ice floor" ,FLOOR, ICE, VAR_4},
        {"furrowed soil" ,FLOOR, SOIL, VAR_1},
        {"ice floor" ,FLOOR, ICE, VAR_1},
        {"semi-molten rock" ,WALL, MAGMA, VAR_1},// unminable magma wall
        {"magma" ,FLOOR, MAGMA, VAR_1},
        {"soil wall" ,WALL, SOIL, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"glowing floor" ,FLOOR, CYAN_GLOW, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"smooth obsidian wall RD2",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH ,  "--SS--E-" },
        
        // 270
        {"smooth obsidian wall R2D",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH ,  "--S---EE" },
        {"smooth obsidian wall R2U",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH ,  "N-----EE" },
        {"smooth obsidian wall RU2",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  , "NN----E-" },
        {"smooth obsidian wall L2U",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  , "N---WW--" },
        {"smooth obsidian wall LU2",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  , "NN--W---" },
        {"smooth obsidian wall L2D",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  , "--S-WW--" },
        {"smooth obsidian wall LD2",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  , "--SSW---" },
        {"smooth obsidian wall LRUD",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  ,"N-S-W-E-" },
        {"smooth obsidian wall RUD",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  , "N-S---E-" },
        {"smooth obsidian wall LRD",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  , "--S-W-E-" },
        
        // 280
        {"smooth obsidian wall LRU",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  , "N---W-E-" },
        {"smooth obsidian wall LUD",WALL,OBSIDIAN,VAR_1 , TILE_SMOOTH  , "N-S-W---" },
        {"smooth obsidian wall RD",WALL,OBSIDIAN,VAR_1  , TILE_SMOOTH  , "--S---E-" },
        {"smooth obsidian wall RU",WALL,OBSIDIAN,VAR_1  , TILE_SMOOTH  , "N-----E-" },
        {"smooth obsidian wall LU",WALL,OBSIDIAN,VAR_1  , TILE_SMOOTH  , "N---W---" },
        {"smooth obsidian wall LD",WALL,OBSIDIAN,VAR_1  , TILE_SMOOTH  , "--S-W---" },
        {"smooth obsidian wall UD",WALL,OBSIDIAN,VAR_1  , TILE_SMOOTH  , "N-S-----" },
        {"smooth obsidian wall LR",WALL,OBSIDIAN,VAR_1  , TILE_SMOOTH  , "----W-E-" },
        {"smooth featstone wall RD2",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "--SS--E-" },
        {"smooth featstone wall R2D",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "--S---EE" },
        
        // 290
        {"smooth featstone wall R2U",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "N-----EE" },
        {"smooth featstone wall RU2",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "NN----E-" },
        {"smooth featstone wall L2U",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "N---WW--" },
        {"smooth featstone wall LU2",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "NN--W---" },
        {"smooth featstone wall L2D",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "--S-WW--" },
        {"smooth featstone wall LD2",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "--SSW---" },
        {"smooth featstone wall LRUD",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  ,"N-S-W-E-" },
        {"smooth featstone wall RUD",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "N-S---E-" },
        {"smooth featstone wall LRD",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "--S-W-E-" },
        {"smooth featstone wall LRU",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "N---W-E-" },
        
        //300
        {"smooth featstone wall LUD",WALL,FEATSTONE,VAR_1, TILE_SMOOTH  , "N-S-W---" },
        {"smooth featstone wall RD",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "--S---E-" },
        {"smooth featstone wall RU",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "N-----E-" },
        {"smooth featstone wall LU",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "N---W---" },
        {"smooth featstone wall LD",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "--S-W---" },
        {"smooth featstone wall UD",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "N-S-----" },
        {"smooth featstone wall LR",WALL,FEATSTONE,VAR_1 , TILE_SMOOTH  , "----W-E-" },
        {"smooth stone wall RD2",WALL,STONE,VAR_1 , TILE_SMOOTH  , "--SS--E-" },
        {"smooth stone wall R2D",WALL,STONE,VAR_1 , TILE_SMOOTH  , "--S---EE" },
        {"smooth stone wall R2U",WALL,STONE,VAR_1 , TILE_SMOOTH  , "N-----EE" },
        
        //310
        {"smooth stone wall RU2",WALL,STONE,VAR_1 , TILE_SMOOTH  , "NN----E-" },
        {"smooth stone wall L2U",WALL,STONE,VAR_1 , TILE_SMOOTH  , "N---WW--" },
        {"smooth stone wall LU2",WALL,STONE,VAR_1 , TILE_SMOOTH  , "NN--W---" },
        {"smooth stone wall L2D",WALL,STONE,VAR_1 , TILE_SMOOTH  , "--S-WW--" },
        {"smooth stone wall LD2",WALL,STONE,VAR_1 , TILE_SMOOTH  , "--SSW---" },
        {"smooth stone wall LRUD",WALL,STONE,VAR_1 , TILE_SMOOTH  , "N-S-W-E-" },
        {"smooth stone wall RUD",WALL,STONE,VAR_1 , TILE_SMOOTH  , "N-S---E-" },
        {"smooth stone wall LRD",WALL,STONE,VAR_1 , TILE_SMOOTH  , "--S-W-E-"  },
        {"smooth stone wall LRU",WALL,STONE,VAR_1 , TILE_SMOOTH  , "N---W-E-" },
        {"smooth stone wall LUD",WALL,STONE,VAR_1 , TILE_SMOOTH  , "N-S-W---" },
        
        //320
        {"smooth stone wall RD",WALL,STONE,VAR_1 , TILE_SMOOTH  , "--S---E-" },
        {"smooth stone wall RU",WALL,STONE,VAR_1 , TILE_SMOOTH  , "N-----E-" },
        {"smooth stone wall LU",WALL,STONE,VAR_1 , TILE_SMOOTH  , "N---W---" },
        {"smooth stone wall LD",WALL,STONE,VAR_1 , TILE_SMOOTH  ,  "--S-W---"  },
        {"smooth stone wall UD",WALL,STONE,VAR_1 , TILE_SMOOTH  , "N-S-----" },
        {"smooth stone wall LR",WALL,STONE,VAR_1 , TILE_SMOOTH  , "----W-E-" },
        {"obsidian fortification",FORTIFICATION,OBSIDIAN,VAR_1},
        {"featstone? fortification",FORTIFICATION,FEATSTONE,VAR_1},
        {"cracked obsidian wall",WALL,OBSIDIAN,VAR_1, TILE_CRACKED },
        {"damaged obsidian wall",WALL,OBSIDIAN,VAR_1, TILE_DAMAGED },
        
        // 330
        {"worn obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"obsidian wall",WALL,OBSIDIAN,VAR_1},
                /*MAPTILE_FEATSTONE_WALL_WORN1,
                    MAPTILE_FEATSTONE_WALL_WORN2,
                      MAPTILE_FEATSTONE_WALL_WORN3,
                        MAPTILE_FEATSTONE_WALL,*/
        {"cracked featstone wall",WALL,STONE,VAR_1, TILE_CRACKED },
        {"damaged featstone wall",WALL,STONE,VAR_1, TILE_DAMAGED },
        {"worn featstone wall",WALL,STONE,VAR_1,    TILE_WORN },
        {"featstone wall",WALL,STONE,VAR_1},
        {"stone floor",FLOOR,STONE,VAR_1},
        {"stone floor",FLOOR,STONE,VAR_2},
        {"stone floor",FLOOR,STONE,VAR_3},
        {"stone floor",FLOOR,STONE,VAR_4},
        
        // 340
        {"obsidian floor",FLOOR,OBSIDIAN,VAR_1},
        {"obsidian floor",FLOOR,OBSIDIAN,VAR_2},
        {"obsidian floor",FLOOR,OBSIDIAN,VAR_3},
        {"obsidian floor",FLOOR,OBSIDIAN,VAR_4},
        {"featstone floor 1",FLOOR,FEATSTONE,VAR_1},
        {"featstone floor 2",FLOOR,FEATSTONE,VAR_2},
        {"featstone floor 3",FLOOR,FEATSTONE,VAR_3},
        {"featstone floor 4",FLOOR,FEATSTONE,VAR_4},
        {"grass 1",FLOOR,GRASS,VAR_1},
        {"grass 2",FLOOR,GRASS,VAR_2},
        
        // 350
        {"grass 3",FLOOR,GRASS,VAR_3},
        {"grass 4",FLOOR,GRASS,VAR_4},
        {"soil floor",FLOOR,SOIL,VAR_1},
        {"soil floor",FLOOR,SOIL,VAR_2},
        {"soil floor",FLOOR,SOIL,VAR_3},
        {"soil floor",FLOOR,SOIL,VAR_4},
        {"wet soil floor",FLOOR,SOIL,VAR_1},
        {"wet soil floor",FLOOR,SOIL,VAR_2},
        {"wet soil floor",FLOOR,SOIL,VAR_3},
        {"wet soil floor",FLOOR,SOIL,VAR_4},
        
        // 360
        {"ice fortification",FORTIFICATION,ICE,VAR_1},
        {"cracked ice wall",WALL,ICE,VAR_1, TILE_CRACKED},
        {"damaged ice wall",WALL,ICE,VAR_1, TILE_DAMAGED},
        {"worn ice wall",WALL,ICE,VAR_1,    TILE_WORN },
        {"ice wall",WALL,ICE,VAR_1},
        {"river N",FLOOR,SOIL,VAR_1, TILE_RIVER , "N" },
        {"river S",FLOOR,SOIL,VAR_1, TILE_RIVER , "S" },
        {"river E",FLOOR,SOIL,VAR_1, TILE_RIVER , "E" },
        {"river W",FLOOR,SOIL,VAR_1, TILE_RIVER , "W" },
        {"river NW",FLOOR,SOIL,VAR_1, TILE_RIVER, "NW"},
        
        //370
        {"river NE",FLOOR,SOIL,VAR_1, TILE_RIVER , "NE" },
        {"river SW",FLOOR,SOIL,VAR_1, TILE_RIVER , "SW" },
        {"river SE",FLOOR,SOIL,VAR_1, TILE_RIVER , "SE" },
        {"stream bed N",FLOOR,SOIL,VAR_1, TILE_STREAM , "N" },
        {"stream bed S",FLOOR,SOIL,VAR_1, TILE_STREAM , "S" },
        {"stream bed E",FLOOR,SOIL,VAR_1, TILE_STREAM , "E" },
        {"stream bed W",FLOOR,SOIL,VAR_1, TILE_STREAM , "W" },
        {"stream bed NW",FLOOR,SOIL,VAR_1, TILE_STREAM, "NW" },
        {"stream bed NE",FLOOR,SOIL,VAR_1, TILE_STREAM, "NE" },
        {"stream bed SW",FLOOR,SOIL,VAR_1, TILE_STREAM, "SW" },
        
        // 380
        {"stream bed SE",FLOOR,SOIL,VAR_1, TILE_STREAM, "SE" },
        {"stream top",FLOOR,SOIL,VAR_1, TILE_STREAM_TOP },
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"dry grass 1",FLOOR,GRASS_DRY,VAR_1},
        {"dry grass 2",FLOOR,GRASS_DRY,VAR_2},
        {"dry grass 3",FLOOR,GRASS_DRY,VAR_3},
        
        // 390
        {"dry grass 4",FLOOR,GRASS_DRY,VAR_4},
        {"dead tree",TREE_DEAD,SOIL,VAR_1},
        {"dead sapling",SAPLING_DEAD,SOIL,VAR_1},
        {"dead shrub",SHRUB_DEAD,SOIL,VAR_1},
        {"dead grass 1",FLOOR,GRASS_DEAD,VAR_1},
        {"dead grass 2",FLOOR,GRASS_DEAD,VAR_2},
        {"dead grass 3",FLOOR,GRASS_DEAD,VAR_3},
        {"dead grass 4",FLOOR,GRASS_DEAD,VAR_4},
        {"grass B1",FLOOR,GRASS2,VAR_1},
        {"grass B2",FLOOR,GRASS2,VAR_2},
        
        // 400
        {"grass B3",FLOOR,GRASS2,VAR_3},
        {"grass B4",FLOOR,GRASS2,VAR_4},
        {"boulder",BOULDER,STONE,VAR_1},
        {"obsidian boulder",BOULDER,OBSIDIAN,VAR_1},
        {"featstone? boulder",BOULDER,FEATSTONE,VAR_1},
        {"pebbles 1",PEBBLES,STONE,VAR_1},
        {"pebbles 2",PEBBLES,STONE,VAR_2},
        {"pebbles 3",PEBBLES,STONE,VAR_3},
        {"pebbles 4",PEBBLES,STONE,VAR_4},
        {"obsidian shards",PEBBLES,OBSIDIAN,VAR_1},
        
        // 410
        {"obsidian shards",PEBBLES,OBSIDIAN,VAR_2},
        {"obsidian shards",PEBBLES,OBSIDIAN,VAR_3},
        {"obsidian shards",PEBBLES,OBSIDIAN,VAR_4},
        {"featstone? pebbles",PEBBLES,FEATSTONE,VAR_1},
        {"featstone? pebbles",PEBBLES,FEATSTONE,VAR_2},
        {"featstone? pebbles",PEBBLES,FEATSTONE,VAR_3},
        {"featstone? pebbles",PEBBLES,FEATSTONE,VAR_4},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "--SS--E-"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "--S---EE"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "N-----EE" },
        
        // 420
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "NN----E-"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "N---WW--"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "NN--W---"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "--S-WW--" },
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "--SSW---"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "N-S-W-E-"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "N-S---E-"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "--S-W-E-"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "N---W-E-"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "N-S-W---"},
        
        // 430
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "--S---E-"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "N-----E-"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "N---W---"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "--S-W---"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "N-S-----"},
        {"smooth vein wall",WALL,VEIN,VAR_1 , TILE_SMOOTH , "----W-E-"},
        {"vein fortification",FORTIFICATION,VEIN,VAR_1},
        {"cracked vein wall",WALL,VEIN,VAR_1, TILE_CRACKED },
        {"damaged vein wall",WALL,VEIN,VAR_1, TILE_DAMAGED },
        {"worn vein wall",WALL,VEIN,VAR_1   , TILE_WORN },
        
        // 440
        {"vein wall",WALL,VEIN,VAR_1},
        {"vein floor",FLOOR,VEIN,VAR_1},
        {"vein floor",FLOOR,VEIN,VAR_2},
        {"vein floor",FLOOR,VEIN,VAR_3},
        {"vein floor",FLOOR,VEIN,VAR_4},
        {"vein boulder",BOULDER,VEIN,VAR_1},
        {"vein pebbles",PEBBLES,VEIN,VAR_1},
        {"vein pebbles",PEBBLES,VEIN,VAR_2},
        {"vein pebbles",PEBBLES,VEIN,VAR_3},
        {"vein pebbles",PEBBLES,VEIN,VAR_4},
        
        // 450
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "--SS--E-"},
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "--S---EE" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "N-----EE" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "NN----E-"},
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "N---WW--" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "NN--W---" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "--S-WW--" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "--SSW---" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "N-S-W-E-"},
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "N-S---E-" },
        
        // 460
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "--S-W-E-" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "N---W-E-" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "N-S-W---"},
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "--S---E-"},
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "N-----E-" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "N---W---" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "--S-W---" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "N-S-----" },
        {"smooth ice wall",WALL,ICE,VAR_1 , TILE_SMOOTH , "----W-E-"},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 470
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 480
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        
        // 490
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"constructed floor",FLOOR,CONSTRUCTED, VAR_1},
        {"constructed fortification",FORTIFICATION,CONSTRUCTED, VAR_1},
        {"constructed pillar",PILLAR,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "--SS--E-" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "--S---EE" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "N-----EE" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "NN----E-" },
        
        // 500
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "N---WW--" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "NN--W---" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "--S-WW--" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "--SSW---" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "N-S-W-E-" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "N-S---E-" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "--S-W-E-" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "N---W-E-" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "N-S-W---" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "--S---E-" },
        
        // 510
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "N-----E-" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "N---W---" },
		{"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "--S-W---" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "N-S-----" },
        {"constructed wall",WALL,CONSTRUCTED, VAR_1 ,TILE_NORMAL, "----W-E-" },
        {"constructed stair up/down",STAIR_UPDOWN,CONSTRUCTED, VAR_1},
        {"constructed stair down",STAIR_DOWN,CONSTRUCTED, VAR_1},
        {"constructed stair up",STAIR_UP,CONSTRUCTED, VAR_1},
        {"constructed ramp",RAMP,CONSTRUCTED, VAR_1},
        {0 ,EMPTY, AIR, VAR_1} // end
    };
    
    inline
    bool isWallTerrain(int in)
    {
        return tileTypeTable[in].c >= WALL && tileTypeTable[in].c <= FORTIFICATION ;
    }

    inline
    bool isFloorTerrain(int in)
    {
        return tileTypeTable[in].c >= FLOOR && tileTypeTable[in].c <= PEBBLES;
    }
    
    inline
    bool isRampTerrain(int in)
    {
        return tileTypeTable[in].c == RAMP;
    }
    
    inline
    bool isStairTerrain(int in)
    {
        return tileTypeTable[in].c >= STAIR_UP && tileTypeTable[in].c <= STAIR_UPDOWN;
    }
    
    inline
    bool isOpenTerrain(int in)
    {
        return tileTypeTable[in].c == EMPTY;
    }
    
    inline
    int getVegetationType(int in)
    {
        return tileTypeTable[in].c;
    }

	//zilpin: for convenience, when you'll be using the tile information a lot.
    inline const
    TileRow * getTileTypeP(int in)
    {
		if( in<0 || in>=TILE_TYPE_ARRAY_LENGTH ) return NULL;
        return ( const TileRow * ) &tileTypeTable[in];
    }

	//zilpin: Find the first tile entry which matches the given search criteria.
	//All parameters are optional.
	//To omit, use the 'invalid' enum for that type (e.g. tileclass_invalid, tilematerial_invalid, etc)
	//For tile directions, pass NULL to omit.
	//Returns matching index in tileTypeTable, or -1 if none found.
	inline
	int32_t findTileType( const TileClass tclass, const TileMaterial tmat, const TileVariant tvar, const TileSpecial tspecial, const TileDirection tdir )
	{
		int32_t tt;
		for(tt=0;tt<TILE_TYPE_ARRAY_LENGTH; ++tt){
			if( tclass>-1	) if( tclass != tileTypeTable[tt].c ) continue;
			if( tmat>-1		) if( tmat != tileTypeTable[tt].m ) continue;
			if( tvar>-1		) if( tvar != tileTypeTable[tt].v ) continue;
			if( tspecial>-1 ) if( tspecial != tileTypeTable[tt].s ) continue;
			if( tdir.whole  ) if( tdir.whole != tileTypeTable[tt].d.whole ) continue;
			//Match!
			return tt;
		}
		return -1;
	}
	//Convenience version of the above, to pass strings as the direction
	inline
	int32_t findTileType( const TileClass tclass, const TileMaterial tmat, const TileVariant tvar, const TileSpecial tspecial, const char *tdirStr )
	{
		if(tdirStr){
			TileDirection tdir(tdirStr);
			return findTileType(tclass,tmat,tvar,tspecial, tdir );
		}else{
			return findTileType(tclass,tmat,tvar,tspecial, NULL );
		}
	}


	//zilpin: Find a tile type similar to the one given, but with a different class.
	//Useful for tile-editing operations.
	//If no match found, returns the sourceType
	//Definitely needs improvement for wall directions, etc.
	inline
	int32_t findSimilarTileType( const int32_t sourceTileType, const TileClass tclass ){
		int32_t tt, maybe=0, match=0;
		int value=0, matchv=0;
		const TileRow *source = &tileTypeTable[sourceTileType];
		const char * sourcename = source->name;
		const uint32_t sourcenameint = *((const uint32_t *)sourcename);

#ifdef assert
		assert( sourceTileType >=0 && sourceTileType < TILE_TYPE_ARRAY_LENGTH );
#endif

		for(tt=0;tt<TILE_TYPE_ARRAY_LENGTH; ++tt){
			if( tclass == tileTypeTable[tt].c ){
				//shortcut null entries
				if(!tileTypeTable[tt].name) continue;
				//Special flag match is absolutely mandatory!
				if( source->s != tileTypeTable[tt].s ) continue;

				maybe=tt;  value=0;
				//Material is high-value match
				if( tileTypeTable[tt].m == source->m ) value|=8;
				//Direction is medium value match
				if( tileTypeTable[tt].d.whole == source->d.whole ) value|=4;
				//Variant is low-value match
				if( tileTypeTable[tt].v == source->v ) value|=1;

				//Check value against last match
				if( value>matchv ){
					match=tt;
					matchv=value;
				}
			}
		}
		if( match ) return match;
		return sourceTileType;
	}


}



#endif // TILETYPES_H_INCLUDED
