#include "Internal.h"
#include "DataDefs.h"
#include "MiscUtils.h"

#include "df/world.h"
#include "df/world_data.h"
#include "df/ui.h"

namespace {
    template<class T>
    inline T &_toref(T &r) { return r; }
    template<class T>
    inline T &_toref(T *&p) { return *p; }
}

// Instantiate all the static objects
#include "df/static.inc"
#include "df/static.enums.inc"
