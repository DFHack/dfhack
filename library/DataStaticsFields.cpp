#include "Internal.h"
#include "DataDefs.h"
#include "MiscUtils.h"
#include "VersionInfo.h"

#include "df/world.h"
#include "df/world_data.h"
#include "df/ui.h"

#include "DataIdentity.h"

#include <stddef.h>

#pragma GCC diagnostic ignored "-Winvalid-offsetof"

namespace df {
#define ATOM_IDENTITY_TRAITS(type) \
    primitive_identity identity_traits<type>::identity(sizeof(type));

    ATOM_IDENTITY_TRAITS(char);
    ATOM_IDENTITY_TRAITS(int8_t);
    ATOM_IDENTITY_TRAITS(uint8_t);
    ATOM_IDENTITY_TRAITS(int16_t);
    ATOM_IDENTITY_TRAITS(uint16_t);
    ATOM_IDENTITY_TRAITS(int32_t);
    ATOM_IDENTITY_TRAITS(uint32_t);
    ATOM_IDENTITY_TRAITS(int64_t);
    ATOM_IDENTITY_TRAITS(uint64_t);
    ATOM_IDENTITY_TRAITS(bool);
    ATOM_IDENTITY_TRAITS(float);
    ATOM_IDENTITY_TRAITS(std::string);
    ATOM_IDENTITY_TRAITS(void*);

#undef ATOM_IDENTITY_TRAITS
}

#define TID(type) (&identity_traits< type >::identity)

#define FLD(mode, name) struct_field_info::mode, #name, offsetof(CUR_STRUCT, name)
#define GFLD(mode, name) struct_field_info::mode, #name, 0
#define FLD_END struct_field_info::END

// Field definitions
#include "df/static.fields.inc"
