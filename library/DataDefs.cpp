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

#include <string>
#include <vector>
#include <map>

#include "MemAccess.h"
#include "Core.h"
#include "VersionInfo.h"
#include "tinythread.h"
// must be last due to MS stupidity
#include "DataDefs.h"
#include "DataIdentity.h"
#include "VTableInterpose.h"
#include "Error.h"

#include "MiscUtils.h"

using namespace DFHack;

const char *DFHack::identity_type_str(const identity_type type) {
    switch(type) {
    case IDTYPE_GLOBAL:
        return "global";
    case IDTYPE_FUNCTION:
        return "function";
    case IDTYPE_PRIMITIVE:
        return "primitive";
    case IDTYPE_POINTER:
        return "pointer";
    case IDTYPE_CONTAINER:
        return "container";
    case IDTYPE_PTR_CONTAINER:
        return "ptr_container";
    case IDTYPE_BIT_CONTAINER:
        return "bit_container";
    case IDTYPE_BITFIELD:
        return "bitfield";
    case IDTYPE_ENUM:
        return "enum";
    case IDTYPE_STRUCT:
        return "struct";
    case IDTYPE_CLASS:
        return "class";
    case IDTYPE_BUFFER:
        return "buffer";
    case IDTYPE_STL_PTR_VECTOR:
        return "stl_ptr_vector";
    case IDTYPE_OPAQUE:
        return "opaque";
    }
    return "ERROR_UNKNOWN_IDENTITY_TYPE";
}

void *type_identity::do_allocate_pod() {
    size_t sz = byte_size();
    void *p = malloc(sz);
    memset(p, 0, sz);
    return p;
}

void type_identity::do_copy_pod(void *tgt, const void *src) {
    memmove(tgt, src, byte_size());
};

bool type_identity::do_destroy_pod(void *obj) {
    free(obj);
    return true;
}

void *type_identity::allocate() {
    if (can_allocate())
        return do_allocate();
    else
        return NULL;
}

bool type_identity::copy(void *tgt, const void *src) {
    if (can_allocate() && tgt && src)
        return do_copy(tgt, src);
    else
        return false;
}

bool type_identity::destroy(void *obj) {
    if (can_allocate() && obj)
        return do_destroy(obj);
    else
        return false;
}

void *enum_identity::do_allocate() {
    size_t sz = byte_size();
    void *p = malloc(sz);
    memcpy(p, &first_item_value, std::min(sz, sizeof(int64_t)));
    return p;
}

/* The order of global object constructor calls is
 * undefined between compilation units. Therefore,
 * this list has to be plain data, so that it gets
 * initialized by the loader in the initial mmap.
 */
compound_identity *compound_identity::list = NULL;
std::vector<compound_identity*> compound_identity::top_scope;

compound_identity::compound_identity(size_t size, TAllocateFn alloc,
                                     compound_identity *scope_parent, const char *dfhack_name)
    : constructed_identity(size, alloc), dfhack_name(dfhack_name), scope_parent(scope_parent)
{
    next = list; list = this;
}

void compound_identity::doInit(Core *)
{
    if (scope_parent)
        scope_parent->scope_children.push_back(this);
    else
        top_scope.push_back(this);
}

std::string compound_identity::getFullName() const
{
    if (scope_parent)
        return scope_parent->getFullName() + "." + getName();
    else
        return getName();
}

static tthread::mutex *known_mutex = NULL;

void compound_identity::Init(Core *core)
{
    if (!known_mutex)
        known_mutex = new tthread::mutex();

    // This cannot be done in the constructors, because
    // they are called in an undefined order.
    for (compound_identity *p = list; p; p = p->next)
        p->doInit(core);
}

bitfield_identity::bitfield_identity(size_t size,
                                     compound_identity *scope_parent, const char *dfhack_name,
                                     int num_bits, const bitfield_item_info *bits,
                                     const type_identity *_base_type)
    : compound_identity(size, NULL, scope_parent, dfhack_name), bits(bits), num_bits(num_bits),
      base_type{*_base_type}
{
}

enum_identity::enum_identity(size_t size,
                             compound_identity *scope_parent, const char *dfhack_name,
                             type_identity *base_type,
                             int64_t first_item_value, int64_t last_item_value,
                             const char *const *keys,
                             const ComplexData *complex,
                             const void *attrs, struct_identity *attr_type)
    : compound_identity(size, NULL, scope_parent, dfhack_name),
      keys(keys), complex(complex),
      first_item_value(first_item_value), last_item_value(last_item_value),
      base_type(base_type), attrs(attrs), attr_type(attr_type)
{
    if (complex) {
        count = complex->size();
        last_item_value = complex->index_value_map.back();
    }
    else {
        count = int(last_item_value-first_item_value+1);
    }
}

enum_identity::ComplexData::ComplexData(std::initializer_list<int64_t> values)
{
    size_t i = 0;
    for (int64_t value : values) {
        value_index_map[value] = i;
        index_value_map.push_back(value);
        i++;
    }
}

struct_identity::struct_identity(size_t size, TAllocateFn alloc,
                                 compound_identity *scope_parent, const char *dfhack_name,
                                 struct_identity *parent, const struct_field_info *fields,
                                 bool is_u)
    : compound_identity(size, alloc, scope_parent, dfhack_name),
      parent(parent), has_children(false), fields(fields), is_union{is_u}
{
}

void struct_identity::doInit(Core *core)
{
    compound_identity::doInit(core);

    if (parent) {
        parent->children.push_back(this);
        parent->has_children = true;
    }
}

bool struct_identity::is_subclass(struct_identity *actual)
{
    if (!has_children && actual != this)
        return false;

    for (; actual; actual = actual->getParent())
        if (actual == this) return true;

    return false;
}

std::string pointer_identity::getFullName() const
{
    return (target ? target->getFullName() : std::string("void")) + "*";
}

std::string container_identity::getFullName(type_identity *item) const
{
    return "<" + (item ? item->getFullName() : std::string("void")) + ">";
}

std::string ptr_container_identity::getFullName(type_identity *item) const
{
    return "<" + (item ? item->getFullName() : std::string("void")) + "*>";
}

std::string bit_container_identity::getFullName(type_identity *) const
{
    return "<bool>";
}

std::string df::buffer_container_identity::getFullName(type_identity *item) const
{
    return (item ? item->getFullName() : std::string("void")) +
           (size > 0 ? stl_sprintf("[%d]", size) : std::string("[]"));
}

virtual_identity::virtual_identity(size_t size, TAllocateFn alloc,
                                   const char *dfhack_name, const char *original_name,
                                   virtual_identity *parent, const struct_field_info *fields,
                                   const std::vector<std::vector<struct_field_info>> *oms,
                                   const char **omn)
    : struct_identity(size, alloc, NULL, dfhack_name, parent, fields), original_name(original_name),
      vtable_ptr(NULL), own_method_signatures{oms}, own_method_names{omn}
{
}

virtual_identity::~virtual_identity()
{
    // Remove interpose entries, so that they don't try accessing this object later
    for (auto it = interpose_list.begin(); it != interpose_list.end(); ++it)
        if (it->second)
            it->second->on_host_delete(this);
    interpose_list.clear();
}

/* Vtable name to identity lookup. */
static std::map<std::string, virtual_identity*> name_lookup;

/* Vtable pointer to identity lookup. */
std::map<void*, virtual_identity*> virtual_identity::known;

void virtual_identity::doInit(Core *core)
{
    struct_identity::doInit(core);

    auto vtname = getOriginalName();
    name_lookup[vtname] = this;

    vtable_ptr = core->vinfo->getVTable(vtname);
    if (vtable_ptr)
        known[vtable_ptr] = this;
}

virtual_identity *virtual_identity::find(const std::string &name)
{
    auto name_it = name_lookup.find(name);

    return (name_it != name_lookup.end()) ? name_it->second : NULL;
}

virtual_identity *virtual_identity::get(virtual_ptr instance_ptr)
{
    if (!instance_ptr) return NULL;

    return find(get_vtable(instance_ptr));
}

virtual_identity *virtual_identity::find(void *vtable)
{
    if (!vtable)
        return NULL;

    // Actually, a reader/writer lock would be sufficient,
    // since the table is only written once per class.
    tthread::lock_guard<tthread::mutex> lock(*known_mutex);

    std::map<void*, virtual_identity*>::iterator it = known.find(vtable);

    if (it != known.end())
        return it->second;

    // If using a reader/writer lock, re-grab as write here, and recheck
    Core &core = Core::getInstance();
    std::string name = core.p->doReadClassName(vtable);

    auto name_it = name_lookup.find(name);
    if (name_it != name_lookup.end()) {
        virtual_identity *p = name_it->second;

        if (p->vtable_ptr && p->vtable_ptr != vtable) {
            std::cerr << "Conflicting vtable ptr for class '" << p->getName()
                      << "': found 0x" << std::hex << uintptr_t(vtable)
                      << ", previous 0x" << uintptr_t(p->vtable_ptr) << std::dec << std::endl;
            abort();
        } else if (!p->vtable_ptr) {
            uintptr_t pv = uintptr_t(vtable);
            pv -= Core::getInstance().vinfo->getRebaseDelta();
            std::cerr << "<vtable-address name='" << p->getOriginalName() << "' value='0x"
                      << std::hex << pv << std::dec << "'/>" << std::endl;
        }

        known[vtable] = p;
        p->vtable_ptr = vtable;
        return p;
    }

    std::cerr << "UNKNOWN CLASS '" << name << "': vtable = 0x"
              << std::hex << uintptr_t(vtable) << std::dec << std::endl;

    known[vtable] = NULL;
    return NULL;
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
    throw DFHack::Error::VTableMissing(getName());
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
        int value = getBitfieldField(p, i, std::max(1,items[i].size));

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
    for (int i = 0; i < bytes*8; i++) {
        int value = getBitfieldField(p, i, 1);

        if (value)
        {
            int ridx = int(i) - base;
            const char *name = (ridx >= 0 && ridx < size) ? items[ridx] : NULL;
            pvec->push_back(format_key(name, i));
        }
    }
}
