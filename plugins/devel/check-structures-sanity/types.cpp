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
CheckedStructure::CheckedStructure(type_identity *identity, size_t count) :
    CheckedStructure(identity, count, nullptr, false)
{
}
CheckedStructure::CheckedStructure(type_identity *identity, size_t count, enum_identity *eid, bool inside_structure) :
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
    eid = field->eid;
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

type_identity *Checker::wrap_in_stl_ptr_vector(type_identity *base)
{
    static std::map<type_identity *, std::unique_ptr<df::stl_ptr_vector_identity>> wrappers;
    auto it = wrappers.find(base);
    if (it != wrappers.end())
    {
        return it->second.get();
    }
    return (wrappers[base] = dts::make_unique<df::stl_ptr_vector_identity>(base, nullptr)).get();
}

type_identity *Checker::wrap_in_pointer(type_identity *base)
{
    static std::map<type_identity *, std::unique_ptr<df::pointer_identity>> wrappers;
    auto it = wrappers.find(base);
    if (it != wrappers.end())
    {
        return it->second.get();
    }
    return (wrappers[base] = dts::make_unique<df::pointer_identity>(base)).get();
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
