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
    class DFHACK_EXPORT primitive_identity : public type_identity {
    public:
        primitive_identity(size_t size) : type_identity(size) {};

        virtual identity_type type() { return IDTYPE_PRIMITIVE; }
    };

    class DFHACK_EXPORT pointer_identity : public primitive_identity {
        type_identity *target;

    public:
        pointer_identity(type_identity *target = NULL)
            : primitive_identity(sizeof(void*)), target(target) {};

        virtual identity_type type() { return IDTYPE_POINTER; }

        type_identity *getTarget() { return target; }

        static int lua_read(lua_State *state, int fname_idx, void *ptr, type_identity *target);
        static void lua_write(lua_State *state, int fname_idx, void *ptr, type_identity *target, int val_index);

        virtual int lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class DFHACK_EXPORT container_identity : public constructed_identity {
        type_identity *item;
        enum_identity *ienum;

    public:
        container_identity(size_t size, TAllocateFn alloc, type_identity *item, enum_identity *ienum = NULL)
            : constructed_identity(size, alloc), item(item), ienum(ienum) {};

        virtual identity_type type() { return IDTYPE_CONTAINER; }

        type_identity *getItemType() { return item; }
    };
}

namespace df
{
    using DFHack::primitive_identity;
    using DFHack::pointer_identity;
    using DFHack::container_identity;

    class number_identity_base : public primitive_identity {
    public:
        number_identity_base(size_t size) : primitive_identity(size) {};

        virtual int lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);

    protected:
        virtual double read(void *ptr) = 0;
        virtual void write(void *ptr, double val) = 0;
    };

    template<class T>
    class number_identity : public number_identity_base {
    public:
        number_identity() : number_identity_base(sizeof(T)) {}
    protected:
        virtual double read(void *ptr) { return double(*(T*)ptr); }
        virtual void write(void *ptr, double val) { *(T*)ptr = T(val); }
    };

    class bool_identity : public primitive_identity {
    public:
        bool_identity() : primitive_identity(sizeof(bool)) {};

        virtual int lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class stl_string_identity : public primitive_identity {
    public:
        stl_string_identity() : primitive_identity(sizeof(std::string)) {};

        virtual int lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class stl_ptr_vector_identity : public container_identity {
    public:
        stl_ptr_vector_identity(type_identity *item = NULL, enum_identity *ienum = NULL)
            : container_identity(sizeof(std::vector<void*>),allocator_fn<std::vector<void*> >,item, ienum)
        {};

        virtual DFHack::identity_type type() { return DFHack::IDTYPE_STL_PTR_VECTOR; }
    };

#define NUMBER_IDENTITY_TRAITS(type) \
    template<> struct identity_traits<type> { \
        static number_identity<type> identity; \
        static number_identity_base *get() { return &identity; } \
    };

    NUMBER_IDENTITY_TRAITS(char);
    NUMBER_IDENTITY_TRAITS(int8_t);
    NUMBER_IDENTITY_TRAITS(uint8_t);
    NUMBER_IDENTITY_TRAITS(int16_t);
    NUMBER_IDENTITY_TRAITS(uint16_t);
    NUMBER_IDENTITY_TRAITS(int32_t);
    NUMBER_IDENTITY_TRAITS(uint32_t);
    NUMBER_IDENTITY_TRAITS(int64_t);
    NUMBER_IDENTITY_TRAITS(uint64_t);
    NUMBER_IDENTITY_TRAITS(float);

    template<> struct identity_traits<bool> {
        static bool_identity identity;
        static bool_identity *get() { return &identity; }
    };

    template<> struct identity_traits<std::string> {
        static stl_string_identity identity;
        static stl_string_identity *get() { return &identity; }
    };

    template<> struct identity_traits<void*> {
        static pointer_identity identity;
        static pointer_identity *get() { return &identity; }
    };

    template<> struct identity_traits<std::vector<void*> > {
        static stl_ptr_vector_identity identity;
        static stl_ptr_vector_identity *get() { return &identity; }
    };

#undef NUMBER_IDENTITY_TRAITS

    // Container declarations

    template<class Enum, class FT> struct identity_traits<enum_field<Enum,FT> > {
        static primitive_identity *get();
    };

    template<class T> struct identity_traits<T *> {
        static pointer_identity *get();
    };

    template<class T, int sz> struct identity_traits<T [sz]> {
        static container_identity *get();
    };

    template<class T> struct identity_traits<std::vector<T> > {
        static container_identity *get();
    };

    template<class T> struct identity_traits<std::vector<T*> > {
        static stl_ptr_vector_identity *get();
    };

    template<class T> struct identity_traits<std::deque<T> > {
        static container_identity *get();
    };

    template<class T> struct identity_traits<BitArray<T> > {
        static container_identity *get();
    };

    template<class T> struct identity_traits<DfArray<T> > {
        static container_identity *get();
    };

    // Container definitions

    template<class Enum, class FT>
    primitive_identity *identity_traits<enum_field<Enum,FT> >::get() {
        return identity_traits<FT>::get();
    }

    template<class T>
    pointer_identity *identity_traits<T *>::get() {
        static pointer_identity identity(identity_traits<T>::get());
        return &identity;
    }

    template<class T, int sz>
    container_identity *identity_traits<T [sz]>::get() {
        typedef T container[sz];
        static container_identity identity(sizeof(container), NULL,
                                            identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    container_identity *identity_traits<std::vector<T> >::get() {
        typedef std::vector<T> container;
        static container_identity identity(sizeof(container), &allocator_fn<container>,
                                            identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    stl_ptr_vector_identity *identity_traits<std::vector<T*> >::get() {
        static stl_ptr_vector_identity identity(identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    container_identity *identity_traits<std::deque<T> >::get() {
        typedef std::deque<T> container;
        static container_identity identity(sizeof(container), &allocator_fn<container>,
                                            identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    container_identity *identity_traits<BitArray<T> >::get() {
        typedef BitArray<T> container;
        static type_identity *eid = identity_traits<T>::get();
        static enum_identity *reid = eid->type() == DFHack::IDTYPE_ENUM ? (enum_identity*)eid : NULL;
        static container_identity identity(sizeof(container), &allocator_fn<container>,
                                            &identity_traits<bool>::identity, reid);
        return &identity;
    }

    template<class T>
    container_identity *identity_traits<DfArray<T> >::get() {
        typedef DfArray<T> container;
        static container_identity identity(sizeof(container), &allocator_fn<container>,
                                            identity_traits<T>::get());
        return &identity;
    }
}

