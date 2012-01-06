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
#include "tinythread.h"
// must be last due to MS stupidity
#include "DataDefs.h"

#include "MiscUtils.h"

using namespace DFHack;

/* The order of global object constructor calls is
 * undefined between compilation units. Therefore,
 * this list has to be plain data, so that it gets
 * initialized by the loader in the initial mmap.
 */
virtual_identity *virtual_identity::list = NULL;

virtual_identity::virtual_identity(const char *dfhack_name, const char *original_name, virtual_identity *parent)
    : dfhack_name(dfhack_name), original_name(original_name), parent(parent),
      prev(NULL), vtable_ptr(NULL), has_children(true)
{
    // Link into the static list. Nothing else can be safely done at this point.
    next = list; list = this;    
}

/* Vtable to identity lookup. */
static tthread::mutex *known_mutex = NULL;
std::map<void*, virtual_identity*> virtual_identity::known;

virtual_identity *virtual_identity::get(virtual_ptr instance_ptr)
{
    if (!instance_ptr) return NULL;

    // Actually, a reader/writer lock would be sufficient,
    // since the table is only written once per class.
    tthread::lock_guard<tthread::mutex> lock(*known_mutex);

    void *vtable = get_vtable(instance_ptr);
    std::map<void*, virtual_identity*>::iterator it = known.find(vtable);

    if (it != known.end())
        return it->second;

    // If using a reader/writer lock, re-grab as write here, and recheck
    Core &core = Core::getInstance();
    std::string name = core.p->doReadClassName(vtable);

    virtual_identity *actual = NULL;

    for (virtual_identity *p = list; p; p = p->next) {
        if (strcmp(name.c_str(), p->getOriginalName()) != 0) continue;

        if (p->vtable_ptr && p->vtable_ptr != vtable) {
            std::cerr << "Conflicting vtable ptr for class '" << p->getName()
                      << "': found 0x" << std::hex << unsigned(vtable)
                      << ", previous 0x" << unsigned(p->vtable_ptr) << std::dec << std::endl;
            abort();
        } else if (!p->vtable_ptr) {
            std::cerr << "class '" << p->getName() << "': vtable = 0x"
                      << std::hex << unsigned(vtable) << std::dec << std::endl;
        }

        known[vtable] = p;
        p->vtable_ptr = vtable;
        return p;
    }

    std::cerr << "UNKNOWN CLASS '" << name << "': vtable = 0x"
              << std::hex << unsigned(vtable) << std::dec << std::endl;

    known[vtable] = NULL;
    return NULL;
}

bool virtual_identity::is_subclass(virtual_identity *actual)
{
    for (; actual; actual = actual->parent)
        if (actual == this) return true;

    return false;
}

void virtual_identity::adjust_vtable(virtual_ptr obj, virtual_identity *main)
{
    if (vtable_ptr) {
        *(void**)obj = vtable_ptr;
        return;
    }

    if (main && main != this && is_subclass(main))
        return;

    std::cerr << "Attempt to create class '" << getName() << "' without known vtable." << std::endl;
    abort();
}

virtual_ptr virtual_identity::clone(virtual_ptr obj)
{
    virtual_identity *id = get(obj);
    if (!id) return NULL;
    virtual_ptr copy = id->instantiate();
    if (!copy) return NULL;
    id->do_copy(copy, obj);
    return copy;
}

void virtual_identity::Init(Core *core)
{
    if (!known_mutex)
        known_mutex = new tthread::mutex();

    // This cannot be done in the constructors, because
    // they are called in an undefined order.
    for (virtual_identity *p = list; p; p = p->next) {
        p->has_children = false;
        p->children.clear();
    }
    for (virtual_identity *p = list; p; p = p->next) {
        if (p->parent) {
            p->parent->children.push_back(p);
            p->parent->has_children = true;
        }
    }

    // Read pre-filled vtable ptrs
    OffsetGroup *ptr_table = core->vinfo->getGroup("vtable");
    for (virtual_identity *p = list; p; p = p->next) {
        void * tmp;
        if (ptr_table->getSafeAddress(p->getName(),tmp))
            p->vtable_ptr = tmp;
    }
}

std::string DFHack::bitfieldToString(const void *p, int size, const bitfield_item_info *items)
{
    std::string res;
    const char *data = (const char*)p;

    for (int i = 0; i < size*8; i++) {
        unsigned v;

        if (items[i].size > 1) {
            unsigned pdv = *(unsigned*)&data[i/8];
            v = (pdv >> (i%8)) & ((1 << items[i].size)-1);
        } else {
            v = (data[i/8]>>(i%8)) & 1;
        }

        if (v) {
            if (!res.empty())
                res += ' ';

            if (items[i].name)
                res += items[i].name;
            else
                res += stl_sprintf("UNK_%d", i);

            if (items[i].size > 1)
                res += stl_sprintf("=%u", v);
        }

        if (items[i].size > 1)
            i += items[i].size-1;
    }

    return res;
}

#define SIMPLE_GLOBAL(name,tname) \
    tname *df::global::name = NULL;
#define GLOBAL(name,tname) SIMPLE_GLOBAL(name,df::tname)
DF_KNOWN_GLOBALS
#undef GLOBAL
#undef SIMPLE_GLOBAL

void DFHack::InitDataDefGlobals(Core *core) {
    OffsetGroup *global_table = core->vinfo->getGroup("global");
    void * tmp;

#define SIMPLE_GLOBAL(name,tname) \
    if (global_table->getSafeAddress(#name,tmp)) df::global::name = (tname*)tmp;
#define GLOBAL(name,tname) SIMPLE_GLOBAL(name,df::tname)
DF_KNOWN_GLOBALS
#undef GLOBAL
#undef SIMPLE_GLOBAL
}
