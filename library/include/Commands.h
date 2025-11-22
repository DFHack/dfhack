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
        command_result help(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result load(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result enable(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result plug(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result type(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result keybinding(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result alias(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);

    }
}
