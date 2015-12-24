/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include "modules/Items.h"
#include "DataDefs.h"

#include "df/caste_raw_flags.h"
#include "df/job_skill.h"
#include "df/mental_attribute_type.h"
#include "df/misc_trait_type.h"
#include "df/physical_attribute_type.h"
#include "df/unit.h"

namespace df
{
    struct nemesis_record;
    struct burrow;
    struct identity;
    struct historical_entity;
    struct entity_position_assignment;
    struct entity_position;
    struct unit_misc_trait;
}

/**
 * \defgroup grp_units Unit module parts
 * @ingroup grp_modules
 */
namespace DFHack
{
namespace Units
{
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
    df::unit * origin;
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint32_t race;
    int32_t civ;

    df::unit_flags1 flags1;
    df::unit_flags2 flags2;
    df::unit_flags3 flags3;

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
 * The Creatures module - allows reading all non-vermin creatures and their properties
 * \ingroup grp_modules
 * \ingroup grp_units
 */

DFHACK_EXPORT bool isValid();

/* Read Functions */
// Read creatures in a box, starting with index. Returns -1 if no more creatures
// found. Call repeatedly do get all creatures in a specified box (uses tile coords)
DFHACK_EXPORT int32_t getNumCreatures();
DFHACK_EXPORT int32_t GetCreatureInBox(const int32_t index, df::unit ** furball,
    const uint16_t x1, const uint16_t y1,const uint16_t z1,
    const uint16_t x2, const uint16_t y2,const uint16_t z2);
DFHACK_EXPORT df::unit * GetCreature(const int32_t index);
DFHACK_EXPORT void CopyCreature(df::unit * source, t_unit & target);

DFHACK_EXPORT bool ReadJob(const df::unit * unit, std::vector<t_material> & mat);

DFHACK_EXPORT bool ReadInventoryByIdx(const uint32_t index, std::vector<df::item *> & item);
DFHACK_EXPORT bool ReadInventoryByPtr(const df::unit * unit, std::vector<df::item *> & item);

DFHACK_EXPORT int32_t FindIndexById(int32_t id);

/* Getters */
DFHACK_EXPORT uint32_t GetDwarfRaceIndex ( void );
DFHACK_EXPORT int32_t GetDwarfCivId ( void );

DFHACK_EXPORT void CopyNameTo(df::unit *creature, df::language_name * target);

/// Returns the true position of the unit (non-trivial in case of caged).
DFHACK_EXPORT df::coord getPosition(df::unit *unit);

DFHACK_EXPORT df::general_ref *getGeneralRef(df::unit *unit, df::general_ref_type type);
DFHACK_EXPORT df::specific_ref *getSpecificRef(df::unit *unit, df::specific_ref_type type);

DFHACK_EXPORT df::item *getContainer(df::unit *unit);

DFHACK_EXPORT void setNickname(df::unit *unit, std::string nick);
DFHACK_EXPORT df::language_name *getVisibleName(df::unit *unit);

DFHACK_EXPORT df::identity *getIdentity(df::unit *unit);
DFHACK_EXPORT df::nemesis_record *getNemesis(df::unit *unit);

DFHACK_EXPORT bool isHidingCurse(df::unit *unit);
DFHACK_EXPORT int getPhysicalAttrValue(df::unit *unit, df::physical_attribute_type attr);
DFHACK_EXPORT int getMentalAttrValue(df::unit *unit, df::mental_attribute_type attr);
DFHACK_EXPORT bool casteFlagSet(int race, int caste, df::caste_raw_flags flag);

DFHACK_EXPORT bool isCrazed(df::unit *unit);
DFHACK_EXPORT bool isOpposedToLife(df::unit *unit);
DFHACK_EXPORT bool hasExtravision(df::unit *unit);
DFHACK_EXPORT bool isBloodsucker(df::unit *unit);
DFHACK_EXPORT bool isMischievous(df::unit *unit);

DFHACK_EXPORT df::unit_misc_trait *getMiscTrait(df::unit *unit, df::misc_trait_type type, bool create = false);

DFHACK_EXPORT bool isDead(df::unit *unit);
DFHACK_EXPORT bool isAlive(df::unit *unit);
DFHACK_EXPORT bool isSane(df::unit *unit);
DFHACK_EXPORT bool isCitizen(df::unit *unit);
DFHACK_EXPORT bool isDwarf(df::unit *unit);
DFHACK_EXPORT bool isWar(df::unit* unit);
DFHACK_EXPORT bool isHunter(df::unit* unit);
DFHACK_EXPORT bool isAvailableForAdoption(df::unit* unit);
DFHACK_EXPORT bool isOwnCiv(df::unit* unit);
DFHACK_EXPORT bool isOwnRace(df::unit* unit);

DFHACK_EXPORT std::string getRaceNameById(int32_t race_id);
DFHACK_EXPORT std::string getRaceName(df::unit* unit);
DFHACK_EXPORT std::string getRaceNamePluralById(int32_t race_id);
DFHACK_EXPORT std::string getRaceNamePlural(df::unit* unit);
DFHACK_EXPORT std::string getRaceBabyNameById(int32_t race_id);
DFHACK_EXPORT std::string getRaceBabyName(df::unit* unit);
DFHACK_EXPORT std::string getRaceChildNameById(int32_t race_id);
DFHACK_EXPORT std::string getRaceChildName(df::unit* unit);

DFHACK_EXPORT bool isBaby(df::unit* unit);
DFHACK_EXPORT bool isChild(df::unit* unit);
DFHACK_EXPORT bool isAdult(df::unit* unit);
DFHACK_EXPORT bool isEggLayer(df::unit* unit);
DFHACK_EXPORT bool isGrazer(df::unit* unit);
DFHACK_EXPORT bool isMilkable(df::unit* unit);
DFHACK_EXPORT bool isTrainableWar(df::unit* unit);
DFHACK_EXPORT bool isTrainableHunting(df::unit* unit);
DFHACK_EXPORT bool isTamable(df::unit* unit);
DFHACK_EXPORT bool isMale(df::unit* unit);
DFHACK_EXPORT bool isFemale(df::unit* unit);
DFHACK_EXPORT bool isMerchant(df::unit* unit);
DFHACK_EXPORT bool isForest(df::unit* unit);
DFHACK_EXPORT bool isMarkedForSlaughter(df::unit* unit);

DFHACK_EXPORT double getAge(df::unit *unit, bool true_age = false);

DFHACK_EXPORT int getNominalSkill(df::unit *unit, df::job_skill skill_id, bool use_rust = false);
DFHACK_EXPORT int getEffectiveSkill(df::unit *unit, df::job_skill skill_id);
DFHACK_EXPORT int getExperience(df::unit *unit, df::job_skill skill_id, bool total = false);

DFHACK_EXPORT bool isValidLabor(df::unit *unit, df::unit_labor labor);

DFHACK_EXPORT int computeMovementSpeed(df::unit *unit);
DFHACK_EXPORT float computeSlowdownFactor(df::unit *unit);

struct NoblePosition {
    df::historical_entity *entity;
    df::entity_position_assignment *assignment;
    df::entity_position *position;
};

DFHACK_EXPORT bool getNoblePositions(std::vector<NoblePosition> *pvec, df::unit *unit);

DFHACK_EXPORT std::string getProfessionName(df::unit *unit, bool ignore_noble = false, bool plural = false);
DFHACK_EXPORT std::string getCasteProfessionName(int race, int caste, df::profession pid, bool plural = false);

DFHACK_EXPORT int8_t getProfessionColor(df::unit *unit, bool ignore_noble = false);
DFHACK_EXPORT int8_t getCasteProfessionColor(int race, int caste, df::profession pid);

DFHACK_EXPORT std::string getSquadName(df::unit *unit);
}
}
#endif
