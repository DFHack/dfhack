#include <stddef.h>

#include "DataFuncs.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

#define TID(type) (&identity_traits< type >::identity)

#define FLD(mode, name) struct_field_info::mode, #name, offsetof(CUR_STRUCT, name)
#define GFLD(mode, name) struct_field_info::mode, #name, (size_t)&df::global::name
#define METHOD(mode, name) struct_field_info::mode, #name, 0, wrap_function(&CUR_STRUCT::name)
#define METHOD_N(mode, func, name) struct_field_info::mode, #name, 0, wrap_function(&CUR_STRUCT::func)
#define FLD_END struct_field_info::END
