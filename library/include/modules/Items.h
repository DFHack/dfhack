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
#include "DataDefs.h"
#include "Export.h"
#include "MemAccess.h"
#include "Module.h"
#include "Types.h"

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
    ItemTypeInfo(df::item_type type_ = df::enums::item_type::NONE, int16_t subtype_ = -1) { decode(type_, subtype_); }
    template<class T> ItemTypeInfo(T *ptr) { decode(ptr); }

    bool isValid() const { return (type != df::enums::item_type::NONE) && (subtype == -1 || custom); }

    bool decode(df::item_type type_, int16_t subtype_ = -1);
    bool decode(df::item *ptr);

    template<class T> bool decode(T *ptr) {
        return ptr ? decode(ptr->item_type, ptr->item_subtype) : decode(df::enums::item_type::NONE);
    }

    std::string getToken();
    std::string toString();

    // Token should look like "TOOL" or "TOOL:ITEM_TOOL_HIVE".
    bool find(const std::string &token);

    bool matches(df::job_item_vector_id vec_id);
    bool matches(const df::job_item &jitem, MaterialInfo *mat = NULL,
        bool skip_vector = false, df::item_type itype = df::item_type::NONE);
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
// Returns true if item type uses creature/caste pair as material.
DFHACK_EXPORT bool isCasteMaterial(df::item_type itype);
// Returns the number of raw-defined subtypes of item type (or -1 if N/A).
DFHACK_EXPORT int getSubtypeCount(df::item_type itype);
// Returns the raw definition for given item type and subtype or NULL.
DFHACK_EXPORT df::itemdef *getSubtypeDef(df::item_type itype, int subtype);

// Look for a particular item by ID.
DFHACK_EXPORT df::item *findItemByID(int32_t id);

// Retrieve refs
DFHACK_EXPORT df::general_ref *getGeneralRef(df::item *item, df::general_ref_type type);
DFHACK_EXPORT df::specific_ref *getSpecificRef(df::item *item, df::specific_ref_type type);

// Retrieve the owner of the item or NULL.
DFHACK_EXPORT df::unit *getOwner(df::item *item);
// Set the owner of the item. Pass NULL as unit to remove the owner.
DFHACK_EXPORT bool setOwner(df::item *item, df::unit *unit);

// Get the item's container item or NULL.
DFHACK_EXPORT df::item *getContainer(df::item *item);
/// Ref to the outermost object item is contained in. Possible ref types: UNIT, ITEM_GENERAL, VERMIN_EVENT
/// (init_ref is used to initialize the ref to the item itself before recursive calls.)
DFHACK_EXPORT void getOuterContainerRef(df::specific_ref &spec_ref, df::item *item, bool init_ref = true);
DFHACK_EXPORT inline df::specific_ref getOuterContainerRef(df::item *item) { df::specific_ref s; getOuterContainerRef(s, item); return s; }
// Fill vector with all items stored inside of the item.
DFHACK_EXPORT void getContainedItems(df::item *item, /*output*/ std::vector<df::item *> *items);

// Get building that holds the item or NULL.
DFHACK_EXPORT df::building *getHolderBuilding(df::item *item);
// Get unit that holds the item or NULL.
DFHACK_EXPORT df::unit *getHolderUnit(df::item *item);

// Get the true position of the item (non-trivial if in inventory). Returns invalid coord if holder off map.
DFHACK_EXPORT df::coord getPosition(df::item *item);

/// Returns the title of a codex or "tool", either as the codex title or as the title of the
/// first page or writing it has that has a non blank title. An empty string is returned if
/// no title is found (which is the case for everything that isn't a "book").
DFHACK_EXPORT std::string getBookTitle(df::item *item);

/// Returns the description string of the item with quality modifiers.
/// type: 0 = prickle berries [2], 1 = prickle berry, 2 = prickle berries
/// If decorate, add quality/improvement indicators and "(foreign)".
DFHACK_EXPORT std::string getDescription(df::item *item, int type = 0, bool decorate = false);
// Includes wear level, book/artifact title, and any caged units.
DFHACK_EXPORT std::string getReadableDescription(df::item *item);

DFHACK_EXPORT bool moveToGround(df::item *item, df::coord pos);
DFHACK_EXPORT bool moveToContainer(df::item *item, df::item *container);
DFHACK_EXPORT bool moveToBuilding(df::item *item, df::building_actual *building,
    df::building_item_role_type use_mode = df::building_item_role_type::TEMP, bool force_in_building = false);
DFHACK_EXPORT bool moveToInventory(df::item *item, df::unit *unit,
    df::inv_item_role_type mode = df::inv_item_role_type::Hauled, int body_part = -1);

/// Remove item from jobs and inventories, hide and forbid.
/// Unless no_uncat, item is marked for garbage collection.
DFHACK_EXPORT bool remove(df::item *item, bool no_uncat = false);

// Detaches the item from its current location and turns it into a projectile.
DFHACK_EXPORT df::proj_itemst *makeProjectile(df::item *item);

// Gets value of base-quality item with specified type and material.
DFHACK_EXPORT int getItemBaseValue(int16_t item_type, int16_t item_subtype, int16_t mat_type, int32_t mat_subtype);

// Gets the value of a specific item, taking into account civ values and trade agreements if a caravan is given.
DFHACK_EXPORT int getValue(df::item *item, df::caravan_state *caravan = NULL);

DFHACK_EXPORT bool createItem(std::vector<df::item *> &out_items, df::unit *creator, df::item_type type,
    int16_t item_subtype, int16_t mat_type, int32_t mat_index, bool no_floor = false);

// Returns true if the item is free from mandates, or false if mandates prevent trading the item.
DFHACK_EXPORT bool checkMandates(df::item *item);
// Checks whether the item can be traded.
DFHACK_EXPORT bool canTrade(df::item *item);
// Returns false if the item or any contained items cannot be traded.
DFHACK_EXPORT bool canTradeWithContents(df::item *item);
// Returns true if the item is empty and can be traded or if the item contains any item that can be traded.
DFHACK_EXPORT bool canTradeAnyWithContents(df::item *item);
// Marks the given item for trade at the given depot.
DFHACK_EXPORT bool markForTrade(df::item *item, df::building_tradedepotst *depot);
// Returns true if an active caravan will pay extra for the given item.
DFHACK_EXPORT bool isRequestedTradeGood(df::item *item, df::caravan_state *caravan = NULL);

// Returns true if the item can currently be melted. If game_ui, then able to be marked is enough.
DFHACK_EXPORT bool canMelt(df::item *item, bool game_ui = false);
// Marks the item for melting.
DFHACK_EXPORT bool markForMelting(df::item *item);
// Cancels an existing melting designation.
DFHACK_EXPORT bool cancelMelting(df::item *item);

// Checks whether the item is an assigned hauling vehicle.
DFHACK_EXPORT bool isRouteVehicle(df::item *item);
// Checks whether the item is assigned to a squad.
DFHACK_EXPORT bool isSquadEquipment(df::item *item);
// Returns the item's capacity as a storage container.
DFHACK_EXPORT int32_t getCapacity(df::item *item);
}
}
