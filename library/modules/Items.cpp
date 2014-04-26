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
#include <sstream>
#include <vector>
#include <cstdio>
#include <map>
#include <set>
using namespace std;

#include "Types.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "modules/Materials.h"
#include "modules/Items.h"
#include "modules/Units.h"
#include "modules/MapCache.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "Error.h"
#include "MiscUtils.h"

#include "df/ui.h"
#include "df/world.h"
#include "df/item.h"
#include "df/building.h"
#include "df/building_actual.h"
#include "df/tool_uses.h"
#include "df/itemdef_weaponst.h"
#include "df/itemdef_trapcompst.h"
#include "df/itemdef_toyst.h"
#include "df/itemdef_toolst.h"
#include "df/itemdef_instrumentst.h"
#include "df/itemdef_armorst.h"
#include "df/itemdef_ammost.h"
#include "df/itemdef_siegeammost.h"
#include "df/itemdef_glovesst.h"
#include "df/itemdef_shoesst.h"
#include "df/itemdef_shieldst.h"
#include "df/itemdef_helmst.h"
#include "df/itemdef_pantsst.h"
#include "df/itemdef_foodst.h"
#include "df/trapcomp_flags.h"
#include "df/job_item.h"
#include "df/general_ref.h"
#include "df/general_ref_unit_itemownerst.h"
#include "df/general_ref_contains_itemst.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_projectile.h"
#include "df/viewscreen_itemst.h"
#include "df/vermin.h"
#include "df/proj_itemst.h"
#include "df/proj_list_link.h"

#include "df/unit_inventory_item.h"
#include "df/body_part_raw.h"
#include "df/unit.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/body_part_template_flags.h"
#include "df/general_ref_unit_holderst.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::ui;
using df::global::ui_selected_unit;
using df::global::proj_next_id;

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
    else if (subtype != -1)
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

    return toLower(ENUM_KEY_STR(item_type, type));
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

bool ItemTypeInfo::matches(const df::job_item &item, MaterialInfo *mat, bool skip_vector)
{
    using namespace df::enums::item_type;

    if (!isValid())
        return mat ? mat->matches(item) : false;

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
    RQ(1,not_bin); RQ(1,lye_bearing);

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

bool Items::copyItem(df::item * itembase, DFHack::dfh_item &item)
{
    if(!itembase)
        return false;
    df::item * itreal = (df::item *) itembase;
    item.origin = itembase;
    item.x = itreal->pos.x;
    item.y = itreal->pos.y;
    item.z = itreal->pos.z;
    item.id = itreal->id;
    item.age = itreal->age;
    item.flags = itreal->flags;
    item.matdesc.item_type = itreal->getType();
    item.matdesc.item_subtype = itreal->getSubtype();
    item.matdesc.mat_type = itreal->getMaterial();
    item.matdesc.mat_index = itreal->getMaterialIndex();
    item.wear_level = itreal->getWear();
    item.quality = itreal->getQuality();
    item.quantity = itreal->getStackSize();
    return true;
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

        if (!strict_virtual_cast<df::general_ref_unit_itemownerst>(ref))
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
                return ref->vermin->pos;

            default:
                break;
            }
        }

        return df::coord();
    }

    return item->pos;
}

static char quality_table[] = { 0, '-', '+', '*', '=', '@' };

static void addQuality(std::string &tmp, int quality)
{
    if (quality > 0 && quality <= 5) {
        char c = quality_table[quality];
        tmp = c + tmp + c;
    }
}

std::string Items::getDescription(df::item *item, int type, bool decorate)
{
    CHECK_NULL_POINTER(item);

    std::string tmp;
    item->getItemDescription(&tmp, type);

    if (decorate) {
        if (item->flags.bits.foreign)
            tmp = "(" + tmp + ")";

        addQuality(tmp, item->getQuality());

        if (item->isImproved()) {
            tmp = "<" + tmp + ">";
            addQuality(tmp, item->getImprovementQuality());
        }
    }

    return tmp;
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

    for (size_t i = 0; i < item->general_refs.size(); i++)
    {
        df::general_ref *ref = item->general_refs[i];

        switch (ref->getType())
        {
        case general_ref_type::PROJECTILE:
        case general_ref_type::BUILDING_HOLDER:
        case general_ref_type::BUILDING_CAGED:
        case general_ref_type::BUILDING_TRIGGER:
        case general_ref_type::BUILDING_TRIGGERTARGET:
        case general_ref_type::BUILDING_CIVZONE_ASSIGNED:
            return false;

        default:
            continue;
        }
    }

    if (item->flags.bits.on_ground)
    {
        if (!mc.removeItemOnGround(item))
            Core::printerr("Item was marked on_ground, but not in block: %d (%d,%d,%d)\n",
                           item->id, item->pos.x, item->pos.y, item->pos.z);

        item->flags.bits.on_ground = false;
        return true;
    }
    else if (item->flags.bits.in_inventory)
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
                    // Viewscreens hold general_ref_contains_itemst pointers
                    for (auto screen = Core::getTopViewscreen(); screen; screen = screen->parent)
                    {
                        auto vsitem = strict_virtual_cast<df::viewscreen_itemst>(screen);
                        if (vsitem && vsitem->item == item2)
                            return false;
                    }

                    item2->flags.bits.weight_computed = false;

                    removeRef(item2->general_refs, general_ref_type::CONTAINS_ITEM, item->id);
                }
                break;

            case general_ref_type::UNIT_HOLDER:
                if (auto unit = ref->getUnit())
                {
                    // Unit view sidebar holds inventory item pointers
                    if (ui->main.mode == ui_sidebar_mode::ViewUnits &&
                        (!ui_selected_unit ||
                         vector_get(world->units.active, *ui_selected_unit) == unit))
                        return false;

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
    else if (item->flags.bits.removed)
    {
        item->flags.bits.removed = false;

        if (item->flags.bits.garbage_collect)
        {
            item->flags.bits.garbage_collect = false;
            item->categorize(true);
        }

        return true;
    }
    else
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

bool DFHack::Items::moveToBuilding(MapExtras::MapCache &mc, df::item *item, df::building_actual *building,int16_t use_mode)
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

    if (!detachItem(mc, item))
    {
        delete ref;
        return NULL;
    }

    item->pos = pos;
    item->flags.bits.in_job = true;

    auto proj = new df::proj_itemst();
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
