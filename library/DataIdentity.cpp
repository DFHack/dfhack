#include <stddef.h>

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#include <variant>

#include "DataFuncs.h"
#include "DataIdentity.h"

// the space after the uses of "type" in OPAQUE_IDENTITY_TRAITS_NAME is _required_
// without it the macro generates a syntax error when type is a template specification

namespace df {
#define NUMBER_IDENTITY_TRAITS(category, type, name) \
    category##_identity<type> identity_traits<type>::identity(name);
#define INTEGER_IDENTITY_TRAITS(type, name) NUMBER_IDENTITY_TRAITS(integer, type, name)
#define FLOAT_IDENTITY_TRAITS(type) NUMBER_IDENTITY_TRAITS(float, type, #type)
#define OPAQUE_IDENTITY_TRAITS_NAME(name, ...) \
    opaque_identity identity_traits<__VA_ARGS__ >::identity(sizeof(__VA_ARGS__), allocator_noassign_fn<__VA_ARGS__ >, name)
#define OPAQUE_IDENTITY_TRAITS(...) OPAQUE_IDENTITY_TRAITS_NAME(#__VA_ARGS__, __VA_ARGS__ )

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

    OPAQUE_IDENTITY_TRAITS(std::condition_variable);
    OPAQUE_IDENTITY_TRAITS(std::fstream);
    OPAQUE_IDENTITY_TRAITS(std::mutex);
    OPAQUE_IDENTITY_TRAITS(std::future<void>);
    OPAQUE_IDENTITY_TRAITS(std::function<void()>);
    OPAQUE_IDENTITY_TRAITS(std::function<bool()>);
    OPAQUE_IDENTITY_TRAITS(std::function<int*()>);
    OPAQUE_IDENTITY_TRAITS(std::function<std::string()>);
    OPAQUE_IDENTITY_TRAITS(std::optional<std::function<void()> >);
    OPAQUE_IDENTITY_TRAITS(std::variant<std::string, std::function<void()> >);
    OPAQUE_IDENTITY_TRAITS(std::weak_ptr<df::widget_container>);

    buffer_container_identity buffer_container_identity::base_instance;
}
