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
    enum TileClass
    {
        EMPTY,// empty
        
        WALL,
        PILLAR,
        FORTIFICATION,
        
        STAIR_UP,
        STAIR_DOWN,
        STAIR_UPDOWN,
        
        RAMP,// ramps have no direction
        RAMP_TOP,// the top of a ramp. I assume it's used for path finding.
        
        FLOOR,// generic floor
        TREE_DEAD,
        TREE_OK,
        SAPLING_DEAD,
        SAPLING_OK,
        SHRUB_DEAD,
        SHRUB_OK,
        BOULDER,
        PEBBLES
    };
    // material -- what material the tile is made of
    enum TileMaterial
    {
        AIR,// empty
        SOIL,// ordinary soil. material depends on geology
        STONE,// ordinary layer stone. material depends on geology
        FEATSTONE,// map feature stone. used for things like hell, the hell temple or adamantine tubes. material depends on local/global feature
        OBSIDIAN,// cast obsidian
        
        VEIN,// vein stone. material depends on mineral veins present
        ICE,// frozen water... not much to say. you can determine what was on the tile before it froze by looking into the 'ice vein' objects
        GRASS,// grass (has 4 variants)
        GRASS2,// grass (has 4 variants)
        GRASS_DEAD,// dead grass (has 4 variants)
        GRASS_DRY,// dry grass (has 4 variants)
        DRIFTWOOD,// non-specified wood - normally on top of the local layer stone/soil.
        HFS,// the stuff demon pits are made of - this makes them different from ordinary pits.
        MAGMA,// material for semi-molten rock and 'magma flow' tiles
        CAMPFIRE,// human armies make them when they siege. The original tile may be lost?
        FIRE,// burning grass
        ASHES,// what remains from a FIRE
        CONSTRUCTED,// tile material depends on the construction present
        CYAN_GLOW// the glowy stuff that disappears from the demon temple when you take the sword.
    };
    // variants are used for tiles, where there are multiple variants of the same - like grass floors
    enum TileVariant
    {
        VAR_1,
        VAR_2,
        VAR_3,
        VAR_4
    };
    
    struct TileRow
    {
        const char * name;
        TileClass c;
        TileMaterial m;
        TileVariant v;
    };
	
	#define TILE_TYPE_ARRAY_LENGTH 520

    const TileRow tileTypeTable[TILE_TYPE_ARRAY_LENGTH] =
    {
        // 0
        {"void",EMPTY, AIR, VAR_1},
        {"ramp top",RAMP_TOP, AIR, VAR_1},
        {"pool",FLOOR, SOIL, VAR_1},
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
        {"chasm",FLOOR, AIR, VAR_1},
        {"obsidian stair up/down",STAIR_UPDOWN, OBSIDIAN, VAR_1},
        {"obsidian stair down",STAIR_DOWN, OBSIDIAN, VAR_1},
        {"obsidian stair up",STAIR_UP, OBSIDIAN, VAR_1},
        {"soil stair up/down",STAIR_UPDOWN, SOIL, VAR_1},
        
        // 40
        {"soil stair down",STAIR_DOWN, SOIL, VAR_1},
        {"soil stair up",STAIR_UP, SOIL, VAR_1},
        {"eerie pit",FLOOR, HFS, VAR_1},
        {"smooth stone floor",FLOOR, STONE, VAR_1},
        {"smooth obsidian floor",FLOOR, OBSIDIAN, VAR_1},
        {"smooth featstone? floor",FLOOR, FEATSTONE, VAR_1},
        {"smooth vein floor",FLOOR, VEIN, VAR_1},
        {"smooth ice floor",FLOOR, ICE, VAR_1},
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
        {"waterfall landing",FLOOR, SOIL, VAR_1}, // verify material
        
        // 90
        {"river source",FLOOR, SOIL, VAR_1}, // verify material
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
        {"cracked stone wall" ,WALL, STONE, VAR_1},
        {"damaged stone wall" ,WALL, STONE, VAR_1},
        {"worn stone wall" ,WALL, STONE, VAR_1},
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
        {"smooth obsidian wall RD2",WALL,OBSIDIAN,VAR_1},
        
        // 270
        {"smooth obsidian wall R2D",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall R2U",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall RU2",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall L2U",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall LU2",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall L2D",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall LD2",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall LRUD",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall RUD",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall LRD",WALL,OBSIDIAN,VAR_1},
        
        // 280
        {"smooth obsidian wall LRU",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall LUD",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall RD",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall RU",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall LU",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall LD",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall UD",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall LR",WALL,OBSIDIAN,VAR_1},
        {"smooth featstone wall RD2",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall R2D",WALL,FEATSTONE,VAR_1},
        
        // 290
        {"smooth featstone wall R2U",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall RU2",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall L2U",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall LU2",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall L2D",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall LD2",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall LRUD",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall RUD",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall LRD",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall LRU",WALL,FEATSTONE,VAR_1},
        
        //300
        {"smooth featstone wall LUD",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall RD",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall RU",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall LU",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall LD",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall UD",WALL,FEATSTONE,VAR_1},
        {"smooth featstone wall LR",WALL,FEATSTONE,VAR_1},
        {"smooth stone wall RD2",WALL,STONE,VAR_1},
        {"smooth stone wall R2D",WALL,STONE,VAR_1},
        {"smooth stone wall R2U",WALL,STONE,VAR_1},
        
        //310
        {"smooth stone wall RU2",WALL,STONE,VAR_1},
        {"smooth stone wall L2U",WALL,STONE,VAR_1},
        {"smooth stone wall LU2",WALL,STONE,VAR_1},
        {"smooth stone wall L2D",WALL,STONE,VAR_1},
        {"smooth stone wall LD2",WALL,STONE,VAR_1},
        {"smooth stone wall LRUD",WALL,STONE,VAR_1},
        {"smooth stone wall RUD",WALL,STONE,VAR_1},
        {"smooth stone wall LRD",WALL,STONE,VAR_1},
        {"smooth stone wall LRU",WALL,STONE,VAR_1},
        {"smooth stone wall LUD",WALL,STONE,VAR_1},
        
        //320
        {"smooth stone wall RD",WALL,STONE,VAR_1},
        {"smooth stone wall RU",WALL,STONE,VAR_1},
        {"smooth stone wall LU",WALL,STONE,VAR_1},
        {"smooth stone wall LD",WALL,STONE,VAR_1},
        {"smooth stone wall UD",WALL,STONE,VAR_1},
        {"smooth stone wall LR",WALL,STONE,VAR_1},
        {"obsidian fortification",FORTIFICATION,OBSIDIAN,VAR_1},
        {"featstone? fortification",FORTIFICATION,FEATSTONE,VAR_1},
        {"cracked obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"damaged obsidian wall",WALL,OBSIDIAN,VAR_1},
        
        // 330
        {"worn obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"obsidian wall",WALL,OBSIDIAN,VAR_1},
                /*MAPTILE_FEATSTONE_WALL_WORN1,
                    MAPTILE_FEATSTONE_WALL_WORN2,
                      MAPTILE_FEATSTONE_WALL_WORN3,
                        MAPTILE_FEATSTONE_WALL,*/
        {"cracked featstone wall",WALL,STONE,VAR_1},
        {"damaged featstone wall",WALL,STONE,VAR_1},
        {"worn featstone wall",WALL,STONE,VAR_1},
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
        {"cracked ice wall",WALL,ICE,VAR_1},
        {"damaged ice wall",WALL,ICE,VAR_1},
        {"worn ice wall",WALL,ICE,VAR_1},
        {"ice wall",WALL,ICE,VAR_1},
        {"river N",FLOOR,SOIL,VAR_1},
        {"river S",FLOOR,SOIL,VAR_1},
        {"river E",FLOOR,SOIL,VAR_1},
        {"river W",FLOOR,SOIL,VAR_1},
        {"river NW",FLOOR,SOIL,VAR_1},
        
        //370
        {"river NE",FLOOR,SOIL,VAR_1},
        {"river SW",FLOOR,SOIL,VAR_1},
        {"river SE",FLOOR,SOIL,VAR_1},
        {"stream bed N",FLOOR,SOIL,VAR_1},
        {"stream bed S",FLOOR,SOIL,VAR_1},
        {"stream bed E",FLOOR,SOIL,VAR_1},
        {"stream bed W",FLOOR,SOIL,VAR_1},
        {"stream bed NW",FLOOR,SOIL,VAR_1},
        {"stream bed NE",FLOOR,SOIL,VAR_1},
        {"stream bed SW",FLOOR,SOIL,VAR_1},
        
        // 380
        {"stream bed SE",FLOOR,SOIL,VAR_1},
        {"stream top",FLOOR,SOIL,VAR_1},
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
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        
        // 420
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        
        // 430
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"smooth vein wall",WALL,VEIN,VAR_1},
        {"vein fortification",FORTIFICATION,VEIN,VAR_1},
        {"cracked vein wall",WALL,VEIN,VAR_1},
        {"damaged vein wall",WALL,VEIN,VAR_1},
        {"worn vein wall",WALL,VEIN,VAR_1},
        
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
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        
        // 460
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
        {"smooth ice wall",WALL,ICE,VAR_1},
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
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        
        // 500
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        
        // 510
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
        {"constructed wall",WALL,CONSTRUCTED, VAR_1},
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
}



#endif // TILETYPES_H_INCLUDED
