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

#include "Internal.h"
#include "Types.h"
#include "modules/Materials.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "Error.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "MiscUtils.h"

#include "df/plotinfost.h"
#include "df/item.h"
#include "df/builtin_mats.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/body_part_raw.h"
#include "df/historical_figure.h"
#include "df/inorganic_raw.h"
#include "df/job_item.h"
#include "df/job_material_category.h"
#include "df/dfhack_material_category.h"
#include "df/matter_state.h"
#include "df/material.h"
#include "df/material_vec_ref.h"
#include "df/descriptor_color.h"
#include "df/descriptor_pattern.h"
#include "df/descriptor_shape.h"
#include "df/physical_attribute_type.h"
#include "df/plant_raw.h"
#include "df/mental_attribute_type.h"
#include "df/color_modifier_raw.h"
#include "df/world.h"

#include <string>
#include <vector>
#include <map>
#include <cstring>

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::plotinfo;

bool MaterialInfo::decode(df::item* item)
{
    if (!item)
        return decode(-1);
    else
        return decode(item->getActualMaterial(), item->getActualMaterialIndex());
}

bool MaterialInfo::decode(const df::material_vec_ref& vr, int idx)
{
    if (size_t(idx) >= vr.mat_type.size() || size_t(idx) >= vr.mat_index.size())
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

    if (type < 0)
    {
        mode = None;
        return false;
    }

    auto& raws = world->raws;

    if (size_t(type) >= sizeof(raws.mat_table.builtin) / sizeof(void*))
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
        subtype = type - CREATURE_BASE;
        creature = df::creature_raw::find(index);
        if (!creature || size_t(subtype) >= creature->material.size())
            return false;
        material = creature->material[subtype];
    }
    else if (type < PLANT_BASE)
    {
        mode = Creature;
        subtype = type - FIGURE_BASE;
        figure = df::historical_figure::find(index);
        if (!figure)
            return false;
        creature = df::creature_raw::find(figure->race);
        if (!creature || size_t(subtype) >= creature->material.size())
            return false;
        material = creature->material[subtype];
    }
    else if (type < END_BASE)
    {
        mode = Plant;
        subtype = type - PLANT_BASE;
        plant = df::plant_raw::find(index);
        if (!plant || size_t(subtype) >= plant->material.size())
            return false;
        material = plant->material[subtype];
    }
    else
    {
        material = raws.mat_table.builtin[type];
    }

    return (material != NULL);
}

bool MaterialInfo::find(const std::string& token)
{
    std::vector<std::string> items;
    split_string(&items, token, ":");
    return find(items);
}

bool MaterialInfo::find(const std::vector<std::string>& items)
{
    if (items.empty())
        return false;

    if (items[0] == "INORGANIC" && items.size() > 1)
        return findInorganic(vector_get(items, 1));
    if (items[0] == "CREATURE_MAT" || items[0] == "CREATURE")
        return findCreature(vector_get(items, 1), vector_get(items, 2));
    if (items[0] == "PLANT_MAT" || items[0] == "PLANT")
        return findPlant(vector_get(items, 1), vector_get(items, 2));

    if (items.size() == 1)
    {
        if (findBuiltin(items[0]))
            return true;
        if (findInorganic(items[0]))
            return true;
        if (findPlant(items[0], ""))
            return true;
    }
    else if (items.size() == 2)
    {
        if (items[0] == "COAL" && findBuiltin(items[0]))
        {
            if (items[1] == "COKE")
                this->index = 0;
            else if (items[1] == "CHARCOAL")
                this->index = 1;
            return true;
        }
        if (items[1] == "NONE" && findBuiltin(items[0]))
            return true;
        if (findPlant(items[0], items[1]))
            return true;
        if (findCreature(items[0], items[1]))
            return true;
    }

    return false;
}

bool MaterialInfo::findBuiltin(const std::string& token)
{
    if (token.empty())
        return decode(-1);

    if (token == "NONE")
    {
        decode(-1);
        return true;
    }

    auto& raws = world->raws;
    for (int i = 0; i < NUM_BUILTIN; i++)
    {
        auto obj = raws.mat_table.builtin[i];
        if (obj && obj->id == token)
            return decode(i, -1);
    }
    return decode(-1);
}

bool MaterialInfo::findInorganic(const std::string& token)
{
    if (token.empty())
        return decode(-1);

    if (token == "NONE")
    {
        decode(0, -1);
        return true;
    }

    auto& raws = world->raws;
    for (size_t i = 0; i < raws.inorganics.all.size(); i++)
    {
        df::inorganic_raw* p = raws.inorganics.all[i];
        if (p->id == token)
            return decode(0, i);
    }
    return decode(-1);
}

bool MaterialInfo::findPlant(const std::string& token, const std::string& subtoken)
{
    if (token.empty())
        return decode(-1);
    auto& raws = world->raws;
    for (size_t i = 0; i < raws.plants.all.size(); i++)
    {
        df::plant_raw* p = raws.plants.all[i];
        if (p->id != token)
            continue;

        // As a special exception, return the structural material with empty subtoken
        if (subtoken.empty())
            return decode(p->material_defs.type[plant_material_def::basic_mat], p->material_defs.idx[plant_material_def::basic_mat]);

        for (size_t j = 0; j < p->material.size(); j++)
            if (p->material[j]->id == subtoken)
                return decode(PLANT_BASE + j, i);

        break;
    }
    return decode(-1);
}

bool MaterialInfo::findCreature(const std::string& token, const std::string& subtoken)
{
    if (token.empty() || subtoken.empty())
        return decode(-1);
    auto& raws = world->raws;
    for (size_t i = 0; i < raws.creatures.all.size(); i++)
    {
        df::creature_raw* p = raws.creatures.all[i];
        if (p->creature_id != token)
            continue;

        for (size_t j = 0; j < p->material.size(); j++)
            if (p->material[j]->id == subtoken)
                return decode(CREATURE_BASE + j, i);

        break;
    }
    return decode(-1);
}

bool MaterialInfo::findProduct(df::material* material, const std::string& name)
{
    if (!material || name.empty())
        return decode(-1);

    auto& pids = material->reaction_product.id;
    for (size_t i = 0; i < pids.size(); i++)
        if ((*pids[i]) == name)
            return decode(material->reaction_product.material, i);

    return decode(-1);
}

std::string MaterialInfo::getToken() const
{
    if (isNone())
        return "NONE";

    if (!material)
        return fmt::format("INVALID:{}:{}", type, index);

    switch (mode)
    {
    case Builtin:
        if (material->id == "COAL")
        {
            if (index == 0)
                return "COAL:COKE";
            else if (index == 1)
                return "COAL:CHARCOAL";
        }
        return material->id;
    case Inorganic:
        return "INORGANIC:" + inorganic->id;
    case Creature:
        return "CREATURE:" + creature->creature_id + ":" + material->id;
    case Plant:
        return "PLANT:" + plant->id + ":" + material->id;
    default:
        return fmt::format("INVALID_MODE:{}:{}", type, index);
    }
}

std::string MaterialInfo::toString(uint16_t temp, bool named) const
{
    if (isNone())
        return "any";

    if (!material)
        return fmt::format("INVALID:{}:{}", type, index);

    df::matter_state state = matter_state::Solid;
    if (temp >= material->heat.melting_point)
        state = matter_state::Liquid;
    if (temp >= material->heat.boiling_point)
        state = matter_state::Gas;

    std::string name = material->state_name[state];
    if (!material->prefix.empty())
        name = material->prefix + " " + name;

    if (named && figure)
        name += fmt::format(" of HF {}", index);
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

bool MaterialInfo::isAnyCloth() const
{
    using namespace df::enums::material_flags;

    return material && (
        material->flags.is_set(THREAD_PLANT) ||
        material->flags.is_set(SILK) ||
        material->flags.is_set(YARN)
        );
}

bool MaterialInfo::matches(const df::job_material_category& cat) const
{
    if (!material)
        return false;

#define TEST(bit,flag) if (cat.bits.bit && material->flags.is_set(flag)) return true;

    using namespace df::enums::material_flags;
    TEST(plant, STRUCTURAL_PLANT_MAT);
    TEST(plant, SEED_MAT);
    TEST(plant, THREAD_PLANT);
    TEST(plant, ALCOHOL_PLANT);
    TEST(plant, POWDER_MISC_PLANT);
    TEST(plant, LIQUID_MISC_PLANT);
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
    return false;
}

bool MaterialInfo::matches(const df::dfhack_material_category& cat) const
{
    if (!material)
        return false;

    df::job_material_category mc;
    mc.whole = cat.whole;
    if (matches(mc))
        return true;

    using namespace df::enums::material_flags;
    using namespace df::enums::inorganic_flags;
    TEST(metal, IS_METAL);
    TEST(stone, IS_STONE);
    if (cat.bits.stone && type == 0 && index == -1)
        return true;
    if (cat.bits.sand && inorganic && inorganic->flags.is_set(SOIL_SAND))
        return true;
    TEST(glass, IS_GLASS);
    if (cat.bits.clay && linear_index(material->reaction_product.id, std::string("FIRED_MAT")) >= 0)
        return true;
    if (cat.bits.milk && linear_index(material->reaction_product.id, std::string("CHEESE_MAT")) >= 0)
        return true;
    TEST(gem, IS_GEM);
    return false;
}

#undef TEST

bool MaterialInfo::matches(const df::job_item& jitem, df::item_type itype) const
{
    if (!isValid()) return false;

    df::job_item_flags1 ok1, mask1;
    getMatchBits(ok1, mask1);

    df::job_item_flags2 ok2, mask2, xmask2;
    getMatchBits(ok2, mask2);

    df::job_item_flags3 ok3, mask3;
    getMatchBits(ok3, mask3);

    xmask2.bits.non_economic = itype != df::item_type::BOULDER;
    mask2.whole &= ~xmask2.whole;

    return bits_match(jitem.flags1.whole, ok1.whole, mask1.whole) &&
        bits_match(jitem.flags2.whole, ok2.whole, mask2.whole) &&
        bits_match(jitem.flags3.whole, ok3.whole, mask3.whole);
}

void MaterialInfo::getMatchBits(df::job_item_flags1& ok, df::job_item_flags1& mask) const
{
    ok.whole = mask.whole = 0;
    if (!isValid()) return;

#define MAT_FLAG(name) material->flags.is_set(material_flags::name)
#define FLAG(field, name) (field && field->flags.is_set(name))
#define TEST(bit, check) \
    mask.bits.bit = true; ok.bits.bit = !!(check);

    bool structural = MAT_FLAG(STRUCTURAL_PLANT_MAT);

    TEST(millable, structural && FLAG(plant, plant_raw_flags::MILL));
    TEST(sharpenable, MAT_FLAG(IS_STONE));
    TEST(processable, structural && FLAG(plant, plant_raw_flags::THREAD));
    TEST(cookable, MAT_FLAG(EDIBLE_COOKED));
    TEST(extract_bearing_plant, structural && FLAG(plant, plant_raw_flags::EXTRACT_STILL_VIAL));
    TEST(extract_bearing_fish, false);
    TEST(extract_bearing_vermin, false);
    TEST(processable_to_vial, structural && FLAG(plant, plant_raw_flags::EXTRACT_VIAL));
    TEST(processable_to_barrel, structural && FLAG(plant, plant_raw_flags::EXTRACT_BARREL));
    TEST(solid, !(MAT_FLAG(ALCOHOL_PLANT) ||
        MAT_FLAG(ALCOHOL_CREATURE) ||
        MAT_FLAG(LIQUID_MISC_PLANT) ||
        MAT_FLAG(LIQUID_MISC_CREATURE) ||
        MAT_FLAG(LIQUID_MISC_OTHER)));
    TEST(tameable_vermin, false);
    TEST(sharpenable, MAT_FLAG(IS_STONE));
    TEST(milk, linear_index(material->reaction_product.id, std::string("CHEESE_MAT")) >= 0);
    TEST(undisturbed, MAT_FLAG(SILK));
    //04000000 - "milkable" - vtable[107],1,1
}

void MaterialInfo::getMatchBits(df::job_item_flags2& ok, df::job_item_flags2& mask) const
{
    ok.whole = mask.whole = 0;
    if (!isValid()) return;

    bool is_cloth = isAnyCloth();

    TEST(dye, MAT_FLAG(IS_DYE));
    TEST(dyeable, is_cloth);
    TEST(dyed, is_cloth);
    TEST(sewn_imageless, is_cloth);
    TEST(glass_making, MAT_FLAG(CRYSTAL_GLASSABLE));

    TEST(fire_safe, material->heat.melting_point > 11000
        && material->heat.boiling_point > 11000
        && material->heat.ignite_point > 11000
        && material->heat.heatdam_point > 11000
        && (material->heat.colddam_point == 60001 || material->heat.colddam_point < 11000));
    TEST(magma_safe, material->heat.melting_point > 12000
        && material->heat.boiling_point > 12000
        && material->heat.ignite_point > 12000
        && material->heat.heatdam_point > 12000
        && (material->heat.colddam_point == 60001 || material->heat.colddam_point < 12000));
    TEST(deep_material, FLAG(inorganic, inorganic_flags::SPECIAL));
    TEST(non_economic, !inorganic || !(plotinfo && vector_get(plotinfo->economic_stone, index)));

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

void MaterialInfo::getMatchBits(df::job_item_flags3& ok, df::job_item_flags3& mask) const
{
    ok.whole = mask.whole = 0;
    if (!isValid()) return;

    TEST(hard, MAT_FLAG(ITEMS_HARD));
}

#undef MAT_FLAG
#undef FLAG
#undef TEST

bool DFHack::parseJobMaterialCategory(df::job_material_category* cat, const std::string& token)
{
    cat->whole = 0;

    std::vector<std::string> items;
    split_string(&items, toLower_cp437(token), ",", true);

    for (size_t i = 0; i < items.size(); i++)
    {
        if (!set_bitfield_field(cat, items[i], 1))
            return false;
    }

    return true;
}

bool DFHack::parseJobMaterialCategory(df::dfhack_material_category* cat, const std::string& token)
{
    cat->whole = 0;

    std::vector<std::string> items;
    split_string(&items, toLower_cp437(token), ",", true);

    for (size_t i = 0; i < items.size(); i++)
    {
        if (!set_bitfield_field(cat, items[i], 1))
            return false;
    }

    return true;
}

bool DFHack::isSoilInorganic(int material)
{
    auto raw = df::inorganic_raw::find(material);

    return raw && raw->flags.is_set(inorganic_flags::SOIL_ANY);
}

bool DFHack::isStoneInorganic(int material)
{
    auto raw = df::inorganic_raw::find(material);

    if (!raw ||
        raw->flags.is_set(inorganic_flags::SOIL_ANY) ||
        raw->material.flags.is_set(material_flags::IS_METAL))
        return false;

    return true;
}

t_matgloss::t_matgloss()
{
    fore = 0;
    back = 0;
    bright = 0;

    value = 0;
    wall_tile = 0;
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

std::string Materials::getDescription(const t_material& mat)
{
    MaterialInfo mi(mat.mat_type, mat.mat_index);
    if (mi.creature)
        return mi.creature->creature_id + " " + mi.material->id;
    else if (mi.plant)
        return mi.plant->id + " " + mi.material->id;
    else
        return mi.material->id;
}

// type of material only so we know which vector to retrieve
// This is completely worthless now
std::string Materials::getType(const t_material& mat)
{
    MaterialInfo mi(mat.mat_type, mat.mat_index);
    switch (mi.mode)
    {
    case MaterialInfo::Builtin:
        return "builtin";
    case MaterialInfo::Inorganic:
        return "inorganic";
    case MaterialInfo::Creature:
        return "creature";
    case MaterialInfo::Plant:
        return "plant";
    default:
        return "unknown";
    }
}
