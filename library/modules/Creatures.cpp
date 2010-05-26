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

#include "dfhack/DFCommonInternal.h"
#include "../private/APIPrivate.h"

#include "dfhack/DFMemInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFVector.h"
#include "dfhack/DFError.h"
#include "dfhack/DFTypes.h"

// we connect to those
#include <shms.h>
#include <mod-core.h>
#include <mod-creature2010.h>
#include "modules/Materials.h"
#include "modules/Creatures.h"


#define SHMCREATURESHDR ((Creatures2010::shm_creature_hdr *)d->d->shm_start)
#define SHMCMD(num) ((shm_cmd *)d->d->shm_start)[num]->pingpong
#define SHMHDR ((shm_core_hdr *)d->d->shm_start)
#define SHMDATA(type) ((type *)(d->d->shm_start + SHM_HEADER))

using namespace DFHack;

struct Creatures::Private
{
    bool Inited;
    bool Started;
    Creatures2010::creature_offsets creatures;
    uint32_t creature_module;
    uint32_t dwarf_race_index_addr;
    uint32_t dwarf_civ_id_addr;
    DfVector <uint32_t> *p_cre;
    DFContextPrivate *d;
    Process *owner;
};

Creatures::Creatures(DFContextPrivate* _d)
{
    d = new Private;
    d->d = _d;
    Process * p = d->owner = _d->p;
    d->Inited = false;
    d->Started = false;
    d->d->InitReadNames(); // throws on error
    try
    {
        memory_info * minfo = d->d->offset_descriptor;
        Creatures2010::creature_offsets &creatures = d->creatures;
        creatures.vector = minfo->getAddress ("creature_vector");
        creatures.pos_offset = minfo->getOffset ("creature_position");
        creatures.profession_offset = minfo->getOffset ("creature_profession");
        creatures.custom_profession_offset = minfo->getOffset ("creature_custom_profession");
        creatures.race_offset = minfo->getOffset ("creature_race");
        creatures.civ_offset = minfo->getOffset ("creature_civ");
        creatures.flags1_offset = minfo->getOffset ("creature_flags1");
        creatures.flags2_offset = minfo->getOffset ("creature_flags2");
        creatures.name_offset = minfo->getOffset ("creature_name");
        creatures.sex_offset = minfo->getOffset ("creature_sex");
        creatures.caste_offset = minfo->getOffset ("creature_caste");
        creatures.id_offset = minfo->getOffset ("creature_id");
        creatures.labors_offset = minfo->getOffset ("creature_labors");
        creatures.happiness_offset = minfo->getOffset ("creature_happiness");
        creatures.artifact_name_offset = minfo->getOffset("creature_artifact_name");
        creatures.soul_vector_offset = minfo->getOffset("creature_soul_vector");
        creatures.default_soul_offset = minfo->getOffset("creature_default_soul");
        creatures.physical_offset = minfo->getOffset("creature_physical");
        creatures.mood_offset = minfo->getOffset("creature_mood");
        creatures.mood_skill_offset = minfo->getOffset("creature_mood_skill");
        creatures.pickup_equipment_bit = minfo->getOffset("creature_pickup_equipment_bit");
	creatures.current_job_offset = minfo->getOffset("creature_current_job");
        // soul offsets
        creatures.soul_skills_vector_offset = minfo->getOffset("soul_skills_vector");
        creatures.soul_mental_offset = minfo->getOffset("soul_mental");
        creatures.soul_traits_offset = minfo->getOffset("soul_traits");
        
        // appearance
        creatures.appearance_vector_offset = minfo->getOffset("creature_appearance_vector");

        //birth
        creatures.birth_year_offset = minfo->getOffset("creature_birth_year");
        creatures.birth_time_offset = minfo->getOffset("creature_birth_time");
        
        // name offsets for the creature module
        creatures.name_firstname_offset = minfo->getOffset("name_firstname");
        creatures.name_nickname_offset = minfo->getOffset("name_nickname");
        creatures.name_words_offset = minfo->getOffset("name_words");
        d->dwarf_race_index_addr = minfo->getAddress("dwarf_race_index");
        d->dwarf_civ_id_addr = minfo->getAddress("dwarf_civ_id");
        /*
        // upload offsets to the SHM
        if(p->getModuleIndex("Creatures2010",1,d->creature_module))
        {
            // supply the module with offsets so it can work with them
            memcpy(SHMDATA(Creatures2010::creature_offsets),&creatures,sizeof(Creatures2010::creature_offsets));
            const uint32_t cmd = Creatures2010::CREATURE_INIT + (d->creature_module << 16);
            p->SetAndWait(cmd);
        }
        */
        d->Inited = true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->Inited = false;
        throw;
    }
}

Creatures::~Creatures()
{
    if(d->Started)
        Finish();
}

bool Creatures::Start( uint32_t &numcreatures )
{
    d->p_cre = new DfVector <uint32_t> (d->owner, d->creatures.vector);
    d->Started = true;
    numcreatures =  d->p_cre->size();
    return true;
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
    // SHM fast path
    Process * p = d->owner;
    /*
    if(d->creature_module)
    {
        SHMCREATURESHDR->index = index;
        const uint32_t cmd = Creatures2010::CREATURE_AT_INDEX + (d->creature_module << 16);
        p->SetAndWait(cmd);
        memcpy(&furball,SHMDATA(t_creature),sizeof(t_creature));
        return true;
    }
    */
    // non-SHM slow path
    memory_info * minfo = d->d->offset_descriptor;
    
    // read pointer from vector at position
    uint32_t temp = d->p_cre->at (index);
    furball.origin = temp;
    Creatures2010::creature_offsets &offs = d->creatures;
    
    //read creature from memory
    
    // name
    d->d->readName(furball.name,temp + offs.name_offset);
    
    // basic stuff
    p->readDWord (temp + offs.happiness_offset, furball.happiness);
    p->readDWord (temp + offs.id_offset, furball.id);
    p->read (temp + offs.pos_offset, 3 * sizeof (uint16_t), (uint8_t *) & (furball.x)); // xyz really
    p->readDWord (temp + offs.race_offset, furball.race);
    furball.civ = p->readDWord (temp + offs.civ_offset);
    p->readByte (temp + offs.sex_offset, furball.sex);
    p->readWord (temp + offs.caste_offset, furball.caste);
    p->readDWord (temp + offs.flags1_offset, furball.flags1.whole);
    p->readDWord (temp + offs.flags2_offset, furball.flags2.whole);
    
    // physical attributes
    p->read(temp + offs.physical_offset, sizeof(t_attrib) * 6, (uint8_t *)&furball.strength);
        
    // mood stuff
    furball.mood = (int16_t) p->readWord (temp + offs.mood_offset);
    furball.mood_skill = p->readWord (temp + offs.mood_skill_offset);
    d->d->readName(furball.artifact_name, temp + offs.artifact_name_offset);
    
    // custom profession
    p->readSTLString(temp + offs.custom_profession_offset, furball.custom_profession, sizeof(furball.custom_profession));
    //fill_char_buf (furball.custom_profession, p->readSTLString (temp + offs.custom_profession_offset));

    // labors
    p->read (temp + offs.labors_offset, NUM_CREATURE_LABORS, furball.labors);
    
    // profession
    furball.profession = p->readByte (temp + offs.profession_offset);

    furball.current_job.occupationPtr = p->readDWord (temp + offs.current_job_offset);
    if(furball.current_job.occupationPtr)
    {
        furball.current_job.active = true;
        furball.current_job.jobType = p->readByte (furball.current_job.occupationPtr + minfo->getOffset("job_type") );
        furball.current_job.jobId = p->readDWord (furball.current_job.occupationPtr + minfo->getOffset("job_id") );
    }
    else
    {
        furball.current_job.active = false;;
    }

    furball.birth_year = p->readDWord (temp + offs.birth_year_offset );
    furball.birth_time = p->readDWord (temp + offs.birth_time_offset );

    // current job HACK: the job object isn't cleanly represented here
    /*
    uint32_t jobIdAddr = p->readDWord (temp + offs.creature_current_job_offset);

    if (jobIdAddr)
    {
        furball.current_job.active = true;
        furball.current_job.jobId = p->readByte (jobIdAddr + offs.creature_current_job_id_offset);
    }
    else
    {
        furball.current_job.active = false;
    }
    */

    /*
        p->readDWord(temp + offs.creature_pregnancy_offset, furball.pregnancy_timer);
    */

    /*
    // enum soul pointer vector
    DfVector <uint32_t> souls(p,temp + offs.creature_soul_vector_offset);
    */
    uint32_t soul = p->readDWord(temp + offs.default_soul_offset);
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
            furball.defaultSoul.skills[i].rating = p->readByte (temp2 + 4);
            furball.defaultSoul.skills[i].experience = p->readWord (temp2 + 8);
        }
        // mental attributes are part of the soul
        p->read(soul + offs.soul_mental_offset, sizeof(t_attrib) * 13, (uint8_t *)&furball.defaultSoul.analytical_ability);
        
        // traits as well
        p->read(soul + offs.soul_traits_offset, sizeof (uint16_t) * NUM_CREATURE_TRAITS, (uint8_t *) &furball.defaultSoul.traits);
    }

    DfVector <uint32_t> app(p, temp + offs.appearance_vector_offset);
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
    /*
    if(d->creature_module)
    {
        // supply the module with offsets so it can work with them
        SHMCREATURESHDR->index = index;
        SHMCREATURESHDR->x = x1;
        SHMCREATURESHDR->y = y1;
        SHMCREATURESHDR->z = z1;
        SHMCREATURESHDR->x2 = x2;
        SHMCREATURESHDR->y2 = y2;
        SHMCREATURESHDR->z2 = z2;
        const uint32_t cmd = Creatures2010::CREATURE_FIND_IN_BOX + (d->creature_module << 16);
        p->SetAndWait(cmd);
        if(SHMCREATURESHDR->index != -1)
            memcpy(&furball,SHMDATA(void),sizeof(t_creature));
        return SHMCREATURESHDR->index;
    }
    else*/
    {
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
}



bool Creatures::WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS])
{
    if(!d->Started) return false;
    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    
    p->write(temp + d->creatures.labors_offset, NUM_CREATURE_LABORS, labors);
    uint32_t pickup_equip;
    p->readDWord(temp + d->creatures.pickup_equipment_bit, pickup_equip);
    pickup_equip |= 1u;
    p->writeDWord(temp + d->creatures.pickup_equipment_bit, pickup_equip);
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
    if(!d->Inited) return false;
    if(!furball->current_job.active) return false;
    Process * p = d->owner;
    memory_info * minfo = d->d->offset_descriptor;

    DfVector <uint32_t> cmats(p, furball->current_job.occupationPtr + minfo->getOffset("job_materials_vector"));
    mat.resize(cmats.size());
    for(i=0;i<cmats.size();i++)
    {
        mat[i].itemType = p->readWord(cmats[i] + minfo->getOffset("job_material_maintype"));
        mat[i].subType = p->readWord(cmats[i] + minfo->getOffset("job_material_sectype1"));
        mat[i].subIndex = p->readWord(cmats[i] + minfo->getOffset("job_material_sectype2"));
        mat[i].index = p->readDWord(cmats[i] + minfo->getOffset("job_material_sectype3"));
        mat[i].flags = p->readDWord(cmats[i] + minfo->getOffset("job_material_flags"));
    }
    return true;
}
