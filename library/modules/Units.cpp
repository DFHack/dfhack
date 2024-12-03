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

#include "Core.h"
#include "Error.h"
#include "Internal.h"
#include "MemAccess.h"
#include "MiscUtils.h"
#include "ModuleFactory.h"
#include "Types.h"
#include "VersionInfo.h"

#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/activity_entry.h"
#include "df/burrow.h"
#include "df/caste_raw.h"
#include "df/creature_interaction_effect_display_namest.h"
#include "df/creature_raw.h"
#include "df/curse_attr_change.h"
#include "df/entity_position.h"
#include "df/entity_position_assignment.h"
#include "df/entity_raw.h"
#include "df/entity_raw_flags.h"
#include "df/entity_site_link.h"
#include "df/identity_type.h"
#include "df/game_mode.h"
#include "df/general_ref.h"
#include "df/histfig_entity_link_positionst.h"
#include "df/histfig_hf_link_spousest.h"
#include "df/histfig_relationship_type.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/historical_kills.h"
#include "df/history_event_hist_figure_diedst.h"
#include "df/identity.h"
#include "df/item.h"
#include "df/job.h"
#include "df/nemesis_record.h"
#include "df/plotinfost.h"
#include "df/syndrome.h"
#include "df/tile_occupancy.h"
#include "df/training_assignment.h"
#include "df/unit.h"
#include "df/unit_action.h"
#include "df/unit_action_type_group.h"
#include "df/unit_inventory_item.h"
#include "df/unit_misc_trait.h"
#include "df/unit_path_goal.h"
#include "df/unit_relationship_type.h"
#include "df/unit_skill.h"
#include "df/unit_soul.h"
#include "df/unit_syndrome.h"
#include "df/unit_wound.h"
#include "df/world.h"
#include "df/world_site.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <map>
#include <numeric>
#include <stddef.h>
#include <string>
#include <vector>

using std::max;
using std::min;
using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;
using df::global::gamemode;
using df::global::gametype;
using df::global::plotinfo;
using df::global::world;

#define IS_ACTIVE_CASTE_FLAG(cf) !unit->curse.rem_tags1.bits.cf && \
(unit->curse.add_tags1.bits.cf || casteFlagSet(unit->race, unit->caste, caste_raw_flags::cf))

// Common flags to exclude for fort members
constexpr uint32_t exclude_flags1 = (
    df::unit_flags1::mask_invader_origin | // isInvader
    df::unit_flags1::mask_active_invader | // isInvader
    df::unit_flags1::mask_diplomat | // isVisiting
    df::unit_flags1::mask_forest | // isForest
    df::unit_flags1::mask_merchant | // isVisiting
    df::unit_flags1::mask_marauder // isInvader
);
constexpr uint32_t exclude_flags2 = (
    df::unit_flags2::mask_visitor | // isVisiting
    df::unit_flags2::mask_visitor_uninvited | // isVisiting
    df::unit_flags2::mask_resident |
    df::unit_flags2::mask_underworld
);

bool Units::isActive(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return !unit->flags1.bits.inactive;
}

bool Units::isVisible(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return Maps::isTileVisible(getPosition(unit));
}

bool Units::isCitizen(df::unit *unit, bool include_insane) {
    CHECK_NULL_POINTER(unit);
    // Copied from the conditions used to decide game over,
    // except that the game appears to let melancholy/raving
    // dwarves count as citizens.
    if (unit->flags1.whole & exclude_flags1 ||
        unit->flags2.whole & exclude_flags2 ||
        (!include_insane && !isSane(unit)))
        return false;
    return isOwnGroup(unit);
}

bool Units::isResident(df::unit *unit, bool include_insane) {
    CHECK_NULL_POINTER(unit);
    if (!include_insane && !isSane(unit))
        return false;

    return isOwnCiv(unit) &&
        !isVisiting(unit) &&
        !isForest(unit) &&
        !isAnimal(unit) &&
        !isOwnGroup(unit);
}

bool Units::isFortControlled(df::unit *unit)
{   // Reverse-engineered from ambushing unit code
    CHECK_NULL_POINTER(unit);
    if (*gamemode != game_mode::DWARF)
        return false;

    if (unit->mood == mood_type::Berserk ||
        isCrazed(unit) ||
        isOpposedToLife(unit) ||
        unit->enemy.undead ||
        unit->flags3.bits.ghostly)
        return false;

    if (unit->flags1.whole & exclude_flags1)
        return false;
    else if (unit->flags1.bits.tame)
        return true;
    else if (unit->flags2.whole & exclude_flags2 || isAgitated(unit))
        return false;
    return isOwnCiv(unit);
}

bool Units::isOwnCiv(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->civ_id == plotinfo->civ_id;
}

bool Units::isOwnGroup(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto histfig = df::historical_figure::find(unit->hist_figure_id);
    if (!histfig)
        return false;

    for (auto link : histfig->entity_links)
        if (link->entity_id == plotinfo->group_id &&
            link->getType() == histfig_entity_link_type::MEMBER)
            return true;
    return false;
}

bool Units::isOwnRace(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->race == plotinfo->race_id;
}

bool Units::isAlive(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return !isDead(unit) && !unit->curse.add_tags1.bits.NOT_LIVING;
}

bool Units::isDead(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags2.bits.killed || unit->flags3.bits.ghostly;
}

bool Units::isKilled(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags2.bits.killed;
}

bool Units::isSane(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    if (isDead(unit) ||
        isOpposedToLife(unit) ||
        unit->enemy.undead)
        return false;

    if (unit->enemy.normal_race == unit->enemy.were_race && isCrazed(unit))
        return false;

    switch (unit->mood)
    {
        case mood_type::Melancholy:
        case mood_type::Raving:
        case mood_type::Berserk:
            return false;
        default:
            break;
    }
    return true;
}

bool Units::isCrazed(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    if (unit->flags3.bits.scuttle)
        return false;
    return IS_ACTIVE_CASTE_FLAG(CRAZED);
}

bool Units::isGhost(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags3.bits.ghostly;
}

bool Units::isHidden(df::unit *unit)
{   // Reverse-engineered from ambushing unit code
    CHECK_NULL_POINTER(unit);
    if (*df::global::debug_showambush)
        return false;

    if (*gamemode == game_mode::ADVENTURE)
    {
        if (unit == World::getAdventurer())
            return false;
        else if (unit->flags1.bits.hidden_in_ambush)
            return true;
    }
    else if (*gametype == game_type::DWARF_ARENA)
        return false;
    else if (unit->flags1.bits.hidden_in_ambush && !isFortControlled(unit))
        return true;

    if (unit->flags1.bits.caged)
    {
        auto spec_ref = getOuterContainerRef(unit);
        if (spec_ref.type == specific_ref_type::UNIT)
            return isHidden(spec_ref.data.unit);
    }
    else if (*gamemode == game_mode::ADVENTURE || isFortControlled(unit))
        return false;
    return !isVisible(unit);
}

bool Units::isHidingCurse(df::unit *unit) {
    auto identity = Units::getIdentity(unit);
    return identity && identity->type == df::identity_type::HidingCurse;
}

bool Units::isMale(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->sex == pronoun_type::he;
}

bool Units::isFemale(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->sex == pronoun_type::she;
}

bool Units::isBaby(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->profession == profession::BABY;
}

bool Units::isChild(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->profession == profession::CHILD;
}

bool Units::isAdult(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return !isBaby(unit) && !isChild(unit);
}

bool Units::isGay(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    if (!unit->status.current_soul)
        return false;

    df::orientation_flags orientation = unit->status.current_soul->orientation_flags;
    return (!Units::isFemale(unit) || !(orientation.whole & (orientation.mask_marry_male | orientation.mask_romance_male)))
           && (!Units::isMale(unit) || !(orientation.whole & (orientation.mask_marry_female | orientation.mask_romance_female)));
}

bool Units::isNaked(df::unit *unit, bool no_items) {
    CHECK_NULL_POINTER(unit);
    if (no_items)
        return unit->inventory.empty();

    for (auto inv_item : unit->inventory)
    {   // TODO: Check for proper coverage (bad thought)
        if (inv_item->mode == df::unit_inventory_item::Worn)
            return false;
    }
    return true;
}

bool Units::isVisiting(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags1.bits.merchant ||
        unit->flags1.bits.diplomat ||
        unit->flags2.bits.visitor ||
        unit->flags2.bits.visitor_uninvited;
}


bool Units::isTrainableHunting(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::TRAINABLE_HUNTING);
}

bool Units::isTrainableWar(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::TRAINABLE_WAR);
}

bool Units::isTrained(df::unit *unit) {
    using namespace animal_training_level;
    CHECK_NULL_POINTER(unit);
    // Case A: Trained for war/hunting (those don't have a training level, strangely)
    if(isWar(unit) || isHunter(unit))
        return true;
    // Case B: Tamed and trained wild creature, gets a training level,
    // (does not include domesticated creatures)
    switch (unit->training_level)
    {
        case Trained:
        case WellTrained:
        case SkilfullyTrained:
        case ExpertlyTrained:
        case ExceptionallyTrained:
        case MasterfullyTrained:
            return true;
        default:
            break;
    }
    return false;
}

bool Units::isHunter(df::unit *unit) {
    CHECK_NULL_POINTER(unit)
    return unit->profession == profession::TRAINED_HUNTER
        || unit->profession2 == profession::TRAINED_HUNTER;
}

bool Units::isWar(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->profession == profession::TRAINED_WAR
        || unit->profession2 == profession::TRAINED_WAR;
}

bool Units::isTame(df::unit *unit) {
    using namespace animal_training_level;
    CHECK_NULL_POINTER(unit);
    if(!unit->flags1.bits.tame)
        return false;

    switch (unit->training_level)
    {
        case SemiWild: // ??
        case Trained:
        case WellTrained:
        case SkilfullyTrained:
        case ExpertlyTrained:
        case ExceptionallyTrained:
        case MasterfullyTrained:
        case Domesticated:
            return true;
        case WildUntamed:
        default:
            break;
    }
    return false;
}

bool Units::isTamable(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    if (isInvader(unit))
        return false;
    auto caste = getCasteRaw(unit->race, unit->caste);

    return caste && (caste->flags.is_set(caste_raw_flags::PET) ||
        caste->flags.is_set(caste_raw_flags::PET_EXOTIC));
}

bool Units::isDomesticated(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags1.bits.tame
        && unit->training_level == animal_training_level::Domesticated;
}

static df::training_assignment *get_training_assignment(df::unit *unit) {
    return binsearch_in_vector(plotinfo->training.training_assignments,
        &df::training_assignment::animal_id, unit->id);
}

bool Units::isMarkedForTraining(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return get_training_assignment(unit) != NULL;
}

bool Units::isMarkedForTaming(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto assignment = get_training_assignment(unit);
    return assignment && !assignment->flags.bits.train_war && !assignment->flags.bits.train_hunt;
}

bool Units::isMarkedForWarTraining(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto assignment = get_training_assignment(unit);
    return assignment && assignment->flags.bits.train_war;
}

bool Units::isMarkedForHuntTraining(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto assignment = get_training_assignment(unit);
    return assignment && assignment->flags.bits.train_hunt;
}

bool Units::isMarkedForSlaughter(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags2.bits.slaughter;
}

bool Units::isMarkedForGelding(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags3.bits.marked_for_gelding;
}

bool Units::isGeldable(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::GELDABLE);
}

bool Units::isGelded(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    for(auto wound : unit->body.wounds)
        for (auto part : wound->parts)
            if (part->flags2.bits.gelded)
                return true;
    return false;
}

bool Units::isEggLayer(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto caste = getCasteRaw(unit->race, unit->caste);

    return caste && (caste->flags.is_set(caste_raw_flags::LAYS_EGGS) ||
        caste->flags.is_set(caste_raw_flags::LAYS_UNUSUAL_EGGS));
}

bool Units::isEggLayerRace(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto raw = df::creature_raw::find(unit->race);
    for (auto caste : raw->caste) {
        if (caste->flags.is_set(caste_raw_flags::LAYS_EGGS) ||
            caste->flags.is_set(caste_raw_flags::LAYS_UNUSUAL_EGGS))
            return true;
    }
    return false;
}

bool Units::isGrazer(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::GRAZER);
}

bool Units::isMilkable(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::MILKABLE);
}

bool Units::isForest(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags1.bits.forest;
}

bool Units::isMischievous(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return IS_ACTIVE_CASTE_FLAG(MISCHIEVOUS);
}

bool Units::isAvailableForAdoption(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags3.bits.available_for_adoption;
}

bool Units::isPet(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->relationship_ids[unit_relationship_type::PetOwner] != -1;
}

bool Units::hasExtravision(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return IS_ACTIVE_CASTE_FLAG(EXTRAVISION);
}

bool Units::isOpposedToLife(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return IS_ACTIVE_CASTE_FLAG(OPPOSED_TO_LIFE);
}

bool Units::isBloodsucker(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return IS_ACTIVE_CASTE_FLAG(BLOODSUCKER);
}
#undef IS_ACTIVE_CASTE_FLAG

bool Units::isDwarf(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->race == plotinfo->race_id
        || unit->enemy.normal_race == plotinfo->race_id;
}

bool Units::isAnimal(df::unit *unit) {
    using namespace caste_raw_flags;
    CHECK_NULL_POINTER(unit)
    auto &cf = unit->enemy.caste_flags;
    // We're (somewhat) matching Dwarf Therapist's animal check.
    // We care about wild animals too, however.
    return !cf.is_set(CAN_LEARN) && !cf.is_set(CAN_SPEAK) &&
        (unit->flags2.bits.roaming_wilderness_population_source ||
        cf.is_set(PET) ||
        cf.is_set(PET_EXOTIC) ||
        cf.is_set(TRAINABLE_WAR) || // These last two may be redundant
        cf.is_set(TRAINABLE_HUNTING));
}

bool Units::isMerchant(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags1.bits.merchant;
}

bool Units::isDiplomat(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags1.bits.diplomat;
}

bool Units::isVisitor(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags2.bits.visitor || unit->flags2.bits.visitor_uninvited;
}

bool Units::isWildlife(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->animal.population.population_idx >= 0
        && !isMerchant(unit)
        && !isForest(unit)
        && !isFortControlled(unit);
}

bool Units::isAgitated(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->flags4.bits.agitated_wilderness_creature;
}

bool Units::isInvader(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return (unit->flags1.bits.marauder ||
        unit->flags1.bits.invader_origin ||
        unit->flags1.bits.active_invader) &&
        !isOwnGroup(unit);
}

bool Units::isUndead(df::unit *unit, bool hiding_curse) {
    CHECK_NULL_POINTER(unit);
    const auto &cb = unit->curse.add_tags1.bits;
    return unit->flags3.bits.ghostly ||
        ((cb.OPPOSED_TO_LIFE || cb.NOT_LIVING) && (hiding_curse || !isHidingCurse(unit)));
}

bool Units::isNightCreature(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->enemy.caste_flags.is_set(caste_raw_flags::NIGHT_CREATURE);
}

bool Units::isSemiMegabeast(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->enemy.caste_flags.is_set(caste_raw_flags::SEMIMEGABEAST);
}

bool Units::isMegabeast(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->enemy.caste_flags.is_set(caste_raw_flags::MEGABEAST);
}

bool Units::isTitan(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->enemy.caste_flags.is_set(caste_raw_flags::TITAN);
}

bool Units::isForgottenBeast(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return unit->enemy.caste_flags.is_set(caste_raw_flags::FEATURE_BEAST);
}

bool Units::isDemon(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto &cf = unit->enemy.caste_flags;
    return cf.is_set(caste_raw_flags::DEMON)
        || cf.is_set(caste_raw_flags::UNIQUE_DEMON);
}

bool Units::isDanger(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    if (isTame(unit) || isOwnGroup(unit))
        return false;

    return isCrazed(unit)
        || isInvader(unit)
        || isAgitated(unit)
        || isSemiMegabeast(unit)
        || (!isVisiting(unit) && (isOpposedToLife(unit) || isNightCreature(unit)))
        || isGreatDanger(unit);
}

bool Units::isGreatDanger(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return isDemon(unit) || isTitan(unit) ||
        isMegabeast(unit) || isForgottenBeast(unit);
}

bool Units::isUnitInBox(df::unit *u, const cuboid &box) {
    CHECK_NULL_POINTER(u);
    return box.containsPos(getPosition(u));
}

bool Units::getUnitsInBox(vector<df::unit *> &units, const cuboid &box, std::function<bool(df::unit *)> filter) {
    if (!world)
        return false;

    units.clear();
    for (auto unit : world->units.active)
        if (filter(unit) && isUnitInBox(unit, box))
            units.push_back(unit);
    return true;
}

static int32_t get_noble_position_id(const df::historical_entity::T_positions &positions, const string &noble) {
    string target_id = toUpper_cp437(noble);
    for (auto position : positions.own)
        if (position->code == target_id)
            return position->id;
    return -1;
}

static void add_assigned_noble_units(vector<df::unit *> &units,
    const df::historical_entity::T_positions &positions, int32_t noble_position_id, size_t limit)
{
    for (auto assignment : positions.assignments) {
        if (assignment->position_id != noble_position_id)
            continue;
        auto histfig = df::historical_figure::find(assignment->histfig);
        if (!histfig)
            continue;
        auto unit = df::unit::find(histfig->unit_id);
        if (!unit)
            continue;
        units.emplace_back(unit);
        if (limit > 0 && units.size() >= limit)
            break;
    }
}

static void add_entity_nobles(vector<df::unit *> &units,
    string noble, size_t limit, df::historical_entity *he)
{
    if (!he)
        return;
    int32_t noble_position_id = get_noble_position_id(he->positions, noble);
    if (noble_position_id >= 0)
        add_assigned_noble_units(units, he->positions, noble_position_id, limit);
}


static void get_units_by_noble_role(vector<df::unit *> &units, string noble, size_t limit = 0) {
    if (!plotinfo)
        return;
    add_entity_nobles(units, noble, limit, df::historical_entity::find(plotinfo->civ_id));
    add_entity_nobles(units, noble, limit, df::historical_entity::find(plotinfo->group_id));
}

bool Units::getUnitsByNobleRole(vector<df::unit *> &units, string noble) {
    units.clear();
    get_units_by_noble_role(units, noble);
    return !units.empty();
}

df::unit *Units::getUnitByNobleRole(string noble) {
    vector<df::unit *> units;
    get_units_by_noble_role(units, noble, 1);
    if (units.empty())
        return NULL;
    return units[0];
}

void Units::forCitizens(std::function<void(df::unit *)> fn, bool exclude_residents, bool include_insane) {
    for (auto unit : citizensRange(world->units.active, exclude_residents, include_insane))
        fn(unit);
}

bool Units::getCitizens(vector<df::unit *> &citizens, bool exclude_residents, bool include_insane) {
    for (auto unit : citizensRange(world->units.active, exclude_residents, include_insane))
        citizens.emplace_back(unit);
    return true;
}

df::coord Units::getPosition(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    if (unit->flags1.bits.caged) {
        if (auto cage = getContainer(unit))
            return Items::getPosition(cage);
    }
    return unit->pos;
}

bool Units::teleport(df::unit *unit, df::coord target_pos)
{   // Make sure source and dest map blocks are valid
    auto old_occ = Maps::getTileOccupancy(unit->pos);
    auto new_occ = Maps::getTileOccupancy(target_pos);
    if (!old_occ || !new_occ)
        return false;

    // Clear appropriate occupancy flags at old tile
    if (unit->flags1.bits.on_ground)
        // This is potentially wrong, but the game will recompute this as needed
        old_occ->bits.unit_grounded = false;
    else
        old_occ->bits.unit = false;

    // If there's already somebody standing at the destination, then force the unit to lay down
    if (new_occ->bits.unit)
        unit->flags1.bits.on_ground = true;

    // Set appropriate occupancy flags at new tile
    if (unit->flags1.bits.on_ground)
        new_occ->bits.unit_grounded = true;
    else
        new_occ->bits.unit = true;

    // Move unit to destination
    unit->pos = target_pos;
    unit->idle_area = target_pos;

    // Move unit's riders (including babies) to destination
    if (unit->flags1.bits.ridden)
        for (auto rider : world->units.other[units_other_id::ANY_RIDER])
            if (rider->relationship_ids[unit_relationship_type::RiderMount] == unit->id)
                rider->pos = unit->pos;
    return true;
}

df::general_ref *Units::getGeneralRef(df::unit *unit, df::general_ref_type type) {
    CHECK_NULL_POINTER(unit);
    return findRef(unit->general_refs, type);
}

df::specific_ref *Units::getSpecificRef(df::unit *unit, df::specific_ref_type type) {
    CHECK_NULL_POINTER(unit);
    return findRef(unit->specific_refs, type);
}

df::item *Units::getContainer(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return findItemRef(unit->general_refs, general_ref_type::CONTAINED_IN_ITEM);
}

void Units::getOuterContainerRef(df::specific_ref &spec_ref, df::unit *unit, bool init_ref)
{   // Reverse-engineered from ambushing unit code
    CHECK_NULL_POINTER(unit);
    if (init_ref) {
        spec_ref.type = specific_ref_type::UNIT;
        spec_ref.data.unit = unit;
    }

    if (unit->flags1.bits.caged) {
        if (auto cage = getContainer(unit))
            return Items::getOuterContainerRef(spec_ref, cage);
    }
    return;
}

static df::identity *getFigureIdentity(df::historical_figure *figure) {
    if (figure && figure->info && figure->info->reputation)
        return df::identity::find(figure->info->reputation->cur_identity);
    return NULL;
}

df::identity *Units::getIdentity(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto figure = df::historical_figure::find(unit->hist_figure_id);
    return getFigureIdentity(figure);
}

df::nemesis_record *Units::getNemesis(df::unit *unit) {
    if (!unit)
        return NULL;

    for (auto gref : unit->general_refs) {
        auto nem = gref->getNemesis();
        if (nem && nem->unit == unit)
            return nem;
    }
    return NULL;
}

void Units::setNickname(df::unit *unit, string nick) {
    using namespace identity_type;
    CHECK_NULL_POINTER(unit);
    // There are >= 3 copies of the name, and the one
    // in the unit is not the authoritative one.
    // This is the reason why military units often
    // lose nicknames set from Dwarf Therapist.
    Translation::setNickname(&unit->name, nick);

    if (unit->status.current_soul)
        Translation::setNickname(&unit->status.current_soul->name, nick);

    auto figure = df::historical_figure::find(unit->hist_figure_id);
    if (!figure)
        return;

    Translation::setNickname(&figure->name, nick);

    auto identity = getFigureIdentity(figure);
    if (!identity)
        return;

    df::historical_figure *id_hfig = NULL;
    switch (identity->type)
    {
        case None:
        case HidingCurse:
        case FalseIdentity:
        case InfiltrationIdentity:
        case Identity:
            break; // We want the nickname to end up in the identity
        case Impersonating:
        case TrueName:
            id_hfig = df::historical_figure::find(identity->histfig_id);
            break;
    }

    if (id_hfig)
        Translation::setNickname(&id_hfig->name, nick);
    else
        Translation::setNickname(&identity->name, nick);
}

df::language_name *Units::getVisibleName(df::historical_figure *hf) {
    CHECK_NULL_POINTER(hf);
    if (auto identity = getFigureIdentity(hf))
    {
        auto imp_hf = df::historical_figure::find(identity->impersonated_hf);
        return (imp_hf && imp_hf->name.has_name) ? &imp_hf->name : &identity->name;
    }
    return &hf->name;
}

df::language_name *Units::getVisibleName(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto hf = df::historical_figure::find(unit->hist_figure_id);
    return hf ? getVisibleName(hf) : &unit->name;
}

bool Units::assignTrainer(df::unit *unit, int32_t trainer_id) {
    CHECK_NULL_POINTER(unit);
    if (!isTamable(unit) || isDomesticated(unit) || isMarkedForTraining(unit))
        return false;
    else if (trainer_id != -1 && !df::unit::find(trainer_id))
        return false;

    auto assignment = new df::training_assignment();
    assignment->animal_id = unit->id;
    assignment->trainer_id = trainer_id;
    assignment->flags.whole = 0;
    assignment->flags.bits.any_trainer = trainer_id == -1;

    insert_into_vector(plotinfo->training.training_assignments,
        &df::training_assignment::animal_id, assignment);
    return true;
}

bool Units::unassignTrainer(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return erase_from_vector(plotinfo->training.training_assignments,
        &df::training_assignment::animal_id, unit->id);
}

void Units::makeown(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto fp = df::global::unitst_make_own;
    CHECK_NULL_POINTER(fp);

    using FT = std::function<void(df::unit *)>;
    auto f = reinterpret_cast<FT *>(fp);
    (*f)(unit);
}

void Units::setAutomaticProfessions(df::unit* unit) {
    CHECK_NULL_POINTER(unit);
    auto fp = df::global::unitst_set_automatic_professions;
    CHECK_NULL_POINTER(fp);

    using FT = std::function<void(df::unit*)>;
    auto f = reinterpret_cast<FT*>(fp);
    (*f)(unit);
}


// functionality reverse-engineered from DF's unitst::set_goal
void Units::setPathGoal(df::unit *unit, df::coord pos, df::unit_path_goal goal)
{
    if (unit->path.dest != pos || unit->path.goal != goal)
    {
        unit->path.dest = pos;
        unit->path.goal = goal;
        unit->path.path.clear();
    }

    if (unit->flags1.bits.rider && unit->mount_type == df::rider_positions_type::STANDARD)
    {
        if (auto mount = df::unit::find(unit->relationship_ids[df::unit_relationship_type::RiderMount]))
        {
            mount->path.dest = pos;
            mount->path.goal = goal;
            mount->path.path.clear();
        }
    }
}

df::unit *Units::create(int16_t race, int16_t caste) {
    auto fp = df::global::unitst_more_convenient_create;
    CHECK_NULL_POINTER(fp);

    using FT = std::function<df::unit * (int16_t, int16_t)>;
    auto f = reinterpret_cast<FT *>(fp);
    return (*f)(race, caste);
}

df::caste_raw *Units::getCasteRaw(int race, int caste) {
    auto creature = df::creature_raw::find(race);
    return creature ? vector_get(creature->caste, caste) : NULL;
}

df::caste_raw *Units::getCasteRaw(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return getCasteRaw(unit->race, unit->caste);
}

int Units::getPhysicalAttrValue(df::unit *unit, df::physical_attribute_type attr) {
    auto &aobj = unit->body.physical_attrs[attr];
    int value = max(0, aobj.value - aobj.soft_demotion);

    if (auto mod = unit->curse.attr_change)
    {
        int mvalue = (value * mod->phys_att_perc[attr] / 100) + mod->phys_att_add[attr];
        if (isHidingCurse(unit))
            value = min(value, mvalue);
        else
            value = mvalue;
    }
    return max(0, value);
}

int Units::getMentalAttrValue(df::unit *unit, df::mental_attribute_type attr) {
    auto soul = unit->status.current_soul;
    if (!soul)
        return 0;

    auto &aobj = soul->mental_attrs[attr];
    int value = max(0, aobj.value - aobj.soft_demotion);

    if (auto mod = unit->curse.attr_change)
    {
        int mvalue = (value * mod->ment_att_perc[attr] / 100) + mod->ment_att_add[attr];
        if (isHidingCurse(unit))
            value = min(value, mvalue);
        else
            value = mvalue;
    }
    return max(0, value);
}

bool Units::casteFlagSet(int race, int caste, df::caste_raw_flags flag) {
    auto craw = getCasteRaw(race, caste);
    return craw ? craw->flags.is_set(flag) : false;
}

df::unit_misc_trait *Units::getMiscTrait(df::unit *unit, df::misc_trait_type type, bool create) {
    CHECK_NULL_POINTER(unit);
    auto &vec = unit->status.misc_traits;
    for (auto trait : vec)
        if (trait->id == type)
            return trait;

    if (create) {
        auto obj = new df::unit_misc_trait();
        obj->id = type;
        vec.push_back(obj);
        return obj;
    }
    return NULL;
}

string Units::getRaceNameById(int32_t id) {
    auto raw = df::creature_raw::find(id);
    return raw ? raw->creature_id : "";
}

string Units::getRaceName(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return getRaceNameById(unit->race);
}

string Units::getRaceNamePluralById(int32_t id) {
    auto raw = df::creature_raw::find(id);
    return raw ? raw->name[1] : ""; // Plural
}

string Units::getRaceNamePlural(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return getRaceNamePluralById(unit->race);
}

string Units::getRaceReadableNameById(int32_t id) {
    auto raw = df::creature_raw::find(id);
    return raw ? raw->name[0] : ""; // Singular
}

string Units::getRaceReadableName(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    return getRaceReadableNameById(unit->race);
}

string Units::getRaceBabyNameById(int32_t id, bool plural) {
    if (auto raw = df::creature_raw::find(id))
    {
        string &baby_name = raw->general_baby_name[plural ? 1 : 0];
        if (baby_name.empty())
            return getRaceReadableNameById(id) + (plural ? " babies" : " baby");
        return baby_name;
    }
    return "";
}

string Units::getRaceBabyName(df::unit *unit, bool plural) {
    CHECK_NULL_POINTER(unit);
    return getRaceBabyNameById(unit->race, plural);
}

string Units::getRaceChildNameById(int32_t id, bool plural) {
    if (auto raw = df::creature_raw::find(id))
    {
        string &child_name = raw->general_child_name[plural ? 1 : 0];
        if (child_name.empty())
            return getRaceReadableNameById(id) + (plural ? " children" : " child");
        return child_name;
    }
    return "";
}

string Units::getRaceChildName(df::unit *unit, bool plural) {
    CHECK_NULL_POINTER(unit);
    return getRaceChildNameById(unit->race, plural);
}

static string getTameTag(df::unit *unit) {
    using namespace animal_training_level;
    switch (unit->training_level)
    {
        case SemiWild:
            return "semi-wild";
        case Trained:
            return "trained";
        case WellTrained:
            return "-trained-";
        case SkilfullyTrained:
            return "+trained+";
        case ExpertlyTrained:
            return "*trained*";
        case ExceptionallyTrained:
            return "\xF0trained\xF0"; // "≡trained≡"
        case MasterfullyTrained:
            return "\x0Ftrained\x0F"; // "☼trained☼"
        case Domesticated:
            return "tame";
        case WildUntamed:
        default:
            return "wild";
    }
}

string Units::getReadableName(df::historical_figure *hf) {
    CHECK_NULL_POINTER(hf);
    string prof_name = getProfessionName(hf, false, false, true);

    if (hf->info && hf->info->curse) {
        auto &curse = *hf->info->curse;
        if (curse.original_race >= 0 && !curse.undead_name.empty()) // Zombie
            prof_name = curse.undead_name; // Replaces entire profession
        else if (curse.use_display_name && !curse.name.empty()) // Necromancer, etc.
            prof_name += " " + curse.name;
        else if (hf->flags.is_set(histfig_flags::ghost))
            prof_name = "Ghostly " + prof_name;
    }

    string name = Translation::TranslateName(getVisibleName(hf));
    return name.empty() ? prof_name : name + ", " + prof_name;
}

string Units::getReadableName(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    string prof_name = getProfessionName(unit, false, false, true);

    if (unit->curse.name_visible) // Necromancer, etc.
        prof_name += " " + unit->curse.name;
    else if (isGhost(unit)) // TODO: Should be "Ghost" instead of "Ghostly Peasant"
        prof_name = "Ghostly " + prof_name;
    // TODO: impersonating deity/force
    if (unit->enemy.undead) { // Zombie
        if (unit->enemy.undead->undead_name.empty())
            prof_name += " Corpse";
        else // Replaces entire profession
            prof_name = unit->enemy.undead->undead_name;
    }

    if (isTame(unit))
        prof_name += " (" + getTameTag(unit) + ")";

    string name = Translation::TranslateName(getVisibleName(unit));
    return name.empty() ? prof_name : name + ", " + prof_name;
}

double Units::getAge(df::unit *unit, bool true_age) {
    using df::global::cur_year;
    using df::global::cur_year_tick;
    CHECK_NULL_POINTER(unit);

    if (!cur_year || !cur_year_tick)
        return -1;
    double year_ticks = 403200.0;
    double birth_time = unit->birth_year + unit->birth_time / year_ticks;
    double cur_time = *cur_year + *cur_year_tick / year_ticks;

    if (!true_age)
        if (auto identity = getIdentity(unit))
            if (identity->birth_year != -1)
                birth_time = identity->birth_year + identity->birth_second / year_ticks;

    return cur_time - birth_time;
}

int Units::getKillCount(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto hf = df::historical_figure::find(unit->hist_figure_id);
    if (!hf || !hf->info || !hf->info->kills)
        return 0;

    auto kills = hf->info->kills;
    int count = std::accumulate(kills->killed_count.begin(), kills->killed_count.end(), 0);
    for (auto event : kills->events)
        if (virtual_cast<df::history_event_hist_figure_diedst>(df::history_event::find(event)))
            count++;
    return count;
}

static inline void adjust_skill_rating(int &rating, bool is_adventure, int value,
    int dwarf3_4, int dwarf1_2, int adv9_10, int adv3_4, int adv1_2)
{
    if (is_adventure) {
        if (value >= adv1_2)
            rating >>= 1;
        else if (value >= adv3_4)
            rating = rating*3/4;
        else if (value >= adv9_10)
            rating = rating*9/10;
    }
    else {
        if (value >= dwarf1_2)
            rating >>= 1;
        else if (value >= dwarf3_4)
            rating = rating*3/4;
    }
}

int Units::getNominalSkill(df::unit *unit, df::job_skill skill_id, bool use_rust) {
    CHECK_NULL_POINTER(unit);
    // This is 100% reverse-engineered from DF code
    if (!unit->status.current_soul)
        return 0;
    // Retrieve skill from unit soul
    auto skill = binsearch_in_vector(unit->status.current_soul->skills, &df::unit_skill::id, skill_id);
    if (skill) {
        int rating = int(skill->rating);
        if (use_rust)
            rating -= skill->rusty;
        return max(0, rating);
    }
    return 0;
}

int Units::getEffectiveSkill(df::unit *unit, df::job_skill skill_id) {
    CHECK_NULL_POINTER(unit);
    // This is 100% reverse-engineered from DF code
    int rating = getNominalSkill(unit, skill_id, true);
    // Apply special states
    if (unit->counters.soldier_mood == df::unit::T_counters::None) {
        if (unit->counters.nausea > 0)
            rating >>= 1;
        if (unit->counters.winded > 0)
            rating >>= 1;
        if (unit->counters.stunned > 0)
            rating >>= 1;
        if (unit->counters.dizziness > 0)
            rating >>= 1;
        if (unit->counters2.fever > 0)
            rating >>= 1;
    }

    if (unit->counters.soldier_mood != df::unit::T_counters::MartialTrance) {
        if (!unit->flags3.bits.ghostly && !unit->flags3.bits.scuttle &&
            !unit->flags2.bits.vision_good && !unit->flags2.bits.vision_damaged &&
            !hasExtravision(unit))
        {
            rating >>= 2;
        }

        if (unit->counters.pain >= 100 && unit->mood == -1)
            rating >>= 1;

        if (unit->counters2.exhaustion >= 2000)
        {
            rating = rating*3/4;
            if (unit->counters2.exhaustion >= 4000)
            {
                rating = rating*3/4;
                if (unit->counters2.exhaustion >= 6000)
                    rating = rating*3/4;
            }
        }
    }

    // Hunger, etc., timers
    bool is_adventure = (gamemode && *gamemode == game_mode::ADVENTURE);

    if (!unit->flags3.bits.scuttle && isBloodsucker(unit)) {
        if (auto trait = getMiscTrait(unit, misc_trait_type::TimeSinceSuckedBlood))
        {
            adjust_skill_rating(
                rating, is_adventure, trait->value,
                302400, 403200,           // dwf 3/4; 1/2
                1209600, 1209600, 2419200 // adv 9/10; 3/4; 1/2
            );
        }
    }

    adjust_skill_rating(
        rating, is_adventure, unit->counters2.thirst_timer,
        50000, 50000, 115200, 172800, 345600
    );
    adjust_skill_rating(
        rating, is_adventure, unit->counters2.hunger_timer,
        75000, 75000, 172800, 1209600, 2592000
    );

    if (is_adventure && unit->counters2.sleepiness_timer >= 846000)
        rating >>= 2;
    else
    {
        adjust_skill_rating(
            rating, is_adventure, unit->counters2.sleepiness_timer,
            150000, 150000, 172800, 259200, 345600
        );
    }
    return rating;
}

int Units::getExperience(df::unit *unit, df::job_skill skill_id, bool total) {
    CHECK_NULL_POINTER(unit);
    if (!unit->status.current_soul)
        return 0;

    auto skill = binsearch_in_vector(unit->status.current_soul->skills, &df::unit_skill::id, skill_id);
    if (!skill)
        return 0;

    int xp = skill->experience;
    int rating = skill->rating;
    // Exact formula used by the game
    if (total && skill->rating > 0)
        xp += 500*rating + 100*rating*(rating - 1)/2;
    return xp;
}

bool Units::isValidLabor(df::unit *unit, df::unit_labor labor) {
    CHECK_NULL_POINTER(unit);
    if (!is_valid_enum_item(labor) || labor == unit_labor::NONE)
        return false;

    auto entity = df::historical_entity::find(unit->civ_id);
    return !entity || !entity->entity_raw || entity->entity_raw->jobs.permitted_labor[labor];
}

bool Units::setLaborValidity(df::unit_labor labor, bool isValid) {
    if (!is_valid_enum_item(labor) || labor == unit_labor::NONE)
        return false;
    auto entity = df::historical_entity::find(plotinfo->civ_id);
    if (!entity || !entity->entity_raw)
        return false;
    entity->entity_raw->jobs.permitted_labor[labor] = isValid;
    return true;
}
/*
static inline void adjust_speed_rating(int &rating, bool is_adventure, int value,
    int dwarf100, int dwarf200, int adv50, int adv75, int adv100, int adv200)
{
    if  (is_adventure) {
        if (value >= adv200)
            rating += 200;
        else if (value >= adv100)
            rating += 100;
        else if (value >= adv75)
            rating += 75;
        else if (value >= adv50)
            rating += 50;
    }
    else {
        if (value >= dwarf200)
            rating += 200;
        else if (value >= dwarf100)
            rating += 100;
    }
}

static int calcInventoryWeight(df::unit *unit) {
    int armor_skill = Units::getEffectiveSkill(unit, job_skill::ARMOR);
    int armor_mul = 15 - min(15, armor_skill);

    int inv_weight = 0, inv_weight_fraction = 0;

    for (auto inv_item : unit->inventory) {
        auto item = inv_item->item;
        if (!item->flags.bits.weight_computed)
            continue;

        int wval = item->weight.whole;
        int wfval = item->weight.fraction;
        auto mode = inv_item->mode;

        if ((mode == df::unit_inventory_item::Worn ||
             mode == df::unit_inventory_item::WrappedAround) &&
             item->isArmor() && armor_skill > 1)
        {
            wval = wval * armor_mul / 16;
            wfval = wfval * armor_mul / 16;
        }

        inv_weight += wval;
        inv_weight_fraction += wfval;
    }
    return inv_weight*100 + inv_weight_fraction / 10000;
}
*/
int Units::computeMovementSpeed(df::unit *unit)
{   // TODO: Fix for v50
    CHECK_NULL_POINTER(unit);
    // Pure reverse-engineered computation of unit _slowness_,
    // i.e., number of ticks to move * 100.

    // Base speed
    int speed = 0;
    /*
    auto craw = getCasteRaw(unit->race, unit->caste);
    if (!craw)
        return 0;

    int speed = craw->misc.unused_01; // Formerly misc.speed

    if (unit->flags3.bits.ghostly)
        return speed;

    // Curse multiplier
    if (unit->curse.speed_mul_percent != 100)
    {
        speed *= 100;
        if (unit->curse.speed_mul_percent != 0)
            speed /= unit->curse.speed_mul_percent;
    }
    speed += unit->curse.speed_add;

    // Swimming
    bool in_magma = (unit->status2.liquid_type.bits.liquid_type == tile_liquid::Magma);
    if (unit->flags2.bits.swimming)
    {
        speed = craw->misc.unused_10; // Formerly misc.swim_speed
        if (in_magma)
            speed *= 2;

        if (craw->flags.is_set(caste_raw_flags::CAN_SWIM))
        {
            int skill = Units::getEffectiveSkill(unit, job_skill::SWIMMING);
            // Originally a switch
            if (skill > 1)
                speed = speed * max(6, 21-skill) / 20;
        }
    }
    else {
        int delta = 150*unit->status2.liquid_depth;
        if (in_magma)
            delta *= 2;
        speed += delta;
    }

    // General counters and flags
    if (isBaby(unit))
        speed += 3000;

    if (unit->flags3.bits.diving)
        speed /= 20;

    if (unit->counters2.exhaustion >= 2000)
    {
        speed += 200;
        if (unit->counters2.exhaustion >= 4000)
        {
            speed += 200;
            if (unit->counters2.exhaustion >= 6000)
                speed += 200;
        }
    }

    if (unit->flags2.bits.gutted)
        speed += 2000;

    if (unit->counters.soldier_mood == df::unit::T_counters::None) {
        if (unit->counters.nausea > 0)
            speed += 1000;
        if (unit->counters.winded > 0)
            speed += 1000;
        if (unit->counters.stunned > 0)
            speed += 1000;
        if (unit->counters.dizziness > 0)
            speed += 1000;
        if (unit->counters2.fever > 0)
            speed += 1000;
    }

    if (unit->counters.soldier_mood != df::unit::T_counters::MartialTrance) {
        if (unit->counters.pain >= 100 && unit->mood == -1)
            speed += 1000;
    }

    // Hunger, etc., timers
    bool is_adventure = (gamemode && *gamemode == game_mode::ADVENTURE);
    if (!unit->flags3.bits.scuttle && Units::isBloodsucker(unit)) {
        if (auto trait = Units::getMiscTrait(unit, misc_trait_type::TimeSinceSuckedBlood))
        {
            adjust_speed_rating(
                speed, is_adventure, trait->value,
                302400, 403200,                    // dwf 100; 200
                1209600, 1209600, 1209600, 2419200 // adv 50; 75; 100; 200
            );
        }
    }

    adjust_speed_rating(
        speed, is_adventure, unit->counters2.thirst_timer,
        50000, 0x7fffffff, 172800, 172800, 172800, 345600
    );
    adjust_speed_rating(
        speed, is_adventure, unit->counters2.hunger_timer,
        75000, 0x7fffffff, 1209600, 1209600, 1209600, 2592000
    );
    adjust_speed_rating(
        speed, is_adventure, unit->counters2.sleepiness_timer,
        57600, 150000, 172800, 259200, 345600, 864000
    );

    // Activity state
    if (unit->relationship_ids[unit_relationship_type::Draggee] != -1)
        speed += 1000;

    if (unit->flags1.bits.on_ground)
        speed += 2000;
    else if (unit->flags3.bits.on_crutch)
    {
        int skill = Units::getEffectiveSkill(unit, job_skill::CRUTCH_WALK);
        speed += 2000 - 100*min(20, skill);
    }

    if (unit->flags1.bits.hidden_in_ambush && !Units::isMischievous(unit))
    {
        int skill = Units::getEffectiveSkill(unit, job_skill::SNEAK);
        speed += 2000 - 100*min(20, skill);
    }

    if (unsigned(unit->counters2.paralysis-1) <= 98)
        speed += unit->counters2.paralysis*10;

    if (unsigned(unit->counters.webbed-1) <= 8)
        speed += unit->counters.webbed*100;

    // Muscle and fat weight vs expected size
    auto &s_info = unit->body.size_info;
    speed = max(speed*3/4, min(speed*3/2, int(int64_t(speed)*s_info.size_cur/s_info.size_base)));

    // Attributes
    int strength_attr = Units::getPhysicalAttrValue(unit, physical_attribute_type::STRENGTH);
    int agility_attr = Units::getPhysicalAttrValue(unit, physical_attribute_type::AGILITY);

    int total_attr = max(200, min(3800, strength_attr + agility_attr));
    speed = ((total_attr-200)*(speed/2) + (3800-total_attr)*(speed*3/2))/3600;

    // Stance
    if (!unit->flags1.bits.on_ground && unit->status2.limbs_stand_max > 2)
    {   // WTF
        int as = unit->status2.limbs_stand_max;
        int x = (as-1) - (as>>1);
        int y = as - unit->status2.limbs_stand_count;
        if (unit->flags3.bits.on_crutch) y--;
        y = y * 500 / x;
        if (y > 0) speed += y;
    }

    // Mood
    if (unit->mood == mood_type::Melancholy)
        speed += 8000;

    // Inventory encumberance
    int total_weight = calcInventoryWeight(unit);
    int free_weight = max(1, s_info.size_cur/10 + strength_attr*3);

    if (free_weight < total_weight)
    {
        int delta = (total_weight - free_weight)/10 + 1;
        if (!is_adventure)
            delta = min(5000, delta);
        speed += delta;
    }

    // Skipped: unknown loop on inventory items that amounts to 0 change

    if (is_adventure) {
        auto player = World::getAdventurer();
        if (player && player->id == unit->relationship_ids[unit_relationship_type::GroupLeader])
            speed = min(speed, computeMovementSpeed(player));
    }
    */
    return min(10000, max(0, speed));
}

static bool entityRawFlagSet(int civ_id, df::entity_raw_flags flag) {
    auto entity = df::historical_entity::find(civ_id);
    return entity && entity->entity_raw && entity->entity_raw->flags.is_set(flag);
}

float Units::computeSlowdownFactor(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    // These slowdowns are actually done by skipping a move if random(x) != 0,
    // so it follows the geometric distribution. The mean expected slowdown is x.

    float coeff = 1.0f;

    if (!unit->job.hunt_target && (!gamemode || *gamemode == game_mode::DWARF)) {
        if (!unit->flags1.bits.marauder &&
            casteFlagSet(unit->race, unit->caste, caste_raw_flags::MEANDERER) &&
            !(unit->following && isCitizen(unit)) &&
            linear_index(unit->inventory, &df::unit_inventory_item::mode, df::unit_inventory_item::Hauled) < 0)
        {
            coeff *= 4.0f;
        }

        if (unit->relationship_ids[unit_relationship_type::GroupLeader] < 0 &&
            unit->flags1.bits.active_invader &&
            !unit->job.current_job && !unit->flags3.bits.no_meandering &&
            unit->profession != profession::THIEF && unit->profession != profession::MASTER_THIEF &&
            !entityRawFlagSet(unit->civ_id, entity_raw_flags::ITEM_THIEF))
        {
            coeff *= 3.0f;
        }
    }

    if (unit->flags3.bits.floundering)
        coeff *= 3.0f;
    return coeff;
}

static bool noble_pos_compare(const Units::NoblePosition &a, const Units::NoblePosition &b) {
    if (a.position->precedence < b.position->precedence)
        return true;
    else if (a.position->precedence > b.position->precedence)
        return false;
    return a.position->id < b.position->id;
}

bool Units::getNoblePositions(vector<NoblePosition> *pvec, df::historical_figure *hf) {
    pvec->clear();
    if (!hf)
        return false;

    for (auto link: hf->entity_links) {
        auto epos = strict_virtual_cast<df::histfig_entity_link_positionst>(link);
        if (!epos)
            continue;

        NoblePosition npos;

        npos.entity = df::historical_entity::find(epos->entity_id);
        if (!npos.entity)
            continue;

        npos.assignment = binsearch_in_vector(npos.entity->positions.assignments, epos->assignment_id);
        if (!npos.assignment)
            continue;

        npos.position = binsearch_in_vector(npos.entity->positions.own, npos.assignment->position_id);
        if (!npos.position)
            continue;

        pvec->push_back(npos);
    }

    if (pvec->empty())
        return false;

    std::sort(pvec->begin(), pvec->end(), noble_pos_compare);
    return true;
}

bool Units::getNoblePositions(vector<NoblePosition> *pvec, df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    auto hf = df::historical_figure::find(unit->hist_figure_id);
    return getNoblePositions(pvec, hf);
}

df::profession Units::getProfession(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    if (auto identity = getIdentity(unit))
        return identity->profession;
    return unit->profession;
}

static string get_land_title(Units::NoblePosition *np)
{   // Utility fn to get " of Sitename" string.
    CHECK_NULL_POINTER(np);
    if (np->position->land_holder <= 0)
        return ""; // Not a baron, count, or duke equivalent.
    for (auto site_link : np->entity->site_links)
        if (site_link->flags.bits.land_for_holding && site_link->position_profile_id == np->assignment->id)
        {
            auto site = df::world_site::find(site_link->target);
            return site ? " of " + Translation::TranslateName(&site->name, true) : "";
        }
    return "";
}

static string get_noble_title(df::pronoun_type my_sex, bool plural, bool land_title,
    Units::NoblePosition *np = NULL, Units::NoblePosition *np_spouse = NULL)
{   // Utility fn to get (spouse) title string of more important noble position.
    string result;
    int32_t plural_idx = plural ? 1 : 0;
    // Try spouse title
    if (np_spouse && (!np || np_spouse->position->precedence < np->position->precedence)) {
        switch (my_sex)
        {
            case pronoun_type::she:
                result = np_spouse->position->spouse_female[plural_idx];
                break;
            case pronoun_type::he:
                result = np_spouse->position->spouse_male[plural_idx];
                break;
            default:
                break;
        }
        if (result.empty()) // Try generic title
            result = np_spouse->position->spouse[plural_idx];
        if (!result.empty()) // Success
            return land_title ? result + get_land_title(np_spouse) : result;
    }
    // Try our own title
    if (np) {
        switch (my_sex)
        {
            case pronoun_type::she:
                result = np->position->name_female[plural_idx];
                break;
            case pronoun_type::he:
                result = np->position->name_male[plural_idx];
                break;
            default:
                break;
        }
        if (result.empty()) // Try generic title
            result = np->position->name[plural_idx];
        if (!result.empty()) // Success
            return land_title ? result + get_land_title(np) : result;
    }
    return ""; // Not a noble
}

static bool has_active_entity_link(df::historical_figure *hf, df::histfig_entity_link_type type)
{   // Returns true if first link of type is non-zero.
    CHECK_NULL_POINTER(hf);
    for (auto link : hf->entity_links) {
        if (link->getType() == type)
            return link->link_strength > 0;
    }
    return false;
}

static df::historical_figure *find_spouse_hf(df::historical_figure *my_hf) {
    CHECK_NULL_POINTER(my_hf);
    for (auto link : my_hf->histfig_links) {
        if (auto spouse_link = strict_virtual_cast<df::histfig_hf_link_spousest>(link))
            return df::historical_figure::find(spouse_link->target_hf);
    }
    return NULL;
}

static string get_linked_title(df::historical_figure *hf, bool plural, bool land_title)
{   // Utility fn for getProfessionName common code.
    // TODO: Change ignore_noble and land_title to enum, option to ignore spouse/prisoner/slave?
    if (!hf)
        return "";
    else if (has_active_entity_link(hf, histfig_entity_link_type::PRISONER))
        return "Prisoner";
    else if (has_active_entity_link(hf, histfig_entity_link_type::SLAVE))
        return "Slave";

    vector<Units::NoblePosition> np, np_spouse;
    getNoblePositions(&np, hf);
    getNoblePositions(&np_spouse, find_spouse_hf(hf));

    return get_noble_title(hf->sex, plural, land_title,
        (np.empty() ? NULL : &np[0]), (np_spouse.empty() ? NULL : &np_spouse[0]));
}

string Units::getProfessionName(df::historical_figure *hf, bool ignore_noble, bool plural, bool land_title) {
    CHECK_NULL_POINTER(hf);
    if (!ignore_noble) {
        string title = get_linked_title(hf, plural, land_title);
        if (!title.empty())
            return title;
    }
    auto fi = getFigureIdentity(hf);
    return getCasteProfessionName(hf->race, hf->caste, (fi ? fi->profession : hf->profession), plural);
}

string Units::getProfessionName(df::unit *unit, bool ignore_noble, bool plural, bool land_title) {
    CHECK_NULL_POINTER(unit);
    string prof = unit->custom_profession;
    if (!prof.empty())
        return prof;
    else if (!ignore_noble) {
        prof = get_linked_title(df::historical_figure::find(unit->hist_figure_id), plural, land_title);
        if (!prof.empty())
            return prof;
    }
    prof = getCasteProfessionName(unit->race, unit->caste, getProfession(unit), plural);
    return isAgitated(unit) ? "Agitated " + prof : prof;
}

string Units::getCasteProfessionName(int race, int casteid, df::profession pid, bool plural) {
    string prof, race_prefix;
    if (!is_valid_enum_item(pid) || pid == profession::NONE)
        return "";

    int16_t current_race = plotinfo->race_id;
    if (auto adv = World::getAdventurer())
        current_race = adv->race;
    bool use_race_prefix = (race >= 0 && race != current_race); // TODO: Use anyway if unit->curse.name_visible and not noble

    if (auto creature = df::creature_raw::find(race)) {
        if (auto caste = vector_get(creature->caste, casteid))
        {
            race_prefix = caste->caste_name[0];

            if (plural)
                prof = caste->caste_profession_name.plural[pid];
            else
                prof = caste->caste_profession_name.singular[pid];

            if (prof.empty()) {
                switch (pid)
                {
                    case profession::CHILD:
                        prof = caste->child_name[plural ? 1 : 0];
                        if (!prof.empty())
                            use_race_prefix = false;
                        break;
                    case profession::BABY:
                        prof = caste->baby_name[plural ? 1 : 0];
                        if (!prof.empty())
                            use_race_prefix = false;
                        break;
                    default:
                        break;
                }
            }
        }

        if (race_prefix.empty())
            race_prefix = creature->name[0];

        if (prof.empty()) {
            if (plural)
                prof = creature->profession_name.plural[pid];
            else
                prof = creature->profession_name.singular[pid];

            if (prof.empty()) {
                switch (pid)
                {
                    case profession::CHILD:
                        prof = creature->general_child_name[plural ? 1 : 0];
                        if (!prof.empty())
                            use_race_prefix = false;
                        break;
                    case profession::BABY:
                        prof = creature->general_baby_name[plural ? 1 : 0];
                        if (!prof.empty())
                            use_race_prefix = false;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (race_prefix.empty())
        race_prefix = "Animal";

    if (prof.empty()) {
        switch (pid)
        {   // TODO: Handle hardcoded plurals
            case profession::TRAINED_WAR:
                prof = "War " + (use_race_prefix ? race_prefix : "Peasant");
                use_race_prefix = false;
                break;
            case profession::TRAINED_HUNTER:
                prof = "Hunting " + (use_race_prefix ? race_prefix : "Peasant");
                use_race_prefix = false;
                break;
            case profession::STANDARD:
                if (!use_race_prefix)
                    prof = "Peasant";
                break;
            default:
                if (auto caption = ENUM_ATTR(profession, caption, pid))
                    prof = caption;
                else
                    prof = ENUM_KEY_STR(profession, pid);
        }
    }

    if (use_race_prefix) { // TODO: Handle "Ghostly Peasant" -> "Ghost" here?
        if (!prof.empty())
            race_prefix += " ";
        prof = race_prefix + prof;
    }
    return Translation::capitalize(prof, true);
}

int8_t Units::getProfessionColor(df::unit *unit, bool ignore_noble) {
    CHECK_NULL_POINTER(unit);
    vector<NoblePosition> np;
    if (!ignore_noble && getNoblePositions(&np, unit)) // TODO: Account for Prisoner/Slave
        if (np[0].position->flags.is_set(entity_position_flags::COLOR))
            return np[0].position->color[0] + np[0].position->color[2] * 8;
    return getCasteProfessionColor(unit->race, unit->caste, unit->profession);
}

int8_t Units::getCasteProfessionColor(int race, int casteid, df::profession pid) {
    if (!is_valid_enum_item(pid) || pid == profession::NONE)
        return 3;
    // If it's not a Peasant, it's hardcoded
    if (pid != profession::STANDARD)
        return ENUM_ATTR(profession, color, pid);

    if (auto creature = df::creature_raw::find(race)) {
        if (auto caste = vector_get(creature->caste, casteid)) {
            if (caste->flags.is_set(caste_raw_flags::HAS_COLOR))
                return caste->caste_color[0] + caste->caste_color[2] * 8;
        }
        return creature->color[0] + creature->color[2] * 8;
    }
    // Default to dwarven peasant color
    return 3;
}

df::goal_type Units::getGoalType(df::unit *unit, size_t goalIndex) {
    CHECK_NULL_POINTER(unit);
    df::goal_type goal = df::goal_type::STAY_ALIVE;
    if (unit->status.current_soul && unit->status.current_soul->personality.dreams.size() > goalIndex)
        goal = unit->status.current_soul->personality.dreams[goalIndex]->type;
    return goal;
}

string Units::getGoalName(df::unit *unit, size_t goalIndex) {
    CHECK_NULL_POINTER(unit);
    df::goal_type goal = getGoalType(unit, goalIndex);
    bool achieved_goal = isGoalAchieved(unit, goalIndex);

    string goal_name = achieved_goal ? ENUM_ATTR(goal_type, achieved_short_name, goal) : ENUM_ATTR(goal_type, short_name, goal);
    if (goal == df::goal_type::START_A_FAMILY)
    {
        string parent = ENUM_KEY_STR(histfig_relationship_type, histfig_relationship_type::Parent);
        size_t start_pos = goal_name.find(parent);
        if (start_pos != string::npos)
        {
            df::histfig_relationship_type parent_type = isFemale(unit) ? histfig_relationship_type::Mother : histfig_relationship_type::Father;
            goal_name.replace(start_pos, parent.length(), ENUM_KEY_STR(histfig_relationship_type, parent_type));
        }
    }
    return goal_name;
}

bool Units::isGoalAchieved(df::unit *unit, size_t goalIndex) {
    CHECK_NULL_POINTER(unit);
    return unit->status.current_soul
        && unit->status.current_soul->personality.dreams.size() > goalIndex
        && unit->status.current_soul->personality.dreams[goalIndex]->flags.bits.accomplished;
}

df::activity_entry *Units::getMainSocialActivity(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    if (unit->social_activities.empty())
        return nullptr;
    return df::activity_entry::find(unit->social_activities[unit->social_activities.size() - 1]);
}

df::activity_event *Units::getMainSocialEvent(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    df::activity_entry *entry = getMainSocialActivity(unit);
    if (!entry || entry->events.empty())
        return nullptr;
    return entry->events[entry->events.size() - 1];
}

// 50000 and up is level 0, 25000 and up is level 1, etc.
const vector<int32_t> Units::stress_cutoffs {50000, 25000, 10000, -10000, -25000, -50000, -100000};

int Units::getStressCategory(df::unit *unit) {
    CHECK_NULL_POINTER(unit);
    if (!unit->status.current_soul)
        return int(stress_cutoffs.size()) / 2;
    return getStressCategoryRaw(unit->status.current_soul->personality.stress);
}

int Units::getStressCategoryRaw(int32_t stress_level) {
    int max_level = int(stress_cutoffs.size()) - 1;
    int level = max_level;
    for (int i = max_level; i >= 0; i--)
        if (stress_level >= stress_cutoffs[i])
            level = i;
    return level;
}

static int32_t *getActionTimerPointer(df::unit_action *action) {
    switch (action->type)
    {
        case unit_action_type::None:
            break;
        case unit_action_type::Move:
            return &action->data.move.timer;
        case unit_action_type::Attack:
            if (action->data.attack.timer1 != 0) // Wind-up timer is still active, work on it
                return &action->data.attack.timer1;
            else // Wind-up timer is finished, work on recovery timer
                return &action->data.attack.timer2;
        case unit_action_type::HoldTerrain:
            return &action->data.holdterrain.timer;
        case unit_action_type::Climb:
            return &action->data.climb.timer;
        case unit_action_type::Job:
            return &action->data.job.timer;
        case unit_action_type::Talk:
            return &action->data.talk.timer;
        case unit_action_type::Unsteady:
            return &action->data.unsteady.timer;
        case unit_action_type::Dodge:
            return &action->data.dodge.timer;
        case unit_action_type::Recover:
            return &action->data.recover.timer;
        case unit_action_type::StandUp:
            return &action->data.standup.timer;
        case unit_action_type::LieDown:
            return &action->data.liedown.timer;
        case unit_action_type::JobRecover:
            return &action->data.jobrecover.timer;
        case unit_action_type::PushObject:
            return &action->data.pushobject.timer;
        case unit_action_type::SuckBlood:
            return &action->data.suckblood.timer;
        case unit_action_type::Mount:
            return &action->data.mount.timer;
        case unit_action_type::Dismount:
            return &action->data.dismount.timer;
        case unit_action_type::HoldItem:
            return &action->data.holditem.timer;
        case unit_action_type::LeadAnimal:
        case unit_action_type::StopLeadAnimal:
        case unit_action_type::Jump:
        case unit_action_type::ReleaseTerrain:
        case unit_action_type::Parry:
        case unit_action_type::Block:
        case unit_action_type::ReleaseItem:
            break;
    }
    return nullptr;
}

static void mutateActionTimerCore(df::unit_action *action,
    std::function<double(double)> mutator)
{
    int32_t *timer = getActionTimerPointer(action);
    if (timer != nullptr && *timer > 0)
    {
        double value = *timer;
        value = mutator(value);
        if (value > INT32_MAX)
            value = INT32_MAX;
        else if (value < INT32_MIN)
            value = INT32_MIN;
        *timer = value;
    }
}

void Units::subtractActionTimers(color_ostream &out, df::unit *unit,
    int32_t amount, df::unit_action_type affectedActionType)
{
    CHECK_NULL_POINTER(unit);
    for (auto action : unit->actions)
        if (affectedActionType == action->type)
            mutateActionTimerCore(action, [=](double timerValue){ return max(timerValue - amount, 1.0); });
}

void Units::subtractGroupActionTimers(color_ostream &out, df::unit *unit,
    int32_t amount, df::unit_action_type_group affectedActionTypeGroup)
{
    CHECK_NULL_POINTER(unit);
    for (auto action : unit->actions) {
        auto list = ENUM_ATTR(unit_action_type, group, action->type);
        for (size_t i = 0; i < list.size; i++) {
            if (list.items[i] == affectedActionTypeGroup)
            {
                mutateActionTimerCore(action, [=](double timerValue){ return max(timerValue - amount, 1.0); });
                break;
            }
        }
    }
}

static bool validateMultiplyActionTimerAmount(color_ostream &out, float amount) {
    if (amount < 0) {
        out.printerr("Can't multiply action timer(s) by negative amount.\n");
        return false;
    }
    return true;
}

void Units::multiplyActionTimers(color_ostream &out, df::unit *unit,
    float amount, df::unit_action_type affectedActionType)
{
    CHECK_NULL_POINTER(unit);
    if (!validateMultiplyActionTimerAmount(out, amount))
        return;
    for (auto action : unit->actions)
        if (affectedActionType == action->type)
            mutateActionTimerCore(action, [=](double timerValue){ return max(timerValue * amount, 1.0); });
}

void Units::multiplyGroupActionTimers(color_ostream &out, df::unit *unit,
    float amount, df::unit_action_type_group affectedActionTypeGroup)
{
    CHECK_NULL_POINTER(unit);
    if (!validateMultiplyActionTimerAmount(out, amount))
        return;
    for (auto action : unit->actions) {
        auto list = ENUM_ATTR(unit_action_type, group, action->type);
        for (size_t i = 0; i < list.size; i++) {
            if (list.items[i] == affectedActionTypeGroup)
            {
                mutateActionTimerCore(action, [=](double timerValue){ return max(timerValue * amount, 1.0); });
                break;
            }
        }
    }
}

static bool validateSetActionTimerAmount(color_ostream &out, int32_t amount) {
    if (amount <= 0) {
        out.printerr("Can't set action timer(s) to non-positive amount.\n");
        return false;
    }
    return true;
}

void Units::setActionTimers(color_ostream &out, df::unit *unit,
    int32_t amount, df::unit_action_type affectedActionType)
{
    CHECK_NULL_POINTER(unit);
    if (!validateSetActionTimerAmount(out, amount))
        return;
    for (auto action : unit->actions)
        if (affectedActionType == action->type)
            mutateActionTimerCore(action, [=](double timerValue){ return amount; });
}

void Units::setGroupActionTimers(color_ostream &out, df::unit *unit,
    int32_t amount, df::unit_action_type_group affectedActionTypeGroup)
{
    CHECK_NULL_POINTER(unit);
    if (!validateSetActionTimerAmount(out, amount))
        return;
    for (auto action : unit->actions) {
        auto list = ENUM_ATTR(unit_action_type, group, action->type);
        for (size_t i = 0; i < list.size; i++) {
            if (list.items[i] == affectedActionTypeGroup)
            {
                mutateActionTimerCore(action, [=](double timerValue){ return amount; });
                break;
            }
        }
    }
}
