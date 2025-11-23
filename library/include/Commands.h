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
        command_result fpause(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result clear(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result kill_lua(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result script(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result show(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result hide(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result sc_script(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
        command_result dump_rpc(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts);
    }
}
