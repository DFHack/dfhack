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

#include <string>
#include <vector>
#include <map>
#include <cstring>
using namespace std;

#include "Types.h"
#include "modules/Materials.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "Error.h"
#include "ModuleFactory.h"
#include "Core.h"

#include "MiscUtils.h"

#include "df/world.h"
#include "df/ui.h"
#include "df/item.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/body_part_raw.h"
#include "df/historical_figure.h"

#include "df/job_item.h"
#include "df/job_material_category.h"
#include "df/dfhack_material_category.h"
#include "df/matter_state.h"
#include "df/material_vec_ref.h"
#include "df/builtin_mats.h"

#include "df/descriptor_color.h"
#include "df/descriptor_pattern.h"
#include "df/descriptor_shape.h"

#include "df/physical_attribute_type.h"
#include "df/mental_attribute_type.h"
#include <df/color_modifier_raw.h>

using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;

bool MaterialInfo::decode(df::item *item)
{
    if (!item)
        return decode(-1);
    else
        return decode(item->getActualMaterial(), item->getActualMaterialIndex());
}

bool MaterialInfo::decode(const df::material_vec_ref &vr, int idx)
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

    if (type < 0) {
        mode = None;
        return false;
    }

    df::world_raws &raws = world->raws;

    if (size_t(type) >= sizeof(raws.mat_table.builtin)/sizeof(void*))
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
        if (!creature || size_t(subtype) >= creature->material.size())
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
        if (!creature || size_t(subtype) >= creature->material.size())
            return false;
        material = creature->material[subtype];
    }
    else if (type < END_BASE)
    {
        mode = Plant;
        subtype = type-PLANT_BASE;
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

bool MaterialInfo::find(const std::string &token)
{
    std::vector<std::string> items;
    split_string(&items, token, ":");
    return find(items);
}

bool MaterialInfo::find(const std::vector<std::string> &items)
{
    if (items.empty())
        return false;

    if (items[0] == "INORGANIC" && items.size() > 1)
        return findInorganic(vector_get(items,1));
    if (items[0] == "CREATURE_MAT" || items[0] == "CREATURE")
        return findCreature(vector_get(items,1), vector_get(items,2));
    if (items[0] == "PLANT_MAT" || items[0] == "PLANT")
        return findPlant(vector_get(items,1), vector_get(items,2));

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
        if (items[0] == "COAL" && findBuiltin(items[0])) {
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

bool MaterialInfo::findBuiltin(const std::string &token)
{
    if (token.empty())
        return decode(-1);

    if (token == "NONE") {
        decode(-1);
        return true;
    }

    df::world_raws &raws = world->raws;
    for (int i = 0; i < NUM_BUILTIN; i++)
    {
        auto obj = raws.mat_table.builtin[i];
        if (obj && obj->id == token)
            return decode(i, -1);
    }
    return decode(-1);
}

bool MaterialInfo::findInorganic(const std::string &token)
{
    if (token.empty())
        return decode(-1);

    if (token == "NONE") {
        decode(0, -1);
        return true;
    }

    df::world_raws &raws = world->raws;
    for (size_t i = 0; i < raws.inorganics.size(); i++)
    {
        df::inorganic_raw *p = raws.inorganics[i];
        if (p->id == token)
            return decode(0, i);
    }
    return decode(-1);
}

bool MaterialInfo::findPlant(const std::string &token, const std::string &subtoken)
{
    if (token.empty())
        return decode(-1);
    df::world_raws &raws = world->raws;
    for (size_t i = 0; i < raws.plants.all.size(); i++)
    {
        df::plant_raw *p = raws.plants.all[i];
        if (p->id != token)
            continue;

        // As a special exception, return the structural material with empty subtoken
        if (subtoken.empty())
            return decode(p->material_defs.type_basic_mat, p->material_defs.idx_basic_mat);

        for (size_t j = 0; j < p->material.size(); j++)
            if (p->material[j]->id == subtoken)
                return decode(PLANT_BASE+j, i);

        break;
    }
    return decode(-1);
}

bool MaterialInfo::findCreature(const std::string &token, const std::string &subtoken)
{
    if (token.empty() || subtoken.empty())
        return decode(-1);
    df::world_raws &raws = world->raws;
    for (size_t i = 0; i < raws.creatures.all.size(); i++)
    {
        df::creature_raw *p = raws.creatures.all[i];
        if (p->creature_id != token)
            continue;

        for (size_t j = 0; j < p->material.size(); j++)
            if (p->material[j]->id == subtoken)
                return decode(CREATURE_BASE+j, i);

        break;
    }
    return decode(-1);
}

bool MaterialInfo::findProduct(df::material *material, const std::string &name)
{
    if (!material || name.empty())
        return decode(-1);

    auto &pids = material->reaction_product.id;
    for (size_t i = 0; i < pids.size(); i++)
        if ((*pids[i]) == name)
            return decode(material->reaction_product.material, i);

    return decode(-1);
}

std::string MaterialInfo::getToken()
{
    if (isNone())
        return "NONE";

    if (!material)
        return stl_sprintf("INVALID:%d:%d", type, index);

    switch (mode) {
    case Builtin:
        if (material->id == "COAL") {
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
        return stl_sprintf("INVALID_MODE:%d:%d", type, index);
    }
}

std::string MaterialInfo::toString(uint16_t temp, bool named)
{
    if (isNone())
        return "any";

    if (!material)
        return stl_sprintf("INVALID:%d:%d", type, index);

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

bool MaterialInfo::matches(const df::dfhack_material_category &cat)
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
    return false;
}

#undef TEST

bool MaterialInfo::matches(const df::job_item &item)
{
    if (!isValid()) return false;

    df::job_item_flags1 ok1, mask1;
    getMatchBits(ok1, mask1);

    df::job_item_flags2 ok2, mask2;
    getMatchBits(ok2, mask2);

    df::job_item_flags3 ok3, mask3;
    getMatchBits(ok3, mask3);

    return bits_match(item.flags1.whole, ok1.whole, mask1.whole) &&
           bits_match(item.flags2.whole, ok2.whole, mask2.whole) &&
           bits_match(item.flags3.whole, ok3.whole, mask3.whole);
}

void MaterialInfo::getMatchBits(df::job_item_flags1 &ok, df::job_item_flags1 &mask)
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
    TEST(distillable, structural && FLAG(plant, plant_raw_flags::DRINK));
    TEST(processable, structural && FLAG(plant, plant_raw_flags::THREAD));
    TEST(bag, isAnyCloth() || MAT_FLAG(LEATHER));
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
    TEST(deep_material, FLAG(inorganic, inorganic_flags::SPECIAL));
    TEST(non_economic, !inorganic || !(ui && vector_get(ui->economic_stone, index)));

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

bool DFHack::parseJobMaterialCategory(df::job_material_category *cat, const std::string &token)
{
    cat->whole = 0;

    std::vector<std::string> items;
    split_string(&items, toLower(token), ",", true);

    for (size_t i = 0; i < items.size(); i++)
    {
        if (!set_bitfield_field(cat, items[i], 1))
            return false;
    }

    return true;
}

bool DFHack::parseJobMaterialCategory(df::dfhack_material_category *cat, const std::string &token)
{
    cat->whole = 0;

    std::vector<std::string> items;
    split_string(&items, toLower(token), ",", true);

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

Module* DFHack::createMaterials()
{
    return new Materials();
}


Materials::Materials()
{
}

Materials::~Materials()
{
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

bool Materials::CopyInorganicMaterials (std::vector<t_matglossInorganic> & inorganic)
{
    size_t size = world->raws.inorganics.size();
    inorganic.clear();
    inorganic.reserve (size);
    for (size_t i = 0; i < size;i++)
    {
        df::inorganic_raw *orig = world->raws.inorganics[i];
        t_matglossInorganic mat;
        mat.id = orig->id;
        mat.name = orig->material.stone_name;

        mat.ore_types = orig->metal_ore.mat_index;
        mat.ore_chances = orig->metal_ore.probability;
        mat.strand_types = orig->thread_metal.mat_index;
        mat.strand_chances = orig->thread_metal.probability;
        mat.value = orig->material.material_value;
        mat.wall_tile = orig->material.tile;
        mat.boulder_tile = orig->material.item_symbol;
        mat.fore = orig->material.basic_color[0];
        mat.bright = orig->material.basic_color[1];
        mat.is_gem = orig->material.flags.is_set(material_flags::IS_GEM);
        inorganic.push_back(mat);
    }
    return true;
}

bool Materials::CopyOrganicMaterials (std::vector<t_matgloss> & organic)
{
    size_t size = world->raws.plants.all.size();
    organic.clear();
    organic.reserve (size);
    for (size_t i = 0; i < size;i++)
    {
        t_matgloss mat;
        mat.id = world->raws.plants.all[i]->id;
        organic.push_back(mat);
    }
    return true;
}

bool Materials::CopyWoodMaterials (std::vector<t_matgloss> & tree)
{
    size_t size = world->raws.plants.trees.size();
    tree.clear();
    tree.reserve (size);
    for (size_t i = 0; i < size;i++)
    {
        t_matgloss mat;
        mat.id = world->raws.plants.trees[i]->id;
        tree.push_back(mat);
    }
    return true;
}

bool Materials::CopyPlantMaterials (std::vector<t_matgloss> & plant)
{
    size_t size = world->raws.plants.bushes.size();
    plant.clear();
    plant.reserve (size);
    for (size_t i = 0; i < size;i++)
    {
        t_matgloss mat;
        mat.id = world->raws.plants.bushes[i]->id;
        plant.push_back(mat);
    }
    return true;
}

bool Materials::ReadCreatureTypes (void)
{
    size_t size = world->raws.creatures.all.size();
    race.clear();
    race.reserve (size);
    for (size_t i = 0; i < size;i++)
    {
        t_matgloss mat;
        mat.id = world->raws.creatures.all[i]->creature_id;
        race.push_back(mat);
    }
    return true;
}

bool Materials::ReadOthers(void)
{
    other.clear();
    FOR_ENUM_ITEMS(builtin_mats, i)
    {
        t_matglossOther mat;
        mat.id = world->raws.mat_table.builtin[i]->id;
        other.push_back(mat);
    }
    return true;
}

bool Materials::ReadDescriptorColors (void)
{
    size_t size = world->raws.language.colors.size();

    color.clear();
    if(size == 0)
        return false;
    color.reserve(size);
    for (size_t i = 0; i < size;i++)
    {
        df::descriptor_color *c = world->raws.language.colors[i];
        t_descriptor_color col;
        col.id = c->id;
        col.name = c->name;
        col.red = c->red;
        col.green = c->green;
        col.blue = c->blue;
        color.push_back(col);
    }

    size = world->raws.language.patterns.size();
    alldesc.clear();
    alldesc.reserve(size);
    for (size_t i = 0; i < size;i++)
    {
        t_matgloss mat;
        mat.id = world->raws.language.patterns[i]->id;
        alldesc.push_back(mat);
    }
    return true;
}

bool Materials::ReadCreatureTypesEx (void)
{
    size_t size = world->raws.creatures.all.size();
    raceEx.clear();
    raceEx.reserve (size);
    for (size_t i = 0; i < size; i++)
    {
        df::creature_raw *cr = world->raws.creatures.all[i];
        t_creaturetype mat;
        mat.id = cr->creature_id;
        mat.tile_character = cr->creature_tile;
        mat.tilecolor.fore = cr->color[0];
        mat.tilecolor.back = cr->color[1];
        mat.tilecolor.bright = cr->color[2];

        size_t sizecas = cr->caste.size();
        for (size_t j = 0; j < sizecas;j++)
        {
            df::caste_raw *ca = cr->caste[j];
            /* caste name */
            t_creaturecaste caste;
            caste.id = ca->caste_id;
            caste.singular = ca->caste_name[0];
            caste.plural = ca->caste_name[1];
            caste.adjective = ca->caste_name[2];

            // color mod reading
            // Caste + offset > color mod vector
            auto & colorings = ca->color_modifiers;
            size_t sizecolormod = colorings.size();
            caste.ColorModifier.resize(sizecolormod);
            for(size_t k = 0; k < sizecolormod;k++)
            {
                // color mod [0] -> color list
                auto & indexes = colorings[k]->pattern_index;
                size_t sizecolorlist = indexes.size();
                caste.ColorModifier[k].colorlist.resize(sizecolorlist);
                for(size_t l = 0; l < sizecolorlist; l++)
                    caste.ColorModifier[k].colorlist[l] = indexes[l];
                // color mod [color_modifier_part_offset] = string part
                caste.ColorModifier[k].part = colorings[k]->part;
                caste.ColorModifier[k].startdate = colorings[k]->start_date;
                caste.ColorModifier[k].enddate = colorings[k]->end_date;
            }

            // body parts
            caste.bodypart.empty();
            size_t sizebp = ca->body_info.body_parts.size();
            for (size_t k = 0; k < sizebp; k++)
            {
                df::body_part_raw *bp = ca->body_info.body_parts[k];
                t_bodypart part;
                part.id = bp->token;
                part.category = bp->category;
                caste.bodypart.push_back(part);
            }
            using namespace df::enums::mental_attribute_type;
            using namespace df::enums::physical_attribute_type;
            for (int32_t k = 0; k < 7; k++)
            {
                auto & physical = ca->attributes.phys_att_range;
                caste.strength[k] = physical[STRENGTH][k];
                caste.agility[k] = physical[AGILITY][k];
                caste.toughness[k] = physical[TOUGHNESS][k];
                caste.endurance[k] = physical[ENDURANCE][k];
                caste.recuperation[k] = physical[RECUPERATION][k];
                caste.disease_resistance[k] = physical[DISEASE_RESISTANCE][k];

                auto & mental = ca->attributes.ment_att_range;
                caste.analytical_ability[k] = mental[ANALYTICAL_ABILITY][k];
                caste.focus[k] = mental[FOCUS][k];
                caste.willpower[k] = mental[WILLPOWER][k];
                caste.creativity[k] = mental[CREATIVITY][k];
                caste.intuition[k] = mental[INTUITION][k];
                caste.patience[k] = mental[PATIENCE][k];
                caste.memory[k] = mental[MEMORY][k];
                caste.linguistic_ability[k] = mental[LINGUISTIC_ABILITY][k];
                caste.spatial_sense[k] = mental[SPATIAL_SENSE][k];
                caste.musicality[k] = mental[MUSICALITY][k];
                caste.kinesthetic_sense[k] = mental[KINESTHETIC_SENSE][k];
                caste.empathy[k] = mental[EMPATHY][k];
                caste.social_awareness[k] = mental[SOCIAL_AWARENESS][k];
            }
            mat.castes.push_back(caste);
        }
        for (size_t j = 0; j < world->raws.creatures.all[i]->material.size(); j++)
        {
            t_creatureextract extract;
            extract.id = world->raws.creatures.all[i]->material[j]->id;
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

std::string Materials::getDescription(const t_material & mat)
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
std::string Materials::getType(const t_material & mat)
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
