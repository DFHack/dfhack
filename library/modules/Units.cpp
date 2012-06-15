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


#include "Internal.h"

#include <stddef.h>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
using namespace std;

#include "VersionInfo.h"
#include "MemAccess.h"
#include "Error.h"
#include "Types.h"

// we connect to those
#include "modules/Units.h"
#include "modules/Items.h"
#include "modules/Materials.h"
#include "modules/Translation.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "MiscUtils.h"

#include "df/world.h"
#include "df/ui.h"
#include "df/job.h"
#include "df/unit_inventory_item.h"
#include "df/unit_soul.h"
#include "df/nemesis_record.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/entity_position.h"
#include "df/entity_position_assignment.h"
#include "df/histfig_entity_link_positionst.h"
#include "df/assumed_identity.h"
#include "df/burrow.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::ui;

bool Units::isValid()
{
    return (world->units.all.size() > 0);
}

int32_t Units::getNumCreatures()
{
    return world->units.all.size();
}

df::unit * Units::GetCreature (const int32_t index)
{
    if (!isValid()) return NULL;

    // read pointer from vector at position
    if(size_t(index) > world->units.all.size())
        return 0;
    return world->units.all[index];
}

// returns index of creature actually read or -1 if no creature can be found
int32_t Units::GetCreatureInBox (int32_t index, df::unit ** furball,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
    if (!isValid())
        return -1;

    size_t size = world->units.all.size();
    while (size_t(index) < size)
    {
        // read pointer from vector at position
        df::unit * temp = world->units.all[index];
        if (temp->pos.x >= x1 && temp->pos.x < x2)
        {
            if (temp->pos.y >= y1 && temp->pos.y < y2)
            {
                if (temp->pos.z >= z1 && temp->pos.z < z2)
                {
                    *furball = temp;
                    return index;
                }
            }
        }
        index++;
    }
    *furball = NULL;
    return -1;
}

void Units::CopyCreature(df::unit * source, t_unit & furball)
{
    if(!isValid()) return;
    // read pointer from vector at position
    furball.origin = source;

    //read creature from memory
    // name
    Translation::readName(furball.name, &source->name);

    // basic stuff
    furball.id = source->id;
    furball.x = source->pos.x;
    furball.y = source->pos.y;
    furball.z = source->pos.z;
    furball.race = source->race;
    furball.civ = source->civ_id;
    furball.sex = source->sex;
    furball.caste = source->caste;
    furball.flags1.whole = source->flags1.whole;
    furball.flags2.whole = source->flags2.whole;
    furball.flags3.whole = source->flags3.whole;
    // custom profession
    furball.custom_profession = source->custom_profession;
    // profession
    furball.profession = source->profession;
    // happiness
    furball.happiness = source->status.happiness;
    // physical attributes
    memcpy(&furball.strength, source->body.physical_attrs, sizeof(source->body.physical_attrs));

    // mood stuff
    furball.mood = source->mood;
    furball.mood_skill = source->job.mood_skill; // FIXME: really? More like currently used skill anyway.
    Translation::readName(furball.artifact_name, &source->status.artifact_name);

    // labors
    memcpy(&furball.labors, &source->status.labors, sizeof(furball.labors));

    furball.birth_year = source->relations.birth_year;
    furball.birth_time = source->relations.birth_time;
    furball.pregnancy_timer = source->relations.pregnancy_timer;
    // appearance
    furball.nbcolors = source->appearance.colors.size();
    if(furball.nbcolors>MAX_COLORS)
        furball.nbcolors = MAX_COLORS;
    for(uint32_t i = 0; i < furball.nbcolors; i++)
    {
        furball.color[i] = source->appearance.colors[i];
    }

    //likes. FIXME: where do they fit in now? The soul?
    /*
    DfVector <uint32_t> likes(d->p, temp + offs.creature_likes_offset);
    furball.numLikes = likes.getSize();
    for(uint32_t i = 0;i<furball.numLikes;i++)
    {
        uint32_t temp2 = *(uint32_t *) likes[i];
        p->read(temp2,sizeof(t_like),(uint8_t *) &furball.likes[i]);
    }
    */
    /*
    if(d->Ft_soul)
    {
        uint32_t soul = p->readDWord(addr_cr + offs.default_soul_offset);
        furball.has_default_soul = false;

        if(soul)
        {
            furball.has_default_soul = true;
            // get first soul's skills
            DfVector <uint32_t> skills(soul + offs.soul_skills_vector_offset);
            furball.defaultSoul.numSkills = skills.size();

            for (uint32_t i = 0; i < furball.defaultSoul.numSkills;i++)
            {
                uint32_t temp2 = skills[i];
                // a byte: this gives us 256 skills maximum.
                furball.defaultSoul.skills[i].id = p->readByte (temp2);
                furball.defaultSoul.skills[i].rating =
                    p->readByte (temp2 + offsetof(t_skill, rating));
                furball.defaultSoul.skills[i].experience =
                    p->readWord (temp2 + offsetof(t_skill, experience));
            }

            // mental attributes are part of the soul
            p->read(soul + offs.soul_mental_offset,
                sizeof(t_attrib) * NUM_CREATURE_MENTAL_ATTRIBUTES,
                (uint8_t *)&furball.defaultSoul.analytical_ability);

            // traits as well
            p->read(soul + offs.soul_traits_offset,
                sizeof (uint16_t) * NUM_CREATURE_TRAITS,
                (uint8_t *) &furball.defaultSoul.traits);
        }
    }
    */
    if(source->job.current_job == NULL)
    {
        furball.current_job.active = false;
    }
    else
    {
        furball.current_job.active = true;
        furball.current_job.jobType = source->job.current_job->job_type;
        furball.current_job.jobId = source->job.current_job->id;
    }
}

int32_t Units::FindIndexById(int32_t creature_id)
{
    return df::unit::binsearch_index(world->units.all, creature_id);
}
/*
bool Creatures::WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS])
{
    if(!d->Started || !d->Ft_advanced) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;

    p->write(temp + d->creatures.labors_offset, NUM_CREATURE_LABORS, labors);
    uint32_t pickup_equip;
    p->readDWord(temp + d->creatures.pickup_equipment_bit, pickup_equip);
    pickup_equip |= 1u;
    p->writeDWord(temp + d->creatures.pickup_equipment_bit, pickup_equip);
    return true;
}

bool Creatures::WriteHappiness(const uint32_t index, const uint32_t happinessValue)
{
    if(!d->Started || !d->Ft_advanced) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord (temp + d->creatures.happiness_offset, happinessValue);
    return true;
}

bool Creatures::WriteFlags(const uint32_t index,
                           const uint32_t flags1,
                           const uint32_t flags2)
{
    if(!d->Started || !d->Ft_basic) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord (temp + d->creatures.flags1_offset, flags1);
    p->writeDWord (temp + d->creatures.flags2_offset, flags2);
    return true;
}

bool Creatures::WriteFlags(const uint32_t index,
                           const uint32_t flags1,
                           const uint32_t flags2,
                           const uint32_t flags3)
{
    if(!d->Started || !d->Ft_basic) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord (temp + d->creatures.flags1_offset, flags1);
    p->writeDWord (temp + d->creatures.flags2_offset, flags2);
    p->writeDWord (temp + d->creatures.flags3_offset, flags3);
    return true;
}

bool Creatures::WriteSkills(const uint32_t index, const t_soul &soul)
{
    if(!d->Started || !d->Ft_soul) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    uint32_t souloff = p->readDWord(temp + d->creatures.default_soul_offset);

    if(!souloff)
    {
        return false;
    }

    DfVector<uint32_t> skills(souloff + d->creatures.soul_skills_vector_offset);

    for (uint32_t i=0; i<soul.numSkills; i++)
    {
        uint32_t temp2 = skills[i];
        p->writeByte(temp2 + offsetof(t_skill, rating), soul.skills[i].rating);
        p->writeWord(temp2 + offsetof(t_skill, experience), soul.skills[i].experience);
    }

    return true;
}

bool Creatures::WriteAttributes(const uint32_t index, const t_creature &creature)
{
    if(!d->Started || !d->Ft_advanced || !d->Ft_soul) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    uint32_t souloff = p->readDWord(temp + d->creatures.default_soul_offset);

    if(!souloff)
    {
        return false;
    }

    // physical attributes
    p->write(temp + d->creatures.physical_offset,
        sizeof(t_attrib) * NUM_CREATURE_PHYSICAL_ATTRIBUTES,
        (uint8_t *)&creature.strength);

    // mental attributes are part of the soul
    p->write(souloff + d->creatures.soul_mental_offset,
        sizeof(t_attrib) * NUM_CREATURE_MENTAL_ATTRIBUTES,
        (uint8_t *)&creature.defaultSoul.analytical_ability);

    return true;
}

bool Creatures::WriteSex(const uint32_t index, const uint8_t sex)
{
    if(!d->Started || !d->Ft_basic ) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeByte (temp + d->creatures.sex_offset, sex);

    return true;
}

bool Creatures::WriteTraits(const uint32_t index, const t_soul &soul)
{
    if(!d->Started || !d->Ft_soul) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    uint32_t souloff = p->readDWord(temp + d->creatures.default_soul_offset);

    if(!souloff)
    {
        return false;
    }

    p->write(souloff + d->creatures.soul_traits_offset,
            sizeof (uint16_t) * NUM_CREATURE_TRAITS,
            (uint8_t *) &soul.traits);

    return true;
}

bool Creatures::WriteMood(const uint32_t index, const uint16_t mood)
{
    if(!d->Started || !d->Ft_advanced) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeWord(temp + d->creatures.mood_offset, mood);
    return true;
}

bool Creatures::WriteMoodSkill(const uint32_t index, const uint16_t moodSkill)
{
    if(!d->Started || !d->Ft_advanced) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeWord(temp + d->creatures.mood_skill_offset, moodSkill);
    return true;
}

bool Creatures::WriteJob(const t_creature * furball, std::vector<t_material> const& mat)
{
    if(!d->Inited || !d->Ft_job_materials) return false;
    if(!furball->current_job.active) return false;

    unsigned int i;
    Process * p = d->owner;
    Private::t_offsets & off = d->creatures;
    DfVector <uint32_t> cmats(furball->current_job.occupationPtr + off.job_materials_vector);

    for(i=0;i<cmats.size();i++)
    {
        p->writeWord(cmats[i] + off.job_material_itemtype_o, mat[i].itemType);
        p->writeWord(cmats[i] + off.job_material_subtype_o, mat[i].subType);
        p->writeWord(cmats[i] + off.job_material_subindex_o, mat[i].subIndex);
        p->writeDWord(cmats[i] + off.job_material_index_o, mat[i].index);
        p->writeDWord(cmats[i] + off.job_material_flags_o, mat[i].flags);
    }
    return true;
}

bool Creatures::WritePos(const uint32_t index, const t_creature &creature)
{
    if(!d->Started) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->write (temp + d->creatures.pos_offset, 3 * sizeof (uint16_t), (uint8_t *) & (creature.x));
    return true;
}

bool Creatures::WriteCiv(const uint32_t index, const int32_t civ)
{
    if(!d->Started) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord(temp + d->creatures.civ_offset, civ);
    return true;
}

bool Creatures::WritePregnancy(const uint32_t index, const uint32_t pregTimer)
{
    if(!d->Started) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord(temp + d->creatures.pregnancy_offset, pregTimer);
    return true;
}
*/
uint32_t Units::GetDwarfRaceIndex()
{
    return ui->race_id;
}

int32_t Units::GetDwarfCivId()
{
    return ui->civ_id;
}
/*
bool Creatures::getCurrentCursorCreature(uint32_t & creature_index)
{
    if(!d->cursorWindowInited) return false;
    Process * p = d->owner;
    creature_index = p->readDWord(d->current_cursor_creature_offset);
    return true;
}
*/
/*
bool Creatures::ReadJob(const t_creature * furball, vector<t_material> & mat)
{
    unsigned int i;
    if(!d->Inited || !d->Ft_job_materials) return false;
    if(!furball->current_job.active) return false;

    Process * p = d->owner;
    Private::t_offsets & off = d->creatures;
    DfVector <uint32_t> cmats(furball->current_job.occupationPtr + off.job_materials_vector);
    mat.resize(cmats.size());
    for(i=0;i<cmats.size();i++)
    {
        mat[i].itemType = p->readWord(cmats[i] + off.job_material_itemtype_o);
        mat[i].subType = p->readWord(cmats[i] + off.job_material_subtype_o);
        mat[i].subIndex = p->readWord(cmats[i] + off.job_material_subindex_o);
        mat[i].index = p->readDWord(cmats[i] + off.job_material_index_o);
        mat[i].flags = p->readDWord(cmats[i] + off.job_material_flags_o);
    }
    return true;
}
*/
bool Units::ReadInventoryByIdx(const uint32_t index, std::vector<df::item *> & item)
{
    if(index >= world->units.all.size()) return false;
    df::unit * temp = world->units.all[index];
    return ReadInventoryByPtr(temp, item);
}

bool Units::ReadInventoryByPtr(const df::unit * unit, std::vector<df::item *> & items)
{
    if(!isValid()) return false;
    if(!unit) return false;
    items.clear();
    for (size_t i = 0; i < unit->inventory.size(); i++)
        items.push_back(unit->inventory[i]->item);
    return true;
}

void Units::CopyNameTo(df::unit * creature, df::language_name * target)
{
    Translation::copyName(&creature->name, target);
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

df::item *Units::getContainer(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    for (size_t i = 0; i < unit->refs.size(); i++)
    {
        df::general_ref *ref = unit->refs[i];
        if (ref->getType() == general_ref_type::CONTAINED_IN_ITEM)
            return ref->getItem();
    }

    return NULL;
}

static df::assumed_identity *getFigureIdentity(df::historical_figure *figure)
{
    if (figure && figure->info && figure->info->reputation)
        return df::assumed_identity::find(figure->info->reputation->cur_identity);

    return NULL;
}

df::assumed_identity *Units::getIdentity(df::unit *unit)
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
            auto id_hfig = df::historical_figure::find(identity->histfig_id);

            if (id_hfig)
            {
                // Even DF doesn't do this bit, because it's apparently
                // only used for demons masquerading as gods, so you
                // can't ever change their nickname in-game.
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

    if (auto identity = getIdentity(unit))
    {
        auto id_hfig = df::historical_figure::find(identity->histfig_id);

        if (id_hfig)
            return &id_hfig->name;

        return &identity->name;
    }

    return &unit->name;
}

df::nemesis_record *Units::getNemesis(df::unit *unit)
{
    if (!unit)
        return NULL;

    for (unsigned i = 0; i < unit->refs.size(); i++)
    {
        df::nemesis_record *rv = unit->refs[i]->getNemesis();
        if (rv && rv->unit == unit)
            return rv;
    }

    return NULL;
}

static bool casteFlagSet(int race, int caste, df::caste_raw_flags flag)
{
    auto creature = df::creature_raw::find(race);
    if (!creature)
        return false;

    auto craw = vector_get(creature->caste, caste);
    if (!craw)
        return false;

    return craw->flags.is_set(flag);
}

static bool isCrazed(df::unit *unit)
{
    if (unit->flags3.bits.scuttle)
        return false;
    if (unit->curse.rem_tags1.bits.CRAZED)
        return false;
    if (unit->curse.add_tags1.bits.CRAZED)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::CRAZED);
}

static bool isOpposedToLife(df::unit *unit)
{
    if (unit->curse.rem_tags1.bits.OPPOSED_TO_LIFE)
        return false;
    if (unit->curse.add_tags1.bits.OPPOSED_TO_LIFE)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::CANNOT_UNDEAD);
}

bool DFHack::Units::isDead(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->flags1.bits.dead ||
           unit->flags3.bits.ghostly;
}

bool DFHack::Units::isAlive(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return !unit->flags1.bits.dead &&
           !unit->flags3.bits.ghostly &&
           !unit->curse.add_tags1.bits.NOT_LIVING;
}

bool DFHack::Units::isSane(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    if (unit->flags1.bits.dead ||
        unit->flags3.bits.ghostly ||
        isOpposedToLife(unit) ||
        unit->unknown8.unk2)
        return false;

    if (unit->unknown8.normal_race == unit->unknown8.were_race && isCrazed(unit))
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

bool DFHack::Units::isCitizen(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    // Copied from the conditions used to decide game over,
    // except that the game appears to let melancholy/raving
    // dwarves count as citizens.

    if (!isDwarf(unit) || !isSane(unit))
        return false;

    if (unit->flags1.bits.marauder ||
        unit->flags1.bits.invader_origin ||
        unit->flags1.bits.active_invader ||
        unit->flags1.bits.forest ||
        unit->flags1.bits.merchant ||
        unit->flags1.bits.diplomat)
        return false;

    if (unit->flags1.bits.tame)
        return true;

    return unit->civ_id == ui->civ_id &&
           unit->civ_id != -1 &&
           !unit->flags2.bits.underworld &&
           !unit->flags2.bits.resident &&
           !unit->flags2.bits.visitor_uninvited &&
           !unit->flags2.bits.visitor;
}

bool DFHack::Units::isDwarf(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->race == ui->race_id ||
           unit->unknown8.normal_race == ui->race_id;
}

double DFHack::Units::getAge(df::unit *unit, bool true_age)
{
    using df::global::cur_year;
    using df::global::cur_year_tick;

    CHECK_NULL_POINTER(unit);

    if (!cur_year || !cur_year_tick)
        return -1;

    double year_ticks = 403200.0;
    double birth_time = unit->relations.birth_year + unit->relations.birth_time/year_ticks;
    double cur_time = *cur_year + *cur_year_tick / year_ticks;

    if (!true_age && unit->relations.curse_year >= 0)
    {
        if (auto identity = getIdentity(unit))
        {
            if (identity->histfig_id < 0)
                birth_time = identity->birth_year + identity->birth_second/year_ticks;
        }
    }

    return cur_time - birth_time;
}

static bool noble_pos_compare(const Units::NoblePosition &a, const Units::NoblePosition &b)
{
    if (a.position->precedence < b.position->precedence)
        return true;
    if (a.position->precedence > b.position->precedence)
        return false;
    return a.position->id < b.position->id;
}

bool DFHack::Units::getNoblePositions(std::vector<NoblePosition> *pvec, df::unit *unit)
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

std::string DFHack::Units::getProfessionName(df::unit *unit, bool ignore_noble, bool plural)
{
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

std::string DFHack::Units::getCasteProfessionName(int race, int casteid, df::profession pid, bool plural)
{
    std::string prof, race_prefix;

    if (pid < 0 || !is_valid_enum_item(pid))
        return "";

    bool use_race_prefix = (race >= 0 && race != df::global::ui->race_id);

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
