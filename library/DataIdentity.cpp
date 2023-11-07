#include <stddef.h>

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "DataFuncs.h"
#include "DataIdentity.h"

// the space after the uses of "type" in OPAQUE_IDENTITY_TRAITS_NAME is _required_
// without it the macro generates a syntax error when type is a template specification

namespace df {
#define NUMBER_IDENTITY_TRAITS(category, type, name) \
    category##_identity<type> identity_traits<type>::identity(name);
#define INTEGER_IDENTITY_TRAITS(type, name) NUMBER_IDENTITY_TRAITS(integer, type, name)
#define FLOAT_IDENTITY_TRAITS(type) NUMBER_IDENTITY_TRAITS(float, type, #type)
#define OPAQUE_IDENTITY_TRAITS_NAME(type, name) \
    opaque_identity identity_traits<type >::identity(sizeof(type), allocator_noassign_fn<type >, name)
#define STL_OPAQUE_IDENTITY_TRAITS(type) OPAQUE_IDENTITY_TRAITS_NAME(std::type, #type)

    INTEGER_IDENTITY_TRAITS(char,               "char");
    INTEGER_IDENTITY_TRAITS(signed char,        "int8_t");
    INTEGER_IDENTITY_TRAITS(unsigned char,      "uint8_t");
    INTEGER_IDENTITY_TRAITS(short,              "int16_t");
    INTEGER_IDENTITY_TRAITS(unsigned short,     "uint16_t");
    INTEGER_IDENTITY_TRAITS(int,                "int32_t");
    INTEGER_IDENTITY_TRAITS(unsigned int,       "uint32_t");
    INTEGER_IDENTITY_TRAITS(long,               "long");
    INTEGER_IDENTITY_TRAITS(unsigned long,      "unsigned long");
    INTEGER_IDENTITY_TRAITS(long long,          "int64_t");
    INTEGER_IDENTITY_TRAITS(unsigned long long, "uint64_t");
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

    STL_OPAQUE_IDENTITY_TRAITS(condition_variable);
    STL_OPAQUE_IDENTITY_TRAITS(fstream);
    STL_OPAQUE_IDENTITY_TRAITS(mutex);
    STL_OPAQUE_IDENTITY_TRAITS(future<void>);
    STL_OPAQUE_IDENTITY_TRAITS(function<void()>);
    STL_OPAQUE_IDENTITY_TRAITS(optional<std::function<void()> >);

    buffer_container_identity buffer_container_identity::base_instance;
}
