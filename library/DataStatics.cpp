#include "Internal.h"
#include "DataDefs.h"
#include "MiscUtils.h"
#include "VersionInfo.h"

#include "df/world.h"
#include "df/world_data.h"
#include "df/ui.h"

namespace {
    template<class T>
    inline T &_toref(T &r) { return r; }
    template<class T>
    inline T &_toref(T *&p) { return *p; }
}

#define INIT_GLOBAL_FUNCTION_PREFIX \
    DFHack::OffsetGroup *global_table_ = DFHack::Core::getInstance().vinfo->getGroup("global"); \
    void * tmp_;

#define INIT_GLOBAL_FUNCTION_ITEM(type,name) \
    if (global_table_->getSafeAddress(#name,tmp_)) name = (type*)tmp_;

// Instantiate all the static objects
#include "df/static.inc"
#include "df/static.enums.inc"
