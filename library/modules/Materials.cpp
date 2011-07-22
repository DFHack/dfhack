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


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
#include <cstring>
using namespace std;

#include "dfhack/Types.h"
#include "dfhack/modules/Materials.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/Process.h"
#include "dfhack/Vector.h"
#include <dfhack/Error.h>
#include "ModuleFactory.h"
#include <dfhack/Core.h>

using namespace DFHack;

////////////////////////////
// START materials flag code
////////////////////////////

struct t_matFlagInfo
{
    uint32_t offset;
    uint32_t mask;
};

enum t_matFlagType
{
    MAT_FLAG_GEM,
    NUM_MAT_FLAGS
};

static bool hasFlag(t_matFlagType flag_type, vector<uint32_t> flags)
{
    // More flags can be found at
    // http://www.qmtpro.com/~quietust/df/material_flags.txt
    static t_matFlagInfo mat_flag_info[NUM_MAT_FLAGS] =
    {
        {1, 0x10000} // MAT_FLAG_GEM
    };

    t_matFlagInfo &info = mat_flag_info[flag_type];

    if ( info.offset > flags.size() )
        return false;

    return ( (flags[info.offset] & info.mask) == info.mask );
}

//////////////////////////
// END materials flag code
//////////////////////////

Module* DFHack::createMaterials()
{
    return new Materials();
}

class Materials::Private
{
    public:
    Process * owner;
    OffsetGroup * OG_Materials;
    uint32_t vector_organic_all;
    uint32_t vector_organic_plants;
    uint32_t vector_organic_trees;
    uint32_t vector_races;
    uint32_t vector_other;
};

Materials::Materials()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->owner = c.p;
    OffsetGroup *OG_Materials = d->OG_Materials = c.vinfo->getGroup("Materials");
    {
        df_inorganic = (vector <df_inorganic_material *> *) OG_Materials->getAddress("inorganics");
        d->vector_organic_all = OG_Materials->getAddress ("organics_all");
        d->vector_organic_plants = OG_Materials->getAddress ("organics_plants");
        d->vector_organic_trees = OG_Materials->getAddress ("organics_trees");
        d->vector_races = OG_Materials->getAddress("creature_type_vector");
    }
}

Materials::~Materials()
{
    delete d;
}

bool Materials::Finish()
{
    return true;
}

/*
bool API::ReadInorganicMaterials (vector<t_matgloss> & inorganic)
{
    Process *p = d->owner;
    memory_info * minfo = p->getDescriptor();
    int matgloss_address = minfo->getAddress ("mat_inorganics");
    int matgloss_colors = minfo->getOffset ("material_color");
    int matgloss_stone_name_offset = minfo->getOffset("matgloss_stone_name");

    DfVector <uint32_t> p_matgloss (p, matgloss_address);

    uint32_t size = p_matgloss.getSize();
    inorganic.resize (0);
    inorganic.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = p_matgloss[i];
        // read the string pointed at by
        t_matgloss mat;
        //cout << temp << endl;
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        p->readSTLString (temp, mat.id, 128);

        p->readSTLString (temp+matgloss_stone_name_offset, mat.name, 128);
        mat.fore = (uint8_t) p->readWord (temp + matgloss_colors);
        mat.back = (uint8_t) p->readWord (temp + matgloss_colors + 2);
        mat.bright = (uint8_t) p->readWord (temp + matgloss_colors + 4);

        inorganic.push_back (mat);
    }
    return true;
}
*/

t_matgloss::t_matgloss()
{
    fore    = 0;
    back    = 0;
    bright  = 0;

    value        = 0;
    wall_tile    = 0;
    boulder_tile = 0;
}

t_matglossInorganic::t_matglossInorganic()
{
}

// NOTE: There doesn't appear to be an IS_ORE materials flag, so we have to
// do it this way.
bool t_matglossInorganic::isOre()
{
    if (!ore_chances.empty())
    {
        if (ore_chances[0] > 0)
            return true;
    }

    if (!strand_chances.empty())
    {
        if (strand_chances[0] > 0)
            return true;
    }

    return false;
}

bool t_matglossInorganic::isGem()
{
    return hasFlag(MAT_FLAG_GEM, flags);
}

// good for now
inline bool ReadNamesOnly(Process* p, uint32_t address, vector<t_matgloss> & names)
{
    DfVector <uint32_t> p_matgloss (address);
    uint32_t size = p_matgloss.size();
    names.clear();
    names.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        t_matgloss mat;
        mat.id = *(std::string *)p_matgloss[i];
        //p->readSTLString (p_matgloss[i], mat.id, 128);
        names.push_back(mat);
    }
    return true;
}

bool Materials::ReadInorganicMaterials (void)
{
    Process * p = d->owner;
    uint32_t size = df_inorganic->size();
    inorganic.clear();
    inorganic.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        df_inorganic_material * orig = df_inorganic->at(i);
        t_matglossInorganic mat;
        mat.id = orig->Inorganic_ID;
        mat.name = orig->STONE_NAME;
        //p->readSTLString (p_matgloss[i] + mat_name, mat.name, 128);

        mat.ore_types = orig->METAL_ORE_matID;
        mat.ore_chances = orig->METAL_ORE_prob;
        mat.strand_types = orig->THREAD_METAL_matID;
        mat.strand_chances = orig->THREAD_METAL_prob;
        mat.value = orig->MATERIAL_VALUE;
        mat.wall_tile = orig->TILE;
        mat.boulder_tile = orig->ITEM_SYMBOL;
        mat.bright = orig->BASIC_COLOR_bright;
        mat.fore = orig->BASIC_COLOR_foreground;

        if (orig->flagarray_properties != NULL)
        {
            for (uint32_t j = 0; j < orig->flagarray_properties_length; j++)
            {
                uint32_t *ptr = orig->flagarray_properties + j;
                mat.flags.push_back( *ptr );
            }
        }

        inorganic.push_back(mat);
    }
    return true;
}

bool Materials::ReadOrganicMaterials (void)
{
    return ReadNamesOnly(d->owner, d->vector_organic_all, organic );
}

bool Materials::ReadWoodMaterials (void)
{
    return ReadNamesOnly(d->owner, d->vector_organic_trees, tree );
}

bool Materials::ReadPlantMaterials (void)
{
    return ReadNamesOnly(d->owner, d->vector_organic_plants, plant );
}

bool Materials::ReadCreatureTypes (void)
{
    return ReadNamesOnly(d->owner, d->vector_races, race );
    return true;
}

bool Materials::ReadOthers(void)
{
    Process * p = d->owner;
    uint32_t matBase = d->OG_Materials->getAddress ("other");
    uint32_t i = 0;
    std::string * ptr;

    other.clear();

    while(1)
    {
        t_matglossOther mat;
        ptr = (std::string *) p->readDWord(matBase + i*4);
        if(ptr==0)
            break;
        mat.id = *ptr;
        other.push_back(mat);
        i++;
    }
    return true;
}

bool Materials::ReadDescriptorColors (void)
{
    Process * p = d->owner;
    OffsetGroup * OG_Descriptors = p->getDescriptor()->getGroup("Materials")->getGroup("descriptors");
    DfVector <uint32_t> p_colors (OG_Descriptors->getAddress ("colors_vector"));
    uint32_t size = p_colors.size();

    color.clear();
    if(size == 0)
        return false;
    color.reserve(size);
    for (uint32_t i = 0; i < size;i++)
    {
        t_descriptor_color col;
        col.id = p->readSTLString (p_colors[i] + OG_Descriptors->getOffset ("rawname") );
        col.name = p->readSTLString (p_colors[i] + OG_Descriptors->getOffset ("name") );
        col.red = p->readFloat( p_colors[i] + OG_Descriptors->getOffset ("color_r") );
        col.green = p->readFloat( p_colors[i] + OG_Descriptors->getOffset ("color_v") );
        col.blue = p->readFloat( p_colors[i] + OG_Descriptors->getOffset ("color_b") );
        color.push_back(col);
    }
    return ReadNamesOnly(d->owner, OG_Descriptors->getAddress ("all_colors_vector"), alldesc );
    return true;
}

bool Materials::ReadCreatureTypesEx (void)
{
    Process *p = d->owner;
    VersionInfo *mem = p->getDescriptor();
    OffsetGroup * OG_string = mem->getGroup("string");
    uint32_t sizeof_string = OG_string->getHexValue ("sizeof");

    OffsetGroup * OG_Mats = mem->getGroup("Materials");
    DfVector <uint32_t> p_races (OG_Mats->getAddress ("creature_type_vector"));

    OffsetGroup * OG_Creature = OG_Mats->getGroup("creature");
        uint32_t castes_vector_offset = OG_Creature->getOffset ("caste_vector");
        uint32_t extract_vector_offset = OG_Creature->getOffset ("extract_vector");
        uint32_t tile_offset = OG_Creature->getOffset ("tile");
        uint32_t tile_color_offset = OG_Creature->getOffset ("tile_color");

    bool have_advanced = false;
    uint32_t caste_colormod_offset;
    uint32_t caste_attributes_offset;
    uint32_t caste_bodypart_offset;
    uint32_t bodypart_id_offset;
    uint32_t bodypart_category_offset;
    //FIXME: this is unused. why do we mess with it when it's not even read?
    //uint32_t bodypart_layers_offset;
    //uint32_t bodypart_singular_offset;
    //uint32_t bodypart_plural_offset;
    uint32_t color_modifier_part_offset;
    uint32_t color_modifier_startdate_offset;
    uint32_t color_modifier_enddate_offset;
    try
    {
        OffsetGroup * OG_Caste = OG_Creature->getGroup("caste");
            caste_colormod_offset = OG_Caste->getOffset ("color_modifiers");
            caste_attributes_offset = OG_Caste->getOffset ("attributes");
            caste_bodypart_offset = OG_Caste->getOffset ("bodypart_vector");
        OffsetGroup * OG_CasteBodyparts = OG_Creature->getGroup("caste_bodyparts");
            bodypart_id_offset = OG_CasteBodyparts->getOffset ("id");
            bodypart_category_offset = OG_CasteBodyparts->getOffset ("category");
            //bodypart_layers_offset = OG_CasteBodyparts->getOffset ("layers_vector"); // unused
            //bodypart_singular_offset = OG_CasteBodyparts->getOffset ("singular_vector"); // unused
            //bodypart_plural_offset = OG_CasteBodyparts->getOffset ("plural_vector"); // unused
        OffsetGroup * OG_CasteColorMods = OG_Creature->getGroup("caste_color_mods");
            color_modifier_part_offset = OG_CasteColorMods->getOffset ("part");
            color_modifier_startdate_offset = OG_CasteColorMods->getOffset ("startdate");
            color_modifier_enddate_offset = OG_CasteColorMods->getOffset ("enddate");
        have_advanced = true;
    }
    catch (Error::All &){};

    uint32_t size = p_races.size();
    uint32_t sizecas = 0;
    uint32_t sizecolormod;
    uint32_t sizecolorlist;
    uint32_t sizebp;
    raceEx.clear();
    raceEx.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        t_creaturetype mat;
        // FROM race READ
        // std::string rawname AT 0,
        // char tile_character AT tile_offset,
        // word tilecolor.fore : tile_color_offset,
        // word tilecolor.back : tile_color_offset + 2,
        // word tilecolor.bright : tile_color_offset + 4
        mat.id = p->readSTLString (p_races[i]);
        mat.tile_character = p->readByte( p_races[i] + tile_offset );
        mat.tilecolor.fore = p->readWord( p_races[i] + tile_color_offset );
        mat.tilecolor.back = p->readWord( p_races[i] + tile_color_offset + 2 );
        mat.tilecolor.bright = p->readWord( p_races[i] + tile_color_offset + 4 );

        DfVector <uint32_t> p_castes(p_races[i] + castes_vector_offset);
        sizecas = p_castes.size();
        for (uint32_t j = 0; j < sizecas;j++)
        {
            /* caste name */
            t_creaturecaste caste;
            uint32_t caste_start = p_castes[j];
            caste.id = p->readSTLString (caste_start);
            caste.singular = p->readSTLString (caste_start + sizeof_string);
            caste.plural = p->readSTLString (caste_start + 2 * sizeof_string);
            caste.adjective = p->readSTLString (caste_start + 3 * sizeof_string);
            //cout << "Caste " << caste.rawname << " " << caste.singular << ": 0x" << hex << caste_start << endl;
            if(have_advanced)
            {
                /* color mod reading */
                // Caste + offset > color mod vector
                DfVector <uint32_t> p_colormod(caste_start + caste_colormod_offset);
                sizecolormod = p_colormod.size();
                caste.ColorModifier.resize(sizecolormod);
                for(uint32_t k = 0; k < sizecolormod;k++)
                {
                    // color mod [0] -> color list
                    DfVector <uint32_t> p_colorlist(p_colormod[k]);
                    sizecolorlist = p_colorlist.size();
                    caste.ColorModifier[k].colorlist.resize(sizecolorlist);
                    for(uint32_t l = 0; l < sizecolorlist; l++)
                        caste.ColorModifier[k].colorlist[l] = p_colorlist[l];
                    // color mod [color_modifier_part_offset] = string part
                    caste.ColorModifier[k].part = p->readSTLString( p_colormod[k] + color_modifier_part_offset);
                    caste.ColorModifier[k].startdate = p->readDWord( p_colormod[k] + color_modifier_startdate_offset );
                    caste.ColorModifier[k].enddate = p->readDWord( p_colormod[k] + color_modifier_enddate_offset );
                }
                /* body parts */
                DfVector <uint32_t> p_bodypart(caste_start + caste_bodypart_offset);
                caste.bodypart.empty();
                sizebp = p_bodypart.size();
                for(uint32_t k = 0; k < sizebp; k++)
                {
                    t_bodypart part;
                    part.id = p->readSTLString (p_bodypart[k] + bodypart_id_offset);
                    part.category = p->readSTLString (p_bodypart[k] + bodypart_category_offset);
                    caste.bodypart.push_back(part);
                }
                p->read(caste_start + caste_attributes_offset, sizeof(t_attrib) * NUM_CREAT_ATTRIBS, (uint8_t *)&caste.strength);
            }
            else
            {
                memset(&caste.strength, 0,  sizeof(t_attrib) * NUM_CREAT_ATTRIBS);
            }
            mat.castes.push_back(caste);
        }
        DfVector <uint32_t> p_extract(p_races[i] + extract_vector_offset);
        for(uint32_t j = 0; j < p_extract.size(); j++)
        {
            t_creatureextract extract;
            extract.id = p->readSTLString( p_extract[j] );
            mat.extract.push_back(extract);
        }
        raceEx.push_back(mat);
    }
    return true;
}

void Materials::ReadAllMaterials(void)
{
    this->ReadInorganicMaterials();
    this->ReadOrganicMaterials();
    this->ReadWoodMaterials();
    this->ReadPlantMaterials();
    this->ReadCreatureTypes();
    this->ReadCreatureTypesEx();
    this->ReadDescriptorColors();
    this->ReadOthers();
}

/// miserable pile of magic. The material system is insane.
// FIXME: this contains potential errors when the indexes are -1 and compared to unsigned numbers!
std::string Materials::getDescription(const t_material & mat)
{
    std::string out;
    int32_t typeC;
    if ( (mat.subIndex<419) || (mat.subIndex>618) )
    {
        if ( (mat.subIndex<19) || (mat.subIndex>218) )
        {
            if (mat.subIndex)
                if (mat.subIndex>0x292)
                    return "?";
                else
                {
                    if (mat.subIndex>=this->other.size())
                    {
                        if (mat.itemType == 0) {
                            if(mat.subIndex<0)
                                return "any inorganic";
                            else
                                return this->inorganic[mat.subIndex].id;
                        }
                        if(mat.subIndex<0)
                            return "any";
                        if(mat.subIndex>=this->raceEx.size())
                            return "stuff";
                        return this->raceEx[mat.subIndex].id;
                    }
                    else
                    {
                        if (mat.index==-1)
                            return std::string(this->other[mat.subIndex].id);
                        else
                            return std::string(this->other[mat.subIndex].id) + " derivate";
                    }
                }
            else
                if(mat.index<0)
                    return "any inorganic";
                else
                    return this->inorganic[mat.index].id;
        }
        else
        {
            if (mat.index>=this->raceEx.size())
                return "unknown race";
            typeC = mat.subIndex;
            typeC -=19;
            if ((typeC<0) || (typeC>=this->raceEx[mat.index].extract.size()))
            {
                return string(this->raceEx[mat.index].id).append(" extract");
            }
            return std::string(this->raceEx[mat.index].id).append(" ").append(this->raceEx[mat.index].extract[typeC].id);
        }
    }
    else
    {
        return this->organic[mat.index].id;
    }
    return out;
}

//type of material only so we know which vector to retrieve
// FIXME: this also contains potential errors when the indexes are -1 and compared to unsigned numbers!
std::string Materials::getType(const t_material & mat)
{
    if((mat.subIndex<419) || (mat.subIndex>618))
    {
        if((mat.subIndex<19) || (mat.subIndex>218))
        {
            if(mat.subIndex)
            {
                if(mat.subIndex>0x292)
                {
                    return "unknown";
                }
                else
                {
                    if(mat.subIndex>=this->other.size())
                    {
                        if(mat.subIndex<0)
                            return "any";

                        if(mat.subIndex>=this->raceEx.size())
                            return "unknown";

                        return "racex";
                    }
                    else
                    {
                        if (mat.index==-1)
                            return "other";
                        else
                            return "other derivate";
                    }
                }
            }
            else
                return "inorganic";
        }
        else
        {
            if (mat.index>=this->raceEx.size())
                return "unknown";

            return "racex extract";
        }
    }
    else
    {
        return "organic";
    }
}
