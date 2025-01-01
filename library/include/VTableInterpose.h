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

#include "DataFuncs.h"

namespace DFHack
{
    /* VMethod interpose API.

       This API allows replacing an entry in the original vtable
       with code defined by DFHack, while retaining ability to
       call the original code. The API can be safely used from
       plugins, and multiple hooks for the same vmethod are
       automatically chained (subclass before superclass; at same
       level highest priority called first; undefined order otherwise).

       Usage:

       struct my_hack : df::someclass {
           typedef df::someclass interpose_base;

           // You may define additional methods here, but NOT non-static fields

           DEFINE_VMETHOD_INTERPOSE(int, foo, (int arg)) {
               ... this->field ... // access fields of the df::someclass object
               ...
               int orig_retval = INTERPOSE_NEXT(foo)(arg); // call the original method
               ...
           }
       };

       IMPLEMENT_VMETHOD_INTERPOSE(my_hack, foo);
       or
       IMPLEMENT_VMETHOD_INTERPOSE_PRIO(my_hack, foo, priority);

       void init() {
           if (!INTERPOSE_HOOK(my_hack, foo).apply())
               error();
       }

       void shutdown() {
           INTERPOSE_HOOK(my_hack, foo).remove();
       }

       Important caveat:

       This will NOT intercept calls to the superclass vmethod
       from overriding vmethod bodies in subclasses, i.e. whenever
       DF code contains something like this, the call to "superclass::foo()"
       doesn't actually use vtables, and thus will never trigger any hooks:

       class superclass { virtual foo() { ... } };
       class subclass : superclass { virtual foo() { ... superclass::foo(); ... } };

       The only workaround is to implement and apply a second hook for subclass::foo,
       and repeat that for any other subclasses and sub-subclasses that override this
       vmethod.
     */

    template<bool> struct StaticAssert;
    template<> struct StaticAssert<true> {};

#define STATIC_ASSERT(condition) { StaticAssert<(condition)>(); }

    /* Wrapping around compiler-specific representation of pointers to methods. */

#if defined(_MSC_VER)
#define METHOD_POINTER_SIZE (sizeof(void*)*2)
#elif defined(__GXX_ABI_VERSION)
#define METHOD_POINTER_SIZE (sizeof(void*)*2)
#else
#error Unknown compiler type
#endif

#define ASSERT_METHOD_POINTER(type) \
    STATIC_ASSERT(df::return_type<type>::is_method && sizeof(type)==METHOD_POINTER_SIZE);

    DFHACK_EXPORT bool is_vmethod_pointer_(void*);
    DFHACK_EXPORT int vmethod_pointer_to_idx_(void*);
    DFHACK_EXPORT void* method_pointer_to_addr_(void*);
    DFHACK_EXPORT void addr_to_method_pointer_(void*,void*);

    template<class T> bool is_vmethod_pointer(T ptr) {
        ASSERT_METHOD_POINTER(T);
        return is_vmethod_pointer_(&ptr);
    }
    template<class T> int vmethod_pointer_to_idx(T ptr) {
        ASSERT_METHOD_POINTER(T);
        return vmethod_pointer_to_idx_(&ptr);
    }
    template<class T> void *method_pointer_to_addr(T ptr) {
        ASSERT_METHOD_POINTER(T);
        return method_pointer_to_addr_(&ptr);
    }
    template<class T> T addr_to_method_pointer(void *addr) {
        ASSERT_METHOD_POINTER(T);
        T rv;
        addr_to_method_pointer_(&rv, addr);
        return rv;
    }

    /* Access to vmethod pointers from the vtable. */

    template<class P>
    P virtual_identity::get_vmethod_ptr(P selector)
    {
        typedef typename df::return_type<P>::class_type host_class;
        virtual_identity &identity = host_class::_identity;
        int idx = vmethod_pointer_to_idx(selector);
        return addr_to_method_pointer<P>(identity.get_vmethod_ptr(idx));
    }


#define DEFINE_VMETHOD_INTERPOSE(rtype, name, args) \
    typedef rtype (interpose_base::*interpose_ptr_##name)args; \
    static DFHack::VMethodInterposeLink<interpose_base,interpose_ptr_##name> interpose_##name; \
    rtype interpose_fn_##name args

#define IMPLEMENT_VMETHOD_INTERPOSE_PRIO(class,name,priority) \
    DFHack::VMethodInterposeLink<class::interpose_base,class::interpose_ptr_##name> \
        class::interpose_##name(&class::interpose_base::name, &class::interpose_fn_##name, priority, #class"::"#name);

#define IMPLEMENT_VMETHOD_INTERPOSE(class,name) IMPLEMENT_VMETHOD_INTERPOSE_PRIO(class,name,0)

#define INTERPOSE_NEXT(name) (this->*interpose_##name.chain)
#define INTERPOSE_HOOK(class, name) (class::interpose_##name)

    class DFHACK_EXPORT VMethodInterposeLinkBase {
        /*
          These link objects try to:
          1) Allow multiple hooks into the same vmethod
          2) Auto-remove hooks when a plugin is unloaded.
        */
        friend class virtual_identity;

        const virtual_identity *host; // Class with the vtable
        int vmethod_idx;        // Index of the interposed method in the vtable
        void *interpose_method; // Pointer to the code of the interposing method
        void *chain_mptr;       // Pointer to the chain field in the subclass below
        int priority;           // Higher priority hooks are called earlier
        const char *name_str;   // Name of the hook

        bool applied;           // True if this hook is currently applied
        void *saved_chain;      // Pointer to the code of the original vmethod or next hook

        // Chain of hooks within the same host
        VMethodInterposeLinkBase *next, *prev;
        // Subclasses that inherit this topmost hook directly
        std::set<const virtual_identity*> child_hosts;
        // Hooks within subclasses that branch off this topmost hook
        std::set<VMethodInterposeLinkBase*> child_next;
        // (See the cpp file for a more detailed description of these links)

        void set_chain(void *chain);
        void on_host_delete(const virtual_identity *host);

        VMethodInterposeLinkBase *get_first_interpose(const virtual_identity *id);
        bool find_child_hosts(const virtual_identity *cur, void *vmptr);
    public:
        VMethodInterposeLinkBase(const virtual_identity *host, int vmethod_idx, void *interpose_method, void *chain_mptr, int priority, const char *name);
        ~VMethodInterposeLinkBase();

        bool is_applied() { return applied; }
        bool apply(bool enable = true);
        void remove();

        const char *name() { return name_str; }
    };

    template<class Base, class Ptr>
    class VMethodInterposeLink : public VMethodInterposeLinkBase {
    public:
        // Exactly the same as the saved_chain field of superclass,
        // but converted to the appropriate pointer-to-method type.
        // Kept up to date via the chain_mptr pointer.
        Ptr chain;

        operator Ptr () { return chain; }

        template<class Ptr2>
        VMethodInterposeLink(Ptr target, Ptr2 src, int priority, const char *name)
            : VMethodInterposeLinkBase(
                &Base::_identity,
                vmethod_pointer_to_idx(target),
                method_pointer_to_addr(src),
                &chain,
                priority, name
              )
        { src = target; /* check compatibility */ }
    };
}
