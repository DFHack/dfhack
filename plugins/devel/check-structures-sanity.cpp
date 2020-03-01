#include "Console.h"
#include "PluginManager.h"
#include "MemAccess.h"
#include "DataDefs.h"
#include "DataIdentity.h"
#include "LuaTools.h"
#include "LuaWrapper.h"

#include "df/large_integer.h"

#if defined(WIN32) && defined(DFHACK64)
#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <deque>
#include <inttypes.h>
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

static const char *const *get_enum_item_key(enum_identity *identity, int64_t value)
{
    size_t index;
    if (auto cplx = identity->getComplex())
    {
        auto it = cplx->value_index_map.find(value);
        if (it == cplx->value_index_map.cend())
        {
            return nullptr;
        }
        index = it->second;
    }
    else
    {
        if (value < identity->getFirstItem() || value > identity->getLastItem())
        {
            return nullptr;
        }
        index = value - identity->getFirstItem();
    }

    return &identity->getKeys()[index];
}

static const struct_field_info *find_union_tag(const struct_field_info *fields, const struct_field_info *union_field)
{
    if (union_field->mode != struct_field_info::SUBSTRUCT ||
            !union_field->type ||
            union_field->type->type() != IDTYPE_UNION)
    {
        // not a union
        return nullptr;
    }

    const struct_field_info *tag_field = union_field + 1;

    std::string name(union_field->name);
    if (name.length() >= 4 && name.substr(name.length() - 4) == "data")
    {
        name.erase(name.length() - 4, 4);
        name += "type";

        if (tag_field->mode != struct_field_info::END && tag_field->name == name)
        {
            // fast path; we already have the correct field
        }
        else
        {
            for (auto field = fields; field->mode != struct_field_info::END; field++)
            {
                if (field->name == name)
                {
                    tag_field = field;
                    break;
                }
            }
        }
    }
    else if (name.length() > 7 && name.substr(name.length() - 7) == "_target" && fields != union_field && (union_field - 1)->name == name.substr(0, name.length() - 7))
    {
        tag_field = union_field - 1;
    }

    if (tag_field->mode != struct_field_info::PRIMITIVE ||
            !tag_field->type ||
            tag_field->type->type() != IDTYPE_ENUM)
    {
        // no tag
        return nullptr;
    }

    return tag_field;
}

static const struct_field_info *find_union_vector_tag_vector(const struct_field_info *fields, const struct_field_info *union_field)
{
    if (union_field->mode != struct_field_info::CONTAINER ||
            !union_field->type ||
            union_field->type->type() != IDTYPE_CONTAINER)
    {
        // not a vector
        return nullptr;
    }

    auto container_type = static_cast<container_identity *>(union_field->type);
    if (container_type->getFullName(nullptr) != "vector<void>" ||
            !container_type->getItemType() ||
            container_type->getItemType()->type() != IDTYPE_UNION)
    {
        // not a union
        return nullptr;
    }

    const struct_field_info *tag_field = union_field + 1;

    std::string name(union_field->name);
    if (name.length() >= 4 && name.substr(name.length() - 4) == "data")
    {
        name.erase(name.length() - 4, 4);
        name += "type";

        if (tag_field->mode != struct_field_info::END && tag_field->name == name)
        {
            // fast path; we already have the correct field
        }
        else
        {
            for (auto field = fields; field->mode != struct_field_info::END; field++)
            {
                if (field->name == name)
                {
                    tag_field = field;
                    break;
                }
            }
        }
    }
    else if (name.length() > 7 && name.substr(name.length() - 7) == "_target" && fields != union_field && (union_field - 1)->name == name.substr(0, name.length() - 7))
    {
        tag_field = union_field - 1;
    }

    if (tag_field->mode != struct_field_info::CONTAINER ||
            !tag_field->type ||
            tag_field->type->type() != IDTYPE_CONTAINER)
    {
        // no tag vector
        return nullptr;
    }

    auto tag_container_type = static_cast<container_identity *>(tag_field->type);
    if (tag_container_type->getFullName(nullptr) != "vector<void>" ||
            !tag_container_type->getItemType() ||
            tag_container_type->getItemType()->type() != IDTYPE_ENUM)
    {
        // not an enum
        return nullptr;
    }

    return tag_field;
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

#ifndef WIN32
    // this function doesn't make sense on windows, where std::string is not pointer-sized.
    const std::string *check_possible_stl_string_pointer(const void *const*);
#endif
    bool check_access(const ToCheck &, void *, type_identity *);
    bool check_access(const ToCheck &, void *, type_identity *, size_t);
    bool check_vtable(const ToCheck &, void *, type_identity *);
    void queue_field(ToCheck &&, const struct_field_info *);
    void queue_static_array(const ToCheck &, void *, type_identity *, size_t, bool = false, enum_identity * = nullptr);
    bool maybe_queue_union(const ToCheck &, const struct_field_info *, const struct_field_info *);
    void queue_union(const ToCheck &, const ToCheck &);
    void queue_union_vector(const ToCheck &, const ToCheck &);
    void check_dispatch(ToCheck &);
    void check_global(const ToCheck &);
    void check_primitive(const ToCheck &);
    void check_stl_string(const ToCheck &);
    void check_pointer(const ToCheck &);
    void check_bitfield(const ToCheck &);
    int64_t check_enum(const ToCheck &);
    void check_container(const ToCheck &);
    size_t check_vector_size(const ToCheck &, size_t);
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

#ifndef WIN32
const std::string *Checker::check_possible_stl_string_pointer(const void *const*base)
{
    std::string empty_string;
    if (*base == *reinterpret_cast<void **>(&empty_string))
    {
        return reinterpret_cast<const std::string *>(base);
    }

    const struct string_data_inner
    {
        size_t length;
        size_t capacity;
        int32_t refcount;
    } *str_data = static_cast<const string_data_inner *>(*base) - 1;

    uint32_t tag = *reinterpret_cast<const uint32_t *>(PTR_ADD(str_data, -8));
    if (tag == 0xdfdf4ac8)
    {
        size_t allocated_size = *reinterpret_cast<const size_t *>(PTR_ADD(str_data, -16));
        size_t expected_size = sizeof(*str_data) + str_data->capacity + 1;

        if (allocated_size != expected_size)
        {
            return nullptr;
        }
    }
    else
    {
        return nullptr;
    }

    if (str_data->capacity < str_data->length)
    {
        return nullptr;
    }

    const char *ptr = reinterpret_cast<const char *>(*base);
    for (size_t i = 0; i < str_data->length; i++)
    {
        if (!*ptr++)
        {
            return nullptr;
        }
    }

    if (*ptr++)
    {
        return nullptr;
    }

    return reinterpret_cast<const std::string *>(base);
}
#endif


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
            // TODO: flags inside field->count
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
            auto pname = get_enum_item_key(ienum, int64_t(i));
            auto name = pname ? *pname : nullptr;

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

bool Checker::maybe_queue_union(const ToCheck & item, const struct_field_info *fields, const struct_field_info *union_field)
{
    auto tag_field = find_union_tag(fields, union_field);
    if (tag_field)
    {
        ToCheck union_item(item, "." + std::string(union_field->name), PTR_ADD(item.ptr, union_field->offset), union_field->type);
        ToCheck tag_item(item, "." + std::string(tag_field->name), PTR_ADD(item.ptr, tag_field->offset), tag_field->type);
        queue_union(union_item, tag_item);

        return true;
    }

    tag_field = find_union_vector_tag_vector(fields, union_field);
    if (tag_field)
    {
        ToCheck union_vector_item(item, "." + std::string(union_field->name), PTR_ADD(item.ptr, union_field->offset), union_field->type);
        ToCheck tag_vector_item(item, "." + std::string(tag_field->name), PTR_ADD(item.ptr, tag_field->offset), tag_field->type);

        queue_union_vector(union_vector_item, tag_vector_item);

        return true;
    }

    return false;
}

void Checker::queue_union(const ToCheck & item, const ToCheck & tag_item)
{
    auto union_type = static_cast<union_identity *>(item.identity);
    auto tag_type = static_cast<enum_identity *>(tag_item.identity);

    int64_t tag_value = check_enum(tag_item);

    auto ptag_key = get_enum_item_key(tag_type, tag_value);
    auto tag_key = ptag_key ? *ptag_key : nullptr;
    if (!ptag_key)
    {
        FAIL("tagged union tag (" << join_strings("", tag_item.path) << ") out of range (" << tag_value << ")");
    }
    else if (!tag_key)
    {
        FAIL("tagged union tag (" << join_strings("", tag_item.path) << ") unnamed (" << tag_value << ")");
    }

    const struct_field_info *item_field = nullptr;
    if (tag_key)
    {
        for (auto field = union_type->getFields(); field->mode != struct_field_info::END; field++)
        {
            if (!strcmp(tag_key, field->name))
            {
                item_field = field;
                break;
            }
        }
    }

    if (item_field)
    {
        // good to go
        ToCheck tagged_union_item(item, "." + std::string(item_field->name), item.ptr, item_field->type);
        queue_field(std::move(tagged_union_item), item_field);
        return;
    }

    // if it's all uninitialized, ignore it
    uint8_t uninit_value = *reinterpret_cast<const uint8_t *>(item.ptr);
    bool all_uninitialized = uninit_value == 0 || uninit_value == 0xd2;
    if (all_uninitialized)
    {
        for (size_t offset = 0; offset < union_type->byte_size(); offset++)
        {
            if (*reinterpret_cast<const uint8_t *>(PTR_ADD(item.ptr, offset)) != uninit_value)
            {
                all_uninitialized = false;
                break;
            }
        }
    }
    if (all_uninitialized)
    {
        return;
    }

    // if we don't know the key, we already warned above
    if (tag_key)
    {
        FAIL("tagged union (" << join_strings("", tag_item.path) << ") missing member for tag " << tag_key << " (" << tag_value << ")");
    }

    // if there's a pointer (we only check the first field for now)
    // assume this could also be a pointer
    if (union_type->getFields()->mode == struct_field_info::POINTER)
    {
        ToCheck untagged_union_item(item, tag_key ? "." + std::string(tag_key) : stl_sprintf(".?%" PRId64 "?", tag_value), item.ptr, df::identity_traits<void *>::get());
        queue.push_back(std::move(untagged_union_item));
    }
}

void Checker::queue_union_vector(const ToCheck & item, const ToCheck & tag_item)
{
    auto union_type = static_cast<union_identity *>(static_cast<container_identity *>(item.identity)->getItemType());
    auto tag_type = static_cast<enum_identity *>(static_cast<container_identity *>(tag_item.identity)->getItemType());

    auto union_count = check_vector_size(item, union_type->byte_size());
    auto tag_count = check_vector_size(tag_item, tag_type->byte_size());

    if (union_count != tag_count)
    {
        FAIL("tagged union vector size (" << union_count << ") does not match tag vector (" << join_strings("", tag_item.path) << ") size (" << tag_count << ")");
    }

    auto union_base = *reinterpret_cast<void **>(item.ptr);
    auto tag_base = *reinterpret_cast<void **>(tag_item.ptr);

    auto count = std::min(union_count, tag_count);
    for (size_t i = 0; i < count; i++, union_base = PTR_ADD(union_base, union_type->byte_size()), tag_base = PTR_ADD(tag_base, tag_type->byte_size()))
    {
        ToCheck union_item(item, i, union_base, union_type);
        ToCheck tag(tag_item, i, tag_base, tag_type);
        queue_union(union_item, tag);
    }
}

void Checker::check_dispatch(ToCheck & item)
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

                // check recursively if it might be a valid pointer
                if (allocated_size == sizeof(void *))
                {
                    item.path.push_back(".?ptr?");
                    item.path.push_back("");
                    item.identity = df::identity_traits<void *>::get();
                }
            }
#ifndef WIN32
            else if (auto str = check_possible_stl_string_pointer(&item.ptr))
            {
                FAIL("untyped pointer is actually stl-string with value \"" << *str << "\" (length " << str->length() << ")");
            }
#endif
            else
            {
                FAIL("pointer to memory with no size information");
            }
        }

        // could have been set above
        if (!item.identity)
        {
            return;
        }
    }

    if (!check_access(item, item.ptr, item.identity) && item.identity->type() != IDTYPE_GLOBAL)
    {
        return;
    }

    // special case for large_integer weirdness
    if (item.identity == df::identity_traits<df::large_integer>::get())
    {
        item.identity = df::identity_traits<int64_t>::get();
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
        case IDTYPE_UNION:
            FAIL("untagged union");
            check_struct(item);
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
        else
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

    auto key = get_enum_item_key(identity, value);
    if (!key)
    {
        if (identity->getComplex())
        {
            FAIL("enum value (" << value << ") is not defined (complex enum)");
        }
        else
        {
            FAIL("enum value (" << value << ") outside of defined range (" << identity->getFirstItem() << " to " << identity->getLastItem() << ")");
        }
    }
    else if (!*key)
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

size_t Checker::check_vector_size(const ToCheck & item, size_t item_size)
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
        return 0;
    }

    vector_data vector = *reinterpret_cast<vector_data *>(item.ptr);

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

    if (!item_size)
    {
        return 0;
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

    if (local_ok && capacity && !vector.start)
    {
        FAIL("vector has null pointer but capacity " << (capacity / item_size));
        return 0;
    }

    if (!check_access(item, reinterpret_cast<void *>(vector.start), item.identity, capacity))
    {
        return 0;
    }

    return local_ok ? ulength / item_size : 0;
}

void Checker::check_vector(const ToCheck & item, type_identity *item_identity, bool pointer)
{
    size_t item_size = pointer ? sizeof(void *) : item_identity->byte_size();

    if (!item_identity && pointer && !sizes)
    {
        // non-identified vector type in structures
        item_size = 0;
    }

    size_t count = check_vector_size(item, item_size);

    if (item.path.back() == ".bad" || count == 0)
    {
        // don't check contents
        return;
    }

    void *start = *reinterpret_cast<void **>(item.ptr);
    auto ienum = static_cast<enum_identity *>(static_cast<container_identity *>(item.identity)->getIndexEnumType());
    queue_static_array(item, start, item_identity, count, pointer, ienum);
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
        else
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
            if (maybe_queue_union(item, fields, field))
            {
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
