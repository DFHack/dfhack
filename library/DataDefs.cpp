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
    //FIXME: ... nuked. the group was empty...
/*
    // Read pre-filled vtable ptrs
    OffsetGroup *ptr_table = core->vinfo->getGroup("vtable");
    for (virtual_identity *p = list; p; p = p->next) {
        void * tmp;
        if (ptr_table->getSafeAddress(p->getName(),tmp))
            p->vtable_ptr = tmp;
    }
    */
}

bool DFHack::findBitfieldField(unsigned *idx, const std::string &name,
                               unsigned size, const bitfield_item_info *items)
{
    for (unsigned i = 0; i < size; i++) {
        if (items[i].name && items[i].name == name)
        {
            *idx = i;
            return true;
        }
    }

    return false;
}

void DFHack::setBitfieldField(void *p, unsigned idx, unsigned size, int value)
{
    uint8_t *data = ((uint8_t*)p) + (idx/8);
    unsigned shift = idx%8;
    uint32_t mask = ((1<<size)-1) << shift;
    uint32_t vmask = ((value << shift) & mask);

#define ACCESS(type) *(type*)data = type((*(type*)data & ~mask) | vmask)

    if (!(mask & ~0xFFU)) ACCESS(uint8_t);
    else if (!(mask & ~0xFFFFU)) ACCESS(uint16_t);
    else ACCESS(uint32_t);

#undef ACCESS
}

int DFHack::getBitfieldField(const void *p, unsigned idx, unsigned size)
{
    const uint8_t *data = ((const uint8_t*)p) + (idx/8);
    unsigned shift = idx%8;
    uint32_t mask = ((1<<size)-1) << shift;

#define ACCESS(type) return int((*(type*)data & mask) >> shift)

    if (!(mask & ~0xFFU)) ACCESS(uint8_t);
    else if (!(mask & ~0xFFFFU)) ACCESS(uint16_t);
    else ACCESS(uint32_t);

#undef ACCESS
}

void DFHack::bitfieldToString(std::vector<std::string> *pvec, const void *p,
                              unsigned size, const bitfield_item_info *items)
{
    for (unsigned i = 0; i < size; i++) {
        int value = getBitfieldField(p, i, std::min(1,items[i].size));

        if (value) {
            std::string name = format_key(items[i].name, i);

            if (items[i].size > 1)
                name += stl_sprintf("=%u", value);

            pvec->push_back(name);
        }

        if (items[i].size > 1)
            i += items[i].size-1;
    }
}

int DFHack::findEnumItem(const std::string &name, int size, const char *const *items)
{
    for (int i = 0; i < size; i++) {
        if (items[i] && items[i] == name)
            return i;
    }

    return -1;
}

void DFHack::flagarrayToString(std::vector<std::string> *pvec, const void *p,
                               int bytes, int base, int size, const char *const *items)
{
    for (unsigned i = 0; i < bytes*8; i++) {
        int value = getBitfieldField(p, i, 1);

        if (value)
        {
            int ridx = int(i) - base;
            const char *name = (ridx >= 0 && ridx < size) ? items[ridx] : NULL;
            pvec->push_back(format_key(name, i));
        }
    }
}
