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

/*
 * Units
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
    struct activity_entry;
    struct activity_event;
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

static const int MAX_COLORS = 15;


/**
 * The Units module - allows reading all non-vermin units and their properties
 */

/* Read Functions */
// Read units in a box, starting with index. Returns -1 if no more units
// found. Call repeatedly do get all units in a specified box (uses tile coords)
DFHACK_EXPORT int32_t getNumUnits();
DFHACK_EXPORT df::unit *getUnit(const int32_t index);
DFHACK_EXPORT bool getUnitsInBox(std::vector<df::unit*> &units,
    int16_t x1, int16_t y1, int16_t z1,
    int16_t x2, int16_t y2, int16_t z2);

DFHACK_EXPORT int32_t findIndexById(int32_t id);

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
DFHACK_EXPORT bool isOwnGroup(df::unit* unit);
DFHACK_EXPORT bool isOwnRace(df::unit* unit);
DFHACK_EXPORT bool isVisible(df::unit* unit);

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
DFHACK_EXPORT bool isTame(df::unit* unit);
DFHACK_EXPORT bool isTrained(df::unit* unit);
DFHACK_EXPORT bool isGay(df::unit* unit);
DFHACK_EXPORT bool isNaked(df::unit* unit);
DFHACK_EXPORT bool isUndead(df::unit* unit);
DFHACK_EXPORT bool isGelded(df::unit* unit);
DFHACK_EXPORT bool isDomesticated(df::unit* unit);

DFHACK_EXPORT double getAge(df::unit *unit, bool true_age = false);
DFHACK_EXPORT int getKillCount(df::unit *unit);

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

DFHACK_EXPORT df::activity_entry *getMainSocialActivity(df::unit *unit);
DFHACK_EXPORT df::activity_event *getMainSocialEvent(df::unit *unit);

}
}
