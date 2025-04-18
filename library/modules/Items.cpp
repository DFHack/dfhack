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
#include "ModuleFactory.h"
#include "Types.h"
#include "VersionInfo.h"

#include "modules/Buildings.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Materials.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/artifact_record.h"
#include "df/building.h"
#include "df/building_actual.h"
#include "df/building_tradedepotst.h"
#include "df/buildingitemst.h"
#include "df/builtin_mats.h"
#include "df/caravan_state.h"
#include "df/creature_raw.h"
#include "df/dfhack_material_category.h"
#include "df/d_init.h"
#include "df/entity_buy_prices.h"
#include "df/entity_buy_requests.h"
#include "df/entity_raw.h"
#include "df/entity_sell_category.h"
#include "df/entity_sell_prices.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/general_ref_contains_itemst.h"
#include "df/general_ref_projectile.h"
#include "df/general_ref_unit_holderst.h"
#include "df/general_ref_unit_itemownerst.h"
#include "df/historical_entity.h"
#include "df/item.h"
#include "df/item_bookst.h"
#include "df/item_magicalst.h"
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
#include "df/plant_raw.h"
#include "df/plant_growth.h"
#include "df/plant_growth_print.h"
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
#include "df/world.h"
#include "df/world_site.h"
#include "df/written_content.h"

#include <string>
#include <vector>

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;
using df::global::world;

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

bool ItemTypeInfo::decode(df::item_type type_, int16_t subtype_) {
    type = type_;
    subtype = subtype_;
    custom = Items::getSubtypeDef(type_, subtype_);

    return isValid();
}

bool ItemTypeInfo::decode(df::item *ptr) {
    if (ptr)
        return decode(ptr->getType(), ptr->getSubtype());
    return decode(item_type::NONE);
}

string ItemTypeInfo::getToken() {
    string rv = ENUM_KEY_STR(item_type, type);
    if (custom)
        rv += ":" + custom->id;
    else if (subtype != -1 && type != item_type::PLANT_GROWTH)
        rv += stl_sprintf(":%d", subtype);
    return rv;
}

string ItemTypeInfo::toString()
{   using namespace df::enums::item_type;
    switch (type)
    {
#define ITEM(type,vec,tclass) \
    case type: \
        if (VIRTUAL_CAST_VAR(cv, df::tclass, custom)) \
            return cv->name;
ITEMDEF_VECTORS
#undef ITEM

    default:
        break;
    }

    if (const char *name = ENUM_ATTR(item_type, caption, type))
        return name;
    return toLower_cp437(ENUM_KEY_STR(item_type, type));
}

bool ItemTypeInfo::find(const string &token)
{   using namespace df::enums::item_type;
    vector<string> items;
    split_string(&items, token, ":");

    type = NONE;
    subtype = -1;
    custom = NULL;

    if (items.size() < 1 || items.size() > 2)
        return false;
    if (items[0] == "NONE")
        return true;
    if (!find_enum_item(&type, items[0]) || type == NONE)
        return false;
    if (items.size() == 1)
        return true;

    auto &defs = world->raws.itemdefs;
    switch (type)
    {
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

bool ItemTypeInfo::matches(df::job_item_vector_id vec_id) {
    auto other_id = ENUM_ATTR(job_item_vector_id, other, vec_id);

    auto explicit_item = ENUM_ATTR(items_other_id, item, other_id);
    if (explicit_item != item_type::NONE && type != explicit_item)
        return false;

    auto generic_item = ENUM_ATTR(items_other_id, generic_item, other_id);
    if (generic_item.size > 0) {
        for (size_t i = 0; i < generic_item.size; i++)
            if (generic_item.items[i] == type)
                return true;
        return false;
    }
    return true;
}

bool ItemTypeInfo::matches(const df::job_item &jitem, MaterialInfo *mat,
                           bool skip_vector, df::item_type itype)
{   using namespace df::enums::item_type;
    if (!isValid())
        return mat ? mat->matches(jitem, itype) : false;

    if (Items::isCasteMaterial(type) && mat && !mat->isNone())
        return false;
    if (!skip_vector && !matches(jitem.vector_id))
        return false;

    df::job_item_flags1 ok1, mask1, item_ok1, item_mask1, xmask1;
    df::job_item_flags2 ok2, mask2, item_ok2, item_mask2, xmask2;
    df::job_item_flags3 ok3, mask3, item_ok3, item_mask3;

    if (mat) {
        mat->getMatchBits(ok1, mask1);
        mat->getMatchBits(ok2, mask2);
        mat->getMatchBits(ok3, mask3);
    }

#define OK(id,name) item_ok##id.bits.name = true
#define RQ(id,name) item_mask##id.bits.name = true

    // Set up the flag mask
    RQ(1,millable); RQ(1,sharpenable); RQ(1,processable);
    RQ(1,extract_bearing_plant); RQ(1,extract_bearing_fish); RQ(1,extract_bearing_vermin);
    RQ(1,processable_to_vial); RQ(1,processable_to_barrel);
    RQ(1,solid); RQ(1,tameable_vermin); RQ(1,sand_bearing); RQ(1,milk); RQ(1,milkable);
    RQ(1,not_bin); RQ(1,lye_bearing); RQ(1, undisturbed);

    RQ(2,dye); RQ(2,dyeable); RQ(2,dyed); RQ(2,glass_making); RQ(2,screw);
    RQ(2,building_material); RQ(2,fire_safe); RQ(2,magma_safe);
    RQ(2,plant); RQ(2,silk); RQ(2,yarn);
    RQ(2,totemable); RQ(2,plaster_containing); RQ(2,body_part); RQ(2,lye_milk_free);
    RQ(2,blunt); RQ(2,unengraved); RQ(2,hair_wool);

    RQ(3,any_raw_material); RQ(3,non_pressed); RQ(3,food_storage);

    // Only checked if boulder
    xmask2.bits.non_economic = true;

    // Compute the "ok" mask
    OK(1,solid);
    OK(1,not_bin);

    // TODO: furniture, ammo, finished good, craft
    switch (type)
    {
        case PLANT:
            OK(1,millable); OK(1,processable);
            OK(1,extract_bearing_plant);
            OK(1,processable_to_vial);
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
                if (linear_index(def->tool_use, tool_uses::FOOD_STORAGE) >= 0)
                    OK(3,food_storage);
            }
            else
                OK(3,food_storage);
            break;
        case BARREL:
            OK(1,lye_bearing);
            OK(1,milk);
            OK(2,lye_milk_free);
            OK(3,food_storage);
            xmask1.bits.cookable = true;
            break;
        case BAG:
            OK(1,sand_bearing); OK(1,milk);
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
            if ((!jitem.flags2.bits.plant && ok2.bits.plant) ||
                (!jitem.flags2.bits.silk && ok2.bits.silk) ||
                (!jitem.flags2.bits.yarn && ok2.bits.yarn))
            {   // Material flags must match exactly
                return false;
            }
            OK(1,undisturbed);
            OK(2,plant);
            OK(2,silk);
            OK(2,yarn);
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
            }
            else
                OK(2,screw);
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
        (item_ok3.whole & ~item_mask3.whole)
    )
        Core::printerr("ItemTypeInfo.matches inconsistent\n");
#undef OK
#undef RQ

    mask1.whole &= ~xmask1.whole;
    mask2.whole &= ~xmask2.whole;

    return bits_match(jitem.flags1.whole, ok1.whole, mask1.whole)
        && bits_match(jitem.flags2.whole, ok2.whole, mask2.whole)
        && bits_match(jitem.flags3.whole, ok3.whole, mask3.whole)
        && bits_match(jitem.flags1.whole, item_ok1.whole, item_mask1.whole)
        && bits_match(jitem.flags2.whole, item_ok2.whole, item_mask2.whole)
        && bits_match(jitem.flags3.whole, item_ok3.whole, item_mask3.whole);
}

bool Items::isCasteMaterial(df::item_type itype) {
    return ENUM_ATTR(item_type, is_caste_mat, itype);
}

int Items::getSubtypeCount(df::item_type itype)
{   using namespace df::enums::item_type;
    auto &defs = world->raws.itemdefs;

    switch (itype)
    {
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
{   using namespace df::enums::item_type;
    auto &defs = world->raws.itemdefs;

    switch (itype)
    {
#define ITEM(type,vec,tclass) \
    case type: \
        return vector_get(defs.vec, subtype);
ITEMDEF_VECTORS
#undef ITEM

    default:
        return NULL;
    }
}
#undef ITEMDEF_VECTORS

df::item *Items::findItemByID(int32_t id) {
    return id < 0 ? NULL : df::item::find(id);
}

df::general_ref *Items::getGeneralRef(df::item *item, df::general_ref_type type) {
    CHECK_NULL_POINTER(item);
    return findRef(item->general_refs, type);
}

df::specific_ref *Items::getSpecificRef(df::item *item, df::specific_ref_type type) {
    CHECK_NULL_POINTER(item);
    return findRef(item->specific_refs, type);
}

df::unit *Items::getOwner(df::item *item) {
    auto ref = getGeneralRef(item, general_ref_type::UNIT_ITEMOWNER);
    return ref ? ref->getUnit() : NULL;
}

bool Items::setOwner(df::item *item, df::unit *unit) {
    CHECK_NULL_POINTER(item);

    for (int i = item->general_refs.size()-1; i >= 0; i--)
    {
        auto ref = item->general_refs[i];
        if (ref->getType() != general_ref_type::UNIT_ITEMOWNER)
            continue;

        if (auto cur = ref->getUnit()) {
            if (cur == unit)
                return true;
            erase_from_vector(cur->owned_items, item->id);
        }

        vector_erase_at(item->general_refs, i);
        delete ref;
    }

    item->flags.bits.owned = false;

    if (unit) {
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

df::item *Items::getContainer(df::item *item) {
    auto ref = getGeneralRef(item, general_ref_type::CONTAINED_IN_ITEM);
    return ref ? ref->getItem() : NULL;
}

void Items::getOuterContainerRef(df::specific_ref &spec_ref, df::item *item, bool init_ref)
{   // Reverse-engineered from ambushing unit code.
    CHECK_NULL_POINTER(item);
    if (init_ref) {
        spec_ref.type = specific_ref_type::ITEM_GENERAL;
        spec_ref.data.object = item;
    }

    if (item->flags.bits.removed || !item->flags.bits.in_inventory)
        return;

    for (auto gref : item->general_refs)
    {
        switch (gref->getType())
        {
            case general_ref_type::CONTAINED_IN_ITEM:
                if (auto item2 = gref->getItem())
                    return Items::getOuterContainerRef(spec_ref, item2);
                break;
            case general_ref_type::UNIT_HOLDER:
                if (auto unit = gref->getUnit())
                    return Units::getOuterContainerRef(spec_ref, unit);
                break;
            default:
                break;
        }
    }

    if (auto sref = getSpecificRef(item, specific_ref_type::VERMIN_ESCAPED_PET)) {
        spec_ref.type = specific_ref_type::VERMIN_EVENT;
        spec_ref.data.vermin = sref->data.vermin;
    }
    return;
}

void Items::getContainedItems(df::item *item, vector<df::item *> *items) {
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(items);
    items->clear();

    for (auto gref : item->general_refs) {
        if (gref->getType() != general_ref_type::CONTAINS_ITEM)
            continue;
        else if (auto child = gref->getItem())
            items->push_back(child);
    }
}

df::building *Items::getHolderBuilding(df::item *item) {
    auto ref = getGeneralRef(item, general_ref_type::BUILDING_HOLDER);
    return ref ? ref->getBuilding() : NULL;
}

df::unit *Items::getHolderUnit(df::item *item) {
    auto ref = getGeneralRef(item, general_ref_type::UNIT_HOLDER);
    return ref ? ref->getUnit() : NULL;
}

df::coord Items::getPosition(df::item *item) {
    CHECK_NULL_POINTER(item);
    // Function reverse-engineered from DF code.
    if (item->flags.bits.removed)
        return df::coord();

    if (item->flags.bits.in_inventory) {
        for (auto gref : item->general_refs)
        {
            switch (gref->getType())
            {
                case general_ref_type::CONTAINED_IN_ITEM:
                    if (auto item2 = gref->getItem())
                        return getPosition(item2);
                    break;
                case general_ref_type::UNIT_HOLDER:
                    if (auto unit = gref->getUnit())
                        return Units::getPosition(unit);
                    break;
                /*case general_ref_type::BUILDING_HOLDER: // Note: Not "in_inventory"
                    if (auto bld = ref->getBuilding())
                        return df::coord(bld->centerx, bld->centery, bld->z);
                    break;*/
                default:
                    break;
            }
        }

        if (auto sref = getSpecificRef(item, specific_ref_type::VERMIN_ESCAPED_PET))
            return sref->data.vermin->pos;
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

static void addQuality(string &tmp, int quality) {
    if (quality > 0 && quality <= 5) {
        char c = quality_table[quality];
        tmp = c + tmp + c;
    }
}

static bool title_from_content(string &result, const vector<int32_t> &vec) {
    for (auto content : vec)
    {
        auto w_contents = df::written_content::find(content);
        if (w_contents && !w_contents->title.empty()) {
            result = w_contents->title;
            return true;
        }
    }
    return false;
}

static string title_from_imps(const vector<df::itemimprovement *> &vec) {
    string result;
    for (auto imp : vec) {
        if (auto page = virtual_cast<df::itemimprovement_pagesst>(imp))
            if (title_from_content(result, page->contents))
                break;

        if (auto writing = virtual_cast<df::itemimprovement_writingst>(imp))
            if (title_from_content(result, writing->contents))
                break;
    }
    return result;
}

string Items::getBookTitle(df::item *item) {
    CHECK_NULL_POINTER(item);

    if (auto book = virtual_cast<df::item_bookst>(item))
        return book->title.empty() ? title_from_imps(book->improvements) : book->title;

    if (auto tool = virtual_cast<df::item_toolst>(item))
        if (tool->hasToolUse(tool_uses::CONTAIN_WRITING))
            return title_from_imps(tool->improvements);
    return "";
}

string Items::getDescription(df::item *item, int type, bool decorate) {
    CHECK_NULL_POINTER(item);
    string tmp;
    item->getItemDescription(&tmp, type);

    if (decorate) {
        // Special indicators get added in a specific order
        // Innermost is at the top, and outermost is at the bottom

        // First, figure out the quality levels we're going to display
        int craftquality = item->getQuality();
        int craftquality_only_imps = item->getImprovementQuality();
        bool has_displayed_item_improvements = item->isImproved();
        if (!has_displayed_item_improvements && (craftquality < craftquality_only_imps))
            craftquality = craftquality_only_imps;

        // First, actual item quality
        addQuality(tmp, craftquality);

        // Next, magic enchants
        if (item->getMagic() != NULL)
            tmp = '\x11' + tmp + '\x10'; // <| |>

        // Next, improvements
        if (has_displayed_item_improvements) {
            tmp = '\xAE' + tmp + '\xAF'; // («) + tmp + (»)
            if (df::global::d_init->display.flags.is_set(d_init_flags1::SHOW_IMP_QUALITY))
                addQuality(tmp, craftquality_only_imps);
        }

        // Dwarf mode only, forbid/foreign
        if (*df::global::gamemode == game_mode::DWARF) {
            if (item->flags.bits.forbid)
                tmp = '{' + tmp + '}';
            if (item->flags.bits.foreign)
                tmp = '(' + tmp + ')';
        }

        // Wear
        switch (item->getWear())
        {
            case 1: tmp = 'x' + tmp + 'x'; break;
            case 2: tmp = 'X' + tmp + 'X'; break;
            case 3: tmp = "XX" + tmp + "XX"; break;
        }

        // Fire
        if (item->flags.bits.on_fire)
            tmp = '\x13' + tmp + '\x13'; // !! !!

        // Finally, Adventure civzone
        if ((*df::global::gamemode == game_mode::ADVENTURE) &&
            Items::getGeneralRef(item, general_ref_type::BUILDING_CIVZONE_ASSIGNED) != NULL)
            tmp = '$' + tmp + '$';
    }
    return tmp;
}

static df::artifact_record *get_artifact(df::item *item) {
    if (!item->flags.bits.artifact)
        return NULL;
    if (auto gref = Items::getGeneralRef(item, general_ref_type::IS_ARTIFACT))
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
    if (auto name = Items::getBookTitle(item); !name.empty())
        return name;
    if (auto artifact = get_artifact(item); artifact && artifact->name.has_name)
        return Translation::translateName(&artifact->name) +
            ", " + Translation::translateName(&artifact->name, true) +
            " (" + get_item_type_str(item) + ")";
    return Items::getDescription(item, 0, true);
}

string Items::getReadableDescription(df::item *item) {
    CHECK_NULL_POINTER(item);
    auto desc = get_base_desc(item);

    if (auto gref = Items::getGeneralRef(item, general_ref_type::CONTAINS_UNIT)) {
        if (auto unit = gref->getUnit())
        {
            auto str = " [" + Units::getReadableName(unit);
            if (Units::isInvader(unit) || Units::isOpposedToLife(unit))
                str += " (hostile)";
            return desc + str + "]";
        }
    }
    return desc;
}

static bool removeItemOnGround(df::item *item)
{   // Replaces the MapCache fn
    auto block = Maps::getTileBlock(item->pos);
    if (!block)
        return false;

    erase_from_vector(block->items, item->id);

    for (auto b_item : block->items)
    {
        auto other_item = df::item::find(b_item);
        if (other_item && other_item->pos == item->pos)
            return true; // Don't touch occupancy
    }

    auto &occ = index_tile(block->occupancy, item->pos);
    occ.bits.item = false;

    if (occ.bits.building == tile_building_occ::Planned)
        if (auto bld = Buildings::findAtTile(item->pos))
        {   // TODO: Maybe recheck other tiles like the game does.
            bld->flags.bits.site_blocked = false;
        }
    return true;
}

static void resetUnitInvFlags(df::unit *unit, df::unit_inventory_item *inv_item) {
    if (inv_item->mode == df::inv_item_role_type::Worn ||
        inv_item->mode == df::inv_item_role_type::WrappedAround)
    {
        unit->flags2.bits.calculated_inventory = false;
        unit->flags2.bits.calculated_insulation = false;
    }
    else if (inv_item->mode == df::inv_item_role_type::StuckIn)
        unit->flags3.bits.stuck_weapon_computed = false;
}

static bool detachItem(df::item *item)
{   // Remove item from any inventory or map block
    if (!item->specific_refs.empty() || item->world_data_id != -1)
        return false;

    bool building_clutter = false;
    for (auto ref : item->general_refs) {
        switch (ref->getType())
        {
            case general_ref_type::BUILDING_HOLDER:
                if (item->flags.bits.in_building)
                    return false; // Construction mat
                building_clutter = true;
                break;
            case general_ref_type::BUILDING_CAGED:
            case general_ref_type::BUILDING_TRIGGER:
            case general_ref_type::BUILDING_TRIGGERTARGET:
            case general_ref_type::BUILDING_CIVZONE_ASSIGNED:
                return false;
            default:
                break;
        }
    }

    if (building_clutter) {
        if (auto building = virtual_cast<df::building_actual>(Items::getHolderBuilding(item))) {
            for (size_t i = 0; i < building->contained_items.size(); i++)
            {
                auto ci = building->contained_items[i];
                if (ci->item == item) {
                    removeRef(item->general_refs, general_ref_type::BUILDING_HOLDER, building->id);
                    vector_erase_at(building->contained_items, i);
                    delete ci;
                    return true;
                }
            }
        }
    }

    if (auto ref = virtual_cast<df::general_ref_projectile>(Items::getGeneralRef(item, general_ref_type::PROJECTILE)))
        return linked_list_remove(&world->projectiles.all, ref->projectile_id)
            && removeRef(item->general_refs, general_ref_type::PROJECTILE, ref->getID());

    if (item->flags.bits.on_ground) {
        if (!removeItemOnGround(item))
            Core::printerr("Item was marked on_ground, but not in block: %d (%d,%d,%d)\n",
                item->id, item->pos.x, item->pos.y, item->pos.z);
        item->flags.bits.on_ground = false;
        return true;
    }

    if (item->flags.bits.in_inventory) {
        bool found = false;
        for (int i = item->general_refs.size()-1; i >= 0; i--) {
            auto ref = item->general_refs[i];

            switch (ref->getType())
            {
            case general_ref_type::CONTAINED_IN_ITEM:
                if (auto item2 = ref->getItem())
                {
                    item2->flags.bits.weight_computed = false;
                    removeRef(item2->general_refs, general_ref_type::CONTAINS_ITEM, item->id);
                }
                break;
            case general_ref_type::UNIT_HOLDER:
                if (auto unit = ref->getUnit())
                    for (int i = unit->inventory.size()-1; i >= 0; i--) {
                        auto inv_item = unit->inventory[i];
                        if (inv_item->item != item)
                            continue; // Next inventory item
                        resetUnitInvFlags(unit, inv_item);

                        vector_erase_at(unit->inventory, i);
                        delete inv_item;
                    }
                break;
            default:
                continue; // Next ref
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

        if (item->flags.bits.garbage_collect) {
            item->flags.bits.garbage_collect = false;
            item->categorize(true);
        }
        return true;
    }
    return false;
}

bool DFHack::Items::moveToGround(df::item *item, df::coord pos) {
    CHECK_NULL_POINTER(item);
    if (!detachItem(item))
        return false;

    item->pos = pos;
    item->flags.bits.on_ground = true;

    // The moveToGround function can return false even when it succeeds,
    // so we don't check the return value.
    item->moveToGround(pos.x, pos.y, pos.z);
    return true;
}

bool DFHack::Items::moveToContainer(df::item *item, df::item *container) {
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(container);

    auto cpos = getPosition(container);
    if (!cpos.isValid())
        return false;

    auto ref1 = df::allocate<df::general_ref_contains_itemst>();
    auto ref2 = df::allocate<df::general_ref_contained_in_itemst>();

    if (!ref1 || !ref2) {
        delete ref1;
        delete ref2;
        Core::printerr("Could not allocate container refs.\n");
        return false;
    }
    else if (!detachItem(item)) {
        delete ref1;
        delete ref2;
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

bool DFHack::Items::moveToBuilding(df::item *item, df::building_actual *building,
    df::building_item_role_type use_mode, bool force_in_building)
{
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(building);
    CHECK_INVALID_ARGUMENT(use_mode == building_item_role_type::TEMP ||
        use_mode == building_item_role_type::PERM);

    auto ref = df::allocate<df::general_ref_building_holderst>();
    if(!ref) {
        Core::printerr("Could not allocate building holder refs.\n");
        return false;
    }
    else if (!detachItem(item)) {
        delete ref;
        return false;
    }

    item->pos.x = building->centerx;
    item->pos.y = building->centery;
    item->pos.z = building->z;
    if (use_mode == building_item_role_type::PERM || force_in_building)
        item->flags.bits.in_building = true;

    ref->building_id = building->id;
    item->general_refs.push_back(ref);

    auto con = new df::buildingitemst;
    con->item = item;
    con->use_mode = use_mode;
    building->contained_items.push_back(con);

    return true;
}

bool DFHack::Items::moveToInventory(df::item *item, df::unit *unit,
    df::inv_item_role_type mode, int body_part)
{
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(unit);
    CHECK_NULL_POINTER(unit->body.body_plan);
    CHECK_INVALID_ARGUMENT(is_valid_enum_item(mode));
    int body_plan_size = unit->body.body_plan->body_parts.size();
    CHECK_INVALID_ARGUMENT(body_part < 0 || body_part <= body_plan_size);

    auto holderReference = df::allocate<df::general_ref_unit_holderst>();
    if (!holderReference) {
        Core::printerr("Could not allocate UNIT_HOLDER reference.\n");
        return false;
    }
    else if (!detachItem(item)) {
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

bool Items::remove(df::item *item, bool no_uncat) {
    CHECK_NULL_POINTER(item);

    if (auto spec_ref = getSpecificRef(item, specific_ref_type::JOB))
        Job::removeJob(spec_ref->data.job);
    if (!detachItem(item))
        return false;
    if (auto pos = getPosition(item); pos.isValid())
        item->pos = pos;
    if (!no_uncat)
        item->uncategorize();

    // Hide them from jobs and the UI until the item can be garbage collected
    item->flags.bits.forbid = true;
    item->flags.bits.hidden = true;

    item->flags.bits.removed = true;
    item->flags.bits.garbage_collect = !no_uncat;
    return true;
}

df::proj_itemst *Items::makeProjectile(df::item *item)
{   using df::global::proj_next_id;
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
    else if (!detachItem(item)) {
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

    linked_list_append(&world->projectiles.all, proj->link);
    return proj;
}

int Items::getItemBaseValue(int16_t item_type, int16_t item_subtype,
    int16_t mat_type, int32_t mat_subtype)
{
    int value = 0;
    switch (item_type)
    {   using namespace df::enums::item_type;
        case SEEDS:
        case DRINK:
        case POWDER_MISC:
        case LIQUID_MISC:
        case COIN:
        case GLOB:
        case ORTHOPEDIC_CAST:
        case BRANCH:
            value = 1;
            break;
        case MEAT:
        case PLANT:
        case PLANT_GROWTH:
            value = 2;
            break;
        case BOULDER:
        case WOOD:
            value = 3;
            break;
        case BAR:
        case BLOCKS:
        case SKIN_TANNED:
        case SHEET:
            value = 5;
            break;
        case ROUGH:
        case THREAD:
            value = 6;
            break;
        case CLOTH:
            value = 7;
            break;
        case DOOR:
        case FLOODGATE:
        case BED:
        case CHAIR:
        case CHAIN:
        case FLASK:
        case GOBLET:
        case TOY:
        case CAGE:
        case BARREL:
        case BUCKET:
        case ANIMALTRAP:
        case TABLE:
        case COFFIN:
        case BOX:
        case BAG:
        case BIN:
        case ARMORSTAND:
        case WEAPONRACK:
        case CABINET:
        case FIGURINE:
        case AMULET:
        case SCEPTER:
        case CROWN:
        case RING:
        case EARRING:
        case BRACELET:
        case GEM:
        case ANVIL:
        case TOTEM:
        case BACKPACK:
        case QUIVER:
        case BALLISTAARROWHEAD:
        case CHEESE:
        case PIPE_SECTION:
        case HATCH_COVER:
        case GRATE:
        case QUERN:
        case MILLSTONE:
        case SPLINT:
        case CRUTCH:
        case SLAB:
        case BOOK:
            value = 10;
            break;
        case SMALLGEM:
        case SIEGEAMMO:
        case TRACTION_BENCH:
            value = 20;
            break;
        case df::enums::item_type::WINDOW:
        case STATUE:
            value = 25;
            break;
        case CATAPULTPARTS:
        case BALLISTAPARTS:
        case TRAPPARTS:
            value = 30;
            break;
        case INSTRUMENT:
            value = 10;
            if (auto def = df::itemdef_instrumentst::find(item_subtype))
                value = def->value;
            break;
        case WEAPON:
            value = 10;
            if (auto def = df::itemdef_weaponst::find(item_subtype))
                value = def->value;
            break;
        case ARMOR:
            value = 10;
            if (auto def = df::itemdef_armorst::find(item_subtype))
                value = def->value;
            break;
        case SHOES:
            value = 5;
            if (auto def = df::itemdef_shoesst::find(item_subtype))
                value = def->value;
            break;
        case SHIELD:
            value = 10;
            if (auto def = df::itemdef_shieldst::find(item_subtype))
                value = def->value;
            break;
        case HELM:
            value = 10;
            if (auto def = df::itemdef_helmst::find(item_subtype))
                value = def->value;
            break;
        case GLOVES:
            value = 5;
            if (auto def = df::itemdef_glovesst::find(item_subtype))
                value = def->value;
            break;
        case AMMO:
            value = 1;
            if (auto def = df::itemdef_ammost::find(item_subtype))
                value = def->value;
            break;
        case PANTS:
            value = 10;
            if (auto def = df::itemdef_pantsst::find(item_subtype))
                value = def->value;
            break;
        case TRAPCOMP:
            value = 10;
            if (auto def = df::itemdef_trapcompst::find(item_subtype))
                value = def->value;
            break;
        case TOOL:
            value = 10;
            if (auto def = df::itemdef_toolst::find(item_subtype))
                value = def->value;
            break;
        case FISH:
        case FISH_RAW:
        case EGG:
            value = 2;
            if (auto craw = Units::getCasteRaw(mat_type, mat_subtype)) {
                mat_type = craw->misc.bone_mat;
                mat_subtype = craw->misc.bone_matidx;
            }
            break;
        case VERMIN:
            if (auto craw = Units::getCasteRaw(mat_type, mat_subtype))
                value = craw->misc.petvalue / 2;
            return value ? value : 1;
        case PET:
            if (auto craw = Units::getCasteRaw(mat_type, mat_subtype))
                value = craw->misc.petvalue;
            return value;
        case FOOD:
            return 1;
        case CORPSE:
        case CORPSEPIECE:
        case REMAINS:
        case ROCK:
            return 0;
    }

    MaterialInfo mat;
    if (mat.decode(mat_type, mat_subtype))
        value *= mat.material->material_value;
    return value;
}

static int32_t get_war_multiplier(df::item *item, df::caravan_state *caravan) {
    CHECK_NULL_POINTER(item);
    static const int32_t DEFAULT_WAR_MULTIPLIER = 256;
    if (!caravan)
        return DEFAULT_WAR_MULTIPLIER;

    auto caravan_he = df::historical_entity::find(caravan->entity);
    if (!caravan_he)
        return DEFAULT_WAR_MULTIPLIER;

    int32_t war_alignment = caravan_he->entity_raw->sphere_alignment[sphere_type::WAR];
    if (war_alignment == DEFAULT_WAR_MULTIPLIER)
        return DEFAULT_WAR_MULTIPLIER;

    switch (item->getType())
    {   using namespace df::enums::item_type;
    case WEAPON:
    {
        auto weap_def = df::itemdef_weaponst::find(item->getSubtype());
        auto caravan_cre_raw = df::creature_raw::find(caravan_he->race);
        if (!weap_def || !caravan_cre_raw || caravan_cre_raw->adultsize < weap_def->minimum_size)
            return DEFAULT_WAR_MULTIPLIER;
        break;
    }
    case ARMOR:
    case SHOES:
    case HELM:
    case GLOVES:
    case PANTS:
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
    case SHIELD:
    case AMMO:
    case BACKPACK:
    case QUIVER:
        break;
    default:
        return DEFAULT_WAR_MULTIPLIER;
    }
    return war_alignment;
}

static const int32_t DEFAULT_AGREEMENT_MULTIPLIER = 128;

static int32_t get_buy_request_multiplier(df::item *item, const df::entity_buy_prices *buy_prices) {
    CHECK_NULL_POINTER(item);
    if (!buy_prices)
        return DEFAULT_AGREEMENT_MULTIPLIER;

    int16_t item_type = item->getType();
    int16_t item_subtype = item->getSubtype();
    int16_t mat_type = item->getMaterial();
    int32_t mat_subtype = item->getMaterialIndex();

    for (size_t idx = 0; idx < buy_prices->price.size(); ++idx) {
        if ((buy_prices->items->item_type[idx] == item_type) &&
            (buy_prices->items->item_subtype[idx] == -1 || buy_prices->items->item_subtype[idx] == item_subtype) &&
            (buy_prices->items->mat_types[idx] == -1 || buy_prices->items->mat_types[idx] == mat_type) &&
            (buy_prices->items->mat_indices[idx] == -1 || buy_prices->items->mat_indices[idx] == mat_subtype)
        )
            return buy_prices->price[idx];
    }
    return DEFAULT_AGREEMENT_MULTIPLIER;
}

template<typename T>
static int get_price(const vector<T> &res, int32_t val, const vector<int32_t> &pri) {
    for (size_t idx = 0; idx < res.size(); ++idx)
        if (res[idx] == val)
            return vector_get(pri, idx, -1);
    return -1;
}

template<typename T1, typename T2>
static int get_price(const vector<T1> &mat_res, int32_t mat, const vector<T2> &gloss_res,
    int32_t gloss, const vector<int32_t> &pri)
{
    for (size_t idx = 0; idx < mat_res.size(); ++idx)
        if (mat_res[idx] == mat && (gloss_res[idx] == -1 || gloss_res[idx] == gloss))
            return vector_get(pri, idx, -1);
    return -1;
}

static int32_t get_sell_request_multiplier(df::item *item,
    const df::historical_entity::T_resources &resources, const vector<int32_t> *prices)
{
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(prices);
    static const df::dfhack_material_category silk_cat(df::dfhack_material_category::mask_silk);
    static const df::dfhack_material_category yarn_cat(df::dfhack_material_category::mask_yarn);
    static const df::dfhack_material_category leather_cat(df::dfhack_material_category::mask_leather);

    int16_t item_type = item->getType();
    int16_t item_subtype = item->getSubtype();
    int16_t mat_type = item->getMaterial();
    int32_t mat_subtype = item->getMaterialIndex();

    MaterialInfo mi(mat_type, mat_subtype);

#define TRY_RETURN_PRICE(res, cat) \
    if (int32_t price = get_price(resources.res, mat_subtype, \
        prices[entity_sell_category::cat]); price != -1) return price
#define TRY_RETURN_PRICE_2(mat_res, gloss_res, cat) \
    if (int32_t price = get_price(resources.mat_res, mat_type, \
        resources.gloss_res, mat_subtype, \
        prices[entity_sell_category::cat]); price != -1) return price

    switch (item_type)
    {   using namespace df::enums::item_type;
    case BAR:
        if (mi.mode == MaterialInfo::Inorganic)
            TRY_RETURN_PRICE(metals, MetalBars);
        break;
    case SMALLGEM:
        if (mi.mode == MaterialInfo::Inorganic)
            TRY_RETURN_PRICE(gems, SmallCutGems);
        break;
    case BLOCKS:
        if (mi.mode == MaterialInfo::Inorganic)
            TRY_RETURN_PRICE(stones, StoneBlocks);
        break;
    case ROUGH:
        TRY_RETURN_PRICE_2(misc_mat.glass.mat_type, misc_mat.glass.mat_index, Glass);
        break;
    case BOULDER:
        TRY_RETURN_PRICE(stones, Stone);
        TRY_RETURN_PRICE_2(misc_mat.clay.mat_type, misc_mat.clay.mat_index, Clay);
        break;
    case WOOD:
        TRY_RETURN_PRICE_2(organic.wood.mat_type, organic.wood.mat_index, Wood);
        break;
    case CHAIN:
        if (mi.mode == MaterialInfo::Plant)
            TRY_RETURN_PRICE_2(organic.fiber.mat_type, organic.fiber.mat_index, RopesPlant);
        if (mi.matches(silk_cat))
            TRY_RETURN_PRICE_2(organic.silk.mat_type, organic.silk.mat_index, RopesSilk);
        if (mi.matches(yarn_cat))
            TRY_RETURN_PRICE_2(organic.wool.mat_type, organic.wool.mat_index, RopesYarn);
        break;
    case FLASK:
        TRY_RETURN_PRICE_2(misc_mat.flasks.mat_type, misc_mat.flasks.mat_index, FlasksWaterskins);
        break;
    case GOBLET:
        TRY_RETURN_PRICE_2(misc_mat.crafts.mat_type, misc_mat.crafts.mat_index, CupsMugsGoblets);
        break;
    case INSTRUMENT:
        TRY_RETURN_PRICE(instrument_type, Instruments);
        break;
    case TOY:
        TRY_RETURN_PRICE(toy_type, Toys);
        break;
    case CAGE:
        TRY_RETURN_PRICE_2(misc_mat.cages.mat_type, misc_mat.cages.mat_index, Cages);
        break;
    case BARREL:
        TRY_RETURN_PRICE_2(misc_mat.barrels.mat_type, misc_mat.barrels.mat_index, Barrels);
        break;
    case BUCKET:
        TRY_RETURN_PRICE_2(misc_mat.barrels.mat_type, misc_mat.barrels.mat_index, Buckets);
        break;
    case WEAPON:
        TRY_RETURN_PRICE(weapon_type, Weapons);
        TRY_RETURN_PRICE(digger_type, DiggingImplements);
        TRY_RETURN_PRICE(training_weapon_type, TrainingWeapons);
        break;
    case ARMOR:
        TRY_RETURN_PRICE(armor_type, Bodywear);
        break;
    case SHOES:
        TRY_RETURN_PRICE(shoes_type, Footwear);
        break;
    case SHIELD:
        TRY_RETURN_PRICE(shield_type, Shields);
        break;
    case HELM:
        TRY_RETURN_PRICE(helm_type, Headwear);
        break;
    case GLOVES:
        TRY_RETURN_PRICE(gloves_type, Handwear);
        break;
    case BAG:
        if (mi.matches(leather_cat))
            TRY_RETURN_PRICE_2(organic.leather.mat_type, organic.leather.mat_index, BagsLeather);
        if (mi.mode == MaterialInfo::Plant)
            TRY_RETURN_PRICE_2(organic.fiber.mat_type, organic.fiber.mat_index, BagsPlant);
        if (mi.matches(silk_cat))
            TRY_RETURN_PRICE_2(organic.silk.mat_type, organic.silk.mat_index, BagsSilk);
        if (mi.matches(yarn_cat))
            TRY_RETURN_PRICE_2(organic.wool.mat_type, organic.wool.mat_index, BagsYarn);
        break;
    case FIGURINE:
    case AMULET:
    case SCEPTER:
    case CROWN:
    case RING:
    case EARRING:
    case BRACELET:
    case TOTEM:
    case BOOK:
        TRY_RETURN_PRICE_2(misc_mat.crafts.mat_type, misc_mat.crafts.mat_index, Crafts);
        break;
    case AMMO:
        TRY_RETURN_PRICE(ammo_type, Ammo);
        break;
    case GEM:
        if (mi.mode == MaterialInfo::Inorganic)
            TRY_RETURN_PRICE(gems, LargeCutGems);
        break;
    case ANVIL:
        TRY_RETURN_PRICE_2(metal.anvil.mat_type, metal.anvil.mat_index, Anvils);
        break;
    case MEAT:
        TRY_RETURN_PRICE_2(misc_mat.meat.mat_type, misc_mat.meat.mat_index, Meat);
        break;
    case FISH:
    case FISH_RAW:
        TRY_RETURN_PRICE_2(fish_races, fish_castes, Fish);
        break;
    case VERMIN:
    case PET:
        TRY_RETURN_PRICE_2(animals.pet_races, animals.pet_castes, Pets);
        break;
    case SEEDS:
        TRY_RETURN_PRICE_2(seeds.mat_type, seeds.mat_index, Seeds);
        break;
    case PLANT:
        TRY_RETURN_PRICE_2(plants.mat_type, plants.mat_index, Plants);
        break;
    case SKIN_TANNED:
        TRY_RETURN_PRICE_2(organic.leather.mat_type, organic.leather.mat_index, Leather);
        break;
    case PLANT_GROWTH:
        if (mi.mode == MaterialInfo::Plant) {
            TRY_RETURN_PRICE_2(tree_fruit_plants, tree_fruit_growths, FruitsNuts);
            TRY_RETURN_PRICE_2(shrub_fruit_plants, shrub_fruit_growths, GardenVegetables);
        }
        break;
    case THREAD:
        if (mi.mode == MaterialInfo::Plant)
            TRY_RETURN_PRICE_2(organic.fiber.mat_type, organic.fiber.mat_index, ThreadPlant);
        if (mi.matches(silk_cat))
            TRY_RETURN_PRICE_2(organic.silk.mat_type, organic.silk.mat_index, ThreadSilk);
        if (mi.matches(yarn_cat))
            TRY_RETURN_PRICE_2(organic.wool.mat_type, organic.wool.mat_index, ThreadYarn);
        break;
    case CLOTH:
        if (mi.mode == MaterialInfo::Plant)
            TRY_RETURN_PRICE_2(organic.fiber.mat_type, organic.fiber.mat_index, ClothPlant);
        if (mi.matches(silk_cat))
            TRY_RETURN_PRICE_2(organic.silk.mat_type, organic.silk.mat_index, ClothSilk);
        if (mi.matches(yarn_cat))
            TRY_RETURN_PRICE_2(organic.wool.mat_type, organic.wool.mat_index, ClothYarn);
        break;
    case PANTS:
        TRY_RETURN_PRICE(pants_type, Legwear);
        break;
    case BACKPACK:
        TRY_RETURN_PRICE_2(misc_mat.backpacks.mat_type, misc_mat.backpacks.mat_index, Backpacks);
        break;
    case QUIVER:
        TRY_RETURN_PRICE_2(misc_mat.quivers.mat_type, misc_mat.quivers.mat_index, Quivers);
        break;
    case TRAPCOMP:
        TRY_RETURN_PRICE(trapcomp_type, TrapComponents);
        break;
    case DRINK:
        TRY_RETURN_PRICE_2(misc_mat.booze.mat_type, misc_mat.booze.mat_index, Drinks);
        break;
    case POWDER_MISC:
        TRY_RETURN_PRICE_2(misc_mat.powders.mat_type, misc_mat.powders.mat_index, Powders);
        TRY_RETURN_PRICE_2(misc_mat.sand.mat_type, misc_mat.sand.mat_index, Sand);
        break;
    case CHEESE:
        TRY_RETURN_PRICE_2(misc_mat.cheese.mat_type, misc_mat.cheese.mat_index, Cheese);
        break;
    case LIQUID_MISC:
        TRY_RETURN_PRICE_2(misc_mat.extracts.mat_type, misc_mat.extracts.mat_index, Extracts);
        break;
    case SPLINT:
        TRY_RETURN_PRICE_2(misc_mat.barrels.mat_type, misc_mat.barrels.mat_index, Splints);
        break;
    case CRUTCH:
        TRY_RETURN_PRICE_2(misc_mat.barrels.mat_type, misc_mat.barrels.mat_index, Crutches);
        break;
    case TOOL:
        TRY_RETURN_PRICE(tool_type, Tools);
        break;
    case EGG:
        TRY_RETURN_PRICE_2(egg_races, egg_castes, Eggs);
        break;
    case SHEET:
        TRY_RETURN_PRICE_2(organic.parchment.mat_type, organic.parchment.mat_index, Parchment);
        break;
    /// DOOR, FLOODGATE, BED, CHAIR, WINDOW, ANIMALTRAP, TABLE, COFFIN, STATUE, CORPSE, BOX, BIN,
    /// ARMORSTAND, WEAPONRACK, CABINET, CORPSEPIECE, REMAINS, CATAPULTPARTS, BALLISTAPARTS,
    /// SIEGEAMMO, BALLISTAARROWHEAD, TRAPPARTS, FOOD, COIN, GLOB, ROCK, PIPE_SECTION,
    /// HATCH_COVER, GRATE, QUERN, MILLSTONE, TRACTION_BENCH, ORTHOPEDIC_CAST, SLAB, BRANCH
    default:
        break;
    }

#undef TRY_RETURN_PRICE
#undef TRY_RETURN_PRICE_2

    for (size_t idx = 0; idx < resources.wood_products.item_type.size(); ++idx)
        if ((resources.wood_products.item_type[idx] == item_type) &&
            (resources.wood_products.item_subtype[idx] == -1 || resources.wood_products.item_subtype[idx] == item_subtype) &&
            (resources.wood_products.material.mat_type[idx] == mat_type) &&
            (resources.wood_products.material.mat_index[idx] == -1 || resources.wood_products.material.mat_index[idx] == mat_subtype)
        )
            return vector_get(prices[entity_sell_category::Miscellaneous], idx, DEFAULT_AGREEMENT_MULTIPLIER);
    return DEFAULT_AGREEMENT_MULTIPLIER;
}

static int32_t get_sell_request_multiplier(df::item *item, const df::caravan_state *caravan) {
    CHECK_NULL_POINTER(caravan);
    auto sell_prices = caravan->sell_prices;
    if (!sell_prices)
        return DEFAULT_AGREEMENT_MULTIPLIER;

    auto caravan_he = df::historical_entity::find(caravan->entity);
    if (!caravan_he)
        return DEFAULT_AGREEMENT_MULTIPLIER;
    return get_sell_request_multiplier(item, caravan_he->resources, &sell_prices->price[0]);
}

static int32_t get_sell_request_multiplier(df::unit *unit, const df::caravan_state *caravan) {
    CHECK_NULL_POINTER(unit);
    CHECK_NULL_POINTER(caravan);
    auto sell_prices = caravan->sell_prices;
    if (!sell_prices)
        return DEFAULT_AGREEMENT_MULTIPLIER;

    auto caravan_he = df::historical_entity::find(caravan->entity);
    if (!caravan_he)
        return DEFAULT_AGREEMENT_MULTIPLIER;

    auto &resources = caravan_he->resources;
    int32_t price = get_price(resources.animals.pet_races, unit->race, resources.animals.pet_castes, unit->caste,
                              sell_prices->price[entity_sell_category::Pets]);
    return (price != -1) ? price : DEFAULT_AGREEMENT_MULTIPLIER;
}

int Items::getValue(df::item *item, df::caravan_state *caravan) {
    CHECK_NULL_POINTER(item);
    int16_t item_type = item->getType();
    int16_t item_subtype = item->getSubtype();
    int16_t mat_type = item->getMaterial();
    int32_t mat_subtype = item->getMaterialIndex();

    // Get base value for item type, subtype, and material
    int value = getItemBaseValue(item_type, item_subtype, mat_type, mat_subtype);
    // Entity value modifications
    value *= get_war_multiplier(item, caravan);
    value >>= 8;

    // Improve value based on quality
    switch (item->getQuality())
    {
        case 1: value *= 1.1; value += 3; break; // (-) well-crafted
        case 2: value *= 1.2; value += 6; break; // (+) Finely-crafted
        case 3: value *= 1.333; value += 10; break; // (*) superior quality
        case 4: value *= 1.5; value += 15; break; // (≡) exceptional
        case 5: value *= 2; value += 30; break; // (☼) masterful, artifact
    }

    // Add improvement values
    int impValue = item->getThreadDyeValue(caravan) + item->getImprovementsValue(caravan);
    if (item_type == item_type::AMMO) // Ammo improvements are worth less
        impValue /= 30;
    value += impValue;

    // Degrade value due to wear
    switch (item->getWear())
    {
        case 1: value = value * 3 / 4; break; // (x) worn
        case 2: value = value / 2; break; // (X) threadbare
        case 3: value = value / 4; break; // (XX) tattered
    }

    // Magical powers have 500 value each
    auto magic = item->getMagic();
    if (magic != NULL)
        value += magic->power.size() * 500;

    // Artifacts have 10x value
    if (item->flags.bits.artifact_mood)
        value *= 10;

    // Modify buy/sell prices
    if (caravan) {
        int32_t buy_multiplier = get_buy_request_multiplier(item, caravan->buy_prices);
        if (buy_multiplier == DEFAULT_AGREEMENT_MULTIPLIER)
            value *= get_sell_request_multiplier(item, caravan);
        else
            value *= buy_multiplier;
        value >>= 7;
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
        auto creature = df::creature_raw::find(mat_type);
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
            if (gref->getType() != general_ref_type::CONTAINS_UNIT)
                continue;
            auto unit = gref->getUnit();
            if (!unit)
                continue;
            auto caste = Units::getCasteRaw(unit);
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

bool Items::createItem(vector<df::item *> &out_items, df::unit *unit, df::item_type item_type,
    int16_t item_subtype, int16_t mat_type, int32_t mat_index, bool no_floor)
{   // Based on Quietust's plugins/createitem.cpp
    CHECK_NULL_POINTER(unit);
    auto pos = Units::getPosition(unit);
    auto block = Maps::getTileBlock(pos);
    CHECK_NULL_POINTER(block);

    auto prod = df::allocate<df::reaction_product_itemst>();
    prod->item_type = item_type;
    prod->item_subtype = item_subtype;
    prod->mat_type = mat_type;
    prod->mat_index = mat_index;
    prod->probability = 100;
    prod->count = 1;

    switch(item_type)
    {   using namespace df::enums::item_type;
        case BAR:
        case POWDER_MISC:
        case LIQUID_MISC:
        case DRINK:
            prod->product_dimension = 150;
            break;
        case THREAD:
            prod->product_dimension = 15000;
            break;
        case CLOTH:
            prod->product_dimension = 10000;
            break;
        default:
            prod->product_dimension = 1;
    }

    // makeItem
    vector<df::reaction_product *> out_products;
    vector<df::reaction_reagent *> in_reag;
    vector<df::item *> in_items;

    out_items.clear();
    prod->produce(unit, &out_products, &out_items, &in_reag, &in_items, 1, job_skill::NONE,
        0, df::historical_entity::find(unit->civ_id),
        World::isFortressMode() ? df::world_site::find(World::GetCurrentSiteId()) : NULL, NULL);
    delete prod;

    DEBUG(items).print("produced %zd items\n", out_items.size());

    for (auto out_item : out_items)
    {   // Plant growths need a valid "growth print", otherwise they behave oddly
        if (auto growth = virtual_cast<df::item_plant_growthst>(out_item))
        {
            int growth_print = -1;
            // Make sure it's made of a valid plant material, then grab its definition
            if (growth->mat_type >= 419 && growth->mat_type <= 618 && growth->mat_index >= 0 && (unsigned)growth->mat_index < world->raws.plants.all.size())
            {
                auto plant_def = world->raws.plants.all[growth->mat_index];
                // Make sure it subtype is also valid
                if (growth->subtype >= 0 && (unsigned)growth->subtype < plant_def->growths.size())
                {
                    auto growth_def = plant_def->growths[growth->subtype];
                    // Try and find a growth print matching the current time
                    // (in practice, only tree leaves use this for autumn color changes)
                    for (size_t i = 0; i < growth_def->prints.size(); i++)
                    {
                        auto print_def = growth_def->prints[i];
                        if (print_def->timing_start <= *df::global::cur_year_tick && *df::global::cur_year_tick <= print_def->timing_end)
                        {
                            growth_print = i;
                            break;
                        }
                    }
                    // If we didn't find one, then pick the first one (if it exists)
                    if (growth_print == -1 && !growth_def->prints.empty())
                        growth_print = 0;
                }
            }
            growth->growth_print = growth_print;
        }
        if (!no_floor)
            out_item->moveToGround(pos.x, pos.y, pos.z);
    }
    return !out_items.empty();
}

/*
 * Trade Info
 */

bool Items::checkMandates(df::item *item) {
    CHECK_NULL_POINTER(item);
    for (auto mandate : world->mandates.all) {
        if ((mandate->mode == df::mandate_type::Export) &&
            (item->getType() == mandate->item_type) &&
            (mandate->item_subtype == -1 || item->getSubtype() == mandate->item_subtype) &&
            (mandate->mat_type == -1 || item->getMaterial() == mandate->mat_type) &&
            (mandate->mat_index == -1 || item->getMaterialIndex() == mandate->mat_index)
        )
            return false;
    }
    return true;
}

bool Items::canTrade(df::item *item) {
    CHECK_NULL_POINTER(item);
    if (item->flags.bits.owned || item->flags.bits.artifact ||
        item->flags.bits.spider_web || item->flags.bits.in_job
    )
        return false;

    for (auto gref : item->general_refs) {
        switch (gref->getType())
        {
            case general_ref_type::UNIT_HOLDER:
            case general_ref_type::BUILDING_HOLDER:
                return false;
            default:
                break;
        }
    }

    if (getSpecificRef(item, specific_ref_type::JOB))
        return false; // Ignore any items assigned to a job
    return checkMandates(item);
}

bool Items::canTradeWithContents(df::item *item) {
    CHECK_NULL_POINTER(item);
    if (item->flags.bits.in_inventory || !canTrade(item))
        return false;

    vector<df::item *> contained_items;
    getContainedItems(item, &contained_items);
    for (auto cit : contained_items)
        if (!canTrade(cit))
            return false;
    return true;
}

bool Items::canTradeAnyWithContents(df::item *item) {
    CHECK_NULL_POINTER(item);
    if (item->flags.bits.in_inventory)
        return false;

    vector<df::item *> contained_items;
    getContainedItems(item, &contained_items);
    if (contained_items.empty())
        return canTrade(item);

    for (auto cit : contained_items)
        if (canTrade(cit))
            return true;
    return false;
}

bool Items::markForTrade(df::item *item, df::building_tradedepotst *depot) {
    CHECK_NULL_POINTER(item);
    CHECK_NULL_POINTER(depot);
    // Validate that the depot is in a good state
    if ((depot->getBuildStage() < depot->getMaxBuildStage()) ||
        (!depot->jobs.empty() && depot->jobs[0]->job_type == job_type::DestroyBuilding)
    )
        return false;

    auto href = df::allocate<df::general_ref_building_holderst>();
    if (!href)
        return false;

    auto job = new df::job();
    job->job_type = job_type::BringItemToDepot;
    job->pos = df::coord(depot->centerx, depot->centery, depot->z);

    // job <-> item link
    if (!Job::attachJobItem(job, item, df::job_role_type::Hauled)) {
        delete job;
        delete href;
        return false;
    }

    // job <-> building link
    href->building_id = depot->id;
    depot->jobs.push_back(job);
    job->general_refs.push_back(href);

    // Add to job list
    Job::linkIntoWorld(job);
    return true;
}

static bool is_requested_trade_good(df::item *item, df::caravan_state *caravan) {
    CHECK_NULL_POINTER(caravan);
    if (caravan->time_remaining <= 0)
        return false;

    auto &trade_state = caravan->trade_state;
    if (trade_state == df::caravan_state::Approaching || trade_state == df::caravan_state::AtDepot)
        return get_buy_request_multiplier(item, caravan->buy_prices) > DEFAULT_AGREEMENT_MULTIPLIER;
    return false;
}

bool Items::isRequestedTradeGood(df::item *item, df::caravan_state *caravan) {
    CHECK_NULL_POINTER(item);
    if (caravan)
        return is_requested_trade_good(item, caravan);

    for (auto caravan : df::global::plotinfo->caravans)
        if (is_requested_trade_good(item, caravan))
            return true;
    return false;
}

/// When called with game_ui = true, this is equivalent to Bay12's itemst::meltable()
/// (i.e., returning true if and only if the item has a "designate for melting" button in game)
bool Items::canMelt(df::item *item, bool game_ui) {
    CHECK_NULL_POINTER(item);
    MaterialInfo mat(item);
    if (mat.getCraftClass() != craft_material_class::Metal)
        return false;

    switch(item->getType())
    {   using namespace df::enums::item_type;
        // These are not meltable, even if made from metal
        case CORPSE:
        case CORPSEPIECE:
        case REMAINS:
        case FISH:
        case FISH_RAW:
        case VERMIN:
        case PET:
        case FOOD:
        case EGG:
            return false;
        default:
            break;
    }

    if (item->flags.bits.artifact)
        return false;
    // Ignore checks below when asked to behave like itemst::meltable()
    if (game_ui)
        return true;

    if (item->getType() == item_type::BAR)
        return false;

    // Do not melt nonempty containers and items in unit inventories
    for (auto gref : item->general_refs) {
        switch (gref->getType())
        {   using namespace df::enums::general_ref_type;
            case CONTAINS_ITEM:
            case UNIT_HOLDER:
            case CONTAINS_UNIT:
                return false;
            case CONTAINED_IN_ITEM:
                if (auto cont = gref->getItem(); getGeneralRef(cont, UNIT_HOLDER))
                    return false;
                break;
            default:
                break;
        }
    }
    return true;
}

bool Items::markForMelting(df::item *item) {
    CHECK_NULL_POINTER(item);
    if (item->flags.bits.melt || !canMelt(item, true))
        return false;
    insert_into_vector(world->items.other.ANY_MELT_DESIGNATED, &df::item::id, item);
    item->flags.bits.melt = true;
    return true;
}

bool Items::cancelMelting(df::item *item) {
    CHECK_NULL_POINTER(item);
    if (!item->flags.bits.melt)
        return false;
    erase_from_vector(world->items.other.ANY_MELT_DESIGNATED, &df::item::id, item->id);
    item->flags.bits.melt = false;
    return true;
}

bool Items::isRouteVehicle(df::item *item) {
    CHECK_NULL_POINTER(item);
    auto vehicle = df::vehicle::find(item->getVehicleID());
    return vehicle && vehicle->route_id >= 0;
}

bool Items::isSquadEquipment(df::item *item)
{   using df::global::plotinfo;
    CHECK_NULL_POINTER(item);
    if (!plotinfo)
        return false;
    auto &vec = plotinfo->equipment.items_assigned[item->getType()];
    return vector_contains(vec, item->id);
}

/// Reverse engineered, code reference: 0x140953150 in 50.11-win64-steam
/// Our name for this function: itemst::getCapacity
/// Bay12 name for this function: not known
int32_t Items::getCapacity(df::item *item) {
    CHECK_NULL_POINTER(item);
    switch (item->getType())
    {   using namespace df::enums::item_type;
        case FLASK:
        case GOBLET:
            return 180;
        case CAGE:
        case BARREL:
        case COFFIN:
        case BOX:
        case BAG:
        case BIN:
        case ARMORSTAND:
        case WEAPONRACK:
        case CABINET:
            return 6000;
        case BUCKET:
            return 600;
        case ANIMALTRAP:
        case BACKPACK:
            return 3000;
        case QUIVER:
            return 1200;
        case TOOL:
            if (auto tool = virtual_cast<df::item_toolst>(item))
                return tool->subtype->container_capacity;
            break;
        default:
            break;
    }
    return 0;
}
