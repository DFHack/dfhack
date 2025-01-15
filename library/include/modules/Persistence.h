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

#include "ColorText.h"
#include "Error.h"
#include "Export.h"

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

        // these throw if used when isValid() returns false
        std::string &val();
        const std::string &val() const;
        int &ival(int i);
        int ival(int i) const;

        // safer, non-throwing accessors with convenience bool functions
        int get_int(int i) const {
            if (!isValid())
                return -1;
            return ival(i);
        }
        bool get_bool(int i) {
            return get_int(i) == 1;
        }
        void set_int(int i, int value) {
            if (isValid())
                ival(i) = value;
        }
        void set_bool(int i, bool value) {
            set_int(i, value ? 1 : 0);
        }
        const std::string & get_str();
        void set_str(const std::string & value) {
            if (isValid())
                val() = value;
        }

        // Data mangling functions below this point are deprecated and
        // will be removed in some future release when we have provided
        // an alternate way to store binary data.

        // Pack binary data into string field.
        // Since DF serialization chokes on NUL bytes,
        // use bit magic to ensure none of the bytes is 0.
        // Choose the lowest bit for padding so that
        // sign-extend can be used normally.

        size_t data_size() const
        {
            return val().size();
        }

        bool check_data(size_t off, size_t sz = 1) const
        {
            return (data_size() >= off + sz);
        }
        void ensure_data(size_t off, size_t sz = 0)
        {
            if (data_size() < off + sz)
            {
                val().resize(off + sz, '\x01');
            }
        }
        template<size_t N>
        uint8_t (&pdata(size_t off))[N]
        {
            ensure_data(off, N);
            typedef uint8_t array[N];
            return *(array *)(val().data() + off);
        }
        template<size_t N>
        const uint8_t (&pdata(size_t off) const)[N]
        {
            CHECK_INVALID_ARGUMENT(check_data(off, N));
            typedef const uint8_t array[N];
            return *(array *)(val().data() + off);
        }
        template<size_t N>
        const uint8_t (&cpdata(size_t off) const)[N]
        {
            return pdata<N>(off);
        }

        static const size_t int7_size = 1;
        uint8_t get_uint7(size_t off) const
        {
            auto p = pdata<int7_size>(off);
            return p[0] >> 1;
        }
        int8_t get_int7(size_t off) const
        {
            return int8_t(get_uint7(off));
        }
        void set_uint7(size_t off, uint8_t val)
        {
            auto p = pdata<int7_size>(off);
            p[0] = uint8_t((val << 1) | 1);
        }
        void set_int7(size_t off, int8_t val)
        {
            set_uint7(off, uint8_t(val));
        }

        static const size_t int28_size = 4;
        uint32_t get_uint28(size_t off) const
        {
            auto p = pdata<int28_size>(off);
            return (p[0]>>1) | ((p[1]&~1U)<<6) | ((p[2]&~1U)<<13) | ((p[3]&~1U)<<20);
        }
        int32_t get_int28(size_t off) const
        {
            return int32_t(get_uint28(off));
        }
        void set_uint28(size_t off, uint32_t val)
        {
            auto p = pdata<int28_size>(off);
            p[0] = uint8_t((val<<1) | 1);
            p[1] = uint8_t((val>>6) | 1);
            p[2] = uint8_t((val>>13) | 1);
            p[3] = uint8_t((val>>20) | 1);
        }
        void set_int28(size_t off, int32_t val)
        {
            set_uint28(off, val);
        }

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
        // Returns the tickcount of the most recent save.
        DFHACK_EXPORT uint32_t getSaveDur();
    }
}
