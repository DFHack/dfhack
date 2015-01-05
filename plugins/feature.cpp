// Map feature manager - list features and discover/undiscover individual ones

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/feature_init.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("feature");
REQUIRE_GLOBAL(world);


static command_result feature(color_ostream &out, vector <string> &parameters)
{
    CoreSuspender suspend;

    if (parameters.empty())
        return CR_WRONG_USAGE;

    string cmd = parameters[0];

    if (cmd == "list")
    {
        if (parameters.size() != 1)
            return CR_WRONG_USAGE;
        for (size_t i = 0; i < world->features.map_features.size(); i++)
        {
            df::feature_init *feature_init = world->features.map_features[i];
            string name;
            feature_init->getName(&name);
            out.print("Feature #%i (\"%s\", type %s) is %s\n",
                      i, name.c_str(), ENUM_KEY_STR(feature_type, feature_init->getType()).c_str(),
                      feature_init->flags.is_set(feature_init_flags::Discovered) ? "discovered" : "hidden");
        }
    }
    else if(cmd == "show")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;
        size_t i = atoi(parameters[1].c_str());
        if ((i < 0) || (i >= world->features.map_features.size()))
        {
            out.print("No such feature!\n");
            return CR_FAILURE;
        }
        df::feature_init *feature_init = world->features.map_features[i];
        if (feature_init->flags.is_set(feature_init_flags::Discovered))
        {
            out.print("Selected feature is already discovered!\n");
            return CR_OK;
        }
        feature_init->flags.set(feature_init_flags::Discovered);
        string name;
        feature_init->getName(&name);
        out.print("Feature #%i (\"%s\", type %s) is now discovered\n",
                  i, name.c_str(), ENUM_KEY_STR(feature_type, feature_init->getType()).c_str());
    }
    else if(cmd == "hide")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;
        size_t i = atoi(parameters[1].c_str());
        if ((i < 0) || (i >= world->features.map_features.size()))
        {
            out.print("No such feature!\n");
            return CR_FAILURE;
        }
        df::feature_init *feature_init = world->features.map_features[i];
        if (!feature_init->flags.is_set(feature_init_flags::Discovered))
        {
            out.print("Selected feature is already hidden!\n");
            return CR_OK;
        }
        feature_init->flags.clear(feature_init_flags::Discovered);
        string name;
        feature_init->getName(&name);
        out.print("Feature #%i (\"%s\", type %s) is now hidden\n",
                  i, name.c_str(), ENUM_KEY_STR(feature_type, feature_init->getType()).c_str());
    }
    else return CR_WRONG_USAGE;

    return CR_OK;
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "feature", "List or manage map features.", feature, false,
        "  feature list\n"
        "    Lists all map features in the region.\n"
        "  feature show <ID>\n"
        "    Marks the specified map feature as discovered.\n"
        "  feature hide <ID>\n"
        "    Marks the specified map feature as undiscovered.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}