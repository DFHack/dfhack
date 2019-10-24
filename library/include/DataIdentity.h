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

#include <deque>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "DataDefs.h"

/*
 * Definitions of DFHack namespace structs used by generated headers.
 */

namespace DFHack
{
    class DFHACK_EXPORT function_identity_base : public type_identity {
        int num_args;
        bool vararg;

    public:
        function_identity_base(int num_args, bool vararg = false)
            : type_identity(0), num_args(num_args), vararg(vararg) {};

        virtual identity_type type() const { return IDTYPE_FUNCTION; }

        int getNumArgs() { return num_args; }
        bool adjustArgs() { return vararg; }

        std::string getFullName() const { return "function"; }

        virtual void invoke(lua_State *state, int base) = 0;

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class DFHACK_EXPORT primitive_identity : public type_identity {
    public:
        primitive_identity(size_t size) : type_identity(size) {};

        virtual identity_type type() const { return IDTYPE_PRIMITIVE; }
    };

    class DFHACK_EXPORT opaque_identity : public constructed_identity {
        std::string name;

    public:
        opaque_identity(size_t size, TAllocateFn alloc, const std::string &name)
          : constructed_identity(size, alloc), name(name) {};

        virtual std::string getFullName() const { return name; }
        virtual identity_type type() const { return IDTYPE_OPAQUE; }
    };

    class DFHACK_EXPORT pointer_identity : public primitive_identity {
        type_identity *target;

    public:
        pointer_identity(type_identity *target = NULL)
            : primitive_identity(sizeof(void*)), target(target) {};

        virtual identity_type type() const { return IDTYPE_POINTER; }

        type_identity *getTarget() const { return target; }

        std::string getFullName() const;

        static void lua_read(lua_State *state, int fname_idx, void *ptr, type_identity *target);
        static void lua_write(lua_State *state, int fname_idx, void *ptr, type_identity *target, int val_index);

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class DFHACK_EXPORT container_identity : public constructed_identity {
        type_identity *item;
        enum_identity *ienum;

    public:
        container_identity(size_t size, TAllocateFn alloc, type_identity *item, enum_identity *ienum = NULL)
            : constructed_identity(size, alloc), item(item), ienum(ienum) {};

        virtual identity_type type() const { return IDTYPE_CONTAINER; }

        std::string getFullName() const { return getFullName(item); }

        virtual void build_metatable(lua_State *state);
        virtual bool isContainer() { return true; }

        type_identity *getItemType() const { return item; }
        type_identity *getIndexEnumType() { return ienum; }

        virtual std::string getFullName(type_identity *item) const;

        enum CountMode {
            COUNT_LEN, COUNT_READ, COUNT_WRITE
        };

        int lua_item_count(lua_State *state, void *ptr, CountMode cnt);

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index);

        virtual bool is_readonly() { return false; }

        virtual bool resize(void *ptr, int size) { return false; }
        virtual bool erase(void *ptr, int index) { return false; }
        virtual bool insert(void *ptr, int index, void *pitem) { return false; }

        virtual bool lua_insert2(lua_State *state, int fname_idx, void *ptr, int idx, int val_index);

    protected:
        virtual int item_count(void *ptr, CountMode cnt) = 0;
        virtual void *item_pointer(type_identity *item, void *ptr, int idx) = 0;
    };

    class DFHACK_EXPORT ptr_container_identity : public container_identity {
    public:
        ptr_container_identity(size_t size, TAllocateFn alloc,
                               type_identity *item, enum_identity *ienum = NULL)
            : container_identity(size, alloc, item, ienum) {};

        virtual identity_type type() const { return IDTYPE_PTR_CONTAINER; }

        std::string getFullName(type_identity *item) const;

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index);

        virtual bool lua_insert2(lua_State *state, int fname_idx, void *ptr, int idx, int val_index);
    };

    class DFHACK_EXPORT bit_container_identity : public container_identity {
    public:
        bit_container_identity(size_t size, TAllocateFn alloc, enum_identity *ienum = NULL)
            : container_identity(size, alloc, NULL, ienum) {};

        virtual identity_type type() const { return IDTYPE_BIT_CONTAINER; }

        std::string getFullName(type_identity *item) const;

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index);

    protected:
        virtual void *item_pointer(type_identity *, void *, int) { return NULL; }

        virtual bool get_item(void *ptr, int idx) = 0;
        virtual void set_item(void *ptr, int idx, bool val) = 0;
    };
}

namespace df
{
    using DFHack::function_identity_base;
    using DFHack::primitive_identity;
    using DFHack::opaque_identity;
    using DFHack::pointer_identity;
    using DFHack::container_identity;
    using DFHack::ptr_container_identity;
    using DFHack::bit_container_identity;

    class DFHACK_EXPORT number_identity_base : public primitive_identity {
        const char *name;

    public:
        number_identity_base(size_t size, const char *name)
            : primitive_identity(size), name(name) {};

        std::string getFullName() const { return name; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) = 0;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) = 0;

    };

    class DFHACK_EXPORT integer_identity_base : public number_identity_base {
    public:
        integer_identity_base(size_t size, const char *name)
            : number_identity_base(size, name) {}

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);

    protected:
        virtual int64_t read(void *ptr) = 0;
        virtual void write(void *ptr, int64_t val) = 0;
    };

    class DFHACK_EXPORT float_identity_base : public number_identity_base {
    public:
        float_identity_base(size_t size, const char *name)
            : number_identity_base(size, name) {}

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);

    protected:
        virtual double read(void *ptr) = 0;
        virtual void write(void *ptr, double val) = 0;
    };

    template<class T>
    class integer_identity : public integer_identity_base {
    public:
        integer_identity(const char *name) : integer_identity_base(sizeof(T), name) {}

    protected:
        virtual int64_t read(void *ptr) { return int64_t(*(T*)ptr); }
        virtual void write(void *ptr, int64_t val) { *(T*)ptr = T(val); }
    };

    template<class T>
    class float_identity : public float_identity_base {
    public:
        float_identity(const char *name) : float_identity_base(sizeof(T), name) {}

    protected:
        virtual double read(void *ptr) { return double(*(T*)ptr); }
        virtual void write(void *ptr, double val) { *(T*)ptr = T(val); }
    };

    class DFHACK_EXPORT bool_identity : public primitive_identity {
    public:
        bool_identity() : primitive_identity(sizeof(bool)) {};

        std::string getFullName() const { return "bool"; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class DFHACK_EXPORT ptr_string_identity : public primitive_identity {
    public:
        ptr_string_identity() : primitive_identity(sizeof(char*)) {};

        std::string getFullName() const { return "char*"; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class DFHACK_EXPORT stl_string_identity : public DFHack::constructed_identity {
    public:
        stl_string_identity()
            : constructed_identity(sizeof(std::string), &allocator_fn<std::string>)
        {};

        std::string getFullName() const { return "string"; }

        virtual DFHack::identity_type type() const { return DFHack::IDTYPE_PRIMITIVE; }

        virtual bool isPrimitive() { return true; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class DFHACK_EXPORT stl_ptr_vector_identity : public ptr_container_identity {
    public:
        typedef std::vector<void*> container;

        /*
         * This class assumes that std::vector<T*> is equivalent
         * in layout and behavior to std::vector<void*> for any T.
         */

        stl_ptr_vector_identity(type_identity *item = NULL, enum_identity *ienum = NULL)
            : ptr_container_identity(sizeof(container),allocator_fn<container>,item, ienum)
        {};

        std::string getFullName(type_identity *item) const {
            return "vector" + ptr_container_identity::getFullName(item);
        }

        virtual DFHack::identity_type type() const { return DFHack::IDTYPE_STL_PTR_VECTOR; }

        virtual bool resize(void *ptr, int size) {
            (*(container*)ptr).resize(size);
            return true;
        }
        virtual bool erase(void *ptr, int size) {
            auto &ct = *(container*)ptr;
            ct.erase(ct.begin()+size);
            return true;
        }
        virtual bool insert(void *ptr, int idx, void *item) {
            auto &ct = *(container*)ptr;
            ct.insert(ct.begin()+idx, item);
            return true;
        }

    protected:
        virtual int item_count(void *ptr, CountMode) {
            return (int)((container*)ptr)->size();
        };
        virtual void *item_pointer(type_identity *, void *ptr, int idx) {
            return &(*(container*)ptr)[idx];
        }
    };

// Due to export issues, this stuff can only work in the main dll
#ifdef BUILD_DFHACK_LIB
    class buffer_container_identity : public container_identity {
        int size;

    public:
        buffer_container_identity()
            : container_identity(0, NULL, NULL, NULL), size(0)
        {}

        buffer_container_identity(int size, type_identity *item, enum_identity *ienum = NULL)
            : container_identity(0, NULL, item, ienum), size(size)
        {}

        size_t byte_size() const { return getItemType()->byte_size()*size; }

        std::string getFullName(type_identity *item) const;
        int getSize() const { return size; }

        virtual DFHack::identity_type type() const { return DFHack::IDTYPE_BUFFER; }

        static buffer_container_identity base_instance;

    protected:
        virtual int item_count(void *ptr, CountMode) { return size; }
        virtual void *item_pointer(type_identity *item, void *ptr, int idx) {
            return ((uint8_t*)ptr) + idx * item->byte_size();
        }
    };

    class stl_container_base_identity : public container_identity {
    public:
        const char * const name;
    protected:
        stl_container_base_identity(size_t size,
                                    DFHack::TAllocateFn alloc,
                                    type_identity *item,
                                    enum_identity *ienum,
                                    const char *n)
            :container_identity{size, alloc, item, ienum}, name{n}
        {}
    };

    template<class T>
    class stl_container_identity : public stl_container_base_identity {
    public:
        stl_container_identity(const char *name, type_identity *item, enum_identity *ienum = NULL)
            : stl_container_base_identity{sizeof(T), &allocator_fn<T>, item, ienum, name}
        {}

        std::string getFullName(type_identity *item) const {
            return name + container_identity::getFullName(item);
        }

        virtual bool resize(void *ptr, int size) {
            (*(T*)ptr).resize(size);
            return true;
        }
        virtual bool erase(void *ptr, int size) {
            auto &ct = *(T*)ptr;
            ct.erase(ct.begin()+size);
            return true;
        }
        virtual bool insert(void *ptr, int idx, void *item) {
            auto &ct = *(T*)ptr;
            ct.insert(ct.begin()+idx, *(typename T::value_type*)item);
            return true;
        }

    protected:
        virtual int item_count(void *ptr, CountMode) { return (int)((T*)ptr)->size(); }
        virtual void *item_pointer(type_identity *item, void *ptr, int idx) {
            return &(*(T*)ptr)[idx];
        }
    };

    template<class T>
    class ro_stl_container_identity : public stl_container_base_identity {
    public:
        ro_stl_container_identity(const char *name, type_identity *item, enum_identity *ienum = NULL)
            : stl_container_base_identity{sizeof(T), &allocator_fn<T>, item, ienum, name}
        {}

        std::string getFullName(type_identity *item) const {
            return name + container_identity::getFullName(item);
        }

        virtual bool is_readonly() { return true; }
        virtual bool resize(void *ptr, int size) { return false; }
        virtual bool erase(void *ptr, int size) { return false; }
        virtual bool insert(void *ptr, int idx, void *item) { return false; }

    protected:
        virtual int item_count(void *ptr, CountMode) { return (int)((T*)ptr)->size(); }
        virtual void *item_pointer(type_identity *item, void *ptr, int idx) {
            auto iter = (*(T*)ptr).begin();
            for (; idx > 0; idx--) ++iter;
            return (void*)&*iter;
        }
    };

    class bit_array_identity : public bit_container_identity {
    public:
        /*
         * This class assumes that BitArray<T> is equivalent
         * in layout and behavior to BitArray<int> for any T.
         */

        typedef BitArray<int> container;

        bit_array_identity(enum_identity *ienum = NULL)
            : bit_container_identity(sizeof(container), &allocator_fn<container>, ienum)
        {}

        std::string getFullName(type_identity *item) const {
            return "BitArray<>";
        }

        virtual bool resize(void *ptr, int size) {
            ((container*)ptr)->resize((size+7)/8);
            return true;
        }

    protected:
        virtual int item_count(void *ptr, CountMode cnt) {
            return cnt == COUNT_LEN ? ((container*)ptr)->size * 8 : -1;
        }
        virtual bool get_item(void *ptr, int idx) {
            return ((container*)ptr)->is_set(idx);
        }
        virtual void set_item(void *ptr, int idx, bool val) {
            ((container*)ptr)->set(idx, val);
        }
    };
#endif

    class DFHACK_EXPORT stl_bit_vector_identity : public bit_container_identity {
    public:
        typedef std::vector<bool> container;

        stl_bit_vector_identity(enum_identity *ienum = NULL)
            : bit_container_identity(sizeof(container), &allocator_fn<container>, ienum)
        {}

        std::string getFullName(type_identity *item) const {
            return "vector" + bit_container_identity::getFullName(item);
        }

        virtual bool resize(void *ptr, int size) {
            (*(container*)ptr).resize(size);
            return true;
        }

    protected:
        virtual int item_count(void *ptr, CountMode) {
            return (int)((container*)ptr)->size();
        }
        virtual bool get_item(void *ptr, int idx) {
            return (*(container*)ptr)[idx];
        }
        virtual void set_item(void *ptr, int idx, bool val) {
            (*(container*)ptr)[idx] = val;
        }
    };

#ifdef BUILD_DFHACK_LIB
    template<class T>
    class enum_list_attr_identity : public container_identity {
    public:
        typedef enum_list_attr<T> container;

        enum_list_attr_identity(type_identity *item)
            : container_identity(sizeof(container), NULL, item, NULL)
        {}

        std::string getFullName(type_identity *item) const {
            return "enum_list_attr" + container_identity::getFullName(item);
        }

    protected:
        virtual int item_count(void *ptr, CountMode cm) {
            return cm == COUNT_WRITE ? 0 : (int)((container*)ptr)->size;
        }
        virtual void *item_pointer(type_identity *item, void *ptr, int idx) {
            return (void*)&((container*)ptr)->items[idx];
        }
    };
#endif

#define NUMBER_IDENTITY_TRAITS(category, type) \
    template<> struct DFHACK_EXPORT identity_traits<type> { \
        static category##_identity<type> identity; \
        static category##_identity_base *get() { return &identity; } \
    };

#define INTEGER_IDENTITY_TRAITS(type) NUMBER_IDENTITY_TRAITS(integer, type)
#define FLOAT_IDENTITY_TRAITS(type) NUMBER_IDENTITY_TRAITS(float, type)

    INTEGER_IDENTITY_TRAITS(char);
    INTEGER_IDENTITY_TRAITS(signed char);
    INTEGER_IDENTITY_TRAITS(unsigned char);
    INTEGER_IDENTITY_TRAITS(short);
    INTEGER_IDENTITY_TRAITS(unsigned short);
    INTEGER_IDENTITY_TRAITS(int);
    INTEGER_IDENTITY_TRAITS(unsigned int);
    INTEGER_IDENTITY_TRAITS(long);
    INTEGER_IDENTITY_TRAITS(unsigned long);
    INTEGER_IDENTITY_TRAITS(long long);
    INTEGER_IDENTITY_TRAITS(unsigned long long);
    FLOAT_IDENTITY_TRAITS(float);
    FLOAT_IDENTITY_TRAITS(double);

    template<> struct DFHACK_EXPORT identity_traits<bool> {
        static bool_identity identity;
        static bool_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::string> {
        static stl_string_identity identity;
        static stl_string_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::fstream> {
        static opaque_identity identity;
        static opaque_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<char*> {
        static ptr_string_identity identity;
        static ptr_string_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<const char*> {
        static ptr_string_identity identity;
        static ptr_string_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<void*> {
        static pointer_identity identity;
        static pointer_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::vector<void*> > {
        static stl_ptr_vector_identity identity;
        static stl_ptr_vector_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::vector<bool> > {
        static stl_bit_vector_identity identity;
        static stl_bit_vector_identity *get() { return &identity; }
    };

#undef NUMBER_IDENTITY_TRAITS
#undef INTEGER_IDENTITY_TRAITS
#undef FLOAT_IDENTITY_TRAITS

    // Container declarations

#ifdef BUILD_DFHACK_LIB
    template<class Enum, class FT> struct identity_traits<enum_field<Enum,FT> > {
        static primitive_identity *get();
    };
#endif

    template<class T> struct identity_traits<T *> {
        static pointer_identity *get();
    };

#ifdef BUILD_DFHACK_LIB
    template<class T, int sz> struct identity_traits<T [sz]> {
        static container_identity *get();
    };

    template<class T> struct identity_traits<std::vector<T> > {
        static container_identity *get();
    };
#endif

    template<class T> struct identity_traits<std::vector<T*> > {
        static stl_ptr_vector_identity *get();
    };

#ifdef BUILD_DFHACK_LIB
    template<class T> struct identity_traits<std::deque<T> > {
        static container_identity *get();
    };

    template<class T> struct identity_traits<std::set<T> > {
        static container_identity *get();
    };

    template<> struct identity_traits<BitArray<int> > {
        static bit_array_identity identity;
        static bit_container_identity *get() { return &identity; }
    };

    template<class T> struct identity_traits<BitArray<T> > {
        static bit_container_identity *get();
    };

    template<class T> struct identity_traits<DfArray<T> > {
        static container_identity *get();
    };

    template<class T> struct identity_traits<enum_list_attr<T> > {
        static container_identity *get();
    };
#endif

    // Container definitions

#ifdef BUILD_DFHACK_LIB
    template<class Enum, class FT>
    inline primitive_identity *identity_traits<enum_field<Enum,FT> >::get() {
        return identity_traits<FT>::get();
    }
#endif

    template<class T>
    inline pointer_identity *identity_traits<T *>::get() {
        static pointer_identity identity(identity_traits<T>::get());
        return &identity;
    }

#ifdef BUILD_DFHACK_LIB
    template<class T, int sz>
    inline container_identity *identity_traits<T [sz]>::get() {
        static buffer_container_identity identity(sz, identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline container_identity *identity_traits<std::vector<T> >::get() {
        typedef std::vector<T> container;
        static stl_container_identity<container> identity("vector", identity_traits<T>::get());
        return &identity;
    }
#endif

    template<class T>
    inline stl_ptr_vector_identity *identity_traits<std::vector<T*> >::get() {
        static stl_ptr_vector_identity identity(identity_traits<T>::get());
        return &identity;
    }

#ifdef BUILD_DFHACK_LIB
    template<class T>
    inline container_identity *identity_traits<std::deque<T> >::get() {
        typedef std::deque<T> container;
        static stl_container_identity<container> identity("deque", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline container_identity *identity_traits<std::set<T> >::get() {
        typedef std::set<T> container;
        static ro_stl_container_identity<container> identity("set", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline bit_container_identity *identity_traits<BitArray<T> >::get() {
        static bit_array_identity identity(identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline container_identity *identity_traits<DfArray<T> >::get() {
        typedef DfArray<T> container;
        static stl_container_identity<container> identity("DfArray", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline container_identity *identity_traits<enum_list_attr<T> >::get() {
        static enum_list_attr_identity<T> identity(identity_traits<T>::get());
        return &identity;
    }
#endif
}
