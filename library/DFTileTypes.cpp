// vim: sts=4 sta et shiftwidth=4:
#include "dfhack/DFIntegers.h"
#include "dfhack/DFTileTypes.h"
#include "dfhack/DFExport.h"

namespace DFHack
{
    const TileRow tileTypeTable[TILE_TYPE_ARRAY_LENGTH] =
    {
        // 0
        {"void",EMPTY, AIR, VAR_1},
        {"ramp top",RAMP_TOP, AIR, VAR_1},
        {"pool",POOL, SOIL, VAR_1},
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
        {"chasm",ENDLESS_PIT, AIR, VAR_1},
        {"obsidian stair up/down",STAIR_UPDOWN, OBSIDIAN, VAR_1},
        {"obsidian stair down",STAIR_DOWN, OBSIDIAN, VAR_1},
        {"obsidian stair up",STAIR_UP, OBSIDIAN, VAR_1},
        {"soil stair up/down",STAIR_UPDOWN, SOIL, VAR_1},

        // 40
        {"soil stair down",STAIR_DOWN, SOIL, VAR_1},
        {"soil stair up",STAIR_UP, SOIL, VAR_1},
        {"eerie pit",ENDLESS_PIT, HFS, VAR_1},
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
        {"obsidian pillar",PILLAR, OBSIDIAN, VAR_1,TILE_SMOOTH},
        {"featstone? pillar",PILLAR, FEATSTONE, VAR_1,TILE_SMOOTH},
        {"vein pillar",PILLAR, VEIN, VAR_1,TILE_SMOOTH},
        {"ice pillar",PILLAR, ICE, VAR_1,TILE_SMOOTH},
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
        {"glowing barrier" ,WALL, CYAN_GLOW, VAR_1},
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
        {"worn obsidian wall",WALL,OBSIDIAN,VAR_1, TILE_WORN},
        {"obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"cracked featstone wall",WALL,FEATSTONE,VAR_1, TILE_CRACKED },
        {"damaged featstone wall",WALL,FEATSTONE,VAR_1, TILE_DAMAGED },
        {"worn featstone wall",WALL,FEATSTONE,VAR_1,    TILE_WORN },
        {"featstone wall",WALL,FEATSTONE,VAR_1},
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
        {"river N",RIVER_BED,SOIL,VAR_1, TILE_NORMAL, "N" },
        {"river S",RIVER_BED,SOIL,VAR_1, TILE_NORMAL, "S" },
        {"river E",RIVER_BED,SOIL,VAR_1, TILE_NORMAL, "E" },
        {"river W",RIVER_BED,SOIL,VAR_1, TILE_NORMAL, "W" },
        {"river NW",RIVER_BED,SOIL,VAR_1,TILE_NORMAL, "NW"},

        //370
        {"river NE",RIVER_BED,SOIL,VAR_1, TILE_NORMAL , "NE" },
        {"river SW",RIVER_BED,SOIL,VAR_1, TILE_NORMAL , "SW" },
        {"river SE",RIVER_BED,SOIL,VAR_1, TILE_NORMAL , "SE" },
        {"brook bed N",BROOK_BED,SOIL,VAR_1, TILE_NORMAL , "N" },
        {"brook bed S",BROOK_BED,SOIL,VAR_1, TILE_NORMAL , "S" },
        {"brook bed E",BROOK_BED,SOIL,VAR_1, TILE_NORMAL , "E" },
        {"brook bed W",BROOK_BED,SOIL,VAR_1, TILE_NORMAL , "W" },
        {"brook bed NW",BROOK_BED,SOIL,VAR_1, TILE_NORMAL, "NW" },
        {"brook bed NE",BROOK_BED,SOIL,VAR_1, TILE_NORMAL, "NE" },
        {"brook bed SW",BROOK_BED,SOIL,VAR_1, TILE_NORMAL, "SW" },

        // 380
        {"brook bed SE",BROOK_BED,SOIL,VAR_1, TILE_NORMAL, "SE" },
        {"brook top",BROOK_TOP,SOIL,VAR_1 },
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

    //set tile class string lookup table (e.g. for printing to user)
#define X(name,comment) #name,
    const char * TileShapeString[tileshape_count+1] = {
        TILESHAPE_MACRO
        0
    };
#undef X

    //string lookup table (e.g. for printing to user)
#define X(name,comment) #name,
    const char * TileMaterialString[tilematerial_count+1] = {
        TILEMATERIAL_MACRO
        0
    };
#undef X

    //string lookup table (e.g. for printing to user)
#define X(name,comment) #name,
    const char * TileSpecialString[tilespecial_count+1] = {
        TILESPECIAL_MACRO
        0
    };
#undef X

    int32_t findSimilarTileType( const int32_t sourceTileType, const TileShape tshape )
    {
        int32_t match=0;
        int value=0, matchv=0;
        const TileRow *source = &tileTypeTable[sourceTileType];

		//Shortcut.
		//If the current tile is already a shape match, leave.
		if( tshape == source->shape ) return sourceTileType;


		//Cheap pseudo-entropy, by using address of the variable on the stack.
		//No need for real random numbers.
		static int entropy;
		entropy += (int)( (void *)(&match) );
		entropy ^= ((entropy & 0xFF000000)>>24) ^ ((entropy & 0x00FF0000)>>16);


        #ifdef assert
        assert( sourceTileType >=0 && sourceTileType < TILE_TYPE_ARRAY_LENGTH );
        #endif

		//Special case for smooth pillars.
		//When you want a smooth wall, no need to search for best match.  Just use a pillar instead.
		//Choosing the right direction would require knowing neighbors.
		if( WALL==tshape && (TILE_SMOOTH==source->special || CONSTRUCTED==source->material) ){
			switch( source->material ){
				case CONSTRUCTED:      match=495;  break;
				case ICE:              match= 83;  break;
				case VEIN:             match= 82;  break;
				case FEATSTONE:        match= 81;  break;
				case OBSIDIAN:         match= 80;  break;
				case STONE:            match= 79;  break;
			}
			if( match ) return match;
		}


		//Run through until perfect match found or hit end.
        for(int32_t tt=0;tt<TILE_TYPE_ARRAY_LENGTH && value<(8|4|1); ++tt)
        {
            if( tshape == tileTypeTable[tt].shape )
            {
                //shortcut null entries
                if(!tileTypeTable[tt].name) continue;

                //Special flag match is absolutely mandatory!
                if( source->special != tileTypeTable[tt].special ) continue;

				//Special case for constructions.
				//Never turn a construction into a non-contruction.
				if( CONSTRUCTED == source->material && CONSTRUCTED != tileTypeTable[tt].material ) continue;

                value=0;
                //Material is high-value match
                if( tileTypeTable[tt].material == source->material ) value|=8;
                //Direction is medium value match
                if( tileTypeTable[tt].direction.whole == source->direction.whole ) value|=4;
                //Variant is low-value match
                if( tileTypeTable[tt].variant == source->variant ) value|=1;

                //Check value against last match.
                if( value>matchv )
                {
                    match=tt;
                    matchv=value;
                }
            }
        }

		//Post-processing for floors.
		//Give raw floors variation.
		//Variant matters, but does not matter for source.
		//Error on the side of caution.
		if( FLOOR==tshape && CONSTRUCTED!=source->material && !source->special )
		{
			//Trying to make a floor type with variants, so randomize the variant.
			//Very picky, only handle known safe tile types.
			//Some floors have 4 variants, some have 3, so the order of these matters.
			switch( match ){
				case 261:
					//Furrowed soil got chosen by accident.  Fix that.
					match=352+(3&entropy); 
					break; 
				case 336:	//STONE
				case 340:	//OBSIDIAN
				case 344:	//featstone
				case 349:	//grass
				case 352:	//soil
				case 356:	//wet soil
				case 387:	//dry grass
				case 394:	//dead grass
				case 398:	//grass B
				case 441:	//vein
					match +=  3&entropy;
					break;
				case 242:	//ASHES
				case 258:	//ICE
					match += (1&entropy) + (2&entropy);
					break;
			}
		}

        if( match ) return match;
        return sourceTileType;
    }
}
