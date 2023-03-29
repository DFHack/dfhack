#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "StockpileUtils.h"
#include "StockpileSerializer.h"

#include "modules/Filesystem.h"

#include "df/building.h"
#include "df/building_stockpilest.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("stockpiles");

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(stockpiles, log, DebugCategory::LINFO);
}

static command_result do_command(color_ostream &out, vector<string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(log,out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        plugin_name,
        "Import, export, or modify stockpile settings and features.",
        do_command));

    return CR_OK;
}

static bool call_stockpiles_lua(color_ostream *out, const char *fn_name,
        int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA) {
    DEBUG(log).print("calling stockpiles lua function: '%s'\n", fn_name);

    CoreSuspender guard;

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!out)
        out = &Core::getInstance().getConsole();

    return Lua::CallLuaModuleFunction(*out, L, "plugins.stockpiles", fn_name,
            nargs, nres,
            std::forward<Lua::LuaLambda&&>(args_lambda),
            std::forward<Lua::LuaLambda&&>(res_lambda));
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    bool show_help = false;
    if (!call_stockpiles_lua(&out, "parse_commandline", 1, 1,
            [&](lua_State *L) {
                Lua::PushVector(L, parameters);
            },
            [&](lua_State *L) {
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
        if (!cereal.serialize_to_file(fname, includedElements)) {
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
        if (!cereal.unserialize_from_file(fname, mode, filters)) {
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
    DFHACK_LUA_END
};
