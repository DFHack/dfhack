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

#include "Types.h"
#include "modules/Materials.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "Vector.h"
#include "Error.h"
#include "ModuleFactory.h"
#include "Core.h"

#include "MiscUtils.h"

#include "df/world.h"
#include "df/ui.h"
#include "df/item.h"
#include "df/inorganic_raw.h"
#include "df/plant_raw.h"
#include "df/plant_raw_flags.h"
#include "df/creature_raw.h"
#include "df/historical_figure.h"

#include "df/job_item.h"
#include "df/job_material_category.h"
#include "df/matter_state.h"
#include "df/material_vec_ref.h"

using namespace DFHack;
using namespace df::enums;

bool MaterialInfo::decode(df::item *item)
{
    if (!item)
        return decode(-1);
    else
        return decode(item->getActualMaterial(), item->getActualMaterialIndex());
}

bool MaterialInfo::decode(const df::material_vec_ref &vr, int idx)
{
    if (idx < 0 || idx >= vr.mat_type.size() || idx >= vr.mat_index.size())
        return decode(-1);
    else
        return decode(vr.mat_type[idx], vr.mat_index[idx]);
}

bool MaterialInfo::decode(int16_t type, int32_t index)
{
    this->type = type;
    this->index = index;

    material = NULL;
    mode = Builtin; subtype = 0;
    inorganic = NULL; plant = NULL; creature = NULL;
    figure = NULL;

    df::world_raws &raws = df::global::world->raws;

    if (type < 0 || type >= sizeof(raws.mat_table.builtin)/sizeof(void*))
        return false;

    if (index < 0)
    {
        material = raws.mat_table.builtin[type];
    }
    else if (type == 0)
    {
        mode = Inorganic;
        inorganic = df::inorganic_raw::find(index);
        if (!inorganic)
            return false;
        material = &inorganic->material;
    }
    else if (type < CREATURE_BASE)
    {
        material = raws.mat_table.builtin[type];
    }
    else if (type < FIGURE_BASE)
    {
        mode = Creature;
        subtype = type-CREATURE_BASE;
        creature = df::creature_raw::find(index);
        if (!creature || subtype >= creature->material.size())
            return false;
        material = creature->material[subtype];
    }
    else if (type < PLANT_BASE)
    {
        mode = Creature;
        subtype = type-FIGURE_BASE;
        figure = df::historical_figure::find(index);
        if (!figure)
            return false;
        creature = df::creature_raw::find(figure->race);
        if (!creature || subtype >= creature->material.size())
            return false;
        material = creature->material[subtype];
    }
    else if (type < END_BASE)
    {
        mode = Plant;
        subtype = type-PLANT_BASE;
        plant = df::plant_raw::find(index);
        if (!plant || subtype >= plant->material.size())
            return false;
        material = plant->material[subtype];
    }
    else
    {
        material = raws.mat_table.builtin[type];
    }

    return (material != NULL);
}

bool MaterialInfo::find(const std::string &token, const std::string &subtoken)
{
    if (findBuiltin(token))
        return true;
    if (subtoken.empty())
    {
        if (findInorganic(token))
            return true;
    }
    else
    {
        if (findPlant(token, subtoken))
            return true;
        if (findCreature(token, subtoken))
            return true;
    }
    return false;
}

bool MaterialInfo::findBuiltin(const std::string &token)
{
    df::world_raws &raws = df::global::world->raws;
    for (int i = 1; i < NUM_BUILTIN; i++)
        if (raws.mat_table.builtin[i]->id == token)
            return decode(i, -1);
    return decode(-1);
}

bool MaterialInfo::findInorganic(const std::string &token)
{
    df::world_raws &raws = df::global::world->raws;
    for (unsigned i = 0; i < raws.inorganics.size(); i++)
    {
        df::inorganic_raw *p = raws.inorganics[i];
        if (p->id == token)
            return decode(0, i);
    }
    return decode(-1);
}

bool MaterialInfo::findPlant(const std::string &token, const std::string &subtoken)
{
    df::world_raws &raws = df::global::world->raws;
    for (unsigned i = 0; i < raws.plants.all.size(); i++)
    {
        df::plant_raw *p = raws.plants.all[i];
        if (p->id != token)
            continue;

        for (unsigned j = 0; j < p->material.size(); j++)
            if (p->material[j]->id == subtoken)
                return decode(PLANT_BASE+j, i);

        break;
    }
    return decode(-1);
}

bool MaterialInfo::findCreature(const std::string &token, const std::string &subtoken)
{
    df::world_raws &raws = df::global::world->raws;
    for (unsigned i = 0; i < raws.creatures.all.size(); i++)
    {
        df::creature_raw *p = raws.creatures.all[i];
        if (p->creature_id != token)
            continue;

        for (unsigned j = 0; j < p->material.size(); j++)
            if (p->material[j]->id == subtoken)
                return decode(CREATURE_BASE+j, i);

        break;
    }
    return decode(-1);
}

std::string MaterialInfo::toString(uint16_t temp, bool named)
{
    if (type == -1)
        return "NONE";
    if (!material)
        return stl_sprintf("INVALID %d:%d", type, index);

    df::matter_state state = matter_state::Solid;
    if (temp >= material->heat.melting_point)
        state = matter_state::Liquid;
    if (temp >= material->heat.boiling_point)
        state = matter_state::Gas;

    std::string name = material->state_name[state];
    if (!material->prefix.empty())
        name = material->prefix + " " + name;

    if (named && figure)
        name += stl_sprintf(" of HF %d", index);
    return name;
}

df::craft_material_class MaterialInfo::getCraftClass()
{
    if (!material)
        return craft_material_class::None;

    if (type == 0 && index == -1)
        return craft_material_class::Stone;

    FOR_ENUM_ITEMS(material_flags, i)
    {
        df::craft_material_class ccv = ENUM_ATTR(material_flags, type, i);
        if (ccv == craft_material_class::None)
            continue;
        if (material->flags.is_set(i))
            return ccv;
    }

    return craft_material_class::None;
}

bool MaterialInfo::isAnyCloth()
{
    using namespace df::enums::material_flags;

    return material && (
        material->flags.is_set(THREAD_PLANT) ||
        material->flags.is_set(SILK) ||
        material->flags.is_set(YARN)
    );
}

bool MaterialInfo::matches(const df::job_material_category &cat)
{
    if (!material)
        return false;

    using namespace df::enums::material_flags;
#define TEST(bit,flag) if (cat.bits.bit && material->flags.is_set(flag)) return true;
    TEST(plant, STRUCTURAL_PLANT_MAT);
    TEST(wood, WOOD);
    TEST(cloth, THREAD_PLANT);
    TEST(silk, SILK);
    TEST(leather, LEATHER);
    TEST(bone, BONE);
    TEST(shell, SHELL);
    TEST(wood2, WOOD);
    TEST(soap, SOAP);
    TEST(tooth, TOOTH);
    TEST(horn, HORN);
    TEST(pearl, PEARL);
    TEST(yarn, YARN);
#undef TEST
    return false;
}

bool MaterialInfo::matches(const df::job_item &item)
{
    if (!isValid()) return false;

    df::job_item_flags1 ok1, mask1;
    getMatchBits(ok1, mask1);

    df::job_item_flags2 ok2, mask2;
    getMatchBits(ok2, mask2);

    df::job_item_flags3 ok3, mask3;
    getMatchBits(ok3, mask3);

    return ((item.flags1.whole & mask1.whole) == (item.flags1.whole & ok1.whole)) &&
           ((item.flags2.whole & mask2.whole) == (item.flags2.whole & ok2.whole)) &&
           ((item.flags3.whole & mask3.whole) == (item.flags3.whole & ok3.whole));
}

void MaterialInfo::getMatchBits(df::job_item_flags1 &ok, df::job_item_flags1 &mask)
{
    ok.whole = mask.whole = 0;
    if (!isValid()) return;

#define MAT_FLAG(name) material->flags.is_set(df::enums::material_flags::name)
#define FLAG(field, name) (field && field->flags.is_set(name))
#define TEST(bit, check) \
    mask.bits.bit = true; ok.bits.bit = !!(check);

    bool structural = MAT_FLAG(STRUCTURAL_PLANT_MAT);

    TEST(millable, structural && FLAG(plant, plant_raw_flags::MILL));
    TEST(sharpenable, MAT_FLAG(IS_STONE));
    TEST(distillable, structural && FLAG(plant, plant_raw_flags::DRINK));
    TEST(processable, structural && FLAG(plant, plant_raw_flags::THREAD));
    TEST(bag, isAnyCloth());
    TEST(cookable, MAT_FLAG(EDIBLE_COOKED));
    TEST(extract_bearing_plant, structural && FLAG(plant, plant_raw_flags::EXTRACT_STILL_VIAL));
    TEST(extract_bearing_fish, false);
    TEST(extract_bearing_vermin, false);
    TEST(processable_to_vial, structural && FLAG(plant, plant_raw_flags::EXTRACT_VIAL));
    TEST(processable_to_bag, structural && FLAG(plant, plant_raw_flags::LEAVES));
    TEST(processable_to_barrel, structural && FLAG(plant, plant_raw_flags::EXTRACT_BARREL));
    TEST(solid, !(MAT_FLAG(ALCOHOL_PLANT) ||
                  MAT_FLAG(ALCOHOL_CREATURE) ||
                  MAT_FLAG(LIQUID_MISC_PLANT) ||
                  MAT_FLAG(LIQUID_MISC_CREATURE) ||
                  MAT_FLAG(LIQUID_MISC_OTHER)));
    TEST(tameable_vermin, false);
    TEST(sharpenable, MAT_FLAG(IS_GLASS));
    TEST(milk, linear_index(material->reaction_product.id, std::string("CHEESE_MAT")) >= 0);
    //04000000 - "milkable" - vtable[107],1,1
}

void MaterialInfo::getMatchBits(df::job_item_flags2 &ok, df::job_item_flags2 &mask)
{
    ok.whole = mask.whole = 0;
    if (!isValid()) return;

    bool is_cloth = isAnyCloth();

    TEST(dye, MAT_FLAG(IS_DYE));
    TEST(dyeable, is_cloth);
    TEST(dyed, is_cloth);
    TEST(sewn_imageless, is_cloth);
    TEST(glass_making, MAT_FLAG(CRYSTAL_GLASSABLE));

    TEST(fire_safe, material->heat.melting_point > 11000);
    TEST(magma_safe, material->heat.melting_point > 12000);
    TEST(deep_material, FLAG(inorganic, df::enums::inorganic_flags::DEEP_ANY));
    TEST(non_economic, inorganic && !(df::global::ui && df::global::ui->economic_stone[index]));

    TEST(plant, plant);
    TEST(silk, MAT_FLAG(SILK));
    TEST(leather, MAT_FLAG(LEATHER));
    TEST(bone, MAT_FLAG(BONE));
    TEST(shell, MAT_FLAG(SHELL));
    TEST(totemable, false);
    TEST(horn, MAT_FLAG(HORN));
    TEST(pearl, MAT_FLAG(PEARL));
    TEST(soap, MAT_FLAG(SOAP));
    TEST(ivory_tooth, MAT_FLAG(TOOTH));
    //TEST(hair_wool, MAT_FLAG(YARN));
    TEST(yarn, MAT_FLAG(YARN));
}

void MaterialInfo::getMatchBits(df::job_item_flags3 &ok, df::job_item_flags3 &mask)
{
    ok.whole = mask.whole = 0;
    if (!isValid()) return;

    TEST(hard, MAT_FLAG(ITEMS_HARD));
}

#undef MAT_FLAG
#undef FLAG
#undef TEST

Module* DFHack::createMaterials()
{
    return new Materials();
}

class Materials::Private
{
    public:
    Process * owner;
    OffsetGroup * OG_Materials;
    uint32_t vector_races;
    uint32_t vector_other;
};

Materials::Materials()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->owner = c.p;
    df_organic = 0;
    df_trees = 0;
    df_plants = 0;
    df_inorganic = 0;
    OffsetGroup *OG_Materials = d->OG_Materials = c.vinfo->getGroup("Materials");
    {
        OG_Materials->getSafeAddress("inorganics",(uint32_t &)df_inorganic);
        OG_Materials->getSafeAddress("organics_all",(uint32_t &)df_organic);
        OG_Materials->getSafeAddress("organics_plants",(uint32_t &)df_plants);
        OG_Materials->getSafeAddress("organics_trees",(uint32_t &)df_trees);
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

t_matgloss::t_matgloss()
{
    fore    = 0;
    back    = 0;
    bright  = 0;

    value        = 0;
    wall_tile    = 0;
    boulder_tile = 0;
}

bool t_matglossInorganic::isOre()
{
    if (!ore_chances.empty())
        return true;
    if (!strand_chances.empty())
        return true;
    return false;
}

bool t_matglossInorganic::isGem()
{
    return is_gem;
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
        names.push_back(mat);
    }
    return true;
}

bool Materials::CopyInorganicMaterials (std::vector<t_matglossInorganic> & inorganic)
{
    Process * p = d->owner;
    if(!df_inorganic)
        return false;
    uint32_t size = df_inorganic->size();
    inorganic.clear();
    inorganic.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        df_inorganic_type * orig = df_inorganic->at(i);
        t_matglossInorganic mat;
        mat.id = orig->ID;
        mat.name = orig->mat.STONE_NAME;

        mat.ore_types = orig->METAL_ORE_matID;
        mat.ore_chances = orig->METAL_ORE_prob;
        mat.strand_types = orig->THREAD_METAL_matID;
        mat.strand_chances = orig->THREAD_METAL_prob;
        mat.value = orig->mat.MATERIAL_VALUE;
        mat.wall_tile = orig->mat.TILE;
        mat.boulder_tile = orig->mat.ITEM_SYMBOL;
        mat.bright = orig->mat.BASIC_COLOR_bright;
        mat.fore = orig->mat.BASIC_COLOR_foreground;
        mat.is_gem = orig->mat.mat_flags.is_set(MATERIAL_IS_GEM);
        inorganic.push_back(mat);
    }
    return true;
}

bool Materials::CopyOrganicMaterials (std::vector<t_matgloss> & organic)
{
    if(df_organic)
        return ReadNamesOnly(d->owner, (uint32_t) df_organic, organic );
    else return false;
}

bool Materials::CopyWoodMaterials (std::vector<t_matgloss> & tree)
{
    if(df_trees)
        return ReadNamesOnly(d->owner, (uint32_t) df_trees, tree );
    else return false;
}

bool Materials::CopyPlantMaterials (std::vector<t_matgloss> & plant)
{
    if(df_plants)
        return ReadNamesOnly(d->owner, (uint32_t) df_plants, plant );
    else return false;
}

bool Materials::ReadCreatureTypes (void)
{
    return ReadNamesOnly(d->owner, d->vector_races, race );
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

bool Materials::ReadAllMaterials(void)
{
    bool ok = true;
    ok &= this->ReadCreatureTypes();
    ok &= this->ReadCreatureTypesEx();
    ok &= this->ReadDescriptorColors();
    ok &= this->ReadOthers();
    return ok;
}

/// miserable pile of magic. The material system is insane.
// FIXME: this contains potential errors when the indexes are -1 and compared to unsigned numbers!
std::string Materials::getDescription(const t_material & mat)
{
    std::string out;
    int32_t typeC;
    if ( (mat.material<419) || (mat.material>618) )
    {
        if ( (mat.material<19) || (mat.material>218) )
        {
            if (mat.material)
                if (mat.material>0x292)
                    return "?";
                else
                {
                    if (mat.material>=this->other.size())
                    {
                        if (mat.itemType == 0) {
                            if(mat.material<0)
                                return "any inorganic";
                            else
                                return this->df_inorganic->at(mat.material)->ID;
                        }
                        if(mat.material<0)
                            return "any";
                        if(mat.material>=this->raceEx.size())
                            return "stuff";
                        return this->raceEx[mat.material].id;
                    }
                    else
                    {
                        if (mat.index==-1)
                            return std::string(this->other[mat.material].id);
                        else
                            return std::string(this->other[mat.material].id) + " derivate";
                    }
                }
            else
                if(mat.index<0)
                    return "any inorganic";
                else if (mat.index < df_inorganic->size())
                    return this->df_inorganic->at(mat.index)->ID;
                else
                    return "INDEX OUT OF BOUNDS!";
        }
        else
        {
            if (mat.index>=this->raceEx.size())
                return "unknown race";
            typeC = mat.material;
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
        return this->df_organic->at(mat.material)->ID;
    }
    return out;
}

//type of material only so we know which vector to retrieve
// FIXME: this also contains potential errors when the indexes are -1 and compared to unsigned numbers!
std::string Materials::getType(const t_material & mat)
{
    if((mat.material<419) || (mat.material>618))
    {
        if((mat.material<19) || (mat.material>218))
        {
            if(mat.material)
            {
                if(mat.material>0x292)
                {
                    return "unknown";
                }
                else
                {
                    if(mat.material>=this->other.size())
                    {
                        if(mat.material<0)
                            return "any";

                        if(mat.material>=this->raceEx.size())
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
