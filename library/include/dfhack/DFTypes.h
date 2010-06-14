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

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include "DFPragma.h"
#include "DFExport.h"

namespace DFHack
{

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
    unsigned int on_ground : 1; // Item on ground
    unsigned int in_job : 1; // item currently being used in a job
    unsigned int in_inventory : 1; // Item in a creatures inventory
    unsigned int u_ngrd1 : 1; // only occurs when not on ground, unknown function
    
    unsigned int in_workshop : 1; // Item is in a workshops inventory
    unsigned int u_ngrd2 : 1; // only occurs when not on ground, unknown function
    unsigned int u_ngrd3 : 1; // only occurs when not on ground, unknown function
    unsigned int rotten : 1; // Item is rotten
    
    unsigned int unk1 : 1; // unknown function
    unsigned int u_ngrd4 : 1; // only occurs when not on ground, unknown function
    unsigned int unk2 : 1; // unknown function
    unsigned int u_ngrd5 : 1; // only occurs when not on ground, unknown function

    unsigned int unk3 : 1; // unknown function
    unsigned int u_ngrd6 : 1; // only occurs when not on ground, unknown function
    unsigned int narrow : 1; // Item is narrow
    unsigned int u_ngrd7 : 1; // only occurs when not on ground, unknown function
            
    unsigned int worn : 1; // item shows wear
    unsigned int unk4 : 1; // unknown function
    unsigned int u_ngrd8 : 1; // only occurs when not on ground, unknown function
    unsigned int forbid : 1; // designate forbid item
    
    unsigned int unk5 : 1; // unknown function
    unsigned int dump : 1; // designate dump item
    unsigned int on_fire: 1; //indicates if item is on fire, Will Set Item On Fire if Set!
    unsigned int melt : 1; // designate melt item, if item cannot be melted, does nothing it seems
    
    // 0100 0000 - 8000 0000
    unsigned int hidden : 1; // designate hide item
    unsigned int u_ngrd9 : 1; // only occurs when not on ground, unknown function
    unsigned int unk6 : 1; // unknown function
    unsigned int unk7 : 1; // unknown function
    
    unsigned int unk8 : 1; // unknown function
    unsigned int unk9 : 1; // unknown function
    unsigned int unk10 : 1; // unknown function
    unsigned int unk11 : 1; // unknown function
};

union t_itemflags
{
    uint32_t whole;
    naked_itemflags bits;
};

//cooked
struct t_viewscreen 
{
    int32_t type;
    //There is more info in these objects, but I don't know what it is yet
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

struct t_level
{
	uint32_t level;
	std::string name;
	uint32_t xpNxtLvl;
};

}// namespace DFHack
#endif // TYPES_H_INCLUDED
