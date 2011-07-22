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
#include <string>

namespace DFHack
{
    struct t_syndrome
    {
        // it's lonely here...
    };
    /// Research by Quietust
    struct df_material
    {
        std::string Material_ID;
        std::string IS_GEM_singular;
        std::string IS_GEM_plural;
        std::string STONE_NAME;
        int16_t SPEC_HEAT;
        int16_t HEATDAM_POINT;
        int16_t COLDDAM_POINT;
        int16_t IGNITE_POINT;
        int16_t MELTING_POINT;
        int16_t BOILING_POINT;
        int16_t MAT_FIXED_TEMP;
        //int16_t padding; // added by compiler automatically
        int32_t SOLID_DENSITY;
        int32_t LIQUID_DENSITY;
        int32_t MOLAR_MASS;
        int32_t STATE_COLOR_SOLID;  // (color token index)
        int32_t STATE_COLOR_LIQUID; // (color token index)
        int32_t STATE_COLOR_GAS;    // (color token index)
        int32_t STATE_COLOR_POWDER; // (color token index)
        int32_t STATE_COLOR_PASTE;  // (color token index)
        int32_t STATE_COLOR_PRESSED;// (color token index)
        std::string STATE_NAME_SOLID;
        std::string STATE_NAME_LIQUID;
        std::string STATE_NAME_GAS;
        std::string STATE_NAME_POWDER;
        std::string STATE_NAME_PASTE;
        std::string STATE_NAME_PRESSED;
        std::string STATE_ADJ_SOLID;
        std::string STATE_ADJ_LIQUID;
        std::string STATE_ADJ_GAS;
        std::string STATE_ADJ_POWDER;
        std::string STATE_ADJ_PASTE;
        std::string STATE_ADJ_PRESSED;
        int32_t ABSORPTION;
        int32_t BENDING_YIELD;
        int32_t SHEAR_YIELD;
        int32_t TORSION_YIELD;
        int32_t IMPACT_YIELD;
        int32_t TENSILE_YIELD;
        int32_t COMPRESSIVE_YIELD;
        int32_t BENDING_FRACTURE;
        int32_t SHEAR_FRACTURE;
        int32_t TORSION_FRACTURE;
        int32_t IMPACT_FRACTURE;
        int32_t TENSILE_FRACTURE;
        int32_t COMPRESSIVE_FRACTURE;
        int32_t BENDING_STRAIN_AT_YIELD;
        int32_t SHEAR_STRAIN_AT_YIELD;
        int32_t TORSION_STRAIN_AT_YIELD;
        int32_t IMPACT_STRAIN_AT_YIELD;
        int32_t TENSILE_STRAIN_AT_YIELD;
        int32_t COMPRESSIVE_STRAIN_AT_YIELD;
        int32_t MAX_EDGE;
        int32_t MATERIAL_VALUE;
        uint32_t * flagarray_properties;
        uint32_t flagarray_properties_length;
        int16_t EXTRACT_STORAGE;// (item type)
        int16_t BUTCHER_SPECIAL_type;// (item type)
        int16_t BUTCHER_SPECIAL_subtype;// (item subtype)
        //int16_t padding; // added by compiler
        std::string MEAT_NAME_1st_parm; // (adj)
        std::string MEAT_NAME_2nd_parm;
        std::string MEAT_NAME_3rd_parm;
        std::string BLOCK_NAME_1st_parm;
        std::string BLOCK_NAME_2nd_parm;
        std::vector <std::string> MATERIAL_REACTION_PRODUCT_1st_parm;// (e.g. TAN_MAT)
        std::vector <void *>  unknown1;
        std::vector <void *>  unknown2;
        std::vector <std::string> MATERIAL_REACTION_PRODUCT_2nd_parm;// (e.g. LOCAL_CREATURE_MAT)
        std::vector <std::string> MATERIAL_REACTION_PRODUCT_3rd_parm;// (e.g. LEATHER)
        std::vector <std::string> MATERIAL_REACTION_PRODUCT_4th_parm;// (if you used CREATURE_MAT or PLANT_MAT)
        int16_t unknown3;
        //int16_t padding; // added by compiler
        int32_t unknown4;
        std::string HARDENS_WITH_WATER_1st_parm;// (e.g. INORGANIC)
        std::string HARDENS_WITH_WATER_2nd_parm;// (e.g. GYPSUM)
        std::string HARDENS_WITH_WATER_3rd_parm;// (if you used CREATURE_MAT or PLANT_MAT)
        std::vector <std::string> REACTION_CLASS;
        int8_t TILE;
        int16_t BASIC_COLOR_foreground;
        int16_t BASIC_COLOR_bright;
        // what exactly ARE those colors?
        int16_t BUILD_COLOR1;
        int16_t BUILD_COLOR2;
        int16_t BUILD_COLOR3;
        // same...
        int16_t TILE_COLOR1;
        int16_t TILE_COLOR2;
        int16_t TILE_COLOR3;
        int8_t ITEM_SYMBOL;
        int16_t POWDER_DYE; // (color token index)
        int16_t TEMP_DIET_INFO;// (whatever it means)
        std::vector <t_syndrome *> SYNDROME;
        int32_t SOAP_LEVEL;
        std::string PREFIX;
        // etc...
    };
    /// Research by Quietust
    struct df_inorganic_base
    {
        std::string Inorganic_ID;
        void * inorganic_flags;
        uint32_t inorganic_flags_length;
        std::vector <uint32_t> empty1;
        std::vector <int16_t> METAL_ORE_matID;
        std::vector <int16_t> METAL_ORE_prob;
        std::vector <uint32_t> empty2;
        std::vector <int16_t> THREAD_METAL_matID;
        std::vector <int16_t> THREAD_METAL_prob;
        std::vector <uint32_t> unknown_in_1;
        std::vector <uint32_t> empty3;
        std::vector <int16_t> ENVIRONMENT_SPEC_matID;
        std::vector <int16_t> ENVIRONMENT_SPEC_inclusion_type;
        std::vector <int8_t> ENVIRONMENT_SPEC_prob;
        std::vector <int16_t> ENVIRONMENT_location;
        std::vector <int16_t> ENVIRONMENT_inclusion_type;
        std::vector <int8_t> ENVIRONMENT_prob;
        int32_t unknown_in_2;
    };
    struct df_inorganic_material:public df_inorganic_base, public df_material {};
    /**
     * A copy of the game's material data.
     * \ingroup grp_materials
     */
    class DFHACK_EXPORT t_matgloss
    {
    public:
        std::string id; // raw name
        std::string name; // a sensible human-readable name
        uint8_t fore;
        uint8_t back;
        uint8_t bright;

        int32_t  value;        // Material value
        uint8_t wall_tile;    // Tile when a natural wall
        uint8_t boulder_tile; // Tile when a dug-out stone;

    public:
        t_matgloss();
    };
    /**
     * A copy of the game's inorganic material data.
     * \ingroup grp_materials
     */
    class DFHACK_EXPORT t_matglossInorganic : public t_matgloss
    {
    public:
        // Types of metals the ore will produce when smelted.  Each number
        // is an index into the inorganic matglass vector.
        std::vector<int16_t> ore_types;

        // Percent chance that the ore will produce each type of metal
        // when smelted.
        std::vector<int16_t> ore_chances;

        // Types of metals the ore will produce from strand extraction.
        // Each number is an index into the inorganic matglass vector.
        std::vector<int16_t> strand_types;

        // Percent chance that the ore will produce each type of metal
        // fram strand extraction.
        std::vector<int16_t> strand_chances;

    public:
        //t_matglossInorganic();

        bool isOre();
        bool isGem();
    };

    /**
     * \ingroup grp_materials
     */
    struct t_descriptor_color
    {
        std::string id;
        std::string name;
        float red;
        float green;
        float blue;
    };
    /**
     * \ingroup grp_materials
     */
    struct t_matglossPlant
    {
        std::string id;
        std::string name;
        uint8_t fore;
        uint8_t back;
        uint8_t bright;
        std::string drink_name;
        std::string food_name;
        std::string extract_name;
    };
    /**
     * \ingroup grp_materials
     */
    struct t_bodypart
    {
        std::string id;
        std::string category;
        std::string singular;
        std::string plural;
    };
    /**
     * \ingroup grp_materials
     */
    struct t_colormodifier
    {
        std::string part;
        std::vector<uint32_t> colorlist;
        uint32_t startdate; /* in days */
        uint32_t enddate; /* in days */
    };
    /**
     * \ingroup grp_materials
     */
    struct t_creaturecaste
    {
        std::string id;
        std::string singular;
        std::string plural;
        std::string adjective;
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
        std::string id;
    };
    /**
     * \ingroup grp_materials
     */
    struct t_creatureextract
    {
        std::string id;
    };
    /**
     * \ingroup grp_materials
     */
    struct t_creaturetype
    {
        std::string id;
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

        std::vector<df_inorganic_material*>* df_inorganic;
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

