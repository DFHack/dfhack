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

#include "DataFuncs.h"

namespace DFHack
{
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

    /* VMethod interpose API.

       This API allows replacing an entry in the original vtable
       with code defined by DFHack, while retaining ability to
       call the original code. The API can be safely used from
       plugins, and multiple hooks for the same vmethod are
       automatically chained in undefined order.

       Usage:

       struct my_hack : df::someclass {
           typedef df::someclass interpose_base;

           DEFINE_VMETHOD_INTERPOSE(void, foo, (int arg)) {
               // If needed by the code, claim the suspend lock.
               // DO NOT USE THE USUAL CoreSuspender, OR IT WILL DEADLOCK!
               // CoreSuspendClaimer suspend;
               ...
               INTERPOSE_NEXT(foo)(arg) // call the original
               ...
           }
       };

       IMPLEMENT_VMETHOD_INTERPOSE(my_hack, foo);

       void init() {
           if (!INTERPOSE_HOOK(my_hack, foo).apply())
               error();
       }

       void shutdown() {
           INTERPOSE_HOOK(my_hack, foo).remove();
       }
     */

#define DEFINE_VMETHOD_INTERPOSE(rtype, name, args) \
    typedef rtype (interpose_base::*interpose_ptr_##name)args; \
    static DFHack::VMethodInterposeLink<interpose_base,interpose_ptr_##name> interpose_##name; \
    rtype interpose_fn_##name args

#define IMPLEMENT_VMETHOD_INTERPOSE(class,name) \
    DFHack::VMethodInterposeLink<class::interpose_base,class::interpose_ptr_##name> \
        class::interpose_##name(&class::interpose_base::name, &class::interpose_fn_##name);

#define INTERPOSE_NEXT(name) (this->*interpose_##name.chain)
#define INTERPOSE_HOOK(class, name) (class::interpose_##name)

    class DFHACK_EXPORT VMethodInterposeLinkBase {
        /*
          These link objects try to:
          1) Allow multiple hooks into the same vmethod
          2) Auto-remove hooks when a plugin is unloaded.
        */
        friend class virtual_identity;

        virtual_identity *host; // Class with the vtable
        int vmethod_idx;
        void *interpose_method; // Pointer to the code of the interposing method
        void *chain_mptr;       // Pointer to the chain field below

        bool applied;
        void *saved_chain;      // Previous pointer to the code
        VMethodInterposeLinkBase *next, *prev; // Other hooks for the same method

        // inherited vtable members
        std::set<virtual_identity*> child_hosts;
        std::set<VMethodInterposeLinkBase*> child_next;

        void set_chain(void *chain);
        void on_host_delete(virtual_identity *host);

        VMethodInterposeLinkBase *get_first_interpose(virtual_identity *id);
        void find_child_hosts(virtual_identity *cur, void *vmptr);
    public:
        VMethodInterposeLinkBase(virtual_identity *host, int vmethod_idx, void *interpose_method, void *chain_mptr);
        ~VMethodInterposeLinkBase();

        bool is_applied() { return applied; }
        bool apply();
        void remove();
    };

    template<class Base, class Ptr>
    class VMethodInterposeLink : public VMethodInterposeLinkBase {
    public:
        Ptr chain;

        operator Ptr () { return chain; }

        template<class Ptr2>
        VMethodInterposeLink(Ptr target, Ptr2 src)
            : VMethodInterposeLinkBase(
                &Base::_identity,
                vmethod_pointer_to_idx(target),
                method_pointer_to_addr(src),
                &chain
              )
        { src = target; /* check compatibility */ }
    };
}
