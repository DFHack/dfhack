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

#pragma once
#ifndef CL_MOD_CREATURES
#define CL_MOD_CREATURES
/*
* Creatures
*/
#include "Export.h"
#include "Module.h"
#include "modules/Items.h"
#include "df/unit.h"

/**
 * \defgroup grp_units Unit module parts
 * @ingroup grp_modules
 */
namespace DFHack
{
    // FIXME: WTF IS THIS SHIT?
    /*
    struct t_labor
    {
    string name;
    uint8_t value;
    t_labor() { 
    value =0;
    }
    t_labor(const t_labor & b){
    name=b.name;
    value=b.value;
    }
    t_labor & operator=(const t_labor &b){
    name=b.name;
    value=b.value;
    return *this;
    }
    };
    struct t_skill
    {
    string name;
    uint16_t id;
    uint32_t experience;
    uint16_t rating;
    t_skill(){
    id=rating=0;
    experience=0;
    }
    t_skill(const t_skill & b)
    {
    name=b.name;
    id=b.id;
    experience=b.experience;
    rating=b.rating;
    }
    t_skill & operator=(const t_skill &b)
    {
    name=b.name;
    id=b.id;
    experience=b.experience;
    rating=b.rating;
    return *this;
    }
    };

    struct t_trait
    {
    uint16_t value;
    string displayTxt;
    string name;
    t_trait(){
    value=0;
    }
    t_trait(const t_trait &b)
    {
    name=b.name;
    displayTxt=b.displayTxt;
    value=b.value;
    }
    t_trait & operator=(const t_trait &b)
    {
    name=b.name;
    displayTxt=b.displayTxt;
    value=b.value;
    return *this;
    }
    };
    */
    /**
     * \ingroup grp_units
     */
    struct t_skill
    {
        uint32_t id;
        uint32_t rating;
        uint32_t experience;
    };
    /**
     * \ingroup grp_units
     */
    struct t_job
    {
        bool active;
        uint32_t jobId;
        uint8_t jobType;
        uint32_t occupationPtr;
    };
    /**
     * \ingroup grp_units
     */
    struct t_like
    {
        int16_t type;
        int16_t itemClass;
        int16_t itemIndex;
        t_matglossPair material;
        bool active;
        uint32_t mystery;
    };

    // FIXME: THIS IS VERY, VERY BAD.
    #define NUM_CREATURE_LABORS 96
    #define NUM_CREATURE_TRAITS 30
    #define NUM_CREATURE_MENTAL_ATTRIBUTES 13
    #define NUM_CREATURE_PHYSICAL_ATTRIBUTES 6
    /**
     * Structure for holding a copy of a DF unit's soul
     * \ingroup grp_units
     */
    struct t_soul
    {
        uint8_t numSkills;
        t_skill skills[256];
        //uint8_t numLikes;
        //t_like likes[32];
        uint16_t traits[NUM_CREATURE_TRAITS];
        t_attrib analytical_ability;
        t_attrib focus;
        t_attrib willpower;
        t_attrib creativity;
        t_attrib intuition;
        t_attrib patience;
        t_attrib memory;
        t_attrib linguistic_ability;
        t_attrib spatial_sense;
        t_attrib musicality;
        t_attrib kinesthetic_sense;
        t_attrib empathy;
        t_attrib social_awareness;
    };
    #define MAX_COLORS  15
    struct df_unit;
    /**
     * Structure for holding a limited copy of a DF unit
     * \ingroup grp_units
     */
    struct t_unit
    {
        df::unit * origin;
        uint16_t x;
        uint16_t y;
        uint16_t z;
        uint32_t race;
        int32_t civ;

        df::unit_flags1 flags1;
        df::unit_flags2 flags2;
        df::unit_flags3 flags3;

        t_name name;

        int16_t mood;
        int16_t mood_skill;
        t_name artifact_name;

        uint8_t profession;
        std::string custom_profession;

        // enabled labors
        uint8_t labors[NUM_CREATURE_LABORS];
        t_job current_job;

        uint32_t happiness;
        uint32_t id;
        t_attrib strength;
        t_attrib agility;
        t_attrib toughness;
        t_attrib endurance;
        t_attrib recuperation;
        t_attrib disease_resistance;
        int32_t squad_leader_id;
        uint8_t sex;
        uint16_t caste;
        uint32_t pregnancy_timer; //Countdown timer to giving birth
        //bool has_default_soul;
        //t_soul defaultSoul;
        uint32_t nbcolors;
        uint32_t color[MAX_COLORS];

        int32_t birth_year;
        uint32_t birth_time;
    };

    /**
     * The Creatures module - allows reading all non-vermin creatures and their properties
     * \ingroup grp_modules
     * \ingroup grp_units
     */
    class DFHACK_EXPORT Units : public Module
    {
    public:
        Units();
        ~Units();
        bool Start( uint32_t & numCreatures );
        bool Finish();

        /* Read Functions */
        // Read creatures in a box, starting with index. Returns -1 if no more creatures
        // found. Call repeatedly do get all creatures in a specified box (uses tile coords)
        int32_t GetCreatureInBox(const int32_t index, df::unit ** furball,
            const uint16_t x1, const uint16_t y1,const uint16_t z1,
            const uint16_t x2, const uint16_t y2,const uint16_t z2);
        df::unit * GetCreature(const int32_t index);
        void CopyCreature(df::unit * source, t_unit & target);

        bool ReadJob(const df::unit * unit, std::vector<t_material> & mat);

        bool ReadInventoryByIdx(const uint32_t index, std::vector<df::item *> & item);
        bool ReadInventoryByPtr(const df::unit * unit, std::vector<df::item *> & item);

        bool ReadOwnedItemsByIdx(const uint32_t index, std::vector<int32_t> & item);
        bool ReadOwnedItemsByPtr(const df::unit * unit, std::vector<int32_t> & item);

        int32_t FindIndexById(int32_t id);

        /* Getters */
        uint32_t GetDwarfRaceIndex ( void );
        int32_t GetDwarfCivId ( void );

        void CopyNameTo(df::unit *creature, df::language_name * target);

        bool RemoveOwnedItemByIdx(const uint32_t index, int32_t id);
        bool RemoveOwnedItemByPtr(df::unit * unit, int32_t id);

    private:
        struct Private;
        Private *d;
    };
}
#endif
