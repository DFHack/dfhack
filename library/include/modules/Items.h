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
#include "Virtual.h"
#include "modules/Materials.h"
#include "MemAccess.h"

#include "DataDefs.h"
#include "df/item.h"
#include "df/item_type.h"

namespace df
{
    struct itemdef;
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

class Context;
class DFContextShared;
class Units;

/**
 * Describes relationship of an item with other objects
 * \ingroup grp_items
 */
struct t_itemref : public t_virtual
{
    int32_t value;
};

struct df_contaminant
{
    int16_t material;
    int32_t mat_index;
    int16_t mat_state; // FIXME: enum or document in text
    int16_t temperature;
    int16_t temperature_fraction; // maybe...
    int32_t size; ///< 1-24=spatter, 25-49=smear, 50-* = coating
};

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
 * Type for holding item improvements. broken/unused.
 * \ingroup grp_items
 */
struct t_improvement
{
    t_material matdesc;
    int32_t quality;
};

/**
 * The Items module
 * \ingroup grp_modules
 * \ingroup grp_items
 */
class DFHACK_EXPORT Items : public Module
{
public:
    /**
     * All the known item types as an enum
     * From http://df.magmawiki.com/index.php/DF2010:Item_token
     */

public:
    Items();
    ~Items();
    bool Start();
    bool Finish();
    /// Read the item vector from DF into a supplied vector
    bool readItemVector(std::vector<df::item *> &items);
    /// Look for a particular item by ID
    df::item * findItemByID(int32_t id);

    /// Make a partial copy of a DF item
    bool copyItem(df::item * source, dfh_item & target);
    /// write copied item back to its origin
    bool writeItem(const dfh_item & item);

    /// get the class name of an item
    std::string getItemClass(const df::item * item);
    /// who owns this item we already read?
    int32_t getItemOwnerID(const df::item * item);
    /// which item is it contained in?
    int32_t getItemContainerID(const df::item * item);
    /// which items does it contain?
    bool getContainedItems(const df::item * item, /*output*/ std::vector<int32_t> &items);
    /// wipe out the owner records
    bool removeItemOwner(df::item * item, Units *creatures);
    /// read item references, filtered by class
    bool readItemRefs(const df::item * item, const ClassNameCheck &classname,
                      /*output*/ std::vector<int32_t> &values);
    /// get list of item references that are unknown along with their values
    bool unknownRefs(const df::item * item, /*output*/ std::vector<std::pair<std::string, int32_t> >& refs);
private:
    class Private;
    Private* d;
};
}
