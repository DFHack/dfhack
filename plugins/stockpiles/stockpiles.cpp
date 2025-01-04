#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"
#include "StockpileUtils.h"
#include "StockpileSerializer.h"

#include "modules/Filesystem.h"

#include "df/building.h"
#include "df/building_stockpilest.h"
#include "df/hauling_route.h"
#include "df/hauling_stop.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("stockpiles");

REQUIRE_GLOBAL(world);

namespace DFHack
{
DBG_DECLARE(stockpiles, log, DebugCategory::LINFO);
}

static command_result do_command(color_ostream& out, vector<string>& parameters);

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    DEBUG(log, out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        plugin_name,
        "Import, export, or modify stockpile settings.",
        do_command));

    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.stockpiles", "parse_commandline", std::make_tuple(parameters),
            1, [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

/////////////////////////////////////////////////////
// Lua API
//

static df::building_stockpilest* get_stockpile(int id) {
    return virtual_cast<df::building_stockpilest>(df::building::find(id));
}

static bool stockpiles_export(color_ostream& out, string fname, int id, uint32_t includedElements) {
    df::building_stockpilest* sp = get_stockpile(id);
    if (!sp) {
        out.printerr("Specified building isn't a stockpile: %d.\n", id);
        return false;
    }

    if (!is_dfstockfile(fname))
        fname += ".dfstock";

    try {
        StockpileSerializer cereal(sp);
        if (!cereal.serialize_to_file(out, fname, includedElements)) {
            out.printerr("could not save to '%s'\n", fname.c_str());
            return false;
        }
    }
    catch (std::exception& e) {
        out.printerr("serialization failed: protobuf exception: %s\n", e.what());
        return false;
    }

    return true;
}

static bool stockpiles_import(color_ostream& out, string fname, int id, string mode_str, string filter) {
    df::building_stockpilest* sp = get_stockpile(id);
    if (!sp) {
        out.printerr("Specified building isn't a stockpile: %d.\n", id);
        return false;
    }

    if (!is_dfstockfile(fname))
        fname += ".dfstock";

    if (!Filesystem::exists(fname)) {
        out.printerr("ERROR: file doesn't exist: '%s'\n", fname.c_str());
        return false;
    }

    DeserializeMode mode = DESERIALIZE_MODE_SET;
    if (mode_str == "enable")
        mode = DESERIALIZE_MODE_ENABLE;
    else if (mode_str == "disable")
        mode = DESERIALIZE_MODE_DISABLE;

    vector<string> filters;
    split_string(&filters, filter, ",", true);

    try {
        StockpileSerializer cereal(sp);
        if (!cereal.unserialize_from_file(out, fname, mode, filters)) {
            out.printerr("deserialization failed: '%s'\n", fname.c_str());
            return false;
        }
    }
    catch (std::exception& e) {
        out.printerr("deserialization failed: protobuf exception: %s\n", e.what());
        return false;
    }

    return true;
}

static df::stockpile_settings * get_stop_settings(color_ostream& out, int route_id, int stop_id) {
    auto route = df::hauling_route::find(route_id);
    if (!route) {
        out.printerr("Specified hauling route not found: %d.\n", route_id);
        return NULL;
    }

    df::hauling_stop *stop = binsearch_in_vector(route->stops, &df::hauling_stop::id, stop_id);
    if (!stop) {
        out.printerr("Specified hauling stop not found in route %d: %d.\n", route_id, stop_id);
        return NULL;
    }

    return &stop->settings;
}

static bool stockpiles_route_export(color_ostream& out, string fname, int route_id, int stop_id, uint32_t includedElements) {
    auto settings = get_stop_settings(out, route_id, stop_id);
    if (!settings)
        return false;

    if (!is_dfstockfile(fname))
        fname += ".dfstock";

    try {
        StockpileSettingsSerializer cereal(settings);
        if (!cereal.serialize_to_file(out, fname, includedElements)) {
            out.printerr("could not save to '%s'\n", fname.c_str());
            return false;
        }
    }
    catch (std::exception& e) {
        out.printerr("serialization failed: protobuf exception: %s\n", e.what());
        return false;
    }

    return true;
}

static bool stockpiles_route_import(color_ostream& out, string fname, int route_id, int stop_id, string mode_str, string filter) {
    auto settings = get_stop_settings(out, route_id, stop_id);
    if (!settings)
        return false;

    if (!is_dfstockfile(fname))
        fname += ".dfstock";

    if (!Filesystem::exists(fname)) {
        out.printerr("ERROR: file doesn't exist: '%s'\n", fname.c_str());
        return false;
    }

    DeserializeMode mode = DESERIALIZE_MODE_SET;
    if (mode_str == "enable")
        mode = DESERIALIZE_MODE_ENABLE;
    else if (mode_str == "disable")
        mode = DESERIALIZE_MODE_DISABLE;

    vector<string> filters;
    split_string(&filters, filter, ",", true);

    try {
        StockpileSettingsSerializer cereal(settings);
        if (!cereal.unserialize_from_file(out, fname, mode, filters)) {
            out.printerr("deserialization failed: '%s'\n", fname.c_str());
            return false;
        }
    }
    catch (std::exception& e) {
        out.printerr("deserialization failed: protobuf exception: %s\n", e.what());
        return false;
    }

    return true;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(stockpiles_export),
    DFHACK_LUA_FUNCTION(stockpiles_import),
    DFHACK_LUA_FUNCTION(stockpiles_route_export),
    DFHACK_LUA_FUNCTION(stockpiles_route_import),
    DFHACK_LUA_END
};
