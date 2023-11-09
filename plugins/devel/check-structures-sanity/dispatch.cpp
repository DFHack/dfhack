#include "check-structures-sanity.h"

#include <cinttypes>

#include "df/large_integer.h"

Checker::Checker(color_ostream & out) :
    out(out),
    checked_count(0),
    error_count(0),
    maxerrors(~size_t(0)),
    maxerrors_reported(false),
    enums(false),
    sizes(false),
    unnamed(false),
    failfast(false),
    noprogress(!out.is_console()),
    maybepointer(false)
{
    Core::getInstance().p->getMemRanges(mapped);
}

color_ostream & Checker::fail(int line, const QueueItem & item, const CheckedStructure & cs)
{
    error_count++;
    out << COLOR_LIGHTRED << "sanity check failed (line " << line << "): ";
    out << COLOR_RESET << (cs.identity ? cs.identity->getFullName() : "?");
    out << " (accessed as " << item.path << "): ";
    out << COLOR_YELLOW;
    if (maxerrors && maxerrors != ~size_t(0))
        maxerrors--;
    return out;
}

bool Checker::queue_item(const QueueItem & item, CheckedStructure cs)
{
    if (!cs.identity)
    {
        UNEXPECTED;
        return false;
    }

    if (cs.identity->type() == IDTYPE_CLASS)
    {
        if (cs.count)
        {
            UNEXPECTED;
        }

        if (get_vtable_name(item, cs, true))
        {
            auto actual_identity = virtual_identity::get(reinterpret_cast<virtual_ptr>(const_cast<void *>(item.ptr)));
            if (static_cast<virtual_identity *>(cs.identity)->is_subclass(actual_identity))
            {
                cs.identity = actual_identity;
            }
        }
    }

    auto ptr_end = PTR_ADD(item.ptr, cs.full_size());

    auto prev = data.lower_bound(item.ptr);
    if (prev != data.cbegin() && uintptr_t(prev->first) > uintptr_t(item.ptr))
    {
        prev--;
    }
    if (prev != data.cend() && uintptr_t(prev->first) <= uintptr_t(item.ptr) && uintptr_t(prev->first) + prev->second.second.full_size() > uintptr_t(item.ptr))
    {
        auto offset = uintptr_t(item.ptr) - uintptr_t(prev->first);
        if (!prev->second.second.has_type_at_offset(cs, offset))
        {
            if (offset == 0 && cs.identity == df::identity_traits<void *>::get())
            {
                FAIL("unknown pointer is " << prev->second.second.identity->getFullName() << ", previously seen at " << prev->second.first);
                return false;
            }
            // TODO
            FAIL("TODO: handle merging structures: " << item.path << " overlaps " << prev->second.first << " (backward)");
            return false;
        }

        // we've already checked this structure, or we're currently queued to do so
        return false;
    }

    auto overlap_start = data.lower_bound(item.ptr);
    auto overlap_end = data.lower_bound(ptr_end);
    for (auto overlap = overlap_start; overlap != overlap_end; overlap++)
    {
        auto offset = uintptr_t(overlap->first) - uintptr_t(item.ptr);
        if (!cs.has_type_at_offset(overlap->second.second, offset))
        {
            // TODO
            FAIL("TODO: handle merging structures: " << overlap->second.first << " overlaps " << item.path << " (forward)");
            return false;
        }
    }

    data.erase(overlap_start, overlap_end);

    data[item.ptr] = std::make_pair(item.path, cs);
    queue.push_back(item);
    return true;
}

void Checker::queue_globals()
{
    auto fields = df::global::_identity.getFields();
    for (auto field = fields; field->mode != struct_field_info::END; field++)
    {
        if (!field->offset)
        {
            UNEXPECTED;
            continue;
        }

        // offset is the position of the DFHack pointer to this global.
        auto ptr = *reinterpret_cast<const void **>(field->offset);

        QueueItem item(stl_sprintf("df.global.%s", field->name), ptr);
        CheckedStructure cs(field);

        if (!ptr)
        {
            FAIL("unknown global address");
            continue;
        }

        if (!strcmp(field->name, "enabler"))
        {
            // don't care about libgraphics as we have the source code
            continue;
        }

        queue_item(item, cs);
    }
}

bool Checker::process_queue()
{
    if (queue.empty())
    {
        return false;
    }

    auto item = std::move(queue.front());
    queue.pop_front();

    auto cs = data.find(item.ptr);
    if (cs == data.end())
    {
        // happens if pointer is determined to be part of a larger structure
        return true;
    }

    dispatch_item(item, cs->second.second);

    return true;
}


void Checker::dispatch_item(const QueueItem & base, const CheckedStructure & cs)
{
    if (!is_valid_dereference(base, cs))
    {
        return;
    }

    if (!cs.count)
    {
        dispatch_single_item(base, cs);
        return;
    }

    if (sizes && !cs.inside_structure)
    {
        if (auto allocated_size = get_allocated_size(base))
        {
            auto expected_size = cs.identity->byte_size();
            if (cs.allocated_count)
                expected_size *= cs.allocated_count;
            else if (cs.count)
                expected_size *= cs.count;

            if (cs.identity->type() == IDTYPE_CLASS && get_vtable_name(base, cs, true))
            {
                if (cs.count)
                {
                    UNEXPECTED;
                }

                auto virtual_type = virtual_identity::get(static_cast<virtual_ptr>(const_cast<void *>(base.ptr)));
                expected_size = virtual_type->byte_size();
            }

            auto & item = base;

            if (allocated_size > expected_size)
            {
                FAIL("identified structure is too small (expected " << expected_size << " bytes, but there are " << allocated_size << " bytes allocated)");
            }
            else if (allocated_size < expected_size)
            {
                FAIL("identified structure is too big (expected " << expected_size << " bytes, but there are " << allocated_size << " bytes allocated)");
            }
        }
        else
        {
            UNEXPECTED;
        }
    }

    auto ptr = base.ptr;
    auto size = cs.identity->byte_size();
    for (size_t i = 0; i < cs.count; i++)
    {
        QueueItem item(base, i, ptr);
        dispatch_single_item(item, cs);
        ptr = PTR_ADD(ptr, size);
    }
}

void Checker::dispatch_single_item(const QueueItem & item, const CheckedStructure & cs)
{
    checked_count++;

    if (!maxerrors)
    {
        if (!maxerrors_reported)
        {
            FAIL("error limit reached. bailing out with " << (queue.size() + 1) << " items remaining in the queue.");
            maxerrors_reported = true;
        }
        queue.clear();
        return;
    }

    switch (cs.identity->type())
    {
        case IDTYPE_GLOBAL:
        case IDTYPE_FUNCTION:
            UNEXPECTED;
            break;
        case IDTYPE_PRIMITIVE:
            dispatch_primitive(item, cs);
            break;
        case IDTYPE_POINTER:
            dispatch_pointer(item, cs);
            break;
        case IDTYPE_CONTAINER:
            dispatch_container(item, cs);
            break;
        case IDTYPE_PTR_CONTAINER:
            dispatch_ptr_container(item, cs);
            break;
        case IDTYPE_BIT_CONTAINER:
            dispatch_bit_container(item, cs);
            break;
        case IDTYPE_BITFIELD:
            dispatch_bitfield(item, cs);
            break;
        case IDTYPE_ENUM:
            dispatch_enum(item, cs);
            break;
        case IDTYPE_STRUCT:
            dispatch_struct(item, cs);
            break;
        case IDTYPE_CLASS:
            dispatch_class(item, cs);
            break;
        case IDTYPE_BUFFER:
            dispatch_buffer(item, cs);
            break;
        case IDTYPE_STL_PTR_VECTOR:
            dispatch_stl_ptr_vector(item, cs);
            break;
        case IDTYPE_OPAQUE:
            break;
        case IDTYPE_UNION:
            dispatch_untagged_union(item, cs);
            break;
    }
}

void Checker::dispatch_primitive(const QueueItem & item, const CheckedStructure & cs)
{
    if (cs.identity == df::identity_traits<std::string>::get())
    {
        check_stl_string(item);
    }
    else if (cs.identity == df::identity_traits<char *>::get())
    {
        // TODO check c strings
        UNEXPECTED;
    }
    else if (cs.identity == df::identity_traits<bool>::get())
    {
        auto val = *reinterpret_cast<const uint8_t *>(item.ptr);
        if (val > 1 && perturb_byte && val != perturb_byte && val != (perturb_byte ^ 0xff))
        {
            FAIL("invalid value for bool: " << int(val));
        }
    }
    else if (dynamic_cast<df::integer_identity_base *>(cs.identity))
    {
        check_possible_pointer(item, cs);

        // TODO check ints?
    }
    else if (dynamic_cast<df::float_identity_base *>(cs.identity))
    {
        // TODO check floats?
    }
    else
    {
        UNEXPECTED;
    }
}
void Checker::dispatch_pointer(const QueueItem & item, const CheckedStructure & cs)
{
    auto target_ptr = validate_and_dereference<const void *>(item);
    if (!target_ptr)
    {
        return;
    }

#ifdef DFHACK64
    if (uintptr_t(target_ptr) == 0xd2d2d2d2d2d2d2d2)
#else
    if (uintptr_t(target_ptr) == 0xd2d2d2d2)
#endif
    {
        return;
    }

    QueueItem target_item(item.path, target_ptr);
    auto target = static_cast<pointer_identity *>(cs.identity)->getTarget();
    if (!target)
    {
        check_unknown_pointer(target_item);
        return;
    }

    CheckedStructure target_cs(target);

    // 256 is an arbitrarily chosen size threshold
    if (cs.count || target->byte_size() <= 256)
    {
        // target is small, or we are inside an array of pointers; handle now
        if (queue_item(target_item, target_cs))
        {
            // we insert it into the queue to make sure we're not stuck in a loop
            // get it back out of the queue to prevent the queue growing too big
            queue.pop_back();
            dispatch_item(target_item, target_cs);
        }
    }
    else
    {
        // target is large and not part of an array; handle later
        queue_item(target_item, target_cs);
    }
}
void Checker::dispatch_container(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<container_identity *>(cs.identity);
    auto base_container = identity->getFullName(nullptr);
    if (base_container == "vector<void>")
    {
        check_stl_vector(item, identity->getItemType(), identity->getIndexEnumType());
    }
    else if (base_container == "deque<void>")
    {
        // TODO: check deque?
    }
    else if (base_container == "DfArray<void>")
    {
        // TODO: check DfArray
    }
    else if (base_container.starts_with("map<"))
    {
        // TODO: check map
    }
    else if (base_container.starts_with("unordered_map<"))
    {
        // TODO: check unordered_map
    }
    else
    {
        UNEXPECTED;
    }
}
void Checker::dispatch_ptr_container(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<container_identity *>(cs.identity);
    auto base_container = identity->getFullName(nullptr);
    {
        UNEXPECTED;
    }
}
void Checker::dispatch_bit_container(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<container_identity *>(cs.identity);
    auto base_container = identity->getFullName(nullptr);
    if (base_container == "BitArray<>")
    {
        // TODO: check DF bit array
    }
    else if (base_container == "vector<bool>")
    {
        // TODO: check stl bit vector
    }
    else
    {
        UNEXPECTED;
    }
}
void Checker::dispatch_bitfield(const QueueItem & item, const CheckedStructure & cs)
{
    check_possible_pointer(item, cs);

    if (!enums)
    {
        return;
    }

    auto bitfield_type = static_cast<bitfield_identity *>(cs.identity);
    uint64_t bitfield_value;
    switch (bitfield_type->byte_size())
    {
        case 1:
            bitfield_value = validate_and_dereference<uint8_t>(item);
            // don't check for uninitialized; too small to be sure
            break;
        case 2:
            bitfield_value = validate_and_dereference<uint16_t>(item);
            if (bitfield_value == 0xd2d2)
            {
                bitfield_value = 0;
            }
            break;
        case 4:
            bitfield_value = validate_and_dereference<uint32_t>(item);
            if (bitfield_value == 0xd2d2d2d2)
            {
                bitfield_value = 0;
            }
            break;
        case 8:
            bitfield_value = validate_and_dereference<uint64_t>(item);
            if (bitfield_value == 0xd2d2d2d2d2d2d2d2)
            {
                bitfield_value = 0;
            }
            break;
        default:
            UNEXPECTED;
            bitfield_value = 0;
            break;
    }

    auto num_bits = bitfield_type->getNumBits();
    auto bits = bitfield_type->getBits();
    for (int i = 0; i < 64; i++)
    {
        if (!(num_bits & 1))
        {
            bitfield_value >>= 1;
            continue;
        }

        bitfield_value >>= 1;

        if (i >= num_bits || !bits[i].size)
        {
            FAIL("bitfield bit " << i << " is out of range");
        }
        else if (unnamed && bits[i].size > 0 && !bits[i].name)
        {
            FAIL("bitfield bit " << i << " is unnamed");
        }
        else if (unnamed && !bits[i + bits[i].size].name)
        {
            FAIL("bitfield bit " << i << " (part of a field starting at bit " << (i + bits[i].size) << ") is unnamed");
        }
    }
}
void Checker::dispatch_enum(const QueueItem & item, const CheckedStructure & cs)
{
    check_possible_pointer(item, cs);

    if (!enums)
    {
        return;
    }

    auto enum_type = static_cast<enum_identity *>(cs.identity);
    auto enum_value = get_int_value(item, enum_type->getBaseType());
    if (enum_type->byte_size() == 2 && uint16_t(enum_value) == 0xd2d2)
    {
        return;
    }
    else if (enum_type->byte_size() == 4 && uint32_t(enum_value) == 0xd2d2d2d2)
    {
        return;
    }
    else if (enum_type->byte_size() == 8 && uint64_t(enum_value) == 0xd2d2d2d2d2d2d2d2)
    {
        return;
    }

    if (is_in_global(item) && enum_value == 0)
    {
        return;
    }

    auto enum_name = get_enum_item_key(enum_type, enum_value);
    if (!enum_name)
    {
        FAIL("enum value (" << enum_value << ") is out of range");
        return;
    }
    if (unnamed && !*enum_name)
    {
        FAIL("enum value (" << enum_value << ") is unnamed");
    }
}
void Checker::dispatch_struct(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<struct_identity *>(cs.identity);
    for (auto p = identity; p; p = p->getParent())
    {
        auto fields = p->getFields();
        if (!fields)
        {
            continue;
        }

        for (auto field = fields; field->mode != struct_field_info::END; field++)
        {
            dispatch_field(item, cs, identity, field);
        }
    }
}
void Checker::dispatch_field(const QueueItem & item, const CheckedStructure & cs, struct_identity *identity, const struct_field_info *field)
{
    if (field->mode == struct_field_info::OBJ_METHOD ||
        field->mode == struct_field_info::CLASS_METHOD)
    {
        return;
    }

    auto field_ptr = PTR_ADD(item.ptr, field->offset);
    QueueItem field_item(item, field->name, field_ptr);
    CheckedStructure field_cs(field);

    auto tag_field = find_union_tag(identity, field);
    if (tag_field)
    {
        auto tag_ptr = PTR_ADD(item.ptr, tag_field->offset);
        QueueItem tag_item(item, tag_field->name, tag_ptr);
        CheckedStructure tag_cs(tag_field);
        auto attr_name = field->extra ? field->extra->union_tag_attr : nullptr;
        if (tag_cs.identity->isContainer())
        {
            dispatch_tagged_union_vector(field_item, tag_item, field_cs, tag_cs, attr_name);
        }
        else
        {
            dispatch_tagged_union(field_item, tag_item, field_cs, tag_cs, attr_name);
        }
        return;
    }

    dispatch_item(field_item, field_cs);
}
void Checker::dispatch_class(const QueueItem & item, const CheckedStructure & cs)
{
    auto vtable_name = get_vtable_name(item, cs);
    if (!vtable_name)
    {
        // bail out now because virtual_identity::get will crash
        return;
    }

    auto base_identity = static_cast<virtual_identity *>(cs.identity);
    auto vptr = static_cast<virtual_ptr>(const_cast<void *>(item.ptr));
    auto identity = virtual_identity::get(vptr);
    if (!identity)
    {
        FAIL("unidentified subclass of " << base_identity->getFullName() << ": " << vtable_name);
        return;
    }
    if (base_identity != identity && !base_identity->is_subclass(identity))
    {
        FAIL("expected subclass of " << base_identity->getFullName() << ", but got " << identity->getFullName());
        return;
    }

    if (data.count(item.ptr) && data.at(item.ptr).first == item.path)
    {
        // TODO: handle cases where this may overlap later data
        data.at(item.ptr).second.identity = identity;
    }

    dispatch_struct(QueueItem(item.path + "<" + identity->getFullName() + ">", item.ptr), CheckedStructure(identity));
}
void Checker::dispatch_buffer(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<container_identity *>(cs.identity);

    auto item_identity = identity->getItemType();
    dispatch_item(item, CheckedStructure(item_identity, identity->byte_size() / item_identity->byte_size(), static_cast<enum_identity *>(identity->getIndexEnumType()), true));
}
void Checker::dispatch_stl_ptr_vector(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<container_identity *>(cs.identity);
    auto ptr_type = wrap_in_pointer(identity->getItemType());
    check_stl_vector(item, ptr_type, identity->getIndexEnumType());
}
void Checker::dispatch_tagged_union(const QueueItem & item, const QueueItem & tag_item, const CheckedStructure & cs, const CheckedStructure & tag_cs, const char *attr_name)
{
    if (tag_cs.identity->type() != IDTYPE_ENUM || cs.identity->type() != IDTYPE_UNION)
    {
        UNEXPECTED;
        return;
    }

    auto union_type = static_cast<union_identity *>(cs.identity);
    auto union_data_ptr = reinterpret_cast<const uint8_t *>(item.ptr);
    uint8_t padding_byte = *union_data_ptr;
    bool all_padding = false;
    if (padding_byte == 0x00 || padding_byte == 0xd2 || padding_byte == 0xff)
    {
        all_padding = true;
        for (size_t i = 0; i < union_type->byte_size(); i++)
        {
            if (union_data_ptr[i] != padding_byte)
            {
                all_padding = false;
                break;
            }
        }
    }

    auto tag_identity = static_cast<enum_identity *>(tag_cs.identity);
    auto tag_value = get_int_value(tag_item, tag_identity->getBaseType());
    if (all_padding && padding_byte == 0xd2)
    {
        // special case: completely uninitialized
        switch (tag_identity->byte_size())
        {
            case 1:
                if (tag_value == 0xd2)
                {
                    return;
                }
                break;
            case 2:
                if (tag_value == 0xd2d2)
                {
                    return;
                }
                break;
            case 4:
                if (tag_value == 0xd2d2d2d2)
                {
                    return;
                }
                break;
            case 8:
                if (uint64_t(tag_value) == 0xd2d2d2d2d2d2d2d2)
                {
                    return;
                }
                break;
            default:
                UNEXPECTED;
                break;
        }
    }

    auto tag_name = get_enum_item_attr_or_key(tag_identity, tag_value, attr_name);
    if (!tag_name)
    {
        FAIL("tagged union tag (accessed as " << tag_item.path << ") value (" << tag_value << ") not defined in enum " << tag_cs.identity->getFullName());
        return;
    }

    if (!*tag_name)
    {
        FAIL("tagged union tag (accessed as " << tag_item.path << ") value (" << tag_value << ") is unnamed");
        return;
    }

    for (auto field = union_type->getFields(); field->mode != struct_field_info::END; field++)
    {
        if (strcmp(field->name, *tag_name))
        {
            continue;
        }

        if (field->offset != 0)
        {
            UNEXPECTED;
        }

        dispatch_item(QueueItem(item, field->name, item.ptr), field);
        return;
    }

    // don't ask for fields if it's all padding
    if (all_padding)
    {
        return;
    }

    FAIL("tagged union missing " << *tag_name << " field to match tag (accessed as " << tag_item.path << ") value (" << tag_value << ")");
}
void Checker::dispatch_tagged_union_vector(const QueueItem & item, const QueueItem & tag_item, const CheckedStructure & cs, const CheckedStructure & tag_cs, const char *attr_name)
{
    auto union_container_identity = static_cast<container_identity *>(cs.identity);
    CheckedStructure union_item_cs(union_container_identity->getItemType());
    if (union_container_identity->type() != IDTYPE_CONTAINER)
    {
        // assume pointer container
        union_item_cs.identity = wrap_in_pointer(union_item_cs.identity);
    }

    auto tag_container_identity = static_cast<container_identity *>(tag_cs.identity);
    auto tag_container_base = tag_container_identity->getFullName(nullptr);
    if (tag_container_base == "vector<void>")
    {
        auto vec_union = validate_vector_size(item, union_item_cs);
        CheckedStructure tag_item_cs(tag_container_identity->getItemType());
        auto vec_tag = validate_vector_size(tag_item, tag_item_cs);
        if (!vec_union.first || !vec_tag.first)
        {
            // invalid vectors (already warned)
            return;
        }
        if (!vec_union.second.count && !vec_tag.second.count)
        {
            // empty vectors
            return;
        }
        if (vec_union.second.count != vec_tag.second.count)
        {
            FAIL("tagged union vector is " << vec_union.second.count << " elements, but tag vector (accessed as " << tag_item.path << ") is " << vec_tag.second.count << " elements");
        }

        for (size_t i = 0; i < vec_union.second.count && i < vec_tag.second.count; i++)
        {
            dispatch_tagged_union(QueueItem(item, i, vec_union.first), QueueItem(tag_item, i, vec_tag.first), union_item_cs, tag_item_cs, attr_name);
            vec_union.first = PTR_ADD(vec_union.first, union_item_cs.identity->byte_size());
            vec_tag.first = PTR_ADD(vec_tag.first, tag_item_cs.identity->byte_size());
        }
    }
    else if (tag_container_base == "vector<bool>")
    {
        // TODO
        UNEXPECTED;
    }
    else
    {
        UNEXPECTED;
    }
}
void Checker::dispatch_untagged_union(const QueueItem & item, const CheckedStructure & cs)
{
    // special case for large_integer weirdness
    if (cs.identity == df::identity_traits<df::large_integer>::get())
    {
        // it's 16 bytes on 64-bit linux due to a messy header in libgraphics
        // but only the first 8 bytes are ever used
        dispatch_primitive(item, CheckedStructure(df::identity_traits<int64_t>::get(), 0, nullptr, cs.inside_structure));
        return;
    }

    FAIL("unhandled untagged union: " << item.path);
}

void Checker::check_unknown_pointer(const QueueItem & item)
{
    const static CheckedStructure cs(nullptr, 0, nullptr, true);
    if (auto allocated_size = get_allocated_size(item))
    {
        FAIL("pointer to a block of " << allocated_size << " bytes of allocated memory");
        if (allocated_size >= MIN_SIZE_FOR_SUGGEST && known_types_by_size.count(allocated_size))
        {
            FAIL("known types of this size: " << join_strings(", ", known_types_by_size.at(allocated_size)));
        }

        // check recursively if it's the right size for a pointer
        // or if it starts with what might be a valid pointer
        QueueItem ptr_item(item, "?ptr?", item.ptr);
        if (allocated_size == sizeof(void *) || (allocated_size > sizeof(void *) && is_valid_dereference(ptr_item, 1, true)))
        {
            CheckedStructure ptr_cs(df::identity_traits<void *>::get());
            if (queue_item(ptr_item, ptr_cs))
            {
                queue.pop_back();
                dispatch_pointer(ptr_item, ptr_cs);
            }
        }
    }
#ifndef WIN32
    else if (auto str = validate_stl_string_pointer(&item.ptr))
    {
        FAIL("untyped pointer is actually stl-string with value \"" << *str << "\" (length " << str->length() << ")");
    }
#endif
    else if (auto vtable_name = get_vtable_name(QueueItem(item.path, &item.ptr), cs, true))
    {
        FAIL("pointer to a vtable: " << vtable_name);
    }
    else if (sizes)
    {
        //FAIL("pointer to memory with no size information");
    }
}

void Checker::check_stl_vector(const QueueItem & item, type_identity *item_identity, type_identity *eid)
{
    auto vec_items = validate_vector_size(item, CheckedStructure(item_identity));

    // skip bad pointer vectors
    if ((item.path.ends_with(".bad") || item.path.ends_with(".temp_save")) && item_identity->type() == IDTYPE_POINTER)
    {
        return;
    }

    if (vec_items.first && vec_items.second.count)
    {
        QueueItem items_item(item.path, vec_items.first);
        queue_item(items_item, vec_items.second);
    }
}

void Checker::check_stl_string(const QueueItem & item)
{
    const static CheckedStructure cs(df::identity_traits<std::string>::get(), 0, nullptr, true);

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
        uintptr_t start;
        size_t length;
        union
        {
            char local_data[16];
            size_t capacity;
        };
    };
#endif

    auto string = reinterpret_cast<const string_data *>(item.ptr);
#ifdef WIN32
    const bool is_gcc = false;
    const bool is_local = string->capacity < 16;
#else
    const bool is_gcc = true;
    const bool is_local = string->start == reinterpret_cast<uintptr_t>(&string->local_data[0]);
#endif
    const char *start = is_local ? &string->local_data[0] : reinterpret_cast<const char *>(string->start);
    ptrdiff_t length = string->length;
    ptrdiff_t capacity = string->capacity;

    (void)start;
    if (length < 0)
    {
        FAIL("string length is negative (" << length << ")");
    }
    else if (is_gcc && length > 0 && !is_valid_dereference(QueueItem(item, "?start?", reinterpret_cast<void *>(string->start)), 1))
    {
        // nullptr is NOT okay here
        FAIL("invalid string pointer " << stl_sprintf("0x%" PRIxPTR, string->start));
        return;
    }
    else if (is_local && length >= 16)
    {
        FAIL("string length is too large for small string (" << length << ")");
    }
    else if ((!is_gcc || !is_local) && capacity < 0)
    {
        FAIL("string capacity is negative (" << capacity << ")");
    }
    else if ((!is_gcc || !is_local) && capacity < length)
    {
        FAIL("string capacity (" << capacity << ") is less than length (" << length << ")");
    }
}
void Checker::check_possible_pointer(const QueueItem & item, const CheckedStructure & cs)
{
    if (sizes && maybepointer && uintptr_t(item.ptr) % sizeof(void *) == 0)
    {
        auto ptr = validate_and_dereference<const void *>(item, true);
        QueueItem ptr_item(item, "?maybe_pointer?", ptr);
        if (ptr && is_valid_dereference(ptr_item, 1, true))
        {
            check_unknown_pointer(ptr_item);
        }
    }
}
