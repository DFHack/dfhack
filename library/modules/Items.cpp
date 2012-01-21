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
#include "ModuleFactory.h"
#include "Core.h"
#include "Virtual.h"
#include "MiscUtils.h"

#include "df/world.h"
#include "df/item.h"
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

using namespace DFHack;
using namespace DFHack::Simple;
using namespace df::enums;
using df::global::world;

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

    FOR_ENUM_ITEMS(item_type, i)
    {
        const char *key = ENUM_ATTR(item_type, key, i);
        if (key && key == items[0])
        {
            type = i;
            break;
        }
    }

    if (type == NONE)
        return false;
    if (items.size() == 1)
        return true;

    df::world_raws::T_itemdefs &defs = df::global::world->raws.itemdefs;

    switch (type) {
#define ITEM(type,vec,tclass) \
    case type: \
        for (int i = 0; i < defs.vec.size(); i++) { \
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
            df::enum_field<df::tool_uses,int16_t> key(tool_uses::FOOD_STORAGE);
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
        Core::getInstance().con.printerr("ItemTypeInfo.matches inconsistent\n");

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

int32_t Items::getItemOwnerID(const df::item * item)
{
    for (uint32_t i = 0; i < item->itemrefs.size(); i++)
    {
        df::general_ref *ref = item->itemrefs[i];
        if (ref->getType() == df::general_ref_type::UNIT_ITEMOWNER)
            return ref->getID();
    }
    return -1;
}

int32_t Items::getItemContainerID(const df::item * item)
{
    for (uint32_t i = 0; i < item->itemrefs.size(); i++)
    {
        df::general_ref *ref = item->itemrefs[i];
        if (ref->getType() == df::general_ref_type::CONTAINED_IN_ITEM)
            return ref->getID();
    }
    return -1;
}

bool Items::getContainedItems(const df::item * item, std::vector<int32_t> &items)
{
    return readItemRefs(item, df::general_ref_type::CONTAINS_ITEM, items);
}

bool Items::readItemRefs(const df::item * item, df::general_ref_type type, std::vector<int32_t> &values)
{
    values.clear();

    for (uint32_t i = 0; i < item->itemrefs.size(); i++)
    {
        df::general_ref *ref = item->itemrefs[i];
        if (ref->getType() == type)
            values.push_back(ref->getID());
    }

    return !values.empty();
}

bool Items::removeItemOwner(df::item * item, Units *creatures)
{
    for (uint32_t i = 0; i < item->itemrefs.size(); i++)
    {
        df::general_ref *ref = item->itemrefs[i];
        if (ref->getType() != df::general_ref_type::UNIT_ITEMOWNER)
            continue;

        df::unit *unit = ref->getUnit();

        if (unit == NULL || !creatures->RemoveOwnedItemByPtr(unit, item->id))
        {
            cerr << "RemoveOwnedItemIdx: CREATURE " << ref->getID() << " ID " << item->id << " FAILED!" << endl;
            return false;
        }
        delete ref;
        item->itemrefs.erase(item->itemrefs.begin() + i--);
    }

    item->flags.bits.owned = 0;

    return true;
}

std::string Items::getItemClass(const df::item * item)
{
    const t_virtual * virt = (t_virtual *) item;
    return virt->getClassName();
}

