#include "Internal.h"
#include "DataDefs.h"
#include "MiscUtils.h"
#include "VersionInfo.h"

#include "df/world.h"
#include "df/world_data.h"
#include "df/ui.h"

#include "DataIdentity.h"
#include "DataFuncs.h"

#include <stddef.h>

#pragma GCC diagnostic ignored "-Winvalid-offsetof"

namespace df {
#define NUMBER_IDENTITY_TRAITS(type) \
    number_identity<type> identity_traits<type>::identity(#type);

    NUMBER_IDENTITY_TRAITS(char);
    NUMBER_IDENTITY_TRAITS(int8_t);
    NUMBER_IDENTITY_TRAITS(uint8_t);
    NUMBER_IDENTITY_TRAITS(int16_t);
    NUMBER_IDENTITY_TRAITS(uint16_t);
    NUMBER_IDENTITY_TRAITS(int32_t);
    NUMBER_IDENTITY_TRAITS(uint32_t);
    NUMBER_IDENTITY_TRAITS(int64_t);
    NUMBER_IDENTITY_TRAITS(uint64_t);
    NUMBER_IDENTITY_TRAITS(float);
    NUMBER_IDENTITY_TRAITS(double);

    bool_identity identity_traits<bool>::identity;
    stl_string_identity identity_traits<std::string>::identity;
    ptr_string_identity identity_traits<char*>::identity;
    ptr_string_identity identity_traits<const char*>::identity;
    pointer_identity identity_traits<void*>::identity;
    stl_ptr_vector_identity identity_traits<std::vector<void*> >::identity;
    stl_bit_vector_identity identity_traits<std::vector<bool> >::identity;
    bit_array_identity identity_traits<BitArray<int> >::identity;

    buffer_container_identity buffer_container_identity::base_instance;

#undef NUMBER_IDENTITY_TRAITS
}

#define TID(type) (&identity_traits< type >::identity)

#define FLD(mode, name) struct_field_info::mode, #name, offsetof(CUR_STRUCT, name)
#define GFLD(mode, name) struct_field_info::mode, #name, (size_t)&df::global::name
#define METHOD(mode, name) struct_field_info::mode, #name, 0, wrap_function(&CUR_STRUCT::name)
#define FLD_END struct_field_info::END

// Field definitions
#include "df/static.fields.inc"
