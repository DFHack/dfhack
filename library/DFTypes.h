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

#include "Export.h"

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
/*
bits:

0    Can the dwarf move or are they waiting for their movement timer
1   Dead (might also be set for incoming/leaving critters that are alive)
2   Currently in mood
3   Had a mood
4   "marauder" -- wide class of invader/inside creature attackers
5   Drowning
6   Active merchant
7   "forest" (used for units no longer linked to merchant/diplomacy, they just try to leave mostly)
8   Left (left the map)
9   Rider
10  Incoming
11  Diplomat
12  Zombie
13  Skeleton
14  Can swap tiles during movement (prevents multiple swaps)
15  On the ground (can be conscious)
16  Projectile
17  Active invader (for organized ones)
18  Hidden in ambush
19  Invader origin (could be inactive and fleeing)
20  Will flee if invasion turns around
21  Active marauder/invader moving inward
22  Marauder resident/invader moving in all the way
23  Check against flows next time you get a chance
24  Ridden
25  Caged
26  Tame
27  Chained
28  Royal guard
29  Fortress guard
30  Suppress wield for beatings/etc
31  Is an important historical figure 
*/

union t_creaturflags1
{
    uint32_t whole;
    struct {
        unsigned int move_state : 1; // Can the dwarf move or are they waiting for their movement timer
        unsigned int dead : 1; // might also be set for incoming/leaving critters that are alive
        unsigned int has_mood : 1; // Currently in mood
        unsigned int had_mood : 1; // Had a mood
        
        unsigned int marauder : 1; // wide class of invader/inside creature attackers
        unsigned int drowning : 1;
        unsigned int merchant : 1; // active merchant
        unsigned int forest : 1; // used for units no longer linked to merchant/diplomacy, they just try to leave mostly
        
        unsigned int left : 1; // left the map
        unsigned int rider : 1;
        unsigned int incoming : 1;
        unsigned int diplomat : 1;

        unsigned int zombie : 1;
        unsigned int skeleton : 1;
        unsigned int can_swap : 1; // Can swap tiles during movement (prevents multiple swaps)
        unsigned int on_ground : 1; // can be conscious
        
        unsigned int projectile : 1;
        unsigned int active_invader : 1; // for organized ones
        unsigned int hidden_in_ambush : 1;
        unsigned int invader_origin : 1; // could be inactive and fleeing
        
        unsigned int coward : 1; // Will flee if invasion turns around
        unsigned int hidden_ambusher : 1; // maybe
        unsigned int invades : 1; // Active marauder/invader moving inward
        unsigned int check_flows : 1; // Check against flows next time you get a chance

        // 0100 0000 - 8000 0000
        unsigned int ridden : 1;
        unsigned int caged : 1;
        unsigned int tame : 1;
        unsigned int chained : 1;
        
        unsigned int royal_guard : 1;
        unsigned int fortress_guard : 1;
        unsigned int suppress_wield : 1; // Suppress wield for beatings/etc
        unsigned int important_historical_figure : 1; // Is an important historical figure 
        
    } bits;
};

/*
bits:

0    Swimming
1   Play combat for sparring
2   Do not notify about level gains (for embark etc)
3   Unused

4   Nerves calculated
5   Body part info calculated
6   Is important historical figure (slight variation)
7   Has been killed by kill function (slightly different from dead, not necessarily violent death)

8   Must be forgotten by forget function (just cleanup)
9   Must be deleted (cleanup)
10  Recently forgotten (cleanup)
11  Offered for trade

12  Trade resolved
13  Has breaks
14  Gutted
15  Circulatory spray

16  Locked in for trading (it's a projectile on the other set of flags, might be what the flying was)
17  Marked for slaughter
18  Underworld creature
19  Current resident

20  Marked for special cleanup as unused load from unit block on disk
21  Insulation from clothing calculated
22  Uninvited guest
23  Visitor

24  Inventory order calculated
25  Vision -- have good part
26  Vision -- have damaged part
27  Vision -- have missing part

28  Breathing -- have good part
29  Breathing -- having a problem
30  Roaming wilderness population source
31  Roaming wilderness population source -- not a map feature 
*/

union t_creaturflags2
{
    uint32_t whole;
    struct {
        unsigned int swimming : 1;
        unsigned int sparring : 1;
        unsigned int no_notify : 1; // Do not notify about level gains (for embark etc)
        unsigned int unused : 1;
        
        unsigned int calculated_nerves : 1;
        unsigned int calculated_bodyparts : 1;
        unsigned int important_historical_figure : 1; // slight variation
        unsigned int killed : 1; // killed by kill() function
        
        unsigned int cleanup_1 : 1; // Must be forgotten by forget function (just cleanup)
        unsigned int cleanup_2 : 1; // Must be deleted (cleanup)
        unsigned int cleanup_3 : 1; // Recently forgotten (cleanup)
        unsigned int for_trade : 1; // Offered for trade
        
        unsigned int trade_resolved : 1;
        unsigned int has_breaks : 1;
        unsigned int gutted : 1;
        unsigned int circulatory_spray : 1;
        
        unsigned int locked_in_for_trading : 1;
        unsigned int slaughter : 1; // marked for slaughter
        unsigned int underworld : 1; // Underworld creature
        unsigned int resident : 1; // Current resident
        
        unsigned int cleanup_4 : 1; // Marked for special cleanup as unused load from unit block on disk
        unsigned int calculated_insulation : 1; // Insulation from clothing calculated
        unsigned int visitor_uninvited : 1; // Uninvited guest
        unsigned int visitor : 1; // visitor
        
        unsigned int calculated_inventory : 1; // Inventory order calculated
        unsigned int vision_good : 1; // Vision -- have good part
        unsigned int vision_damaged : 1; // Vision -- have damaged part
        unsigned int vision_missing : 1; // Vision -- have missing part
        
        unsigned int breathing_good : 1; // Breathing -- have good part
        unsigned int breathing_problem : 1; // Breathing -- having a problem
        unsigned int roaming_wilderness_population_source : 1;
        unsigned int roaming_wilderness_population_source_not_a_map_feature : 1;
        
    } bits;
};
/*
struct t_labor
{
    string name;
    uint8_t value;
    t_labor() { 
        value =0;
    }
    t_labor(const t_labor & b){
        name=b.name;
        value=b.value;
    }
    t_labor & operator=(const t_labor &b){
        name=b.name;
        value=b.value;
        return *this;
    }
};
struct t_skill
{
    string name;
    uint16_t id;
    uint32_t experience;
    uint16_t rating;
    t_skill(){
        id=rating=0;
        experience=0;
    }
    t_skill(const t_skill & b)
    {
        name=b.name;
        id=b.id;
        experience=b.experience;
        rating=b.rating;
    }
    t_skill & operator=(const t_skill &b)
    {
        name=b.name;
        id=b.id;
        experience=b.experience;
        rating=b.rating;
        return *this;
    }
};

struct t_trait
{
    uint16_t value;
    string displayTxt;
    string name;
    t_trait(){
        value=0;
    }
    t_trait(const t_trait &b)
    {
        name=b.name;
        displayTxt=b.displayTxt;
        value=b.value;
    }
    t_trait & operator=(const t_trait &b)
    {
        name=b.name;
        displayTxt=b.displayTxt;
        value=b.value;
        return *this;
    }
};
*/

/*
CREATURE
*/

struct t_lastname
{
    int names[7];
};
struct t_squadname
{
    int names[6];
};
struct t_skill
{
    uint16_t id;
    uint32_t experience;
    uint16_t rating;
};
struct t_job
{
    bool active;
    uint8_t jobId;
};

#define NUM_CREATURE_TRAITS 30
#define NUM_CREATURE_LABORS 102
struct t_creature
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint32_t type;
    t_creaturflags1 flags1;
    t_creaturflags2 flags2;
    char first_name [128];
    char nick_name [128];
    t_lastname last_name;
    t_squadname squad_name;
    uint8_t profession;
    char custom_profession[128];
    // enabled labors
    uint8_t labors[NUM_CREATURE_LABORS];
    // personality traits
    uint16_t traits[NUM_CREATURE_TRAITS];
    uint8_t numSkills;
    t_skill skills[256];
    /*
    //string last_name;
    string current_job;
    */
    t_job current_job;
    uint32_t happiness;
    uint32_t id;
    uint32_t agility;
    uint32_t strength;
    uint32_t toughness;
    uint32_t money;
    int32_t squad_leader_id;
    uint8_t sex;
    /*
    vector <t_skill> skills;
    vector <t_trait> traits;
    vector <t_labor> labors;
    t_creature() { 
        x=y=z=0;
        type=happiness=id=agility=strength=toughness=money=0;
        squad_leader_id = -1;
        sex=0;
        }
    t_creature(const t_creature & b)
    {
        x = b.x;
        y = b.y;
        z = b.z;
        type = b.type;
        flags1 = b.flags1;
        flags2 = b.flags2;
        first_name = b.first_name;
        nick_name = b.nick_name;
        //string last_name;
        trans_name = b.trans_name;
        generic_name = b.generic_name;
        generic_squad_name = b.generic_squad_name;
        trans_squad_name = b.trans_squad_name;
        profession = b.profession;
        custom_profession = b.custom_profession;
        current_job = b.current_job;
        happiness = b.happiness;
        id = b.id;
        agility = b.agility;
        strength = b.strength;
        toughness = b.toughness;
        money = b.money;
        squad_leader_id = b.squad_leader_id;
        sex = b.sex;
        skills = b.skills;
        traits = b.traits;
        labors = b.labors;
    }
    t_creature & operator=(const t_creature &b)
    {
        x = b.x;
        y = b.y;
        z = b.z;
        type = b.type;
        flags1 = b.flags1;
        flags2 = b.flags2;
        first_name = b.first_name;
        nick_name = b.nick_name;
        //string last_name;
        trans_name = b.trans_name;
        generic_name = b.generic_name;
        generic_squad_name = b.generic_squad_name;
        trans_squad_name = b.trans_squad_name;
        profession = b.profession;
        custom_profession = b.custom_profession;
        current_job = b.current_job;
        happiness = b.happiness;
        id = b.id;
        agility = b.agility;
        strength = b.strength;
        toughness = b.toughness;
        money = b.money;
        squad_leader_id = b.squad_leader_id;
        sex = b.sex;
        skills = b.skills;
        traits = b.traits;
        return *this;
    }*/
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
