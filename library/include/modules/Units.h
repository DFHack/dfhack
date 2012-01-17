/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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
#ifndef CL_MOD_CREATURES
#define CL_MOD_CREATURES
/*
* Creatures
*/
#include "Export.h"
#include "Module.h"
#include "modules/Items.h"
/**
 * \defgroup grp_units Unit module parts
 * @ingroup grp_modules
 */
namespace DFHack
{
    /**
     * easy access to first crature flags block
     * \ingroup grp_units
     */
    union t_unitflags1
    {
        uint32_t whole;/*!< Access all flags as a single 32bit number. */
        struct
        {
            unsigned int move_state : 1;       /*!< 0 : Can the dwarf move or are they waiting for their movement timer */
            unsigned int dead : 1;             /*!< 1 : Dead (might also be set for incoming/leaving critters that are alive) */
            unsigned int has_mood : 1;         /*!< 2 : Currently in mood */
            unsigned int had_mood : 1;         /*!< 3 : Had a mood already */

            unsigned int marauder : 1;         /*!< 4 : wide class of invader/inside creature attackers */
            unsigned int drowning : 1;         /*!< 5 : Is currently drowning */
            unsigned int merchant : 1;         /*!< 6 : An active merchant */
            unsigned int forest : 1;           /*!< 7 : used for units no longer linked to merchant/diplomacy, they just try to leave mostly */

            unsigned int left : 1;             /*!< 8 : left the map */
            unsigned int rider : 1;            /*!< 9 : Is riding an another creature */
            unsigned int incoming : 1;         /*!< 10 */
            unsigned int diplomat : 1;         /*!< 11 */

            unsigned int zombie : 1;           /*!< 12 */
            unsigned int skeleton : 1;         /*!< 13 */
            unsigned int can_swap : 1;         /*!< 14: Can swap tiles during movement (prevents multiple swaps) */
            unsigned int on_ground : 1;        /*!< 15: The creature is laying on the floor, can be conscious */

            unsigned int projectile : 1;       /*!< 16: Launched into the air? Funny. */
            unsigned int active_invader : 1;   /*!< 17: Active invader (for organized ones) */ 
            unsigned int hidden_in_ambush : 1; /*!< 18 */
            unsigned int invader_origin : 1;   /*!< 19: Invader origin (could be inactive and fleeing) */ 

            unsigned int coward : 1;           /*!< 20: Will flee if invasion turns around */
            unsigned int hidden_ambusher : 1;  /*!< 21: Active marauder/invader moving inward? */
            unsigned int invades : 1;          /*!< 22: Marauder resident/invader moving in all the way */
            unsigned int check_flows : 1;      /*!< 23: Check against flows next time you get a chance */

            unsigned int ridden : 1;           /*!< 24*/
            unsigned int caged : 1;            /*!< 25*/
            unsigned int tame : 1;             /*!< 26*/
            unsigned int chained : 1;          /*!< 27*/

            unsigned int royal_guard : 1;      /*!< 28*/
            unsigned int fortress_guard : 1;   /*!< 29*/
            unsigned int suppress_wield : 1;   /*!< 30: Suppress wield for beatings/etc */
            unsigned int important_historical_figure : 1; /*!< 31: Is an important historical figure */
        } bits;
    };

    union t_unitflags2
    {
        uint32_t whole; /*!< Access all flags as a single 32bit number. */
        struct
        {
            unsigned int swimming : 1;
            unsigned int sparring : 1;
            unsigned int no_notify : 1; /*!< Do not notify about level gains (for embark etc)*/
            unsigned int unused : 1;

            unsigned int calculated_nerves : 1;
            unsigned int calculated_bodyparts : 1;
            unsigned int important_historical_figure : 1; /*!< Is important historical figure (slight variation)*/
            unsigned int killed : 1; /*!< Has been killed by kill function (slightly different from dead, not necessarily violent death)*/

            unsigned int cleanup_1 : 1; /*!< Must be forgotten by forget function (just cleanup) */
            unsigned int cleanup_2 : 1; /*!< Must be deleted (cleanup) */
            unsigned int cleanup_3 : 1; /*!< Recently forgotten (cleanup) */
            unsigned int for_trade : 1; /*!< Offered for trade */

            unsigned int trade_resolved : 1;
            unsigned int has_breaks : 1;
            unsigned int gutted : 1;
            unsigned int circulatory_spray : 1;

            unsigned int locked_in_for_trading : 1; /*!< Locked in for trading (it's a projectile on the other set of flags, might be what the flying was) */
            unsigned int slaughter : 1; /*!< marked for slaughter */
            unsigned int underworld : 1; /*!< Underworld creature */
            unsigned int resident : 1; /*!< Current resident */

            unsigned int cleanup_4 : 1; /*!< Marked for special cleanup as unused load from unit block on disk */
            unsigned int calculated_insulation : 1; /*!< Insulation from clothing calculated */
            unsigned int visitor_uninvited : 1; /*!< Uninvited guest */
            unsigned int visitor : 1; /*!< visitor */

            unsigned int calculated_inventory : 1; /*!< Inventory order calculated */
            unsigned int vision_good : 1; /*!< Vision -- have good part */
            unsigned int vision_damaged : 1; /*!< Vision -- have damaged part */
            unsigned int vision_missing : 1; /*!< Vision -- have missing part */

            unsigned int breathing_good : 1; /*!< Breathing -- have good part */
            unsigned int breathing_problem : 1; /*!< Breathing -- having a problem */
            unsigned int roaming_wilderness_population_source : 1;
            unsigned int roaming_wilderness_population_source_not_a_map_feature : 1;
        } bits;
    };

    union t_unitflags3
    {
        uint32_t whole; /*!< Access all flags as a single 32bit number. */
        struct
        {
            unsigned int unk0 : 1; /*!< Is 1 for new and dead creatures,
                                     periodicaly set to 0 for non-dead creatures.
                                     */
            unsigned int unk1 : 1; /*!< Is 1 for new creatures, periodically set
                                     to 0 for non-dead creatures. */
            unsigned int unk2 : 1; /*!< Is set to 1 every tick for non-dead
                                     creatures. */
            unsigned int unk3 : 1; /*!< Is periodically set to 0 for non-dead
                                     creatures. */
            unsigned int announce_titan : 1; /*!< Announces creature like an
                                               FB or titan. */
            unsigned int unk5 : 1; 
            unsigned int unk6 : 1; 
            unsigned int unk7 : 1; 
            unsigned int unk8 : 1; /*!< Is set to 1 every tick for non-dead
                                     creatures. */
            unsigned int unk9 : 1; /*!< Is set to 0 every tick for non-dead
                                     creatures. */
            unsigned int scuttle : 1; /*!< Scuttle creature: causes creature
                                        to be killed, leaving a behind corpse
                                        and generating negative thoughts like
                                        a real kill. */
            unsigned int unk11 : 1;
            unsigned int ghostly : 1; /*!< Creature is a ghost. */

            unsigned int unk13_31 : 19;
        } bits;
    };

    // FIXME: WTF IS THIS SHIT?
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
    /**
     * \ingroup grp_units
     */
    struct t_skill
    {
        uint32_t id;
        uint32_t rating;
        uint32_t experience;
    };
    /**
     * \ingroup grp_units
     */
    struct t_job
    {
        bool active;
        uint32_t jobId;
        uint8_t jobType;
        uint32_t occupationPtr;
    };
    /**
     * \ingroup grp_units
     */
    struct t_like
    {
        int16_t type;
        int16_t itemClass;
        int16_t itemIndex;
        t_matglossPair material;
        bool active;
        uint32_t mystery;
    };

    // FIXME: THIS IS VERY, VERY BAD.
    #define NUM_CREATURE_LABORS 96
    #define NUM_CREATURE_TRAITS 30
    #define NUM_CREATURE_MENTAL_ATTRIBUTES 13
    #define NUM_CREATURE_PHYSICAL_ATTRIBUTES 6
    /**
     * Structure for holding a copy of a DF unit's soul
     * \ingroup grp_units
     */
    struct t_soul
    {
        uint8_t numSkills;
        t_skill skills[256];
        //uint8_t numLikes;
        //t_like likes[32];
        uint16_t traits[NUM_CREATURE_TRAITS];
        t_attrib analytical_ability;
        t_attrib focus;
        t_attrib willpower;
        t_attrib creativity;
        t_attrib intuition;
        t_attrib patience;
        t_attrib memory;
        t_attrib linguistic_ability;
        t_attrib spatial_sense;
        t_attrib musicality;
        t_attrib kinesthetic_sense;
        t_attrib empathy;
        t_attrib social_awareness;
    };
    #define MAX_COLORS  15
    struct df_unit;
    /**
     * Structure for holding a limited copy of a DF unit
     * \ingroup grp_units
     */
    struct t_unit
    {
        df_unit * origin;
        uint16_t x;
        uint16_t y;
        uint16_t z;
        uint32_t race;
        int32_t civ;

        t_unitflags1 flags1;
        t_unitflags2 flags2;
        t_unitflags3 flags3;

        t_name name;

        int16_t mood;
        int16_t mood_skill;
        t_name artifact_name;

        uint8_t profession;
        std::string custom_profession;

        // enabled labors
        uint8_t labors[NUM_CREATURE_LABORS];
        t_job current_job;

        uint32_t happiness;
        uint32_t id;
        t_attrib strength;
        t_attrib agility;
        t_attrib toughness;
        t_attrib endurance;
        t_attrib recuperation;
        t_attrib disease_resistance;
        int32_t squad_leader_id;
        uint8_t sex;
        uint16_t caste;
        uint32_t pregnancy_timer; //Countdown timer to giving birth
        //bool has_default_soul;
        //t_soul defaultSoul;
        uint32_t nbcolors;
        uint32_t color[MAX_COLORS];

        int32_t birth_year;
        uint32_t birth_time;
    };

    /**
     * Unit attribute descriptor
     * \ingroup grp_units
     */
    struct df_attrib
    {
        uint32_t unk_0;
        uint32_t unk_4;
        uint32_t unk_8;
        uint32_t unk_c;
        uint32_t unk_10;
        uint32_t unk_14;
        uint32_t unk_18;
    };
    /**
     * Unit skill descriptor
     * \ingroup grp_units
     */
    struct df_skill
    {
        uint16_t id;    // 0
        int32_t rating; // 4
        uint32_t experience;    // 8
        uint32_t unk_c;
        uint32_t rusty; // 10
        uint32_t unk_14;
        uint32_t unk_18;
        uint32_t unk_1c;
    };
    /**
     * Unit like descriptor
     * \ingroup grp_units
     */
    struct df_like
    {
        int16_t type;
        int16_t itemClass;
        int16_t itemIndex;
        int16_t material_type;
        int32_t material_index;
        bool active;
        uint32_t mystery;
    };
    /**
     * A creature's soul, as it appears in DF memory
     * \ingroup grp_units
     */
    struct df_soul
    {
        uint32_t creature_id;
        df_name name;   // 4
        uint32_t unk_70;
        uint16_t unk_74;
        uint16_t unk_76;
        int32_t unk_78;
        int32_t unk_7c;
        int32_t unk_80;
        int32_t unk_84;
        df_attrib mental[NUM_CREATURE_MENTAL_ATTRIBUTES];       // 88..1f3
        std::vector<df_skill*> skills;  // 1f4;
        std::vector<df_like*> likes;     // pointers to 14 0x14-byte structures ... likes?
        uint16_t traits[NUM_CREATURE_TRAITS];   // 214
        std::vector<int16_t*> unk_250;  // 1 pointer to 2 shorts
        uint32_t unk_260;
        uint32_t unk_264;
        uint32_t unk_268;
        uint32_t unk_26c;
    };
    /**
     * A creature job - what it's supposed to be doing.
     * \ingroup grp_units
     */
    struct df_job
    {
        
    };
    /**
     * A creature though - dwarves staring at waterfalls!
     * \ingroup grp_units
     */
    struct df_thought
    {
        // needs enum
        int16_t type;
        // possible int16_t here
        int32_t age;
        int32_t subtype;
        int32_t severity;
        //possibly more.
    };
    /**
     * A creature, as it appears in DF memory
     * \ingroup grp_units
     */
    struct df_unit
    {
        df_name name;   // 0
        std::string custom_profession;  // 6c (MSVC)
        uint8_t profession;     // 88
        uint32_t race;  // 8c
        uint16_t x;     // 90
        uint16_t y;     // 92
        uint16_t z;     // 94

        uint16_t unk_x96; // 96
        uint16_t unk_y98; // 98
        uint16_t unk_z9a; // 9a

        uint32_t unk_9c;
        uint16_t unk_a0;
        int16_t unk_a2;
        uint32_t unk_a4;

        uint16_t dest_x;        // a8
        uint16_t dest_y;        // aa
        uint16_t dest_z;        // ac
        uint16_t unk_ae;        // -1

        std::vector<uint32_t> unk_b0;   // b0->df (3*4 in MSVC) -> 68->8b (3*3 in glibc)
        std::vector<uint32_t> unk_c0;
        std::vector<uint32_t> unk_d0;

        t_unitflags1 flags1;         // e0
        t_unitflags2 flags2;         // e4
        t_unitflags3 flags3;         // e8

        void ** unk_ec;
        int32_t unk_f0;
        int16_t unk_f4;
        int16_t unk_f6;
        uint16_t caste;         // f8
        uint8_t sex;            // fa
        uint32_t id;            // fc
        uint16_t unk_100;
        uint16_t unk_102;
        int32_t unk_104;
        uint32_t civ;           // 108
        uint32_t unk_10c;
        int32_t unk_110;
        
        std::vector<uint32_t> unk_114;
        std::vector<uint32_t> unk_124;
        std::vector<uint32_t> unk_134;

        uint32_t unk_144;

        std::vector<void*> unk_148;
        std::vector<void*> unk_158;

        int32_t unk_168;
        int32_t unk_16c;
        uint32_t unk_170;
        uint32_t unk_174;
        uint16_t unk_178;

        std::vector<uint32_t> unk_17c;
        std::vector<uint32_t> unk_18c;
        std::vector<uint32_t> unk_19c;
        std::vector<uint32_t> unk_1ac;
        uint32_t pickup_equipment_bit;  // 1bc
        std::vector<uint32_t> unk_1c0;
        std::vector<uint32_t> unk_1d0;
        std::vector<uint32_t> unk_1e0;

        int32_t unk_1f0;
        int16_t unk_1f4;
        int32_t unk_1f8;
        int32_t unk_1fc;
        int32_t unk_200;
        int16_t unk_204;
        uint32_t unk_208;
        uint32_t unk_20c;

        int16_t mood;           // 210
        uint32_t pregnancy_timer;       // 214
        void* pregnancy_ptr;    // 218
        int32_t unk_21c;
        uint32_t unk_220;
        uint32_t birth_year;    // 224
        uint32_t birth_time;    // 228
        uint32_t unk_22c;
        uint32_t unk_230;
        df_unit * unk_234; // suspiciously close to the pregnancy/birth stuff. Mother?
        uint32_t unk_238;
        int32_t unk_23c;
        int32_t unk_240;
        int32_t unk_244;
        int32_t unk_248;
        int32_t unk_24c;
        int32_t unk_250;
        int32_t unk_254;
        int32_t unk_258;
        int32_t unk_25c;
        int32_t unk_260;
        int16_t unk_264;
        int32_t unk_268;
        int32_t unk_26c;
        int16_t unk_270;
        int32_t unk_274;
        int32_t unk_278;
        int32_t unk_27c;
        int16_t unk_280;
        int32_t unk_284;

        std::vector<df::item *> inventory;   // 288 - vector of item pointers
        std::vector<int32_t> owned_items;  // 298 - vector of item IDs
        std::vector<uint32_t> unk_2a8;
        std::vector<uint32_t> unk_2b8;
        std::vector<uint32_t> unk_2c8;

        uint32_t unk_2d8;
        uint32_t unk_2dc;
        uint32_t unk_2e0;
        uint32_t unk_2e4;
        uint32_t unk_2e8;
        uint32_t unk_2ec;
        uint32_t unk_2f0;
        df_job * current_job;   // 2f4
        uint32_t unk_2f8; // possibly current skill?
        uint32_t unk_2fc;
        uint32_t unk_300;
        uint32_t unk_304;

        std::vector<uint32_t> unk_308;
        std::vector<uint32_t> unk_318;
        std::vector<uint32_t> unk_328;
        std::vector<uint32_t> unk_338;
        std::vector<uint32_t> unk_348;
        std::vector<uint32_t> unk_358;
        std::vector<uint32_t> unk_368;
        std::vector<uint32_t> unk_378;
        std::vector<uint32_t> unk_388;

        uint32_t unk_398;
        int32_t unk_39c;
        int32_t unk_3a0;
        int32_t unk_3a4;
        int32_t unk_3a8;
        int32_t unk_3ac;
        int32_t unk_3b0;
        int32_t unk_3b4;
        int32_t unk_3b8;
        int32_t unk_3bc;
        int32_t unk_3c0;
        uint32_t unk_3c4;
        uint32_t unk_3c8;

        df_attrib physical[NUM_CREATURE_PHYSICAL_ATTRIBUTES];   // 3cc..473
        uint32_t unk_474;
        uint32_t unk_478;
        uint32_t unk_47c;
        uint32_t unk_480;
        uint32_t unk_484;
        uint32_t unk_488;

        uint32_t unk_48c;       // blood_max?
        uint32_t blood_count;   // 490
        uint32_t unk_494;
        // dirt, grime, FB blood, mud and plain old filth stuck to the poor thing's parts and pieces
        std::vector<void*> contaminants;
        std::vector<uint16_t> unk_4a8;
        std::vector<uint16_t> unk_4b8;
        uint32_t unk_4c8;
        std::vector<int16_t> unk_4cc;
        std::vector<int32_t> unk_4dc;
        std::vector<int32_t> unk_4ec;
        std::vector<int32_t> unk_4fc;
        std::vector<uint16_t> unk_50c;
        void* unk_51c;
        uint16_t unk_520;
        uint16_t unk_522;
        uint16_t* unk_524;
        uint16_t unk_528;
        uint16_t unk_52a;
        std::vector<uint32_t> appearance;        // 52c
        int16_t unk_53c;
        int16_t unk_53e;
        int16_t job_counter; // tick until next job update?
        int16_t unk_542;
        int16_t unk_544;
        int16_t unk_546;
        int16_t unk_548;
        int16_t unk_54a;
        int16_t unk_54c;
        int16_t unk_54e;
        int16_t unk_550;
        int16_t unk_552;
        int16_t unk_x554;       // coords ? (-30.000x3)
        int16_t unk_y556;
        int16_t unk_z558;
        int16_t unk_x55a;       // coords again
        int16_t unk_y55c;
        int16_t unk_z55e;
        int16_t unk_560;
        int16_t unk_562;

        uint32_t unk_564;
        uint32_t unk_568;
        uint32_t unk_56c;
        uint32_t unk_570;
        uint32_t unk_574;
        uint32_t unk_578;
        uint32_t unk_57c;
        uint32_t unk_580;
        uint32_t unk_584;
        uint32_t unk_588;
        uint32_t unk_58c;
        uint32_t unk_590;
        uint32_t unk_594;
        uint32_t unk_598;
        uint32_t unk_59c;

        std::vector<void*> unk_5a0;
        void* unk_5b0;          // pointer to X (12?) vector<int16_t>
        uint32_t unk_5b4;       // 0x3e8 (1000)
        uint32_t unk_5b8;       // 0x3e8 (1000)
        std::vector<uint32_t> unk_5bc;
        std::vector<uint32_t> unk_5cc;
        int16_t unk_5dc;
        int16_t unk_5de;
        df_name artifact_name; // 5e0
        std::vector<df_soul*> souls;      // 64c
        df_soul* current_soul;  // 65c
        std::vector<uint32_t> unk_660;
        uint8_t labors[NUM_CREATURE_LABORS];    // 670..6cf

        std::vector<uint32_t> unk_6d0;
        std::vector<uint32_t> unk_6e0;
        std::vector<df_thought *> thoughts;
        std::vector<uint32_t> unk_700;
        uint32_t happiness;     // 710
        uint16_t unk_714;
        uint16_t unk_716;
        std::vector<void*> unk_718;
        std::vector<void*> unk_728;
        std::vector<void*> unk_738;
        std::vector<void*> unk_748;
        uint16_t unk_758;
        uint16_t unk_x75a;      // coords (-30000*3)
        uint16_t unk_y75c;
        uint16_t unk_z75e;
        std::vector<uint16_t> unk_760;
        std::vector<uint16_t> unk_770;
        std::vector<uint16_t> unk_780;
        uint32_t hist_figure_id;        // 790
        uint16_t able_stand;            // 794
        uint16_t able_stand_impair;     // 796
        uint16_t able_grasp;            // 798
        uint16_t able_grasp_impair;     // 79a
        uint32_t unk_79c; // able_fly/impair (maybe)
        uint32_t unk_7a0; // able_fly/impair (maybe)
        std::vector<void*> unk_7a4;
        uint32_t unk_7b4;
        uint32_t unk_7b8;
        uint32_t unk_7bc;
        int32_t unk_7c0;

        std::vector<uint32_t> unk_7c4;
        std::vector<uint32_t> unk_7d4;
        std::vector<uint32_t> unk_7e4;
        std::vector<uint32_t> unk_7f4;
        std::vector<uint32_t> unk_804;
        std::vector<uint32_t> unk_814;

        uint32_t unk_824;
        void* unk_828;
        void* unk_82c;
        uint32_t unk_830;
        void* unk_834;
        void* unk_838;
        void* unk_83c;

        std::vector<void*> unk_840;
        std::vector<uint32_t> unk_850;
        std::vector<uint32_t> unk_860;
        uint32_t unk_870;
        uint32_t unk_874;
        std::vector<uint8_t> unk_878;
        std::vector<uint8_t> unk_888;
        std::vector<uint32_t> unk_898;
        std::vector<uint8_t> unk_8a8;
        std::vector<uint16_t> unk_8b8;
        std::vector<uint16_t> unk_8c8;
        std::vector<uint32_t> unk_8d8;
        std::vector<uint32_t> unk_8e8;
        std::vector<uint32_t> unk_8f8;
        std::vector<uint32_t> unk_908;

        int32_t unk_918;
        uint16_t unk_91c;
        uint16_t unk_91e;
        uint16_t unk_920;
        uint16_t unk_922;
        uint32_t unk_924;
        uint32_t unk_928;
        std::vector<uint16_t> unk_92c;
        uint32_t unk_93c;
    };
    /**
     * The Creatures module - allows reading all non-vermin creatures and their properties
     * \ingroup grp_modules
     * \ingroup grp_units
     */
    class DFHACK_EXPORT Units : public Module
    {
    public:
        std::vector <df_unit *> * creatures;
    public:
        Units();
        ~Units();
        bool Start( uint32_t & numCreatures );
        bool Finish();

        /* Read Functions */
        // Read creatures in a box, starting with index. Returns -1 if no more creatures
        // found. Call repeatedly do get all creatures in a specified box (uses tile coords)
        int32_t GetCreatureInBox(const int32_t index, df_unit ** furball,
            const uint16_t x1, const uint16_t y1,const uint16_t z1,
            const uint16_t x2, const uint16_t y2,const uint16_t z2);
        df_unit * GetCreature(const int32_t index);
        void CopyCreature(df_unit * source, t_unit & target);

        bool ReadJob(const df_unit * unit, std::vector<t_material> & mat);

        bool ReadInventoryByIdx(const uint32_t index, std::vector<df::item *> & item);
        bool ReadInventoryByPtr(const df_unit * unit, std::vector<df::item *> & item);

        bool ReadOwnedItemsByIdx(const uint32_t index, std::vector<int32_t> & item);
        bool ReadOwnedItemsByPtr(const df_unit * unit, std::vector<int32_t> & item);

        int32_t FindIndexById(int32_t id);

        /* Getters */
        uint32_t GetDwarfRaceIndex ( void );
        int32_t GetDwarfCivId ( void );

        /* Write Functions */
        //bool WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS]);
        //bool WriteHappiness(const uint32_t index, const uint32_t happinessValue);
        //bool WriteFlags(const uint32_t index, const uint32_t flags1, const uint32_t flags2);
        //bool WriteFlags(const uint32_t index, const uint32_t flags1, const uint32_t flags2, uint32_t flags3);
        //bool WriteSkills(const uint32_t index, const t_soul &soul);
        //bool WriteAttributes(const uint32_t index, const t_creature &creature);
        //bool WriteSex(const uint32_t index, const uint8_t sex);
        //bool WriteTraits(const uint32_t index, const t_soul &soul);
        //bool WriteMood(const uint32_t index, const uint16_t mood);
        //bool WriteMoodSkill(const uint32_t index, const uint16_t moodSkill);
        //bool WriteJob(const t_creature * furball, std::vector<t_material> const& mat);
        //bool WritePos(const uint32_t index, const t_creature &creature);
        //bool WriteCiv(const uint32_t index, const int32_t civ);
        //bool WritePregnancy(const uint32_t index, const uint32_t pregTimer);

        void CopyNameTo(df_unit *creature, df_name * target);

    protected:
        friend class Items;
        bool RemoveOwnedItemByIdx(const uint32_t index, int32_t id);
        bool RemoveOwnedItemByPtr(df_unit * unit, int32_t id);

    private:
        struct Private;
        Private *d;
    };
}
#endif
