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

#define SHMCREATURESHDR ((Creatures2010::shm_creature_hdr *)d->shm_start)

using namespace DFHack;

bool API::InitReadCreatures( uint32_t &numcreatures )
{
    if(!d->InitReadNames()) return false;
    try
    {
        memory_info * minfo = d->offset_descriptor;
        Creatures2010::creature_offsets & off = d->creatures;
        off.creature_vector = minfo->getAddress ("creature_vector");
        off.creature_pos_offset = minfo->getOffset ("creature_position");
        off.creature_profession_offset = minfo->getOffset ("creature_profession");
        off.creature_race_offset = minfo->getOffset ("creature_race");
        off.creature_flags1_offset = minfo->getOffset ("creature_flags1");
        off.creature_flags2_offset = minfo->getOffset ("creature_flags2");
        off.creature_name_offset = minfo->getOffset ("creature_name");
        off.creature_sex_offset = minfo->getOffset ("creature_sex");
        off.creature_id_offset = minfo->getOffset ("creature_id");
        off.creature_labors_offset = minfo->getOffset ("creature_labors");
        off.creature_happiness_offset = minfo->getOffset ("creature_happiness");
        off.creature_artifact_name_offset = minfo->getOffset("creature_artifact_name");
        
        // name offsets for the creature module
        off.name_firstname_offset = minfo->getOffset("name_firstname");
        off.name_nickname_offset = minfo->getOffset("name_nickname");
        off.name_words_offset = minfo->getOffset("name_words");
        
        d->p_cre = new DfVector (d->p, off.creature_vector, 4);
        d->creaturesInited = true;
        numcreatures =  d->p_cre->getSize();

        // --> SHM initialization (if possible) <--
        g_pProcess->getModuleIndex("Creatures2010",1,d->creature_module);
        
        if(d->creature_module)
        {
            // supply the module with offsets so it can work with them
            memcpy(SHMDATA(Creatures2010::creature_offsets),&d->creatures,sizeof(Creatures2010::creature_offsets));
            const uint32_t cmd = Creatures2010::CREATURE_INIT + (d->creature_module << 16);
            g_pProcess->SetAndWait(cmd);
        }
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->creaturesInited = false;
        numcreatures = 0;
        throw;
    }
}

bool API::ReadCreature (const int32_t index, t_creature & furball)
{
    if(!d->creaturesInited) return false;
    if(d->creature_module)
    {
        // supply the module with offsets so it can work with them
        SHMCREATURESHDR->index = index;
        const uint32_t cmd = Creatures2010::CREATURE_AT_INDEX + (d->creature_module << 16);
        g_pProcess->SetAndWait(cmd);
        memcpy(&furball,SHMDATA(t_creature),sizeof(t_creature));
        // cerr << "creature read from SHM!" << endl;
        return true;
    }
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
    d->readName(furball.name,temp + offs.creature_name_offset);
    //d->readName(furball.squad_name, temp + offs.creature_squad_name_offset);
    d->readName(furball.artifact_name, temp + offs.creature_artifact_name_offset);
    // custom profession
    //fill_char_buf (furball.custom_profession, d->p->readSTLString (temp + offs.creature_custom_profession_offset));

    // labors
    g_pProcess->read (temp + offs.creature_labors_offset, NUM_CREATURE_LABORS, furball.labors);
    // traits
    //g_pProcess->read (temp + offs.creature_traits_offset, sizeof (uint16_t) * NUM_CREATURE_TRAITS, (uint8_t *) &furball.traits);
    // learned skills
    /*
    DfVector skills (d->p, temp + offs.creature_skills_offset, 4 );
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
    */
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

    return true;
}
/*
bool API::WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS])
{
    if(!d->creaturesInited) return false;
    uint32_t temp = * (uint32_t *) d->p_cre->at (index);
    WriteRaw(temp + d->creatures.creature_labors_offset, NUM_CREATURE_LABORS, labors);
}
*/
/*
bool API::getCurrentCursorCreature(uint32_t & creature_index)
{
    if(!d->cursorWindowInited) return false;
    creature_index = g_pProcess->readDWord(d->current_cursor_creature_offset);
    return true;
}
*/
void API::FinishReadCreatures()
{
    if(d->p_cre)
    {
        delete d->p_cre;
        d->p_cre = 0;
    }
    d->creaturesInited = false;
    //FinishReadNameTables();
}