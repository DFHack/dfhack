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

#include "Internal.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "MemAccess.h"
#include "Core.h"
#include "VersionInfo.h"
#include "VTableInterpose.h"

#include "MiscUtils.h"

using namespace DFHack;

/*
 *  Code for accessing method pointers directly. Very compiler-specific.
 *
 *  Pointers to methods in C++ are conceptually similar to pointers to
 *  functions, but with some complications. Specifically, the target of
 *  such pointer can be either:
 *
 *  - An ordinary non-virtual method, in which case the pointer behaves
 *    not much differently from a simple function pointer.
 *  - A virtual method, in which case calling the pointer must emulate
 *    an ordinary call to that method, i.e. fetch the real code address
 *    from the vtable at the appropriate index.
 *
 *  This means that pointers to virtual methods actually have to encode
 *  the relevant vtable index value in some way. Also, since these two
 *  types of pointers cannot be distinguished by data type and differ
 *  only in value, any sane compiler would ensure that any non-virtual
 *  method that can potentially be called via a pointer uses the same
 *  parameter passing rules as an equivalent virtual method, so that
 *  the same parameter passing code would work with both types of pointer.
 *
 *  This means that with a few small low-level compiler-specific wrappers
 *  to access the data inside such pointers it is possible to:
 *
 *  - Convert a non-virtual method pointer into a code address that
 *    can be directly put into a vtable.
 *  - Convert a pointer taken out of a vtable into a fake non-virtual
 *    method pointer that can be used to easily call the original
 *    vmethod body.
 *  - Extract a vtable index out of a virtual method pointer.
 *
 *  Taken together, these features allow delegating all the difficult
 *  and fragile tasks like passing parameters and calculating the
 *  vtable index to the C++ compiler.
 */

#if defined(_MSC_VER)

// MSVC may use up to 3 different representations
// based on context, but adding the /vmg /vmm options
// forces it to stick to this one. It can accomodate
// multiple, but not virtual inheritance.
struct MSVC_MPTR {
    void *method;
    uint32_t this_shift; // was intptr_t pre-0.43.05
};

// Debug builds sometimes use additional thunks that
// just jump to the real one, presumably to attach some
// additional debug info.
static uint32_t *follow_jmp(void *ptr)
{
    uint8_t *p = (uint8_t*)ptr;

    for (;;)
    {
        switch (*p)
        {
#ifdef DFHACK64
        case 0x48: // REX prefix
            p++;
            break;
#endif
        case 0xE9: // jmp near rel32
            p += 5 + *(int32_t*)(p+1);
            break;
        case 0xEB: // jmp short rel8
            p += 2 + *(int8_t*)(p+1);
            break;
        default:
            return (uint32_t*)p;
        }
    }
}

bool DFHack::is_vmethod_pointer_(void *pptr)
{
    auto pobj = (MSVC_MPTR*)pptr;
    if (!pobj->method) return false;

    // MSVC implements pointers to vmethods via thunks.
    // This expects that they all follow a very specific pattern.
    auto pval = follow_jmp(pobj->method);
    switch (pval[0]) {
    case 0x20FF018BU: // mov eax, [ecx]; jmp [eax]
    case 0x60FF018BU: // mov eax, [ecx]; jmp [eax+0x??]
    case 0xA0FF018BU: // mov eax, [ecx]; jmp [eax+0x????????]
        return true;
    default:
        return false;
    }
}

int DFHack::vmethod_pointer_to_idx_(void *pptr)
{
    auto pobj = (MSVC_MPTR*)pptr;
    if (!pobj->method || pobj->this_shift != 0) return -1;

    auto pval = follow_jmp(pobj->method);
    switch (pval[0]) {
    case 0x20FF018BU: // mov eax, [ecx]; jmp [eax]
        return 0;
    case 0x60FF018BU: // mov eax, [ecx]; jmp [eax+0x??]
        return ((int8_t)pval[1])/sizeof(void*);
    case 0xA0FF018BU: // mov eax, [ecx]; jmp [eax+0x????????]
        return ((int32_t)pval[1])/sizeof(void*);
    default:
        return -1;
    }
}

void* DFHack::method_pointer_to_addr_(void *pptr)
{
    if (is_vmethod_pointer_(pptr)) return NULL;
    auto pobj = (MSVC_MPTR*)pptr;
    return pobj->method;
}

void DFHack::addr_to_method_pointer_(void *pptr, void *addr)
{
    auto pobj = (MSVC_MPTR*)pptr;
    pobj->method = addr;
    pobj->this_shift = 0;
}

#elif defined(__GXX_ABI_VERSION)

// GCC seems to always use this structure - possibly unless
// virtual inheritance is involved, but that's irrelevant.
struct GCC_MPTR {
    intptr_t method; // Code pointer or tagged vtable offset
    intptr_t this_shift;
};

bool DFHack::is_vmethod_pointer_(void *pptr)
{
    auto pobj = (GCC_MPTR*)pptr;
    return (pobj->method & 1) != 0;
}

int DFHack::vmethod_pointer_to_idx_(void *pptr)
{
    auto pobj = (GCC_MPTR*)pptr;
    if ((pobj->method & 1) == 0 || pobj->this_shift != 0)
        return -1;
    return (pobj->method-1)/sizeof(void*);
}

void* DFHack::method_pointer_to_addr_(void *pptr)
{
    auto pobj = (GCC_MPTR*)pptr;
    if ((pobj->method & 1) != 0 || pobj->this_shift != 0)
        return NULL;
    return (void*)pobj->method;
}

void DFHack::addr_to_method_pointer_(void *pptr, void *addr)
{
    auto pobj = (GCC_MPTR*)pptr;
    pobj->method = (intptr_t)addr;
    pobj->this_shift = 0;
}

#else
#error Unknown compiler type
#endif

void *virtual_identity::get_vmethod_ptr(int idx)
{
    assert(idx >= 0);
    void **vtable = (void**)vtable_ptr;
    if (!vtable) return NULL;
    return vtable[idx];
}

void *virtual_identity::get_original_vmethod_ptr(int idx) const {
    const auto it = interpose_list.find(idx);
    if (it == interpose_list.end())
        return static_cast<void **>(vtable_ptr)[idx];
    auto l = it->second;
    while (l->prev != nullptr)
        l = l->prev;
    return l->saved_chain;
}

bool virtual_identity::set_vmethod_ptr(MemoryPatcher &patcher, int idx, void *ptr)
{
    assert(idx >= 0);
    void **vtable = (void**)vtable_ptr;
    if (!vtable) return NULL;
    return patcher.write(&vtable[idx], &ptr, sizeof(void*));
}

/*
   VMethod interposing data structures.

   In order to properly support adding and removing hooks,
   it is necessary to track them. This is what this class
   is for. The task is further complicated by propagating
   hooks to child classes that use exactly the same original
   vmethod implementation.

   Every applied link contains in the saved_chain field a
   pointer to the next vmethod body that should be called
   by the hook the link represents. This is the actual
   control flow structure that needs to be maintained.

   There also are connections between link objects themselves,
   which constitute the bookkeeping for doing that. Finally,
   every link is associated with a fixed virtual_identity host,
   which represents the point in the class hierarchy where
   the hook is applied.

   When there are no subclasses (i.e. only one host), the
   structures look like this:

   +--------------+             +------------+
   | link1        |-next------->| link2      |-next=NULL
   |s_c: original |<-------prev-|s_c: $link1 |<--+
   +--------------+             +------------+   |
                                                 |
         host->interpose_list[vmethod_idx] ------+
         vtable: $link2

   The original vtable entry is stored in the saved_chain of the
   first link. The interpose_list map points to the last one.
   The hooks are called in order: link2 -> link1 -> original.

   When there are subclasses that use the same vmethod, but don't
   hook it, the topmost link gets a set of the child_hosts, and
   the hosts have the link added to their interpose_list:

   +--------------+                    +----------------+
   | link0 @host0 |<--+-interpose_list-| host1          |
   |              |-child_hosts-+----->| vtable: $link  |
   +--------------+   |         |      +----------------+
                      |         |
                      |         |      +----------------+
                      +-interpose_list-| host2          |
                                +----->| vtable: $link  |
                                       +----------------+

   When a child defines its own hook, the child_hosts link is
   severed and replaced with a child_next pointer to the new
   hook. The hook still points back the chain with prev.
   All child links to subclasses of host2 are migrated from
   link1 to link2.

   +--------------+-next=NULL         +--------------+-next=NULL
   | link1 @host1 |-child_next------->| link2 @host2 |-child_*--->subclasses
   |              |<-------------prev-|s_c: $link1   |
   +--------------+<-------+          +--------------+<-------+
                           |                                  |
   +--------------+        |          +--------------+        |
   | host1        |-i_list-+          | host2        |-i_list-+
   |vtable: $link1|                   |vtable: $link2|
   +--------------+                   +--------------+

 */

void VMethodInterposeLinkBase::set_chain(void *chain)
{
    saved_chain = chain;
    addr_to_method_pointer_(chain_mptr, chain);
}

VMethodInterposeLinkBase::VMethodInterposeLinkBase(virtual_identity *host, int vmethod_idx, void *interpose_method, void *chain_mptr, int priority, const char *name)
    : host(host), vmethod_idx(vmethod_idx), interpose_method(interpose_method),
      chain_mptr(chain_mptr), priority(priority), name_str(name),
      applied(false), saved_chain(NULL), next(NULL), prev(NULL)
{
    if (vmethod_idx < 0 || interpose_method == NULL)
    {
        /*
         * A failure here almost certainly means a problem in one
         * of the pointer-to-method access wrappers above:
         *
         * - vmethod_idx      comes from vmethod_pointer_to_idx_
         * - interpose_method comes from method_pointer_to_addr_
         */

        fprintf(stderr, "Bad VMethodInterposeLinkBase arguments: %d %p (%s)\n",
                vmethod_idx, interpose_method, name_str);
        fflush(stderr);
        abort();
    }
}

VMethodInterposeLinkBase::~VMethodInterposeLinkBase()
{
    if (is_applied())
        remove();
}

VMethodInterposeLinkBase *VMethodInterposeLinkBase::get_first_interpose(virtual_identity *id)
{
    auto item = id->interpose_list[vmethod_idx];
    if (!item)
        return NULL;

    if (item->host != id)
        return NULL;
    while (item->prev && item->prev->host == id)
        item = item->prev;

    return item;
}

bool VMethodInterposeLinkBase::find_child_hosts(virtual_identity *cur, void *vmptr)
{
    auto &children = cur->getChildren();
    bool found = false;

    for (size_t i = 0; i < children.size(); i++)
    {
        auto child = static_cast<virtual_identity*>(children[i]);
        auto base = get_first_interpose(child);

        if (base)
        {
            assert(base->prev == NULL);

            if (base->saved_chain != vmptr)
                continue;

            child_next.insert(base);
            found = true;
        }
        else if (child->vtable_ptr)
        {
            void *cptr = child->get_vmethod_ptr(vmethod_idx);
            if (cptr != vmptr)
                continue;

            child_hosts.insert(child);
            found = true;

            find_child_hosts(child, vmptr);
        }
        else
        {
            // If this vtable is not known, but any of the children
            // have the same vmethod, this one definitely does too
            if (find_child_hosts(child, vmptr))
            {
                child_hosts.insert(child);
                found = true;
            }
        }
    }

    return found;
}

void VMethodInterposeLinkBase::on_host_delete(virtual_identity *from)
{
    if (from == host)
    {
        // When in own host, fully delete
        remove();
    }
    else
    {
        // Otherwise, drop the link to that child:
        assert(child_hosts.count(from) != 0 &&
               from->interpose_list[vmethod_idx] == this);

        // Find and restore the original vmethod ptr
        auto last = this;
        while (last->prev) last = last->prev;

        MemoryPatcher patcher;

        from->set_vmethod_ptr(patcher, vmethod_idx, last->saved_chain);

        // Unlink the chains
        child_hosts.erase(from);
        from->interpose_list[vmethod_idx] = NULL;
    }
}

bool VMethodInterposeLinkBase::apply(bool enable)
{
    if (!enable)
    {
        remove();
        return true;
    }

    if (is_applied())
        return true;
    if (!host->vtable_ptr)
    {
        std::cerr << "VMethodInterposeLinkBase::apply(" << enable << "): " << name()
            << ": no vtable pointer: " << host->getName() << endl;
        return false;
    }

    // Retrieve the current vtable entry
    VMethodInterposeLinkBase *old_link = host->interpose_list[vmethod_idx];
    VMethodInterposeLinkBase *next_link = NULL;

    while (old_link && old_link->host == host && old_link->priority > priority)
    {
        next_link = old_link;
        old_link = old_link->prev;
    }

    void *old_ptr = next_link ? next_link->saved_chain : host->get_vmethod_ptr(vmethod_idx);
    assert(old_ptr != NULL && (!old_link || old_link->interpose_method == old_ptr));

    // Apply the new method ptr
    MemoryPatcher patcher;

    set_chain(old_ptr);

    if (next_link)
    {
        next_link->set_chain(interpose_method);
    }
    else if (!host->set_vmethod_ptr(patcher, vmethod_idx, interpose_method))
    {
        std::cerr << "VMethodInterposeLinkBase::apply(" << enable << "): " << name() << ": set_vmethod_ptr failed" << endl;
        set_chain(NULL);
        return false;
    }

    // Push the current link into the home host
    applied = true;
    prev = old_link;
    next = next_link;

    if (next_link)
        next_link->prev = this;
    else
        host->interpose_list[vmethod_idx] = this;

    child_hosts.clear();
    child_next.clear();

    if (old_link && old_link->host == host)
    {
        // If the old link is home, just push into the plain chain
        assert(old_link->next == next_link);
        old_link->next = this;

        // Child links belong to the topmost local entry
        child_hosts.swap(old_link->child_hosts);
        child_next.swap(old_link->child_next);
    }
    else if (next_link)
    {
        if (old_link)
        {
            assert(old_link->child_next.count(next_link));
            old_link->child_next.erase(next_link);
            old_link->child_next.insert(this);
        }
    }
    else
    {
        // If creating a new local chain, find children with same vmethod
        find_child_hosts(host, old_ptr);

        if (old_link)
        {
            // Enter the child chain set
            assert(old_link->child_hosts.count(host));
            old_link->child_hosts.erase(host);
            old_link->child_next.insert(this);

            // Subtract our own children from the parent's sets
            for (auto it = child_next.begin(); it != child_next.end(); ++it)
                old_link->child_next.erase(*it);
            for (auto it = child_hosts.begin(); it != child_hosts.end(); ++it)
                old_link->child_hosts.erase(*it);
        }
    }

    assert (!next_link || (child_next.empty() && child_hosts.empty()));

    // Chain subclass hooks
    for (auto it = child_next.begin(); it != child_next.end(); ++it)
    {
        auto nlink = *it;
        assert(nlink->saved_chain == old_ptr && nlink->prev == old_link);
        nlink->set_chain(interpose_method);
        nlink->prev = this;
    }

    // Chain passive subclass hosts
    for (auto it = child_hosts.begin(); it != child_hosts.end(); ++it)
    {
        auto nhost = *it;
        assert(nhost->interpose_list[vmethod_idx] == old_link);
        nhost->set_vmethod_ptr(patcher, vmethod_idx, interpose_method);
        nhost->interpose_list[vmethod_idx] = this;
    }

    return true;
}

void VMethodInterposeLinkBase::remove()
{
    if (!is_applied())
        return;

    // Remove the link from prev to this
    if (prev)
    {
        if (prev->host == host)
            prev->next = next;
        else
        {
            prev->child_next.erase(this);

            if (next)
                prev->child_next.insert(next);
            else
                prev->child_hosts.insert(host);
        }
    }

    if (next)
    {
        next->set_chain(saved_chain);
        next->prev = prev;

        assert(child_next.empty() && child_hosts.empty());
    }
    else
    {
        MemoryPatcher patcher;

        // Remove from the list in the identity and vtable
        host->interpose_list[vmethod_idx] = prev;
        host->set_vmethod_ptr(patcher, vmethod_idx, saved_chain);

        for (auto it = child_next.begin(); it != child_next.end(); ++it)
        {
            auto nlink = *it;
            assert(nlink->saved_chain == interpose_method && nlink->prev == this);
            nlink->set_chain(saved_chain);
            nlink->prev = prev;
            if (prev)
                prev->child_next.insert(nlink);
        }

        for (auto it = child_hosts.begin(); it != child_hosts.end(); ++it)
        {
            auto nhost = *it;
            assert(nhost->interpose_list[vmethod_idx] == this);
            nhost->interpose_list[vmethod_idx] = prev;
            nhost->set_vmethod_ptr(patcher, vmethod_idx, saved_chain);
            if (prev)
                prev->child_hosts.insert(nhost);
        }
    }

    applied = false;
    prev = next = NULL;
    child_next.clear();
    child_hosts.clear();
    set_chain(NULL);
}
