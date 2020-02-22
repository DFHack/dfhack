#include "Console.h"
#include "PluginManager.h"
#include "MemAccess.h"
#include "DataDefs.h"
#include "DataIdentity.h"
#include "LuaTools.h"
#include "LuaWrapper.h"

#if defined(WIN32) && defined(DFHACK64)
#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <deque>
#include <set>
#include <typeinfo>

using namespace DFHack;

DFHACK_PLUGIN("check-structures-sanity");

static command_result command(color_ostream &, std::vector<std::string> &);

#ifdef WIN32
#define UNEXPECTED __debugbreak()
#else
#define UNEXPECTED __asm__ volatile ("int $0x03")
#endif

DFhackCExport command_result plugin_init(color_ostream &, std::vector<PluginCommand> & commands)
{
    commands.push_back(PluginCommand(
        "check-structures-sanity",
        "performs a sanity check on df-structures",
        command,
        false,
        "check-structures-sanity [-enums] [-sizes] [-lowmem] [starting_point]\n"
        "\n"
        "-enums: report unexpected or unnamed enum or bitfield values.\n"
        "-sizes: report struct and class sizes that don't match structures. (requires sizecheck)\n"
        "-lowmem: use depth-first search instead of breadth-first search. uses less memory but may produce less sensible field names.\n"
        "starting_point: a lua expression or a word like 'screen', 'item', or 'building'. (defaults to df.global)\n"
        "\n"
        "by default, check-structures-sanity reports invalid pointers, vectors, strings, and vtables."
    ));
    return CR_OK;
}

struct ToCheck
{
    std::vector<std::string> path;
    void *ptr;
    type_identity *identity;
    std::unique_ptr<type_identity> temp_identity;

    ToCheck()
    {
    }

    ToCheck(const ToCheck & parent, size_t idx, void *ptr, type_identity *identity) :
        ToCheck(parent, stl_sprintf("[%zu]", idx), ptr, identity)
    {
    }

    ToCheck(const ToCheck & parent, const std::string & name, void *ptr, type_identity *identity) :
        path(parent.path.cbegin(), parent.path.cend()),
        ptr(ptr),
        identity(identity)
    {
        path.push_back(name);
    }
};

class Checker
{
    color_ostream & out;
    std::vector<t_memrange> mapped;
    std::set<void *> seen_addr;
public:
    std::deque<ToCheck> queue;
    size_t num_checked;
    bool enums;
    bool sizes;
    bool lowmem;
private:
    bool ok;

    bool address_in_runtime_data(void *);
    bool check_access(const ToCheck &, void *, type_identity *);
    bool check_access(const ToCheck &, void *, type_identity *, size_t);
    bool check_vtable(const ToCheck &, void *, type_identity *);
    void queue_field(ToCheck &&, const struct_field_info *);
    void queue_static_array(const ToCheck &, void *, type_identity *, size_t, bool = false, enum_identity * = nullptr);
    bool maybe_queue_tagged_union(const ToCheck &, const struct_field_info *);
    void check_dispatch(const ToCheck &);
    void check_global(const ToCheck &);
    void check_primitive(const ToCheck &);
    void check_stl_string(const ToCheck &);
    void check_pointer(const ToCheck &);
    void check_bitfield(const ToCheck &);
    int64_t check_enum(const ToCheck &);
    void check_container(const ToCheck &);
    void check_vector(const ToCheck &, type_identity *, bool);
    void check_deque(const ToCheck &, type_identity *);
    void check_dfarray(const ToCheck &, type_identity *);
    void check_bitarray(const ToCheck &);
    void check_bitvector(const ToCheck &);
    void check_struct(const ToCheck &);
    void check_virtual(const ToCheck &);
public:
    Checker(color_ostream &);
    bool check();
};

static command_result command(color_ostream & out, std::vector<std::string> & parameters)
{
    CoreSuspender suspend;

    Checker checker(out);

#define BOOL_PARAM(name) \
    auto name ## _idx = std::find(parameters.begin(), parameters.end(), "-" #name); \
    if (name ## _idx != parameters.cend()) \
    { \
        checker.name = true; \
        parameters.erase(name ## _idx); \
    }
    BOOL_PARAM(enums);
    BOOL_PARAM(sizes);
    BOOL_PARAM(lowmem);
#undef BOOL_PARAM

    if (parameters.size() > 1)
    {
        return CR_WRONG_USAGE;
    }

    if (parameters.empty())
    {
        ToCheck global;
        global.path.push_back("df.global.");
        global.ptr = nullptr;
        global.identity = &df::global::_identity;

        checker.queue.push_back(std::move(global));
    }
    else
    {
        using namespace DFHack::Lua;
        using namespace DFHack::Lua::Core;
        using namespace DFHack::LuaWrapper;

        StackUnwinder unwinder(State);
        PushModulePublic(out, "utils", "df_expr_to_ref");
        Push(parameters.at(0));
        if (!SafeCall(out, 1, 1))
        {
            return CR_FAILURE;
        }
        if (!lua_touserdata(State, -1))
        {
            return CR_WRONG_USAGE;
        }

        ToCheck ref;
        ref.path.push_back(parameters.at(0));
        ref.path.push_back(""); // tell check_struct that it is a pointer
        ref.ptr = get_object_ref(State, -1);
        lua_getfield(State, -1, "_type");
        lua_getfield(State, -1, "_identity");
        ref.identity = reinterpret_cast<type_identity *>(lua_touserdata(State, -1));
        if (!ref.identity)
        {
            out.printerr("could not determine type identity\n");
            return CR_FAILURE;
        }

        checker.queue.push_back(std::move(ref));
    }

    return checker.check() ? CR_OK : CR_FAILURE;
}

Checker::Checker(color_ostream & out) :
    out(out)
{
    Core::getInstance().p->getMemRanges(mapped);
    enums = false;
    sizes = false;
    lowmem = false;
}

bool Checker::check()
{
    seen_addr.clear();
    num_checked = 0;
    ok = true;

    while (!queue.empty())
    {
        ToCheck current;
        if (lowmem)
        {
            current = std::move(queue.back());
            queue.pop_back();
        }
        else
        {
            current = std::move(queue.front());
            queue.pop_front();
        }

        check_dispatch(current);

        num_checked++;
        if (out.is_console() && num_checked % 1000 == 0)
        {
            out << "checked " << num_checked << " fields\r" << std::flush;
        }
    }

    out << "checked " << num_checked << " fields" << std::endl;

    return ok;
}

#define FAIL(message) \
    do \
    { \
        ok = false; \
        out << COLOR_LIGHTRED << "sanity check failed (line " << __LINE__ << "): "; \
        out << COLOR_RESET << (item.identity ? item.identity->getFullName() : "?") << " (accessed as "; \
        for (auto & p : item.path) { out << p; } \
        out << "): "; \
        out << COLOR_YELLOW << message; \
        out << COLOR_RESET << std::endl; \
    } while (false)

#define PTR_ADD(base, offset) (reinterpret_cast<void *>(reinterpret_cast<uintptr_t>((base)) + static_cast<ptrdiff_t>((offset))))

bool Checker::address_in_runtime_data(void *ptr)
{
    for (auto & range : mapped)
    {
        if (!range.isInRange(ptr))
        {
            continue;
        }

#ifdef WIN32
        // TODO: figure out how to differentiate statically-allocated pages
        // from malloc'd data pages
        UNEXPECTED;
        return false;
#else
        return !strcmp(range.name, "[heap]");
#endif
    }

    return false;
}

bool Checker::check_access(const ToCheck & item, void *base, type_identity *identity)
{
    return check_access(item, base, identity, identity ? identity->byte_size() : 0);
}

bool Checker::check_access(const ToCheck & item, void *base, type_identity *identity, size_t size)
{
    if (!base)
    {
        // null pointer: can't access, but not an error
        return false;
    }

    // assumes MALLOC_PERTURB_=45
#ifdef DFHACK64
#define UNINIT_PTR 0xd2d2d2d2d2d2d2d2
#define FAIL_PTR(message) FAIL(stl_sprintf("0x%016zx: ", reinterpret_cast<uintptr_t>(base)) << message)
#else
#define UNINIT_PTR 0xd2d2d2d2
#define FAIL_PTR(message) FAIL(stl_sprintf("0x%08zx: ", reinterpret_cast<uintptr_t>(base)) << message)
#endif
    if (reinterpret_cast<uintptr_t>(base) == UNINIT_PTR)
    {
        FAIL_PTR("uninitialized pointer");
        return false;
    }

    bool found = true;
    void *expected_start = base;
    size_t remaining_size = size;
    while (found)
    {
        found = false;

        for (auto & range : mapped)
        {
            if (!range.isInRange(expected_start))
            {
                continue;
            }

            found = true;

            if (!range.valid || !range.read)
            {
                FAIL_PTR("pointer to invalid memory range");
                return false;
            }

            if (size && !range.isInRange(PTR_ADD(expected_start, remaining_size - 1)))
            {
                void *next_start = PTR_ADD(range.end, 1);
                remaining_size -= reinterpret_cast<ptrdiff_t>(next_start) - reinterpret_cast<ptrdiff_t>(expected_start);
                expected_start = next_start;
                break;
            }

            return true;
        }
    }

    if (expected_start == base)
    {
        FAIL_PTR("pointer not in any mapped range");
    }
    else
    {
        FAIL_PTR("pointer exceeds mapped memory bounds (size " << size << ")");
    }
    return false;
#undef FAIL_PTR
}

bool Checker::check_vtable(const ToCheck & item, void *vtable, type_identity *identity)
{
    if (!check_access(item, PTR_ADD(vtable, -ptrdiff_t(sizeof(void *))), identity, sizeof(void *)))
        return false;
    char **info = *(reinterpret_cast<char ***>(vtable) - 1);

#ifdef WIN32
    if (!check_access(item, PTR_ADD(info, 12), identity, 4))
        return false;

#ifdef DFHACK64
    void *base;
    if (!RtlPcToFileHeader(info, &base))
        return false;

    char *typeinfo = reinterpret_cast<char *>(base) + reinterpret_cast<int32_t *>(info)[3];
    char *name = typeinfo + 16;
#else
    char *name = reinterpret_cast<char *>(info) + 8;
#endif
#else
    if (!check_access(item, info + 1, identity, sizeof(void *)))
        return false;
    char *name = *(info + 1);
#endif

    for (auto & range : mapped)
    {
        if (!range.isInRange(name))
        {
            continue;
        }

        if (!range.valid || !range.read)
        {
            FAIL("pointer to invalid memory range");
            return false;
        }

        bool letter = false;
        for (char *p = name; ; p++)
        {
            if (!range.isInRange(p))
            {
                return false;
            }

            if (*p >= 'a' && *p <= 'z')
            {
                letter = true;
            }
            else if (!*p)
            {
                return letter;
            }
        }
    }

    return false;
}

void Checker::queue_field(ToCheck && item, const struct_field_info *field)
{
    switch (field->mode)
    {
        case struct_field_info::END:
            UNEXPECTED;
            break;
        case struct_field_info::PRIMITIVE:
            queue.push_back(std::move(item));
            break;
        case struct_field_info::STATIC_STRING:
            // TODO: check static strings?
            break;
        case struct_field_info::POINTER:
            item.temp_identity = std::unique_ptr<df::pointer_identity>(new df::pointer_identity(field->type));
            item.identity = item.temp_identity.get();
            queue.push_back(std::move(item));
            break;
        case struct_field_info::STATIC_ARRAY:
            queue_static_array(item, item.ptr, field->type, field->count, false, field->eid);
            break;
        case struct_field_info::SUBSTRUCT:
            queue.push_back(std::move(item));
            break;
        case struct_field_info::CONTAINER:
            queue.push_back(std::move(item));
            break;
        case struct_field_info::STL_VECTOR_PTR:
            item.temp_identity = std::unique_ptr<df::stl_ptr_vector_identity>(new df::stl_ptr_vector_identity(field->type, field->eid));
            item.identity = item.temp_identity.get();
            queue.push_back(std::move(item));
            break;
        case struct_field_info::OBJ_METHOD:
        case struct_field_info::CLASS_METHOD:
            // ignore
            break;
    }
}

void Checker::queue_static_array(const ToCheck & array, void *base, type_identity *type, size_t count, bool pointer, enum_identity *ienum)
{
    size_t size = pointer ? sizeof(void *) : type->byte_size();

    for (size_t i = 0; i < count; i++, base = PTR_ADD(base, size))
    {
        ToCheck item(array, i, base, type);
        if (ienum)
        {
            const char *name = nullptr;
            if (auto cplx = ienum->getComplex())
            {
                auto it = cplx->value_index_map.find(int64_t(i));
                if (it != cplx->value_index_map.end())
                {
                    name = ienum->getKeys()[it->second];
                }
            }
            else if (int64_t(i) >= ienum->getFirstItem() && int64_t(i) <= ienum->getLastItem())
            {
                name = ienum->getKeys()[int64_t(i) - ienum->getFirstItem()];
            }

            std::ostringstream str;
            str << "[" << ienum->getFullName() << "::";
            if (name)
            {
                str << name;
            }
            else
            {
                str << "?" << i << "?";
            }
            str << "]";

            item.path.back() = str.str();
        }
        if (pointer)
        {
            item.temp_identity = std::unique_ptr<pointer_identity>(new pointer_identity(type));
            item.identity = item.temp_identity.get();
        }
        queue.push_back(std::move(item));
    }
}

bool Checker::maybe_queue_tagged_union(const ToCheck & item, const struct_field_info *field)
{
    const struct_field_info *tag_field = field + 1;
    if (field->mode != struct_field_info::SUBSTRUCT || tag_field->mode != struct_field_info::PRIMITIVE)
    {
        return false;
    }

    if (!tag_field->type || tag_field->type->type() != IDTYPE_ENUM)
    {
        return false;
    }

    auto tag_identity = static_cast<enum_identity *>(tag_field->type);

    if (!field->type || field->type->type() != IDTYPE_STRUCT)
    {
        return false;
    }

    auto union_identity = static_cast<struct_identity *>(field->type);

    if (!union_identity->getFields() || union_identity->getFields()->mode != struct_field_info::POINTER)
    {
        return false;
    }

    for (auto union_member = union_identity->getFields(); union_member->mode != struct_field_info::END; union_member++)
    {
        // count = 2 means we're in a union
        if (union_member->mode != struct_field_info::POINTER || union_member->count != 2 || union_member->offset)
        {
            return false;
        }
    }

    // unsupported: tagged union with complex enum
    if (tag_identity->getComplex())
    {
        return false;
    }

    // at this point, we're committed to it being a tagged union.

    ToCheck tag_item(item, "." + std::string(tag_field->name), PTR_ADD(item.ptr, tag_field->offset), tag_field->type);
    int64_t tag_value = check_enum(tag_item);
    if (tag_value < tag_identity->getFirstItem() || tag_value > tag_identity->getLastItem())
    {
        FAIL("tagged union (" << field->name << ", " << tag_field->name << ") tag out of range (" << tag_value << ")");
        return true;
    }

    const char *tag_key = tag_identity->getKeys()[tag_value - tag_identity->getFirstItem()];
    if (!tag_key)
    {
        FAIL("tagged union (" << field->name << ", " << tag_field->name << ") tag unnamed (" << tag_value << ")");
        return true;
    }

    const struct_field_info *union_field = nullptr;
    for (auto union_member = union_identity->getFields(); union_member->mode != struct_field_info::END; union_member++)
    {
        if (!strcmp(tag_key, union_member->name))
        {
            union_field = union_member;
            break;
        }
    }

    if (!union_field)
    {
        FAIL("tagged union (" << field->name << ", " << tag_field->name << ") missing member for tag " << tag_key << " (" << tag_value << ")");
        return true;
    }

    ToCheck tagged_union_item(item, stl_sprintf(".%s.%s", field->name, union_field->name), PTR_ADD(item.ptr, field->offset), union_field->type);
    queue_field(std::move(tagged_union_item), union_field);

    return true;
}

void Checker::check_dispatch(const ToCheck & item)
{
    if (reinterpret_cast<uintptr_t>(item.ptr) == UNINIT_PTR)
    {
        // allow uninitialized raw pointers
        return;
    }

    if (!item.identity)
    {
        // warn about bad pointers
        if (!check_access(item, item.ptr, df::identity_traits<void *>::get(), 1))
        {
            return;
        }

        if (sizes)
        {
            uint32_t tag = *reinterpret_cast<uint32_t *>(PTR_ADD(item.ptr, -8));
            if (tag == 0xdfdf4ac8)
            {
                size_t allocated_size = *reinterpret_cast<size_t *>(PTR_ADD(item.ptr, -16));

                FAIL("pointer to a block of " << allocated_size << " bytes of allocated memory");
            }
        }

        return;
    }

    if (!check_access(item, item.ptr, item.identity) && item.identity->type() != IDTYPE_GLOBAL)
    {
        return;
    }

    switch (item.identity->type())
    {
        case IDTYPE_GLOBAL:
            check_global(item);
            break;
        case IDTYPE_FUNCTION:
            // don't check functions
            break;
        case IDTYPE_PRIMITIVE:
            check_primitive(item);
            break;
        case IDTYPE_POINTER:
            check_pointer(item);
            break;
        case IDTYPE_CONTAINER:
        case IDTYPE_PTR_CONTAINER:
        case IDTYPE_BIT_CONTAINER:
        case IDTYPE_STL_PTR_VECTOR:
            check_container(item);
            break;
        case IDTYPE_BUFFER:
            {
                auto item_identity = static_cast<container_identity *>(item.identity)->getItemType();
                auto ienum = static_cast<enum_identity *>(static_cast<container_identity *>(item.identity)->getIndexEnumType());
                queue_static_array(item, item.ptr, item_identity, item.identity->byte_size() / item_identity->byte_size(), false, ienum);
            }
            break;
        case IDTYPE_BITFIELD:
            check_bitfield(item);
            break;
        case IDTYPE_ENUM:
            check_enum(item);
            break;
        case IDTYPE_STRUCT:
            check_struct(item);
            break;
        case IDTYPE_CLASS:
            check_virtual(item);
            break;
        case IDTYPE_OPAQUE:
            // can't check opaque
            break;
    }
}

void Checker::check_global(const ToCheck & globals)
{
    auto identity = static_cast<global_identity *>(globals.identity);

    for (auto field = identity->getFields(); field->mode != struct_field_info::END; field++)
    {
        ToCheck item(globals, field->name, nullptr, field->type);
        item.path.push_back(""); // tell check_struct that this is a pointer

        auto base = reinterpret_cast<void **>(field->offset);
        if (!check_access(item, base, df::identity_traits<void *>::get()))
        {
            continue;
        }

        item.ptr = *base;

        if (!seen_addr.insert(item.ptr).second)
        {
            continue;
        }

        queue_field(std::move(item), field);
    }
}

void Checker::check_primitive(const ToCheck & item)
{
    if (item.identity->getFullName() == "string")
    {
        check_stl_string(item);
        return;
    }

    if (item.identity->getFullName() == "bool")
    {
        auto value = *reinterpret_cast<uint8_t *>(item.ptr);
        if (value > 1 && value != 0xd2)
        {
            FAIL("invalid boolean value " << stl_sprintf("%d (0x%02x)", value, value));
        }
        return;
    }

    // TODO: check other primitives?
}

void Checker::check_stl_string(const ToCheck & item)
{
    if (!seen_addr.insert(item.ptr).second)
    {
        return;
    }

    if (!check_access(item, item.ptr, item.identity))
    {
        return;
    }

#ifdef WIN32
    struct string_data
    {
        union
        {
            uintptr_t start;
            char local_data[16];
        };
        size_t length;
        size_t capacity;
    };
#else
    struct string_data
    {
        struct string_data_inner
        {
            size_t length;
            size_t capacity;
            int32_t refcount;
        } *ptr;
    };
#endif

    if (item.identity->byte_size() != sizeof(string_data))
    {
        UNEXPECTED;
        return;
    }

    auto string = reinterpret_cast<string_data *>(item.ptr);
#ifdef WIN32
    bool is_local = string->capacity < 16;
    char *start = is_local ? &string->local_data[0] : reinterpret_cast<char *>(string->start);
    ptrdiff_t length = string->length;
    ptrdiff_t capacity = string->capacity;
#else
    if (!check_access(item, string->ptr, item.identity, 1))
    {
        // nullptr is NOT okay here
        FAIL("invalid string pointer " << stl_sprintf("%p", string->ptr));
        return;
    }
    if (!check_access(item, string->ptr - 1, item.identity, sizeof(*string->ptr)))
    {
        return;
    }
    char *start = reinterpret_cast<char *>(string->ptr);
    ptrdiff_t length = (string->ptr - 1)->length;
    ptrdiff_t capacity = (string->ptr - 1)->capacity;
#endif

    if (length < 0)
    {
        FAIL("string length is negative (" << length << ")");
    }
    else if (capacity < 0)
    {
        FAIL("string capacity is negative (" << capacity << ")");
    }
    else if (capacity < length)
    {
        FAIL("string capacity (" << capacity << ") is less than length (" << length << ")");
    }

#ifndef WIN32
    const std::string empty_string;
    auto empty_string_data = reinterpret_cast<const string_data *>(&empty_string);
    if (sizes && string->ptr != empty_string_data->ptr)
    {
        uint32_t tag = *reinterpret_cast<uint32_t *>(PTR_ADD(string->ptr - 1, -8));
        if (tag == 0xdfdf4ac8)
        {
            size_t allocated_size = *reinterpret_cast<size_t *>(PTR_ADD(string->ptr - 1, -16));
            size_t expected_size = sizeof(*string->ptr) + capacity + 1;

            if (allocated_size != expected_size)
            {
                FAIL("allocated string data size (" << allocated_size << ") does not match expected size (" << expected_size << ")");
            }
        }
        else if (address_in_runtime_data(string->ptr))
        {
            UNEXPECTED;
        }
    }
#endif

    check_access(item, start, item.identity, capacity);
}

void Checker::check_pointer(const ToCheck & item)
{
    if (!seen_addr.insert(item.ptr).second)
    {
        return;
    }

    auto base = *reinterpret_cast<void **>(item.ptr);
    auto base_int = uintptr_t(base);
    if (base_int != UNINIT_PTR && base_int % alignof(void *) != 0)
    {
        FAIL("unaligned pointer " << stl_sprintf("%p", base));
    }

    auto target_identity = static_cast<pointer_identity *>(item.identity)->getTarget();
    queue.push_back(ToCheck(item, "", base, target_identity));
}

void Checker::check_bitfield(const ToCheck & item)
{
    if (!enums)
    {
        return;
    }

    auto identity = static_cast<bitfield_identity *>(item.identity);
    uint64_t val = 0;
    for (size_t offset = 0; offset < identity->byte_size(); offset++)
    {
        val |= uint64_t(*reinterpret_cast<uint8_t *>(PTR_ADD(item.ptr, offset))) << (8 * offset);
    }

    size_t num_bits = identity->getNumBits();
    auto bits = identity->getBits();
    for (size_t i = 0; i < num_bits; i++)
    {
        if (bits[i].size < 0)
            continue;
        if (bits[i].name)
            continue;

        if (!(val & (1ULL << i)))
            continue;

        if (bits[i].size)
        {
            FAIL("bitfield bit " << i << " is unnamed");
        }
        else
        {
            FAIL("bitfield bit " << i << " past the defined end of the bitfield");
        }
    }
}

int64_t Checker::check_enum(const ToCheck & item)
{
    auto identity = static_cast<enum_identity *>(item.identity);

    int64_t value;
    switch (identity->byte_size())
    {
        case 1:
            if (identity->getFirstItem() < 0)
                value = *reinterpret_cast<int8_t *>(item.ptr);
            else
                value = *reinterpret_cast<uint8_t *>(item.ptr);
            break;
        case 2:
            if (identity->getFirstItem() < 0)
                value = *reinterpret_cast<int16_t *>(item.ptr);
            else
                value = *reinterpret_cast<uint16_t *>(item.ptr);
            break;
        case 4:
            if (identity->getFirstItem() < 0)
                value = *reinterpret_cast<int32_t *>(item.ptr);
            else
                value = *reinterpret_cast<uint32_t *>(item.ptr);
            break;
        case 8:
            value = *reinterpret_cast<int64_t *>(item.ptr);
            break;
        default:
            UNEXPECTED;
            return -1;
    }

    if (!enums)
    {
        return value;
    }

    size_t index;
    if (auto cplx = identity->getComplex())
    {
        auto it = cplx->value_index_map.find(value);
        if (it == cplx->value_index_map.cend())
        {
            FAIL("enum value (" << value << ") is not defined (complex enum)");
            return value;
        }
        index = it->second;
    }
    else
    {
        if (value < identity->getFirstItem() || value > identity->getLastItem())
        {
            FAIL("enum value (" << value << ") outside of defined range (" << identity->getFirstItem() << " to " << identity->getLastItem() << ")");
            return value;
        }
        index = value - identity->getFirstItem();
    }

    if (!identity->getKeys()[index])
    {
        FAIL("enum value (" << value << ") is unnamed");
    }

    return value;
}

void Checker::check_container(const ToCheck & item)
{
    auto identity = static_cast<container_identity *>(item.identity);

    if (!seen_addr.insert(item.ptr).second)
    {
        return;
    }

    auto void_name = identity->getFullName(nullptr);
    if (void_name == "vector<void>")
    {
        check_vector(item, identity->getItemType(), false);
    }
    else if (void_name == "vector<void*>")
    {
        check_vector(item, identity->getItemType(), true);
    }
    else if (void_name == "deque<void>")
    {
        check_deque(item, identity->getItemType());
    }
    else if (void_name == "DfArray<void>")
    {
        check_dfarray(item, identity->getItemType());
    }
    else if (void_name == "BitArray<>")
    {
        check_bitarray(item);
    }
    else if (void_name == "vector<bool>")
    {
        check_bitvector(item);
    }
    else
    {
        FAIL("TODO: " << void_name);
        UNEXPECTED;
    }
}

void Checker::check_vector(const ToCheck & item, type_identity *item_identity, bool pointer)
{
    struct vector_data
    {
        uintptr_t start;
        uintptr_t finish;
        uintptr_t end_of_storage;
    };

    if (item.identity->byte_size() != sizeof(vector_data))
    {
        UNEXPECTED;
        return;
    }

    vector_data vector = *reinterpret_cast<vector_data *>(item.ptr);

    size_t item_size = pointer ? sizeof(void *) : item_identity->byte_size();

    ptrdiff_t length = vector.finish - vector.start;
    ptrdiff_t capacity = vector.end_of_storage - vector.start;

    bool local_ok = true;
    if (vector.start > vector.finish)
    {
        local_ok = false;
        FAIL("vector length is negative (" << (length / ptrdiff_t(item_size)) << ")");
    }
    if (vector.start > vector.end_of_storage)
    {
        local_ok = false;
        FAIL("vector capacity is negative (" << (capacity / ptrdiff_t(item_size)) << ")");
    }
    else if (vector.finish > vector.end_of_storage)
    {
        local_ok = false;
        FAIL("vector capacity (" << (capacity / ptrdiff_t(item_size)) << ") is less than its length (" << (length / ptrdiff_t(item_size)) << ")");
    }

    if (!item_identity && pointer && !sizes)
    {
        // non-identified vector type in structures
        return;
    }

    size_t ulength = size_t(length);
    size_t ucapacity = size_t(capacity);
    if (ulength % item_size != 0)
    {
        local_ok = false;
        FAIL("vector length is non-integer (" << (ulength / item_size) << " items plus " << (ulength % item_size) << " bytes)");
    }
    if (ucapacity % item_size != 0)
    {
        local_ok = false;
        FAIL("vector capacity is non-integer (" << (ucapacity / item_size) << " items plus " << (ucapacity % item_size) << " bytes)");
    }

    if (item.path.back() == ".bad")
    {
        // don't check contents
        local_ok = false;
    }

    if (local_ok && check_access(item, reinterpret_cast<void *>(vector.start), item.identity, capacity))
    {
        auto ienum = static_cast<enum_identity *>(static_cast<container_identity *>(item.identity)->getIndexEnumType());
        queue_static_array(item, reinterpret_cast<void *>(vector.start), item_identity, ulength / item_size, pointer, ienum);
    }
    else if (local_ok && capacity && !vector.start)
    {
        FAIL("vector has null pointer but capacity " << (ucapacity / item_size));
    }
}

void Checker::check_deque(const ToCheck & item, type_identity *item_identity)
{
    // TODO: check deque?
}

void Checker::check_dfarray(const ToCheck & item, type_identity *item_identity)
{
    struct dfarray_data
    {
        uintptr_t start;
        unsigned short size;
    };

    if (item.identity->byte_size() != sizeof(dfarray_data))
    {
        UNEXPECTED;
        return;
    }

    dfarray_data dfarray = *reinterpret_cast<dfarray_data *>(item.ptr);

    size_t length = dfarray.size;
    size_t item_size = item_identity->byte_size();

    if (check_access(item, reinterpret_cast<void *>(dfarray.start), item.identity, item_size * length))
    {
        auto ienum = static_cast<enum_identity *>(static_cast<container_identity *>(item.identity)->getIndexEnumType());
        queue_static_array(item, reinterpret_cast<void *>(dfarray.start), item_identity, length, false, ienum);
    }
}

void Checker::check_bitarray(const ToCheck & item)
{
    // TODO: check DFHack::BitArray?
}

void Checker::check_bitvector(const ToCheck & item)
{
    struct biterator_data
    {
        uintptr_t ptr;
        unsigned int offset;
    };

    struct bvector_data
    {
        biterator_data start;
        biterator_data finish;
        uintptr_t end_of_storage;
    };

    if (item.identity->byte_size() != sizeof(bvector_data))
    {
        UNEXPECTED;
        return;
    }

    // TODO: check vector<bool>?
}

void Checker::check_struct(const ToCheck & item)
{
    bool is_pointer = item.path.back().empty();
    bool is_virtual = !item.path.back().empty() && item.path.back().at(0) == '<';
    bool is_virtual_pointer = is_virtual && item.path.size() >= 2 && item.path.at(item.path.size() - 2).empty();
    if (sizes && uintptr_t(item.ptr) % 32 == 16 && (is_pointer || is_virtual_pointer))
    {
        uint32_t tag = *reinterpret_cast<uint32_t *>(PTR_ADD(item.ptr, -8));
        if (tag == 0xdfdf4ac8)
        {
            size_t allocated_size = *reinterpret_cast<size_t *>(PTR_ADD(item.ptr, -16));
            size_t expected_size = item.identity->byte_size();

            if (allocated_size != expected_size)
            {
                FAIL("allocated structure size (" << allocated_size << ") does not match expected size (" << expected_size << ")");
            }
        }
        else if (address_in_runtime_data(item.ptr))
        {
            UNEXPECTED;
        }
    }

    for (auto identity = static_cast<struct_identity *>(item.identity); identity; identity = identity->getParent())
    {
        auto fields = identity->getFields();
        if (!fields)
        {
            continue;
        }

        for (auto field = fields; field->mode != struct_field_info::END; field++)
        {
            if (maybe_queue_tagged_union(item, field))
            {
                field++;
                continue;
            }

            ToCheck child(item, std::string(".") + field->name, PTR_ADD(item.ptr, field->offset), field->type);

            queue_field(std::move(child), field);
        }
    }
}

void Checker::check_virtual(const ToCheck & item)
{
    if (!seen_addr.insert(item.ptr).second)
    {
        return;
    }

    if (!check_access(item, item.ptr, item.identity))
    {
        return;
    }

    auto identity = static_cast<virtual_identity *>(item.identity);

    void *vtable = *reinterpret_cast<void **>(item.ptr);
    if (!check_vtable(item, vtable, identity))
    {
        FAIL("invalid vtable pointer");
        return;
    }
    else if (!identity->is_instance(reinterpret_cast<virtual_ptr>(item.ptr)))
    {
        auto class_name = Core::getInstance().p->readClassName(vtable);
        FAIL("vtable is not a known subclass (subclass is " << class_name << ")");
        return;
    }

    auto vident = virtual_identity::get(reinterpret_cast<virtual_ptr>(item.ptr));
    ToCheck virtual_item(item, "<" + vident->getFullName() + ">", item.ptr, vident);
    check_struct(virtual_item);
}
