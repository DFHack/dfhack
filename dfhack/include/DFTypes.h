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

#include "Tranquility.h"
#include "Export.h"

namespace DFHack
{

template <int SIZE>
struct junk_fill
{
    uint8_t data[SIZE];
    /*
    void Dump()
    {
        cout<<hex;
        for (int i=0;i<SIZE;i++)
            cout<<setw(2)<<i<<" ";
        cout<<endl;
        for (int i=0;i<SIZE;i++)
        {
            cout<<setw(2)<<(int)data[i]<<" ";
            if ((i%32)==32-1)
                cout<<endl;
        }
        cout<<endl;
    }
    */
};
    
/*
struct t_vein
{
    uint32_t vtable;
    int16_t type;
    int16_t assignment[16];
    int16_t unknown;
    uint32_t flags;
    uint32_t address_of; // this is NOT part of the DF vein, but an address of the vein as seen by DFhack.
};
*/


struct t_matglossPair
{
    int16_t type;
    int16_t index;
};

// DF effects, by darius from the bay12 forum
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

// raw
struct t_construction_df40d
{
    int16_t x;
    int16_t y;
    // 4
    int16_t z;
    int16_t unk1;
    // 8
    int16_t unk2;
    t_matglossPair material; // C points to the index part
//    int16_t mat_type;
//    int16_t mat_idx;
};

// cooked
struct t_construction
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
    t_matglossPair material;
//    int16_t mat_type;
//    int16_t mat_idx;
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

//raw
struct t_building_df40d
{
    uint32_t vtable;
    uint32_t x1;
    uint32_t y1;
    uint32_t centerx;
    uint32_t x2;
    uint32_t y2;
    uint32_t centery;
    uint32_t z;
    uint32_t height;
    t_matglossPair material;
    // not complete
};

//cooked
struct t_building
{
    uint32_t origin;
    uint32_t vtable;

    uint32_t x1;
    uint32_t y1;

    uint32_t x2;
    uint32_t y2;

    uint32_t z;

    t_matglossPair material;

    uint32_t type;
    // FIXME: not complete, we need building presence bitmaps for stuff like farm plots and stockpiles, orientation (N,E,S,W) and state (open/closed)
};

/*
case 10:
    ret += "leather";
    break;
case 11:
    ret += "silk cloth";
    break;
case 12:
    ret += "plant thread cloth";
    break;
case 13: // green glass
    ret += "green glass";
    break;
case 14: // clear glass
    ret += "clear glass";
    break;
case 15: // crystal glass
    ret += "crystal glass";
    break;
case 17:
    ret += "ice";
    break;
case 18:
    ret += "charcoal";
    break;
case 19:
    ret += "potash";
    break;
case 20:
    ret += "ashes";
    break;
case 21:
    ret += "pearlash";
    break;
case 24:
    ret += "soap";
    break;

*/

enum MatglossType
{
    Mat_Wood,
    Mat_Stone,
    Mat_Metal,
    Mat_Plant,
    Mat_Leather = 10,
    Mat_SilkCloth = 11,
    Mat_PlantCloth = 12,
    Mat_GreenGlass = 13,
    Mat_ClearGlass = 14,
    Mat_CrystalGlass = 15,
    Mat_Ice = 17,
    Mat_Charcoal =18,
    Mat_Potash = 20,
    Mat_Ashes = 20,
    Mat_PearlAsh = 21,
    Mat_Soap = 24
    //NUM_MATGLOSS_TYPES
};

enum BiomeOffset
{
    eNorthWest,
    eNorth,
    eNorthEast,
    eWest,
    eHere,
    eEast,
    eSouthWest,
    eSouth,
    eSouthEast,
    eBiomeCount
};

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
struct t_item
{
    uint32_t origin;
    uint32_t vtable;

    uint32_t x;
    uint32_t y;
    uint32_t z;
    
    t_itemflags flags;
    uint32_t ID;
    uint32_t type;
    t_matglossPair material;
    /*
    uint8_t matType;
    uint8_t material;
    */
 //   vector<uint8_t> bytes; used for item research
    // FIXME: not complete, we need building presence bitmaps for stuff like farm plots and stockpiles, orientation (N,E,S,W) and state (open/closed)
};

// can add more here later, but this is fine for now
struct t_itemType
{
    char id[128];
    char name[128];
};

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

#define NUM_HOTKEYS 16
struct t_hotkey
{
    char name[10];
    int16_t mode;
    int32_t x;
    int32_t y;
    int32_t z;
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

}// namespace DFHack
#endif // TYPES_H_INCLUDED
