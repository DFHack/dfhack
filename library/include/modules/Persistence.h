/*
https://github.com/DFHack/dfhack
Copyright (c) 2009-2018 DFHack Team

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

/**
 * \defgroup grp_persistence Persistence: code related to saving and loading data
 * @ingroup grp_modules
 */

#include "Export.h"
#include "ColorText.h"

#include <memory>
#include <string>
#include <vector>

namespace DFHack
{
    class Core;

    namespace Persistence
    {
        struct DataEntry;
    }

    class DFHACK_EXPORT PersistentDataItem {
        std::shared_ptr<Persistence::DataEntry> data;
        friend struct Persistence::DataEntry;

    public:
        static const size_t NumInts = 7;

        bool isValid() const;

        // Used for associating this data item with a map block tile mask
        int fake_df_id();

        int entity_id() const;
        const std::string &key() const;

        std::string &val();
        const std::string &val() const;
        int &ival(int i);
        int ival(int i) const;

        PersistentDataItem() : data(nullptr) {}
        PersistentDataItem(const std::shared_ptr<Persistence::DataEntry> &data) : data(data) {}
    };

    namespace Persistence
    {
        class Internal {
            static void clear(color_ostream& out);
            static void save(color_ostream& out);
            static void load(color_ostream& out);
            friend class ::DFHack::Core;
        };

        const int WORLD_ENTITY_ID = -30000;

        // Returns a new PersistentDataItem with the specified key associated wtih the specified
        // entity_id. Pass WORLD_ENTITY_ID for the entity_id to indicate the global world context.
        // If there is no world loaded or the key is empty, returns an invalid item.
        DFHACK_EXPORT PersistentDataItem addItem(int entity_id, const std::string &key);
        // Returns an existing PersistentDataItem with the specified key.
        // If "added" is not null and there is no such item, a new item is returned and
        // the bool value is set to true. If "added" is not null and an item is found or
        // no new item can be created, the bool value is set to false.
        DFHACK_EXPORT PersistentDataItem getByKey(int entity_id, const std::string &key, bool *added = nullptr);
        // If the item is invalid, returns false. Otherwise, deletes the item and returns
        // true. All references to the item are invalid as soon as this function returns.
        DFHACK_EXPORT bool deleteItem(const PersistentDataItem &item);
        // Fills the vector with references to each persistent item.
        DFHACK_EXPORT void getAll(std::vector<PersistentDataItem> &vec, int entity_id);
        // Fills the vector with references to each persistent item with a key that is
        // greater than or equal to "min" and less than "max".
        DFHACK_EXPORT void getAllByKeyRange(std::vector<PersistentDataItem> &vec, int entity_id, const std::string &min, const std::string &max);
        // Fills the vector with references to each persistent item with a key that is
        // equal to the given key.
        DFHACK_EXPORT void getAllByKey(std::vector<PersistentDataItem> &vec, int entity_id, const std::string &key);
    }
}
