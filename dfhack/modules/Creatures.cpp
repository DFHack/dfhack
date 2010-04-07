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

#include "DFCommonInternal.h"
#include "../private/APIPrivate.h"

#include "DFVector.h"
#include "DFMemInfo.h"
#include "DFProcess.h"
#include "DFError.h"
#include "DFTypes.h"

// we connect to those
#include <shms.h>
#include <mod-core.h>
#include <mod-creature2010.h>
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
    DfVector *p_cre;
    APIPrivate *d;
};

Creatures::Creatures(APIPrivate* _d)
{
    d = new Private;
    d->d = _d;
    d->Inited = false;
    d->Started = false;
    d->d->InitReadNames(); // throws on error
    try
    {
        memory_info * minfo = d->d->offset_descriptor;
        Creatures2010::creature_offsets &creatures = d->creatures;
        creatures.creature_vector = minfo->getAddress ("creature_vector");
        creatures.creature_pos_offset = minfo->getOffset ("creature_position");
        creatures.creature_profession_offset = minfo->getOffset ("creature_profession");
        creatures.creature_custom_profession_offset = minfo->getOffset ("creature_custom_profession");
        creatures.creature_race_offset = minfo->getOffset ("creature_race");
        creatures.creature_flags1_offset = minfo->getOffset ("creature_flags1");
        creatures.creature_flags2_offset = minfo->getOffset ("creature_flags2");
        creatures.creature_name_offset = minfo->getOffset ("creature_name");
        creatures.creature_sex_offset = minfo->getOffset ("creature_sex");
        creatures.creature_id_offset = minfo->getOffset ("creature_id");
        creatures.creature_labors_offset = minfo->getOffset ("creature_labors");
        creatures.creature_happiness_offset = minfo->getOffset ("creature_happiness");
        creatures.creature_artifact_name_offset = minfo->getOffset("creature_artifact_name");
        creatures.creature_soul_vector_offset = minfo->getOffset("creature_soul_vector");
        
        // soul offsets
        creatures.soul_skills_vector_offset = minfo->getOffset("soul_skills_vector");
        
        // name offsets for the creature module
        creatures.name_firstname_offset = minfo->getOffset("name_firstname");
        creatures.name_nickname_offset = minfo->getOffset("name_nickname");
        creatures.name_words_offset = minfo->getOffset("name_words");
        
        // upload offsets to the SHM
        if(g_pProcess->getModuleIndex("Creatures2010",1,d->creature_module))
        {
            // supply the module with offsets so it can work with them
            memcpy(SHMDATA(Creatures2010::creature_offsets),&creatures,sizeof(Creatures2010::creature_offsets));
            const uint32_t cmd = Creatures2010::CREATURE_INIT + (d->creature_module << 16);
            g_pProcess->SetAndWait(cmd);
        }
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
    d->p_cre = new DfVector (d->d->p, d->creatures.creature_vector, 4);
    d->Started = true;
    numcreatures =  d->p_cre->getSize();
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
    if(d->creature_module)
    {
        SHMCREATURESHDR->index = index;
        const uint32_t cmd = Creatures2010::CREATURE_AT_INDEX + (d->creature_module << 16);
        g_pProcess->SetAndWait(cmd);
        memcpy(&furball,SHMDATA(t_creature),sizeof(t_creature));
        return true;
    }
    
    // non-SHM slow path
    
    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_cre->at (index);
    furball.origin = temp;
    Creatures2010::creature_offsets &offs = d->creatures;
    //read creature from memory
    g_pProcess->read (temp + offs.creature_pos_offset, 3 * sizeof (uint16_t), (uint8_t *) & (furball.x)); // xyz really
    g_pProcess->readDWord (temp + offs.creature_race_offset, furball.type);
    g_pProcess->readDWord (temp + offs.creature_flags1_offset, furball.flags1.whole);
    g_pProcess->readDWord (temp + offs.creature_flags2_offset, furball.flags2.whole);
    // names
    d->d->readName(furball.name,temp + offs.creature_name_offset);
    //d->readName(furball.squad_name, temp + offs.creature_squad_name_offset);
    d->d->readName(furball.artifact_name, temp + offs.creature_artifact_name_offset);
    // custom profession
    fill_char_buf (furball.custom_profession, g_pProcess->readSTLString (temp + offs.creature_custom_profession_offset));

    // labors
    g_pProcess->read (temp + offs.creature_labors_offset, NUM_CREATURE_LABORS, furball.labors);
    
    // traits
    //g_pProcess->read (temp + offs.creature_traits_offset, sizeof (uint16_t) * NUM_CREATURE_TRAITS, (uint8_t *) &furball.traits);

    // profession
    furball.profession = g_pProcess->readByte (temp + offs.creature_profession_offset);
    // current job HACK: the job object isn't cleanly represented here
    /*
    uint32_t jobIdAddr = g_pProcess->readDWord (temp + offs.creature_current_job_offset);

    if (jobIdAddr)
    {
        furball.current_job.active = true;
        furball.current_job.jobId = g_pProcess->readByte (jobIdAddr + offs.creature_current_job_id_offset);
    }
    else
    {
        furball.current_job.active = false;
    }
*/
    //likes
    /*
    DfVector likes(d->p, temp + offs.creature_likes_offset, 4);
    furball.numLikes = likes.getSize();
    for(uint32_t i = 0;i<furball.numLikes;i++)
    {
        uint32_t temp2 = *(uint32_t *) likes[i];
        g_pProcess->read(temp2,sizeof(t_like),(uint8_t *) &furball.likes[i]);
    }

    furball.mood = (int16_t) g_pProcess->readWord (temp + offs.creature_mood_offset);
*/

    g_pProcess->readDWord (temp + offs.creature_happiness_offset, furball.happiness);
    g_pProcess->readDWord (temp + offs.creature_id_offset, furball.id);
    /*
    g_pProcess->readDWord (temp + offs.creature_agility_offset, furball.agility);
    g_pProcess->readDWord (temp + offs.creature_strength_offset, furball.strength);
    g_pProcess->readDWord (temp + offs.creature_toughness_offset, furball.toughness);
    g_pProcess->readDWord (temp + offs.creature_money_offset, furball.money);
    furball.squad_leader_id = (int32_t) g_pProcess->readDWord (temp + offs.creature_squad_leader_id_offset);
    */
    g_pProcess->readByte (temp + offs.creature_sex_offset, furball.sex);
/*
    g_pProcess->readDWord(temp + offs.creature_pregnancy_offset, furball.pregnancy_timer);
    furball.blood_max = (int32_t) g_pProcess->readDWord(temp + offs.creature_blood_max_offset);
    furball.blood_current = (int32_t) g_pProcess->readDWord(temp + offs.creature_blood_current_offset);
    g_pProcess->readDWord(temp + offs.creature_bleed_offset, furball.bleed_rate);
*/

    // enum soul pointer vector
    DfVector souls(g_pProcess,temp + offs.creature_soul_vector_offset,4);
    // get first soul's skills
    DfVector skills(g_pProcess, *(uint32_t *)souls.at(0) + offs.soul_skills_vector_offset,4 );
    furball.numSkills = skills.getSize();
    for (uint32_t i = 0; i < furball.numSkills;i++)
    {
        uint32_t temp2 = * (uint32_t *) skills[i];
        //skills.read(i, (uint8_t *) &temp2);
        // a byte: this gives us 256 skills maximum.
        furball.skills[i].id = g_pProcess->readByte (temp2);
        furball.skills[i].rating = g_pProcess->readByte (temp2 + 4);
        furball.skills[i].experience = g_pProcess->readWord (temp2 + 8);
    }
    return true;
}

// returns index of creature actually read or -1 if no creature can be found
int32_t Creatures::ReadCreatureInBox (int32_t index, t_creature & furball,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
    if (!d->Started) return -1;
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
        g_pProcess->SetAndWait(cmd);
        if(SHMCREATURESHDR->index != -1)
            memcpy(&furball,SHMDATA(void),sizeof(t_creature));
        return SHMCREATURESHDR->index;
    }
    else
    {
        uint16_t coords[3];
        uint32_t size = d->p_cre->getSize();
        while (uint32_t(index) < size)
        {
            // read pointer from vector at position
            uint32_t temp = * (uint32_t *) d->p_cre->at (index);
            g_pProcess->read (temp + d->creatures.creature_pos_offset, 3 * sizeof (uint16_t), (uint8_t *) &coords);
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
    uint32_t temp = * (uint32_t *) d->p_cre->at (index);
    g_pProcess->write(temp + d->creatures.creature_labors_offset, NUM_CREATURE_LABORS, labors);
}

/*
bool API::getCurrentCursorCreature(uint32_t & creature_index)
{
    if(!d->cursorWindowInited) return false;
    creature_index = g_pProcess->readDWord(d->current_cursor_creature_offset);
    return true;
}
*/