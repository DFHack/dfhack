#pragma once
#include "Export.h"
#include <string>

namespace DFHack {
    namespace Once {
        DFHACK_EXPORT bool alreadyDone(std::string);
        DFHACK_EXPORT bool doOnce(std::string);
    }
}

