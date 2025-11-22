
#include "Commands.h"

#include "ColorText.h"
#include "Core.h"
#include "CoreDefs.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Gui.h"

#include <string>
#include <vector>
#include <ranges>


namespace DFHack
{
    command_result Commands::help(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts)
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

    command_result Commands::load(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts)
    {
        bool all = false;
        bool load = (first == "load");
        bool unload = (first == "unload");
        bool reload = (first == "reload");
        auto plug_mgr = core.getPluginManager();
        if (parts.size())
        {
            for (const auto& p : parts)
            {
                if (p.size() && p[0] == '-')
                {
                    if (p.find('a') != std::string::npos)
                        all = true;
                }
            }
            auto ret = CR_OK;
            if (all)
            {
                if (load && !plug_mgr->loadAll())
                    ret = CR_FAILURE;
                else if (unload && !plug_mgr->unloadAll())
                    ret = CR_FAILURE;
                else if (reload && !plug_mgr->reloadAll())
                    ret = CR_FAILURE;
            }
            else
            {
                for (auto& p : parts)
                {
                    if (p.empty() || p[0] == '-')
                        continue;
                    if (load && !plug_mgr->load(p))
                        ret = CR_FAILURE;
                    else if (unload && !plug_mgr->unload(p))
                        ret = CR_FAILURE;
                    else if (reload && !plug_mgr->reload(p))
                        ret = CR_FAILURE;
                }
            }
            if (ret != CR_OK)
                con.printerr("%s failed\n", first.c_str());
            return ret;
        }
        else
        {
            con.printerr("%s: no arguments\n", first.c_str());
            return CR_FAILURE;
        }

    }

    command_result Commands::enable(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts)
    {
        CoreSuspender suspend;
        bool enable = (first == "enable");
        auto plug_mgr = core.getPluginManager();

        if (parts.size())
        {
            command_result res{CR_FAILURE};

            for (auto& part_ : parts)
            {
                // have to copy to modify as passed argument is const
                std::string part(part_);

                if (has_backslashes(part))
                {
                    con.printerr("Replacing backslashes with forward slashes in \"%s\"\n", part.c_str());
                    replace_backslashes_with_forwardslashes(part);
                }

                auto alias = core.GetAliasCommand(part, true);

                Plugin* plug = (*plug_mgr)[alias];

                if (!plug)
                {
                    std::filesystem::path lua = core.findScript(part + ".lua");
                    if (!lua.empty())
                    {
                        res = core.enableLuaScript(con, part, enable);
                    }
                    else
                    {
                        res = CR_NOT_FOUND;
                        con.printerr("No such plugin or Lua script: %s\n", part.c_str());
                    }
                }
                else if (!plug->can_set_enabled())
                {
                    res = CR_NOT_IMPLEMENTED;
                    con.printerr("Cannot %s plugin: %s\n", first.c_str(), part.c_str());
                }
                else
                {
                    res = plug->set_enabled(con, enable);

                    if (res != CR_OK || plug->is_enabled() != enable)
                        con.printerr("Could not %s plugin: %s\n", first.c_str(), part.c_str());
                }
            }

            return res;
        }
        else
        {
            for (auto& [key, plug] : *plug_mgr)
            {
                if (!plug)
                    continue;
                if (!plug->can_be_enabled()) continue;

                con.print(
                    "%21s  %-3s%s\n",
                    (key + ":").c_str(),
                    plug->is_enabled() ? "on" : "off",
                    plug->can_set_enabled() ? "" : " (controlled internally)"
                );
            }

            Lua::CallLuaModuleFunction(con, "script-manager", "list");

            return CR_OK;
        }
    }

    command_result Commands::plug(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts)
    {
        constexpr auto header_format = "%30s %10s %4s %8s\n";
        constexpr auto row_format = "%30s %10s %4i %8s\n";

        con.print(header_format, "Name", "State", "Cmds", "Enabled");

        auto plug_mgr = core.getPluginManager();

        plug_mgr->refresh();
        for (auto& [key, plug] : *plug_mgr)
        {
            if (!plug)
                continue;

            if (parts.size() && std::ranges::find(parts, key) == parts.end())
                continue;

            color_value color;
            switch (plug->getState())
            {
            case Plugin::PS_LOADED:
                color = COLOR_RESET;
                break;
            case Plugin::PS_UNLOADED:
            case Plugin::PS_UNLOADING:
                color = COLOR_YELLOW;
                break;
            case Plugin::PS_LOADING:
                color = COLOR_LIGHTBLUE;
                break;
            case Plugin::PS_BROKEN:
                color = COLOR_LIGHTRED;
                break;
            default:
                color = COLOR_LIGHTMAGENTA;
                break;
            }
            con.color(color);
            con.print(row_format,
                plug->getName().c_str(),
                Plugin::getStateDescription(plug->getState()),
                plug->size(),
                (plug->can_be_enabled()
                    ? (plug->is_enabled() ? "enabled" : "disabled")
                    : "n/a")
            );
            con.color(COLOR_RESET);
        }

        return CR_OK;
    }

    command_result Commands::type(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts)
    {
        auto plug_mgr = core.getPluginManager();

        if (!parts.size())
        {
            con.printerr("type: no argument\n");
            return CR_WRONG_USAGE;
        }
        con << parts[0];
        bool builtin = is_builtin(con, parts[0]);
        std::filesystem::path lua_path = core.findScript(parts[0] + ".lua");
        Plugin* plug = plug_mgr->getPluginByCommand(parts[0]);
        if (builtin)
        {
            con << " is a built-in command";
            con << std::endl;
        }
        else if (core.IsAlias(parts[0]))
        {
            con << " is an alias: " << core.GetAliasCommand(parts[0]) << std::endl;
        }
        else if (plug)
        {
            con << " is a command implemented by the plugin " << plug->getName() << std::endl;
        }
        else if (!lua_path.empty())
        {
            con << " is a Lua script: " << lua_path << std::endl;
        }
        else
        {
            con << " is not a recognized command." << std::endl;
            plug = plug_mgr->getPluginByName(parts[0]);
            if (plug)
                con << "Plugin " << parts[0] << " exists and implements " << plug->size() << " commands." << std::endl;
            return CR_FAILURE;
        }
        return CR_OK;
    }

    command_result Commands::keybinding(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts)
    {
        if (parts.size() >= 3 && (parts[0] == "set" || parts[0] == "add"))
        {
            std::string keystr = parts[1];
            if (parts[0] == "set")
                core.ClearKeyBindings(keystr);
            // for (int i = parts.size()-1; i >= 2; i--)
            for (const auto& part : parts | std::views::drop(2) | std::views::reverse)
            {
                if (!core.AddKeyBinding(keystr, part))
                {
                    con.printerr("Invalid key spec: %s\n", keystr.c_str());
                    return CR_FAILURE;
                }
            }
        }
        else if (parts.size() >= 2 && parts[0] == "clear")
        {
            // for (size_t i = 1; i < parts.size(); i++)
            for (const auto& part : parts | std::views::drop(1))
            {
                if (!core.ClearKeyBindings(part))
                {
                    con.printerr("Invalid key spec: %s\n", part.c_str());
                    return CR_FAILURE;
                }
            }
        }
        else if (parts.size() == 2 && parts[0] == "list")
        {
            std::vector<std::string> list = core.ListKeyBindings(parts[1]);
            if (list.empty())
                con << "No bindings." << std::endl;
            for (const auto& kb : list)
                con << "  " << kb << std::endl;
        }
        else
        {
            con << "Usage:" << std::endl
                << "  keybinding list <key>" << std::endl
                << "  keybinding clear <key>[@context]..." << std::endl
                << "  keybinding set <key>[@context] \"cmdline\" \"cmdline\"..." << std::endl
                << "  keybinding add <key>[@context] \"cmdline\" \"cmdline\"..." << std::endl
                << "Later adds, and earlier items within one command have priority." << std::endl
                << "Supported keys: [Ctrl-][Alt-][Shift-](A-Z, 0-9, F1-F12, `, or Enter)." << std::endl
                << "Context may be used to limit the scope of the binding, by" << std::endl
                << "requiring the current context to have a certain prefix." << std::endl
                << "Current UI context is: " << std::endl
                << join_strings("\n", Gui::getCurFocus(true)) << std::endl;
        }

        return CR_OK;
    }

    command_result Commands::alias(color_ostream& con, Core& core, const std::string& first, const std::vector<std::string>& parts)
    {
        if (parts.size() >= 3 && (parts[0] == "add" || parts[0] == "replace"))
        {
            const std::string& name = parts[1];
            std::vector<std::string> cmd(parts.begin() + 2, parts.end());
            if (!core.AddAlias(name, cmd, parts[0] == "replace"))
            {
                con.printerr("Could not add alias %s - already exists\n", name.c_str());
                return CR_FAILURE;
            }
        }
        else if (parts.size() >= 2 && (parts[0] == "delete" || parts[0] == "clear"))
        {
            if (!core.RemoveAlias(parts[1]))
            {
                con.printerr("Could not remove alias %s\n", parts[1].c_str());
                return CR_FAILURE;
            }
        }
        else if (parts.size() >= 1 && (parts[0] == "list"))
        {
            auto aliases = core.ListAliases();
            for (auto p : aliases)
            {
                con << p.first << ": " << join_strings(" ", p.second) << std::endl;
            }
        }
        else
        {
            con << "Usage: " << std::endl
                << "  alias add|replace <name> <command...>" << std::endl
                << "  alias delete|clear <name> <command...>" << std::endl
                << "  alias list" << std::endl;
        }

        return CR_OK;
    }

}
