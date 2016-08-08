#include <stddef.h>

#ifndef STATIC_FIELDS_GROUP
#include "DataDefs.h"
#endif

#include "DataFuncs.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

namespace df {
#define NUMBER_IDENTITY_TRAITS(category, type) \
    category##_identity<type> identity_traits<type>::identity(#type);
#define INTEGER_IDENTITY_TRAITS(type) NUMBER_IDENTITY_TRAITS(integer, type)
#define FLOAT_IDENTITY_TRAITS(type) NUMBER_IDENTITY_TRAITS(float, type)

#ifndef STATIC_FIELDS_GROUP
    INTEGER_IDENTITY_TRAITS(char);
    INTEGER_IDENTITY_TRAITS(signed char);
    INTEGER_IDENTITY_TRAITS(unsigned char);
    INTEGER_IDENTITY_TRAITS(short);
    INTEGER_IDENTITY_TRAITS(unsigned short);
    INTEGER_IDENTITY_TRAITS(int);
    INTEGER_IDENTITY_TRAITS(unsigned int);
    INTEGER_IDENTITY_TRAITS(long);
    INTEGER_IDENTITY_TRAITS(unsigned long);
    INTEGER_IDENTITY_TRAITS(long long);
    INTEGER_IDENTITY_TRAITS(unsigned long long);
    FLOAT_IDENTITY_TRAITS(float);
    FLOAT_IDENTITY_TRAITS(double);

    bool_identity identity_traits<bool>::identity;
    stl_string_identity identity_traits<std::string>::identity;
    ptr_string_identity identity_traits<char*>::identity;
    ptr_string_identity identity_traits<const char*>::identity;
    pointer_identity identity_traits<void*>::identity;
    stl_ptr_vector_identity identity_traits<std::vector<void*> >::identity;
    stl_bit_vector_identity identity_traits<std::vector<bool> >::identity;
    bit_array_identity identity_traits<BitArray<int> >::identity;

    static void *fstream_allocator_fn(void *out, const void *in) {
        if (out) { /* *(T*)out = *(const T*)in;*/ return NULL; }
        else if (in) { delete (std::fstream*)in; return (std::fstream*)in; }
        else return new std::fstream();
    }
    opaque_identity identity_traits<std::fstream>::identity(
        sizeof(std::fstream), fstream_allocator_fn, "fstream");

    buffer_container_identity buffer_container_identity::base_instance;
#endif
#undef NUMBER_IDENTITY_TRAITS
#undef INTEGER_IDENTITY_TRAITS
#undef FLOAT_IDENTITY_TRAITS
}

#define TID(type) (&identity_traits< type >::identity)

#define FLD(mode, name) struct_field_info::mode, #name, offsetof(CUR_STRUCT, name)
#define GFLD(mode, name) struct_field_info::mode, #name, (size_t)&df::global::name
#define METHOD(mode, name) struct_field_info::mode, #name, 0, wrap_function(&CUR_STRUCT::name)
#define FLD_END struct_field_info::END
