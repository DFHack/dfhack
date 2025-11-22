#pragma once

#include "ColorText.h"
#include "CoreDefs.h"
#include "Core.h"

#include <string>
#include <vector>

namespace DFHack
{
    namespace Commands
    {
        command_result help(color_ostream& con, const std::string& first, const std::vector<std::string>& parts);
    }
}
