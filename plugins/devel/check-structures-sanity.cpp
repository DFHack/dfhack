#include "Console.h"
#include "PluginManager.h"
#include "MemAccess.h"
#include "DataDefs.h"
#include "DataIdentity.h"

#if defined(WIN32) && defined(DFHACK64)
#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <queue>
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

DFhackCExport command_result plugin_init(color_ostream & out, std::vector<PluginCommand> & commands)
{
    commands.push_back(PluginCommand(
        "check-structures-sanity",
        "performs a sanity check on df-structures",
        command,
        false,
        "checks structures to make sure vectors aren't misidentified"
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
    std::queue<ToCheck> queue;
private:
    bool ok;

    bool check_access(const ToCheck &, void *, type_identity *);
    bool check_access(const ToCheck &, void *, type_identity *, size_t);
    bool check_vtable(const ToCheck &, void *, type_identity *);
    void queue_field(ToCheck &&, const struct_field_info *);
    void queue_static_array(const ToCheck &, void *, type_identity *, size_t, bool = false, enum_identity * = nullptr);
    void check_dispatch(const ToCheck &);
    void check_global(const ToCheck &);
    void check_primitive(const ToCheck &);
    void check_stl_string(const ToCheck &);
    void check_pointer(const ToCheck &);
    void check_bitfield(const ToCheck &);
    void check_enum(const ToCheck &);
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
    if (!parameters.empty())
    {
        return CR_WRONG_USAGE;
    }

    CoreSuspender suspend;

    Checker checker(out);

    ToCheck global;
    global.path.push_back("df::global::");
    global.ptr = nullptr;
    global.identity = &df::global::_identity;

    checker.queue.push(std::move(global));

    return checker.check() ? CR_OK : CR_FAILURE;
}

Checker::Checker(color_ostream & out) :
    out(out)
{
    Core::getInstance().p->getMemRanges(mapped);
}

bool Checker::check()
{
    seen_addr.clear();
    ok = true;

    while (!queue.empty())
    {
        ToCheck current = std::move(queue.front());
        queue.pop();

        check_dispatch(current);
    }

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
            queue.push(std::move(item));
            break;
        case struct_field_info::STATIC_STRING:
            // TODO: check static strings?
            break;
        case struct_field_info::POINTER:
            item.temp_identity = std::unique_ptr<df::pointer_identity>(new df::pointer_identity(field->type));
            item.identity = item.temp_identity.get();
            queue.push(std::move(item));
            break;
        case struct_field_info::STATIC_ARRAY:
            queue_static_array(item, item.ptr, field->type, field->count, false, field->eid);
            break;
        case struct_field_info::SUBSTRUCT:
            queue.push(std::move(item));
            break;
        case struct_field_info::CONTAINER:
            queue.push(std::move(item));
            break;
        case struct_field_info::STL_VECTOR_PTR:
            item.temp_identity = std::unique_ptr<df::stl_ptr_vector_identity>(new df::stl_ptr_vector_identity(field->type, field->eid));
            item.identity = item.temp_identity.get();
            queue.push(std::move(item));
            break;
        case struct_field_info::OBJ_METHOD:
        case struct_field_info::CLASS_METHOD:
            // ignore
            break;
    }
}

void Checker::queue_static_array(const ToCheck & array, void *base, type_identity *type, size_t count, bool pointer, enum_identity *ienum)
{
    size_t size = type->byte_size();
    if (pointer)
    {
        size = sizeof(void *);
    }

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
        queue.push(std::move(item));
    }
}

void Checker::check_dispatch(const ToCheck & item)
{
    if (!item.identity)
    {
        return;
    }

    if (reinterpret_cast<uintptr_t>(item.ptr) == UNINIT_PTR)
    {
        // allow uninitialized raw pointers
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
            size_t refcount;
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
    if (capacity < 0)
    {
        FAIL("string capacity is negative (" << capacity << ")");
    }
    else if (capacity < length)
    {
        FAIL("string capacity (" << capacity << ") is less than length (" << length << ")");
    }

    check_access(item, start, item.identity, capacity);
}

void Checker::check_pointer(const ToCheck & item)
{
    if (!check_access(item, item.ptr, item.identity))
    {
        return;
    }

    if (!seen_addr.insert(item.ptr).second)
    {
        return;
    }

    auto identity = static_cast<pointer_identity *>(item.identity);
    queue.push(ToCheck(item, "", *reinterpret_cast<void **>(item.ptr), identity->getTarget()));
}

void Checker::check_bitfield(const ToCheck & item)
{
    // TODO: check bitfields?
}

void Checker::check_enum(const ToCheck & item)
{
    // TODO: check enums?
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

    if (!item_identity && pointer)
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

    if (local_ok && check_access(item, reinterpret_cast<void *>(vector.start), item.identity, length) && item_identity)
    {
        auto ienum = static_cast<enum_identity *>(static_cast<container_identity *>(item.identity)->getIndexEnumType());
        queue_static_array(item, reinterpret_cast<void *>(vector.start), item_identity, ulength / item_size, pointer, ienum);
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
    for (auto identity = static_cast<struct_identity *>(item.identity); identity; identity = identity->getParent())
    {
        auto fields = identity->getFields();
        if (!fields)
        {
            continue;
        }

        for (auto field = fields; field->mode != struct_field_info::END; field++)
        {
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

    ToCheck virtual_item(item, "", item.ptr, virtual_identity::get(reinterpret_cast<virtual_ptr>(item.ptr)));
    check_struct(virtual_item);
}
