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
#include <vector>
#include <map>

#include "Core.h"
#include "BitArray.h"

// Stop some MS stupidity
#ifdef interface
	#undef interface
#endif

namespace DFHack
{
    class virtual_class {};

#ifdef _MSC_VER
    typedef void *virtual_ptr;
#else
    typedef virtual_class *virtual_ptr;
#endif

    class DFHACK_EXPORT virtual_identity {
        static virtual_identity *list;
        static std::map<void*, virtual_identity*> known;
        
        virtual_identity *prev, *next;
        const char *dfhack_name;
        const char *original_name;
        virtual_identity *parent;
        std::vector<virtual_identity*> children;
        
        void *vtable_ptr;
        bool has_children;

    protected:
        virtual_identity(const char *dfhack_name, const char *original_name, virtual_identity *parent);

        static void *get_vtable(virtual_ptr instance_ptr) { return *(void**)instance_ptr; }

    public:
        const char *getName() { return dfhack_name; }
        const char *getOriginalName() { return original_name ? original_name : dfhack_name; }

        virtual_identity *getParent() { return parent; }
        const std::vector<virtual_identity*> &getChildren() { return children; }

    public:
        static virtual_identity *get(virtual_ptr instance_ptr);
        
        bool is_subclass(virtual_identity *subtype);
        bool is_instance(virtual_ptr instance_ptr) {
            if (!instance_ptr) return false;
            if (vtable_ptr) {
                void *vtable = get_vtable(instance_ptr);
                if (vtable == vtable_ptr) return true;
                if (!has_children) return false;
            }
            return is_subclass(get(instance_ptr));
        }

        bool is_direct_instance(virtual_ptr instance_ptr) {
            if (!instance_ptr) return false;
            return vtable_ptr ? (vtable_ptr == get_vtable(instance_ptr)) 
                              : (this == get(instance_ptr));
        }

    public:
        bool can_instantiate() { return (vtable_ptr != NULL); }
        virtual_ptr instantiate() { return can_instantiate() ? do_instantiate() : NULL; }
        static virtual_ptr clone(virtual_ptr obj);

    protected:
        virtual virtual_ptr do_instantiate() = 0;
        virtual void do_copy(virtual_ptr tgt, virtual_ptr src) = 0;
    public:
        static void Init(Core *core);

        // Strictly for use in virtual class constructors
        void adjust_vtable(virtual_ptr obj, virtual_identity *main);
    };

    template<class T>
    inline T *virtual_cast(virtual_ptr ptr) {
        return T::_identity.is_instance(ptr) ? static_cast<T*>(ptr) : NULL;
    }

    template<class T>
    inline T *strict_virtual_cast(virtual_ptr ptr) {
        return T::_identity.is_direct_instance(ptr) ? static_cast<T*>(ptr) : NULL;
    }

    void InitDataDefGlobals(Core *core);

    template<class T>
    T *ifnull(T *a, T *b) { return a ? a : b; }
}

namespace df
{
    using DFHack::virtual_ptr;
    using DFHack::virtual_identity;
    using DFHack::virtual_class;
    using DFHack::BitArray;

    template<class T>
    class class_virtual_identity : public virtual_identity {
    public:
        class_virtual_identity(const char *dfhack_name, const char *original_name, virtual_identity *parent)
            : virtual_identity(dfhack_name, original_name, parent) {};

        T *instantiate() { return static_cast<T*>(virtual_identity::instantiate()); }
        T *clone(T* obj) { return static_cast<T*>(virtual_identity::clone(obj)); }

    protected:
        virtual virtual_ptr do_instantiate() { return new T(); }
        virtual void do_copy(virtual_ptr tgt, virtual_ptr src) { *static_cast<T*>(tgt) = *static_cast<T*>(src); }
    };

    template<class EnumType, class IntType = int32_t>
    struct enum_field {
        IntType value;

        enum_field() {}
        enum_field(EnumType ev) : value(IntType(ev)) {}
        template<class T>
        enum_field(enum_field<EnumType,T> ev) : value(IntType(ev.value)) {}

        operator EnumType () { return EnumType(value); }
        enum_field<EnumType,IntType> &operator=(EnumType ev) {
            value = IntType(ev); return *this;
        }
    };

    namespace enums {}
}

#define ENUM_ATTR(enum,attr,val) (df::enums::enum::get_##attr(val))
#define ENUM_ATTR_STR(enum,attr,val) DFHack::ifnull(ENUM_ATTR(enum,attr,val),"?")
#define ENUM_KEY_STR(enum,val) ENUM_ATTR_STR(enum,key,val)
#define ENUM_FIRST_ITEM(enum) (df::enums::enum::_first_item_of_##enum)
#define ENUM_LAST_ITEM(enum) (df::enums::enum::_last_item_of_##enum)

namespace df {
#define DF_KNOWN_GLOBALS \
    GLOBAL(cursor,cursor) \
    GLOBAL(selection_rect,selection_rect) \
    GLOBAL(world,world) \
    GLOBAL(ui,ui) \
    GLOBAL(gview,interface) \
    GLOBAL(init,init) \
    GLOBAL(d_init,d_init) \
    SIMPLE_GLOBAL(ui_look_cursor,int) \
    SIMPLE_GLOBAL(ui_workshop_job_cursor,int) \
    GLOBAL(ui_sidebar_menus,ui_sidebar_menus) \
    GLOBAL(ui_build_selector,ui_build_selector) \
    GLOBAL(ui_look_list,ui_look_list)

#define SIMPLE_GLOBAL(name,tname) \
    namespace global { extern DFHACK_EXPORT tname *name; }
#define GLOBAL(name,tname) \
    struct tname; SIMPLE_GLOBAL(name,tname)
DF_KNOWN_GLOBALS
#undef GLOBAL
#undef SIMPLE_GLOBAL
}
