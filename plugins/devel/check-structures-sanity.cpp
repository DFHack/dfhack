#include "Console.h"
#include "PluginManager.h"
#include "MemAccess.h"
#include "DataDefs.h"
#include "DataIdentity.h"

#if defined(WIN32) && defined(DFHACK64)
#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#define WIN32_LEAN_AND_MEAN
#include <winnt.h>
#endif

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

class Checker
{
    color_ostream & out;
    std::vector<t_memrange> mapped;
    std::set<void *> seen_addr;
    std::vector<std::string> path;
    bool ok;

    bool check_access(void *, type_identity *);
    bool check_access(void *, type_identity *, size_t);
    bool check_vtable(void *, type_identity *);
    void check_global(global_identity *);
    void check_fields(void *, const struct_field_info *);
    void check_field(void *, const struct_field_info *);
    void check_item(void *, type_identity *);
    void check_pointer(void *, type_identity *);
    void check_static_array(void *, type_identity *, size_t);
    void check_container(void *, container_identity *);
    void check_vector(void *, container_identity *, type_identity *);
    void check_deque(void *, container_identity *, type_identity *);
    void check_dfarray(void *, container_identity *, type_identity *);
    void check_bit_container(void *, container_identity *);
    void check_virtual(void *, virtual_identity *);
    void check_primitive(void *, type_identity *);

    class Scope
    {
        Checker *parent;

    public:
        Scope(Checker *, const std::string &);
        Scope(Checker *, size_t);
        Scope(const Scope &) = delete;
        ~Scope();
    };
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

    check_global(&df::global::_identity);

    return ok;
}

Checker::Scope::Scope(Checker *parent, const std::string & name) :
    parent(parent)
{
    parent->path.push_back(name);
}

Checker::Scope::Scope(Checker *parent, size_t index) :
    Scope(parent, stl_sprintf("[%zu]", index))
{
}

Checker::Scope::~Scope()
{
    parent->path.pop_back();
}

#define FAIL(message) \
    do \
    { \
        ok = false; \
        out << COLOR_LIGHTRED << "sanity check failed (line " << __LINE__ << "): "; \
        out << COLOR_RESET << (identity ? identity->getFullName() : "?") << " (accessed as "; \
        for (auto & p : path) { out << p; } \
        out << "): "; \
        out << COLOR_YELLOW << message; \
        out << COLOR_RESET << std::endl; \
    } while (false)

#define PTR_ADD(base, offset) (reinterpret_cast<void *>(reinterpret_cast<uintptr_t>((base)) + static_cast<ptrdiff_t>((offset))))

bool Checker::check_access(void *base, type_identity *identity)
{
    return check_access(base, identity, identity ? identity->byte_size() : 0);
}

bool Checker::check_access(void *base, type_identity *identity, size_t size)
{
    if (!base)
    {
        // null pointer: can't access, but not an error
        return false;
    }

    for (auto & range : mapped)
    {
        if (!range.isInRange(base))
        {
            continue;
        }

        if (!range.valid || !range.read)
        {
            FAIL("pointer to invalid memory range");
            return false;
        }

        if (!range.isInRange(PTR_ADD(base, size)))
        {
            FAIL("pointer exceeds mapped memory bounds");
            return false;
        }

        return true;
    }

    FAIL("pointer not in any mapped range");
    return false;
}

bool Checker::check_vtable(void *vtable, type_identity *identity)
{
    if (!check_access(PTR_ADD(vtable, -ptrdiff_t(sizeof(void *))), identity, sizeof(void *)))
        return false;
    char **info = *(reinterpret_cast<char ***>(vtable) - 1);

#ifdef WIN32
    if (!check_access(PTR_ADD(info, 12), identity, 4))
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
    if (!check_access(info + 1, identity, sizeof(void *)))
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

        for (char *p = name; ; p++)
        {
            if (!range.isInRange(p))
            {
                return false;
            }

            if (!*p)
            {
                return true;
            }
        }
    }

    return false;
}

void Checker::check_global(global_identity *identity)
{
    Scope scope(this, "df::global::");

    for (auto field = identity->getFields(); field->mode != struct_field_info::END; field++)
    {
        if (!field->offset)
        {
            Scope scope_2(this, field->name);

            FAIL("global address is unknown");
            continue;
        }

        auto base = reinterpret_cast<void **>(field->offset);
        if (!seen_addr.insert(base).second)
        {
            continue;
        }

        check_field(*base, field);
    }
}

void Checker::check_fields(void *base, const struct_field_info *fields)
{
    for (auto field = fields; field->mode != struct_field_info::END; field++)
    {
        check_field(PTR_ADD(base, field->offset), field);
    }
}

void Checker::check_field(void *base, const struct_field_info *field)
{
    Scope scope(this, field->name);

    switch (field->mode)
    {
        case struct_field_info::END:
            // can't happen - already checked
            break;
        case struct_field_info::PRIMITIVE:
            check_primitive(base, field->type);
            break;
        case struct_field_info::STATIC_STRING:
            // TODO: check static strings?
            break;
        case struct_field_info::POINTER:
            check_pointer(base, field->type);
            break;
        case struct_field_info::STATIC_ARRAY:
            check_static_array(base, field->type, field->count);
            break;
        case struct_field_info::SUBSTRUCT:
            check_item(base, field->type);
            break;
        case struct_field_info::CONTAINER:
            check_container(base, static_cast<container_identity *>(field->type));
            break;
        case struct_field_info::STL_VECTOR_PTR:
            {
                df::stl_ptr_vector_identity temp_identity(field->type, field->eid);
                check_container(base, &temp_identity);
            }
            break;
        case struct_field_info::OBJ_METHOD:
            // ignore
            break;
        case struct_field_info::CLASS_METHOD:
            // ignore
            break;
    }
}

void Checker::check_item(void *base, type_identity *identity)
{
    if (!identity)
    {
        // void
        return;
    }

    switch (identity->type())
    {
        case IDTYPE_GLOBAL:
            // impossible
            break;
        case IDTYPE_FUNCTION:
            // ignore
            break;
        case IDTYPE_PRIMITIVE:
            check_primitive(base, identity);
            break;
        case IDTYPE_POINTER:
            check_pointer(base, static_cast<pointer_identity *>(identity)->getTarget());
            break;
        case IDTYPE_CONTAINER:
        case IDTYPE_BIT_CONTAINER:
        case IDTYPE_PTR_CONTAINER:
        case IDTYPE_STL_PTR_VECTOR:
            check_container(base, static_cast<container_identity *>(identity));
            break;
        case IDTYPE_BITFIELD:
            // TODO: check bitfields?
            break;
        case IDTYPE_ENUM:
            // TODO: check enums?
            break;
        case IDTYPE_STRUCT:
            {
                Scope scope(this, ".");

                check_fields(base, static_cast<struct_identity *>(identity)->getFields());
            }
            break;
        case IDTYPE_CLASS:
            check_virtual(base, static_cast<virtual_identity *>(identity));
            break;
        case IDTYPE_BUFFER:
            {
                auto item_identity = static_cast<container_identity *>(identity)->getItemType();

                check_static_array(base, item_identity, identity->byte_size() / item_identity->byte_size());
            }
            break;
        case IDTYPE_OPAQUE:
            // can't check opaque types
            break;
    }
}

void Checker::check_pointer(void *base, type_identity *identity)
{
    if (!seen_addr.insert(base).second)
    {
        return;
    }

    if (!identity)
    {
        // void pointer
        return;
    }

    if (!check_access(base, identity, sizeof(void *)))
    {
        return;
    }

    auto ptr = *reinterpret_cast<void **>(base);
    if (!check_access(ptr, identity))
    {
        return;
    }

    check_item(ptr, identity);
}

void Checker::check_static_array(void *base, type_identity *identity, size_t count)
{
    size_t item_size = identity->byte_size();

    for (size_t i = 0; i < count; i++)
    {
        Scope scope(this, i);

        check_item(PTR_ADD(base, item_size * i), identity);
    }
}

void Checker::check_container(void *base, container_identity *identity)
{
    if (identity->type() == IDTYPE_STRUCT)
    {
        // DfLinkedList
        check_item(base, identity);
        return;
    }

    if (!seen_addr.insert(base).second)
    {
        return;
    }

    if (identity->type() == IDTYPE_BIT_CONTAINER)
    {
        check_bit_container(base, identity);
        return;
    }

    auto item_identity = identity->getItemType();
    pointer_identity item_ptr_identity(item_identity);
    if (identity->type() == IDTYPE_PTR_CONTAINER || identity->type() == IDTYPE_STL_PTR_VECTOR)
    {
        item_identity = &item_ptr_identity;
    }
    else if (identity->type() != IDTYPE_CONTAINER)
    {
        // unexpected
        return;
    }

    auto void_name = identity->getFullName(nullptr);
    if (void_name == "vector<void>" || void_name == "vector<void*>")
    {
        check_vector(base, identity, item_identity);
    }
    else if (void_name == "deque<void>")
    {
        check_deque(base, identity, item_identity);
    }
    else if (void_name == "DfArray<void>")
    {
        check_dfarray(base, identity, item_identity);
    }
    else
    {
        FAIL("TODO: " << void_name);
        UNEXPECTED;
    }
}

void Checker::check_vector(void *base, container_identity *identity, type_identity *item_identity)
{
    struct vector_data
    {
        uintptr_t start;
        uintptr_t finish;
        uintptr_t end_of_storage;
    };

    if (identity->byte_size() != sizeof(vector_data))
    {
        UNEXPECTED;
        return;
    }

    vector_data vector = *reinterpret_cast<vector_data *>(base);

    size_t item_size = item_identity->byte_size();

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

    if (local_ok && check_access(reinterpret_cast<void *>(vector.start), identity, length))
    {
        check_static_array(reinterpret_cast<void *>(vector.start), item_identity, ulength / item_size);
    }
}

void Checker::check_deque(void *base, container_identity *identity, type_identity *item_identity)
{
    // TODO: check deque?
}

void Checker::check_dfarray(void *base, container_identity *identity, type_identity *item_identity)
{
    struct dfarray_data
    {
        uintptr_t start;
        unsigned short size;
    };

    if (identity->byte_size() != sizeof(dfarray_data))
    {
        UNEXPECTED;
        return;
    }

    dfarray_data dfarray = *reinterpret_cast<dfarray_data *>(base);

    size_t length = dfarray.size;
    size_t item_size = item_identity->byte_size();

    if (check_access(reinterpret_cast<void *>(dfarray.start), identity, item_size * length))
    {
        check_static_array(reinterpret_cast<void *>(dfarray.start), item_identity, length);
    }
}

void Checker::check_bit_container(void *base, container_identity *identity)
{
    if (identity->getFullName() == "BitArray<>")
    {
        // TODO: check DFHack::BitArray?
        return;
    }

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

    if (identity->byte_size() != sizeof(bvector_data) || identity->getFullName() != "vector<bool>")
    {
        UNEXPECTED;
        return;
    }

    // TODO: check vector<bool>?
}

void Checker::check_virtual(void *base, virtual_identity *identity)
{
    if (!seen_addr.insert(base).second)
    {
        return;
    }

    if (!check_access(base, identity))
    {
        return;
    }

    void *vtable = *reinterpret_cast<void **>(base);
    if (!check_vtable(vtable, identity))
    {
        FAIL("invalid vtable pointer");
        return;
    }
    else if (!identity->is_instance(reinterpret_cast<virtual_ptr>(base)))
    {
        auto class_name = Core::getInstance().p->readClassName(vtable);
        FAIL("vtable is not a known subclass (subclass is " << class_name << ")");
        return;
    }

    Scope scope(this, ".");

    check_fields(base, identity->getFields());
}

void Checker::check_primitive(void *base, type_identity *identity)
{
    if (identity->getFullName() != "string")
    {
        // we only care about string "primitives"
        return;
    }

    if (!seen_addr.insert(base).second)
    {
        return;
    }

    if (!check_access(base, identity))
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

    if (identity->byte_size() != sizeof(string_data))
    {
        UNEXPECTED;
        return;
    }

    auto string = reinterpret_cast<string_data *>(base);
#ifdef WIN32
    bool is_local = string->capacity < 16;
    char *start = is_local ? &string->local_data[0] : reinterpret_cast<char *>(string->start);
    ptrdiff_t length = string->length;
    ptrdiff_t capacity = string->capacity;
#else
    if (!check_access(string->ptr, identity, 1))
    {
        return;
    }
    if (!check_access(string->ptr - 1, identity, sizeof(*string->ptr)))
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

    check_access(start, identity, capacity);
}
