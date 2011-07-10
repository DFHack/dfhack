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

#include "Internal.h"

#include <stddef.h>
#include <string>
#include <vector>
#include <map>
#include <cstring>
using namespace std;

#include "ContextShared.h"

#include "dfhack/VersionInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFVector.h"
#include "dfhack/DFError.h"
#include "dfhack/DFTypes.h"

// we connect to those
#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Creatures.h"
#include "ModuleFactory.h"

using namespace DFHack;

struct Creatures::Private
{
    bool Inited;
    bool Started;
    bool Ft_basic;
    bool Ft_advanced;
    bool Ft_jobs;
    bool Ft_job_materials;
    bool Ft_soul;
    bool Ft_inventory;
    bool Ft_owned_items;
    struct t_offsets
    {
        // creature offsets
        uint32_t vector;
        uint32_t pos_offset;
        uint32_t profession_offset;
        uint32_t custom_profession_offset;
        uint32_t race_offset;
        int32_t civ_offset;
        uint32_t flags1_offset;
        uint32_t flags2_offset;
        uint32_t flags3_offset;
        uint32_t name_offset;
        uint32_t sex_offset;
        uint32_t caste_offset;
        uint32_t id_offset;
        uint32_t labors_offset;
        uint32_t happiness_offset;
        uint32_t artifact_name_offset;
        uint32_t physical_offset;
        uint32_t mood_offset;
        uint32_t mood_skill_offset;
        uint32_t pickup_equipment_bit;
        uint32_t soul_vector_offset;
        uint32_t default_soul_offset;
        uint32_t current_job_offset;
        // soul offsets
        uint32_t soul_skills_vector_offset;
        // name offsets (needed for reading creature names)
        uint32_t name_firstname_offset;
        uint32_t name_nickname_offset;
        uint32_t name_words_offset;
        uint32_t soul_mental_offset;
        uint32_t soul_traits_offset;
        uint32_t appearance_vector_offset;
        uint32_t birth_year_offset;
        uint32_t birth_time_offset;
        uint32_t inventory_offset;
        uint32_t owned_items_offset;
        // creature job stuff
        int32_t job_type_offset;
        int32_t job_id_offset;
        int32_t job_materials_vector;
        int32_t job_material_itemtype_o;
        int32_t job_material_subtype_o;
        int32_t job_material_subindex_o;
        int32_t job_material_index_o;
        int32_t job_material_flags_o;
        // creature job material stuff
    } creatures;
    uint32_t creature_module;
    uint32_t dwarf_race_index_addr;
    uint32_t dwarf_civ_id_addr;
    bool IdMapReady;
    std::map<int32_t, int32_t> IdMap;
    DfVector <uint32_t> *p_cre;
    DFContextShared *d;
    Process *owner;
};

Module* DFHack::createCreatures(DFContextShared * d)
{
    return new Creatures(d);
}

Creatures::Creatures(DFContextShared* _d)
{
    d = new Private;
    d->d = _d;
    d->owner = _d->p;
    d->Inited = false;
    d->Started = false;
    d->IdMapReady = false;
    d->p_cre = NULL;
    d->d->InitReadNames(); // throws on error
    VersionInfo * minfo = d->d->offset_descriptor;
    OffsetGroup *OG_Creatures = minfo->getGroup("Creatures");
    OffsetGroup *OG_creature = OG_Creatures->getGroup("creature");
    OffsetGroup *OG_creature_ex = OG_creature->getGroup("advanced");
    OffsetGroup *OG_soul = OG_Creatures->getGroup("soul");
    OffsetGroup * OG_name = minfo->getGroup("name");
    OffsetGroup * OG_jobs = OG_Creatures->getGroup("job");
    OffsetGroup * OG_job_mats = OG_jobs->getGroup("material");
    d->Ft_basic = d->Ft_advanced = d->Ft_jobs = d->Ft_soul = d->Ft_inventory = d->Ft_owned_items = d->Ft_job_materials = false;

    Private::t_offsets &creatures = d->creatures;
    try
    {
        // Creatures
        creatures.vector = OG_Creatures->getAddress ("vector");
        d->dwarf_race_index_addr = OG_Creatures->getAddress("current_race");
        d->dwarf_civ_id_addr = OG_Creatures->getAddress("current_civ");
        // Creatures/creature
        creatures.name_offset = OG_creature->getOffset ("name");
        creatures.custom_profession_offset = OG_creature->getOffset ("custom_profession");
        creatures.profession_offset = OG_creature->getOffset ("profession");
        creatures.race_offset = OG_creature->getOffset ("race");
        creatures.pos_offset = OG_creature->getOffset ("position");
        creatures.flags1_offset = OG_creature->getOffset ("flags1");
        creatures.flags2_offset = OG_creature->getOffset ("flags2");
        creatures.flags3_offset = OG_creature->getOffset ("flags3");
        creatures.sex_offset = OG_creature->getOffset ("sex");
        creatures.caste_offset = OG_creature->getOffset ("caste");
        creatures.id_offset = OG_creature->getOffset ("id");
        creatures.civ_offset = OG_creature->getOffset ("civ");
        // name struct
        creatures.name_firstname_offset = OG_name->getOffset("first");
        creatures.name_nickname_offset = OG_name->getOffset("nick");
        creatures.name_words_offset = OG_name->getOffset("second_words");
        d->Ft_basic = true;
        try
        {
            creatures.pickup_equipment_bit = OG_creature_ex->getOffset("pickup_equipment_bit");
            creatures.mood_offset = OG_creature_ex->getOffset("mood");
            // pregnancy
            // pregnancy_ptr
            creatures.birth_year_offset = OG_creature_ex->getOffset("birth_year");
            creatures.birth_time_offset = OG_creature_ex->getOffset("birth_time");
            creatures.current_job_offset = OG_creature_ex->getOffset("current_job");
            creatures.mood_skill_offset = OG_creature_ex->getOffset("current_job_skill");
            creatures.physical_offset = OG_creature_ex->getOffset("physical");
            creatures.appearance_vector_offset = OG_creature_ex->getOffset("appearance_vector");
            creatures.artifact_name_offset = OG_creature_ex->getOffset("artifact_name");
            creatures.labors_offset = OG_creature_ex->getOffset ("labors");
            creatures.happiness_offset = OG_creature_ex->getOffset ("happiness");
            d->Ft_advanced = true;
        }catch(Error::All&){};
        try
        {
            creatures.inventory_offset = OG_creature_ex->getOffset("inventory_vector");
            d->Ft_inventory = true;
        }
        catch(Error::All&){};
        try
        {
            creatures.owned_items_offset = OG_creature_ex->getOffset("owned_items_vector");
            d->Ft_owned_items = true;
        }
        catch(Error::All&){};
        try
        {
            creatures.soul_vector_offset = OG_creature_ex->getOffset("soul_vector");
            creatures.default_soul_offset = OG_creature_ex->getOffset("current_soul");
            creatures.soul_mental_offset = OG_soul->getOffset("mental");
            creatures.soul_skills_vector_offset = OG_soul->getOffset("skills_vector");
            creatures.soul_traits_offset = OG_soul->getOffset("traits");
            d->Ft_soul = true;
        }
        catch(Error::All&){};
        try
        {
            creatures.job_type_offset = OG_jobs->getOffset("type");
            creatures.job_id_offset = OG_jobs->getOffset("id");
            d->Ft_jobs = true;
            try
            {
                creatures.job_materials_vector = OG_jobs->getOffset("materials_vector");
                creatures.job_material_itemtype_o = OG_job_mats->getOffset("maintype");
                creatures.job_material_subtype_o = OG_job_mats->getOffset("sectype1");
                creatures.job_material_subindex_o = OG_job_mats->getOffset("sectype2");
                creatures.job_material_index_o = OG_job_mats->getOffset("sectype3");
                creatures.job_material_flags_o = OG_job_mats->getOffset("flags");
                d->Ft_job_materials = true;
            }
            catch(Error::All&){};
        }
        catch(Error::All&){};
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
    if(d->Ft_basic)
    {
        d->p_cre = new DfVector <uint32_t> (d->owner, d->creatures.vector);
        d->Started = true;
        numcreatures =  d->p_cre->size();
        d->IdMapReady = false;
        return true;
    }
    return false;
}

bool Creatures::Finish()
{
    if(d->p_cre)
    {
        delete d->p_cre;
        d->p_cre = 0;
    }
    d->Started = false;
    return true;
}

bool Creatures::ReadCreature (const int32_t index, t_creature & furball)
{
    if(!d->Started) return false;
    memset(&furball, 0, sizeof(t_creature));
    // SHM fast path
    Process * p = d->owner;

    // read pointer from vector at position
    uint32_t addr_cr = d->p_cre->at (index);
    furball.origin = addr_cr;
    Private::t_offsets &offs = d->creatures;

    //read creature from memory
    if(d->Ft_basic)
    {
        // name
        d->d->readName(furball.name,addr_cr + offs.name_offset);

        // basic stuff
        p->readDWord (addr_cr + offs.id_offset, furball.id);
        p->read (addr_cr + offs.pos_offset, 3 * sizeof (uint16_t), (uint8_t *) & (furball.x)); // xyz really
        p->readDWord (addr_cr + offs.race_offset, furball.race);
        furball.civ = p->readDWord (addr_cr + offs.civ_offset);
        p->readByte (addr_cr + offs.sex_offset, furball.sex);
        p->readWord (addr_cr + offs.caste_offset, furball.caste);
        p->readDWord (addr_cr + offs.flags1_offset, furball.flags1.whole);
        p->readDWord (addr_cr + offs.flags2_offset, furball.flags2.whole);
        p->readDWord (addr_cr + offs.flags3_offset, furball.flags3.whole);
        // custom profession
        p->readSTLString(addr_cr + offs.custom_profession_offset, furball.custom_profession, sizeof(furball.custom_profession));
        // profession
        furball.profession = p->readByte (addr_cr + offs.profession_offset);
    }
    if(d->Ft_advanced)
    {
        // happiness
        p->readDWord (addr_cr + offs.happiness_offset, furball.happiness);

        // physical attributes
        p->read(addr_cr + offs.physical_offset,
            sizeof(t_attrib) * NUM_CREATURE_PHYSICAL_ATTRIBUTES,
            (uint8_t *)&furball.strength);

        // mood stuff
        furball.mood = (int16_t) p->readWord (addr_cr + offs.mood_offset);
        furball.mood_skill = p->readWord (addr_cr + offs.mood_skill_offset);
        d->d->readName(furball.artifact_name, addr_cr + offs.artifact_name_offset);

        // labors
        p->read (addr_cr + offs.labors_offset, NUM_CREATURE_LABORS, furball.labors);

        furball.birth_year = p->readDWord (addr_cr + offs.birth_year_offset );
        furball.birth_time = p->readDWord (addr_cr + offs.birth_time_offset );
        /*
         * p->readDWord(temp + offs.creature_pregnancy_offset, furball.pregnancy_timer);
         */

        // appearance
        DfVector <uint32_t> app(p, addr_cr + offs.appearance_vector_offset);
        furball.nbcolors = app.size();
        if(furball.nbcolors>MAX_COLORS)
            furball.nbcolors = MAX_COLORS;
        for(uint32_t i = 0; i < furball.nbcolors; i++)
        {
            furball.color[i] = app[i];
        }

        //likes
        /*
        DfVector <uint32_t> likes(d->p, temp + offs.creature_likes_offset);
        furball.numLikes = likes.getSize();
        for(uint32_t i = 0;i<furball.numLikes;i++)
        {
            uint32_t temp2 = *(uint32_t *) likes[i];
            p->read(temp2,sizeof(t_like),(uint8_t *) &furball.likes[i]);
        }*/
    }
    if(d->Ft_soul)
    {
        /*
        // enum soul pointer vector
        DfVector <uint32_t> souls(p,temp + offs.creature_soul_vector_offset);
        */
        uint32_t soul = p->readDWord(addr_cr + offs.default_soul_offset);
        furball.has_default_soul = false;

        if(soul)
        {
            furball.has_default_soul = true;
            // get first soul's skills
            DfVector <uint32_t> skills(p, soul + offs.soul_skills_vector_offset);
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
    if(d->Ft_jobs)
    {
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
    }
    else
    {
        furball.current_job.active = false;
    }
    return true;
}

// returns index of creature actually read or -1 if no creature can be found
int32_t Creatures::ReadCreatureInBox (int32_t index, t_creature & furball,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
    if (!d->Started)
        return -1;

    Process *p = d->owner;
    uint16_t coords[3];
    uint32_t size = d->p_cre->size();
    while (uint32_t(index) < size)
    {
        // read pointer from vector at position
        uint32_t temp = d->p_cre->at(index);
        p->read (temp + d->creatures.pos_offset, 3 * sizeof (uint16_t), (uint8_t *) &coords);
        if (coords[0] >= x1 && coords[0] < x2)
        {
            if (coords[1] >= y1 && coords[1] < y2)
            {
                if (coords[2] >= z1 && coords[2] < z2)
                {
                    ReadCreature (index, furball);
                    return index;
                }
            }
        }
        index++;
    }
    return -1;
}

int32_t Creatures::FindIndexById(int32_t creature_id)
{
    if (!d->Started || !d->Ft_basic)
        return -1;

    if (!d->IdMapReady)
    {
        d->IdMap.clear();

        Process * p = d->owner;
        Private::t_offsets &offs = d->creatures;

        uint32_t size = d->p_cre->size();
        for (uint32_t index = 0; index < size; index++)
        {
            uint32_t temp = d->p_cre->at(index);
            int32_t id = p->readDWord (temp + offs.id_offset);
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

    DfVector<uint32_t> skills(p, souloff + d->creatures.soul_skills_vector_offset);

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
    DfVector <uint32_t> cmats(p, furball->current_job.occupationPtr + off.job_materials_vector);

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

bool Creatures::ReadJob(const t_creature * furball, vector<t_material> & mat)
{
    unsigned int i;
    if(!d->Inited || !d->Ft_job_materials) return false;
    if(!furball->current_job.active) return false;

    Process * p = d->owner;
    Private::t_offsets & off = d->creatures;
    DfVector <uint32_t> cmats(p, furball->current_job.occupationPtr + off.job_materials_vector);
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

bool Creatures::ReadInventoryIdx(const uint32_t index, std::vector<uint32_t> & item)
{
    if(!d->Started || !d->Ft_inventory) return false;
    uint32_t temp = d->p_cre->at (index);
    return this->ReadInventoryPtr(temp, item);
}

bool Creatures::ReadInventoryPtr(const uint32_t temp, std::vector<uint32_t> & item)
{
    unsigned int i;
    if(!d->Started || !d->Ft_inventory) return false;
    Process * p = d->owner;

    DfVector <uint32_t> citem(p, temp + d->creatures.inventory_offset);
    if(citem.size() == 0)
        return false;
    item.resize(citem.size());
    for(i=0;i<citem.size();i++)
        item[i] = p->readDWord(citem[i]);
    return true;
}

bool Creatures::ReadOwnedItemsIdx(const uint32_t index, std::vector<int32_t> & item)
{
    if(!d->Started || !d->Ft_owned_items) return false;
    uint32_t temp = d->p_cre->at (index);
    return this->ReadOwnedItemsPtr(temp, item);
}

bool Creatures::ReadOwnedItemsPtr(const uint32_t temp, std::vector<int32_t> & item)
{
    unsigned int i;
    if(!d->Started || !d->Ft_owned_items) return false;
    Process * p = d->owner;

    DfVector <int32_t> citem(p, temp + d->creatures.owned_items_offset);
    if(citem.size() == 0)
        return false;
    item.resize(citem.size());
    for(i=0;i<citem.size();i++)
        item[i] = citem[i];
    return true;
}

bool Creatures::RemoveOwnedItemIdx(const uint32_t index, int32_t id)
{
    if(!d->Started || !d->Ft_owned_items)
    {
        cerr << "!d->Started || !d->Ft_owned_items FAIL" << endl;
        return false;
    }
    uint32_t temp = d->p_cre->at (index);
    return this->RemoveOwnedItemPtr(temp, id);
}

bool Creatures::RemoveOwnedItemPtr(const uint32_t temp, int32_t id)
{
    if(!d->Started || !d->Ft_owned_items) return false;
    Process * p = d->owner;

    DfVector <int32_t> citem(p, temp + d->creatures.owned_items_offset);

    for (unsigned i = 0; i < citem.size(); i++) {
        if (citem[i] != id)
            continue;
        if (!citem.remove(i--))
            return false;
    }

    return true;
}

void Creatures::CopyNameTo(t_creature &creature, uint32_t address)
{
    Private::t_offsets &offs = d->creatures;

    if(d->Ft_basic)
        d->d->copyName(creature.origin + offs.name_offset, address);
}

