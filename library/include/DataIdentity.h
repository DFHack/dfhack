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

    class DFHACK_EXPORT container_identity : public constructed_identity {
        type_identity *item;
        enum_identity *ienum;

    public:
        container_identity(size_t size, TAllocateFn alloc, type_identity *item, enum_identity *ienum = NULL)
            : constructed_identity(size, alloc), item(item), ienum(ienum) {};

        virtual identity_type type() { return IDTYPE_CONTAINER; }
    };
}

namespace df
{
    using DFHack::primitive_identity;
    using DFHack::container_identity;

#define ATOM_IDENTITY_TRAITS(type) \
    template<> struct identity_traits<type> { \
        static primitive_identity identity; \
        static primitive_identity *get() { return &identity; } \
    };

    ATOM_IDENTITY_TRAITS(char);
    ATOM_IDENTITY_TRAITS(int8_t);
    ATOM_IDENTITY_TRAITS(uint8_t);
    ATOM_IDENTITY_TRAITS(int16_t);
    ATOM_IDENTITY_TRAITS(uint16_t);
    ATOM_IDENTITY_TRAITS(int32_t);
    ATOM_IDENTITY_TRAITS(uint32_t);
    ATOM_IDENTITY_TRAITS(int64_t);
    ATOM_IDENTITY_TRAITS(uint64_t);
    ATOM_IDENTITY_TRAITS(bool);
    ATOM_IDENTITY_TRAITS(float);
    ATOM_IDENTITY_TRAITS(std::string);
    ATOM_IDENTITY_TRAITS(void*);

#undef ATOM_IDENTITY_TRAITS

    // Container declarations

    template<class Enum, class FT> struct identity_traits<enum_field<Enum,FT> > {
        static primitive_identity *get();
    };

    template<class T> struct identity_traits<T *> {
        static container_identity *get();
    };

    template<class T, int sz> struct identity_traits<T [sz]> {
        static container_identity *get();
    };

    template<class T> struct identity_traits<std::vector<T> > {
        static container_identity *get();
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
        static primitive_identity identity(sizeof(FT));
        return &identity;
    }

    template<class T>
    container_identity *identity_traits<T *>::get() {
        typedef T * container;
        static container_identity identity(sizeof(container), &allocator_fn<container>,
                                            identity_traits<T>::get());
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

