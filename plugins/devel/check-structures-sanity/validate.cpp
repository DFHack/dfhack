#include "check-structures-sanity.h"

bool Checker::is_valid_dereference(const QueueItem & item, const CheckedStructure & cs, bool quiet)
{
    auto base = const_cast<void *>(item.ptr);
    auto size = cs.full_size();
    if (!base)
    {
        // cannot dereference null pointer, but not an error
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
    if (uintptr_t(base) == UNINIT_PTR)
    {
        if (!quiet)
        {
            FAIL_PTR("uninitialized pointer");
        }
        return false;
    }

    bool found = true;
    auto expected_start = base;
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
                if (!quiet)
                {
                    FAIL_PTR("pointer to invalid memory range");
                }
                return false;
            }

            auto expected_end = const_cast<void *>(PTR_ADD(expected_start, remaining_size - 1));
            if (size && !range.isInRange(expected_end))
            {
                auto next_start = PTR_ADD(range.end, 1);
                remaining_size -= ptrdiff_t(next_start) - ptrdiff_t(expected_start);
                expected_start = const_cast<void *>(next_start);
                break;
            }

            return true;
        }
    }

    if (quiet)
    {
        return false;
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

const char *Checker::get_vtable_name(const QueueItem & item, const CheckedStructure & cs, bool quiet)
{
    auto vtable = validate_and_dereference<const void *const*>(QueueItem(item, "?vtable?", item.ptr), quiet);
    if (!vtable)
        return nullptr;

    auto info = validate_and_dereference<const char *const*>(QueueItem(item, "?vtable?.info", vtable - 1), quiet);
    if (!info)
        return nullptr;

#ifdef WIN32
#ifdef DFHACK64
    void *base;
    if (!RtlPcToFileHeader(info, &base))
        return nullptr;

    const char *typeinfo = reinterpret_cast<const char *>(base) + reinterpret_cast<int32_t *>(info)[3];
    const char *name = typeinfo + 16;
#else
    const char *name = reinterpret_cast<const char *>(info) + 8;
#endif
#else
    auto name = validate_and_dereference<const char *>(QueueItem(item, "?vtable?.info.name", info + 1), quiet);
#endif

    for (auto & range : mapped)
    {
        if (!range.isInRange(const_cast<char *>(name)))
        {
            continue;
        }

        if (!range.valid || !range.read)
        {
            if (!quiet)
            {
                FAIL("pointer to invalid memory range");
            }
            return nullptr;
        }

        const char *first_letter = nullptr;
        bool letter = false;
        for (const char *p = name; ; p++)
        {
            if (!range.isInRange(const_cast<char *>(p)))
            {
                return nullptr;
            }

            if ((*p >= 'a' && *p <= 'z') || *p == '_')
            {
                if (!letter)
                {
                    first_letter = p;
                }
                letter = true;
            }
            else if (!*p)
            {
                return first_letter;
            }
        }
    }

    return nullptr;
}

std::pair<const void *, size_t> Checker::validate_vector_size(const QueueItem & item, const CheckedStructure & cs, bool quiet)
{
    struct vector_data
    {
        uintptr_t start;
        uintptr_t finish;
        uintptr_t end_of_storage;
    };

    vector_data vector = *reinterpret_cast<const vector_data *>(item.ptr);

    ptrdiff_t length = vector.finish - vector.start;
    ptrdiff_t capacity = vector.end_of_storage - vector.start;

    bool local_ok = true;
    auto item_size = cs.full_size();
    if (!item_size)
    {
        item_size = 1;
        local_ok = false;
    }

    if (vector.start > vector.finish)
    {
        local_ok = false;
        if (!quiet)
        {
            FAIL("vector length is negative (" << (length / ptrdiff_t(item_size)) << ")");
        }
    }
    if (vector.start > vector.end_of_storage)
    {
        local_ok = false;
        if (!quiet)
        {
            FAIL("vector capacity is negative (" << (capacity / ptrdiff_t(item_size)) << ")");
        }
    }
    else if (vector.finish > vector.end_of_storage)
    {
        local_ok = false;
        if (!quiet)
        {
            FAIL("vector capacity (" << (capacity / ptrdiff_t(item_size)) << ") is less than its length (" << (length / ptrdiff_t(item_size)) << ")");
        }
    }

    size_t ulength = size_t(length);
    size_t ucapacity = size_t(capacity);
    if (ulength % item_size != 0)
    {
        local_ok = false;
        if (!quiet)
        {
            FAIL("vector length is non-integer (" << (ulength / item_size) << " items plus " << (ulength % item_size) << " bytes)");
        }
    }
    if (ucapacity % item_size != 0)
    {
        local_ok = false;
        if (!quiet)
        {
            FAIL("vector capacity is non-integer (" << (ucapacity / item_size) << " items plus " << (ucapacity % item_size) << " bytes)");
        }
    }

    if (local_ok && capacity && !vector.start)
    {
        if (!quiet)
        {
            FAIL("vector has null pointer but capacity " << (capacity / item_size));
        }
        return std::make_pair(nullptr, 0);
    }

    auto start_ptr = reinterpret_cast<const void *>(vector.start);

    if (capacity && !is_valid_dereference(QueueItem(item, "?items?", start_ptr), CheckedStructure(cs.identity, capacity / item_size), quiet))
    {
        local_ok = false;
    }

    return local_ok ? std::make_pair(start_ptr, ulength / item_size) : std::make_pair(nullptr, 0);
}
