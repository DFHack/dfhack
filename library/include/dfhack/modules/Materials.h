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
#include "dfhack/BitArray.h"

#include <vector>
#include <string>

namespace DFHack
{
    struct t_syndrome
    {
        // it's lonely here...
    };
    enum material_flags
    {
        MATERIAL_BONE = 0,
        MATERIAL_MEAT,
        MATERIAL_EDIBLE_VERMIN,
        MATERIAL_EDIBLE_RAW,
        MATERIAL_EDIBLE_COOKED,
        MATERIAL_UNK5,
        MATERIAL_ITEMS_METAL,
        MATERIAL_ITEMS_BARRED,
        
        MATERIAL_ITEMS_SCALED = 8,
        MATERIAL_ITEMS_LEATHER,
        MATERIAL_ITEMS_SOFT,
        MATERIAL_ITEMS_HARD,
        MATERIAL_IMPLIES_ANIMAL_KILL,
        MATERIAL_ALCOHOL_PLANT,
        MATERIAL_ALCOHOL_CREATURE,
        MATERIAL_CHEESE_PLANT,
        
        MATERIAL_CHEESE_CREATURE = 16,
        MATERIAL_POWDER_MISC_PLANT,
        MATERIAL_POWDER_MISC_CREATURE,
        MATERIAL_STOCKPILE_GLOB,
        MATERIAL_LIQUID_MISC_PLANT,
        MATERIAL_LIQUID_MISC_CREATURE,
        MATERIAL_LIQUID_MISC_OTHER,
        MATERIAL_WOOD,
        
        MATERIAL_THREAD_PLANT = 24,
        MATERIAL_TOOTH,
        MATERIAL_HORN,
        MATERIAL_PEARL,
        MATERIAL_SHELL,
        MATERIAL_LEATHER,
        MATERIAL_SILK,
        MATERIAL_SOAP,
        
        MATERIAL_ROTS = 32,
        MATERIAL_UNK33,
        MATERIAL_UNK34,
        MATERIAL_UNK35,
        MATERIAL_STRUCTURAL_PLANT_MAT,
        MATERIAL_SEED_MAT,
        MATERIAL_LEAF_MAT,
        MATERIAL_UNK39,
        
        MATERIAL_ENTERS_BLOOD = 40,
        MATERIAL_BLOOD_MAP_DESCRIPTOR,
        MATERIAL_ICHOR_MAP_DESCRIPTOR,
        MATERIAL_GOO_MAP_DESCRIPTOR,
        MATERIAL_SLIME_MAP_DESCRIPTOR,
        MATERIAL_PUS_MAP_DESCRIPTOR,
        MATERIAL_GENERATES_MIASMA,
        MATERIAL_IS_METAL,
        
        MATERIAL_IS_GEM = 48,
        MATERIAL_IS_GLASS,
        MATERIAL_CRYSTAL_GLASSABLE,
        MATERIAL_ITEMS_WEAPON,
        MATERIAL_ITEMS_WEAPON_RANGED,
        MATERIAL_ITEMS_ANVIL,
        MATERIAL_ITEMS_AMMO,
        MATERIAL_ITEMS_DIGGER,
        
        MATERIAL_ITEMS_ARMOR = 56,
        MATERIAL_ITEMS_DELICATE,
        MATERIAL_ITEMS_SIEGE_ENGINE,
        MATERIAL_ITEMS_QUERN,
        MATERIAL_IS_STONE,
        MATERIAL_UNDIGGABLE,
        MATERIAL_YARN,
        MATERIAL_STOCKPILE_GLOB_PASTE,
        
        MATERIAL_STOCKPILE_GLOB_PRESSED = 64,
        MATERIAL_DISPLAY_UNGLAZED,
        MATERIAL_DO_NOT_CLEAN_GLOB,
        MATERIAL_NO_STONE_STOCKPILE,
        MATERIAL_STOCKPILE_THREAD_METAL,
        MATERIAL_UNK69,
        MATERIAL_UNK70,
        MATERIAL_UNK71,
        
        MATERIAL_UNK72 = 72,
        MATERIAL_UNK73,
        MATERIAL_UNK74,
        MATERIAL_UNK75,
        MATERIAL_UNK76,
        MATERIAL_UNK77,
        MATERIAL_UNK78,
        MATERIAL_UNK79,
    };
    enum inorganic_flags
    {
        INORGANIC_LAVA = 0,
        INORGANIC_UNK1,
        INORGANIC_UNK2,
        INORGANIC_SEDIMENTARY,
        INORGANIC_SEDIMENTARY_OCEAN_SHALLOW,
        INORGANIC_IGNEOUS_INTRUSIVE,
        INORGANIC_IGNEOUS_EXTRUSIVE,
        INORGANIC_METAMORPHIC,

        INORGANIC_DEEP_SURFACE = 8,
        INORGANIC_METAL_ORE, // maybe
        INORGANIC_AQUIFER,
        INORGANIC_SOIL,
        INORGANIC_SOIL_OCEAN,
        INORGANIC_SOIL_SAND,
        INORGANIC_SEDIMENTARY_OCEAN_DEEP,
        INORGANIC_THREAD_METAL, // maybe

        INORGANIC_DEEP = 16, // in general
        INORGANIC_SOIL2, // more soil?
        INORGANIC_DEEP_SPECIAL,
        INORGANIC_UNK19,
        INORGANIC_UNK20,
        INORGANIC_UNK21,
        INORGANIC_UNK22,
        INORGANIC_UNK23,

        INORGANIC_UNK24 = 24,
        INORGANIC_WAFERS,
        INORGANIC_UNK26,
        INORGANIC_UNK27,
        INORGANIC_UNK28,
        INORGANIC_UNK29,
        INORGANIC_UNK30,
        INORGANIC_UNK31,
    };
    //Environment locations:
    enum environment_location
    {
        ENV_SOIL,
        ENV_SOIL_OCEAN,
        ENV_SOIL_SAND,
        ENV_METAMORPHIC,
        ENV_SEDIMENTARY,
        ENV_IGNEOUS_INTRUSIVE,
        ENV_IGNEOUS_EXTRUSIVE,
        ENV_ALLUVIAL,
    };
    //Inclusion types:
    enum inclusion_type
    {
        INCLUSION_NONE, // maybe
        INCLUSION_VEIN,
        INCLUSION_CLUSTER,
        INCLUSION_CLUSTER_SMALL,
        INCLUSION_CLUSTER_ONE,
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
        BitArray <material_flags> mat_flags;
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
        int8_t TILE; // Tile when material is a natural wall
        int16_t BASIC_COLOR_foreground;
        int16_t BASIC_COLOR_bright;
        // what exactly ARE those colors?
        int16_t BUILD_COLOR_foreground;
        int16_t BUILD_COLOR_background;
        int16_t BUILD_COLOR_bright;
        // same...
        int16_t TILE_COLOR_foreground;
        int16_t TILE_COLOR_background;
        int16_t TILE_COLOR_bright;
        int8_t ITEM_SYMBOL; // Tile when material is a dug out stone
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
        BitArray<inorganic_flags> inorg_flags;
        std::vector <uint32_t> empty1;
        std::vector <int16_t> METAL_ORE_matID; // Vector of indexes of metals produced when ore is smelted
        std::vector <int16_t> METAL_ORE_prob; // Vector of percent chance of each type of metal being produced on smelting
        std::vector <uint32_t> empty2;
        std::vector <int16_t> THREAD_METAL_matID;// Vector of indexes of metals produced when ore undergoes strand extraction
        std::vector <int16_t> THREAD_METAL_prob; // Vector of percent chance of each type of metal being produced on strand extraction
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
        bool is_gem;

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
     * The plant flags
     * \ingroup grp_materials
     */
    enum plant_flags
    {
        // byte 0
        PLANT_SPRING,
        PLANT_SUMMER,
        PLANT_AUTUMN,
        PLANT_WINTER,
        PLANT_UNK1,
        PLANT_SEED,
        PLANT_LEAVES,
        PLANT_DRINK,
        // byte 1
        PLANT_EXTRACT_BARREL,
        PLANT_EXTRACT_VIAL,
        PLANT_EXTRACT_STILL_VIAL,
        PLANT_UNK2,
        PLANT_THREAD,
        PLANT_MILL,
        PLANT_UNK3,
        PLANT_UNK4,
        // byte 2
        PLANT_UNK5,
        PLANT_UNK6,
        PLANT_UNK7,
        PLANT_UNK8,
        PLANT_WET,
        PLANT_DRY,
        PLANT_BIOME_MOUNTAIN,
        PLANT_BIOME_GLACIER,
        // byte 3
        PLANT_BIOME_TUNDRA,
        PLANT_BIOME_SWAMP_TEMPERATE_FRESHWATER,
        PLANT_BIOME_SWAMP_TEMPERATE_SALTWATER,
        PLANT_BIOME_MARSH_TEMPERATE_FRESHWATER,
        PLANT_BIOME_MARSH_TEMPERATE_SALTWATER,
        PLANT_BIOME_SWAMP_TROPICAL_FRESHWATER,
        PLANT_BIOME_SWAMP_TROPICAL_SALTWATER,
        PLANT_BIOME_SWAMP_MANGROVE,
        // byte 4
        PLANT_BIOME_MARSH_TROPICAL_FRESHWATER,
        PLANT_BIOME_MARSH_TROPICAL_SALTWATER,
        PLANT_BIOME_FOREST_TAIGA,
        PLANT_BIOME_FOREST_TEMPERATE_CONIFER,
        PLANT_BIOME_FOREST_TEMPERATE_BROADLEAF,
        PLANT_BIOME_FOREST_TROPICAL_CONIFER,
        PLANT_BIOME_FOREST_TROPICAL_DRY_BROADLEAF,
        PLANT_BIOME_FOREST_TROPICAL_MOIST_BROADLEAF,
        // byte 5
        PLANT_BIOME_GRASSLAND_TEMPERATE,
        PLANT_BIOME_SAVANNA_TEMPERATE,
        PLANT_BIOME_SHRUBLAND_TEMPERATE,
        PLANT_BIOME_GRASSLAND_TROPICAL,
        PLANT_BIOME_SAVANNA_TROPICAL,
        PLANT_BIOME_SHRUBLAND_TROPICAL,
        PLANT_BIOME_DESERT_BADLAND,
        PLANT_BIOME_DESERT_ROCK,
        // byte 6
        PLANT_BIOME_DESERT_SAND,
        PLANT_BIOME_OCEAN_TROPICAL,
        PLANT_BIOME_OCEAN_TEMPERATE,
        PLANT_BIOME_OCEAN_ARCTIC,
        PLANT_BIOME_POOL_TEMPERATE_FRESHWATER,
        PLANT_BIOME_SUBTERRANEAN_WATER,
        PLANT_BIOME_SUBTERRANEAN_CHASM,
        PLANT_BIOME_SUBTERRANEAN_LAVA,
        // byte 7
        PLANT_GOOD,
        PLANT_EVIL,
        PLANT_SAVAGE,
        PLANT_BIOME_POOL_TEMPERATE_BRACKISHWATER,
        PLANT_BIOME_POOL_TEMPERATE_SALTWATER,
        PLANT_BIOME_POOL_TROPICAL_FRESHWATER,
        PLANT_BIOME_POOL_TROPICAL_BRACKISHWATER,
        PLANT_BIOME_POOL_TROPICAL_SALTWATER,
        // byte 8
        PLANT_BIOME_LAKE_TEMPERATE_FRESHWATER,
        PLANT_BIOME_LAKE_TEMPERATE_BRACKISHWATER,
        PLANT_BIOME_LAKE_TEMPERATE_SALTWATER,
        PLANT_BIOME_LAKE_TROPICAL_FRESHWATER,
        PLANT_BIOME_LAKE_TROPICAL_BRACKISHWATER,
        PLANT_BIOME_LAKE_TROPICAL_SALTWATER,
        PLANT_BIOME_RIVER_TEMPERATE_FRESHWATER,
        PLANT_BIOME_RIVER_TEMPERATE_BRACKISHWATER,
        // byte 9
        PLANT_BIOME_RIVER_TEMPERATE_SALTWATER,
        PLANT_BIOME_RIVER_TROPICAL_FRESHWATER,
        PLANT_BIOME_RIVER_TROPICAL_BRACKISHWATER,
        PLANT_BIOME_RIVER_TROPICAL_SALTWATER,
        PLANT_AUTUMNCOLOR,
        PLANT_SAPLING,
        PLANT_TREE,
        PLANT_GRASS,
    };
    /**
     * The plant RAWs in memory
     * \ingroup grp_materials
     */
    struct df_plant_type
    {
        std::string ID;
        BitArray <plant_flags> flags;
        std::string name, name_plural, adj;
        std::string seed_singular, seed_plural;
        std::string leaves_singular, leaves_plural;
        uint8_t unk1;
        uint8_t unk2;
        char picked_tile, dead_picked_tile;
        char shrub_tile, dead_shrub_tile;
        char leaves_tile;
        char tree_tile, dead_tree_tile;
        char sapling_tile, dead_sapling_tile;
        char grass_tiles[16];
        char alt_grass_tiles[12];
        int32_t growdur;
        int32_t value;
        int8_t picked_color[3], dead_picked_color[3];
        int8_t shrub_color[3], dead_shrub_color[3];
        int8_t seed_color[3];
        int8_t leaves_color[3], dead_leaves_color[3];
        int8_t tree_color[3], dead_tree_color[3];
        int8_t sapling_color[3], dead_sapling_color[3];
        int8_t grass_colors_0[20], grass_colors_1[20], grass_colors_2[20];
        int32_t alt_period[2];
        int8_t shrub_drown_level;
        int8_t tree_drown_level;
        int8_t sapling_drown_level;
        int16_t frequency;
        int16_t clustersize;
        std::vector<std::string *> prefstring;
        std::vector<df_material *> materials;
        // materials and material indexes - only valid when appropriate flags in the BitArray are set
        int16_t material_type_basic_mat;
        int16_t material_type_tree;
        int16_t material_type_drink;
        int16_t material_type_seed;
        int16_t material_type_thread;
        int16_t material_type_mill;
        int16_t material_type_extract_vial;
        int16_t material_type_extract_barrel;
        int16_t material_type_extract_still_vial;
        int16_t material_type_leaves;
        int32_t material_idx_basic_mat;
        int32_t material_idx_tree;
        int32_t material_idx_drink;
        int32_t material_idx_seed;
        int32_t material_idx_thread;
        int32_t material_idx_mill;
        int32_t material_idx_extract_vial;
        int32_t material_idx_extract_barrel;
        int32_t material_idx_extract_still_vial;
        int32_t material_idx_leaves;
        std::string material_str_basic_mat[3];
        std::string material_str_tree[3];
        std::string material_str_drink[3];
        std::string material_str_seed[3];
        std::string material_str_thread[3];
        std::string material_str_mill[3];
        std::string material_str_extract_vial[3];
        std::string material_str_extract_barrel[3];
        std::string material_str_extract_still_vial[3];
        std::string material_str_leaves[3];
        int32_t underground_depth[2];
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

    typedef int32_t t_materialIndex;
    typedef int16_t t_materialType, t_itemType, t_itemSubtype;

    /**
     * this structure describes what are things made of in the DF world
     * \ingroup grp_materials
     */
    struct t_material
    {
        t_itemType itemType;
        t_itemSubtype subType;
        t_materialType material;
        t_materialIndex index;
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

        enum base_material
        {
            INORGANIC,
            AMBER,
            CORAL,
            GLASS_GREEN,
            GLASS_CLEAR,
            GLASS_CRYSTAL,
            WATER,
            COAL,
            POTASH,
            ASH,
            PEARLASH,
            LYE,
            MUD,
            VOMIT,
            SALT,
            FILTH_B,
            FILTH_Y,
            UNKNOWN_SUBSTANCE,
            GRIME
        };

        std::vector<df_inorganic_material*>* df_inorganic;
        std::vector<t_matglossInorganic> inorganic;
        std::vector<df_plant_type*>* df_organic;
        std::vector<t_matgloss> organic;
        std::vector<df_plant_type*>* df_trees;
        std::vector<t_matgloss> tree;
        std::vector<df_plant_type*>* df_plants;
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

        bool ReadAllMaterials(void);

        std::string getType(const t_material & mat);
        std::string getDescription(const t_material & mat);
    private:
        class Private;
        Private* d;
    };
}
#endif

