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

#pragma once
/*
* Items!
*/
#include "Export.h"
#include "Module.h"
#include "Types.h"
#include "modules/Materials.h"
#include "MemAccess.h"

#include "DataDefs.h"
#include "df/item.h"
#include "df/item_type.h"
#include "df/general_ref.h"
#include "df/specific_ref.h"
#include "df/building_actual.h"
#include "df/body_part_raw.h"
#include "df/unit_inventory_item.h"

namespace df
{
    struct itemdef;
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

        bool matches(const df::job_item &item, MaterialInfo *mat = NULL);
    };

    inline bool operator== (const ItemTypeInfo &a, const ItemTypeInfo &b) {
        return a.type == b.type && a.subtype == b.subtype;
    }
    inline bool operator!= (const ItemTypeInfo &a, const ItemTypeInfo &b) {
        return a.type != b.type || a.subtype != b.subtype;
    }

/**
 * Type for holding an item read from DF
 * \ingroup grp_items
 */
struct dfh_item
{
    df::item *origin; // where this was read from
    int16_t x;
    int16_t y;
    int16_t z;
    df::item_flags flags;
    uint32_t age;
    uint32_t id;
    t_material matdesc;
    int32_t quantity;
    int32_t quality;
    int16_t wear_level;
};

/**
 * The Items module
 * \ingroup grp_modules
 * \ingroup grp_items
 */
namespace Items
{

/// Look for a particular item by ID
DFHACK_EXPORT df::item * findItemByID(int32_t id);

/// Make a partial copy of a DF item
DFHACK_EXPORT bool copyItem(df::item * source, dfh_item & target);
/// write copied item back to its origin
DFHACK_EXPORT bool writeItem(const dfh_item & item);

/// Retrieve refs
DFHACK_EXPORT df::general_ref *getGeneralRef(df::item *item, df::general_ref_type type);
DFHACK_EXPORT df::specific_ref *getSpecificRef(df::item *item, df::specific_ref_type type);

/// Retrieve the owner of the item.
DFHACK_EXPORT df::unit *getOwner(df::item *item);
/// Set the owner of the item. Pass NULL as unit to remove the owner.
DFHACK_EXPORT bool setOwner(df::item *item, df::unit *unit);

/// which item is it contained in?
DFHACK_EXPORT df::item *getContainer(df::item *item);
/// which items does it contain?
DFHACK_EXPORT void getContainedItems(df::item *item, /*output*/ std::vector<df::item*> *items);

/// Returns the true position of the item.
DFHACK_EXPORT df::coord getPosition(df::item *item);

/// Returns the description string of the item.
DFHACK_EXPORT std::string getDescription(df::item *item, int type = 0, bool decorate = false);

DFHACK_EXPORT bool moveToGround(MapExtras::MapCache &mc, df::item *item, df::coord pos);
DFHACK_EXPORT bool moveToContainer(MapExtras::MapCache &mc, df::item *item, df::item *container);
DFHACK_EXPORT bool moveToBuilding(MapExtras::MapCache &mc, df::item *item, df::building_actual *building,int16_t use_mode);
DFHACK_EXPORT bool moveToInventory(MapExtras::MapCache &mc, df::item *item, df::unit *unit,
    df::unit_inventory_item::T_mode mode = df::unit_inventory_item::Carried, int body_part = -1);
}
}
