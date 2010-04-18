#include <string>
#include <vector>
#include <integers.h>

#include "shms.h"
#include "mod-core.h"
#include "mod-creature40d.h"
#include <DFTypes.h>

#include <string.h>
#include <malloc.h>
#include <vector>
#include <stdio.h>

extern char *shm;

namespace DFHack{ namespace Creatures{ // start of namespace

#define SHMHDR ((shm_creature_hdr *)shm)
#define SHMDATA(type) ((type *)(shm + SHM_HEADER))

void readName(t_name & name, char * address, creature_offsets & offsets)
{
    // custom profession
    std::string * fname = (std::string *) (address + offsets.name_firstname_offset);
    strncpy(name.first_name,fname->c_str(),127);
    name.first_name[127] = 0;
    
    std::string * nname = (std::string *) (address + offsets.name_nickname_offset);
    strncpy(name.nickname,nname->c_str(),127);
    name.nickname[127] = 0;
    
    memcpy(name.words, (void *)(address + offsets.name_words_offset), 48);
}

void InitOffsets (void* data)
{
    creature_modulestate * state = (creature_modulestate *) data;
    memcpy((void *) &(state->offsets), SHMDATA(void), sizeof(creature_offsets));
    ((creature_modulestate *) data)->inited = true;
}

void ReadCreatureAtIndex(void *data)
{
    creature_modulestate * state = (creature_modulestate *) data;
    creature_offsets & offsets = state->offsets;
    std::vector<char *> * creaturev = (std::vector<char *> *) (offsets.creature_vector + offsets.vector_correct);
    uint32_t length = creaturev->size();
    int32_t index = SHMHDR->index;
    
    // read pointer from vector at position
    char * temp = (char *) creaturev->at (index);
    t_creature *furball = SHMDATA(t_creature);
    furball->origin = (uint32_t) temp;
    //read creature from memory
    memcpy(&(furball->x),temp + offsets.creature_pos_offset,3* sizeof(uint16_t));
    furball->type = *(uint32_t *) (temp + offsets.creature_type_offset);
    furball->flags1.whole = *(uint32_t *) (temp + offsets.creature_flags1_offset);
    furball->flags2.whole = *(uint32_t *) (temp + offsets.creature_flags2_offset);
    // names
    
    readName(furball->name,temp + offsets.creature_name_offset, offsets);
    readName(furball->squad_name, temp + offsets.creature_squad_name_offset, offsets);
    readName(furball->artifact_name, temp + offsets.creature_artifact_name_offset, offsets);
    
    // custom profession
    std::string * custprof = (std::string *) (temp + offsets.creature_custom_profession_offset);
    strncpy(furball->custom_profession,custprof->c_str(),127);
    furball->custom_profession[127] = 0;

    // labors
    memcpy(furball->labors, temp + offsets.creature_labors_offset, NUM_CREATURE_LABORS);
    
    // traits
    memcpy(furball->traits, temp + offsets.creature_traits_offset, sizeof (uint16_t) * NUM_CREATURE_TRAITS);

    typedef struct
    {
        uint8_t id;
        junk_fill <3> fill1;
        uint8_t rating;
        junk_fill <3> fill2;
        uint16_t experience;
    } raw_skill;
    // learned skills
    std::vector <void *> * skillv = (std::vector <void *> *) (temp + offsets.creature_skills_offset + offsets.vector_correct);
    furball->numSkills = skillv->size();
    for (uint32_t i = 0; i < furball->numSkills;i++)
    {
        //skills.read(i, (uint8_t *) &temp2);
        // a byte: this gives us 256 skills maximum.
        furball->skills[i].id = ( (raw_skill*) skillv->at(i))->id;
        furball->skills[i].rating = ( (raw_skill*) skillv->at(i))->rating;
        furball->skills[i].experience = ( (raw_skill*) skillv->at(i))->experience;
    }
    // profession
    furball->profession = *(uint8_t *) (temp + offsets.creature_profession_offset);
    // current job HACK: the job object isn't cleanly represented here
    uint32_t jobIdAddr = *(uint32_t *) (temp + offsets.creature_current_job_offset);

    if (jobIdAddr)
    {
        furball->current_job.active = true;
        furball->current_job.jobId = *(uint8_t *) (jobIdAddr + offsets.creature_current_job_id_offset);
    }
    else
    {
        furball->current_job.active = false;
    }

    //likes
    std::vector <t_like *> * likev = (std::vector <t_like *> *) (temp + offsets.creature_likes_offset + offsets.vector_correct);
    furball->numLikes = likev->size();
    for(uint32_t i = 0;i<furball->numLikes;i++)
    {
        memcpy(&furball->likes[i], likev->at(i), sizeof(t_skill));
    }

    furball->mood = *(int16_t *) (temp + offsets.creature_mood_offset);


    furball->happiness =  *(uint32_t *) (temp + offsets.creature_happiness_offset);
    furball->id = *(uint32_t *) (temp + offsets.creature_id_offset);
    furball->agility = *(uint32_t *) (temp + offsets.creature_agility_offset);
    furball->strength = *(uint32_t *) (temp + offsets.creature_strength_offset);
    furball->toughness = *(uint32_t *) (temp + offsets.creature_toughness_offset);
    furball->money =  *(uint32_t *) (temp + offsets.creature_money_offset);
    furball->squad_leader_id = *(int32_t*) (temp + offsets.creature_squad_leader_id_offset);
    furball->sex = *(uint8_t*) (temp + offsets.creature_sex_offset);

    furball->pregnancy_timer = *(uint32_t *)(temp+offsets.creature_pregnancy_offset);
    furball->blood_max = *(int32_t*) (temp+offsets.creature_blood_max_offset);
    furball->blood_current = *(int32_t*) (temp+offsets.creature_blood_current_offset);
    furball->bleed_rate = *(uint32_t*) (temp+offsets.creature_bleed_offset);
}

void FindNextCreatureInBox (void * data)
{
    int32_t index = SHMHDR->index;
    // sanity
    if(index == -1) return;
        
    creature_modulestate * state = (creature_modulestate *) data;
    creature_offsets & offsets = state->offsets;
    uint32_t x,y,z,x2,y2,z2;
    
    x = SHMHDR->x; x2 = SHMHDR->x2;
    y = SHMHDR->y; y2 = SHMHDR->y2;
    z = SHMHDR->z; z2 = SHMHDR->x2;

    std::vector<char *> * creaturev = (std::vector<char *> *) (offsets.creature_vector + offsets.vector_correct);
    
    uint32_t length = creaturev->size();
    typedef uint16_t coords[3];
    
    // look at all creatures, starting at index
    // if you find one in the specified 'box', return the creature in the data
    // section and the index in the header
    for(;index < length;index++)
    {
        coords& coo = *(coords*) (creaturev->at(index) + offsets.creature_pos_offset);
        if(coo[0] >=x && coo[0] < x2
            && coo[1] >=y && coo[1] < y2
                && coo[2] >=z && coo[2] < z2)
                {
                    // we found a creature!
                    SHMHDR->index = index;
                    ReadCreatureAtIndex(data);
                    return; // we're done here
                }
    }
    // nothing found
    SHMHDR->index = -1;
}

DFPP_module Init( void )
{
    DFPP_module creatures;
    creatures.name = "Creatures40d";
    creatures.version = CREATURES40D_VERSION;
    // freed by the core
    creatures.modulestate = malloc(sizeof(creature_modulestate)); // we store a flag
    memset(creatures.modulestate,0,sizeof(creature_modulestate));
    
    creatures.reserve(NUM_CREATURE_CMDS);
    
    creatures.set_command(CREATURE_INIT, FUNCTION, "Supply the Creature40d module with offsets",InitOffsets,CORE_SUSPENDED);
    creatures.set_command(CREATURE_FIND_IN_BOX, FUNCTION, "Get next creature in a box, return new index or -1", FindNextCreatureInBox, CORE_SUSPENDED);
    creatures.set_command(CREATURE_AT_INDEX, FUNCTION, "Get creature at index", ReadCreatureAtIndex, CORE_SUSPENDED);
    
    return creatures;
}

}} // end of namespace
