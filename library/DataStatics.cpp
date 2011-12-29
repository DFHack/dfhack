#include "Internal.h"
#include "dfhack/DataDefs.h"
#include "dfhack/MiscUtils.h"

#include "dfhack/df/world.h"
#include "dfhack/df/world_data.h"
#include "dfhack/df/ui.h"

namespace {
    template<class T>
    inline T &_toref(T &r) { return r; }
    template<class T>
    inline T &_toref(T *&p) { return *p; }
}

// Instantiate all the static objects
#include "dfhack/df/static.inc"
#include "dfhack/df/static.enums.inc"
