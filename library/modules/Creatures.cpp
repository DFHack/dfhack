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


#include "dfhack/VersionInfo.h"
#include "dfhack/Process.h"
#include "dfhack/Vector.h"
#include "dfhack/Error.h"
#include "dfhack/Types.h"

// we connect to those
#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Creatures.h"
#include "dfhack/modules/Translation.h"
#include "ModuleFactory.h"
#include <dfhack/Core.h>

using namespace DFHack;

struct Creatures::Private
{
    bool Inited;
    bool Started;

    uint32_t dwarf_race_index_addr;
    uint32_t dwarf_civ_id_addr;
    bool IdMapReady;
    std::map<int32_t, int32_t> IdMap;

    Process *owner;
    Translation * trans;
};

Module* DFHack::createCreatures()
{
    return new Creatures();
}

Creatures::Creatures()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->owner = c.p;
    VersionInfo * minfo = c.vinfo;
    d->Inited = false;
    d->Started = false;
    d->IdMapReady = false;
    d->trans = c.getTranslation();
    d->trans->InitReadNames(); // FIXME: throws on error

    OffsetGroup *OG_Creatures = minfo->getGroup("Creatures");

    creatures = 0;
    try
    {
        creatures = (vector <df_creature *> *) OG_Creatures->getAddress ("vector");
        d->dwarf_race_index_addr = OG_Creatures->getAddress("current_race");
        d->dwarf_civ_id_addr = OG_Creatures->getAddress("current_civ");
    }
    catch(Error::All&){};
    d->Inited = true;
}

Creatures::~Creatures()
{
    if(d->Started)
        Finish();
}

bool Creatures::Start( uint32_t &numcreatures )
{
    if(creatures)
    {
        d->Started = true;
        numcreatures =  creatures->size();
        d->IdMap.clear();
        d->IdMapReady = false;
        return true;
    }
    return false;
}

bool Creatures::Finish()
{
    d->Started = false;
    return true;
}

df_creature * Creatures::GetCreature (const int32_t index)
{
    if(!d->Started) return nullptr;

    // read pointer from vector at position
    if(index > creatures->size())
        return nullptr;
    return creatures->at(index);
}

// returns index of creature actually read or -1 if no creature can be found
int32_t Creatures::GetCreatureInBox (int32_t index, df_creature ** furball,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
    if (!d->Started)
        return -1;

    Process *p = d->owner;
    uint16_t coords[3];
    uint32_t size = creatures->size();
    while (uint32_t(index) < size)
    {
        // read pointer from vector at position
        df_creature * temp = creatures->at(index);
        if (temp->x >= x1 && temp->x < x2)
        {
            if (temp->y >= y1 && temp->y < y2)
            {
                if (temp->z >= z1 && temp->z < z2)
                {
                    *furball = temp;
                    return index;
                }
            }
        }
        index++;
    }
    *furball = nullptr;
    return -1;
}

void Creatures::CopyCreature(df_creature * source, t_creature & furball)
{
    if(!d->Started) return;
    // read pointer from vector at position
    furball.origin = source;

    //read creature from memory
    // name
    d->trans->readName(furball.name,&source->name);

    // basic stuff
    furball.id = source->id;
    furball.x = source->x;
    furball.y = source->y;
    furball.z = source->z;
    furball.race = source->race;
    furball.civ = source->civ;
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
    furball.happiness = source->happiness;
    // physical attributes
    memcpy(&furball.strength, source->physical, sizeof(source->physical));

    // mood stuff
    furball.mood = source->mood;
    furball.mood_skill = source->unk_2f8; // FIXME: really? More like currently used skill anyway.
    d->trans->readName(furball.artifact_name, &source->artifact_name);

    // labors
    memcpy(&furball.labors, &source->labors, sizeof(furball.labors));

    furball.birth_year = source->birth_year;
    furball.birth_time = source->birth_time;
    furball.pregnancy_timer = source->pregnancy_timer;
    // appearance
    furball.nbcolors = source->appearance.size();
    if(furball.nbcolors>MAX_COLORS)
        furball.nbcolors = MAX_COLORS;
    for(uint32_t i = 0; i < furball.nbcolors; i++)
    {
        furball.color[i] = source->appearance[i];
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
	/*
    furball.current_job.occupationPtr = p->readDWord (addr_cr + offs.current_job_offset);
    if(furball.current_job.occupationPtr)
    {
        furball.current_job.active = true;
        furball.current_job.jobType = p->readByte (furball.current_job.occupationPtr + offs.job_type_offset );
        furball.current_job.jobId = p->readWord (furball.current_job.occupationPtr + offs.job_id_offset);
    }
    else
    {
        furball.current_job.active = false;
    }
    */
	// no jobs for now...
    {
        furball.current_job.active = false;
    }
}
int32_t Creatures::FindIndexById(int32_t creature_id)
{
    if (!d->Started)
        return -1;

    if (!d->IdMapReady)
    {
        d->IdMap.clear();

        uint32_t size = creatures->size();
        for (uint32_t index = 0; index < size; index++)
        {
            df_creature * temp = creatures->at(index);
            int32_t id = temp->id;
            d->IdMap[id] = index;
        }
    }

    std::map<int32_t,int32_t>::iterator it;
    it = d->IdMap.find(creature_id);
    if(it == d->IdMap.end())
        return -1;
    else
        return it->second;
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
uint32_t Creatures::GetDwarfRaceIndex()
{
    if(!d->Inited) return 0;
    Process * p = d->owner;
    return p->readDWord(d->dwarf_race_index_addr);
}

int32_t Creatures::GetDwarfCivId()
{
    if(!d->Inited) return -1;
    Process * p = d->owner;
    return p->readDWord(d->dwarf_civ_id_addr);
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
bool Creatures::ReadInventoryByIdx(const uint32_t index, std::vector<t_item *> & item)
{
    if(!d->Started) return false;
    if(index >= creatures->size()) return false;
    df_creature * temp = creatures->at(index);
    return this->ReadInventoryByPtr(temp, item);
}

bool Creatures::ReadInventoryByPtr(const df_creature * temp, std::vector<t_item *> & items)
{
    if(!d->Started) return false;
    items = temp->inventory;
    return true;
}

bool Creatures::ReadOwnedItemsByIdx(const uint32_t index, std::vector<int32_t> & item)
{
    if(!d->Started ) return false;
    if(index >= creatures->size()) return false;
    df_creature * temp = creatures->at(index);
    return this->ReadOwnedItemsByPtr(temp, item);
}

bool Creatures::ReadOwnedItemsByPtr(const df_creature * temp, std::vector<int32_t> & items)
{
    unsigned int i;
    if(!d->Started) return false;
    items = temp->owned_items;
    return true;
}

bool Creatures::RemoveOwnedItemByIdx(const uint32_t index, int32_t id)
{
    if(!d->Started)
    {
        cerr << "!d->Started FAIL" << endl;
        return false;
    }
    df_creature * temp = creatures->at (index);
    return this->RemoveOwnedItemByPtr(temp, id);
}

bool Creatures::RemoveOwnedItemByPtr(df_creature * temp, int32_t id)
{
    if(!d->Started) return false;
    Process * p = d->owner;
    vector <int32_t> & vec = temp->owned_items;
    vec.erase(std::remove(vec.begin(), vec.end(), id), vec.end());
/*
    DfVector <int32_t> citem(temp + d->creatures.owned_items_offset);

    for (unsigned i = 0; i < citem.size(); i++) {
        if (citem[i] != id)
            continue;
        if (!citem.remove(i--))
            return false;
    }
*/
    return true;
}

void Creatures::CopyNameTo(df_creature * creature, df_name * target)
{
    d->trans->copyName(&creature->name, target);
}

