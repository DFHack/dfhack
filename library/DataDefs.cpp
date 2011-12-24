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

#include "dfhack/Process.h"
#include "dfhack/Core.h"
#include "dfhack/DataDefs.h"
#include "dfhack/VersionInfo.h"
#include "tinythread.h"

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
        known[vtable] = p;
        p->vtable_ptr = vtable;
        return p;
    }

    known[vtable] = NULL;
    return NULL;
}

bool virtual_identity::check_instance(virtual_ptr instance_ptr, bool allow_subclasses)
{
    virtual_identity *actual = get(instance_ptr);

    if (actual == this) return true;
    if (!allow_subclasses || !actual) return false;

    do {
        actual = actual->parent;
        if (actual == this) return true;
    } while (actual);

    return false;
}

void virtual_identity::Init()
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
}

#define GLOBAL(name,tname) \
    df::tname *df::global::name = NULL;
DF_KNOWN_GLOBALS
#undef GLOBAL

void DFHack::InitDataDefGlobals(Core *core) {
    OffsetGroup *global_table = core->vinfo->getGroup("global");
    uint32_t tmp;

#define GLOBAL(name,tname) \
    if (global_table->getSafeAddress(#name,tmp)) df::global::name = (df::tname*)tmp;
DF_KNOWN_GLOBALS
#undef GLOBAL
}
