#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include <csignal>
#include <map>
#include <vector>
#include "modules/Gui.h"
#include "modules/World.h"
#include "df/enabler.h"

using namespace DFHack;

DFHACK_PLUGIN("zoom");
REQUIRE_GLOBAL(enabler);

command_result df_zoom (color_ostream &out, std::vector <std::string> & parameters);
command_result df_gzoom (color_ostream &out, std::vector <std::string> & parameters);

std::map<std::string, df::zoom_commands> zcmap;
DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("zoom", "adjust screen zoom", df_zoom, false, "zoom [command]"));
    commands.push_back(PluginCommand("gzoom", "zoom to grid location", df_gzoom, false, "gzoom x y z"));
    zcmap["in"] = df::zoom_commands::zoom_in;
    zcmap["out"] = df::zoom_commands::zoom_out;
    zcmap["fullscreen"] = df::zoom_commands::zoom_fullscreen;
    zcmap["reset"] = df::zoom_commands::zoom_reset;
    zcmap["resetgrid"] = df::zoom_commands::zoom_resetgrid;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

command_result df_zoom (color_ostream &out, std::vector <std::string> & parameters)
{
    if (parameters.size() < 1)
        return CR_WRONG_USAGE;
    if (zcmap.find(parameters[0]) == zcmap.end())
    {
        out.printerr("Unrecognized zoom command: %s\n", parameters[0].c_str());
        out.print("Valid commands:");
        for (auto it = zcmap.begin(); it != zcmap.end(); ++it)
        {
            out << " " << it->first;
        }
        out.print("\n");
        return CR_FAILURE;
    }
    df::zoom_commands cmd = zcmap[parameters[0]];
    enabler->zoom_display(cmd);
    if (cmd == df::zoom_commands::zoom_fullscreen)
        enabler->fullscreen = !enabler->fullscreen;
    return CR_OK;
}

command_result df_gzoom (color_ostream &out, std::vector<std::string> & parameters)
{
    if(parameters.size() < 3)
        return CR_WRONG_USAGE;
    if (!World::isFortressMode())
    {
        out.printerr("Not fortress mode\n");
        return CR_FAILURE;
    }
    int x = atoi( parameters[0].c_str());
    int y = atoi( parameters[1].c_str());
    int z = atoi( parameters[2].c_str());
    int xi, yi, zi;
    CoreSuspender cs;
    if(Gui::getCursorCoords(xi, yi, zi))
    {
        Gui::setCursorCoords(x,y,z);
    }
    Gui::setViewCoords(x,y,z);
}

