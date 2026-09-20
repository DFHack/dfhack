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

#include <array>
#include <deque>
#include <future>
#include <map>
#include <optional>
#include <string>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <filesystem>

#include "DataDefs.h"
#include "LuaWrapper.h"

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

        virtual identity_type type() const override { return IDTYPE_FUNCTION; }

        int getNumArgs() const { return num_args; }
        bool adjustArgs() const { return vararg; }

        const std::string getFullName() const override { return "function"; }

        virtual void invoke(lua_State *state, int base) const = 0;

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const override;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const override;
    };

    class DFHACK_EXPORT primitive_identity_base : public type_identity {
    protected:
        primitive_identity_base(size_t size) : type_identity(size) {};

    public:
        virtual identity_type type() const override { return IDTYPE_PRIMITIVE; }
    };

    class DFHACK_EXPORT opaque_identity : public constructed_identity {
        const std::string name;

    public:
        opaque_identity(size_t size, TAllocateFn alloc, const std::string &name)
          : constructed_identity(size, alloc), name(name) {};

        virtual const std::string getFullName() const override { return name; }
        virtual identity_type type() const override { return IDTYPE_OPAQUE; }
    };

    class DFHACK_EXPORT pointer_identity_base : public primitive_identity_base {
        const type_identity *target;

    public:
        pointer_identity_base(const type_identity *target = NULL)
            : primitive_identity_base(sizeof(void*)), target(target) {};

        virtual identity_type type() const override { return IDTYPE_POINTER; }

        const type_identity *getTarget() const { return target; }

        const std::string getFullName() const override;

        static void lua_read(lua_State *state, int fname_idx, void *ptr, const type_identity *target);
        static void lua_write(lua_State *state, int fname_idx, void *ptr, const type_identity *target, int val_index);

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const override;
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const override;
    };

    class DFHACK_EXPORT container_identity : public constructed_identity {
    protected:
        const type_identity *item;
        const enum_identity *ienum;

    public:
        container_identity(size_t size, const TAllocateFn alloc, const type_identity *item, const enum_identity *ienum = NULL)
            : constructed_identity(size, alloc), item(item), ienum(ienum) {};

        virtual identity_type type() const override { return IDTYPE_CONTAINER; }

        const std::string getFullName() const override { return getFullName(item); }

        virtual void build_metatable(lua_State *state) const override;
        virtual bool isContainer() const override { return true; }

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

        virtual identity_type type() const override { return IDTYPE_PTR_CONTAINER; }

        const std::string getFullName(const type_identity *item) const override;

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx) const override;
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx) const override;
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index) const override;

        virtual bool lua_insert2(lua_State *state, int fname_idx, void *ptr, int idx, int val_index) const override;
    };

    class DFHACK_EXPORT bit_container_identity : public container_identity {
    public:
        bit_container_identity(size_t size, TAllocateFn alloc, const enum_identity *ienum = NULL)
            : container_identity(size, alloc, NULL, ienum) {};

        virtual identity_type type() const override { return IDTYPE_BIT_CONTAINER; }

        const std::string getFullName(const type_identity *item) const override;

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx) const override;
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx) const override;
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index) const override;

    protected:
        virtual void *item_pointer(const type_identity *, void *, int) const override { return NULL; }

        virtual bool get_item(void *ptr, int idx) const = 0;
        virtual void set_item(void *ptr, int idx, bool val) const = 0;
    };
}

namespace df
{
    using DFHack::function_identity_base;
    using DFHack::primitive_identity_base;
    using DFHack::opaque_identity;
    using DFHack::pointer_identity_base;
    using DFHack::container_identity;
    using DFHack::ptr_container_identity;
    using DFHack::bit_container_identity;

    class DFHACK_EXPORT number_identity_base : public primitive_identity_base {
        const char *name;

    public:
        number_identity_base(size_t size, const char *name)
            : primitive_identity_base(size), name(name) {};

        const std::string getFullName() const override { return name; }

        virtual bool isInteger() const { return false; }
    };

    /*
     * Identity for statically-sized arrays (C arrays and std::array).
     * Unlike other container identities the item count is fixed,
     * so instances can also serve as the identity of ad-hoc buffers.
     */
    class buffer_container_identity : public container_identity {
        int size;

    public:
        buffer_container_identity()
            : container_identity(0, NULL, NULL, NULL), size(0)
        {}

        buffer_container_identity(int size, const type_identity *item, const enum_identity *ienum = NULL)
            : container_identity(0, NULL, item, ienum), size(size)
        {}

        size_t byte_size() const override { return getItemType()->byte_size()*size; }

        using container_identity::getFullName;
        const std::string getFullName(const type_identity *item) const override;
        int getSize() const { return size; }

        virtual DFHack::identity_type type() const override { return DFHack::IDTYPE_BUFFER; }

        static const buffer_container_identity base_instance;

    protected:
        virtual int item_count(void *ptr, CountMode) const override { return size; }
        virtual void *item_pointer(const type_identity *item, void *ptr, int idx) const override {
            return ((uint8_t*)ptr) + idx * item->byte_size();
        }
    };
}

namespace DFHack
{
    namespace detail
    {
        /*
         * Compile-time type inspection used by type_identity_for<T> to pick
         * the appropriate behavior for the wrapped C++ type.
         */

        template<typename T> struct is_std_vector : std::false_type {};
        template<typename E, typename A> struct is_std_vector<std::vector<E, A>> : std::true_type {};

        template<typename T> struct is_std_deque : std::false_type {};
        template<typename E, typename A> struct is_std_deque<std::deque<E, A>> : std::true_type {};

        template<typename T> struct is_std_array : std::false_type {};
        template<typename E, size_t N> struct is_std_array<std::array<E, N>> : std::true_type {};

        template<typename T> struct is_std_set : std::false_type {};
        template<typename E, typename... Rest> struct is_std_set<std::set<E, Rest...>> : std::true_type {};
        template<typename E, typename... Rest> struct is_std_set<std::unordered_set<E, Rest...>> : std::true_type {};

        template<typename T> struct is_std_map : std::false_type {};
        template<typename K, typename V, typename... Rest> struct is_std_map<std::map<K, V, Rest...>> : std::true_type {};
        template<typename K, typename V, typename... Rest> struct is_std_map<std::unordered_map<K, V, Rest...>> : std::true_type {};

        template<typename T> struct is_bit_array : std::false_type {};
        template<typename E> struct is_bit_array<BitArray<E>> : std::true_type {};

        template<typename T> struct is_df_array : std::false_type {};
        template<typename E> struct is_df_array<DfArray<E>> : std::true_type {};

        template<typename T> struct is_enum_list_attr : std::false_type {};
        template<typename E> struct is_enum_list_attr<enum_list_attr<E>> : std::true_type {};

        template<typename T> struct is_enum_field : std::false_type {};
        template<typename E, typename I> struct is_enum_field<df::enum_field<E, I>> : std::true_type {};

        // C strings are represented by char* and const char*
        template<typename T>
        concept c_string = std::is_same_v<T, char*> || std::is_same_v<T, const char*>;

        // a std::vector of pointers; stored internally as std::vector<void*>
        template<typename T>
        concept ptr_vector = is_std_vector<T>::value && std::is_pointer_v<typename T::value_type>;

        // std::vector<bool> and BitArray<T> are bit containers
        template<typename T>
        concept bit_container = is_bit_array<T>::value ||
            (is_std_vector<T>::value && std::is_same_v<typename T::value_type, bool>);

        // sequence containers with mutable insert/erase/resize
        template<typename T>
        concept seq_container =
            (is_std_vector<T>::value && !std::is_same_v<typename T::value_type, bool> &&
                !std::is_pointer_v<typename T::value_type>) ||
            is_std_deque<T>::value || is_df_array<T>::value;

        // read-only containers without random access (associative types)
        template<typename T>
        concept assoc_container = is_std_set<T>::value || is_std_map<T>::value;

        template<typename T>
        concept any_container = seq_container<T> || assoc_container<T> ||
            bit_container<T> || ptr_vector<T> || is_enum_list_attr<T>::value;

        template<typename T>
        concept stl_string = std::is_same_v<T, std::string>;

        template<typename T>
        concept fs_path = std::is_same_v<T, std::filesystem::path>;

        // df::bitfield_traits<T> exists only for generated bitfield types
        template<typename T>
        concept df_bitfield = requires { typename df::bitfield_traits<T>::base_type; };

        // generated compound types (struct/union/class) carry a static
        // _identity member
        template<typename T>
        concept has_identity_member = requires { T::_identity; };

        // DfOtherVectors marks its descendants with a nested typedef
        template<typename T>
        concept other_vectors = requires { typename T::dfhack_other_vectors; };

        // an explicit df_identity_base typedef always wins; this lets the
        // code generator (and hand-written types) pick the base directly
        template<typename T>
        concept has_explicit_base = requires { typename T::df_identity_base; };

        // The underlying storage actually manipulated by a container identity.
        // std::vector<T*> uses std::vector<void*>, and BitArray<T> uses
        // BitArray<int>, matching the assumptions of the original code.
        template<typename T> struct container_storage { using type = T; };
        template<typename E, typename A> struct container_storage<std::vector<E*, A>> { using type = std::vector<void*>; };
        template<typename E> struct container_storage<BitArray<E>> { using type = BitArray<int>; };
        template<typename T> using container_storage_t = typename container_storage<T>::type;

        template<typename B> struct base_tag { using type = B; };

        /*
         * The intermediate base for container-like T; implements the
         * container_identity virtuals using constexpr dispatch on T.
         */
        template<typename T>
        class container_impl : public std::conditional_t<
                bit_container<T>, bit_container_identity,
                std::conditional_t<ptr_vector<T>, ptr_container_identity, container_identity>> {
            using cbase = std::conditional_t<
                bit_container<T>, bit_container_identity,
                std::conditional_t<ptr_vector<T>, ptr_container_identity, container_identity>>;
            using storage = container_storage_t<T>;

            const char *name;
            const type_identity *key_id;

            // enum_list_attr is a static attribute table and has no allocator
            static constexpr TAllocateFn alloc_fn() {
                if constexpr (is_enum_list_attr<T>::value)
                    return NULL;
                else
                    return &df::allocator_fn<storage>;
            }

        public:
            // sequence and read-only containers
            container_impl(const char *name, const type_identity *item, const enum_identity *ienum = NULL)
                requires (!bit_container<T> && !is_std_map<T>::value)
                : cbase(sizeof(storage), alloc_fn(), item, ienum), name(name), key_id(NULL)
            {}

            // bit containers take an index enum instead of an item identity
            container_impl(const char *name, const enum_identity *ienum)
                requires bit_container<T>
                : cbase(sizeof(storage), alloc_fn(), ienum), name(name), key_id(NULL)
            {}

            // associative containers additionally take a key identity
            container_impl(const char *name, const type_identity *key, const type_identity *item)
                requires is_std_map<T>::value
                : cbase(sizeof(storage), alloc_fn(), item, NULL), name(name), key_id(key)
            {}

            const std::string getFullName() const override { return getFullName(this->item); }

            const std::string getFullName(const type_identity *item) const override {
                if constexpr (is_std_map<T>::value)
                    return std::string(name) + "<" + key_id->getFullName() + ", " + item->getFullName() + ">";
                else if constexpr (is_bit_array<T>::value)
                    return "BitArray<>";
                else
                    return std::string(name) + cbase::getFullName(item);
            }

            virtual bool is_readonly() const override {
                return assoc_container<T>;
            }

            virtual bool resize(void *ptr, int size) const override {
                if constexpr (is_bit_array<T>::value) {
                    ((storage*)ptr)->resize((size+7)/8);
                    return true;
                }
                else if constexpr (std::is_same_v<T, std::vector<bool>> ||
                        seq_container<T> || ptr_vector<T>) {
                    (*(storage*)ptr).resize(size);
                    return true;
                }
                else
                    return false;
            }
            virtual bool erase(void *ptr, int index) const override {
                if constexpr (seq_container<T> || ptr_vector<T>) {
                    auto &ct = *(storage*)ptr;
                    ct.erase(ct.begin()+index);
                    return true;
                }
                else
                    return false;
            }
            virtual bool insert(void *ptr, int index, void *pitem) const override {
                if constexpr (ptr_vector<T>) {
                    auto &ct = *(storage*)ptr;
                    ct.insert(ct.begin()+index, pitem);
                    return true;
                }
                else if constexpr (seq_container<T>) {
                    auto &ct = *(storage*)ptr;
                    ct.insert(ct.begin()+index, *(typename T::value_type*)pitem);
                    return true;
                }
                else
                    return false;
            }
            virtual bool lua_insert2(lua_State* state, int fname_idx, void* ptr, int idx, int val_index) const override {
                if constexpr (seq_container<T>) {
                    using VT = typename T::value_type;
                    VT tmp{};
                    auto id = (type_identity*)lua_touserdata(state, DFHack::LuaWrapper::UPVAL_ITEM_ID);
                    auto pitem = DFHack::LuaWrapper::get_object_internal(state, id, val_index, false);
                    bool useTemporary = (!pitem && id->isPrimitive());

                    if (useTemporary)
                    {
                        pitem = &tmp;
                        id->lua_write(state, fname_idx, pitem, val_index);
                    }

                    if (id != this->item || !pitem)
                        DFHack::LuaWrapper::field_error(state, fname_idx, "incompatible object type", "insert");

                    return insert(ptr, idx, pitem);
                }
                else
                    return cbase::lua_insert2(state, fname_idx, ptr, idx, val_index);
            }

        protected:
            virtual int item_count(void *ptr, container_identity::CountMode cnt) const override {
                if constexpr (is_bit_array<T>::value)
                    return cnt == container_identity::COUNT_LEN ? (int)((storage*)ptr)->size() * 8 : -1;
                else if constexpr (is_enum_list_attr<T>::value)
                    return cnt == container_identity::COUNT_WRITE ? 0 : (int)((storage*)ptr)->size;
                else
                    return (int)((storage*)ptr)->size();
            }
            virtual void *item_pointer(const type_identity *item, void *ptr, int idx) const override {
                if constexpr (bit_container<T>)
                    return NULL;
                else if constexpr (is_enum_list_attr<T>::value)
                    return (void*)&((storage*)ptr)->items[idx];
                else if constexpr (is_std_map<T>::value) {
                    auto iter = (*(storage*)ptr).begin();
                    for (; idx > 0; idx--) ++iter;
                    return (void*)&iter->second;
                }
                else if constexpr (assoc_container<T>) {
                    auto iter = (*(storage*)ptr).begin();
                    for (; idx > 0; idx--) ++iter;
                    return (void*)&*iter;
                }
                else
                    return &(*(storage*)ptr)[idx];
            }
            // get_item/set_item only exist in bit_container_identity; for
            // other bases these are ordinary (unused) member functions, and
            // they implicitly override the virtuals for bit containers.
            bool get_item(void *ptr, int idx) const {
                if constexpr (is_bit_array<T>::value)
                    return ((storage*)ptr)->is_set(idx);
                else if constexpr (bit_container<T>)
                    return (*(storage*)ptr)[idx];
                else
                    return false;
            }
            void set_item(void *ptr, int idx, bool val) const {
                if constexpr (is_bit_array<T>::value)
                    ((storage*)ptr)->set(idx, val);
                else if constexpr (bit_container<T>)
                    (*(storage*)ptr)[idx] = val;
            }
        };

        template<typename T>
        constexpr auto select_identity_base() {
            if constexpr (has_explicit_base<T>)
                return base_tag<typename T::df_identity_base>{};
            else if constexpr (std::is_enum_v<T> || is_enum_field<T>::value)
                return base_tag<enum_identity>{};
            else if constexpr (df_bitfield<T>)
                return base_tag<bitfield_identity>{};
            else if constexpr (has_identity_member<T>) {
                if constexpr (std::is_union_v<T>)
                    return base_tag<union_identity>{};
                else if constexpr (other_vectors<T>)
                    return base_tag<other_vectors_identity>{};
                else if constexpr (std::is_polymorphic_v<T>)
                    return base_tag<virtual_identity>{};
                else
                    return base_tag<struct_identity>{};
            }
            else if constexpr (c_string<T>)
                return base_tag<primitive_identity_base>{};
            else if constexpr (std::is_same_v<T, wchar_t*>)
                return base_tag<opaque_identity>{};
            else if constexpr (std::is_pointer_v<T>)
                return base_tag<pointer_identity_base>{};
            else if constexpr (std::is_arithmetic_v<T>)
                return base_tag<df::number_identity_base>{};
            else if constexpr (stl_string<T> || fs_path<T>)
                return base_tag<constructed_identity>{};
            else if constexpr (std::is_array_v<T> || is_std_array<T>::value)
                return base_tag<df::buffer_container_identity>{};
            else if constexpr (any_container<T>)
                return base_tag<container_impl<T>>{};
            else if constexpr (std::is_class_v<T>)
                return base_tag<opaque_identity>{};
            else
                static_assert(!sizeof(T*), "type_identity_for: no identity category for this type");
        }
    }

    template<typename T>
    using identity_base_of_t = typename decltype(detail::select_identity_base<T>())::type;

    DFHACK_EXPORT void build_global_metatable(lua_State *state, const struct_identity *id);
    DFHACK_EXPORT void lua_read_path(lua_State *state, void *ptr);
    DFHACK_EXPORT void lua_write_path(lua_State *state, int fname_idx, void *ptr, int val_index);

    /*
     * The identity of the C++ type T. The base class is selected by
     * compile-time inspection of T (or an explicit df_identity_base
     * typedef in T), and the remaining behavior is implemented here
     * with constexpr dispatch on the properties of T.
     */
    template<typename T>
    class type_identity_for : public identity_base_of_t<T> {
        using base = identity_base_of_t<T>;

    public:
        // forwards everything to the base class constructor; covers all
        // compound identities, arrays, and opaque types
        template<typename... Args>
            requires (sizeof...(Args) > 0 && std::is_constructible_v<base, Args...>)
        explicit type_identity_for(Args&&... args) : base(std::forward<Args>(args)...) {}

        // numbers take only a name; the size comes from T
        explicit type_identity_for(const char *name)
            requires std::is_same_v<base, df::number_identity_base>
            : base(sizeof(T), name) {}

        // C strings
        type_identity_for()
            requires detail::c_string<T>
            : base(sizeof(T)) {}

        // std::string and std::filesystem::path
        type_identity_for()
            requires (detail::stl_string<T> || detail::fs_path<T>)
            : base(sizeof(T), &df::allocator_fn<T>) {}

        // pointers derive the target from identity_traits, or take an
        // explicit target (used for void*)
        type_identity_for()
            requires (std::is_pointer_v<T> && std::is_same_v<base, pointer_identity_base>)
            : base(df::identity_traits<std::remove_pointer_t<T>>::get()) {}

        // named sequence/set containers
        type_identity_for(const char *name, const type_identity *item, const enum_identity *ienum = NULL)
            requires (std::is_same_v<base, detail::container_impl<T>> &&
                      !detail::bit_container<T> && !detail::is_std_map<T>::value)
            : base(name, item, ienum) {}

        // named associative containers additionally take a key identity
        type_identity_for(const char *name, const type_identity *key, const type_identity *item)
            requires detail::is_std_map<T>::value
            : base(name, key, item) {}

        // std::vector<T*>
        type_identity_for(const type_identity *item = NULL, const enum_identity *ienum = NULL)
            requires detail::ptr_vector<T>
            : base("vector", item, ienum) {}

        // std::vector<bool> and BitArray<T>
        type_identity_for(const enum_identity *ienum = NULL)
            requires detail::bit_container<T>
            : base("vector", ienum) {}

        // the global object wraps no actual type
        explicit type_identity_for(const struct_field_info *fields)
            requires std::is_same_v<T, global_object>
            : base(0, NULL, NULL, "global", NULL, fields) {}

        virtual identity_type type() const override {
            if constexpr (detail::stl_string<T> || detail::fs_path<T>)
                return IDTYPE_PRIMITIVE;
            else if constexpr (detail::ptr_vector<T>)
                return IDTYPE_STL_PTR_VECTOR;
            else if constexpr (std::is_same_v<T, global_object>)
                return IDTYPE_GLOBAL;
            else
                return base::type();
        }

        virtual const std::string getFullName() const override {
            if constexpr (detail::c_string<T>)
                return "char*";
            else if constexpr (detail::stl_string<T>)
                return "string";
            else if constexpr (detail::fs_path<T>)
                return "path";
            else
                return base::getFullName();
        }

        virtual bool isPrimitive() const override {
            if constexpr (detail::stl_string<T> || detail::fs_path<T>)
                return true;
            else
                return base::isPrimitive();
        }

        // isInteger only exists in number_identity_base; for other bases
        // this is an ordinary member function, and it implicitly overrides
        // the virtual for numbers.
        bool isInteger() const {
            return std::is_integral_v<T> && !std::is_same_v<T, bool>;
        }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr) const override {
            if constexpr (std::is_same_v<T, bool>)
                lua_pushboolean(state, *(T*)ptr);
            else if constexpr (std::is_floating_point_v<T>)
                lua_pushnumber(state, double(*(T*)ptr));
            else if constexpr (std::is_integral_v<T>)
                lua_pushinteger(state, int64_t(*(T*)ptr));
            else if constexpr (detail::c_string<T>) {
                auto pstr = *(T*)ptr;
                if (pstr)
                    lua_pushstring(state, pstr);
                else
                    lua_pushnil(state);
            }
            else if constexpr (detail::stl_string<T>) {
                auto pstr = (T*)ptr;
                lua_pushlstring(state, pstr->data(), pstr->size());
            }
            else if constexpr (detail::fs_path<T>)
                DFHack::lua_read_path(state, ptr);
            else
                base::lua_read(state, fname_idx, ptr);
        }

        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index) const override {
            if constexpr (std::is_same_v<T, bool>) {
                char *pb = (char*)ptr;

                if (lua_isboolean(state, val_index) || lua_isnil(state, val_index))
                    *pb = lua_toboolean(state, val_index);
                else if (lua_isnumber(state, val_index))
                    *pb = lua_tointeger(state, val_index);
                else
                    DFHack::LuaWrapper::field_error(state, fname_idx, "boolean or number expected", "write");
            }
            else if constexpr (std::is_floating_point_v<T>) {
                if (!lua_isnumber(state, val_index))
                    DFHack::LuaWrapper::field_error(state, fname_idx, "number expected", "write");

                *(T*)ptr = T(lua_tonumber(state, val_index));
            }
            else if constexpr (std::is_integral_v<T>) {
                int is_num = 0;
                auto value = lua_tointegerx(state, val_index, &is_num);
                if (!is_num)
                    DFHack::LuaWrapper::field_error(state, fname_idx, "integer expected", "write");
                *(T*)ptr = T(value);
            }
            else if constexpr (detail::c_string<T>)
                DFHack::LuaWrapper::field_error(state, fname_idx, "raw pointer string", "write");
            else if constexpr (detail::stl_string<T>) {
                size_t size;
                const char *bytes = lua_tolstring(state, val_index, &size);
                if (!bytes)
                    DFHack::LuaWrapper::field_error(state, fname_idx, "string expected", "write");

                *(T*)ptr = std::string(bytes, size);
            }
            else if constexpr (detail::fs_path<T>)
                DFHack::lua_write_path(state, fname_idx, ptr, val_index);
            else
                base::lua_write(state, fname_idx, ptr, val_index);
        }

        virtual void build_metatable(lua_State *state) const override {
            if constexpr (std::is_same_v<T, global_object>)
                DFHack::build_global_metatable(state, this);
            else
                base::build_metatable(state);
        }
    };
}

namespace df
{
    using DFHack::type_identity_for;

#define NUMBER_IDENTITY_TRAITS(type) \
    template<> struct DFHACK_EXPORT identity_traits<type> { \
        static const bool is_primitive = true; \
        static const type_identity_for<type> identity; \
        static const type_identity_for<type> *get() { return &identity; } \
    };

// the space after the use of "type" in OPAQUE_IDENTITY_TRAITS is _required_
// without it the macro generates a syntax error when type is a template specification

#define OPAQUE_IDENTITY_TRAITS(...) \
    template<> struct DFHACK_EXPORT identity_traits<__VA_ARGS__ > { \
        static const type_identity_for<__VA_ARGS__ > identity; \
        static const type_identity_for<__VA_ARGS__ > *get() { return &identity; } \
    };

    NUMBER_IDENTITY_TRAITS(char);
    NUMBER_IDENTITY_TRAITS(signed char);
    NUMBER_IDENTITY_TRAITS(unsigned char);
    NUMBER_IDENTITY_TRAITS(short);
    NUMBER_IDENTITY_TRAITS(unsigned short);
    NUMBER_IDENTITY_TRAITS(int);
    NUMBER_IDENTITY_TRAITS(unsigned int);
    NUMBER_IDENTITY_TRAITS(long);
    NUMBER_IDENTITY_TRAITS(unsigned long);
    NUMBER_IDENTITY_TRAITS(long long);
    NUMBER_IDENTITY_TRAITS(unsigned long long);
    NUMBER_IDENTITY_TRAITS(wchar_t);
    NUMBER_IDENTITY_TRAITS(float);
    NUMBER_IDENTITY_TRAITS(double);
    NUMBER_IDENTITY_TRAITS(bool);
    OPAQUE_IDENTITY_TRAITS(wchar_t*);
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
    OPAQUE_IDENTITY_TRAITS(std::filesystem::file_time_type);

#ifdef BUILD_DFHACK_LIB
    template<typename T>
    struct DFHACK_EXPORT identity_traits<std::shared_ptr<T>> {
        static opaque_identity *get() {
            using type = std::shared_ptr<T>;
            static std::string name = std::string("shared_ptr<") + typeid(T).name() + ">";
            static opaque_identity identity(sizeof(type), allocator_fn<type>, name);
            return &identity;
        }
    };
#endif

    template<> struct DFHACK_EXPORT identity_traits<std::string> {
        static const bool is_primitive = true;
        static const type_identity_for<std::string> identity;
        static const type_identity_for<std::string> *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::filesystem::path> {
        static const bool is_primitive = true;
        static const type_identity_for<std::filesystem::path> identity;
        static const type_identity_for<std::filesystem::path>* get() { return &identity; }
    };
    template<> struct DFHACK_EXPORT identity_traits<char*> {
        static const bool is_primitive = true;
        static const type_identity_for<char*> identity;
        static const type_identity_for<char*> *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<const char*> {
        static const bool is_primitive = true;
        static const type_identity_for<const char*> identity;
        static const type_identity_for<const char*> *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<void*> {
        static const bool is_primitive = true;
        static const type_identity_for<void*> identity;
        static const type_identity_for<void*> *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::vector<void*> > {
        static const type_identity_for<std::vector<void*> > identity;
        static const type_identity_for<std::vector<void*> > *get() { return &identity; }
    };

    template<> struct DFHACK_EXPORT identity_traits<std::vector<bool> > {
        static const type_identity_for<std::vector<bool> > identity;
        static const type_identity_for<std::vector<bool> > *get() { return &identity; }
    };

#undef NUMBER_IDENTITY_TRAITS
#undef OPAQUE_IDENTITY_TRAITS

    // Container declarations

#ifdef BUILD_DFHACK_LIB
    template<class Enum, class FT> struct identity_traits<enum_field<Enum,FT> > {
        static const enum_identity *get();
    };
#endif

    template<class T> struct identity_traits<T *> {
        static const bool is_primitive = true;
        static const pointer_identity_base *get();
    };

#ifdef BUILD_DFHACK_LIB
    template<class T, int sz> struct identity_traits<T [sz]> {
        static const container_identity *get();
    };

    template<class T, size_t sz> struct identity_traits<std::array<T, sz>>
    {
        static const container_identity* get();
    };

    template<class T> struct identity_traits<std::vector<T> > {
        static const container_identity *get();
    };
#endif

    template<class T> struct identity_traits<std::vector<T*> > {
        static const ptr_container_identity *get();
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

    template<class T> struct identity_traits<std::unordered_set<T> >
    {
        static const container_identity* get();
    };

    template<> struct identity_traits<BitArray<int> > {
        static const type_identity_for<BitArray<int> > identity;
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
        static const type_identity_for<enum_field<Enum,FT> > identity(identity_traits<Enum>::get(), identity_traits<FT>::get());
        return &identity;
    }
#endif

    template<class T>
    inline const pointer_identity_base *identity_traits<T *>::get() {
        static const type_identity_for<T*> identity;
        return &identity;
    }

#ifdef BUILD_DFHACK_LIB
    template<class T, int sz>
    inline const container_identity *identity_traits<T [sz]>::get() {
        static const type_identity_for<T[sz]> identity(sz, identity_traits<T>::get());
        return &identity;
    }

    template<class T, size_t sz>
    inline const container_identity* identity_traits<std::array<T,sz>>::get()
    {
        static const type_identity_for<std::array<T,sz> > identity(int(sz), df::identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const container_identity *identity_traits<std::vector<T> >::get() {
        static const type_identity_for<std::vector<T> > identity("vector", identity_traits<T>::get());
        return &identity;
    }
#endif

    template<class T>
    inline const ptr_container_identity *identity_traits<std::vector<T*> >::get() {
        static const type_identity_for<std::vector<T*> > identity(identity_traits<T>::get());
        return &identity;
    }

    // explicit specializations for these two types
    // for availability in plugins

    extern const DFHACK_EXPORT type_identity_for<std::vector<int32_t> > stl_vector_int32_t_identity;
    inline const container_identity* identity_traits<std::vector<int32_t> >::get() {
        return &stl_vector_int32_t_identity;
    }

    extern const DFHACK_EXPORT type_identity_for<std::vector<int16_t> > stl_vector_int16_t_identity;
    inline const container_identity* identity_traits<std::vector<int16_t> >::get() {
        return &stl_vector_int16_t_identity;
    }

#ifdef BUILD_DFHACK_LIB
    template<class T>
    inline const container_identity *identity_traits<std::deque<T> >::get() {
        static const type_identity_for<std::deque<T> > identity("deque", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const container_identity *identity_traits<std::set<T> >::get() {
        static const type_identity_for<std::set<T> > identity("set", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const container_identity* identity_traits<std::unordered_set<T> >::get()
    {
        static const type_identity_for<std::unordered_set<T> > identity("unordered_set", identity_traits<T>::get());
        return &identity;
    }

    template<class KT, class T>
    inline const container_identity *identity_traits<std::map<KT, T>>::get() {
        static const type_identity_for<std::map<KT, T> > identity("map", identity_traits<KT>::get(), identity_traits<T>::get());
        return &identity;
    }

    template<class KT, class T>
    inline const container_identity *identity_traits<std::unordered_map<KT, T>>::get() {
        static const type_identity_for<std::unordered_map<KT, T> > identity("unordered_map", identity_traits<KT>::get(), identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const bit_container_identity *identity_traits<BitArray<T> >::get() {
        static const type_identity_for<BitArray<T> > identity(identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const container_identity *identity_traits<DfArray<T> >::get() {
        static const type_identity_for<DfArray<T> > identity("DfArray", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    inline const container_identity *identity_traits<enum_list_attr<T> >::get() {
        static const type_identity_for<enum_list_attr<T> > identity("enum_list_attr", identity_traits<T>::get());
        return &identity;
    }
#endif
}
