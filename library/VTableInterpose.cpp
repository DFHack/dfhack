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

#include "Internal.h"

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
 */

#if defined(_MSC_VER)

struct MSVC_MPTR {
    void *method;
    intptr_t this_shift;
};

bool DFHack::is_vmethod_pointer_(void *pptr)
{
    auto pobj = (MSVC_MPTR*)pptr;
    if (!pobj->method) return false;

    // MSVC implements pointers to vmethods via thunks.
    // This expects that they all follow a very specific pattern.
    auto pval = (unsigned*)pobj->method;
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

    auto pval = (unsigned*)pobj->method;
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

struct GCC_MPTR {
    intptr_t method;
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

bool virtual_identity::set_vmethod_ptr(int idx, void *ptr)
{
    assert(idx >= 0);
    void **vtable = (void**)vtable_ptr;
    if (!vtable) return NULL;
    return Core::getInstance().p->patchMemory(&vtable[idx], &ptr, sizeof(void*));
}

void VMethodInterposeLinkBase::set_chain(void *chain)
{
    saved_chain = chain;
    addr_to_method_pointer_(chain_mptr, chain);
}

VMethodInterposeLinkBase::VMethodInterposeLinkBase(virtual_identity *host, int vmethod_idx, void *interpose_method, void *chain_mptr)
    : host(host), vmethod_idx(vmethod_idx), interpose_method(interpose_method), chain_mptr(chain_mptr),
      saved_chain(NULL), next(NULL), prev(NULL)
{
    if (vmethod_idx < 0 || interpose_method == NULL)
    {
        fprintf(stderr, "Bad VMethodInterposeLinkBase arguments: %d %08x\n",
                vmethod_idx, unsigned(interpose_method));
        fflush(stderr);
        abort();
    }
}

VMethodInterposeLinkBase::~VMethodInterposeLinkBase()
{
    if (is_applied())
        remove();
}

bool VMethodInterposeLinkBase::apply()
{
    if (is_applied())
        return true;
    if (!host->vtable_ptr)
        return false;

    // Retrieve the current vtable entry
    void *old_ptr = host->get_vmethod_ptr(vmethod_idx);
    assert(old_ptr != NULL);

    // Check if there are other interpose entries for the same slot
    VMethodInterposeLinkBase *old_link = NULL;

    for (int i = host->interpose_list.size()-1; i >= 0; i--)
    {
        if (host->interpose_list[i]->vmethod_idx != vmethod_idx)
            continue;

        old_link = host->interpose_list[i];
        assert(old_link->next == NULL && old_ptr == old_link->interpose_method);
        break;
    }

    // Apply the new method ptr
    if (!host->set_vmethod_ptr(vmethod_idx, interpose_method))
        return false;

    set_chain(old_ptr);
    host->interpose_list.push_back(this);

    // Link into the chain if any
    if (old_link)
    {
        old_link->next = this;
        prev = old_link;
    }

    return true;
}

void VMethodInterposeLinkBase::remove()
{
    if (!is_applied())
        return;

    // Remove from the list in the identity
    for (int i = host->interpose_list.size()-1; i >= 0; i--)
        if (host->interpose_list[i] == this)
            vector_erase_at(host->interpose_list, i);

    // Remove from the chain
    if (prev)
        prev->next = next;

    if (next)
    {
        next->set_chain(saved_chain);
        next->prev = prev;
    }
    else
    {
        host->set_vmethod_ptr(vmethod_idx, saved_chain);
    }

    prev = next = NULL;
    set_chain(NULL);
}
