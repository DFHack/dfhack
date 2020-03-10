#include "check-structures-sanity.h"

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
    failfast(false)
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
    if (failfast)
        UNEXPECTED;
    return out;
}

void Checker::queue_item(const QueueItem & item, const CheckedStructure & cs)
{
    if (data.count(item.ptr) && data.at(item.ptr).full_size() == cs.full_size())
    {
        // already checked
        // TODO: make sure types are equal
        UNEXPECTED;
        return;
    }

    auto ptr_end = PTR_ADD(item.ptr, cs.full_size());

    auto prev = data.lower_bound(item.ptr);
    if (prev != data.cbegin())
    {
        prev--;
        if (uintptr_t(prev->first) + prev->second.full_size() > uintptr_t(item.ptr))
        {
            // TODO
            UNEXPECTED;
        }
    }

    auto overlap = data.lower_bound(item.ptr);
    auto overlap_end = data.lower_bound(ptr_end);
    while (overlap != overlap_end)
    {
        // TODO
        UNEXPECTED;
        overlap++;
    }

    data[item.ptr] = cs;
    queue.push_back(item);
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

    dispatch_item(item, cs->second);

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
            UNEXPECTED;
            break;
        case IDTYPE_UNION:
            dispatch_untagged_union(item, cs);
            break;
    }
}

void Checker::dispatch_primitive(const QueueItem & item, const CheckedStructure & cs)
{
    if (cs.identity->isConstructed())
    {
        if (cs.identity == df::identity_traits<std::string>::get())
        {
            // TODO check std::string
            UNEXPECTED;
        }
        else
        {
            UNEXPECTED;
        }
    }

    // TODO: check primitives
    UNEXPECTED;
}
void Checker::dispatch_pointer(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<pointer_identity *>(cs.identity);
    // TODO: check pointer
    UNEXPECTED;
}
void Checker::dispatch_container(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<container_identity *>(cs.identity);
    auto base_container = identity->getFullName(nullptr);
    if (base_container == "vector<void>")
    {
        if (identity->getIndexEnumType())
        {
            UNEXPECTED;
        }
        check_stl_vector(item, identity->getItemType());
    }
    else if (base_container == "deque<void>")
    {
        // TODO: check deque?
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
        UNEXPECTED;
    }
    else if (base_container == "vector<bool>")
    {
        // TODO: check stl bit vector
        UNEXPECTED;
    }
    else
    {
        UNEXPECTED;
    }
}
void Checker::dispatch_bitfield(const QueueItem & item, const CheckedStructure & cs)
{
    if (!enums)
    {
        return;
    }

    // TODO: check bitfields
    UNEXPECTED;
}
void Checker::dispatch_enum(const QueueItem & item, const CheckedStructure & cs)
{
    if (!enums)
    {
        return;
    }

    // TODO: check enums
    UNEXPECTED;
}
void Checker::dispatch_struct(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<struct_identity *>(cs.identity);
    for (auto p = identity; p; p = p->getParent())
    {
        auto fields = p->getFields();
        for (auto field = fields; field->mode != struct_field_info::END; field++)
        {
            dispatch_field(item, cs, fields, field);
        }
    }
}
void Checker::dispatch_field(const QueueItem & item, const CheckedStructure & cs, const struct_field_info *fields, const struct_field_info *field)
{
    if (field->mode == struct_field_info::OBJ_METHOD ||
        field->mode == struct_field_info::CLASS_METHOD)
    {
        return;
    }

    auto tag_field = find_union_tag(fields, field);
    if (tag_field)
    {
        UNEXPECTED;
        return;
    }

    auto field_ptr = PTR_ADD(item.ptr, field->offset);
    CheckedStructure field_cs(field);
    dispatch_item(QueueItem(item, field->name, field_ptr), field_cs);
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
    if (!base_identity->is_instance(vptr))
    {
        FAIL("expected subclass of " << base_identity->getFullName() << ", but got " << vtable_name);
        return;
    }

    auto identity = virtual_identity::get(vptr);
    dispatch_struct(QueueItem(item.path + "<" + identity->getFullName() + ">", item.ptr), CheckedStructure(identity));
}
void Checker::dispatch_buffer(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<container_identity *>(cs.identity);
    if (identity->getIndexEnumType())
    {
        UNEXPECTED;
    }

    auto item_identity = identity->getItemType();
    dispatch_item(item, CheckedStructure(item_identity, identity->byte_size() / item_identity->byte_size()));
}
void Checker::dispatch_stl_ptr_vector(const QueueItem & item, const CheckedStructure & cs)
{
    auto identity = static_cast<container_identity *>(cs.identity);
    if (identity->getIndexEnumType())
    {
        UNEXPECTED;
    }
    auto ptr_type = wrap_in_pointer(identity->getItemType());
    check_stl_vector(item, ptr_type);
}
void Checker::dispatch_untagged_union(const QueueItem & item, const CheckedStructure & cs)
{
    if (cs.identity == df::identity_traits<df::large_integer>::get())
    {
        dispatch_primitive(item, CheckedStructure(df::identity_traits<int64_t>::get()));
        return;
    }

    UNEXPECTED;
}

void Checker::check_stl_vector(const QueueItem & item, type_identity *item_identity)
{
    auto vec_items = validate_vector_size(item, CheckedStructure(item_identity));
    if (vec_items.first && vec_items.second)
    {
        QueueItem items_item(item.path, vec_items.first);
        CheckedStructure items_cs(item_identity, vec_items.second);
        queue_item(items_item, items_cs);
    }
}
