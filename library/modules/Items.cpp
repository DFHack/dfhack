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

using namespace DFHack;
using namespace df::enums;

#define ITEMDEF_VECTORS \
    ITEM(WEAPON, weapons) \
    ITEM(TRAPCOMP, trapcomps) \
    ITEM(TOY, toys) \
    ITEM(TOOL, tools) \
    ITEM(INSTRUMENT, instruments) \
    ITEM(ARMOR, armor) \
    ITEM(AMMO, ammo) \
    ITEM(SIEGEAMMO, siege_ammo) \
    ITEM(GLOVES, gloves) \
    ITEM(SHOES, shoes) \
    ITEM(SHIELD, shields) \
    ITEM(HELM, helms) \
    ITEM(PANTS, pants) \
    ITEM(FOOD, food)

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

#define ITEM(type,vec) \
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

std::string ItemTypeInfo::toString()
{
    std::string rv = ENUM_KEY_STR(item_type, type);
    if (custom)
        rv += ":" + custom->id;
    else if (subtype != -1)
        rv += stl_sprintf(":%d", subtype);
    return rv;
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
#define ITEM(type,vec) \
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


Module* DFHack::createItems()
{
    return new Items();
}

class Items::Private
{
    public:
        Process * owner;
        std::map<int32_t, df_item *> idLookupTable;
        uint32_t refVectorOffset;
        uint32_t idFieldOffset;
        void * itemVectorAddress;

        ClassNameCheck isOwnerRefClass;
        ClassNameCheck isContainerRefClass;
        ClassNameCheck isContainsRefClass;

        // Similar to isOwnerRefClass.  Value is unique to each creature, but
        // different than the creature's id.
        ClassNameCheck isUnitHolderRefClass;

        // One of these is present for each creature contained in a cage.
        // The value is similar to that for isUnitHolderRefClass, different
        // than the creature's ID but unique for each creature.
        ClassNameCheck isCagedUnitRefClass;

        // ID of bulding containing/holding the item.
        ClassNameCheck isBuildingHolderRefClass;

        // Building ID of lever/etc which triggers bridge/etc holding
        // this mechanism.
        ClassNameCheck isTriggeredByRefClass;

        // Building ID of bridge/etc which is triggered by lever/etc holding
        // this mechanism.
        ClassNameCheck isTriggerTargetRefClass;

        // Civilization ID of owner of item, for items not owned by the
        // fortress.
        ClassNameCheck isEntityOwnerRefClass;

        // Item has been offered to the caravan.  The value is the
        // civilization ID of
        ClassNameCheck isOfferedRefClass;

        // Item is in a depot for trade.  Purpose of value is unknown, but is
        // different for each item, even in the same depot at the same time.
        ClassNameCheck isTradingRefClass;

        // Item is flying or falling through the air.  The value seems to
        // be the ID for a "projectile information" object.
        ClassNameCheck isProjectileRefClass;

        std::set<std::string> knownItemRefTypes;
};

Items::Items()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->owner = c.p;

    DFHack::OffsetGroup* itemGroup = c.vinfo->getGroup("Items");
    d->itemVectorAddress = itemGroup->getAddress("items_vector");
    d->idFieldOffset = itemGroup->getOffset("id");
    d->refVectorOffset = itemGroup->getOffset("item_ref_vector");

    d->isOwnerRefClass = ClassNameCheck("general_ref_unit_itemownerst");
    d->isContainerRefClass = ClassNameCheck("general_ref_contained_in_itemst");
    d->isContainsRefClass = ClassNameCheck("general_ref_contains_itemst");
    d->isUnitHolderRefClass = ClassNameCheck("general_ref_unit_holderst");
    d->isCagedUnitRefClass = ClassNameCheck("general_ref_contains_unitst");
    d->isBuildingHolderRefClass
        = ClassNameCheck("general_ref_building_holderst");
    d->isTriggeredByRefClass = ClassNameCheck("general_ref_building_triggerst");
    d->isTriggerTargetRefClass
        = ClassNameCheck("general_ref_building_triggertargetst");
    d->isEntityOwnerRefClass = ClassNameCheck("general_ref_entity_itemownerst");
    d->isOfferedRefClass = ClassNameCheck("general_ref_entity_offeredst");
    d->isTradingRefClass = ClassNameCheck("general_ref_unit_tradebringerst");
    d->isProjectileRefClass = ClassNameCheck("general_ref_projectilest");

    std::vector<std::string> known_names;
    ClassNameCheck::getKnownClassNames(known_names);

    for (size_t i = 0; i < known_names.size(); i++)
    {
        if (known_names[i].find("general_ref_") == 0)
            d->knownItemRefTypes.insert(known_names[i]);
    }
}

bool Items::Start()
{
    d->idLookupTable.clear();
    return true;
}

bool Items::Finish()
{
    return true;
}

bool Items::readItemVector(std::vector<df_item *> &items)
{
    std::vector <df_item *> *p_items = (std::vector <df_item *> *) d->itemVectorAddress;

    d->idLookupTable.clear();
    items.resize(p_items->size());

    for (unsigned i = 0; i < p_items->size(); i++)
    {
        df_item * ptr = p_items->at(i);
        items[i] = ptr;
        d->idLookupTable[ptr->id] = ptr;
    }

    return true;
}

bool Items::readItemVectorSubset(std::vector<df_item *> &items, size_t offset, size_t maxsize)
{
    std::vector <df_item *> *p_items = (std::vector <df_item *> *) d->itemVectorAddress;

    // ensure copy size is within bounds of request and itemvector
    size_t outputsize = p_items->size() - offset;
    if (outputsize > maxsize)
    {
        outputsize = maxsize;
    }
    items.resize(outputsize);
    
    for (unsigned i = 0; i < items.size(); i++)
    {
        items[i] = p_items->at(i+offset);
    }

    return true;
}

df_item * Items::findItemByID(int32_t id)
{
    if (id < 0)
        return 0;

    if (d->idLookupTable.empty())
    {
        std::vector<df_item *> tmp;
        readItemVector(tmp);
    }

    return d->idLookupTable[id];
}

Items::~Items()
{
    Finish();
    delete d;
}

bool Items::copyItem(df_item * itembase, DFHack::dfh_item &item)
{
    if(!itembase)
        return false;
    df_item * itreal = (df_item *) itembase;
    item.origin = itembase;
    item.x = itreal->x;
    item.y = itreal->y;
    item.z = itreal->z;
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

int32_t Items::getItemOwnerID(const DFHack::df_item * item)
{
    std::vector<int32_t> vals;
    if (readItemRefs(item, d->isOwnerRefClass, vals))
        return vals[0];
    else
        return -1;
}

int32_t Items::getItemContainerID(const DFHack::df_item * item)
{
    std::vector<int32_t> vals;
    if (readItemRefs(item, d->isContainerRefClass, vals))
        return vals[0];
    else
        return -1;
}

bool Items::getContainedItems(const DFHack::df_item * item, std::vector<int32_t> &items)
{
    return readItemRefs(item, d->isContainsRefClass, items);
}

bool Items::readItemRefs(const df_item * item, const ClassNameCheck &classname, std::vector<int32_t> &values)
{
    const std::vector <t_itemref *> &p_refs = item->itemrefs;
    values.clear();

    for (uint32_t i=0; i<p_refs.size(); i++)
    {
        if (classname(d->owner, p_refs[i]->vptr))
            values.push_back(int32_t(p_refs[i]->value));
    }

    return !values.empty();
}

bool Items::unknownRefs(const df_item * item, std::vector<std::pair<std::string, int32_t> >& refs)
{
    refs.clear();

    const std::vector <t_itemref *> &p_refs = item->itemrefs;

    for (uint32_t i=0; i<p_refs.size(); i++)
    {
        std::string name = p_refs[i]->getClassName();

        if (d->knownItemRefTypes.find(name) == d->knownItemRefTypes.end())
        {
            refs.push_back(pair<string, int32_t>(name, p_refs[i]->value));
        }
    }

    return (refs.size() > 0);
}

bool Items::removeItemOwner(df_item * item, Units *creatures)
{
    std::vector <t_itemref *> &p_refs = item->itemrefs;
    for (uint32_t i=0; i<p_refs.size(); i++)
    {
        if (!d->isOwnerRefClass(d->owner, p_refs[i]->vptr))
            continue;

        int32_t & oid = p_refs[i]->value;
        int32_t ix = creatures->FindIndexById(oid);

        if (ix < 0 || !creatures->RemoveOwnedItemByIdx(ix, item->id))
        {
            cerr << "RemoveOwnedItemIdx: CREATURE " << ix << " ID " << item->id << " FAILED!" << endl;
            return false;
        }
        p_refs.erase(p_refs.begin() + i--);
    }

    item->flags.owned = 0;

    return true;
}

std::string Items::getItemClass(const df_item * item)
{
    const t_virtual * virt = (t_virtual *) item;
    return virt->getClassName();
}

