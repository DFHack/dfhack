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
#include "DataDefs.h"

#include "modules/Items.h"
#include "modules/Maps.h"

#include "df/caste_raw_flags.h"
#include "df/goal_type.h"
#include "df/job_skill.h"
#include "df/mental_attribute_type.h"
#include "df/misc_trait_type.h"
#include "df/physical_attribute_type.h"
#include "df/unit_action.h"
#include "df/unit_action_type_group.h"

#include <ranges>

namespace df {
    struct activity_entry;
    struct activity_event;
    struct burrow;
    struct entity_position_assignment;
    struct entity_position;
    struct historical_entity;
    struct identity;
    struct language_name;
    struct nemesis_record;
    struct unit;
    struct unit_misc_trait;
}

/**
 * \defgroup grp_units Unit module parts
 * @ingroup grp_modules
 */
namespace DFHack {
    namespace Units {
        /**
         * The Units module - allows reading all non-vermin units and their properties
         */

        // Unit is non-dead and on the map.
        DFHACK_EXPORT bool isActive(df::unit *unit);
        // Unit is on visible map tile. Doesn't account for ambushing.
        DFHACK_EXPORT bool isVisible(df::unit *unit);
        // Unit is a non-dead (optionally sane) citizen of fort.
        DFHACK_EXPORT bool isCitizen(df::unit *unit, bool include_insane = false);
        // Long-term resident, not the hostile type.
        DFHACK_EXPORT bool isResident(df::unit *unit, bool include_insane = false);
        // Similar to isCitizen, but includes tame animals. Will reveal ambushers for the fort, etc.
        DFHACK_EXPORT bool isFortControlled(df::unit *unit);

        /// Check if creature belongs to the player's civilization.
        /// (Don't try to pasture/slaughter random untame animals.)
        DFHACK_EXPORT bool isOwnCiv(df::unit *unit);
        // Check if creature belongs to the player's group.
        DFHACK_EXPORT bool isOwnGroup(df::unit *unit);
        /// Check if creature belongs to the player's race.
        /// (Use with isOwnCiv to filter out own dwarves.)
        DFHACK_EXPORT bool isOwnRace(df::unit *unit);

        // Unit not dead nor undead. Naturally inorganic is okay.
        DFHACK_EXPORT bool isAlive(df::unit *unit);
        // Unit is dead or ghost.
        DFHACK_EXPORT bool isDead(df::unit *unit);
        // Dead but without ghost check.
        DFHACK_EXPORT bool isKilled(df::unit *unit);
        // Not dead, insane, zombie, nor crazed (unless active werebeast).
        DFHACK_EXPORT bool isSane(df::unit *unit);
        // Will attack all creatures except crazed members of its own species.
        DFHACK_EXPORT bool isCrazed(df::unit *unit);
        DFHACK_EXPORT bool isGhost(df::unit *unit);
        // Unit is hidden to the player. Accounts for ambushing, any game mode.
        DFHACK_EXPORT bool isHidden(df::unit *unit);
        DFHACK_EXPORT bool isHidingCurse(df::unit *unit);

        DFHACK_EXPORT bool isMale(df::unit *unit);
        DFHACK_EXPORT bool isFemale(df::unit *unit);
        DFHACK_EXPORT bool isBaby(df::unit *unit);
        DFHACK_EXPORT bool isChild(df::unit *unit);
        DFHACK_EXPORT bool isAdult(df::unit *unit);
        // Not willing to breed. Also includes any creature caste without a gender.
        DFHACK_EXPORT bool isGay(df::unit *unit);
        /// Not wearing any items. Note: Not naked if wearing a ring!
        /// Optionally check for empty inventory.
        DFHACK_EXPORT bool isNaked(df::unit *unit, bool no_items = false);
        // E.g., merchants, diplomats, and travelers.
        DFHACK_EXPORT bool isVisiting(df::unit *unit);

        DFHACK_EXPORT bool isTrainableHunting(df::unit *unit);
        DFHACK_EXPORT bool isTrainableWar(df::unit *unit);
        // Trained hunting or war, or is non-wild and non-domesticated.
        DFHACK_EXPORT bool isTrained(df::unit *unit);
        // Check if animal trained for hunting.
        DFHACK_EXPORT bool isHunter(df::unit *unit);
        // Check if animal trained for war.
        DFHACK_EXPORT bool isWar(df::unit *unit);
        DFHACK_EXPORT bool isTame(df::unit *unit);
        DFHACK_EXPORT bool isTamable(df::unit *unit);
        /// Check if creature is domesticated. Seems to be the only way to really
        /// tell if it's completely safe to autonestbox it (training can revert).
        DFHACK_EXPORT bool isDomesticated(df::unit *unit);
        DFHACK_EXPORT bool isMarkedForTraining(df::unit *unit);
        DFHACK_EXPORT bool isMarkedForTaming(df::unit *unit);
        DFHACK_EXPORT bool isMarkedForWarTraining(df::unit *unit);
        DFHACK_EXPORT bool isMarkedForHuntTraining(df::unit *unit);
        DFHACK_EXPORT bool isMarkedForSlaughter(df::unit *unit);
        DFHACK_EXPORT bool isMarkedForGelding(df::unit *unit);
        DFHACK_EXPORT bool isGeldable(df::unit *unit);
        DFHACK_EXPORT bool isGelded(df::unit *unit);
        DFHACK_EXPORT bool isEggLayer(df::unit *unit);
        DFHACK_EXPORT bool isEggLayerRace(df::unit *unit);
        DFHACK_EXPORT bool isGrazer(df::unit *unit);
        DFHACK_EXPORT bool isMilkable(df::unit *unit);
        DFHACK_EXPORT bool isForest(df::unit *unit);
        DFHACK_EXPORT bool isMischievous(df::unit *unit);
        // Check if unit is marked as available for adoption.
        DFHACK_EXPORT bool isAvailableForAdoption(df::unit *unit);
        DFHACK_EXPORT bool isPet(df::unit *unit);

        DFHACK_EXPORT bool hasExtravision(df::unit *unit);
        DFHACK_EXPORT bool isOpposedToLife(df::unit *unit);
        DFHACK_EXPORT bool isBloodsucker(df::unit *unit);

        // Same race as fort (even if active werebeast).
        DFHACK_EXPORT bool isDwarf(df::unit *unit);
        DFHACK_EXPORT bool isAnimal(df::unit *unit);
        DFHACK_EXPORT bool isMerchant(df::unit *unit);
        DFHACK_EXPORT bool isDiplomat(df::unit *unit);
        DFHACK_EXPORT bool isVisitor(df::unit *unit);
        DFHACK_EXPORT bool isInvader(df::unit *unit);
        // Ignores units hiding curse by default to avoid spoiling vampires.
        DFHACK_EXPORT bool isUndead(df::unit *unit, bool hiding_curse = false);
        DFHACK_EXPORT bool isNightCreature(df::unit *unit);
        DFHACK_EXPORT bool isSemiMegabeast(df::unit *unit);
        DFHACK_EXPORT bool isMegabeast(df::unit *unit);
        DFHACK_EXPORT bool isTitan(df::unit *unit);
        DFHACK_EXPORT bool isForgottenBeast(df::unit *unit);
        DFHACK_EXPORT bool isDemon(df::unit *unit);
        /// Probably hostile. Includes undead (optionally those hiding curse), night creatures,
        /// semi-megabeasts, invaders, agitated wildlife, crazed units, and Great Dangers.
        DFHACK_EXPORT bool isDanger(df::unit *unit, bool hiding_curse = false);
        // Megabeasts, titans, forgotten beasts, and demons.
        DFHACK_EXPORT bool isGreatDanger(df::unit *unit);

        // Check if unit is inside the cuboid area.
        DFHACK_EXPORT bool isUnitInBox(df::unit *u, const cuboid &box);
        DFHACK_EXPORT inline bool isUnitInBox(df::unit *u, int16_t x1, int16_t y1, int16_t z1,
            int16_t x2, int16_t y2, int16_t z2) { return isUnitInBox(u, cuboid(x1, y1, z1, x2, y2, z2)); }

        // Fill vector with units in box matching filter.
        DFHACK_EXPORT bool getUnitsInBox(std::vector<df::unit *> &units, const cuboid &box,
            std::function<bool(df::unit *)> filter = [](df::unit *u) { return true; });
        DFHACK_EXPORT inline bool getUnitsInBox(std::vector<df::unit *> &units, int16_t x1, int16_t y1, int16_t z1,
            int16_t x2, int16_t y2, int16_t z2, std::function<bool(df::unit *)> filter = [](df::unit *u) { return true; })
            { return getUnitsInBox(units, cuboid(x1, y1, z1, x2, y2, z2), filter); }

        // Noble string must be in form "CAPTAIN_OF_THE_GUARD", etc.
        DFHACK_EXPORT bool getUnitsByNobleRole(std::vector<df::unit *> &units, std::string noble);
        DFHACK_EXPORT df::unit *getUnitByNobleRole(std::string noble);

        inline auto citizensRange(std::vector<df::unit *> &vec, bool exclude_residents = false, bool include_insane = false) {
            return vec | std::views::filter([=](df::unit *unit) {
                if (isDead(unit) || !isActive(unit))
                    return false;
                return isCitizen(unit, include_insane) ||
                    (!exclude_residents && isResident(unit, include_insane));
            });
        }

        DFHACK_EXPORT void forCitizens(std::function<void(df::unit *)> fn, bool exclude_residents = false, bool include_insane = false);
        DFHACK_EXPORT bool getCitizens(std::vector<df::unit *> &citizens, bool exclude_residents = false, bool include_insane = false);

        // Returns the true position of the unit (non-trivial in case of caged).
        DFHACK_EXPORT df::coord getPosition(df::unit *unit);

        // Moves unit and any riders to the target coordinates. Sets tile occupancy flags.
        DFHACK_EXPORT bool teleport(df::unit *unit, df::coord target_pos);

        DFHACK_EXPORT df::general_ref *getGeneralRef(df::unit *unit, df::general_ref_type type);
        DFHACK_EXPORT df::specific_ref *getSpecificRef(df::unit *unit, df::specific_ref_type type);

        // Cage that the unit is held in or NULL.
        DFHACK_EXPORT df::item *getContainer(df::unit *unit);
        /// Ref to outermost object unit is contained in. Possible ref types: UNIT, ITEM_GENERAL, VERMIN_EVENT
        /// (init_ref is used to initialize the ref to the unit itself before recursive calls.)
        DFHACK_EXPORT void getOuterContainerRef(df::specific_ref &spec_ref, df::unit *unit, bool init_ref = true);
        DFHACK_EXPORT inline df::specific_ref getOuterContainerRef(df::unit *unit) { df::specific_ref s; getOuterContainerRef(s, unit); return s; }

        // Unit's false identity or NULL.
        DFHACK_EXPORT df::identity *getIdentity(df::unit *unit);
        DFHACK_EXPORT df::nemesis_record *getNemesis(df::unit *unit);

        // Set unit's nickname properly.
        DFHACK_EXPORT void setNickname(df::unit *unit, std::string nick);
        // Unit's name accounting for false identities.
        DFHACK_EXPORT df::language_name *getVisibleName(df::unit *unit);

        // trainer_id -1 for any available.
        DFHACK_EXPORT bool assignTrainer(df::unit *unit, int32_t trainer_id = -1);
        DFHACK_EXPORT bool unassignTrainer(df::unit *unit);

        /// Makes the selected unit a member of the current fortress and site.
        /// Note that this function may silently fail for any of several reasons.
        /// If needed, use isOwnCiv or another relevant predicate after invoking
        /// to determine if the makeown operation was successful.
        DFHACK_EXPORT void makeown(df::unit *unit);

        /// Create new unit and add to all units vector (but not active). No HF, nemesis, pos,
        /// labors, or group associations. Will have race, caste, name, soul, body, and mind.
        DFHACK_EXPORT df::unit *create(int16_t race, int16_t caste);

        // Attribute values accounting for curses.
        DFHACK_EXPORT int getPhysicalAttrValue(df::unit *unit, df::physical_attribute_type attr);
        DFHACK_EXPORT int getMentalAttrValue(df::unit *unit, df::mental_attribute_type attr);
        DFHACK_EXPORT bool casteFlagSet(int race, int caste, df::caste_raw_flags flag);

        DFHACK_EXPORT df::unit_misc_trait *getMiscTrait(df::unit *unit, df::misc_trait_type type, bool create = false);

        // Raw token name (e.g., "DWARF")
        DFHACK_EXPORT std::string getRaceNameById(int32_t race_id);
        DFHACK_EXPORT std::string getRaceName(df::unit *unit);
        // Singular name (e.g., "dwarf")
        DFHACK_EXPORT std::string getRaceReadableNameById(int32_t race_id);
        DFHACK_EXPORT std::string getRaceReadableName(df::unit *unit);
        // Plural name (e.g., "dwarves")
        DFHACK_EXPORT std::string getRaceNamePluralById(int32_t race_id);
        DFHACK_EXPORT std::string getRaceNamePlural(df::unit *unit);
        // Baby name (e.g., "dwarven baby")
        DFHACK_EXPORT std::string getRaceBabyNameById(int32_t race_id, bool plural = false);
        DFHACK_EXPORT std::string getRaceBabyName(df::unit *unit, bool plural = false);
        // Child name (e.g., "dwarven child")
        DFHACK_EXPORT std::string getRaceChildNameById(int32_t race_id, bool plural = false);
        DFHACK_EXPORT std::string getRaceChildName(df::unit *unit, bool plural = false);
        // Full readable name, including profession.
        DFHACK_EXPORT std::string getReadableName(df::unit *unit);
        DFHACK_EXPORT std::string getPhysicalDescription(df::unit *unit);

        // Unit's age (in non-integer years). Ignore false identities if true_age.
        DFHACK_EXPORT double getAge(df::unit *unit, bool true_age = false);
        DFHACK_EXPORT int getKillCount(df::unit *unit);

        // Get skill level.
        DFHACK_EXPORT int getNominalSkill(df::unit *unit, df::job_skill skill_id, bool use_rust = false);
        // Takes into account skill rust, exhaustion, pain, etc.
        DFHACK_EXPORT int getEffectiveSkill(df::unit *unit, df::job_skill skill_id);
        // If total, take into account implied experience from current skill level.
        DFHACK_EXPORT int getExperience(df::unit *unit, df::job_skill skill_id, bool total = false);

        // Check if labor is settable for unit.
        DFHACK_EXPORT bool isValidLabor(df::unit *unit, df::unit_labor labor);
        // Set labor toggleability for entire fort.
        DFHACK_EXPORT bool setLaborValidity(df::unit_labor labor, bool isValid);

        // Currenty broken due to move speed changes. Always returns 0.
        DFHACK_EXPORT int computeMovementSpeed(df::unit *unit);
        DFHACK_EXPORT float computeSlowdownFactor(df::unit *unit);

        struct NoblePosition {
            df::historical_entity *entity = NULL;
            df::entity_position_assignment *assignment = NULL;
            df::entity_position *position = NULL;
        };

        DFHACK_EXPORT bool getNoblePositions(std::vector<NoblePosition> *pvec, df::unit *unit);

        // Unit's profession, accounting for false identity.
        DFHACK_EXPORT df::profession getProfession(df::unit *unit);

        /// String representing unit's profession. Can get the regular profession if ignore_noble is true.
        /// Otherwise will get the most important noble role (including noble consorts, prisoners, and slaves).
        /// If land_title is true, then append " of Sitename" when applicable.
        DFHACK_EXPORT std::string getProfessionName(df::unit *unit, bool ignore_noble = false, bool plural = false, bool land_title = false);
        DFHACK_EXPORT std::string getCasteProfessionName(int race, int caste, df::profession pid, bool plural = false);

        DFHACK_EXPORT int8_t getProfessionColor(df::unit *unit, bool ignore_noble = false);
        DFHACK_EXPORT int8_t getCasteProfessionColor(int race, int caste, df::profession pid);

        DFHACK_EXPORT df::goal_type getGoalType(df::unit *unit, size_t goalIndex = 0);
        DFHACK_EXPORT std::string getGoalName(df::unit *unit, size_t goalIndex = 0);
        DFHACK_EXPORT bool isGoalAchieved(df::unit *unit, size_t goalIndex = 0);

        DFHACK_EXPORT df::activity_entry *getMainSocialActivity(df::unit *unit);
        DFHACK_EXPORT df::activity_event *getMainSocialEvent(df::unit *unit);

        // Stress categories. 0 is highest stress, 6 is lowest.
        DFHACK_EXPORT extern const std::vector<int32_t> stress_cutoffs;
        DFHACK_EXPORT int getStressCategory(df::unit *unit);
        DFHACK_EXPORT int getStressCategoryRaw(int32_t stress_level);

        DFHACK_EXPORT void subtractActionTimers(color_ostream &out, df::unit *unit, int32_t amount, df::unit_action_type affectedActionType);
        DFHACK_EXPORT void subtractGroupActionTimers(color_ostream &out, df::unit *unit, int32_t amount, df::unit_action_type_group affectedActionTypeGroup);
        DFHACK_EXPORT void multiplyActionTimers(color_ostream &out, df::unit *unit, float amount, df::unit_action_type affectedActionType);
        DFHACK_EXPORT void multiplyGroupActionTimers(color_ostream &out, df::unit *unit, float amount, df::unit_action_type_group affectedActionTypeGroup);
        DFHACK_EXPORT void setActionTimers(color_ostream &out, df::unit *unit, int32_t amount, df::unit_action_type affectedActionType);
        DFHACK_EXPORT void setGroupActionTimers(color_ostream &out, df::unit *unit, int32_t amount, df::unit_action_type_group affectedActionTypeGroup);
    }
}
