#pragma once

#include "ColorText.h"
#include "Export.h"
#include "RemoteClient.h"

namespace DFHack {
    DFHACK_EXPORT command_result export_datatypes
        (color_ostream &con, const std::vector<std::string> &args);
}
