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
#ifndef CL_MOD_MATERIALS
#define CL_MOD_MATERIALS
/**
 * \defgroup grp_materials Materials module - used for reading raws mostly
 * @ingroup grp_modules
 */
#include "dfhack/Export.h"
#include "dfhack/Module.h"
#include "dfhack/Types.h"

#include <vector>

namespace DFHack
{
    class DFContextShared;
    /**
     * \ingroup grp_materials
     */
    class DFHACK_EXPORT t_matgloss
    {
    public:
        char id[128]; //the id in the raws
        uint8_t fore; // Annoyingly the offset for this differs between types
        uint8_t back;
        uint8_t bright;
        char name[128]; //this is the name displayed ingame

        int32_t  value;        // Material value
        uint16_t wall_tile;    // Tile when a natural wall
        uint16_t boulder_tile; // Tile when a dug-out stone;

    public:
        t_matgloss();
    };

    class DFHACK_EXPORT t_matglossInorganic : public t_matgloss
    {
    public:
        // Types of metals the ore will produce when smelted.  Each number
        // is an index into the inorganic matglass vector.
        std::vector<uint16_t>* ore_types;

        // Percent chance that the ore will produce each type of metal
        // when smelted.
        std::vector<uint16_t>* ore_chances;

        // Types of metals the ore will produce from strand extraction.
        // Each number is an index into the inorganic matglass vector.
        std::vector<uint16_t>* strand_types;

        // Percent chance that the ore will produce each type of metal
        // fram strand extraction.
        std::vector<uint16_t>* strand_chances;

    public:
        t_matglossInorganic();

        bool isOre();
        bool isGem();
    };

    /**
     * \ingroup grp_materials
     */
    struct t_descriptor_color
    {
        char id[128]; // id in the raws
        float red;
        float green;
        float blue;
        char name[128]; //displayed name
    };
    /**
     * \ingroup grp_materials
     */
    struct t_matglossPlant
    {
        char id[128]; //the id in the raws
        uint8_t fore; // Annoyingly the offset for this differs between types
        uint8_t back;
        uint8_t bright;
        char name[128]; //this is the name displayed ingame
        char drink_name[128];  //the name this item becomes a drink
        char food_name[128];
        char extract_name[128];
    };
    /**
     * \ingroup grp_materials
     */
    struct t_bodypart
    {
        char id[128];
        char category[128];
        char single[128];
        char plural[128];
    };
    /**
     * \ingroup grp_materials
     */
    struct t_colormodifier
    {
        char part[128];
        std::vector<uint32_t> colorlist;
        uint32_t startdate; /* in days */
        uint32_t enddate; /* in days */
    };
    /**
     * \ingroup grp_materials
     */
    struct t_creaturecaste
    {
        char rawname[128];
        char singular[128];
        char plural[128];
        char adjective[128];
        std::vector<t_colormodifier> ColorModifier;
        std::vector<t_bodypart> bodypart;

        t_attrib strength;
        t_attrib agility;
        t_attrib toughness;
        t_attrib endurance;
        t_attrib recuperation;
        t_attrib disease_resistance;
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
    };
    #define NUM_CREAT_ATTRIBS 17

    /**
     * \ingroup grp_materials
     */
    struct t_matglossOther
    {
        char rawname[128];
    };
    /**
     * \ingroup grp_materials
     */
    struct t_creatureextract
    {
        char rawname[128];
    };
    /**
     * \ingroup grp_materials
     */
    struct t_creaturetype
    {
        char rawname[128];
        std::vector <t_creaturecaste> castes;
        std::vector <t_creatureextract> extract;
        uint8_t tile_character;
        struct
        {
            uint16_t fore;
            uint16_t back;
            uint16_t bright;
        } tilecolor;
    };

    /**
     * this structure describes what are things made of in the DF world
     * \ingroup grp_materials
     */
    struct t_material
    {
        int16_t itemType;
        int16_t subType;
        int16_t subIndex;
        int32_t index;
        uint32_t flags;
    };
    /**
     * The Materials module
     * \ingroup grp_modules
     * \ingroup grp_materials
     */
    class DFHACK_EXPORT Materials : public Module
    {
    public:
        Materials();
        ~Materials();
        bool Finish();

        std::vector<t_matglossInorganic> inorganic;
        std::vector<t_matgloss> organic;
        std::vector<t_matgloss> tree;
        std::vector<t_matgloss> plant;
        std::vector<t_matgloss> race;
        std::vector<t_creaturetype> raceEx;
        std::vector<t_descriptor_color> color;
        std::vector<t_matglossOther> other;
        std::vector<t_matgloss> alldesc;

        bool ReadInorganicMaterials (void);
        bool ReadOrganicMaterials (void);
        bool ReadWoodMaterials (void);
        bool ReadPlantMaterials (void);
        bool ReadCreatureTypes (void);
        bool ReadCreatureTypesEx (void);
        bool ReadDescriptorColors(void);
        bool ReadOthers (void);

        void ReadAllMaterials(void);

        std::string getType(const t_material & mat);
        std::string getDescription(const t_material & mat);
    private:
        class Private;
        Private* d;
    };
}
#endif

