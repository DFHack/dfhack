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

#include "Core.h"
#include "Debug.h"
#include "Error.h"
#include "Internal.h"
#include "MemAccess.h"
#include "MiscUtils.h"
#include "Types.h"
#include "VersionInfo.h"
#include "ModuleFactory.h"

#include "modules/Job.h"
#include "modules/MapCache.h"
#include "modules/Materials.h"
#include "modules/Items.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/artifact_record.h"
#include "df/body_part_raw.h"
#include "df/body_part_template_flags.h"
#include "df/building.h"
#include "df/building_actual.h"
#include "df/building_tradedepotst.h"
#include "df/builtin_mats.h"
#include "df/caravan_state.h"
#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/dfhack_material_category.h"
#include "df/entity_buy_prices.h"
#include "df/entity_buy_requests.h"
#include "df/entity_sell_category.h"
#include "df/entity_sell_prices.h"
#include "df/entity_raw.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/general_ref_contains_itemst.h"
#include "df/general_ref_projectile.h"
#include "df/general_ref_unit_itemownerst.h"
#include "df/general_ref_unit_holderst.h"
#include "df/historical_entity.h"
#include "df/item.h"
#include "df/item_bookst.h"
#include "df/item_plant_growthst.h"
#include "df/item_toolst.h"
#include "df/item_type.h"
#include "df/itemdef_ammost.h"
#include "df/itemdef_armorst.h"
#include "df/itemdef_foodst.h"
#include "df/itemdef_glovesst.h"
#include "df/itemdef_helmst.h"
#include "df/itemdef_instrumentst.h"
#include "df/itemdef_pantsst.h"
#include "df/itemdef_shieldst.h"
#include "df/itemdef_shoesst.h"
#include "df/itemdef_siegeammost.h"
#include "df/itemdef_toolst.h"
#include "df/itemdef_toyst.h"
#include "df/itemdef_trapcompst.h"
#include "df/itemdef_weaponst.h"
#include "df/itemimprovement.h"
#include "df/itemimprovement_pagesst.h"
#include "df/itemimprovement_writingst.h"
#include "df/job_item.h"
#include "df/mandate.h"
#include "df/map_block.h"
#include "df/material.h"
#include "df/proj_itemst.h"
#include "df/proj_list_link.h"
#include "df/reaction_product_itemst.h"
#include "df/tool_uses.h"
#include "df/trapcomp_flags.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/vehicle.h"
#include "df/vermin.h"
#include "df/viewscreen.h"
#include "df/world.h"
#include "df/world_site.h"
#include "df/written_content.h"

#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <set>

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::plotinfo;
using df::global::ui_selected_unit;
using df::global::proj_next_id;

namespace DFHack {
    DBG_DECLARE(core, items, DebugCategory::LINFO);
}

#define ITEMDEF_VECTORS \
    ITEM(WEAPON, weapons, itemdef_weaponst) \
    ITEM(TRAPCOMP, trapcomps, itemdef_trapcompst) \
    ITEM(TOY, toys, itemdef_toyst) \
    ITEM(TOOL, tools, itemdef_toolst) \
    ITEM(INSTRUMENT, instruments, itemdef_instrumentst) \
    ITEM(ARMOR, armor, itemdef_armorst) \
    ITEM(AMMO, ammo, itemdef_ammost) \
    ITEM(SIEGEAMMO, siege_ammo, itemdef_siegeammost) \
    ITEM(GLOVES, gloves, itemdef_glovesst) \
    ITEM(SHOES, shoes, itemdef_shoesst) \
    ITEM(SHIELD, shields, itemdef_shieldst) \
    ITEM(HELM, helms, itemdef_helmst) \
    ITEM(PANTS, pants, itemdef_pantsst) \
    ITEM(FOOD, food, itemdef_foodst)

int Items::getSubtypeCount(df::item_type itype)
{
    using namespace df::enums::item_type;

    df::world_raws::T_itemdefs &defs = df::global::world->raws.itemdefs;

    switch (itype) {
#define ITEM(type,vec,tclass) \
    case type: \
        return defs.vec.size();
ITEMDEF_VECTORS
#undef ITEM

    default:
        return -1;
    }
}

df::itemdef *Items::getSubtypeDef(df::item_type itype, int subtype)
{
    using namespace df::enums::item_type;

    df::world_raws::T_itemdefs &defs = df::global::world->raws.itemdefs;

    switch (itype) {
#define ITEM(type,vec,tclass) \
    case type: \
        return vector_get(defs.vec, subtype);
ITEMDEF_VECTORS
#undef ITEM

    default:
        return NULL;
    }
}

bool ItemTypeInfo::decode(df::item_type type_, int16_t subtype_)
{
    type = type_;
    subtype = subtype_;
    custom = Items::getSubtypeDef(type_, subtype_);

    return isValid();
}

bool ItemTypeInfo::decode(df::item *ptr)
{
    if (!ptr)
        return decode(item_type::NONE);
    else
        return decode(ptr->getType(), ptr->getSubtype());
}

std::string ItemTypeInfo::getToken()
{
    std::string rv = ENUM_KEY_STR(item_type, type);
    if (custom)
        rv += ":" + custom->id;
    else if (subtype != -1 && type != item_type::PLANT_GROWTH)
        rv += stl_sprintf(":%d", subtype);
    return rv;
}

std::string ItemTypeInfo::toString()
{
    using namespace df::enums::item_type;

    switch (type) {
#define ITEM(type,vec,tclass) \
    case type: \
        if (VIRTUAL_CAST_VAR(cv, df::tclass, custom)) \
            return cv->name;
ITEMDEF_VECTORS
#undef ITEM

    default:
        break;
    }

    const char *name = ENUM_ATTR(item_type, caption, type);
    if (name)
        return name;

    return toLower_cp437(ENUM_KEY_STR(item_type, type));
}

bool ItemTypeInfo::find(const std::string &token)
{
    using namespace df::enums::item_type;

    std::vector<std::string> items;
    split_string(&items, token, ":");

    type = NONE;
    subtype = -1;
    custom = NULL;

    if (items.size() < 1 || items.size() > 2)
        return false;

    if (items[0] == "NONE")
        return true;

    if (!find_enum_item(&type, items[0]))
        return false;
    if (type == NONE)
        return false;
    if (items.size() == 1)
        return true;

    df::world_raws::T_itemdefs &defs = df::global::world->raws.itemdefs;

    switch (type) {
#define ITEM(type,vec,tclass) \
    case type: \
        for (size_t i = 0; i < defs.vec.size(); i++) { \
            if (defs.vec[i]->id == items[1]) { \
                subtype = i; custom = defs.vec[i]; return true; \
            } \
        } \
        break;
ITEMDEF_VECTORS
#undef ITEM

    default:
        if (items[1] == "NONE")
            return true;
        break;
    }

    return (subtype >= 0);
}

bool Items::isCasteMaterial(df::item_type itype)
{
    return ENUM_ATTR(item_type, is_caste_mat, itype);
}

bool ItemTypeInfo::matches(df::job_item_vector_id vec_id)
{
    auto other_id = ENUM_ATTR(job_item_vector_id, other, vec_id);

    auto explicit_item = ENUM_ATTR(items_other_id, item, other_id);
    if (explicit_item != item_type::NONE && type != explicit_item)
        return false;

    auto generic_item = ENUM_ATTR(items_other_id, generic_item, other_id);
    if (generic_item.size > 0)
    {
        for (size_t i = 0; i < generic_item.size; i++)
            if (generic_item.items[i] == type)
                return true;

        return false;
    }

    return true;
}

bool ItemTypeInfo::matches(const df::job_item &item, MaterialInfo *mat,
                           bool skip_vector, df::item_type itype)
{
    using namespace df::enums::item_type;

    if (!isValid())
        return mat ? mat->matches(item, itype) : false;

    if (Items::isCasteMaterial(type) && mat && !mat->isNone())
        return false;

    if (!skip_vector && !matches(item.vector_id))
        return false;

    df::job_item_flags1 ok1, mask1, item_ok1, item_mask1, xmask1;
    df::job_item_flags2 ok2, mask2, item_ok2, item_mask2, xmask2;
    df::job_item_flags3 ok3, mask3, item_ok3, item_mask3;

    ok1.whole = mask1.whole = item_ok1.whole = item_mask1.whole = xmask1.whole = 0;
    ok2.whole = mask2.whole = item_ok2.whole = item_mask2.whole = xmask2.whole = 0;
    ok3.whole = mask3.whole = item_ok3.whole = item_mask3.whole = 0;

    if (mat) {
        mat->getMatchBits(ok1, mask1);
        mat->getMatchBits(ok2, mask2);
        mat->getMatchBits(ok3, mask3);
    }

#define OK(id,name) item_ok##id.bits.name = true
#define RQ(id,name) item_mask##id.bits.name = true

    // Set up the flag mask

    RQ(1,millable); RQ(1,sharpenable); RQ(1,distillable); RQ(1,processable); RQ(1,bag);
    RQ(1,extract_bearing_plant); RQ(1,extract_bearing_fish); RQ(1,extract_bearing_vermin);
    RQ(1,processable_to_vial); RQ(1,processable_to_bag); RQ(1,processable_to_barrel);
    RQ(1,solid); RQ(1,tameable_vermin); RQ(1,sand_bearing); RQ(1,milk); RQ(1,milkable);
    RQ(1,not_bin); RQ(1,lye_bearing); RQ(1, undisturbed);

    RQ(2,dye); RQ(2,dyeable); RQ(2,dyed); RQ(2,glass_making); RQ(2,screw);
    RQ(2,building_material); RQ(2,fire_safe); RQ(2,magma_safe);
    RQ(2,totemable); RQ(2,plaster_containing); RQ(2,body_part); RQ(2,lye_milk_free);
    RQ(2,blunt); RQ(2,unengraved); RQ(2,hair_wool);

    RQ(3,any_raw_material); RQ(3,non_pressed); RQ(3,food_storage);

    // only checked if boulder

    xmask2.bits.non_economic = true;

    // Compute the ok mask

    OK(1,solid);
    OK(1,not_bin);

    // TODO: furniture, ammo, finished good, craft

    switch (type) {
    case PLANT:
        OK(1,millable); OK(1,processable);
        OK(1,distillable);
        OK(1,extract_bearing_plant);
        OK(1,processable_to_vial);
        OK(1,processable_to_bag);
        OK(1,processable_to_barrel);
        break;

    case BOULDER:
        OK(1,sharpenable);
        xmask2.bits.non_economic = false;
    case BAR:
        OK(3,any_raw_material);
    case BLOCKS:
    case WOOD:
        OK(2,building_material);
        OK(2,fire_safe); OK(2,magma_safe);
        break;

    case VERMIN:
        OK(1,extract_bearing_fish);
        OK(1,extract_bearing_vermin);
        OK(1,tameable_vermin);
        OK(1,milkable);
        break;

    case DRINK:
        item_ok1.bits.solid = false;
        break;

    case ROUGH:
        OK(2,glass_making);
        break;

    case ANIMALTRAP:
    case CAGE:
        OK(1,milk);
        OK(1,milkable);
        xmask1.bits.cookable = true;
        break;

    case BUCKET:
        OK(2,lye_milk_free);
    case FLASK:
        OK(1,milk);
        xmask1.bits.cookable = true;
        break;

    case TOOL:
        OK(1,lye_bearing);
        OK(1,milk);
        OK(2,lye_milk_free);
        OK(2,blunt);
        xmask1.bits.cookable = true;

        if (VIRTUAL_CAST_VAR(def, df::itemdef_toolst, custom)) {
            df::tool_uses key(tool_uses::FOOD_STORAGE);
            if (linear_index(def->tool_use, key) >= 0)
                OK(3,food_storage);
        } else {
            OK(3,food_storage);
        }
        break;

    case BARREL:
        OK(1,lye_bearing);
        OK(1,milk);
        OK(2,lye_milk_free);
        OK(3,food_storage);
        xmask1.bits.cookable = true;
        break;

    // TODO: split this into BOX and BAG
    case BOX:
        OK(1,bag); OK(1,sand_bearing); OK(1,milk);
        OK(2,dye); OK(2,plaster_containing);
        xmask1.bits.cookable = true;
        break;

    case BIN:
        item_ok1.bits.not_bin = false;
        break;

    case LIQUID_MISC:
        item_ok1.bits.solid = false;
        OK(1,milk);
        break;

    case POWDER_MISC:
        OK(2,dye);
    case GLOB:
        OK(3,any_raw_material);
        OK(3,non_pressed);
        break;

    case THREAD:
        OK(1,undisturbed);
    case CLOTH:
        OK(2,dyeable); OK(2,dyed);
        break;

    case WEAPON:
    case AMMO:
    case ROCK:
        OK(2,blunt);
        break;

    case TRAPCOMP:
        if (VIRTUAL_CAST_VAR(def, df::itemdef_trapcompst, custom)) {
            if (def->flags.is_set(trapcomp_flags::IS_SCREW))
                OK(2,screw);
        } else {
            OK(2,screw);
        }
        OK(2,blunt);
        break;

    case CORPSE:
    case CORPSEPIECE:
        OK(2,totemable);
        OK(2,body_part);
        OK(2,hair_wool);
        break;

    case SLAB:
        OK(2,unengraved);
        break;

    case ANVIL:
        OK(2,fire_safe); OK(2,magma_safe);
        break;

    default:
        break;
    }

    if ((item_ok1.whole & ~item_mask1.whole) ||
        (item_ok2.whole & ~item_mask2.whole) ||
        (item_ok3.whole & ~item_mask3.whole))
        Core::printerr("ItemTypeInfo.matches inconsistent\n");

#undef OK
#undef RQ

    mask1.whole &= ~xmask1.whole;
    mask2.whole &= ~xmask2.whole;

    return bits_match(item.flags1.whole, ok1.whole, mask1.whole) &&
           bits_match(item.flags2.whole, ok2.whole, mask2.whole) &&
           bits_match(item.flags3.whole, ok3.whole, mask3.whole) &&
           bits_match(item.flags1.whole, item_ok1.whole, item_mask1.whole) &&
           bits_match(item.flags2.whole, item_ok2.whole, item_mask2.whole) &&
           bits_match(item.flags3.whole, item_ok3.whole, item_mask3.whole);
}

df::item * Items::findItemByID(int32_t id)
{
    if (id < 0)
        return 0;
    return df::item::find(id);
}

df::general_ref *Items::getGeneralRef(df::item *item, df::general_ref_type type)
{
    CHECK_NULL_POINTER(item);

    return findRef(item->general_refs, type);
}

df::specific_ref *Items::getSpecificRef(df::item *item, df::specific_ref_type type)
{
    CHECK_NULL_POINTER(item);

    return findRef(item->specific_refs, type);
}

df::unit *Items::getOwner(df::item * item)
{
    auto ref = getGeneralRef(item, general_ref_type::UNIT_ITEMOWNER);

    return ref ? ref->getUnit() : NULL;
}

bool Items::setOwner(df::item *item, df::unit *unit)
{
    CHECK_NULL_POINTER(item);

    for (int i = item->general_refs.size()-1; i >= 0; i--)
    {
        df::general_ref *ref = item->general_refs[i];

        if (ref->getType() != general_ref_type::UNIT_ITEMOWNER)
            continue;

        if (auto cur = ref->getUnit())
        {
            if (cur == unit)
                return true;

            erase_from_vector(cur->owned_items, item->id);
        }

        delete ref;
        vector_erase_at(item->general_refs, i);
    }

    item->flags.bits.owned = false;

    if (unit)
    {
        auto ref = df::allocate<df::general_ref_unit_itemownerst>();
        if (!ref)
            return false;

        item->flags.bits.owned = true;
        ref->unit_id = unit->id;

        insert_into_vector(unit->owned_items, item->id);
        item->general_refs.push_back(ref);
    }

    return true;
}

df::item *Items::getContainer(df::item * item)
{
    auto ref = getGeneralRef(item, general_ref_type::CONTAINED_IN_ITEM);

    return ref ? ref->getItem() : NULL;
}

void Items::getOuterContainerRef(df::specific_ref &spec_ref, df::item *item, bool init_ref)
{
    CHECK_NULL_POINTER(item);
    // Reverse-engineered from ambushing unit code

    if (init_ref)
    {
        spec_ref.type = specific_ref_type::ITEM_GENERAL;
        spec_ref.data.object = item;
    }

    if (item->flags.bits.removed || !item->flags.bits.in_inventory)
        return;

    for (size_t i = 0; i < item->general_refs.size(); i++)
    {
        auto g = item->general_refs[i];
        switch (g->getType())
        {
            case general_ref_type::CONTAINED_IN_ITEM:
                if (auto item2 = g->getItem())
                    return Items::getOuterContainerRef(spec_ref, item2);
                break;
            case general_ref_type::UNIT_HOLDER:
                if (auto unit = g->getUnit())
                    return Units::getOuterContainerRef(spec_ref, unit);
                break;
            default:
                break;
        }
    }

    auto s = findRef(item->specific_refs, specific_ref_type::VERMIN_ESCAPED_PET);
    if (s)
    {
        spec_ref.type = specific_ref_type::VERMIN_EVENT;
        spec_ref.data.vermin = s->data.vermin;
    }
    return;
}

void Items::getContainedItems(df::item *item, std::vector<df::item*> *items)
{
    CHECK_NULL_POINTER(item);

    items->clear();

    for (size_t i = 0; i < item->general_refs.size(); i++)
    {
        df::general_ref *ref = item->general_refs[i];
        if (ref->getType() != general_ref_type::CONTAINS_ITEM)
            continue;

        auto child = ref->getItem();
        if (child)
            items->push_back(child);
    }
}

df::building *Items::getHolderBuilding(df::item * item)
{
    auto ref = getGeneralRef(item, general_ref_type::BUILDING_HOLDER);

    return ref ? ref->getBuilding() : NULL;
}

df::unit *Items::getHolderUnit(df::item * item)
{
    auto ref = getGeneralRef(item, general_ref_type::UNIT_HOLDER);

    return ref ? ref->getUnit() : NULL;
}

df::coord Items::getPosition(df::item *item)
{
    CHECK_NULL_POINTER(item);

    /* Function reverse-engineered from DF code. */

    if (item->flags.bits.removed)
        return df::coord();

    if (item->flags.bits.in_inventory)
    {
        for (size_t i = 0; i < item->general_refs.size(); i++)
        {
            df::general_ref *ref = item->general_refs[i];

            switch (ref->getType())
            {
            case general_ref_type::CONTAINED_IN_ITEM:
                if (auto item2 = ref->getItem())
                    return getPosition(item2);
                break;

            case general_ref_type::UNIT_HOLDER:
                if (auto unit = ref->getUnit())
                    return Units::getPosition(unit);
                break;

            /*case general_ref_type::BUILDING_HOLDER:
                if (auto bld = ref->getBuilding())
                    return df::coord(bld->centerx, bld->centery, bld->z);
                break;*/

            default:
                break;
            }
        }

        for (size_t i = 0; i < item->specific_refs.size(); i++)
        {
            df::specific_ref *ref = item->specific_refs[i];

            switch (ref->type)
            {
            case specific_ref_type::VERMIN_ESCAPED_PET:
                return ref->data.vermin->pos;

            default:
                break;
            }
        }

        return df::coord();
    }

    return item->pos;
}

static const char quality_table[] = {
    '\0',   // (base)
    '-',    // well-crafted
    '+',    // finely-crafted
    '*',    // superior quality
    '\xF0', // (≡) exceptional
    '\x0F'  // (☼) masterful
};

static void addQuality(std::string &tmp, int quality)
{
    if (quality > 0 && quality <= 5) {
        char c = quality_table[quality];
        tmp = c + tmp + c;
    }
}

//  It's not impossible the functionality of this operation is provided by one of the unmapped item functions.
std::string Items::getBookTitle(df::item *item)
{
    CHECK_NULL_POINTER(item);

    std::string tmp;

    if (item->getType() == df::item_type::BOOK)
    {
        auto book = virtual_cast<df::item_bookst>(item);

        if (book->title != "")
        {
            return book->title;
        }
        else
        {
            for (size_t i = 0; i < book->improvements.size(); i++)
            {
                if (auto page = virtual_cast<df::itemimprovement_pagesst>(book->improvements[i]))
                {
                    for (size_t k = 0; k < page->contents.size(); k++)
                    {
                        df::written_content *contents = world->written_contents.all[page->contents[k]];
                        if (contents->title != "")
                        {
                            return contents->title;
                        }
                    }
                }
                else if (auto writing = virtual_cast<df::itemimprovement_writingst>(book->improvements[i]))
                {
                    for (size_t k = 0; k < writing->contents.size(); k++)
                    {
                        df::written_content *contents = world->written_contents.all[writing->contents[k]];
                        if (contents->title != "")
                        {
                            return contents->title;
                        }
                    }
                }
            }
        }
    }
    else if (item->getType() == df::item_type::TOOL)
    {
        auto book = virtual_cast<df::item_toolst>(item);

        if (book->hasToolUse(df::tool_uses::CONTAIN_WRITING))
        {
            for (size_t i = 0; i < book->improvements.size(); i++)
            {
                if (auto page = virtual_cast<df::itemimprovement_pagesst>(book->improvements[i]))
                {
                    for (size_t k = 0; k < page->contents.size(); k++)
                    {
                        df::written_content *contents = world->written_contents.all[page->contents[k]];
                        if (contents->title != "")
                        {
                            return contents->title;
                        }
                    }
                }
                else if (auto writing = virtual_cast<df::itemimprovement_writingst>(book->improvements[i]))
                {
                    for (size_t k = 0; k < writing->contents.size(); k++)
                    {
                        df::written_content *contents = world->written_contents.all[writing->contents[k]];
                        if (contents->title != "")
                        {
                            return contents->title;
                        }
                    }
                }
            }
        }
    }

    return "";
}

std::string Items::getDescription(df::item *item, int type, bool decorate)
{
    CHECK_NULL_POINTER(item);

    std::string tmp;
    item->getItemDescription(&tmp, type);

    if (decorate) {
        addQuality(tmp, item->getQuality());

        if (item->isImproved()) {
            tmp = '\xAE' + tmp + '\xAF'; // («) + tmp + (»)
            addQuality(tmp, item->getImprovementQuality());
        }

        if (item->flags.bits.foreign)
            tmp = "(" + tmp + ")";
    }

    return tmp;
}

static df::artifact_record* get_artifact(df::item *item) {
    if (!item->flags.bits.artifact)
        return NULL;
    if (auto gref = Items::getGeneralRef(item, df::general_ref_type::IS_ARTIFACT))
        return gref->getArtifact();
    return NULL;
}

static string get_item_type_str(df::item *item) {
    ItemTypeInfo iti;
    iti.decode(item);
    auto str = capitalize_string_words(iti.toString());
    if (str == "Trapparts")
        str = "Mechanism";
    return str;
}

static string get_base_desc(df::item *item) {
    if (auto name = Items::getBookTitle(item); name.size())
        return name;
    if (auto artifact = get_artifact(item)) {
        if (artifact->name.has_name)
            return Translation::TranslateName(&artifact->name, false) + ", " + Translation::TranslateName(&artifact->name) + " (" + get_item_type_str(item) + ")";
    }
    return Items::getDescription(item, 0, true);
}

string Items::getReadableDescription(df::item *item) {
    CHECK_NULL_POINTER(item);

    auto desc = get_base_desc(item);

    switch (item->getWear()) {
    case 1: desc = "x" + desc + "x"; break;
    case 2: desc = "X" + desc + "X"; break;
    case 3: desc = "XX" + desc + "XX"; break;
    default:
        break;
    }

    if (auto gref = Items::getGeneralRef(item, df::general_ref_type::CONTAINS_UNIT)) {
        if (auto unit = gref->getUnit()) {
            auto str = " [" + Units::getReadableName(unit);
            if (Units::isInvader(unit) || Units::isOpposedToLife(unit))
                str += " (hostile)";
            str += "]";
            return desc + str;
        }
    }

    return desc;
}

static void resetUnitInvFlags(df::unit *unit, df::unit_inventory_item *inv_item)
{
    if (inv_item->mode == df::unit_inventory_item::Worn ||
        inv_item->mode == df::unit_inventory_item::WrappedAround)
    {
        unit->flags2.bits.calculated_inventory = false;
        unit->flags2.bits.calculated_insulation = false;
    }
    else if (inv_item->mode == df::unit_inventory_item::StuckIn)
    {
        unit->flags3.bits.stuck_weapon_computed = false;
    }
}

static bool detachItem(MapExtras::MapCache &mc, df::item *item)
{
    if (!item->specific_refs.empty())
        return false;
    if (item->world_data_id != -1)
        return false;

    bool building_clutter = false;
    for (size_t i = 0; i < item->general_refs.size(); i++)
    {
        df::general_ref *ref = item->general_refs[i];

        switch (ref->getType())
        {
            case general_ref_type::BUILDING_HOLDER:
                if (item->flags.bits.in_building)
                    return false;
                building_clutter = true;
                break;
            case general_ref_type::BUILDING_CAGED:
            case general_ref_type::BUILDING_TRIGGER:
            case general_ref_type::BUILDING_TRIGGERTARGET:
            case general_ref_type::BUILDING_CIVZONE_ASSIGNED:
                return false;

            default:
                continue;
        }
    }

    if (building_clutter)
    {
        auto building = virtual_cast<df::building_actual>(Items::getHolderBuilding(item));
        if (building)
        {
            for (size_t i = 0; i < building->contained_items.size(); i++)
            {
                auto ci = building->contained_items[i];
                if (ci->item == item)
                {
                    DFHack::removeRef(item->general_refs, general_ref_type::BUILDING_HOLDER, building->id);
                    vector_erase_at(building->contained_items, i);
                    delete ci;
                    return true;
                }
            }
        }
    }

    if (auto *ref = virtual_cast<df::general_ref_projectile>(Items::getGeneralRef(item, general_ref_type::PROJECTILE)))
    {
        return linked_list_remove(&world->proj_list, ref->projectile_id) &&
            DFHack::removeRef(item->general_refs, general_ref_type::PROJECTILE, ref->getID());
    }

    if (item->flags.bits.on_ground)
    {
        if (!mc.removeItemOnGround(item))
            Core::printerr("Item was marked on_ground, but not in block: %d (%d,%d,%d)\n",
                           item->id, item->pos.x, item->pos.y, item->pos.z);

        item->flags.bits.on_ground = false;
        return true;
    }

    if (item->flags.bits.in_inventory)
    {
        bool found = false;

        for (int i = item->general_refs.size()-1; i >= 0; i--)
        {
            df::general_ref *ref = item->general_refs[i];

            switch (ref->getType())
            {
            case general_ref_type::CONTAINED_IN_ITEM:
                if (auto item2 = ref->getItem())
                {
/* TODO: understand how this changes for v50
                    // Viewscreens hold general_ref_contains_itemst pointers
                    for (auto screen = Core::getTopViewscreen(); screen; screen = screen->parent)
                    {
                        auto vsitem = strict_virtual_cast<df::viewscreen_itemst>(screen);
                        if (vsitem && vsitem->item == item2)
                            return false;
                    }
*/
                    item2->flags.bits.weight_computed = false;

                    removeRef(item2->general_refs, general_ref_type::CONTAINS_ITEM, item->id);
                }
                break;

            case general_ref_type::UNIT_HOLDER:
                if (auto unit = ref->getUnit())
                {
/* TODO: understand how this changes for v50
                    // Unit view sidebar holds inventory item pointers
                    if (plotinfo->main.mode == ui_sidebar_mode::ViewUnits &&
                        (!ui_selected_unit ||
                         vector_get(world->units.active, *ui_selected_unit) == unit))
                        return false;
*/

                    for (int i = unit->inventory.size()-1; i >= 0; i--)
                    {
                        df::unit_inventory_item *inv_item = unit->inventory[i];
                        if (inv_item->item != item)
                            continue;

                        resetUnitInvFlags(unit, inv_item);

                        vector_erase_at(unit->inventory, i);
                        delete inv_item;
                    }
                }
                break;

            default:
                continue;
            }

            found = true;
            vector_erase_at(item->general_refs, i);
            delete ref;
        }

        if (!found)
            return false;

        item->flags.bits.in_inventory = false;
        return true;
    }

    if (item->flags.bits.removed)
    {
        item->flags.bits.removed = false;

        if (item->flags.bits.garbage_collect)
        {
            item->flags.bits.garbage_collect = false;
            item->categorize(true);
        }

        return true;
    }

    return false;
}

static void putOnGround(MapExtras::MapCache &mc, df::item *item, df::coord pos)
{
    item->pos = pos;
    item->flags.bits.on_ground = true;

    if (!mc.addItemOnGround(item))
        Core::printerr("Could not add item %d to ground at (%d,%d,%d)\n",
                       item->id, pos.x, pos.y, pos.z);
}

bool DFHack::Items::moveToGround(MapExtras::MapCache &mc, df::item *item, df::coord pos)
{
    CHECK_NULL_POINTER(item);

    if (!detachItem(mc, item))
        return false;

    putOnGround(mc, item, pos);
    return true;
}

bool DFHack::Items::moveToContainer(MapExtras::MapCache &mc, df::item *item, df::item *container)
{
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(container);

    auto cpos = getPosition(container);
    if (!cpos.isValid())
        return false;

    auto ref1 = df::allocate<df::general_ref_contains_itemst>();
    auto ref2 = df::allocate<df::general_ref_contained_in_itemst>();

    if (!ref1 || !ref2)
    {
        delete ref1; delete ref2;
        Core::printerr("Could not allocate container refs.\n");
        return false;
    }

    if (!detachItem(mc, item))
    {
        delete ref1; delete ref2;
        return false;
    }

    item->pos = container->pos;
    item->flags.bits.in_inventory = true;

    container->flags.bits.container = true;
    container->flags.bits.weight_computed = false;

    ref1->item_id = item->id;
    container->general_refs.push_back(ref1);

    ref2->item_id = container->id;
    item->general_refs.push_back(ref2);

    return true;
}

bool DFHack::Items::moveToBuilding(MapExtras::MapCache &mc, df::item *item, df::building_actual *building,
    df::building_item_role_type use_mode, bool force_in_building)
{
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(building);
    CHECK_INVALID_ARGUMENT(use_mode == 0 || use_mode == 2);

    auto ref = df::allocate<df::general_ref_building_holderst>();
    if(!ref)
    {
        delete ref;
        Core::printerr("Could not allocate building holder refs.\n");
        return false;
    }

    if (!detachItem(mc, item))
    {
        delete ref;
        return false;
    }

    item->pos.x=building->centerx;
    item->pos.y=building->centery;
    item->pos.z=building->z;
    if (use_mode == 2 || force_in_building)
        item->flags.bits.in_building=true;

    ref->building_id=building->id;
    item->general_refs.push_back(ref);

    auto con=new df::building_actual::T_contained_items;
    con->item=item;
    con->use_mode=use_mode;
    building->contained_items.push_back(con);

    return true;
}

bool DFHack::Items::moveToInventory(
    MapExtras::MapCache &mc, df::item *item, df::unit *unit,
    df::unit_inventory_item::T_mode mode, int body_part
) {
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(unit);
    CHECK_NULL_POINTER(unit->body.body_plan);
    CHECK_INVALID_ARGUMENT(is_valid_enum_item(mode));
    int body_plan_size = unit->body.body_plan->body_parts.size();
    CHECK_INVALID_ARGUMENT(body_part < 0 || body_part <= body_plan_size);

    auto holderReference = df::allocate<df::general_ref_unit_holderst>();
    if (!holderReference)
    {
        Core::printerr("Could not allocate UNIT_HOLDER reference.\n");
        return false;
    }

    if (!detachItem(mc, item))
    {
        delete holderReference;
        return false;
    }

    item->flags.bits.in_inventory = true;

    auto newInventoryItem = new df::unit_inventory_item();
    newInventoryItem->item = item;
    newInventoryItem->mode = mode;
    newInventoryItem->body_part_id = body_part;
    unit->inventory.push_back(newInventoryItem);

    holderReference->unit_id = unit->id;
    item->general_refs.push_back(holderReference);

    resetUnitInvFlags(unit, newInventoryItem);

    return true;
}

bool Items::remove(MapExtras::MapCache &mc, df::item *item, bool no_uncat)
{
    CHECK_NULL_POINTER(item);

    auto pos = getPosition(item);

    if (!detachItem(mc, item))
        return false;

    if (pos.isValid())
        item->pos = pos;

    if (!no_uncat)
        item->uncategorize();

    item->flags.bits.removed = true;
    item->flags.bits.garbage_collect = !no_uncat;
    return true;
}

df::proj_itemst *Items::makeProjectile(MapExtras::MapCache &mc, df::item *item)
{
    CHECK_NULL_POINTER(item);

    if (!world || !proj_next_id)
        return NULL;

    auto pos = getPosition(item);
    if (!pos.isValid())
        return NULL;

    auto ref = df::allocate<df::general_ref_projectile>();
    if (!ref)
        return NULL;

    auto proj = df::allocate<df::proj_itemst>();
    if (!proj) {
        delete ref;
        return NULL;
    }

    if (!detachItem(mc, item))
    {
        delete ref;
        delete proj;
        return NULL;
    }

    item->pos = pos;
    item->flags.bits.in_job = true;

    proj->link = new df::proj_list_link();
    proj->link->item = proj;
    proj->id = (*proj_next_id)++;

    proj->origin_pos = proj->target_pos = pos;
    proj->cur_pos = proj->prev_pos = pos;
    proj->item = item;

    ref->projectile_id = proj->id;
    item->general_refs.push_back(ref);

    linked_list_append(&world->proj_list, proj->link);

    return proj;
}

int Items::getItemBaseValue(int16_t item_type, int16_t item_subtype, int16_t mat_type, int32_t mat_subtype)
{
    int value = 0;
    switch (item_type)
    {
    case item_type::BAR:
    case item_type::BLOCKS:
    case item_type::SKIN_TANNED:
        value = 5;
        break;

    case item_type::SMALLGEM:
        value = 20;
        break;

    case item_type::BOULDER:
    case item_type::WOOD:
        value = 3;
        break;

    case item_type::ROUGH:
        value = 6;
        break;

    case item_type::DOOR:
    case item_type::FLOODGATE:
    case item_type::BED:
    case item_type::CHAIR:
    case item_type::CHAIN:
    case item_type::FLASK:
    case item_type::GOBLET:
    case item_type::TOY:
    case item_type::CAGE:
    case item_type::BARREL:
    case item_type::BUCKET:
    case item_type::ANIMALTRAP:
    case item_type::TABLE:
    case item_type::COFFIN:
    case item_type::BOX:
    case item_type::BAG:
    case item_type::BIN:
    case item_type::ARMORSTAND:
    case item_type::WEAPONRACK:
    case item_type::CABINET:
    case item_type::FIGURINE:
    case item_type::AMULET:
    case item_type::SCEPTER:
    case item_type::CROWN:
    case item_type::RING:
    case item_type::EARRING:
    case item_type::BRACELET:
    case item_type::GEM:
    case item_type::ANVIL:
    case item_type::TOTEM:
    case item_type::BACKPACK:
    case item_type::QUIVER:
    case item_type::BALLISTAARROWHEAD:
    case item_type::PIPE_SECTION:
    case item_type::HATCH_COVER:
    case item_type::GRATE:
    case item_type::QUERN:
    case item_type::MILLSTONE:
    case item_type::SPLINT:
    case item_type::CRUTCH:
    case item_type::SLAB:
    case item_type::BOOK:
        value = 10;
        break;

    case item_type::WINDOW:
    case item_type::STATUE:
        value = 25;
        break;

    case item_type::CORPSE:
    case item_type::CORPSEPIECE:
    case item_type::REMAINS:
        return 0;

    case item_type::WEAPON:
        if (size_t(item_subtype) < world->raws.itemdefs.weapons.size())
            value = world->raws.itemdefs.weapons[item_subtype]->value;
        else
            value = 10;
        break;

    case item_type::ARMOR:
        if (size_t(item_subtype) < world->raws.itemdefs.armor.size())
            value = world->raws.itemdefs.armor[item_subtype]->value;
        else
            value = 10;
        break;

    case item_type::SHOES:
        if (size_t(item_subtype) < world->raws.itemdefs.shoes.size())
            value = world->raws.itemdefs.shoes[item_subtype]->value;
        else
            value = 5;
        break;

    case item_type::SHIELD:
        if (size_t(item_subtype) < world->raws.itemdefs.shields.size())
            value = world->raws.itemdefs.shields[item_subtype]->value;
        else
            value = 10;
        break;

    case item_type::HELM:
        if (size_t(item_subtype) < world->raws.itemdefs.helms.size())
            value = world->raws.itemdefs.helms[item_subtype]->value;
        else
            value = 10;
        break;

    case item_type::GLOVES:
        if (size_t(item_subtype) < world->raws.itemdefs.gloves.size())
            value = world->raws.itemdefs.gloves[item_subtype]->value;
        else
            value = 5;
        break;

    case item_type::AMMO:
        if (size_t(item_subtype) < world->raws.itemdefs.ammo.size())
            value = world->raws.itemdefs.ammo[item_subtype]->value;
        else
            value = 1;
        break;

    case item_type::MEAT:
    case item_type::PLANT:
    case item_type::PLANT_GROWTH:
        value = 2;
        break;

    case item_type::CHEESE:
        value = 10;
        break;

    case item_type::FISH:
    case item_type::FISH_RAW:
    case item_type::EGG:
        value = 2;
        if (size_t(mat_type) < world->raws.creatures.all.size())
        {
            auto creature = world->raws.creatures.all[mat_type];
            if (size_t(mat_subtype) < creature->caste.size())
            {
                auto caste = creature->caste[mat_subtype];
                mat_type = caste->misc.bone_mat;
                mat_subtype = caste->misc.bone_matidx;
            }
        }
        break;

    case item_type::VERMIN:
        value = 0;
        if (size_t(mat_type) < world->raws.creatures.all.size())
        {
            auto creature = world->raws.creatures.all[mat_type];
            if (size_t(mat_subtype) < creature->caste.size())
                value = creature->caste[mat_subtype]->misc.petvalue;
        }
        value /= 2;
        if (!value)
                return 1;
        return value;

    case item_type::PET:
        if (size_t(mat_type) < world->raws.creatures.all.size())
        {
            auto creature = world->raws.creatures.all[mat_type];
            if (size_t(mat_subtype) < creature->caste.size())
                return creature->caste[mat_subtype]->misc.petvalue;
        }
        return 0;

    case item_type::SEEDS:
    case item_type::DRINK:
    case item_type::POWDER_MISC:
    case item_type::LIQUID_MISC:
    case item_type::COIN:
    case item_type::GLOB:
    case item_type::ORTHOPEDIC_CAST:
    case item_type::BRANCH:
        value = 1;
        break;

    case item_type::THREAD:
        value = 6;
        break;

    case item_type::CLOTH:
        value = 7;
        break;

    case item_type::SHEET:
        value = 5;
        break;

    case item_type::PANTS:
        if (size_t(item_subtype) < world->raws.itemdefs.pants.size())
            value = world->raws.itemdefs.pants[item_subtype]->value;
        else
            value = 10;
        break;

    case item_type::CATAPULTPARTS:
    case item_type::BALLISTAPARTS:
    case item_type::TRAPPARTS:
        value = 30;
        break;

    case item_type::SIEGEAMMO:
    case item_type::TRACTION_BENCH:
        value = 20;
        break;

    case item_type::TRAPCOMP:
        if (size_t(item_subtype) < world->raws.itemdefs.trapcomps.size())
            value = world->raws.itemdefs.trapcomps[item_subtype]->value;
        else
            value = 10;
        break;

    case item_type::FOOD:
        return 10;

    case item_type::TOOL:
        if (size_t(item_subtype) < world->raws.itemdefs.tools.size())
            value = world->raws.itemdefs.tools[item_subtype]->value;
        else
            value = 10;
        break;

    case item_type::INSTRUMENT:
        if (size_t(item_subtype) < world->raws.itemdefs.instruments.size())
            value = world->raws.itemdefs.instruments[item_subtype]->value;
        else
            value = 10;
        break;

//  case item_type::ROCK:
    default:
        return 0;
    }

    MaterialInfo mat;
    if (mat.decode(mat_type, mat_subtype))
        value *= mat.material->material_value;
    return value;
}

static int32_t get_war_multiplier(df::item *item, df::caravan_state *caravan) {
    static const int32_t DEFAULT_WAR_MULTIPLIER = 256;

    if (!caravan)
        return DEFAULT_WAR_MULTIPLIER;
    auto caravan_he = df::historical_entity::find(caravan->entity);
    if (!caravan_he)
        return DEFAULT_WAR_MULTIPLIER;
    int32_t war_alignment = caravan_he->entity_raw->sphere_alignment[df::sphere_type::WAR];
    if (war_alignment == DEFAULT_WAR_MULTIPLIER)
        return DEFAULT_WAR_MULTIPLIER;
    switch (item->getType()) {
    case df::item_type::WEAPON:
    {
        auto weap_def = df::itemdef_weaponst::find(item->getSubtype());
        auto caravan_cre_raw = df::creature_raw::find(caravan_he->race);
        if (!weap_def || !caravan_cre_raw || caravan_cre_raw->adultsize < weap_def->minimum_size)
            return DEFAULT_WAR_MULTIPLIER;
        break;
    }
    case df::item_type::ARMOR:
    case df::item_type::SHOES:
    case df::item_type::HELM:
    case df::item_type::GLOVES:
    case df::item_type::PANTS:
    {
        if (item->getEffectiveArmorLevel() <= 0)
            return DEFAULT_WAR_MULTIPLIER;
        auto caravan_cre_raw = df::creature_raw::find(caravan_he->race);
        auto maker_cre_raw = df::creature_raw::find(item->getMakerRace());
        if (!caravan_cre_raw || !maker_cre_raw)
            return DEFAULT_WAR_MULTIPLIER;
        if (caravan_cre_raw->adultsize < ((maker_cre_raw->adultsize * 6) / 7))
            return DEFAULT_WAR_MULTIPLIER;
        if (caravan_cre_raw->adultsize > ((maker_cre_raw->adultsize * 8) / 7))
            return DEFAULT_WAR_MULTIPLIER;
        break;
    }
    case df::item_type::SHIELD:
    case df::item_type::AMMO:
    case df::item_type::BACKPACK:
    case df::item_type::QUIVER:
        break;
    default:
        return DEFAULT_WAR_MULTIPLIER;
    }
    return war_alignment;
}

static const int32_t DEFAULT_AGREEMENT_MULTIPLIER = 128;

static int32_t get_buy_request_multiplier(df::item *item, const df::entity_buy_prices *buy_prices) {
    if (!buy_prices)
        return DEFAULT_AGREEMENT_MULTIPLIER;

    int16_t item_type = item->getType();
    int16_t item_subtype = item->getSubtype();
    int16_t mat_type = item->getMaterial();
    int32_t mat_subtype = item->getMaterialIndex();

    for (size_t idx = 0; idx < buy_prices->price.size(); ++idx) {
        if (buy_prices->items->item_type[idx] != item_type)
            continue;
        if (buy_prices->items->item_subtype[idx] != -1 && buy_prices->items->item_subtype[idx] != item_subtype)
            continue;
        if (buy_prices->items->mat_types[idx] != -1 && buy_prices->items->mat_types[idx] != mat_type)
            continue;
        if (buy_prices->items->mat_indices[idx] != -1 && buy_prices->items->mat_indices[idx] != mat_subtype)
            continue;
        return buy_prices->price[idx];
    }
    return DEFAULT_AGREEMENT_MULTIPLIER;
}

template<typename T>
static int get_price(const std::vector<T> &res, int32_t val, const std::vector<int32_t> &pri) {
    for (size_t idx = 0; idx < res.size(); ++idx) {
        if (res[idx] == val && pri.size() > idx)
            return pri[idx];
    }
    return -1;
}

template<typename T1, typename T2>
static int get_price(const std::vector<T1> &mat_res, int32_t mat, const std::vector<T2> &gloss_res, int32_t gloss, const std::vector<int32_t> &pri) {
    for (size_t idx = 0; idx < mat_res.size(); ++idx) {
        if (mat_res[idx] == mat && (gloss_res[idx] == -1 || gloss_res[idx] == gloss) && pri.size() > idx)
            return pri[idx];
    }
    return -1;
}

static const uint16_t PLANT_BASE = 419;
static const uint16_t NUM_PLANT_TYPES = 200;

static int32_t get_sell_request_multiplier(df::item *item, const df::historical_entity::T_resources &resources, const std::vector<int32_t> *prices) {
    static const df::dfhack_material_category silk_cat(df::dfhack_material_category::mask_silk);
    static const df::dfhack_material_category yarn_cat(df::dfhack_material_category::mask_yarn);
    static const df::dfhack_material_category leather_cat(df::dfhack_material_category::mask_leather);

    int16_t item_type = item->getType();
    int16_t item_subtype = item->getSubtype();
    int16_t mat_type = item->getMaterial();
    int32_t mat_subtype = item->getMaterialIndex();

    bool inorganic = mat_type == df::builtin_mats::INORGANIC;
    bool is_plant = (uint16_t)(mat_type - PLANT_BASE) < NUM_PLANT_TYPES;

    switch (item_type) {
    case df::item_type::BAR:
        if (inorganic) {
            if (int32_t price = get_price(resources.metals, mat_subtype, prices[df::entity_sell_category::MetalBars]); price != -1)
                return price;
        }
        break;
    case df::item_type::SMALLGEM:
        if (inorganic) {
            if (int32_t price = get_price(resources.gems, mat_subtype, prices[df::entity_sell_category::SmallCutGems]); price != -1)
                return price;
        }
        break;
    case df::item_type::BLOCKS:
        if (inorganic) {
            if (int32_t price = get_price(resources.stones, mat_subtype, prices[df::entity_sell_category::StoneBlocks]); price != -1)
                return price;
        }
        break;
    case df::item_type::ROUGH:
        if (int32_t price = get_price(resources.misc_mat.glass.mat_type, mat_type, resources.misc_mat.glass.mat_index, mat_subtype,
                prices[df::entity_sell_category::Glass]); price != -1)
            return price;
        break;
    case df::item_type::BOULDER:
        if (int32_t price = get_price(resources.stones, mat_subtype, prices[df::entity_sell_category::Stone]); price != -1)
            return price;
        if (int32_t price = get_price(resources.misc_mat.clay.mat_type, mat_type, resources.misc_mat.clay.mat_index, mat_subtype,
                prices[df::entity_sell_category::Clay]); price != -1)
            return price;
        break;
    case df::item_type::WOOD:
        if (int32_t price = get_price(resources.organic.wood.mat_type, mat_type, resources.organic.wood.mat_index, mat_subtype,
                prices[df::entity_sell_category::Wood]); price != -1)
            return price;
        break;
    case df::item_type::CHAIN:
        if (is_plant) {
            if (int32_t price = get_price(resources.organic.fiber.mat_type, mat_type, resources.organic.fiber.mat_index, mat_subtype,
                    prices[df::entity_sell_category::RopesPlant]); price != -1)
                return price;
        }
        {
            MaterialInfo mi;
            mi.decode(mat_type, mat_subtype);
            if (mi.isValid()) {
                if (mi.matches(silk_cat)) {
                    if (int32_t price = get_price(resources.organic.silk.mat_type, mat_type, resources.organic.silk.mat_index, mat_subtype,
                            prices[df::entity_sell_category::RopesSilk]); price != -1)
                        return price;
                }
                if (mi.matches(yarn_cat)) {
                    if (int32_t price = get_price(resources.organic.wool.mat_type, mat_type, resources.organic.wool.mat_index, mat_subtype,
                            prices[df::entity_sell_category::RopesYarn]); price != -1)
                        return price;
                }
            }
        }
        break;
    case df::item_type::FLASK:
        if (int32_t price = get_price(resources.misc_mat.flasks.mat_type, mat_type, resources.misc_mat.flasks.mat_index, mat_subtype,
                prices[df::entity_sell_category::FlasksWaterskins]); price != -1)
            return price;
        break;
    case df::item_type::GOBLET:
        if (int32_t price = get_price(resources.misc_mat.crafts.mat_type, mat_type, resources.misc_mat.crafts.mat_index, mat_subtype,
                prices[df::entity_sell_category::CupsMugsGoblets]); price != -1)
            return price;
        break;
    case df::item_type::INSTRUMENT:
        if (int32_t price = get_price(resources.instrument_type, mat_subtype, prices[df::entity_sell_category::Instruments]); price != -1)
            return price;
        break;
    case df::item_type::TOY:
        if (int32_t price = get_price(resources.toy_type, mat_subtype, prices[df::entity_sell_category::Toys]); price != -1)
            return price;
        break;
    case df::item_type::CAGE:
        if (int32_t price = get_price(resources.misc_mat.cages.mat_type, mat_type, resources.misc_mat.cages.mat_index, mat_subtype,
                prices[df::entity_sell_category::Cages]); price != -1)
            return price;
        break;
    case df::item_type::BARREL:
        if (int32_t price = get_price(resources.misc_mat.barrels.mat_type, mat_type, resources.misc_mat.barrels.mat_index, mat_subtype,
                prices[df::entity_sell_category::Barrels]); price != -1)
            return price;
        break;
    case df::item_type::BUCKET:
        if (int32_t price = get_price(resources.misc_mat.barrels.mat_type, mat_type, resources.misc_mat.barrels.mat_index, mat_subtype,
                prices[df::entity_sell_category::Buckets]); price != -1)
            return price;
        break;
    case df::item_type::WEAPON:
        if (int32_t price = get_price(resources.weapon_type, mat_subtype, prices[df::entity_sell_category::Weapons]); price != -1)
            return price;
        if (int32_t price = get_price(resources.digger_type, mat_subtype, prices[df::entity_sell_category::DiggingImplements]); price != -1)
            return price;
        if (int32_t price = get_price(resources.training_weapon_type, mat_subtype, prices[df::entity_sell_category::TrainingWeapons]); price != -1)
            return price;
        break;
    case df::item_type::ARMOR:
        if (int32_t price = get_price(resources.armor_type, mat_subtype, prices[df::entity_sell_category::Bodywear]); price != -1)
            return price;
        break;
    case df::item_type::SHOES:
        if (int32_t price = get_price(resources.shoes_type, mat_subtype, prices[df::entity_sell_category::Footwear]); price != -1)
            return price;
        break;
    case df::item_type::SHIELD:
        if (int32_t price = get_price(resources.shield_type, mat_subtype, prices[df::entity_sell_category::Shields]); price != -1)
            return price;
        break;
    case df::item_type::HELM:
        if (int32_t price = get_price(resources.helm_type, mat_subtype, prices[df::entity_sell_category::Headwear]); price != -1)
            return price;
        break;
    case df::item_type::GLOVES:
        if (int32_t price = get_price(resources.gloves_type, mat_subtype, prices[df::entity_sell_category::Handwear]); price != -1)
            return price;
        break;
    case df::item_type::BAG:
        {
            MaterialInfo mi;
            mi.decode(mat_type, mat_subtype);
            if (mi.isValid() && mi.matches(leather_cat)) {
                if (int32_t price = get_price(resources.organic.leather.mat_type, mat_type, resources.organic.leather.mat_index, mat_subtype,
                        prices[df::entity_sell_category::BagsLeather]); price != -1)
                    return price;
            }
            if (is_plant) {
                if (int32_t price = get_price(resources.organic.fiber.mat_type, mat_type, resources.organic.fiber.mat_index, mat_subtype,
                        prices[df::entity_sell_category::BagsPlant]); price != -1)
                    return price;
            }
            if (mi.isValid() && mi.matches(silk_cat)) {
                if (int32_t price = get_price(resources.organic.silk.mat_type, mat_type, resources.organic.silk.mat_index, mat_subtype,
                        prices[df::entity_sell_category::BagsSilk]); price != -1)
                    return price;
            }
            if (mi.isValid() && mi.matches(yarn_cat)) {
                if (int32_t price = get_price(resources.organic.wool.mat_type, mat_type, resources.organic.wool.mat_index, mat_subtype,
                        prices[df::entity_sell_category::BagsYarn]); price != -1)
                    return price;
            }
        }
        break;
    case df::item_type::FIGURINE:
    case df::item_type::AMULET:
    case df::item_type::SCEPTER:
    case df::item_type::CROWN:
    case df::item_type::RING:
    case df::item_type::EARRING:
    case df::item_type::BRACELET:
    case df::item_type::TOTEM:
    case df::item_type::BOOK:
        if (int32_t price = get_price(resources.misc_mat.crafts.mat_type, mat_type, resources.misc_mat.crafts.mat_index, mat_subtype,
                prices[df::entity_sell_category::Crafts]); price != -1)
            return price;
        break;
    case df::item_type::AMMO:
        if (int32_t price = get_price(resources.ammo_type, mat_subtype, prices[df::entity_sell_category::Ammo]); price != -1)
            return price;
        break;
    case df::item_type::GEM:
        if (inorganic) {
            if (int32_t price = get_price(resources.gems, mat_subtype, prices[df::entity_sell_category::LargeCutGems]); price != -1)
                return price;
        }
        break;
    case df::item_type::ANVIL:
        if (int32_t price = get_price(resources.metal.anvil.mat_type, mat_type, resources.metal.anvil.mat_index, mat_subtype,
                prices[df::entity_sell_category::Anvils]); price != -1)
            return price;
        break;
    case df::item_type::MEAT:
        if (int32_t price = get_price(resources.misc_mat.meat.mat_type, mat_type, resources.misc_mat.meat.mat_index, mat_subtype,
                prices[df::entity_sell_category::Meat]); price != -1)
            return price;
        break;
    case df::item_type::FISH:
    case df::item_type::FISH_RAW:
        if (int32_t price = get_price(resources.fish_races, mat_type, resources.fish_castes, mat_subtype,
                prices[df::entity_sell_category::Fish]); price != -1)
            return price;
        break;
    case df::item_type::VERMIN:
    case df::item_type::PET:
        if (int32_t price = get_price(resources.animals.pet_races, mat_type, resources.animals.pet_castes, mat_subtype,
                prices[df::entity_sell_category::Pets]); price != -1)
            return price;
        break;
    case df::item_type::SEEDS:
        if (int32_t price = get_price(resources.seeds.mat_type, mat_type, resources.seeds.mat_index, mat_subtype,
                prices[df::entity_sell_category::Seeds]); price != -1)
            return price;
        break;
    case df::item_type::PLANT:
        if (int32_t price = get_price(resources.plants.mat_type, mat_type, resources.plants.mat_index, mat_subtype,
                prices[df::entity_sell_category::Plants]); price != -1)
            return price;
        break;
    case df::item_type::SKIN_TANNED:
        if (int32_t price = get_price(resources.organic.leather.mat_type, mat_type, resources.organic.leather.mat_index, mat_subtype,
                prices[df::entity_sell_category::Leather]); price != -1)
            return price;
        break;
    case df::item_type::PLANT_GROWTH:
        if (is_plant) {
            if (int32_t price = get_price(resources.tree_fruit_plants, mat_type, resources.tree_fruit_growths, mat_subtype,
                    prices[df::entity_sell_category::FruitsNuts]); price != -1)
                return price;
            if (int32_t price = get_price(resources.shrub_fruit_plants, mat_type, resources.shrub_fruit_growths, mat_subtype,
                    prices[df::entity_sell_category::GardenVegetables]); price != -1)
                return price;
        }
        break;
    case df::item_type::THREAD:
        if (is_plant) {
            if (int32_t price = get_price(resources.organic.fiber.mat_type, mat_type, resources.organic.fiber.mat_index, mat_subtype,
                    prices[df::entity_sell_category::ThreadPlant]); price != -1)
                return price;
        }
        {
            MaterialInfo mi;
            mi.decode(mat_type, mat_subtype);
            if (mi.isValid() && mi.matches(silk_cat)) {
                if (int32_t price = get_price(resources.organic.silk.mat_type, mat_type, resources.organic.silk.mat_index, mat_subtype,
                        prices[df::entity_sell_category::ThreadSilk]); price != -1)
                    return price;
            }
            if (mi.isValid() && mi.matches(yarn_cat)) {
                if (int32_t price = get_price(resources.organic.wool.mat_type, mat_type, resources.organic.wool.mat_index, mat_subtype,
                        prices[df::entity_sell_category::ThreadYarn]); price != -1)
                    return price;
            }
        }
        break;
    case df::item_type::CLOTH:
        if (is_plant) {
            if (int32_t price = get_price(resources.organic.fiber.mat_type, mat_type, resources.organic.fiber.mat_index, mat_subtype,
                    prices[df::entity_sell_category::ClothPlant]); price != -1)
                return price;
        }
        {
            MaterialInfo mi;
            mi.decode(mat_type, mat_subtype);
            if (mi.isValid() && mi.matches(silk_cat)) {
                if (int32_t price = get_price(resources.organic.silk.mat_type, mat_type, resources.organic.silk.mat_index, mat_subtype,
                        prices[df::entity_sell_category::ClothSilk]); price != -1)
                    return price;
            }
            if (mi.isValid() && mi.matches(yarn_cat)) {
                if (int32_t price = get_price(resources.organic.wool.mat_type, mat_type, resources.organic.wool.mat_index, mat_subtype,
                        prices[df::entity_sell_category::ClothYarn]); price != -1)
                    return price;
            }
        }
        break;
    case df::item_type::PANTS:
        if (int32_t price = get_price(resources.pants_type, mat_subtype, prices[df::entity_sell_category::Legwear]); price != -1)
            return price;
        break;
    case df::item_type::BACKPACK:
        if (int32_t price = get_price(resources.misc_mat.backpacks.mat_type, mat_type, resources.misc_mat.backpacks.mat_index, mat_subtype,
                prices[df::entity_sell_category::Backpacks]); price != -1)
            return price;
        break;
    case df::item_type::QUIVER:
        if (int32_t price = get_price(resources.misc_mat.quivers.mat_type, mat_type, resources.misc_mat.quivers.mat_index, mat_subtype,
                prices[df::entity_sell_category::Quivers]); price != -1)
            return price;
        break;
    case df::item_type::TRAPCOMP:
        if (int32_t price = get_price(resources.trapcomp_type, mat_subtype, prices[df::entity_sell_category::TrapComponents]); price != -1)
            return price;
        break;
    case df::item_type::DRINK:
        if (int32_t price = get_price(resources.misc_mat.booze.mat_type, mat_type, resources.misc_mat.booze.mat_index, mat_subtype,
                prices[df::entity_sell_category::Drinks]); price != -1)
            return price;
        break;
    case df::item_type::POWDER_MISC:
        if (int32_t price = get_price(resources.misc_mat.powders.mat_type, mat_type, resources.misc_mat.powders.mat_index, mat_subtype,
                prices[df::entity_sell_category::Powders]); price != -1)
            return price;
        if (int32_t price = get_price(resources.misc_mat.sand.mat_type, mat_type, resources.misc_mat.sand.mat_index, mat_subtype,
                prices[df::entity_sell_category::Sand]); price != -1)
            return price;
        break;
    case df::item_type::CHEESE:
        if (int32_t price = get_price(resources.misc_mat.cheese.mat_type, mat_type, resources.misc_mat.cheese.mat_index, mat_subtype,
                prices[df::entity_sell_category::Cheese]); price != -1)
            return price;
        break;
    case df::item_type::LIQUID_MISC:
        if (int32_t price = get_price(resources.misc_mat.extracts.mat_type, mat_type, resources.misc_mat.extracts.mat_index, mat_subtype,
                prices[df::entity_sell_category::Extracts]); price != -1)
            return price;
        break;
    case df::item_type::SPLINT:
        if (int32_t price = get_price(resources.misc_mat.barrels.mat_type, mat_type, resources.misc_mat.barrels.mat_index, mat_subtype,
                prices[df::entity_sell_category::Splints]); price != -1)
            return price;
        break;
    case df::item_type::CRUTCH:
        if (int32_t price = get_price(resources.misc_mat.barrels.mat_type, mat_type, resources.misc_mat.barrels.mat_index, mat_subtype,
                prices[df::entity_sell_category::Crutches]); price != -1)
            return price;
        break;
    case df::item_type::TOOL:
        if (int32_t price = get_price(resources.tool_type, mat_subtype, prices[df::entity_sell_category::Tools]); price != -1)
            return price;
        break;
    case df::item_type::EGG:
        if (int32_t price = get_price(resources.egg_races, mat_type, resources.egg_castes, mat_subtype,
                prices[df::entity_sell_category::Eggs]); price != -1)
            return price;
        break;
    case df::item_type::SHEET:
        if (int32_t price = get_price(resources.organic.parchment.mat_type, mat_type, resources.organic.parchment.mat_index, mat_subtype,
                prices[df::entity_sell_category::Parchment]); price != -1)
            return price;
        break;
    default:
        break;
    }

    for (size_t idx = 0; idx < resources.wood_products.item_type.size(); ++idx) {
        if (resources.wood_products.item_type[idx] == item_type &&
                (resources.wood_products.item_subtype[idx] == -1 || resources.wood_products.item_subtype[idx] == item_subtype) &&
                resources.wood_products.material.mat_type[idx] == mat_type &&
                (resources.wood_products.material.mat_index[idx] == -1 || resources.wood_products.material.mat_index[idx] == mat_subtype) &&
                prices[df::entity_sell_category::Miscellaneous].size() > idx)
            return prices[df::entity_sell_category::Miscellaneous][idx];
    }

    return DEFAULT_AGREEMENT_MULTIPLIER;
}

static int32_t get_sell_request_multiplier(df::item *item, const df::caravan_state *caravan) {
    const df::entity_sell_prices *sell_prices = caravan->sell_prices;
    if (!sell_prices)
        return DEFAULT_AGREEMENT_MULTIPLIER;

    auto caravan_he = df::historical_entity::find(caravan->entity);
    if (!caravan_he)
        return DEFAULT_AGREEMENT_MULTIPLIER;

    return get_sell_request_multiplier(item, caravan_he->resources, &sell_prices->price[0]);
}

static int32_t get_sell_request_multiplier(df::unit *unit, const df::caravan_state *caravan) {
    const df::entity_sell_prices *sell_prices = caravan->sell_prices;
    if (!sell_prices)
        return DEFAULT_AGREEMENT_MULTIPLIER;

    auto caravan_he = df::historical_entity::find(caravan->entity);
    if (!caravan_he)
        return DEFAULT_AGREEMENT_MULTIPLIER;

    auto & resources = caravan_he->resources;
    int32_t price = get_price(resources.animals.pet_races, unit->race, resources.animals.pet_castes, unit->caste,
                              sell_prices->price[df::entity_sell_category::Pets]);
    return (price != -1) ? price : DEFAULT_AGREEMENT_MULTIPLIER;
}

static bool is_requested_trade_good(df::item *item, df::caravan_state *caravan) {
    auto trade_state = caravan->trade_state;
    if (caravan->time_remaining <= 0 ||
            (trade_state != df::caravan_state::T_trade_state::Approaching &&
                trade_state != df::caravan_state::T_trade_state::AtDepot))
        return false;
    return get_buy_request_multiplier(item, caravan->buy_prices) > DEFAULT_AGREEMENT_MULTIPLIER;
}

bool Items::isRequestedTradeGood(df::item *item, df::caravan_state *caravan) {
    if (caravan)
        return is_requested_trade_good(item, caravan);

    for (auto caravan : df::global::plotinfo->caravans) {
        auto trade_state = caravan->trade_state;
        if (caravan->time_remaining <= 0 ||
                (trade_state != df::caravan_state::T_trade_state::Approaching &&
                 trade_state != df::caravan_state::T_trade_state::AtDepot))
            continue;
        if (get_buy_request_multiplier(item, caravan->buy_prices) > DEFAULT_AGREEMENT_MULTIPLIER)
            return true;
    }
    return false;
}

int Items::getValue(df::item *item, df::caravan_state *caravan)
{
    CHECK_NULL_POINTER(item);

    int16_t item_type = item->getType();
    int16_t item_subtype = item->getSubtype();
    int16_t mat_type = item->getMaterial();
    int32_t mat_subtype = item->getMaterialIndex();

    // Get base value for item type, subtype, and material
    int value = getItemBaseValue(item_type, item_subtype, mat_type, mat_subtype);

    // entity value modifications
    value *= get_war_multiplier(item, caravan);
    value >>= 8;

    // Improve value based on quality
    switch (item->getQuality()) {
    case 1:
        value *= 1.1;
        value += 3;
        break;
    case 2:
        value *= 1.2;
        value += 6;
        break;
    case 3:
        value *= 1.333;
        value += 10;
        break;
    case 4:
        value *= 1.5;
        value += 15;
        break;
    case 5:
        value *= 2;
        value += 30;
        break;
    default:
        break;
    }

    // Add improvement values
    int impValue = item->getThreadDyeValue(caravan) + item->getImprovementsValue(caravan);
    if (item_type == item_type::AMMO) // Ammo improvements are worth less
        impValue /= 30;
    value += impValue;

    // Degrade value due to wear
    switch (item->getWear())
    {
    case 1:
        value = value * 3 / 4;
        break;
    case 2:
        value = value / 2;
        break;
    case 3:
        value = value / 4;
        break;
    }

    // Ignore value bonuses from magic, since that never actually happens

    // Artifacts have 10x value
    if (item->flags.bits.artifact_mood)
        value *= 10;

    // modify buy/sell prices
    if (caravan) {
        int32_t buy_multiplier = get_buy_request_multiplier(item, caravan->buy_prices);
        if (buy_multiplier == DEFAULT_AGREEMENT_MULTIPLIER) {
            value *= get_sell_request_multiplier(item, caravan);;
            value >>= 7;
        } else {
            value *= buy_multiplier;
            value >>= 7;
        }
    }

    // Boost value from stack size
    value *= item->getStackSize();
    // ...but not for coins
    if (item_type == item_type::COIN)
    {
        value /= 50;
        if (!value)
            value = 1;
    }

    // Handle vermin swarms
    if (item_type == item_type::VERMIN || item_type == item_type::PET)
    {
        int divisor = 1;
        auto creature = vector_get(world->raws.creatures.all, mat_type);
        if (creature) {
            size_t caste = std::max(0, mat_subtype);
            if (caste < creature->caste.size())
                divisor = creature->caste[caste]->misc.petvalue_divisor;
        }
        if (divisor > 1) {
            value /= divisor;
            if (!value)
                value = 1;
        }
    }

    // Add in value from units contained in cages
    if (item_type == item_type::CAGE) {
        for (auto gref : item->general_refs) {
            if (gref->getType() != df::general_ref_type::CONTAINS_UNIT)
                continue;
            auto unit = gref->getUnit();
            if (!unit)
                continue;
            df::creature_raw *raw = world->raws.creatures.all[unit->race];
            df::caste_raw *caste = raw->caste.at(unit->caste);
            int unit_value = caste->misc.petvalue;
            if (Units::isWar(unit) || Units::isHunter(unit))
                unit_value *= 2;
            if (caravan) {
                unit_value *= get_sell_request_multiplier(unit, caravan);
                unit_value >>= 7;
            }
            value += unit_value;
        }
    }

    return value;
}

bool Items::createItem(std::vector<df::item *> &out_items, df::unit* unit, df::item_type item_type, int16_t item_subtype, int16_t mat_type, int32_t mat_index, int32_t growth_print, bool no_floor) {
    //based on Quietust's plugins/createitem.cpp
    CHECK_NULL_POINTER(unit);
    df::coord pos = Units::getPosition(unit);
    df::map_block* block = Maps::getTileBlock(pos);
    CHECK_NULL_POINTER(block);
    auto prod = df::allocate<df::reaction_product_itemst>();
    prod->item_type = item_type;
    prod->item_subtype = item_subtype;
    prod->mat_type = mat_type;
    prod->mat_index = mat_index;
    prod->probability = 100;
    prod->count = 1;
    switch(item_type) {
    case df::item_type::BAR:
    case df::item_type::POWDER_MISC:
    case df::item_type::LIQUID_MISC:
    case df::item_type::DRINK:
        prod->product_dimension = 150;
        break;
    case df::item_type::THREAD:
        prod->product_dimension = 15000;
        break;
    case df::item_type::CLOTH:
        prod->product_dimension = 10000;
        break;
    default:
        prod->product_dimension = 1;
        break;
    }

    //makeItem
    vector<df::reaction_product*> out_products;
    vector<df::reaction_reagent*> in_reag;
    vector<df::item*> in_items;

    out_items.clear();
    prod->produce(unit, &out_products, &out_items, &in_reag, &in_items, 1, job_skill::NONE,
            0, df::historical_entity::find(unit->civ_id),
            World::isFortressMode() ? df::world_site::find(World::GetCurrentSiteId()) : NULL,
            NULL);
    delete prod;

    DEBUG(items).print("produced %zd items\n", out_items.size());

    for (size_t a = 0; a < out_items.size(); a++ ) {
        // Plant growths need a valid "growth print", otherwise they behave oddly
        auto growth = virtual_cast<df::item_plant_growthst>(out_items[a]);
        if (growth)
            growth->growth_print = growth_print;

        if (!no_floor)
            out_items[a]->moveToGround(pos.x, pos.y, pos.z);
    }

    return out_items.size() != 0;
}

/*
 * Trade Info
 */

bool Items::checkMandates(df::item *item)
{
    CHECK_NULL_POINTER(item);

    for (df::mandate *mandate : world->mandates)
    {
        if (mandate->mode != df::mandate::T_mode::Export)
            continue;

        if (item->getType() != mandate->item_type ||
            (mandate->item_subtype != -1 && item->getSubtype() != mandate->item_subtype))
            continue;

        if (mandate->mat_type != -1 && item->getMaterial() != mandate->mat_type)
            continue;

        if (mandate->mat_index != -1 && item->getMaterialIndex() != mandate->mat_index)
            continue;

        return false;
    }

    return true;
}

bool Items::canTrade(df::item *item)
{
    CHECK_NULL_POINTER(item);

    if (item->flags.bits.owned || item->flags.bits.artifact || item->flags.bits.spider_web || item->flags.bits.in_job)
        return false;

    for (df::general_ref *ref : item->general_refs)
    {
        switch (ref->getType())
        {
        case general_ref_type::UNIT_HOLDER:
            return false;

        case general_ref_type::BUILDING_HOLDER:
            return false;

        default:
            break;
        }
    }

    for (df::specific_ref *ref : item->specific_refs)
    {
        if (ref->type == specific_ref_type::JOB)
        {
            // Ignore any items assigned to a job
            return false;
        }
    }

    return checkMandates(item);
}

bool Items::canTradeWithContents(df::item *item)
{
    CHECK_NULL_POINTER(item);

    if (item->flags.bits.in_inventory)
        return false;

    if (!canTrade(item))
        return false;

    vector<df::item*> contained_items;
    getContainedItems(item, &contained_items);
    for (df::item *cit : contained_items)
    {
        if (!canTrade(cit))
            return false;
    }

    return true;
}

bool Items::canTradeAnyWithContents(df::item *item)
{
    CHECK_NULL_POINTER(item);

    if (item->flags.bits.in_inventory)
        return false;

    vector<df::item*> contained_items;
    getContainedItems(item, &contained_items);

    if (contained_items.empty())
        return canTrade(item);

    for (df::item *cit : contained_items) {
        if (canTrade(cit))
            return true;
    }

    return false;
}

bool Items::markForTrade(df::item *item, df::building_tradedepotst *depot) {
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(depot);

    // validate that the depot is in a good state
    if (depot->getBuildStage() < depot->getMaxBuildStage())
        return false;
    if (depot->jobs.size() && depot->jobs[0]->job_type == df::job_type::DestroyBuilding)
        return false;

    auto href = df::allocate<df::general_ref_building_holderst>();
    if (!href)
        return false;

    auto job = new df::job();
    job->job_type = df::job_type::BringItemToDepot;
    job->pos = df::coord(depot->centerx, depot->centery, depot->z);

    // job <-> item link
    if (!Job::attachJobItem(job, item, df::job_item_ref::Hauled)) {
        delete job;
        delete href;
        return false;
    }

    // job <-> building link
    href->building_id = depot->id;
    depot->jobs.push_back(job);
    job->general_refs.push_back(href);

    // add to job list
    Job::linkIntoWorld(job);

    return true;
}

// When called with game_ui = true, this is equivalent to bay12's itemst::meltable()
// (i.e., returning true if and only if the item has a "designate for melting" button in game)
bool Items::canMelt(df::item *item, bool game_ui)
{
    CHECK_NULL_POINTER(item);

    MaterialInfo mat(item);
    if (mat.getCraftClass() != df::craft_material_class::Metal)
        return false;

    switch(item->getType()) {
        // these are not meltable, even if made from metal
        case item_type::CORPSE:
        case item_type::CORPSEPIECE:
        case item_type::REMAINS:
        case item_type::FISH:
        case item_type::FISH_RAW:
        case item_type::VERMIN:
        case item_type::PET:
        case item_type::FOOD:
        case item_type::EGG:
            return false;
        default:
            break;
    }

    if (item->flags.bits.artifact)
        return false;

    // ignore checks below, when asked to behave like itemst::meltable()
    if (game_ui) return true;

    if (item->getType() == item_type::BAR)
        return false;

    // do not melt nonempty containers and items in unit inventories
    for (auto &g : item->general_refs) {
        switch (g->getType()) {
            case df::general_ref_type::CONTAINS_ITEM:
            case df::general_ref_type::UNIT_HOLDER:
            case df::general_ref_type::CONTAINS_UNIT:
                return false;
            case df::general_ref_type::CONTAINED_IN_ITEM:
            {
                df::item *c = g->getItem();
                for (auto &gg : c->general_refs) {
                    if (gg->getType() == df::general_ref_type::UNIT_HOLDER)
                        return false;
                }
                break;
            }
            default:
                break;
        }
    }

    return true;
};

bool Items::markForMelting(df::item *item)
{
    CHECK_NULL_POINTER(item);
    if (item->flags.bits.melt || !canMelt(item,true)) return false;
    insert_into_vector(world->items.other.ANY_MELT_DESIGNATED, &df::item::id, item);
    item->flags.bits.melt = 1;
    return true;
}

bool Items::cancelMelting(df::item *item)
{
    CHECK_NULL_POINTER(item);
    if (!item->flags.bits.melt) return false;
    erase_from_vector(world->items.other.ANY_MELT_DESIGNATED, &df::item::id, item->id);
    item->flags.bits.melt = 0;
    return true;
}

bool Items::isRouteVehicle(df::item *item)
{
    CHECK_NULL_POINTER(item);
    int id = item->getVehicleID();
    if (id < 0) return false;

    auto vehicle = df::vehicle::find(id);
    return vehicle && vehicle->route_id >= 0;
}

bool Items::isSquadEquipment(df::item *item)
{
    CHECK_NULL_POINTER(item);
    if (!plotinfo)
        return false;

    auto &vec = plotinfo->equipment.items_assigned[item->getType()];
    return binsearch_index(vec, item->id) >= 0;
}

// reverse engineered, code reference: 0x140953150 in 50.11-win64-steam
// our name for this function: itemst::getCapacity
// bay12 name for this function: not known

int32_t Items::getCapacity(df::item* item)
{
    CHECK_NULL_POINTER(item);

    switch (item->getType()) {
    case df::enums::item_type::FLASK:
    case df::enums::item_type::GOBLET:
        return 180;
    case df::enums::item_type::CAGE:
    case df::enums::item_type::BARREL:
    case df::enums::item_type::COFFIN:
    case df::enums::item_type::BOX:
    case df::enums::item_type::BAG:
    case df::enums::item_type::BIN:
    case df::enums::item_type::ARMORSTAND:
    case df::enums::item_type::WEAPONRACK:
    case df::enums::item_type::CABINET:
        return 6000;
    case df::enums::item_type::BUCKET:
        return 600;
    case df::enums::item_type::ANIMALTRAP:
    case df::enums::item_type::BACKPACK:
        return 3000;
    case df::enums::item_type::QUIVER:
        return 1200;
    case df::enums::item_type::TOOL:
        if (auto tool = virtual_cast<df::item_toolst>(item))
            return tool->subtype->container_capacity;
        // fall through
    default:
        ; // fall through to default exit
    }
    return 0;
}
