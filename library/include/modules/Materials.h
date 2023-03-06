/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include "Export.h"
#include "Module.h"
#include "Types.h"
#include "BitArray.h"

#include "DataDefs.h"
#include "df/material.h"
#include "df/inorganic_raw.h"
#include "df/plant_raw.h"

#include <vector>
#include <string>

namespace df
{
    struct item;
    struct plant_raw;
    struct creature_raw;
    struct historical_figure;
    struct material_vec_ref;
    struct job_item;

    union job_material_category;
    union dfhack_material_category;
    union job_item_flags1;
    union job_item_flags2;
    union job_item_flags3;
}

namespace DFHack
{
    struct t_matpair {
        int16_t mat_type;
        int32_t mat_index;

        t_matpair(int16_t type = -1, int32_t index = -1)
            : mat_type(type), mat_index(index) {}
    };

    struct DFHACK_EXPORT MaterialInfo {
        static const int NUM_BUILTIN = 19;
        static const int GROUP_SIZE = 200;
        static const int CREATURE_BASE = NUM_BUILTIN;
        static const int FIGURE_BASE = NUM_BUILTIN + GROUP_SIZE;
        static const int PLANT_BASE = NUM_BUILTIN + GROUP_SIZE*2;
        static const int END_BASE = NUM_BUILTIN + GROUP_SIZE*3;

        int16_t type;
        int32_t index;

        df::material *material;

        enum Mode {
            None,
            Builtin,
            Inorganic,
            Creature,
            Plant
        };
        Mode mode;

        int16_t subtype;
        df::inorganic_raw *inorganic;
        df::creature_raw *creature;
        df::plant_raw *plant;

        df::historical_figure *figure;

    public:
        MaterialInfo(int16_t type = -1, int32_t index = -1) { decode(type, index); }
        MaterialInfo(const t_matpair &mp) { decode(mp.mat_type, mp.mat_index); }
        template<class T> MaterialInfo(T *ptr) { decode(ptr); }

        bool isValid() const { return material != NULL; }

        bool isNone() const { return mode == None; }
        bool isBuiltin() const { return mode == Builtin; }
        bool isInorganic() const { return mode == Inorganic; }
        bool isCreature() const { return mode == Creature; }
        bool isPlant() const { return mode == Plant; }

        bool isAnyInorganic() const { return type == 0; }
        bool isInorganicWildcard() const { return isAnyInorganic() && isBuiltin(); }

        bool decode(int16_t type, int32_t index = -1);
        bool decode(df::item *item);
        bool decode(const df::material_vec_ref &vr, int idx);
        bool decode(const t_matpair &mp) { return decode(mp.mat_type, mp.mat_index); }

        template<class T> bool decode(T *ptr) {
            // Assume and exploit a certain naming convention
            return ptr ? decode(ptr->mat_type, ptr->mat_index) : decode(-1);
        }

        bool find(const std::string &token);
        bool find(const std::vector<std::string> &tokens);

        bool findBuiltin(const std::string &token);
        bool findInorganic(const std::string &token);
        bool findPlant(const std::string &token, const std::string &subtoken);
        bool findCreature(const std::string &token, const std::string &subtoken);

        bool findProduct(df::material *material, const std::string &name);
        bool findProduct(const MaterialInfo &info, const std::string &name) {
            return findProduct(info.material, name);
        }

        std::string getToken() const;
        std::string toString(uint16_t temp = 10015, bool named = true) const;

        bool isAnyCloth() const;

        void getMatchBits(df::job_item_flags1 &ok, df::job_item_flags1 &mask) const;
        void getMatchBits(df::job_item_flags2 &ok, df::job_item_flags2 &mask) const;
        void getMatchBits(df::job_item_flags3 &ok, df::job_item_flags3 &mask) const;

        df::craft_material_class getCraftClass();

        bool matches(const MaterialInfo &mat) const
        {
            if (!mat.isValid()) return true;
            return (type == mat.type) &&
                   (mat.index == -1 || index == mat.index);
        }

        bool matches(const df::job_material_category &cat) const;
        bool matches(const df::dfhack_material_category &cat) const;
        bool matches(const df::job_item &item,
                     df::item_type itype = df::item_type::NONE) const;
    };

    DFHACK_EXPORT bool parseJobMaterialCategory(df::job_material_category *cat, const std::string &token);
    DFHACK_EXPORT bool parseJobMaterialCategory(df::dfhack_material_category *cat, const std::string &token);

    inline bool operator== (const MaterialInfo &a, const MaterialInfo &b) {
        return a.type == b.type && a.index == b.index;
    }
    inline bool operator!= (const MaterialInfo &a, const MaterialInfo &b) {
        return a.type != b.type || a.index != b.index;
    }
    inline bool operator< (const MaterialInfo &a, const MaterialInfo &b) {
        return a.type < b.type || (a.type == b.type && a.index < b.index);
    }

    DFHACK_EXPORT bool isSoilInorganic(int material);
    DFHACK_EXPORT bool isStoneInorganic(int material);

    typedef int32_t t_materialIndex;
    typedef int16_t t_materialType, t_itemType, t_itemSubtype;

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

        int32_t strength[7];
        int32_t agility[7];
        int32_t toughness[7];
        int32_t endurance[7];
        int32_t recuperation[7];
        int32_t disease_resistance[7];
        int32_t analytical_ability[7];
        int32_t focus[7];
        int32_t willpower[7];
        int32_t creativity[7];
        int32_t intuition[7];
        int32_t patience[7];
        int32_t memory[7];
        int32_t linguistic_ability[7];
        int32_t spatial_sense[7];
        int32_t musicality[7];
        int32_t kinesthetic_sense[7];
        int32_t empathy[7];
        int32_t social_awareness[7];
    };

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
        t_itemType item_type;
        t_itemSubtype item_subtype;
        t_materialType mat_type;
        t_materialIndex mat_index;
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

        std::vector<t_matgloss> race;
        std::vector<t_creaturetype> raceEx;
        std::vector<t_descriptor_color> color;
        std::vector<t_matglossOther> other;
        std::vector<t_matgloss> alldesc;

        bool CopyInorganicMaterials (std::vector<t_matglossInorganic> & inorganic);
        bool CopyOrganicMaterials (std::vector<t_matgloss> & organic);
        bool CopyWoodMaterials (std::vector<t_matgloss> & tree);
        bool CopyPlantMaterials (std::vector<t_matgloss> & plant);

        bool ReadCreatureTypes (void);
        bool ReadCreatureTypesEx (void);
        bool ReadDescriptorColors(void);
        bool ReadOthers (void);

        bool ReadAllMaterials(void);

        std::string getType(const t_material & mat);
        std::string getDescription(const t_material & mat);
    };
}
#endif
