#include "Core.h"
#include "Internal.h"
#include "DataDefs.h"
#include "MiscUtils.h"
#include "VersionInfo.h"

#include "df/world.h"
#include "df/world_data.h"
#include "df/plotinfost.h"

#include "DataIdentity.h"

namespace {
    template<class T>
    inline T *_toptr(T &r) { return &r; }
    template<class T>
    inline T *_toptr(T *&p) { return p; }
}
#define _fieldptr(ptr, fn) (ptr) ? _toptr((ptr)->fn) : NULL

#define INIT_GLOBAL_FUNCTION_PREFIX \
    DFHack::VersionInfo *global_table_ = DFHack::Core::getInstance().vinfo.get(); \
    void * tmp_;

#define INIT_GLOBAL_FUNCTION_ITEM(type,name) \
    if (global_table_->getAddress(#name,tmp_)) name = (type*)tmp_;

#define TID(type) (&identity_traits< type >::identity)

// Instantiate all the static objects
#include "df/static.inc"
#include "df/static.enums.inc"
