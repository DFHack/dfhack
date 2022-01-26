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


#include "Internal.h"

#include <stddef.h>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
#include <numeric>
using namespace std;

#include "VersionInfo.h"
#include "MemAccess.h"
#include "Error.h"
#include "Types.h"

// we connect to those
#include "modules/Units.h"
#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Translation.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "MiscUtils.h"

#include "df/activity_entry.h"
#include "df/burrow.h"
#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/curse_attr_change.h"
#include "df/entity_position.h"
#include "df/entity_position_assignment.h"
#include "df/entity_raw.h"
#include "df/entity_raw_flags.h"
#include "df/identity_type.h"
#include "df/game_mode.h"
#include "df/histfig_entity_link_positionst.h"
#include "df/histfig_relationship_type.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/historical_kills.h"
#include "df/history_event_hist_figure_diedst.h"
#include "df/identity.h"
#include "df/job.h"
#include "df/nemesis_record.h"
#include "df/squad.h"
#include "df/tile_occupancy.h"
#include "df/ui.h"
#include "df/unit_inventory_item.h"
#include "df/unit_misc_trait.h"
#include "df/unit_relationship_type.h"
#include "df/unit_skill.h"
#include "df/unit_soul.h"
#include "df/unit_wound.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::ui;
using df::global::gamemode;

int32_t Units::getNumUnits()
{
    return world->units.all.size();
}

df::unit *Units::getUnit (const int32_t index)
{
    return vector_get(world->units.all, index);
}

// returns index of creature actually read or -1 if no creature can be found
bool Units::getUnitsInBox (std::vector<df::unit*> &units,
    int16_t x1, int16_t y1, int16_t z1,
    int16_t x2, int16_t y2, int16_t z2)
{
    if (!world)
        return false;

    if (x1 > x2) swap(x1, x2);
    if (y1 > y2) swap(y1, y2);
    if (z1 > z2) swap(z1, z2);

    units.clear();
    for (df::unit *u : world->units.all)
    {
        if (u->pos.x >= x1 && u->pos.x <= x2)
        {
            if (u->pos.y >= y1 && u->pos.y <= y2)
            {
                if (u->pos.z >= z1 && u->pos.z <= z2)
                {
                    units.push_back(u);
                }
            }
        }
    }
    return true;
}

int32_t Units::findIndexById(int32_t creature_id)
{
    return df::unit::binsearch_index(world->units.all, creature_id);
}

df::coord Units::getPosition(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    if (unit->flags1.bits.caged)
    {
        auto cage = getContainer(unit);
        if (cage)
            return Items::getPosition(cage);
    }

    return unit->pos;
}

bool Units::teleport(df::unit *unit, df::coord target_pos)
{
    // make sure source and dest map blocks are valid
    auto old_occ = Maps::getTileOccupancy(unit->pos);
    auto new_occ = Maps::getTileOccupancy(target_pos);
    if (!old_occ || !new_occ)
        return false;

    // clear appropriate occupancy flags at old tile
    if (unit->flags1.bits.on_ground)
        // this is potentially wrong, but the game will recompute this as needed
        old_occ->bits.unit_grounded = 0;
    else
        old_occ->bits.unit = 0;

    // if there's already somebody standing at the destination, then force the
    // unit to lay down
    if (new_occ->bits.unit)
        unit->flags1.bits.on_ground = 1;

    // set appropriate occupancy flags at new tile
    if (unit->flags1.bits.on_ground)
        new_occ->bits.unit_grounded = 1;
    else
        new_occ->bits.unit = 1;

    // move unit to destination
    unit->pos = target_pos;

    // move unit's riders (including babies) to destination
    if (unit->flags1.bits.ridden)
    {
        for (size_t j = 0; j < world->units.other[units_other_id::ANY_RIDER].size(); j++)
        {
            df::unit *rider = world->units.other[units_other_id::ANY_RIDER][j];
            if (rider->relationship_ids[df::unit_relationship_type::RiderMount] == unit->id)
                rider->pos = unit->pos;
        }
    }

    return true;
}

df::general_ref *Units::getGeneralRef(df::unit *unit, df::general_ref_type type)
{
    CHECK_NULL_POINTER(unit);

    return findRef(unit->general_refs, type);
}

df::specific_ref *Units::getSpecificRef(df::unit *unit, df::specific_ref_type type)
{
    CHECK_NULL_POINTER(unit);

    return findRef(unit->specific_refs, type);
}

df::item *Units::getContainer(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return findItemRef(unit->general_refs, general_ref_type::CONTAINED_IN_ITEM);
}

static df::identity *getFigureIdentity(df::historical_figure *figure)
{
    if (figure && figure->info && figure->info->reputation)
        return df::identity::find(figure->info->reputation->cur_identity);

    return NULL;
}

df::identity *Units::getIdentity(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    df::historical_figure *figure = df::historical_figure::find(unit->hist_figure_id);

    return getFigureIdentity(figure);
}

void Units::setNickname(df::unit *unit, std::string nick)
{
    CHECK_NULL_POINTER(unit);

    // There are >=3 copies of the name, and the one
    // in the unit is not the authoritative one.
    // This is the reason why military units often
    // lose nicknames set from Dwarf Therapist.
    Translation::setNickname(&unit->name, nick);

    if (unit->status.current_soul)
        Translation::setNickname(&unit->status.current_soul->name, nick);

    df::historical_figure *figure = df::historical_figure::find(unit->hist_figure_id);
    if (figure)
    {
        Translation::setNickname(&figure->name, nick);

        if (auto identity = getFigureIdentity(figure))
        {
            df::historical_figure *id_hfig = NULL;

            switch (identity->type) {
            case df::identity_type::None:
            case df::identity_type::HidingCurse:
            case df::identity_type::FalseIdentity:
            case df::identity_type::InfiltrationIdentity:
            case df::identity_type::Identity:
                break;  //  We want the nickname to end up in the identity

            case df::identity_type::Impersonating:
            case df::identity_type::TrueName:
                id_hfig = df::historical_figure::find(identity->histfig_id);
                break;
            }

            if (id_hfig)
            {
                Translation::setNickname(&id_hfig->name, nick);
            }
            else
                Translation::setNickname(&identity->name, nick);
        }
    }
}

df::language_name *Units::getVisibleName(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    // as of 0.44.11, identity names take precedence over associated histfig names
    if (auto identity = getIdentity(unit))
        return &identity->name;

    return &unit->name;
}

df::nemesis_record *Units::getNemesis(df::unit *unit)
{
    if (!unit)
        return NULL;

    for (unsigned i = 0; i < unit->general_refs.size(); i++)
    {
        df::nemesis_record *rv = unit->general_refs[i]->getNemesis();
        if (rv && rv->unit == unit)
            return rv;
    }

    return NULL;
}


bool Units::isHidingCurse(df::unit *unit)
{
    if (!unit->job.hunt_target)
    {
        auto identity = Units::getIdentity(unit);
        if (identity && identity->type == df::identity_type::HidingCurse)
            return true;
    }

    return false;
}

int Units::getPhysicalAttrValue(df::unit *unit, df::physical_attribute_type attr)
{
    auto &aobj = unit->body.physical_attrs[attr];
    int value = std::max(0, aobj.value - aobj.soft_demotion);

    if (auto mod = unit->curse.attr_change)
    {
        int mvalue = (value * mod->phys_att_perc[attr] / 100) + mod->phys_att_add[attr];

        if (isHidingCurse(unit))
            value = std::min(value, mvalue);
        else
            value = mvalue;
    }

    return std::max(0, value);
}

int Units::getMentalAttrValue(df::unit *unit, df::mental_attribute_type attr)
{
    auto soul = unit->status.current_soul;
    if (!soul) return 0;

    auto &aobj = soul->mental_attrs[attr];
    int value = std::max(0, aobj.value - aobj.soft_demotion);

    if (auto mod = unit->curse.attr_change)
    {
        int mvalue = (value * mod->ment_att_perc[attr] / 100) + mod->ment_att_add[attr];

        if (isHidingCurse(unit))
            value = std::min(value, mvalue);
        else
            value = mvalue;
    }

    return std::max(0, value);
}

bool Units::casteFlagSet(int race, int caste, df::caste_raw_flags flag)
{
    auto creature = df::creature_raw::find(race);
    if (!creature)
        return false;

    auto craw = vector_get(creature->caste, caste);
    if (!craw)
        return false;

    return craw->flags.is_set(flag);
}

bool Units::isCrazed(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->flags3.bits.scuttle)
        return false;
    if (unit->curse.rem_tags1.bits.CRAZED)
        return false;
    if (unit->curse.add_tags1.bits.CRAZED)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::CRAZED);
}

bool Units::isOpposedToLife(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->curse.rem_tags1.bits.OPPOSED_TO_LIFE)
        return false;
    if (unit->curse.add_tags1.bits.OPPOSED_TO_LIFE)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::OPPOSED_TO_LIFE);
}

bool Units::hasExtravision(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->curse.rem_tags1.bits.EXTRAVISION)
        return false;
    if (unit->curse.add_tags1.bits.EXTRAVISION)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::EXTRAVISION);
}

bool Units::isBloodsucker(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->curse.rem_tags1.bits.BLOODSUCKER)
        return false;
    if (unit->curse.add_tags1.bits.BLOODSUCKER)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::BLOODSUCKER);
}

bool Units::isMischievous(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->curse.rem_tags1.bits.MISCHIEVOUS)
        return false;
    if (unit->curse.add_tags1.bits.MISCHIEVOUS)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::MISCHIEVOUS);
}

df::unit_misc_trait *Units::getMiscTrait(df::unit *unit, df::misc_trait_type type, bool create)
{
    CHECK_NULL_POINTER(unit);

    auto &vec = unit->status.misc_traits;
    for (size_t i = 0; i < vec.size(); i++)
        if (vec[i]->id == type)
            return vec[i];

    if (create)
    {
        auto obj = new df::unit_misc_trait();
        obj->id = type;
        vec.push_back(obj);
        return obj;
    }

    return NULL;
}

bool Units::isDead(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->flags2.bits.killed ||
           unit->flags3.bits.ghostly;
}

bool Units::isAlive(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return !unit->flags2.bits.killed &&
           !unit->flags3.bits.ghostly &&
           !unit->curse.add_tags1.bits.NOT_LIVING;
}

bool Units::isSane(df::unit *unit)
{
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

bool Units::isCitizen(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    // Copied from the conditions used to decide game over,
    // except that the game appears to let melancholy/raving
    // dwarves count as citizens.

    if (unit->flags1.bits.marauder ||
        unit->flags1.bits.invader_origin ||
        unit->flags1.bits.active_invader ||
        unit->flags1.bits.forest ||
        unit->flags1.bits.merchant ||
        unit->flags1.bits.diplomat ||
        unit->flags2.bits.visitor ||
        unit->flags2.bits.visitor_uninvited ||
        unit->flags2.bits.underworld ||
        unit->flags2.bits.resident)
        return false;

    if (!isSane(unit))
        return false;

    return isOwnGroup(unit);
}

bool Units::isDwarf(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->race == ui->race_id ||
           unit->enemy.normal_race == ui->race_id;
}

// check for profession "war creature"
bool Units::isWar(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return unit->profession  == df::profession::TRAINED_WAR
            || unit->profession2 == df::profession::TRAINED_WAR;
}

// check for profession "hunting creature"
bool Units::isHunter(df::unit* unit)
{
    CHECK_NULL_POINTER(unit)
    return unit->profession  == df::profession::TRAINED_HUNTER
            || unit->profession2 == df::profession::TRAINED_HUNTER;
}

// check if unit is marked as available for adoption
bool Units::isAvailableForAdoption(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    auto refs = unit->specific_refs;
    for(size_t i=0; i<refs.size(); i++)
    {
        auto ref = refs[i];
        auto reftype = ref->type;
        if( reftype == df::specific_ref_type::PETINFO_PET )
        {
            //df::pet_info* pet = ref->pet;
            return true;
        }
    }

    return false;
}

// check if creature belongs to the player's civilization
// (don't try to pasture/slaughter random untame animals)
bool Units::isOwnCiv(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return unit->civ_id == ui->civ_id;
}

// check if creature belongs to the player's group
bool Units::isOwnGroup(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    auto histfig = df::historical_figure::find(unit->hist_figure_id);
    if (!histfig)
        return false;
    for (size_t i = 0; i < histfig->entity_links.size(); i++)
    {
        auto link = histfig->entity_links[i];
        if (link->entity_id == ui->group_id && link->getType() == df::histfig_entity_link_type::MEMBER)
            return true;
    }
    return false;
}

// check if creature belongs to the player's race
// (in combination with check for civ helps to filter out own dwarves)
bool Units::isOwnRace(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return unit->race == ui->race_id;
}

bool Units::isVisible(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return Maps::isTileVisible(unit->pos);
}

// get race name by id or unit pointer
string Units::getRaceNameById(int32_t id)
{
    df::creature_raw *raw = world->raws.creatures.all[id];
    if (raw)
        return raw->creature_id;
    return "";
}
string Units::getRaceName(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return getRaceNameById(unit->race);
}

void df_unit_get_physical_description(df::unit* unit, string* out_str)
{
    static auto* const fn =
        reinterpret_cast<void(THISCALL *)(df::unit*, string*)>(
            Core::getInstance().vinfo->getAddress("unit_get_physical_description"));
    if (fn)
        fn(unit, out_str);
    else
        *out_str = "";
}

string Units::getPhysicalDescription(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    string str;
    df_unit_get_physical_description(unit, &str);
    return str;
}

// get plural of race name (used for display in autobutcher UI and for sorting the watchlist)
string Units::getRaceNamePluralById(int32_t id)
{
    df::creature_raw *raw = world->raws.creatures.all[id];
    if (raw)
        return raw->name[1]; // second field is plural of race name
    return "";
}

string Units::getRaceNamePlural(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return getRaceNamePluralById(unit->race);
}

string Units::getRaceBabyNameById(int32_t id)
{
    df::creature_raw *raw = world->raws.creatures.all[id];
    if (raw)
        return raw->general_baby_name[0];
    return "";
}

string Units::getRaceBabyName(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return getRaceBabyNameById(unit->race);
}

string Units::getRaceChildNameById(int32_t id)
{
    df::creature_raw *raw = world->raws.creatures.all[id];
    if (raw)
        return raw->general_child_name[0];
    return "";
}

string Units::getRaceChildName(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return getRaceChildNameById(unit->race);
}

bool Units::isBaby(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return unit->profession == df::profession::BABY;
}

bool Units::isChild(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return unit->profession == df::profession::CHILD;
}

bool Units::isAdult(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return !isBaby(unit) && !isChild(unit);
}

bool Units::isEggLayer(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    df::creature_raw *raw = world->raws.creatures.all[unit->race];
    df::caste_raw *caste = raw->caste.at(unit->caste);
    return caste->flags.is_set(caste_raw_flags::LAYS_EGGS)
        || caste->flags.is_set(caste_raw_flags::LAYS_UNUSUAL_EGGS);
}

bool Units::isGrazer(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    df::creature_raw *raw = world->raws.creatures.all[unit->race];
    df::caste_raw *caste = raw->caste.at(unit->caste);
    return caste->flags.is_set(caste_raw_flags::GRAZER);
}

bool Units::isMilkable(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    df::creature_raw *raw = world->raws.creatures.all[unit->race];
    df::caste_raw *caste = raw->caste.at(unit->caste);
    return caste->flags.is_set(caste_raw_flags::MILKABLE);
}

bool Units::isTrainableWar(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    df::creature_raw *raw = world->raws.creatures.all[unit->race];
    df::caste_raw *caste = raw->caste.at(unit->caste);
    return caste->flags.is_set(caste_raw_flags::TRAINABLE_WAR);
}

bool Units::isTrainableHunting(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    df::creature_raw *raw = world->raws.creatures.all[unit->race];
    df::caste_raw *caste = raw->caste.at(unit->caste);
    return caste->flags.is_set(caste_raw_flags::TRAINABLE_HUNTING);
}

bool Units::isTamable(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    df::creature_raw *raw = world->raws.creatures.all[unit->race];
    df::caste_raw *caste = raw->caste.at(unit->caste);
    return caste->flags.is_set(caste_raw_flags::PET)
        || caste->flags.is_set(caste_raw_flags::PET_EXOTIC);
}

bool Units::isMale(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return unit->sex == 1;
}

bool Units::isFemale(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return unit->sex == 0;
}


double Units::getAge(df::unit *unit, bool true_age)
{
    using df::global::cur_year;
    using df::global::cur_year_tick;

    CHECK_NULL_POINTER(unit);

    if (!cur_year || !cur_year_tick)
        return -1;

    double year_ticks = 403200.0;
    double birth_time = unit->birth_year + unit->birth_time/year_ticks;
    double cur_time = *cur_year + *cur_year_tick / year_ticks;

    if (!true_age) {
        if (auto identity = getIdentity(unit)) {
            if (identity->birth_year != -1) {
                birth_time = identity->birth_year + identity->birth_second / year_ticks;
            }
        }
    }

    return cur_time - birth_time;
}

int Units::getKillCount(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    auto histfig = df::historical_figure::find(unit->hist_figure_id);
    int count = 0;
    if (histfig && histfig->info->kills)
    {
        auto kills = histfig->info->kills;
        count += std::accumulate(kills->killed_count.begin(), kills->killed_count.end(), 0);
        for (auto it = kills->events.begin(); it != kills->events.end(); ++it)
        {
            if (virtual_cast<df::history_event_hist_figure_diedst>(df::history_event::find(*it)))
                ++count;
        }
    }
    return count;
}

inline void adjust_skill_rating(int &rating, bool is_adventure, int value, int dwarf3_4, int dwarf1_2, int adv9_10, int adv3_4, int adv1_2)
{
    if  (is_adventure)
    {
        if (value >= adv1_2) rating >>= 1;
        else if (value >= adv3_4) rating = rating*3/4;
        else if (value >= adv9_10) rating = rating*9/10;
    }
    else
    {
        if (value >= dwarf1_2) rating >>= 1;
        else if (value >= dwarf3_4) rating = rating*3/4;
    }
}

int Units::getNominalSkill(df::unit *unit, df::job_skill skill_id, bool use_rust)
{
    CHECK_NULL_POINTER(unit);

    /*
     * This is 100% reverse-engineered from DF code.
     */

    if (!unit->status.current_soul)
        return 0;

    // Retrieve skill from unit soul:

    auto skill = binsearch_in_vector(unit->status.current_soul->skills, &df::unit_skill::id, skill_id);

    if (skill)
    {
        int rating = int(skill->rating);
        if (use_rust)
            rating -= skill->rusty;
        return std::max(0, rating);
    }

    return 0;
}

int Units::getExperience(df::unit *unit, df::job_skill skill_id, bool total)
{
    CHECK_NULL_POINTER(unit);

    if (!unit->status.current_soul)
        return 0;

    auto skill = binsearch_in_vector(unit->status.current_soul->skills, &df::unit_skill::id, skill_id);
    if (!skill)
        return 0;

    int xp = skill->experience;
    // exact formula used by the game:
    if (total && skill->rating > 0)
        xp += 500*skill->rating + 100*skill->rating*(skill->rating - 1)/2;
    return xp;
}

int Units::getEffectiveSkill(df::unit *unit, df::job_skill skill_id)
{
    /*
     * This is 100% reverse-engineered from DF code.
     */

    int rating = getNominalSkill(unit, skill_id, true);

    // Apply special states

    if (unit->counters.soldier_mood == df::unit::T_counters::None)
    {
        if (unit->counters.nausea > 0) rating >>= 1;
        if (unit->counters.winded > 0) rating >>= 1;
        if (unit->counters.stunned > 0) rating >>= 1;
        if (unit->counters.dizziness > 0) rating >>= 1;
        if (unit->counters2.fever > 0) rating >>= 1;
    }

    if (unit->counters.soldier_mood != df::unit::T_counters::MartialTrance)
    {
        if (!unit->flags3.bits.ghostly && !unit->flags3.bits.scuttle &&
            !unit->flags2.bits.vision_good && !unit->flags2.bits.vision_damaged &&
            !hasExtravision(unit))
        {
            rating >>= 2;
        }
        if (unit->counters.pain >= 100 && unit->mood == -1)
        {
            rating >>= 1;
        }
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

    // Hunger etc timers

    bool is_adventure = (gamemode && *gamemode == game_mode::ADVENTURE);

    if (!unit->flags3.bits.scuttle && isBloodsucker(unit))
    {
        using namespace df::enums::misc_trait_type;

        if (auto trait = getMiscTrait(unit, TimeSinceSuckedBlood))
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
        adjust_skill_rating(
            rating, is_adventure, unit->counters2.sleepiness_timer,
            150000, 150000, 172800, 259200, 345600
        );

    return rating;
}

bool Units::isValidLabor(df::unit *unit, df::unit_labor labor)
{
    CHECK_NULL_POINTER(unit);
    if (!is_valid_enum_item(labor))
        return false;
    if (labor == df::unit_labor::NONE)
        return false;
    df::historical_entity *entity = df::historical_entity::find(unit->civ_id);
    if (entity && entity->entity_raw && !entity->entity_raw->jobs.permitted_labor[labor])
        return false;
    return true;
}

bool Units::setLaborValidity(df::unit_labor labor, bool isValid)
{
    if (!is_valid_enum_item(labor))
        return false;
    if (labor == df::unit_labor::NONE)
        return false;
    df::historical_entity *entity = df::historical_entity::find(ui->civ_id);
    if (!entity || !entity->entity_raw)
        return false;
    entity->entity_raw->jobs.permitted_labor[labor] = isValid;
    return true;
}

inline void adjust_speed_rating(int &rating, bool is_adventure, int value, int dwarf100, int dwarf200, int adv50, int adv75, int adv100, int adv200)
{
    if  (is_adventure)
    {
        if (value >= adv200) rating += 200;
        else if (value >= adv100) rating += 100;
        else if (value >= adv75) rating += 75;
        else if (value >= adv50) rating += 50;
    }
    else
    {
        if (value >= dwarf200) rating += 200;
        else if (value >= dwarf100) rating += 100;
    }
}

static int calcInventoryWeight(df::unit *unit)
{
    int armor_skill = Units::getEffectiveSkill(unit, job_skill::ARMOR);
    int armor_mul = 15 - std::min(15, armor_skill);

    int inv_weight = 0, inv_weight_fraction = 0;

    for (size_t i = 0; i < unit->inventory.size(); i++)
    {
        auto item = unit->inventory[i]->item;
        if (!item->flags.bits.weight_computed)
            continue;

        int wval = item->weight;
        int wfval = item->weight_fraction;
        auto mode = unit->inventory[i]->mode;

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

    return inv_weight*100 + inv_weight_fraction/10000;
}

int Units::computeMovementSpeed(df::unit *unit)
{
    using namespace df::enums::physical_attribute_type;

    CHECK_NULL_POINTER(unit);

    /*
     * Pure reverse-engineered computation of unit _slowness_,
     * i.e. number of ticks to move * 100.
     */

    // Base speed

    auto creature = df::creature_raw::find(unit->race);
    if (!creature)
        return 0;

    auto craw = vector_get(creature->caste, unit->caste);
    if (!craw)
        return 0;

    int speed = craw->misc.speed;

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

    auto cur_liquid = unit->status2.liquid_type.bits.liquid_type;
    bool in_magma = (cur_liquid == tile_liquid::Magma);

    if (unit->flags2.bits.swimming)
    {
        speed = craw->misc.swim_speed;
        if (in_magma)
            speed *= 2;

        if (craw->flags.is_set(caste_raw_flags::CAN_SWIM))
        {
            int skill = Units::getEffectiveSkill(unit, job_skill::SWIMMING);

            // Originally a switch:
            if (skill > 1)
                speed = speed * std::max(6, 21-skill) / 20;
        }
    }
    else
    {
        int delta = 150*unit->status2.liquid_depth;
        if (in_magma)
            delta *= 2;
        speed += delta;
    }

    // General counters and flags

    if (isBaby(unit))
        speed += 3000;

    if (unit->flags3.bits.unk15)
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

    if (unit->flags2.bits.gutted) speed += 2000;

    if (unit->counters.soldier_mood == df::unit::T_counters::None)
    {
        if (unit->counters.nausea > 0) speed += 1000;
        if (unit->counters.winded > 0) speed += 1000;
        if (unit->counters.stunned > 0) speed += 1000;
        if (unit->counters.dizziness > 0) speed += 1000;
        if (unit->counters2.fever > 0) speed += 1000;
    }

    if (unit->counters.soldier_mood != df::unit::T_counters::MartialTrance)
    {
        if (unit->counters.pain >= 100 && unit->mood == -1)
            speed += 1000;
    }

    // Hunger etc timers

    bool is_adventure = (gamemode && *gamemode == game_mode::ADVENTURE);

    if (!unit->flags3.bits.scuttle && Units::isBloodsucker(unit))
    {
        using namespace df::enums::misc_trait_type;

        if (auto trait = Units::getMiscTrait(unit, TimeSinceSuckedBlood))
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

    if (unit->relationship_ids[df::unit_relationship_type::Draggee] != -1)
        speed += 1000;

    if (unit->flags1.bits.on_ground)
        speed += 2000;
    else if (unit->flags3.bits.on_crutch)
    {
        int skill = Units::getEffectiveSkill(unit, job_skill::CRUTCH_WALK);
        speed += 2000 - 100*std::min(20, skill);
    }

    if (unit->flags1.bits.hidden_in_ambush && !Units::isMischievous(unit))
    {
        int skill = Units::getEffectiveSkill(unit, job_skill::SNEAK);
        speed += 2000 - 100*std::min(20, skill);
    }

    if (unsigned(unit->counters2.paralysis-1) <= 98)
        speed += unit->counters2.paralysis*10;
    if (unsigned(unit->counters.webbed-1) <= 8)
        speed += unit->counters.webbed*100;

    // Muscle and fat weight vs expected size

    auto &s_info = unit->body.size_info;
    speed = std::max(speed*3/4, std::min(speed*3/2, int(int64_t(speed)*s_info.size_cur/s_info.size_base)));

    // Attributes

    int strength_attr = Units::getPhysicalAttrValue(unit, STRENGTH);
    int agility_attr = Units::getPhysicalAttrValue(unit, AGILITY);

    int total_attr = std::max(200, std::min(3800, strength_attr + agility_attr));
    speed = ((total_attr-200)*(speed/2) + (3800-total_attr)*(speed*3/2))/3600;

    // Stance

    if (!unit->flags1.bits.on_ground && unit->status2.limbs_stand_max > 2)
    {
        // WTF
        int as = unit->status2.limbs_stand_max;
        int x = (as-1) - (as>>1);
        int y = as - unit->status2.limbs_stand_count;
        if (unit->flags3.bits.on_crutch) y--;
        y = y * 500 / x;
        if (y > 0) speed += y;
    }

    // Mood

    if (unit->mood == mood_type::Melancholy) speed += 8000;

    // Inventory encumberance

    int total_weight = calcInventoryWeight(unit);
    int free_weight = std::max(1, s_info.size_cur/10 + strength_attr*3);

    if (free_weight < total_weight)
    {
        int delta = (total_weight - free_weight)/10 + 1;
        if (!is_adventure)
            delta = std::min(5000, delta);
        speed += delta;
    }

    // skipped: unknown loop on inventory items that amounts to 0 change

    if (is_adventure)
    {
        auto player = vector_get(world->units.active, 0);
        if (player && player->id == unit->relationship_ids[df::unit_relationship_type::GroupLeader])
            speed = std::min(speed, computeMovementSpeed(player));
    }

    return std::min(10000, std::max(0, speed));
}

static bool entityRawFlagSet(int civ_id, df::entity_raw_flags flag)
{
    auto entity = df::historical_entity::find(civ_id);

    return entity && entity->entity_raw && entity->entity_raw->flags.is_set(flag);
}

float Units::computeSlowdownFactor(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    /*
     * These slowdowns are actually done by skipping a move if random(x) != 0, so
     * it follows the geometric distribution. The mean expected slowdown is x.
     */

    float coeff = 1.0f;

    if (!unit->job.hunt_target && (!gamemode || *gamemode == game_mode::DWARF))
    {
        if (!unit->flags1.bits.marauder &&
            casteFlagSet(unit->race, unit->caste, caste_raw_flags::MEANDERER) &&
            !(unit->following && isCitizen(unit)) &&
            linear_index(unit->inventory, &df::unit_inventory_item::mode,
                         df::unit_inventory_item::Hauled) < 0)
        {
            coeff *= 4.0f;
        }

        if (unit->relationship_ids[df::unit_relationship_type::GroupLeader] < 0 &&
            unit->flags1.bits.active_invader &&
            !unit->job.current_job && !unit->flags3.bits.no_meandering &&
            unit->profession != profession::THIEF && unit->profession != profession::MASTER_THIEF &&
            !entityRawFlagSet(unit->civ_id, entity_raw_flags::ITEM_THIEF))
        {
            coeff *= 3.0f;
        }
    }

    if (unit->flags3.bits.floundering)
    {
        coeff *= 3.0f;
    }

    return coeff;
}


static bool noble_pos_compare(const Units::NoblePosition &a, const Units::NoblePosition &b)
{
    if (a.position->precedence < b.position->precedence)
        return true;
    if (a.position->precedence > b.position->precedence)
        return false;
    return a.position->id < b.position->id;
}

bool Units::getNoblePositions(std::vector<NoblePosition> *pvec, df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    pvec->clear();

    auto histfig = df::historical_figure::find(unit->hist_figure_id);
    if (!histfig)
        return false;

    for (size_t i = 0; i < histfig->entity_links.size(); i++)
    {
        auto link = histfig->entity_links[i];
        auto epos = strict_virtual_cast<df::histfig_entity_link_positionst>(link);
        if (!epos)
            continue;

        NoblePosition pos;

        pos.entity = df::historical_entity::find(epos->entity_id);
        if (!pos.entity)
            continue;

        pos.assignment = binsearch_in_vector(pos.entity->positions.assignments, epos->assignment_id);
        if (!pos.assignment)
            continue;

        pos.position = binsearch_in_vector(pos.entity->positions.own, pos.assignment->position_id);
        if (!pos.position)
            continue;

        pvec->push_back(pos);
    }

    if (pvec->empty())
        return false;

    std::sort(pvec->begin(), pvec->end(), noble_pos_compare);
    return true;
}

std::string Units::getProfessionName(df::unit *unit, bool ignore_noble, bool plural)
{
    CHECK_NULL_POINTER(unit);

    std::string prof = unit->custom_profession;
    if (!prof.empty())
        return prof;

    std::vector<NoblePosition> np;

    if (!ignore_noble && getNoblePositions(&np, unit))
    {
        switch (unit->sex)
        {
        case 0:
            prof = np[0].position->name_female[plural ? 1 : 0];
            break;
        case 1:
            prof = np[0].position->name_male[plural ? 1 : 0];
            break;
        default:
            break;
        }

        if (prof.empty())
            prof = np[0].position->name[plural ? 1 : 0];
        if (!prof.empty())
            return prof;
    }

    return getCasteProfessionName(unit->race, unit->caste, unit->profession, plural);
}

std::string Units::getCasteProfessionName(int race, int casteid, df::profession pid, bool plural)
{
    std::string prof, race_prefix;

    if (pid < (df::profession)0 || !is_valid_enum_item(pid))
        return "";
    int16_t current_race = df::global::ui->race_id;
    if (df::global::gamemode && *df::global::gamemode == df::game_mode::ADVENTURE)
        current_race = world->units.active[0]->race;
    bool use_race_prefix = (race >= 0 && race != current_race);

    if (auto creature = df::creature_raw::find(race))
    {
        if (auto caste = vector_get(creature->caste, casteid))
        {
            race_prefix = caste->caste_name[0];

            if (plural)
                prof = caste->caste_profession_name.plural[pid];
            else
                prof = caste->caste_profession_name.singular[pid];

            if (prof.empty())
            {
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

        if (prof.empty())
        {
            if (plural)
                prof = creature->profession_name.plural[pid];
            else
                prof = creature->profession_name.singular[pid];

            if (prof.empty())
            {
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

    if (prof.empty())
    {
        switch (pid)
        {
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

    if (use_race_prefix)
    {
        if (!prof.empty())
            race_prefix += " ";
        prof = race_prefix + prof;
    }

    return Translation::capitalize(prof, true);
}

int8_t Units::getProfessionColor(df::unit *unit, bool ignore_noble)
{
    CHECK_NULL_POINTER(unit);

    std::vector<NoblePosition> np;

    if (!ignore_noble && getNoblePositions(&np, unit))
    {
        if (np[0].position->flags.is_set(entity_position_flags::COLOR))
            return np[0].position->color[0] + np[0].position->color[2] * 8;
    }

    return getCasteProfessionColor(unit->race, unit->caste, unit->profession);
}

int8_t Units::getCasteProfessionColor(int race, int casteid, df::profession pid)
{
    // make sure it's an actual profession
    if (pid < 0 || !is_valid_enum_item(pid))
        return 3;

    // If it's not a Peasant, it's hardcoded
    if (pid != profession::STANDARD)
        return ENUM_ATTR(profession, color, pid);

    if (auto creature = df::creature_raw::find(race))
    {
        if (auto caste = vector_get(creature->caste, casteid))
        {
            if (caste->flags.is_set(caste_raw_flags::HAS_COLOR))
                return caste->caste_color[0] + caste->caste_color[2] * 8;
        }
        return creature->color[0] + creature->color[2] * 8;
    }

    // default to dwarven peasant color
    return 3;
}

df::goal_type Units::getGoalType(df::unit *unit, size_t goalIndex)
{
    CHECK_NULL_POINTER(unit);

    df::goal_type goal = df::goal_type::STAY_ALIVE;
    if (unit->status.current_soul
        && unit->status.current_soul->personality.dreams.size() > goalIndex)
    {
        goal = unit->status.current_soul->personality.dreams[goalIndex]->type;
    }
    return goal;
}

std::string Units::getGoalName(df::unit *unit, size_t goalIndex)
{
    CHECK_NULL_POINTER(unit);

    df::goal_type goal = getGoalType(unit, goalIndex);
    bool achieved_goal = isGoalAchieved(unit, goalIndex);

    std::string goal_name = achieved_goal ? ENUM_ATTR(goal_type, achieved_short_name, goal) : ENUM_ATTR(goal_type, short_name, goal);
    if (goal == df::goal_type::START_A_FAMILY) {
        std::string parent = ENUM_KEY_STR(histfig_relationship_type, histfig_relationship_type::Parent);
        size_t start_pos = goal_name.find(parent);
        if (start_pos != std::string::npos) {
            df::histfig_relationship_type parent_type = isFemale(unit) ? histfig_relationship_type::Mother : histfig_relationship_type::Father;
            goal_name.replace(start_pos, parent.length(), ENUM_KEY_STR(histfig_relationship_type, parent_type));
        }
    }
    return goal_name;
}

bool Units::isGoalAchieved(df::unit *unit, size_t goalIndex)
{
    CHECK_NULL_POINTER(unit);

    return unit->status.current_soul
        && unit->status.current_soul->personality.dreams.size() > goalIndex
        && unit->status.current_soul->personality.dreams[goalIndex]->flags.whole != 0;
}

std::string Units::getSquadName(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->military.squad_id == -1)
        return "";
    df::squad *squad = df::squad::find(unit->military.squad_id);
    if (!squad)
        return "";
    if (squad->alias.size() > 0)
        return squad->alias;
    return Translation::TranslateName(&squad->name, true);
}

df::activity_entry *Units::getMainSocialActivity(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->social_activities.empty())
        return nullptr;

    return df::activity_entry::find(unit->social_activities[unit->social_activities.size() - 1]);
}

df::activity_event *Units::getMainSocialEvent(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    df::activity_entry *entry = getMainSocialActivity(unit);
    if (!entry || entry->events.empty())
        return nullptr;
    return entry->events[entry->events.size() - 1];
}

bool Units::isMerchant(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->flags1.bits.merchant == 1;
}

bool Units::isDiplomat(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->flags1.bits.diplomat == 1;
}

bool Units::isForest(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return unit->flags1.bits.forest == 1;
}

bool Units::isMarkedForSlaughter(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    return unit->flags2.bits.slaughter == 1;
}

bool Units::isTame(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    bool tame = false;
    if(unit->flags1.bits.tame)
    {
        switch (unit->training_level)
        {
        case df::animal_training_level::SemiWild: //??
        case df::animal_training_level::Trained:
        case df::animal_training_level::WellTrained:
        case df::animal_training_level::SkilfullyTrained:
        case df::animal_training_level::ExpertlyTrained:
        case df::animal_training_level::ExceptionallyTrained:
        case df::animal_training_level::MasterfullyTrained:
        case df::animal_training_level::Domesticated:
            tame=true;
            break;
        case df::animal_training_level::Unk8:     //??
        case df::animal_training_level::WildUntamed:
        default:
            tame=false;
            break;
        }
    }
    return tame;
}

bool Units::isTrained(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    // case a: trained for war/hunting (those don't have a training level, strangely)
    if(Units::isWar(unit) || Units::isHunter(unit))
        return true;

    // case b: tamed and trained wild creature, gets a training level
    bool trained = false;
    switch (unit->training_level)
    {
    case df::animal_training_level::Trained:
    case df::animal_training_level::WellTrained:
    case df::animal_training_level::SkilfullyTrained:
    case df::animal_training_level::ExpertlyTrained:
    case df::animal_training_level::ExceptionallyTrained:
    case df::animal_training_level::MasterfullyTrained:
    //case df::animal_training_level::Domesticated:
        trained = true;
        break;
    default:
        break;
    }
    return trained;
}

bool Units::isGay(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    if (!unit->status.current_soul)
        return false;
    df::orientation_flags orientation = unit->status.current_soul->orientation_flags;
    return (!Units::isFemale(unit) || !(orientation.whole & (orientation.mask_marry_male | orientation.mask_romance_male)))
        && (!Units::isMale(unit) || !(orientation.whole & (orientation.mask_marry_female | orientation.mask_romance_female)));
}

bool Units::isNaked(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    // TODO(kazimuth): is this correct?
    return (unit->inventory.empty());
}

bool Units::isUndead(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    // ignore vampires, they should be treated like normal dwarves
    return (unit->flags3.bits.ghostly ||
            ( (unit->curse.add_tags1.bits.OPPOSED_TO_LIFE || unit->curse.add_tags1.bits.NOT_LIVING)
             && !unit->curse.add_tags1.bits.BLOODSUCKER ));
}

bool Units::isGhost(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->flags3.bits.ghostly;
}

bool Units::isActive(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return !unit->flags1.bits.inactive;
}

bool Units::isKilled(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->flags2.bits.killed;
}

bool Units::isGelded(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    auto wounds = unit->body.wounds;
    for(auto wound = wounds.begin(); wound != wounds.end(); ++wound)
    {
        auto parts = (*wound)->parts;
        for (auto part = parts.begin(); part != parts.end(); ++part)
        {
            if ((*part)->flags2.bits.gelded)
                return true;
        }
    }
    return false;
}

// check if creature is domesticated
// seems to be the only way to really tell if it's completely safe to autonestbox it (training can revert)
bool Units::isDomesticated(df::unit* unit)
{
    CHECK_NULL_POINTER(unit);
    bool tame = false;
    if(unit->flags1.bits.tame)
    {
        switch (unit->training_level)
        {
        case df::animal_training_level::Domesticated:
            tame=true;
            break;
        default:
            tame=false;
            break;
        }
    }
    return tame;
}

// 50000 and up is level 0, 25000 and up is level 1, etc.
const vector<int32_t> Units::stress_cutoffs {50000, 25000, 10000, -10000, -25000, -50000, -100000};

int Units::getStressCategory(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    if (!unit->status.current_soul)
        return int(stress_cutoffs.size()) / 2;

    return getStressCategoryRaw(unit->status.current_soul->personality.stress_level);
}

int Units::getStressCategoryRaw(int32_t stress_level)
{
    int max_level = int(stress_cutoffs.size()) - 1;
    int level = max_level;
    for (int i = max_level; i >= 0; i--)
    {
        if (stress_level >= stress_cutoffs[i])
        {
            level = i;
        }
    }
    return level;
}
