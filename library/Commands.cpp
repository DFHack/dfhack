
#include "ColorText.h"
#include "Commands.h"
#include "Core.h"
#include "CoreDefs.h"

#include <string>
#include <vector>

namespace DFHack
{
    command_result Commands::help(color_ostream& con, const std::string& first, const std::vector<std::string>& parts)
    {
        if (!parts.size())
        {
            if (con.is_console())
            {
                con.print("This is the DFHack console. You can type commands in and manage DFHack plugins from it.\n"
                    "Some basic editing capabilities are included (single-line text editing).\n"
                    "The console also has a command history - you can navigate it with Up and Down keys.\n"
                    "On Windows, you may have to resize your console window. The appropriate menu is accessible\n"
                    "by clicking on the program icon in the top bar of the window.\n\n");
            }
            con.print("Here are some basic commands to get you started:\n"
                "  help|?|man         - This text.\n"
                "  help <tool>        - Usage help for the given plugin, command, or script.\n"
                "  tags               - List the tags that the DFHack tools are grouped by.\n"
                "  ls|dir [<filter>]  - List commands, optionally filtered by a tag or substring.\n"
                "                       Optional parameters:\n"
                "                         --notags: skip printing tags for each command.\n"
                "                         --dev:  include commands intended for developers and modders.\n"
                "  cls|clear          - Clear the console.\n"
                "  fpause             - Force DF to pause.\n"
                "  die                - Force DF to close immediately, without saving.\n"
                "  keybinding         - Modify bindings of commands to in-game key shortcuts.\n"
                "\n"
                "See more commands by running 'ls'.\n\n"
            );

            con.print("DFHack version %s\n", dfhack_version_desc().c_str());
        }
        else
        {
            DFHack::help_helper(con, parts[0]);
        }
        return CR_OK;
    }

}
