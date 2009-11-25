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

namespace DFHack
{
    enum TileClass
    {
        EMPTY,
        
        WALL,
        PILLAR,
        FORTIFICATION,
        
        STAIR_UP,
        STAIR_DOWN,
        STAIR_UPDOWN,
        
        RAMP,
        
        FLOOR,
        TREE_DEAD,
        TREE_OK,
        SAPLING_DEAD,
        SAPLING_OK,
        SHRUB_DEAD,
        SHRUB_OK,
        BOULDER,
        PEBBLES
    };
    enum TileMaterial
    {
        AIR,
        SOIL,
        STONE,
        FEATSTONE, // whatever it is
        OBSIDIAN,
        
        VEIN,
        ICE,
        GRASS,
        GRASS2,
        GRASS_DEAD,
        GRASS_DRY,
        DRIFTWOOD,
        HFS,
        MAGMA,
        CAMPFIRE,
        FIRE,
        ASHES,
        CONSTRUCTED
    };
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

    const TileRow tileTypeTable[520] =
    {
        // 0
        {"void",EMPTY, AIR, VAR_1},
        {"ramp top",EMPTY, AIR, VAR_1},
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
        {0 ,EMPTY, AIR, VAR_1},
        {"magma" ,FLOOR, MAGMA, VAR_1}, // is it really a floor?
        {"soil wall" ,WALL, SOIL, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        
        // 270
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        
        // 280
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        
        // 290
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        
        //300
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth featstone? wall",WALL,FEATSTONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        
        //310
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        
        //320
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"smooth stone wall",WALL,STONE,VAR_1},
        {"obsidian fortification",FORTIFICATION,OBSIDIAN,VAR_1},
        {"featstone? fortification",FORTIFICATION,FEATSTONE,VAR_1},
        {"cracked obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"damaged obsidian wall",WALL,OBSIDIAN,VAR_1},
        
        // 330
        {"worn obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"obsidian wall",WALL,OBSIDIAN,VAR_1},
        {"cracked stone wall",WALL,STONE,VAR_1},
        {"damaged stone wall",WALL,STONE,VAR_1},
        {"worn stone wall",WALL,STONE,VAR_1},
        {"stone wall",WALL,STONE,VAR_1},
        {"stone floor",FLOOR,STONE,VAR_1},
        {"stone floor",FLOOR,STONE,VAR_2},
        {"stone floor",FLOOR,STONE,VAR_3},
        {"stone floor",FLOOR,STONE,VAR_4},
        
        // 340
        {"obsidian floor",FLOOR,OBSIDIAN,VAR_1},
        {"obsidian floor",FLOOR,OBSIDIAN,VAR_2},
        {"obsidian floor",FLOOR,OBSIDIAN,VAR_3},
        {"obsidian floor",FLOOR,OBSIDIAN,VAR_4},
        {"featstone? floor",FLOOR,FEATSTONE,VAR_1},
        {"featstone? floor",FLOOR,FEATSTONE,VAR_2},
        {"featstone? floor",FLOOR,FEATSTONE,VAR_3},
        {"featstone? floor",FLOOR,FEATSTONE,VAR_4},
        {"grass",FLOOR,GRASS,VAR_1},
        {"grass",FLOOR,GRASS,VAR_2},
        
        // 350
        {"grass",FLOOR,GRASS,VAR_3},
        {"grass",FLOOR,GRASS,VAR_4},
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
        {"river",FLOOR,SOIL,VAR_1},
        {"river",FLOOR,SOIL,VAR_1},
        {"river",FLOOR,SOIL,VAR_1},
        {"river",FLOOR,SOIL,VAR_1},
        {"river",FLOOR,SOIL,VAR_1},
        
        //370
        {"river",FLOOR,SOIL,VAR_1},
        {"river",FLOOR,SOIL,VAR_1},
        {"river",FLOOR,SOIL,VAR_1},
        {"stream",FLOOR,SOIL,VAR_1},
        {"stream",FLOOR,SOIL,VAR_1},
        {"stream",FLOOR,SOIL,VAR_1},
        {"stream",FLOOR,SOIL,VAR_1},
        {"stream",FLOOR,SOIL,VAR_1},
        {"stream",FLOOR,SOIL,VAR_1},
        {"stream",FLOOR,SOIL,VAR_1},
        
        // 380
        {"stream",FLOOR,SOIL,VAR_1},
        {"stream top",FLOOR,SOIL,VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {0 ,EMPTY, AIR, VAR_1},
        {"dry grass",FLOOR,GRASS_DRY,VAR_1},
        {"dry grass",FLOOR,GRASS_DRY,VAR_2},
        {"dry grass",FLOOR,GRASS_DRY,VAR_3},
        
        // 390
        {"dry grass",FLOOR,GRASS_DRY,VAR_4},
        {"dead tree",TREE_DEAD,SOIL,VAR_1},
        {"dead sapling",SAPLING_DEAD,SOIL,VAR_1},
        {"dead shrub",SHRUB_DEAD,SOIL,VAR_1},
        {"dead grass",FLOOR,GRASS_DEAD,VAR_1},
        {"dead grass",FLOOR,GRASS_DEAD,VAR_2},
        {"dead grass",FLOOR,GRASS_DEAD,VAR_3},
        {"dead grass",FLOOR,GRASS_DEAD,VAR_4},
        {"grass",FLOOR,GRASS2,VAR_1},
        {"grass",FLOOR,GRASS2,VAR_2},
        
        // 400
        {"grass",FLOOR,GRASS2,VAR_3},
        {"grass",FLOOR,GRASS2,VAR_4},
        {"boulder",BOULDER,STONE,VAR_1},
        {"obsidian boulder",BOULDER,OBSIDIAN,VAR_1},
        {"featstone? boulder",BOULDER,FEATSTONE,VAR_1},
        {"pebbles",PEBBLES,STONE,VAR_1},
        {"pebbles",PEBBLES,STONE,VAR_2},
        {"pebbles",PEBBLES,STONE,VAR_3},
        {"pebbles",PEBBLES,STONE,VAR_4},
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
