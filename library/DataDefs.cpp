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
#include <mutex>

#include "MemAccess.h"
#include "Core.h"
#include "VersionInfo.h"
// must be last due to MS stupidity
#include "DataDefs.h"
#include "DataIdentity.h"
#include "VTableInterpose.h"
#include "Error.h"

#include "MiscUtils.h"

using namespace DFHack;


void *type_identity::do_allocate_pod() const {
    size_t sz = byte_size();
    void *p = malloc(sz);
    memset(p, 0, sz);
    return p;
}

void type_identity::do_copy_pod(void *tgt, const void *src) const {
    memmove(tgt, src, byte_size());
};

bool type_identity::do_destroy_pod(void *obj) const {
    free(obj);
    return true;
}

void *type_identity::allocate() const {
    if (can_allocate())
        return do_allocate();
    else
        return NULL;
}

bool type_identity::copy(void *tgt, const void *src) const {
    if (can_allocate() && tgt && src)
        return do_copy(tgt, src);
    else
        return false;
}

bool type_identity::destroy(void *obj) const {
    if (can_allocate() && obj)
        return do_destroy(obj);
    else
        return false;
}

void *enum_identity::do_allocate() const {
    size_t sz = byte_size();
    void *p = malloc(sz);
    memcpy(p, &first_item_value, std::min(sz, sizeof(int64_t)));
    return p;
}

/****** COMPOUND IDENTITIES ******/

decltype(compound_identity::list) compound_identity::list = nullptr;
decltype(compound_identity::parent_map) compound_identity::parent_map = nullptr;
decltype(compound_identity::children_map) compound_identity::children_map = nullptr;
decltype(compound_identity::top_scope) compound_identity::top_scope = nullptr;

void compound_identity::ensure_compound_identity_init()
{
    static std::once_flag compound_identity_init_flag;
    std::call_once(compound_identity_init_flag, [] () {
        list = new (std::remove_pointer<decltype(list)>::type)();
        parent_map = new (std::remove_pointer<decltype(parent_map)>::type)();
        children_map = new (std::remove_pointer<decltype(children_map)>::type)();
        top_scope = new std::vector<const compound_identity*>();
    });
}

compound_identity::compound_identity(size_t size, TAllocateFn alloc,
    const compound_identity* scope_parent, const char* dfhack_name)
    : constructed_identity(size, alloc), dfhack_name(dfhack_name), scope_parent(scope_parent)
{
    ensure_compound_identity_init();

    if (scope_parent)
    {
        (*parent_map)[this] = scope_parent;
        (*children_map)[scope_parent].push_back(this);
    }
    else
        (*top_scope).push_back(this);

    list->push_back(this);
}

void compound_identity::doInit(Core *) const
{
}

const std::string compound_identity::getFullName() const
{
    if (scope_parent)
        return scope_parent->getFullName() + "." + getName();
    else
        return getName();
}

static std::mutex *known_mutex = NULL;

void compound_identity::Init(Core *core)
{
    if (!known_mutex)
        known_mutex = new std::mutex();

    // do any late initialization required for compound identities

    for (auto ci : *list)
        ci->doInit(core);
}

bitfield_identity::bitfield_identity(size_t size,
                                     const compound_identity *scope_parent, const char *dfhack_name,
                                     int num_bits, const bitfield_item_info *bits)
    : compound_identity(size, NULL, scope_parent, dfhack_name), bits(bits), num_bits(num_bits)
{
}

enum_identity::enum_identity(size_t size,
    const compound_identity *scope_parent, const char *dfhack_name,
    const type_identity *base_type,
                             int64_t first_item_value, int64_t last_item_value,
                             const char *const *keys,
                             const ComplexData *complex,
                             const void *attrs, const struct_identity *attr_type)
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

enum_identity::enum_identity(const enum_identity *base_enum, const type_identity *override_base_type)
    : enum_identity(override_base_type->byte_size(), base_enum->getScopeParent(),
                    base_enum->getName(), override_base_type, base_enum->first_item_value,
                    base_enum->last_item_value, base_enum->keys, base_enum->complex,
                    base_enum->attrs, base_enum->attr_type)
{
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

/****** STRUCT IDENTITIES ******/

decltype(struct_identity::parent_map) struct_identity::parent_map = nullptr;
decltype(struct_identity::children_map) struct_identity::children_map = nullptr;

void struct_identity::ensure_struct_identity_init()
{
    static std::once_flag struct_identity_init_flag;
    std::call_once(struct_identity_init_flag, []() {
        parent_map = new (std::remove_pointer<decltype(parent_map)>::type)();
        children_map = new (std::remove_pointer<decltype(children_map)>::type)();
        });
}



struct_identity::struct_identity(size_t size, TAllocateFn alloc,
    const compound_identity *scope_parent, const char *dfhack_name,
    const struct_identity *parent, const struct_field_info *fields)
    : compound_identity(size, alloc, scope_parent, dfhack_name),
      fields(fields)
{
    ensure_struct_identity_init();
    if (parent)
    {
        (*parent_map)[this] = parent;
        (*children_map)[parent].push_back(this);
    }
}

void struct_identity::doInit(Core *core) const
{
    compound_identity::doInit(core);
}

bool struct_identity::is_subclass(const struct_identity *actual) const
{
    if (!hasChildren() && actual != this)
        return false;

    for (; actual; actual = actual->getParent())
        if (actual == this) return true;

    return false;
}

const std::string pointer_identity::getFullName() const
{
    return (target ? target->getFullName() : std::string("void")) + "*";
}

const std::string container_identity::getFullName(const type_identity *item) const
{
    return '<' + (item ? item->getFullName() : std::string("void")) + '>';
}

const std::string ptr_container_identity::getFullName(const type_identity *item) const
{
    return '<' + (item ? item->getFullName() : std::string("void")) + std::string("*>");
}

const std::string bit_container_identity::getFullName(const type_identity *) const
{
    return "<bool>";
}

const std::string df::buffer_container_identity::getFullName(const type_identity *item) const
{
    return (item ? item->getFullName() : std::string("void")) +
           (size > 0 ? stl_sprintf("[%d]", size) : std::string("[]"));
}

union_identity::union_identity(size_t size, const TAllocateFn alloc,
        const compound_identity *scope_parent, const char *dfhack_name,
        const struct_identity *parent, const struct_field_info *fields)
    : struct_identity(size, alloc, scope_parent, dfhack_name, parent, fields)
{
}

/****** VIRTUAL IDENTITIES ******/

decltype(virtual_identity::name_lookup) virtual_identity::name_lookup = nullptr;
decltype(virtual_identity::known) virtual_identity::known = nullptr;
decltype(virtual_identity::vtable_ptr_map) virtual_identity::vtable_ptr_map = nullptr;
decltype(virtual_identity::interpose_list_map) virtual_identity::interpose_list_map = nullptr;

void virtual_identity::ensure_virtual_identity_init()
{
    static std::once_flag virtual_identity_init_flag;
    std::call_once(virtual_identity_init_flag, []() {
        name_lookup = new (std::remove_pointer<decltype(name_lookup)>::type)();
        known = new (std::remove_pointer<decltype(known)>::type)();
        vtable_ptr_map = new (std::remove_pointer<decltype(vtable_ptr_map)>::type)();
        interpose_list_map = new (std::remove_pointer<decltype(interpose_list_map)>::type)();
    });
}

virtual_identity::virtual_identity(size_t size, const TAllocateFn alloc,
                                   const char *dfhack_name, const char *original_name,
                                   const virtual_identity *parent, const struct_field_info *fields,
                                   bool is_plugin)
    : struct_identity(size, alloc, NULL, dfhack_name, parent, fields), original_name(original_name),
      is_plugin(is_plugin)
{
    ensure_virtual_identity_init();
    // Plugins are initialized after Init was called, so they need to be added to the name table here
    if (is_plugin)
    {
        doInit(&Core::getInstance());
    }
}

virtual_identity::~virtual_identity()
{
    // Remove interpose entries, so that they don't try accessing this object later
    auto& interpose_list = (*interpose_list_map)[this];
    for (auto it = interpose_list.begin(); it != interpose_list.end(); ++it)
        if (it->second)
            it->second->on_host_delete(this);
    interpose_list.clear();

    // Remove global lookup table entries if we're from a plugin
    if (is_plugin)
    {
        (*name_lookup).erase(getOriginalName());

        if (vtable_ptr())
            (*known).erase(vtable_ptr());
    }
}

void virtual_identity::doInit(Core *core) const
{
    struct_identity::doInit(core);

    auto vtname = getOriginalName();
    (*name_lookup)[vtname] = this;

    auto vtable_ptr = core->vinfo->getVTable(vtname);
    if (vtable_ptr)
    {
        (*known)[vtable_ptr] = this;
        (*vtable_ptr_map)[this] = vtable_ptr;
    }
}

const virtual_identity *virtual_identity::find(std::string_view name)
{
    auto name_it = (*name_lookup).find(name);

    return (name_it != (*name_lookup).end()) ? name_it->second : nullptr;
}

const virtual_identity *virtual_identity::get(virtual_ptr instance_ptr)
{
    if (!instance_ptr) return nullptr;

    return find(get_vtable(instance_ptr));
}

const virtual_identity *virtual_identity::find(void *vtable)
{
    if (!vtable || !known_mutex)
        return nullptr;

    ensure_virtual_identity_init();

    // Actually, a reader/writer lock would be sufficient,
    // since the table is only written once per class.
    std::lock_guard<std::mutex> lock(*known_mutex);

    auto it = (*known).find(vtable);
    if (it != (*known).end())
        return it->second;

    // If using a reader/writer lock, re-grab as write here, and recheck
    Core &core = Core::getInstance();
    std::string name = core.p->doReadClassName(vtable);

    auto name_it = (*name_lookup).find(name);
    if (name_it != (*name_lookup).end()) {
        const virtual_identity *p = name_it->second;

        if (p->vtable_ptr() && p->vtable_ptr() != vtable) {
            std::cerr << "Conflicting vtable ptr for class '" << p->getName()
                      << "': found 0x" << std::hex << uintptr_t(vtable)
                      << ", previous 0x" << uintptr_t(p->vtable_ptr()) << std::dec << std::endl;
            abort();
        } else if (!p->vtable_ptr()) {
            uintptr_t pv = uintptr_t(vtable);
            pv -= Core::getInstance().vinfo->getRebaseDelta();
            std::cerr << "<vtable-address name='" << p->getOriginalName() << "' value='0x"
                      << std::hex << pv << std::dec << "'/>" << std::endl;
        }

        (*known)[vtable] = p;
        (*vtable_ptr_map)[p] = vtable;
        return p;
    }

    if (name.find("dfhack_") == std::string::npos) {
        std::cerr << "INFO: Class not in symbols.xml: '" << name << "': vtable = 0x"
                << std::hex << uintptr_t(vtable) << std::dec << std::endl;
    }

    (*known).erase(vtable);
    return nullptr;
}

void virtual_identity::adjust_vtable(virtual_ptr obj, const virtual_identity *main) const
{
    auto vp = vtable_ptr();
    if (vp) {
        *(void**)obj = vp;
        return;
    }

    if (main && main != this && is_subclass(main))
        return;

    std::cerr << "Attempt to create class '" << getName() << "' without known vtable." << std::endl;
    throw DFHack::Error::VTableMissing(getName());
}

virtual_ptr virtual_identity::clone(virtual_ptr obj)
{
    const virtual_identity *id = get(obj);
    if (!id) return nullptr;
    virtual_ptr copy = id->instantiate();
    if (!copy) return nullptr;
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

static const struct_field_info *find_union_tag_candidate(const struct_identity *structure, const struct_field_info *union_field)
{
    if (union_field->extra && union_field->extra->union_tag_field)
    {
        auto defined_field_name = union_field->extra->union_tag_field;
        for (auto p = structure; p; p = p->getParent())
        {
            for (auto field = p->getFields(); field && field->mode != struct_field_info::END; field++)
            {
                if (!strcmp(field->name, defined_field_name))
                {
                    return field;
                }
            }
        }

        return nullptr;
    }

    std::string name(union_field->name);
    if (name.length() >= 4 && name.substr(name.length() - 4) == "data")
    {
        name.erase(name.length() - 4, 4);
        name += "type";

        for (auto p = structure; p; p = p->getParent())
        {
            for (auto field = p->getFields(); field && field->mode != struct_field_info::END; field++)
            {
                if (field->name == name)
                {
                    return field;
                }
            }
        }
    }

    return nullptr;
}

const struct_field_info *DFHack::find_union_tag(const struct_identity *structure, const struct_field_info *union_field)
{
    CHECK_NULL_POINTER(structure);
    CHECK_NULL_POINTER(union_field);

    auto tag_candidate = find_union_tag_candidate(structure, union_field);

    if (!tag_candidate)
    {
        return nullptr;
    }

    if (union_field->mode == struct_field_info::SUBSTRUCT &&
            union_field->type &&
            union_field->type->type() == IDTYPE_UNION)
    {
        // union field

        if (tag_candidate->mode == struct_field_info::PRIMITIVE &&
                tag_candidate->type &&
                tag_candidate->type->type() == IDTYPE_ENUM)
        {
            return tag_candidate;
        }

        return nullptr;
    }

    if (union_field->mode != struct_field_info::CONTAINER ||
            !union_field->type ||
            union_field->type->type() != IDTYPE_CONTAINER)
    {
        // not a union field or a vector; bail
        return nullptr;
    }

    auto container_type = static_cast<const container_identity *>(union_field->type);
    if (container_type->getFullName(nullptr) != "vector<void>" ||
            !container_type->getItemType() ||
            container_type->getItemType()->type() != IDTYPE_UNION)
    {
        // not a vector of unions
        return nullptr;
    }

    if (tag_candidate->mode != struct_field_info::CONTAINER ||
            !tag_candidate->type ||
            tag_candidate->type->type() != IDTYPE_CONTAINER)
    {
        // candidate is not a vector
        return nullptr;
    }

    auto tag_container_type = static_cast<const container_identity *>(tag_candidate->type);
    if (tag_container_type->getFullName(nullptr) == "vector<void>" &&
            tag_container_type->getItemType() &&
            tag_container_type->getItemType()->type() == IDTYPE_ENUM)
    {
        return tag_candidate;
    }

    auto union_fields = ((struct_identity*)union_field->type)->getFields();
    if (tag_container_type->getFullName() == "vector<bool>" &&
            union_fields[0].mode != struct_field_info::END &&
            union_fields[1].mode != struct_field_info::END &&
            union_fields[2].mode == struct_field_info::END)
    {
        return tag_candidate;
    }

    return nullptr;
}
