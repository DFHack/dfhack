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

struct t_matgloss
{
       char id[128];
       uint8_t fore; // Annoyingly the offset for this differs between types
       uint8_t back;
       uint8_t bright;
};
struct t_vein
{
    uint32_t vtable;
    int16_t type;
    int16_t assignment[16];
    int16_t unknown;
    uint32_t flags;
};

struct t_matglossPair
{
    int16_t type;
    int16_t index;
};

// raw
struct t_construction_df40d
{
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t unk1;
    int16_t unk2;
    t_matglossPair material; // 4B
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

struct t_tree_desc
{
    t_matglossPair material;
    uint16_t x;
    uint16_t y;
    uint16_t z;
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

// FIXME: in order in which the raw vectors appear in df memory, move to XML
enum MatglossType
{
    Mat_Wood,
    Mat_Stone,
    Mat_Plant,
    Mat_Metal,
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
    Mat_Soap = 24,
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

union t_creaturflags1
{
    uint32_t whole;
    struct {
        //0000 0001 - 0000 0080
        unsigned int unk1 : 1;
        unsigned int dead : 1;
        unsigned int unk3 : 1;
        unsigned int mood_survivor : 1;
        unsigned int hostile : 1;
        unsigned int unk6 : 1;
        unsigned int unk7_friendly : 1;
        unsigned int unk8_friendly : 1;
        
        //0000 0100 - 0000 8000
        unsigned int unk9_not_on_unit_screen1 : 1;
        unsigned int unk10 : 1;
        unsigned int unk11_not_on_unit_screen2 : 1;
        unsigned int unk12_friendly : 1;

        unsigned int zombie : 1;
        unsigned int skeletal : 1;
        unsigned int unk15_not_part_of_fortress : 1; // resets to 0?
        unsigned int unconscious : 1;
        
        // 0001 0000 - 0080 0000
        unsigned int unk17_not_visible : 1; // hidden? caged?
        unsigned int invader1 : 1;
        unsigned int unk19_not_listed_among_dwarves : 1;
        unsigned int invader2 : 1;
        
        unsigned int unk21 : 1;
        unsigned int hidden_ambusher : 1; // maybe
        unsigned int unk23 : 1;
        unsigned int unk24 : 1;

        // 0100 0000 - 8000 0000
        unsigned int unk25 : 1;
        unsigned int unk26_invisible_hidden : 1;
        unsigned int tame : 1;
        unsigned int unk28 : 1;
        
        unsigned int royal_guard : 1;
        unsigned int fortress_guard : 1;
        unsigned int unk31 : 1;
        unsigned int unk32 : 1;
        
    } bits;
};

union t_creaturflags2
{
    uint32_t whole;
    struct {
        //0000 0001 - 0000 0080
        unsigned int unk1 : 1;
        unsigned int unk2 : 1;
        unsigned int unk3 : 1;
        unsigned int unk4 : 1;
        unsigned int unk5 : 1;
        unsigned int unk6 : 1;
        unsigned int unk7 : 1; // commonly set on dwarves
        unsigned int dead : 1; // another dead bit
        
        //0000 0100 - 0000 8000
        unsigned int unk9 : 1;
        unsigned int unk10 : 1;
        unsigned int unk11 : 1;
        unsigned int unk12 : 1;
        unsigned int unk13 : 1;
        unsigned int unk14 : 1;
        unsigned int unk15 : 1;
        unsigned int ground : 1;
        
        // 0001 0000 - 0080 0000
        unsigned int flying : 1;
        unsigned int slaughter : 1;
        unsigned int underworld : 1;
        unsigned int unk20 : 1;
        unsigned int unk21 : 1;
        unsigned int unk22 : 1;
        unsigned int unk23 : 1;
        unsigned int unk24 : 1;
        
        // 0100 0000 - 8000 0000
        unsigned int unk25 : 1;
        unsigned int unk26 : 1;
        unsigned int unk27 : 1;
        unsigned int unk28 : 1;
        unsigned int unk29 : 1;
        unsigned int unk30 : 1;
        unsigned int unk31 : 1;
        unsigned int unk32 : 1;
        
    } bits;
};

struct t_creature
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint32_t type;
    t_creaturflags1 flags1;
    t_creaturflags2 flags2;
};

// TODO: research this further? consult DF hacker wizards?
union t_designation
{
    uint32_t whole;
    struct {
    unsigned int flow_size : 3; // how much liquid is here?
    unsigned int pile : 1; // stockpile?
/*
 * All the different dig designations... needs more info, probably an enum
 */
    unsigned int dig : 3;
    unsigned int detail : 1;///<- wtf
    unsigned int detail_event : 1;///<- more wtf
    unsigned int hidden :1;

/*
 * This one is rather involved, but necessary to retrieve the base layer matgloss index
 * see http://www.bay12games.com/forum/index.php?topic=608.msg253284#msg253284 for details
 */
    unsigned int geolayer_index :4;
    unsigned int light : 1;
    unsigned int subterranean : 1; // never seen the light of day?
    unsigned int skyview : 1; // sky is visible now, it rains in here when it rains

/*
 * Probably similar to the geolayer_index. Only with a different set of offsets and different data.
 * we don't use this yet
 */
    unsigned int biome : 4;
/*
0 = water
1 = magma
*/
    unsigned int liquid_type : 1;
    unsigned int water_table : 1; // srsly. wtf?
    unsigned int rained : 1; // does this mean actual rain (as in the blue blocks) or a wet tile?
    unsigned int traffic : 2; // needs enum
    unsigned int flow_forbid : 1; // idk wtf bbq
    unsigned int liquid_static : 1;
    unsigned int moss : 1;// I LOVE MOSS
    unsigned int feature_present : 1; // another wtf... is this required for magma pipes to work?
    unsigned int liquid_character : 2; // those ripples on streams?
    } bits;
};

// occupancy flags (rat,dwarf,horse,built wall,not build wall,etc)
union t_occupancy
{
    uint32_t whole;
    struct {
    unsigned int building : 3;// building type... should be an enum?
    // 7 = door
    unsigned int unit : 1;
    unsigned int unit_grounded : 1;
    unsigned int item : 1;
    // splatter. everyone loves splatter.
    unsigned int mud : 1;
    unsigned int vomit :1;
    unsigned int debris1 :1;
    unsigned int debris2 :1;
    unsigned int debris3 :1;
    unsigned int debris4 :1;
    unsigned int blood_g : 1;
    unsigned int blood_g2 : 1;
    unsigned int blood_b : 1;
    unsigned int blood_b2 : 1;
    unsigned int blood_y : 1;
    unsigned int blood_y2 : 1;
    unsigned int blood_m : 1;
    unsigned int blood_m2 : 1;
    unsigned int blood_c : 1;
    unsigned int blood_c2 : 1;
    unsigned int blood_w : 1;
    unsigned int blood_w2 : 1;
    unsigned int blood_o : 1;
    unsigned int blood_o2 : 1;
    unsigned int slime : 1;
    unsigned int slime2 : 1;
    unsigned int blood : 1;
    unsigned int blood2 : 1;
    unsigned int debris5 : 1;
    unsigned int snow : 1;
    } bits;
    struct {
        unsigned int building : 3;// building type... should be an enum?
        // 7 = door
        unsigned int unit : 1;
        unsigned int unit_grounded : 1;
        unsigned int item : 1;
        // splatter. everyone loves splatter.
        unsigned int splatter : 26;
    } unibits;
};

#endif // TYPES_H_INCLUDED
