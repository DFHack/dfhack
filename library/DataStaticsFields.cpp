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

#define TID(type) (&identity_traits< type >::identity)

#define FLD(mode, name) struct_field_info::mode, #name, offsetof(CUR_STRUCT, name)
#define GFLD(mode, name) struct_field_info::mode, #name, 0
#define FLD_END struct_field_info::END

// Field definitions
#include "df/static.fields.inc"
