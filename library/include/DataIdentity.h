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
#include <future>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <variant>

#include "DataDefs.h"

namespace std {
    class condition_variable;
    class mutex;
};

namespace df {
  struct widget_container;
}

/*
 * Definitions of DFHack namespace structs used by generated headers.
 */

namespace DFHack
{
    class DFHACK_EXPORT function_identity_base : public type_identity {
        const int num_args;
        const bool vararg;

    public:
        function_identity_base(int num_args, bool vararg = false)
            : type_identity(0), num_args(num_args), vararg(vararg) {};

        virtual identity_type type() const { return IDTYPE_FUNCTION; }

        int getNumArgs() const { return num_args; }
        bool adjustArgs() const { return vararg; }

        const std::string getFullName() const { return "function"; }

        virtual void invoke(lua_State *state, int base) const = 0;

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const;
    };

    class DFHACK_EXPORT primitive_identity : public type_identity {
    public:
        primitive_identity(size_t size) : type_identity(size) {};

        virtual identity_type type() const { return IDTYPE_PRIMITIVE; }
    };

    class DFHACK_EXPORT opaque_identity : public constructed_identity {
        const std::string name;

    public:
        opaque_identity(size_t size, TAllocateFn alloc, const std::string &name)
          : constructed_identity(size, alloc), name(name) {};

        virtual const std::string getFullName() const { return name; }
        virtual identity_type type() const { return IDTYPE_OPAQUE; }
    };

    class DFHACK_EXPORT pointer_identity : public primitive_identity {
        const type_identity *target;

    public:
        pointer_identity(const type_identity *target = NULL)
            : primitive_identity(sizeof(void*)), target(target) {};

        virtual identity_type type() const { return IDTYPE_POINTER; }

        const type_identity *getTarget() const { return target; }

        const std::string getFullName() const;

        static void lua_read(lua_State *state, int fname_idx, void *ptr, const type_identity *target);
        static void lua_write(lua_State *state, int fname_idx, void *ptr, const type_identity *target, int val_index);

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const;
    };

    class DFHACK_EXPORT container_identity : public constructed_identity {
        const type_identity *item;
        const enum_identity *ienum;

    public:
        container_identity(size_t size, const TAllocateFn alloc, const type_identity *item, const enum_identity *ienum = NULL)
            : constructed_identity(size, alloc), item(item), ienum(ienum) {};

        virtual identity_type type() const { return IDTYPE_CONTAINER; }

        const std::string getFullName() const { return getFullName(item); }

        virtual void build_metatable(lua_State *state) const;
        virtual bool isContainer() const { return true; }

        const type_identity *getItemType() const { return item; }
        const type_identity *getIndexEnumType() const { return ienum; }

        virtual const std::string getFullName(const type_identity *item) const;

        enum CountMode {
            COUNT_LEN, COUNT_READ, COUNT_WRITE
        };

        int lua_item_count(lua_State *state, void *ptr, CountMode cnt) const;

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx) const;
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx) const;
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index) const;

        virtual bool is_readonly() const { return false; }

        virtual bool resize(void *ptr, int size) const { return false; }
        virtual bool erase(void *ptr, int index) const { return false; }
        virtual bool insert(void *ptr, int index, void *pitem) const { return false; }

        virtual bool lua_insert2(lua_State *state, int fname_idx, void *ptr, int idx, int val_index) const;

    protected:
        virtual int item_count(void *ptr, CountMode cnt) const = 0;
        virtual void *item_pointer(const type_identity *item, void *ptr, int idx) const = 0;
    };

    class DFHACK_EXPORT ptr_container_identity : public container_identity {
    public:
        ptr_container_identity(size_t size, TAllocateFn alloc,
            const type_identity *item, const enum_identity *ienum = NULL)
            : container_identity(size, alloc, item, ienum) {};

        virtual identity_type type() const { return IDTYPE_PTR_CONTAINER; }

        const std::string getFullName(const type_identity *item) const;

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx) const;
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx) const;
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index) const;

        virtual bool lua_insert2(lua_State *state, int fname_idx, void *ptr, int idx, int val_index) const;
    };

    class DFHACK_EXPORT bit_container_identity : public container_identity {
    public:
        bit_container_identity(size_t size, TAllocateFn alloc, const enum_identity *ienum = NULL)
            : container_identity(size, alloc, NULL, ienum) {};

        virtual identity_type type() const { return IDTYPE_BIT_CONTAINER; }

        const std::string getFullName(const type_identity *item) const;

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx) const;
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx) const;
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index) const;

    protected:
        virtual void *item_pointer(const type_identity *, void *, int) const { return NULL; }

        virtual bool get_item(void *ptr, int idx) const = 0;
        virtual void set_item(void *ptr, int idx, bool val) const = 0;
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

        const std::string getFullName() const { return name; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const = 0;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const = 0;

    };

    class DFHACK_EXPORT integer_identity_base : public number_identity_base {
    public:
        integer_identity_base(size_t size, const char *name)
            : number_identity_base(size, name) {}

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const;

    protected:
        virtual int64_t read(void *ptr) const = 0;
        virtual void write(void *ptr, int64_t val) const = 0;
    };

    class DFHACK_EXPORT float_identity_base : public number_identity_base {
    public:
        float_identity_base(size_t size, const char *name)
            : number_identity_base(size, name) {}

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const;

    protected:
        virtual double read(void *ptr) const = 0;
        virtual void write(void *ptr, double val) const = 0;
    };

    template<class T>
    class integer_identity : public integer_identity_base {
    public:
        integer_identity(const char *name) : integer_identity_base(sizeof(T), name) {}

    protected:
        virtual int64_t read(void *ptr) const { return int64_t(*(T*)ptr); }
        virtual void write(void *ptr, int64_t val) const { *(T*)ptr = T(val); }
    };

    template<class T>
    class float_identity : public float_identity_base {
    public:
        float_identity(const char *name) : float_identity_base(sizeof(T), name) {}

    protected:
        virtual double read(void *ptr) const { return double(*(T*)ptr); }
        virtual void write(void *ptr, double val) const { *(T*)ptr = T(val); }
    };

    class DFHACK_EXPORT bool_identity : public primitive_identity {
    public:
        bool_identity() : primitive_identity(sizeof(bool)) {};

        const std::string getFullName() const { return "bool"; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const;
    };

    class DFHACK_EXPORT ptr_string_identity : public primitive_identity {
    public:
        ptr_string_identity() : primitive_identity(sizeof(char*)) {};

        const std::string getFullName() const { return "char*"; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const;
    };

    class DFHACK_EXPORT stl_string_identity : public DFHack::constructed_identity {
    public:
        stl_string_identity()
            : constructed_identity(sizeof(std::string), &allocator_fn<std::string>)
        {};

        const std::string getFullName() const { return "string"; }

        virtual DFHack::identity_type type() const { return DFHack::IDTYPE_PRIMITIVE; }

        virtual bool isPrimitive() const { return true; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const;
    };

    class DFHACK_EXPORT stl_ptr_vector_identity : public ptr_container_identity {
    public:
        typedef std::vector<void*> container;

        /*
         * This class assumes that std::vector<T*> is equivalent
         * in layout and behavior to std::vector<void*> for any T.
         */

        stl_ptr_vector_identity(const type_identity *item = NULL, const enum_identity *ienum = NULL)
            : ptr_container_identity(sizeof(container), &df::allocator_fn<container>, item, ienum)
        {};

        const std::string getFullName(const type_identity *item) const {
            return "vector" + ptr_container_identity::getFullName(item);
        }

        virtual DFHack::identity_type type() const { return DFHack::IDTYPE_STL_PTR_VECTOR; }

        virtual bool resize(void *ptr, int size) const {
            (*(container*)ptr).resize(size);
            return true;
        }
        virtual bool erase(void *ptr, int size) const {
            auto &ct = *(container*)ptr;
            ct.erase(ct.begin()+size);
            return true;
        }
        virtual bool insert(void *ptr, int idx, void *item) const {
            auto &ct = *(container*)ptr;
            ct.insert(ct.begin()+idx, item);
            return true;
        }

    protected:
        virtual int item_count(void *ptr, CountMode) const {
            return (int)((container*)ptr)->size();
        };
        virtual void *item_pointer(const type_identity *, void *ptr, int idx) const {
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

        buffer_container_identity(int size, const type_identity *item, const enum_identity *ienum = NULL)
            : container_identity(0, NULL, item, ienum), size(size)
        {}

        size_t byte_size() const { return getItemType()->byte_size()*size; }

        const std::string getFullName(const type_identity *item) const;
        int getSize() const { return size; }

        virtual DFHack::identity_type type() const { return DFHack::IDTYPE_BUFFER; }

        static const buffer_container_identity base_instance;

    protected:
        virtual int item_count(void *ptr, CountMode) const { return size; }
        virtual void *item_pointer(const type_identity *item, void *ptr, int idx) const {
            return ((uint8_t*)ptr) + idx * item->byte_size();
        }
    };
#endif

    template<class T>
    class stl_container_identity : public container_identity {
        const char *name;

    public:
        stl_container_identity(const char *name, const type_identity *item, const enum_identity *ienum = NULL)
            : container_identity(sizeof(T), &allocator_fn<T>, item, ienum), name(name)
        {}

        const std::string getFullName(type_identity *item) const {
            return name + container_identity::getFullName(item);
        }

        virtual bool resize(void *ptr, int size) const {
            (*(T*)ptr).resize(size);
            return true;
        }
        virtual bool erase(void *ptr, int size) const {
            auto &ct = *(T*)ptr;
            ct.erase(ct.begin()+size);
            return true;
        }
        virtual bool insert(void *ptr, int idx, void *item) const {
            auto &ct = *(T*)ptr;
            ct.insert(ct.begin()+idx, *(typename T::value_type*)item);
            return true;
        }

    protected:
        virtual int item_count(void *ptr, CountMode) const { return (int)((T*)ptr)->size(); }
        virtual void *item_pointer(const type_identity *item, void *ptr, int idx) const {
            return &(*(T*)ptr)[idx];
        }
    };

#ifdef BUILD_DFHACK_LIB
    template<class T>
    class ro_stl_container_identity : public container_identity {
    protected:
        const char *name;

    public:
        ro_stl_container_identity(const char *name, const type_identity *item, const enum_identity *ienum = NULL)
            : container_identity(sizeof(T), &allocator_fn<T>, item, ienum), name(name)
        {}

        const std::string getFullName(const type_identity *item) const {
            return name + container_identity::getFullName(item);
        }

        virtual bool is_readonly() const { return true; }
        virtual bool resize(void *ptr, int size) const { return false; }
        virtual bool erase(void *ptr, int size) const { return false; }
        virtual bool insert(void *ptr, int idx, void *item) const { return false; }

    protected:
        virtual int item_count(void *ptr, CountMode) const { return (int)((T*)ptr)->size(); }
        virtual void *item_pointer(const type_identity *item, void *ptr, int idx) const {
            auto iter = (*(T*)ptr).begin();
            for (; idx > 0; idx--) ++iter;
            return (void*)&*iter;
        }
    };

    template<class T>
    class ro_stl_assoc_container_identity : public ro_stl_container_identity<T> {
        const type_identity *key_identity;
        const type_identity *item_identity;

    public:
        ro_stl_assoc_container_identity(const char *name, const type_identity *key, const type_identity *item)
            : ro_stl_container_identity<T>(name, item),
              key_identity(key),
              item_identity(item)
        {}

        virtual const std::string getFullName(const type_identity*) const override {
            return std::string(ro_stl_assoc_container_identity<T>::name) + "<" + key_identity->getFullName() + ", " + item_identity->getFullName() + ">";
        }

    protected:
        virtual void *item_pointer(const type_identity *item, void *ptr, int idx) const override {
            auto iter = (*(T*)ptr).begin();
            for (; idx > 0; idx--) ++iter;
            return (void*)&iter->second;
        }
    };

    class bit_array_identity : public bit_container_identity {
    public:
        /*
         * This class assumes that BitArray<T> is equivalent
         * in layout and behavior to BitArray<int> for any T.
         */

        typedef BitArray<int> container;

        bit_array_identity(const enum_identity *ienum = NULL)
            : bit_container_identity(sizeof(container), &allocator_fn<container>, ienum)
        {}

        const std::string getFullName(type_identity *item) const {
            return "BitArray<>";
        }

        virtual bool resize(void *ptr, int size) const {
            ((container*)ptr)->resize((size+7)/8);
            return true;
        }

    protected:
        virtual int item_count(void *ptr, CountMode cnt) const {
            return cnt == COUNT_LEN ? ((container*)ptr)->size * 8 : -1;
        }
        virtual bool get_item(void *ptr, int idx) const {
            return ((container*)ptr)->is_set(idx);
        }
        virtual void set_item(void *ptr, int idx, bool val) const {
            ((container*)ptr)->set(idx, val);
        }
    };
#endif

    class DFHACK_EXPORT stl_bit_vector_identity : public bit_container_identity {
    public:
        typedef std::vector<bool> container;

        stl_bit_vector_identity(const enum_identity *ienum = NULL)
            : bit_container_identity(sizeof(container), &df::allocator_fn<container>, ienum)
        {}

        const std::string getFullName(const type_identity *item) const {
            return "vector" + bit_container_identity::getFullName(item);
        }

        virtual bool resize(void *ptr, int size) const {
            (*(container*)ptr).resize(size);
            return true;
        }

    protected:
        virtual int item_count(void *ptr, CountMode) const {
            return (int)((container*)ptr)->size();
        }
        virtual bool get_item(void *ptr, int idx) const {
            return (*(container*)ptr)[idx];
        }
        virtual void set_item(void *ptr, int idx, bool val) const {
            (*(container*)ptr)[idx] = val;
        }
    };

#ifdef BUILD_DFHACK_LIB
    template<class T>
    class enum_list_attr_identity : public container_identity {
    public:
        typedef enum_list_attr<T> container;

        enum_list_attr_identity(const type_identity *item)
            : container_identity(sizeof(container), NULL, item, NULL)
        {}

        const std::string getFullName(const type_identity *item) const {
            return "enum_list_attr" + container_identity::getFullName(item);
        }

    protected:
        virtual int item_count(void *ptr, CountMode cm) const {
            return cm == COUNT_WRITE ? 0 : (int)((container*)ptr)->size;
        }
        virtual void *item_pointer(const type_identity *item, void *ptr, int idx) const {
            return (void*)&((container*)ptr)->items[idx];
        }
    };
#endif

#define NUMBER_IDENTITY_TRAITS(category, type) \
    template<> struct DFHACK_EXPORT identity_traits<type> { \
        static const bool is_primitive = true; \
        static const category##_identity<type> identity; \
        static const category##_identity_base *get() { return &identity; } \
    };

#define INTEGER_IDENTITY_TRAITS(type) NUMBER_IDENTITY_TRAITS(integer, type)
#define FLOAT_IDENTITY_TRAITS(type) NUMBER_IDENTITY_TRAITS(float, type)

// the space after the use of "type" in OPAQUE_IDENTITY_TRAITS is _required_
// without it the macro generates a syntax error when type is a template specification

#define OPAQUE_IDENTITY_TRAITS(...) \
    template<> struct DFHACK_EXPORT identity_traits<__VA_ARGS__ > { \
        static const opaque_identity identity; \
        static const opaque_identity *get() { return &identity; } \
    };

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
    OPAQUE_IDENTITY_TRAITS(std::condition_variable);
    OPAQUE_IDENTITY_TRAITS(std::fstream);
    OPAQUE_IDENTITY_TRAITS(std::mutex);
    OPAQUE_IDENTITY_TRAITS(std::future<void>);
    OPAQUE_IDENTITY_TRAITS(std::function<void()>);
    OPAQUE_IDENTITY_TRAITS(std::function<bool()>);
    OPAQUE_IDENTITY_TRAITS(std::function<int*()>);
    OPAQUE_IDENTITY_TRAITS(std::function<std::string()>);
    OPAQUE_IDENTITY_TRAITS(std::optional<std::function<void()> >);
    OPAQUE_IDENTITY_TRAITS(std::variant<std::string, std::function<void()> >);
    OPAQUE_IDENTITY_TRAITS(std::weak_ptr<df::widget_container>);

#ifdef BUILD_DFHACK_LIB
    template<typename T>
    struct DFHACK_EXPORT identity_traits<std::shared_ptr<T>> {
        static opaque_identity *get() {
            typedef std::shared_ptr<T> type;
            static std::string name = std::string("shared_ptr<") + typeid(T).name() + ">";
            static opaque_identity identity(sizeof(type), allocator_noassign_fn<type>, name);
            return &identity;
        }
    };
#endif

    template<> struct DFHACK_EXPORT identity_traits<bool> {
        static const bool is_primitive = true;
        static const bool_identity identity;
        static const bool_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::string> {
        static const bool is_primitive = true;
        static const stl_string_identity identity;
        static const stl_string_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<char*> {
        static const bool is_primitive = true;
        static const ptr_string_identity identity;
        static const ptr_string_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<const char*> {
        static const bool is_primitive = true;
        static const ptr_string_identity identity;
        static const ptr_string_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<void*> {
        static const bool is_primitive = true;
        static const pointer_identity identity;
        static const pointer_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::vector<void*> > {
        static const stl_ptr_vector_identity identity;
        static const stl_ptr_vector_identity *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::vector<bool> > {
        static const stl_bit_vector_identity identity;
        static const stl_bit_vector_identity *get() { return &identity; }
    };

#undef NUMBER_IDENTITY_TRAITS
#undef INTEGER_IDENTITY_TRAITS
#undef FLOAT_IDENTITY_TRAITS
#undef OPAQUE_IDENTITY_TRAITS

    // Container declarations

#ifdef BUILD_DFHACK_LIB
    template<class Enum, class FT> struct identity_traits<enum_field<Enum,FT> > {
        static const enum_identity *get();
    };
#endif

    template<class T> struct identity_traits<T *> {
        static const bool is_primitive = true;
        static const pointer_identity *get();
    };

#ifdef BUILD_DFHACK_LIB
    template<class T, int sz> struct identity_traits<T [sz]> {
        static const container_identity *get();
    };

    template<class T> struct identity_traits<std::vector<T> > {
        static const container_identity *get();
    };
#endif

    template<class T> struct identity_traits<std::vector<T*> > {
        static const stl_ptr_vector_identity *get();
    };

    // explicit specializations for these two types
    // for availability in plugins

    template<> struct identity_traits<std::vector<int32_t> > {
        static const container_identity* get();
    };

    template<> struct identity_traits<std::vector<int16_t> > {
        static const container_identity* get();
    };


#ifdef BUILD_DFHACK_LIB
    template<class T> struct identity_traits<std::deque<T> > {
        static const container_identity *get();
    };

    template<class T> struct identity_traits<std::set<T> > {
        static const container_identity *get();
    };

    template<class KT, class T> struct identity_traits<std::map<KT, T>> {
        static const container_identity *get();
    };

    template<class KT, class T> struct identity_traits<std::unordered_map<KT, T>> {
        static const container_identity *get();
    };

    template<> struct identity_traits<BitArray<int> > {
        static const bit_array_identity identity;
        static const bit_container_identity *get() { return &identity; }
    };

    template<class T> struct identity_traits<BitArray<T> > {
        static const bit_container_identity *get();
    };

    template<class T> struct identity_traits<DfArray<T> > {
        static const container_identity *get();
    };

    template<class T> struct identity_traits<enum_list_attr<T> > {
        static const container_identity *get();
    };
#endif

    // Container definitions

#ifdef BUILD_DFHACK_LIB
    template<class Enum, class FT>
    inline const enum_identity *identity_traits<enum_field<Enum,FT> >::get() {
        static const enum_identity identity(identity_traits<Enum>::get(), identity_traits<FT>::get());
        return &identity;
    }
#endif

    template<class T>
    inline const pointer_identity *identity_traits<T *>::get() {
        static const pointer_identity identity(identity_traits<T>::get());
        return &identity;
    }

#ifdef BUILD_DFHACK_LIB
    template<class T, int sz>
    inline const container_identity *identity_traits<T [sz]>::get() {
        static const buffer_container_identity identity(sz, identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const container_identity *identity_traits<std::vector<T> >::get() {
        typedef std::vector<T> container;
        static const stl_container_identity<container> identity("vector", identity_traits<T>::get());
        return &identity;
    }
#endif

    template<class T>
    inline const stl_ptr_vector_identity *identity_traits<std::vector<T*> >::get() {
        static const stl_ptr_vector_identity identity(identity_traits<T>::get());
        return &identity;
    }

    // explicit specializations for these two types
    // for availability in plugins

    extern const DFHACK_EXPORT stl_container_identity<std::vector<int32_t> > stl_vector_int32_t_identity;
    inline const container_identity* identity_traits<std::vector<int32_t> >::get() {
        return &stl_vector_int32_t_identity;
    }

    extern const DFHACK_EXPORT stl_container_identity<std::vector<int16_t> > stl_vector_int16_t_identity;
    inline const container_identity* identity_traits<std::vector<int16_t> >::get() {
        return &stl_vector_int16_t_identity;
    }

#ifdef BUILD_DFHACK_LIB
    template<class T>
    inline const container_identity *identity_traits<std::deque<T> >::get() {
        typedef std::deque<T> container;
        static const stl_container_identity<container> identity("deque", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const container_identity *identity_traits<std::set<T> >::get() {
        typedef std::set<T> container;
        static const ro_stl_container_identity<container> identity("set", identity_traits<T>::get());
        return &identity;
    }

    template<class KT, class T>
    inline const container_identity *identity_traits<std::map<KT, T>>::get() {
        typedef std::map<KT, T> container;
        static const ro_stl_assoc_container_identity<container> identity("map", identity_traits<KT>::get(), identity_traits<T>::get());
        return &identity;
    }

    template<class KT, class T>
    inline const container_identity *identity_traits<std::unordered_map<KT, T>>::get() {
        typedef std::unordered_map<KT, T> container;
        static const ro_stl_assoc_container_identity<container> identity("unordered_map", identity_traits<KT>::get(), identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const bit_container_identity *identity_traits<BitArray<T> >::get() {
        static const bit_array_identity identity(identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const container_identity *identity_traits<DfArray<T> >::get() {
        typedef DfArray<T> container;
        static const stl_container_identity<container> identity("DfArray", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const container_identity *identity_traits<enum_list_attr<T> >::get() {
        static const enum_list_attr_identity<T> identity(identity_traits<T>::get());
        return &identity;
    }
#endif
}
