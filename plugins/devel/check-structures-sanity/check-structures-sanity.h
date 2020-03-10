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
#define UNEXPECTED __asm__ volatile ("int $0x03")
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

    CheckedStructure();
    explicit CheckedStructure(type_identity *, size_t = 0);
    CheckedStructure(const struct_field_info *);

    size_t full_size() const;
};

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
    std::map<const void *, CheckedStructure> data;
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

    Checker(color_ostream & out);
    void queue_item(const QueueItem & item, const CheckedStructure & cs);
    void queue_globals();
    bool process_queue();

    bool is_valid_dereference(const QueueItem & item, const CheckedStructure & cs, bool quiet = false);
    template<typename T>
    const T validate_and_dereference(const QueueItem & item, bool quiet = false)
    {
        CheckedStructure cs;
        cs.identity = df::identity_traits<typename safe_t<T>::type>::get();
        if (!is_valid_dereference(item, cs, quiet))
            return T();

        return *reinterpret_cast<const T *>(item.ptr);
    }
    const char *get_vtable_name(const QueueItem & item, const CheckedStructure & cs, bool quiet = false);
    std::pair<const void *, size_t> validate_vector_size(const QueueItem & item, const CheckedStructure & cs, bool quiet = false);

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
    void dispatch_field(const QueueItem &, const CheckedStructure &, const struct_field_info *, const struct_field_info *);
    void dispatch_class(const QueueItem &, const CheckedStructure &);
    void dispatch_buffer(const QueueItem &, const CheckedStructure &);
    void dispatch_stl_ptr_vector(const QueueItem &, const CheckedStructure &);
    void dispatch_untagged_union(const QueueItem &, const CheckedStructure &);
    void check_stl_vector(const QueueItem &, type_identity *);

    friend struct CheckedStructure;
    static type_identity *wrap_in_pointer(type_identity *);
    static type_identity *wrap_in_stl_ptr_vector(type_identity *);
};

#define FAIL(message) (static_cast<color_ostream &>(fail(__LINE__, item, cs) << message) << COLOR_RESET << std::endl)
