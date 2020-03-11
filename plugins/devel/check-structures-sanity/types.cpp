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
    identity(identity),
    count(count),
    eid(nullptr)
{
}
CheckedStructure::CheckedStructure(type_identity *identity, size_t count, enum_identity *eid) :
    identity(identity),
    count(count),
    eid(eid)
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
            // TODO: check flags (stored in field->count)
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

#define RETURN_CACHED_WRAPPER(T, base, ...) \
    static std::map<type_identity *, std::unique_ptr<T>> wrappers; \
    auto it = wrappers.find(base); \
    if (it != wrappers.end()) \
    { \
        return it->second.get(); \
    } \
    return (wrappers[base] = dts::make_unique<T>(base __VA_OPT__(,) __VA_ARGS__)).get()

type_identity *Checker::wrap_in_stl_ptr_vector(type_identity *base)
{
    RETURN_CACHED_WRAPPER(df::stl_ptr_vector_identity, base, nullptr);
}

type_identity *Checker::wrap_in_pointer(type_identity *base)
{
    RETURN_CACHED_WRAPPER(df::pointer_identity, base);
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
