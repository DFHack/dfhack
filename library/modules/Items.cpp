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

#include "df/world.h"
#include "df/item.h"
#include "df/building.h"
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

const int const_GloveRightHandedness = 1;
const int const_GloveLeftHandedness = 2;

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

bool ItemTypeInfo::decode(df::item_type type_, int16_t subtype_)
{
    using namespace df::enums::item_type;

    type = type_;
    subtype = subtype_;
    custom = NULL;

    df::world_raws::T_itemdefs &defs = df::global::world->raws.itemdefs;

    switch (type_) {
    case NONE:
        return false;

#define ITEM(type,vec,tclass) \
    case type: \
        custom = vector_get(defs.vec, subtype); \
        break;
ITEMDEF_VECTORS
#undef ITEM

    default:
        break;
    }

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

bool ItemTypeInfo::matches(const df::job_item &item, MaterialInfo *mat)
{
    using namespace df::enums::item_type;

    if (!isValid())
        return mat ? mat->matches(item) : false;

    df::job_item_flags1 ok1, mask1, item_ok1, item_mask1;
    df::job_item_flags2 ok2, mask2, item_ok2, item_mask2;
    df::job_item_flags3 ok3, mask3, item_ok3, item_mask3;

    ok1.whole = mask1.whole = item_ok1.whole = item_mask1.whole = 0;
    ok2.whole = mask2.whole = item_ok2.whole = item_mask2.whole = 0;
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
    RQ(2,building_material); RQ(2,fire_safe); RQ(2,magma_safe); RQ(2,non_economic);
    RQ(2,totemable); RQ(2,plaster_containing); RQ(2,body_part); RQ(2,lye_milk_free);
    RQ(2,blunt); RQ(2,unengraved); RQ(2,hair_wool);

    RQ(3,any_raw_material); RQ(3,non_pressed); RQ(3,food_storage);

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
        OK(2,non_economic);
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
        break;

    case BUCKET:
    case FLASK:
        OK(1,milk);
        break;

    case TOOL:
        OK(1,lye_bearing);
        OK(1,milk);
        OK(2,lye_milk_free);
        OK(2,blunt);

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
        break;

    case BOX:
        OK(1,bag); OK(1,sand_bearing); OK(1,milk);
        OK(2,dye); OK(2,plaster_containing);
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
    item.matdesc.itemType = itreal->getType();
    item.matdesc.subType = itreal->getSubtype();
    item.matdesc.material = itreal->getMaterial();
    item.matdesc.index = itreal->getMaterialIndex();
    item.wear_level = itreal->getWear();
    item.quality = itreal->getQuality();
    item.quantity = itreal->getStackSize();
    return true;
}

df::general_ref *Items::getGeneralRef(df::item *item, df::general_ref_type type)
{
    CHECK_NULL_POINTER(item);

    return findRef(item->itemrefs, type);
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

    for (int i = item->itemrefs.size()-1; i >= 0; i--)
    {
        df::general_ref *ref = item->itemrefs[i];

        if (!strict_virtual_cast<df::general_ref_unit_itemownerst>(ref))
            continue;

        if (auto cur = ref->getUnit())
        {
            if (cur == unit)
                return true;

            erase_from_vector(cur->owned_items, item->id);
        }

        delete ref;
        vector_erase_at(item->itemrefs, i);
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
        item->itemrefs.push_back(ref);
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

    for (size_t i = 0; i < item->itemrefs.size(); i++)
    {
        df::general_ref *ref = item->itemrefs[i];
        if (ref->getType() != general_ref_type::CONTAINS_ITEM)
            continue;

        auto child = ref->getItem();
        if (child)
            items->push_back(child);
    }
}

df::coord Items::getPosition(df::item *item)
{
    CHECK_NULL_POINTER(item);

    if (item->flags.bits.in_inventory ||
        item->flags.bits.in_chest ||
        item->flags.bits.in_building)
    {
        for (size_t i = 0; i < item->itemrefs.size(); i++)
        {
            df::general_ref *ref = item->itemrefs[i];

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

            case general_ref_type::BUILDING_HOLDER:
                if (auto bld = ref->getBuilding())
                    return df::coord(bld->centerx, bld->centery, bld->z);
                break;

            default:
                break;
            }
        }
    }

    return item->pos;
}

static bool detachItem(MapExtras::MapCache &mc, df::item *item)
{
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

        for (int i = item->itemrefs.size()-1; i >= 0; i--)
        {
            df::general_ref *ref = item->itemrefs[i];

            switch (ref->getType())
            {
            case general_ref_type::CONTAINED_IN_ITEM:
                if (auto item2 = ref->getItem())
                {
                    item2->flags.bits.weight_computed = false;

                    removeRef(item2->itemrefs, general_ref_type::CONTAINS_ITEM, item->id);
                }
                break;

            case general_ref_type::UNIT_HOLDER:
                // Remove the item from the unit's inventory
                for (int inv = 0; inv < ref->getUnit()->inventory.size(); inv++)
                {
                    df::unit_inventory_item * currInvItem = ref->getUnit()->inventory.at(inv);
                    if(currInvItem->item->id == item->id)
                    {
                        // Match found; remove it
                        ref->getUnit()->inventory.erase(ref->getUnit()->inventory.begin() + inv);
                        // No other pointers to this object should exist; delete it to prevent memleak
                        delete currInvItem;
                        // Note: we might expect to recalculate the unit's weight at this point, in order to account for the 
                        // detached item.  In fact, this recalculation occurs automatically during each dwarf's "turn".
                        // The slight delay in recalculation is probably not worth worrying about.
                        
                        // Since no item will ever occur twice in a unit's inventory, further searching is unnecessary.
                        break;
                    }
                }
                break;
            case general_ref_type::BUILDING_HOLDER:
                return false;

            default:
                continue;
            }

            found = true;
            vector_erase_at(item->itemrefs, i);
            delete ref;
        }

        if (!found)
            return false;

        item->flags.bits.in_inventory = false;
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

    if (!detachItem(mc, item))
        return false;

    auto ref1 = df::allocate<df::general_ref_contains_itemst>();
    auto ref2 = df::allocate<df::general_ref_contained_in_itemst>();

    if (!ref1 || !ref2)
    {
        delete ref1; delete ref2;
        Core::printerr("Could not allocate container refs.\n");
        putOnGround(mc, item, getPosition(container));
        return false;
    }

    item->pos = container->pos;
    item->flags.bits.in_inventory = true;
    container->flags.bits.container = true;

    container->flags.bits.weight_computed = false;

    ref1->item_id = item->id;
    container->itemrefs.push_back(ref1);
    ref2->item_id = container->id;
    item->itemrefs.push_back(ref2);

    return true;
}

bool DFHack::Items::moveToInventory(MapExtras::MapCache &mc, df::item *item, df::unit *unit, df::body_part_raw * targetBodyPart, bool ignoreRestrictions, int multiEquipLimit, bool verbose)
{
    // Step 1: Check for anti-requisite conditions
    df::unit * itemOwner = Items::getOwner(item);
    if (ignoreRestrictions) 
    {
        // If the ignoreRestrictions cmdline switch was specified, then skip all of the normal preventative rules
        if (verbose) { Core::print("Skipping integrity checks...\n"); }
    }
    else if(!item->isClothing() && !item->isArmorNotClothing())
    {
        if (verbose) { Core::printerr("Item %d is not clothing or armor; it cannot be equipped.  Please choose a different item (or use the Ignore option if you really want to equip an inappropriate item).\n", item->id); }
        return false;
    }
    else if (!item->getType() == df::enums::item_type::GLOVES &&
        !item->getType() == df::enums::item_type::HELM && 
        !item->getType() == df::enums::item_type::ARMOR && 
        !item->getType() == df::enums::item_type::PANTS &&
        !item->getType() == df::enums::item_type::SHOES &&
        !targetBodyPart)
    {
        if (verbose) { Core::printerr("Item %d is of an unrecognized type; it cannot be equipped (because the module wouldn't know where to put it).\n", item->id); }
        return false;
    }
    else if (itemOwner && itemOwner->id != unit->id)
    {
        if (verbose) { Core::printerr("Item %d is owned by someone else.  Equipping it on this unit is not recommended.  Please use DFHack's Confiscate plugin, choose a different item, or use the Ignore option to proceed in spite of this warning.\n", item->id); }
        return false;
    }
    else if (item->flags.bits.in_inventory)
    {
        if (verbose) { Core::printerr("Item %d is already in a unit's inventory.  Direct inventory transfers are not recommended; please move the item to the ground first (or use the Ignore option).\n", item->id); }
        return false;
    }
    else if (item->flags.bits.in_job)
    {
        if (verbose) { Core::printerr("Item %d is reserved for use in a queued job.  Equipping it is not recommended, as this might interfere with the completion of vital jobs.  Use the Ignore option to ignore this warning.\n", item->id); }
        return false;
    }

    // ASSERT: anti-requisite conditions have been satisfied (or disregarded)


    // Step 2: Try to find a bodypart which is eligible to receive equipment AND which is appropriate for the specified item
    df::body_part_raw * confirmedBodyPart = NULL;
    int bpIndex;
    for(bpIndex = 0; bpIndex < unit->body.body_plan->body_parts.size(); bpIndex++)
    {
        df::body_part_raw * currPart = unit->body.body_plan->body_parts[bpIndex];

        // Short-circuit the search process if a BP was specified in the function call
        // Note: this causes a bit of inefficient busy-looping, but the search space is tiny (<100) and we NEED to get the correct bpIndex value in order to perform inventory manipulations
        if (!targetBodyPart)
        {
            // The function call did not specify any particular body part; proceed with normal iteration and evaluation of BP eligibility
        }
        else if (currPart == targetBodyPart)
        {
            // A specific body part was included in the function call, and we've found it; proceed with the normal BP evaluation (suitability, emptiness, etc)
        }
        else if (bpIndex < unit->body.body_plan->body_parts.size())
        {
            // The current body part is not the one that was specified in the function call, but we can keep searching
            if (verbose) { Core::printerr("Found bodypart %s; not a match; continuing search.\n", currPart->part_code.c_str()); }
            continue;
        }
        else 
        {
            // The specified body part has not been found, and we've reached the end of the list.  Report failure.
            if (verbose) { Core::printerr("The specified body part (%s) does not belong to the chosen unit.  Please double-check to ensure that your spelling is correct, and that you have not chosen a dismembered bodypart.\n"); }
            return false;
        }

        if (verbose) { Core::print("Inspecting bodypart %s.\n", currPart->part_code.c_str()); }

        // Inspect the current bodypart
        if (item->getType() == df::enums::item_type::GLOVES && currPart->flags.is_set(df::body_part_template_flags::GRASP) &&
            ((item->getGloveHandedness() == const_GloveLeftHandedness && currPart->flags.is_set(df::body_part_template_flags::LEFT)) ||
            (item->getGloveHandedness() == const_GloveRightHandedness && currPart->flags.is_set(df::body_part_template_flags::RIGHT))))
        {
            if (verbose) { Core::print("Hand found (%s)...", currPart->part_code.c_str()); }
        }
        else if (item->getType() == df::enums::item_type::HELM && currPart->flags.is_set(df::body_part_template_flags::HEAD))
        {
            if (verbose) { Core::print("Head found (%s)...", currPart->part_code.c_str()); }
        }
        else if (item->getType() == df::enums::item_type::ARMOR && currPart->flags.is_set(df::body_part_template_flags::UPPERBODY))
        {
            if (verbose) { Core::print("Upper body found (%s)...", currPart->part_code.c_str()); }
        }
        else if (item->getType() == df::enums::item_type::PANTS && currPart->flags.is_set(df::body_part_template_flags::LOWERBODY))
        {
            if (verbose) { Core::print("Lower body found (%s)...", currPart->part_code.c_str()); }
        }
        else if (item->getType() == df::enums::item_type::SHOES && currPart->flags.is_set(df::body_part_template_flags::STANCE))
        {
            if (verbose) { Core::print("Foot found (%s)...", currPart->part_code.c_str()); }
        }
        else if (targetBodyPart && ignoreRestrictions)
        {
            // The BP in question would normally be considered ineligible for equipment.  But since it was deliberately specified by the user, we'll proceed anyways.
            if (verbose) { Core::print("Non-standard bodypart found (%s)...", targetBodyPart->part_code.c_str()); }
        }
        else if (targetBodyPart)
        {
            // The BP in question is not eligible for equipment and the ignore flag was not specified.  Report failure.
            if (verbose) { Core::printerr("Non-standard bodypart found, but it is ineligible for standard equipment.  Use the Ignore flag to override this warning.\n"); }
            return false;
        }
        else
        {
            if (verbose) { Core::print("Skipping ineligible bodypart.\n"); }
            // This body part is not eligible for the equipment in question; skip it
            continue;
        }

        // ASSERT: The current body part is able to support the specified equipment (or the test has been overridden).  Check whether it is currently empty/available.

        if (multiEquipLimit == INT_MAX)
        {
            // Note: this loop/check is skipped if the MultiEquip option is specified; we'll simply add the item to the bodyPart even if it's already holding a dozen gloves, shoes, and millstones (or whatever)
            if (verbose) { Core::print(" inventory checking skipped..."); }
            confirmedBodyPart = currPart;
            break;
        }
        else
        {
            confirmedBodyPart = currPart;        // Assume that the bodypart is valid; we'll invalidate it if we detect too many collisions while looping
            int collisions = 0;
            for (int inventoryID=0; inventoryID < unit->inventory.size(); inventoryID++)
            {
                df::unit_inventory_item * currInvItem = unit->inventory[inventoryID];
                if (currInvItem->body_part_id == bpIndex)
                {
                    // Collision detected; have we reached the limit?
                    if (++collisions >= multiEquipLimit)
                    {
                        if (verbose) { Core::printerr(" but it already carries %d piece(s) of equipment.  Either remove the existing equipment or use the Multi option.\n", multiEquipLimit); }                    
                        confirmedBodyPart = NULL;
                        break;
                    }
                }
            }

            if (confirmedBodyPart) 
            {
                // Match found; no need to examine any other BPs
                if (verbose) { Core::print(" eligibility confirmed..."); }
                break;
            }
            else if (!targetBodyPart)
            {
                // This body part is not eligible to receive the specified equipment; return to the loop and check the next BP
                continue;
            }
            else
            {
                // A specific body part was designated in the function call, but it was found to be ineligible.
                // Don't return to the BP loop; just fall-through to the failure-reporting code a few lines below.
                break;
            }
        }
    }

    if (!confirmedBodyPart) {
        // No matching body parts found; report failure
        if (verbose) { Core::printerr("\nThe item could not be equipped because the relevant body part(s) of the unit are missing or already occupied.  Try again with the Multi option if you're like to over-equip a body part, or choose a different unit-item combination (e.g. stop trying to put shoes on a trout).\n" ); }
        return false;
    }

    // ASSERT: We've found a bodypart which is suitable for the designated item and ready to receive it (or overridden the relevant checks)

    // Step 3: Perform the manipulations
    if (verbose) { Core::print("equipping item..."); }
    // 3a: attempt to release the item from its current position
    if (!detachItem(mc, item))
    {
        if (verbose) { Core::printerr("\nEquipping failed - failed to retrieve item from its current location/container/inventory.  Please move it to the ground and try again.\n"); }
        return false;
    }
    // 3b: register the item in the unit's inventory
    df::unit_inventory_item * newInventoryItem = df::allocate<df::unit_inventory_item>();
    newInventoryItem->body_part_id = bpIndex;
    newInventoryItem->item = item;
    newInventoryItem->mode = newInventoryItem->Worn;
    unit->inventory.push_back(newInventoryItem);
    item->flags.bits.in_inventory = true;

    // 3c: register a "unit holds item" relationship at the item level
    df::general_ref_unit_holderst * holderReference = df::allocate<df::general_ref_unit_holderst>();
    holderReference->setID(unit->id);
    item->itemrefs.push_back(holderReference);

    // 3d: tell the unit to begin "using" the item (note: if this step is skipped then the unit may not gain any actual protection from its armour)
    if (item->isClothing() || item->isArmorNotClothing()) {
        df::unit::T_used_items * newUsedItem = df::allocate<df::unit::T_used_items>();
        newUsedItem->id = item->id;
        unit->used_items.push_back(newUsedItem);
        if (verbose) { Core::print("Item is clothing/armor; protection aspects have been enabled.\n"); }
    }
    else
    {
        if (verbose) { Core::print("Item is neither clothing nor armor; unit has NOT been instructed to \"use\" it as such.\n"); }
    }

    // 3e: Remove the item from its current location (note: if this step is skipped then the item will appear BOTH on the ground and in the unit's inventory)
    mc.removeItemOnGround(item);

    if (verbose) { Core::print("  Success!\n"); }
    return true;
}