#pragma once

#include "Console.h"
#include "PluginManager.h"
#include "MemAccess.h"
#include "DataDefs.h"
#include "DataIdentity.h"

#include <deque>
#include <map>
#include <string>

using namespace DFHack;

#ifdef WIN32
#define UNEXPECTED __debugbreak()
#else
#define UNEXPECTED __asm__ volatile ("int $0x03; nop")
#endif

#define PTR_ADD(ptr, offset) reinterpret_cast<const void *>(uintptr_t(ptr) + (offset))

struct QueueItem
{
    QueueItem(const std::string &, const void *);
    QueueItem(const QueueItem &, const std::string &, const void *);
    QueueItem(const QueueItem &, size_t, const void *);

    std::string path;
    const void *ptr;
};
struct CheckedStructure
{
    type_identity *identity;
    size_t count;
    size_t allocated_count;
    enum_identity *eid;
    bool ptr_is_array;
    bool inside_structure;

    CheckedStructure();
    explicit CheckedStructure(type_identity *, size_t = 0);
    CheckedStructure(type_identity *, size_t, enum_identity *, bool);
    CheckedStructure(const struct_field_info *);

    size_t full_size() const;
    bool has_type_at_offset(const CheckedStructure &, size_t) const;
};

#define MIN_SIZE_FOR_SUGGEST 64
extern std::map<size_t, std::vector<std::string>> known_types_by_size;
void build_size_table();

namespace
{
    template<typename T, bool is_pointer = std::is_pointer<T>::value>
    struct safe_t
    {
        typedef T type;
    };
    template<typename T>
    struct safe_t<T, true>
    {
        typedef void *type;
    };
}

class Checker
{
    color_ostream & out;
    std::vector<t_memrange> mapped;
    std::map<const void *, std::pair<std::string, CheckedStructure>> data;
    std::deque<QueueItem> queue;
public:
    size_t checked_count;
    size_t error_count;
    size_t maxerrors;
    bool maxerrors_reported;
    bool enums;
    bool sizes;
    bool unnamed;
    bool failfast;
    bool noprogress;
    bool maybepointer;
    uint8_t perturb_byte;

    Checker(color_ostream & out);
    bool queue_item(const QueueItem & item, CheckedStructure cs);
    void queue_globals();
    bool process_queue();

    bool is_in_global(const QueueItem & item);
    bool is_valid_dereference(const QueueItem & item, const CheckedStructure & cs, size_t size, bool quiet);
    inline bool is_valid_dereference(const QueueItem & item, const CheckedStructure & cs, bool quiet = false)
    {
        return is_valid_dereference(item, cs, cs.full_size(), quiet);
    }
    inline bool is_valid_dereference(const QueueItem & item, size_t size, bool quiet = false)
    {
        return is_valid_dereference(item, CheckedStructure(df::identity_traits<void *>::get()), size, quiet);
    }
    template<typename T>
    const T validate_and_dereference(const QueueItem & item, bool quiet = false)
    {
        CheckedStructure cs;
        cs.identity = df::identity_traits<typename safe_t<T>::type>::get();
        if (!is_valid_dereference(item, cs, quiet))
            return T();

        return *reinterpret_cast<const T *>(item.ptr);
    }
    int64_t get_int_value(const QueueItem & item, type_identity *type, bool quiet = false);
    const char *get_vtable_name(const QueueItem & item, const CheckedStructure & cs, bool quiet = false);
    std::pair<const void *, CheckedStructure> validate_vector_size(const QueueItem & item, const CheckedStructure & cs, bool quiet = false);
    size_t get_allocated_size(const QueueItem & item);
#ifndef WIN32
    // this function doesn't make sense on windows, where std::string is not pointer-sized.
    const std::string *validate_stl_string_pointer(const void *const*);
#endif
    static const char *const *get_enum_item_key(enum_identity *identity, int64_t value);
    static const char *const *get_enum_item_attr_or_key(enum_identity *identity, int64_t value, const char *attr_name);

private:
    color_ostream & fail(int, const QueueItem &, const CheckedStructure &);
    void dispatch_item(const QueueItem &, const CheckedStructure &);
    void dispatch_single_item(const QueueItem &, const CheckedStructure &);
    void dispatch_primitive(const QueueItem &, const CheckedStructure &);
    void dispatch_pointer(const QueueItem &, const CheckedStructure &);
    void dispatch_container(const QueueItem &, const CheckedStructure &);
    void dispatch_ptr_container(const QueueItem &, const CheckedStructure &);
    void dispatch_bit_container(const QueueItem &, const CheckedStructure &);
    void dispatch_bitfield(const QueueItem &, const CheckedStructure &);
    void dispatch_enum(const QueueItem &, const CheckedStructure &);
    void dispatch_struct(const QueueItem &, const CheckedStructure &);
    void dispatch_field(const QueueItem &, const CheckedStructure &, struct_identity *, const struct_field_info *);
    void dispatch_class(const QueueItem &, const CheckedStructure &);
    void dispatch_buffer(const QueueItem &, const CheckedStructure &);
    void dispatch_stl_ptr_vector(const QueueItem &, const CheckedStructure &);
    void dispatch_tagged_union(const QueueItem &, const QueueItem &, const CheckedStructure &, const CheckedStructure &, const char *);
    void dispatch_tagged_union_vector(const QueueItem &, const QueueItem &, const CheckedStructure &, const CheckedStructure &, const char *);
    void dispatch_untagged_union(const QueueItem &, const CheckedStructure &);
    void check_unknown_pointer(const QueueItem &);
    void check_stl_vector(const QueueItem &, type_identity *, type_identity *);
    void check_stl_string(const QueueItem &);
    void check_possible_pointer(const QueueItem &, const CheckedStructure &);

    friend struct CheckedStructure;
    static type_identity *wrap_in_pointer(type_identity *);
    static type_identity *wrap_in_stl_ptr_vector(type_identity *);
};

#define FAIL(message) \
    do \
    { \
        auto & failstream = fail(__LINE__, item, cs); \
        failstream << message; \
        failstream << COLOR_RESET << std::endl; \
        if (failfast) \
            UNEXPECTED; \
    } \
    while (false)
