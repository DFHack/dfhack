#ifndef CL_MOD_CREATURES
#define CL_MOD_CREATURES
/*
* Creatures
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
#include "dfhack/modules/Items.h"
namespace DFHack
{
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

    struct naked_creaturflags1
    {
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
    };

    union t_creaturflags1
    {
        uint32_t whole;
        naked_creaturflags1 bits;
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
    struct naked_creaturflags2
    {
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
    };

    union t_creaturflags2
    {
        uint32_t whole;
        naked_creaturflags2 bits;
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

    struct t_skill
    {
        uint32_t id;
        uint32_t rating;
        uint32_t experience;
    };
    struct t_job
    {
        bool active;
        uint32_t jobId;
        uint8_t jobType;
        uint32_t occupationPtr;
    };
    struct t_like
    {
        int16_t type;
        int16_t itemClass;
        int16_t itemIndex;
        t_matglossPair material;
        bool active;
    };


    // FIXME: define in Memory.xml instead?
    #define NUM_CREATURE_TRAITS 30
    #define NUM_CREATURE_LABORS 102
    #define NUM_CREATURE_MENTAL_ATTRIBUTES 13
    #define NUM_CREATURE_PHYSICAL_ATTRIBUTES 6

    struct t_soul
    {
        uint8_t numSkills;
        t_skill skills[256];
        /*
        uint8_t numLikes;
        t_like likes[32];
        */
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

    struct t_creature
    {
        uint32_t origin;
        uint16_t x;
        uint16_t y;
        uint16_t z;
        uint32_t race;
        int32_t civ;

        t_creaturflags1 flags1;
        t_creaturflags2 flags2;

        t_name name;

        int16_t mood;
        int16_t mood_skill;
        t_name artifact_name;

        uint8_t profession;
        char custom_profession[128];

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
        bool has_default_soul;
        t_soul defaultSoul;
        uint32_t nbcolors;
        uint32_t color[MAX_COLORS];

        uint32_t birth_year;
        uint32_t birth_time;
    };

    class DFContextShared;
    struct t_creature;
    class DFHACK_EXPORT Creatures : public Module
    {
    public:
        Creatures(DFHack::DFContextShared * d);
        ~Creatures();
        bool Start( uint32_t & numCreatures );
        bool Finish();

        /* Read Functions */
        // Read creatures in a box, starting with index. Returns -1 if no more creatures
        // found. Call repeatedly do get all creatures in a specified box (uses tile coords)
        int32_t ReadCreatureInBox(const int32_t index, t_creature & furball,
            const uint16_t x1, const uint16_t y1,const uint16_t z1,
            const uint16_t x2, const uint16_t y2,const uint16_t z2);
        bool ReadCreature(const int32_t index, t_creature & furball);
        bool ReadJob(const t_creature * furball, std::vector<t_material> & mat);
		bool ReadInventoryIdx(const uint32_t index, std::vector<uint32_t> & item);
		bool ReadInventoryPtr(const uint32_t index, std::vector<uint32_t> & item);

        /* Getters */
        uint32_t GetDwarfRaceIndex ( void );
        int32_t GetDwarfCivId ( void );

        /* Write Functions */
        bool WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS]);
        bool WriteHappiness(const uint32_t index, const uint32_t happinessValue);
        bool WriteFlags(const uint32_t index, const uint32_t flags1, const uint32_t flags2);
        bool WriteSkills(const uint32_t index, const t_soul &soul);
        bool WriteAttributes(const uint32_t index, const t_creature &creature);
        bool WriteSex(const uint32_t index, const uint8_t sex);
        bool WriteTraits(const uint32_t index, const t_soul &soul);
        bool WriteMood(const uint32_t index, const uint16_t mood);
        bool WriteMoodSkill(const uint32_t index, const uint16_t moodSkill);
		bool WriteJob(const t_creature * furball, std::vector<t_material> const& mat);
        bool WritePos(const uint32_t index, const t_creature &creature);
        bool WriteCiv(const uint32_t index, const int32_t civ);

    private:
        struct Private;
        Private *d;
    };
}
#endif
