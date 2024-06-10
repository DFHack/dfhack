#include "check-structures-sanity.h"

QueueItem::QueueItem(const std::string & path, const void *ptr) :
    path(path),
    ptr(ptr)
{
}
QueueItem::QueueItem(const QueueItem & parent, const std::string & member, const void *ptr) :
    QueueItem(parent.path + "." + member, ptr)
{
}
QueueItem::QueueItem(const QueueItem & parent, size_t index, const void *ptr) :
    QueueItem(parent.path + stl_sprintf("[%zu]", index), ptr)
{
}

CheckedStructure::CheckedStructure() :
    CheckedStructure(nullptr, 0)
{
}
CheckedStructure::CheckedStructure(const type_identity *identity, size_t count) :
    CheckedStructure(identity, count, nullptr, false)
{
}
CheckedStructure::CheckedStructure(const type_identity *identity, size_t count, const enum_identity *eid, bool inside_structure) :
    identity(identity),
    count(count),
    allocated_count(0),
    eid(eid),
    ptr_is_array(false),
    inside_structure(inside_structure)
{
}
CheckedStructure::CheckedStructure(const struct_field_info *field) :
    CheckedStructure()
{
    if (!field || field->mode == struct_field_info::END)
    {
        UNEXPECTED;
    }

    identity = field->type;
    eid = field->extra ? field->extra->index_enum : nullptr;
    inside_structure = true;
    switch (field->mode)
    {
        case struct_field_info::END:
            UNEXPECTED;
            break;
        case struct_field_info::PRIMITIVE:
            if (field->count || !field->type)
            {
                UNEXPECTED;
            }
            break;
        case struct_field_info::STATIC_STRING:
            if (!field->count || field->type)
            {
                UNEXPECTED;
            }
            identity = df::identity_traits<char>::get();
            count = field->count;
            break;
        case struct_field_info::POINTER:
            if (field->count & PTRFLAG_IS_ARRAY)
            {
                ptr_is_array = true;
            }
            identity = Checker::wrap_in_pointer(field->type);
            break;
        case struct_field_info::STATIC_ARRAY:
            if (!field->count || !field->type)
            {
                UNEXPECTED;
            }
            count = field->count;
            break;
        case struct_field_info::SUBSTRUCT:
        case struct_field_info::CONTAINER:
            if (field->count || !field->type)
            {
                UNEXPECTED;
            }
            break;
        case struct_field_info::STL_VECTOR_PTR:
            if (field->count)
            {
                UNEXPECTED;
            }
            identity = Checker::wrap_in_stl_ptr_vector(field->type);
            break;
        case struct_field_info::OBJ_METHOD:
        case struct_field_info::CLASS_METHOD:
            UNEXPECTED;
            break;
    }
}

size_t CheckedStructure::full_size() const
{
    size_t size = identity->byte_size();
    if (count)
    {
        size *= count;
    }

    return size;
}

bool CheckedStructure::has_type_at_offset(const CheckedStructure & type, size_t offset) const
{
    if (!identity)
        return false;

    if (offset == 0 && identity == type.identity)
        return true;

    if (offset >= identity->byte_size() && offset < full_size())
        offset %= identity->byte_size();
    else if (offset >= identity->byte_size())
        return false;

    if (identity->type() == IDTYPE_BUFFER)
    {
        auto target = static_cast<const container_identity *>(identity)->getItemType();
        return CheckedStructure(target, 0).has_type_at_offset(type, offset % target->byte_size());
    }

    auto st = dynamic_cast<const struct_identity *>(identity);
    if (!st)
    {
        return false;
    }

    for (auto p = st; p; p = p->getParent())
    {
        auto fields = p->getFields();
        if (!fields)
            continue;

        for (auto field = fields; field->mode != struct_field_info::END; field++)
        {
            if (field->mode == struct_field_info::OBJ_METHOD || field->mode == struct_field_info::CLASS_METHOD)
                continue;

            if (field->offset > offset)
                continue;

            if (CheckedStructure(field).has_type_at_offset(type, offset - field->offset))
                return true;
        }
    }

    return false;
}

const type_identity *Checker::wrap_in_stl_ptr_vector(const type_identity *base)
{
    static std::map<const type_identity *, std::unique_ptr<const df::stl_ptr_vector_identity>> wrappers;
    auto it = wrappers.find(base);
    if (it != wrappers.end())
    {
        return it->second.get();
    }
    return (wrappers[base] = std::make_unique<df::stl_ptr_vector_identity>(base, nullptr)).get();
}

const type_identity *Checker::wrap_in_pointer(const type_identity *base)
{
    static std::map<const type_identity *, std::unique_ptr<const df::pointer_identity>> wrappers;
    auto it = wrappers.find(base);
    if (it != wrappers.end())
    {
        return it->second.get();
    }
    return (wrappers[base] = std::make_unique<const df::pointer_identity>(base)).get();
}

std::map<size_t, std::vector<std::string>> known_types_by_size;
void build_size_table()
{
    for (auto & ident : compound_identity::getTopScope())
    {
        if (ident->byte_size() >= MIN_SIZE_FOR_SUGGEST)
        {
            known_types_by_size[ident->byte_size()].push_back(ident->getFullName());
        }
    }
}
