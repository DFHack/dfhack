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

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include "DFPragma.h"
#include "DFExport.h"

#ifdef __cplusplus
namespace DFHack
{
#endif

struct t_matglossPair
{
    int16_t type;
    int32_t index;
};

// DF effects, by darius from the bay12 forum

template <int SIZE>
struct junk_fill
{
    uint8_t data[SIZE];
};

enum EFFECT_TYPE
{
    EFF_MIASMA=0,
    EFF_WATER,
    EFF_WATER2,
    EFF_BLOOD,
    EFF_DUST,
    EFF_MAGMA,
    EFF_SMOKE,
    EFF_DRAGONFIRE,
    EFF_FIRE,
    EFF_WEBING,
    EFF_BOILING, // uses matgloss
    EFF_OCEANWAVE
};

struct t_effect_df40d //size 40
{
    uint16_t type;
    t_matglossPair material;
    int16_t lifetime;
    uint16_t x;
    uint16_t y;
    uint16_t z; //14
    int16_t x_direction;
    int16_t y_direction;
    junk_fill <12> unk4;
    uint8_t canCreateNew;//??
    uint8_t isHidden;
};

/*
        dword vtable;
        int minx;
        int miny;
        int centerx;
        int maxx;
        int maxy;
        int centery;
        int z;
        dword height_not_used;
        word  mattype;
        word  matgloss;
        word  type; // NOTE: the actual field is in a different place
*/

//#pragma pack(push,4)
struct t_name
{
    char first_name[128];
    char nickname[128];
    int32_t words[7];
    uint16_t parts_of_speech[7];
    uint32_t language;
    bool has_name;
};

//raw
struct t_item_df40d
{
    uint32_t vtable;
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint32_t flags;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t ID;
    // not complete
};

//From http://dwarffortresswiki.net/index.php/User:Rick/Memory_research
//They all seem to be valid on 40d as well
struct naked_itemflags
{
    unsigned int on_ground : 1;    // 0000 0001 Item on ground
    unsigned int in_job : 1;       // 0000 0002 Item currently being used in a job
    unsigned int u_ngrd1 : 1;      // 0000 0004 unknown, unseen
    unsigned int in_inventory : 1; // 0000 0008 Item in a creature or workshop inventory

    unsigned int u_ngrd2 : 1;      // 0000 0010 unknown, lost (artifact)?, unseen
    unsigned int in_building : 1;  // 0000 0020 Part of a building (including mechanisms, bodies in coffins)
    unsigned int u_ngrd3 : 1;      // 0000 0040 unknown, unseen
    unsigned int dead_dwarf : 1;   // 0000 0080 Dwarf's dead body or body part

    unsigned int rotten : 1;       // 0000 0100 Rotten food
    unsigned int spider_web : 1;   // 0000 0200 Thread in spider web
    unsigned int construction : 1; // 0000 0400 Material used in construction
    unsigned int u_ngrd5 : 1;      // 0000 0800 unknown, unseen

    unsigned int unk3 : 1;         // 0000 1000 unknown, unseen
    unsigned int u_ngrd6 : 1;      // 0000 2000 unknown, unseen
    unsigned int foreign : 1;      // 0000 4000 Item is imported
    unsigned int u_ngrd7 : 1;      // 0000 8000 unknown, unseen

    unsigned int owned : 1;        // 0001 0000 Item is owned by a dwarf
    unsigned int unk4 : 1;         // 0002 0000 unknown, unseen
    unsigned int artifact1 : 1;    // 0004 0000 Artifact ?
    unsigned int forbid : 1;       // 0008 0000 Forbidden item
    
    unsigned int unk5 : 1;         // 0010 0000 unknown, unseen
    unsigned int dump : 1;         // 0020 0000 Designated for dumping
    unsigned int on_fire: 1;       // 0040 0000 Indicates if item is on fire, Will Set Item On Fire if Set!
    unsigned int melt : 1;         // 0080 0000 Designated for melting, if applicable
    
    // 0100 0000 - 8000 0000
    unsigned int hidden : 1;       // 0100 0000 Hidden item
    unsigned int in_chest : 1;     // 0200 0000 Stored in chest/part of well?
    unsigned int unk6 : 1;         // 0400 0000 unknown, unseen
    unsigned int artifact2 : 1;    // 0800 0000 Artifact ?
    
    unsigned int unk8 : 1;         // 1000 0000 unknown, unseen
    unsigned int unk9 : 1;         // 2000 0000 unknown, set when viewing details
    unsigned int unk10 : 1;        // 4000 0000 unknown, unseen
    unsigned int unk11 : 1;        // 8000 0000 unknown, unseen
};

union t_itemflags
{
    uint32_t whole;
    naked_itemflags bits;
};


struct t_note
{
    char symbol;
    uint16_t foreground;
    uint16_t background;
    char name[128];
    uint16_t x;
    uint16_t y;
    uint16_t z;
};


// local are numbered with top left as 0,0, name is indexes into the item vector
struct t_settlement
{
    uint32_t origin;
    t_name name;
    int16_t world_x;
    int16_t world_y;
    int16_t local_x1;
    int16_t local_x2;
    int16_t local_y1;
    int16_t local_y2;
};
    
struct t_attrib
{
    uint32_t level;
    uint32_t field_4; // offset from beginning, purpose unknown
    uint32_t field_8;
    uint32_t field_C;
    uint32_t leveldiff;
    uint32_t field_14;
    uint32_t field_18;
};

#ifdef __cplusplus
struct t_level
{
    uint32_t level;
    std::string name;
    uint32_t xpNxtLvl;
};

}// namespace DFHack
#endif

#endif // TYPES_H_INCLUDED
