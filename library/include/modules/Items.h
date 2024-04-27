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

#pragma once
/*
* Items!
*/
#include "Export.h"
#include "Module.h"
#include "Types.h"
#include "MemAccess.h"
#include "DataDefs.h"

#include "modules/Materials.h"

#include "df/building_item_role_type.h"
#include "df/item_type.h"
#include "df/job_item_vector_id.h"
#include "df/specific_ref.h"
#include "df/unit_inventory_item.h"

namespace df {
    struct body_part_raw;
    struct building_actual;
    struct building_tradedepotst;
    struct caravan_state;
    struct item;
    struct itemdef;
    struct general_ref;
    struct proj_itemst;

    union item_flags;
}

namespace MapExtras {
    class MapCache;
}

/**
 * \defgroup grp_items Items module and its types
 * @ingroup grp_modules
 */

namespace DFHack
{
    struct DFHACK_EXPORT ItemTypeInfo {
        df::item_type type;
        int16_t subtype;

        df::itemdef *custom;

    public:
        ItemTypeInfo(df::item_type type_ = df::enums::item_type::NONE, int16_t subtype_ = -1) {
            decode(type_, subtype_);
        }
        template<class T> ItemTypeInfo(T *ptr) { decode(ptr); }

        bool isValid() const {
            return (type != df::enums::item_type::NONE) && (subtype == -1 || custom);
        }

        bool decode(df::item_type type_, int16_t subtype_ = -1);
        bool decode(df::item *ptr);

        template<class T> bool decode(T *ptr) {
            return ptr ? decode(ptr->item_type, ptr->item_subtype) : decode(df::enums::item_type::NONE);
        }

        std::string getToken();
        std::string toString();

        bool find(const std::string &token);

        bool matches(df::job_item_vector_id vec_id);
        bool matches(const df::job_item &item, MaterialInfo *mat = NULL,
                     bool skip_vector = false,
                     df::item_type itype = df::item_type::NONE);
    };

    inline bool operator== (const ItemTypeInfo &a, const ItemTypeInfo &b) {
        return a.type == b.type && a.subtype == b.subtype;
    }
    inline bool operator!= (const ItemTypeInfo &a, const ItemTypeInfo &b) {
        return a.type != b.type || a.subtype != b.subtype;
    }

/**
 * The Items module
 * \ingroup grp_modules
 * \ingroup grp_items
 */
namespace Items
{

DFHACK_EXPORT bool isCasteMaterial(df::item_type itype);
DFHACK_EXPORT int getSubtypeCount(df::item_type itype);
DFHACK_EXPORT df::itemdef *getSubtypeDef(df::item_type itype, int subtype);

/// Look for a particular item by ID
DFHACK_EXPORT df::item * findItemByID(int32_t id);

/// Retrieve refs
DFHACK_EXPORT df::general_ref *getGeneralRef(df::item *item, df::general_ref_type type);
DFHACK_EXPORT df::specific_ref *getSpecificRef(df::item *item, df::specific_ref_type type);

/// Retrieve the owner of the item.
DFHACK_EXPORT df::unit *getOwner(df::item *item);
/// Set the owner of the item. Pass NULL as unit to remove the owner.
DFHACK_EXPORT bool setOwner(df::item *item, df::unit *unit);

/// which item is it contained in?
DFHACK_EXPORT df::item *getContainer(df::item *item);
/// what is the outermost object it is contained in? Possible ref types: UNIT, ITEM_GENERAL, VERMIN_EVENT
DFHACK_EXPORT void getOuterContainerRef(df::specific_ref &spec_ref, df::item *item, bool init_ref = true);
DFHACK_EXPORT inline df::specific_ref getOuterContainerRef(df::item *item) { df::specific_ref s; getOuterContainerRef(s, item); return s; }
/// which items does it contain?
DFHACK_EXPORT void getContainedItems(df::item *item, /*output*/ std::vector<df::item*> *items);

/// which building holds it?
DFHACK_EXPORT df::building *getHolderBuilding(df::item *item);
/// which unit holds it?
DFHACK_EXPORT df::unit *getHolderUnit(df::item *item);

/// Returns the true position of the item.
DFHACK_EXPORT df::coord getPosition(df::item *item);

/// Returns the title of a codex or "tool", either as the codex title or as the title of the
/// first page or writing it has that has a non blank title. An empty string is returned if
/// no title is found (which is the case for everything that isn't a "book").
DFHACK_EXPORT std::string getBookTitle(df::item *item);

/// Returns the description string of the item.
DFHACK_EXPORT std::string getDescription(df::item *item, int type = 0, bool decorate = false);

DFHACK_EXPORT std::string getReadableDescription(df::item *item);

DFHACK_EXPORT bool moveToGround(MapExtras::MapCache &mc, df::item *item, df::coord pos);
DFHACK_EXPORT bool moveToContainer(MapExtras::MapCache &mc, df::item *item, df::item *container);
DFHACK_EXPORT bool moveToBuilding(MapExtras::MapCache &mc, df::item *item, df::building_actual *building,
    df::building_item_role_type use_mode = df::building_item_role_type::TEMP, bool force_in_building = false);
DFHACK_EXPORT bool moveToInventory(MapExtras::MapCache &mc, df::item *item, df::unit *unit,
    df::unit_inventory_item::T_mode mode = df::unit_inventory_item::Hauled, int body_part = -1);

/// Makes the item removed and marked for garbage collection
DFHACK_EXPORT bool remove(MapExtras::MapCache &mc, df::item *item, bool no_uncat = false);

/// Detaches the items from its current location and turns it into a projectile
DFHACK_EXPORT df::proj_itemst *makeProjectile(MapExtras::MapCache &mc, df::item *item);

/// Gets value of base-quality item with specified type and material
DFHACK_EXPORT int getItemBaseValue(int16_t item_type, int16_t item_subtype, int16_t mat_type, int32_t mat_subtype);

/// Gets the value of a specific item, taking into account civ values and trade agreements if a caravan is given
DFHACK_EXPORT int getValue(df::item *item, df::caravan_state *caravan = NULL);

DFHACK_EXPORT int32_t createItem(df::item_type type, int16_t item_subtype, int16_t mat_type, int32_t mat_index, df::unit* creator);

/// Returns true if the item is free from mandates, or false if mandates prevent trading the item
DFHACK_EXPORT bool checkMandates(df::item *item);
/// Checks whether the item can be traded
DFHACK_EXPORT bool canTrade(df::item *item);
/// Returns false if the item or any contained items cannot be traded
DFHACK_EXPORT bool canTradeWithContents(df::item *item);
/// Returns true if the item is empty and can be traded or if the item contains any item that can be traded
DFHACK_EXPORT bool canTradeAnyWithContents(df::item *item);
/// marks the given item for trade at the given depot
DFHACK_EXPORT bool markForTrade(df::item *item, df::building_tradedepotst *depot);
/// Returns true if an active caravan will pay extra for the given item
DFHACK_EXPORT bool isRequestedTradeGood(df::item *item, df::caravan_state *caravan = NULL);

/// Returns true if the item can be melted
DFHACK_EXPORT bool canMelt(df::item *item, bool game_ui = false);
/// Marks the item for melting
DFHACK_EXPORT bool markForMelting(df::item *item);
/// Cancels an existing melting designation
DFHACK_EXPORT bool cancelMelting(df::item *item);

/// Checks whether the item is an assigned hauling vehicle
DFHACK_EXPORT bool isRouteVehicle(df::item *item);
/// Checks whether the item is assigned to a squad
DFHACK_EXPORT bool isSquadEquipment(df::item *item);
/// Returns the item's capacity as a storage container
DFHACK_EXPORT int32_t getCapacity(df::item* item);

}
}
