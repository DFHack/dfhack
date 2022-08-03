// This template is appropriate for plugins that simply provide one or more
// commands, but don't need to be "enabled" to function.

#include <string>
#include <vector>

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("simple_command_example");

namespace DFHack {
    DBG_DECLARE(simple_command_example, log);
}

// define the structure that will represent the possible commandline options
struct command_options {
    // whether to display help
    bool help = false;

    // example params of different types
    int32_t ticks = -1;
    df::coord start;
    string format;
    vector<string*> list; // note this must be a vector of pointers, not objects

    static struct_identity _identity;
};
static const struct_field_info command_options_fields[] = {
    { struct_field_info::PRIMITIVE,      "help",   offsetof(command_options, help),  &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE,      "ticks",  offsetof(command_options, ticks), &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::SUBSTRUCT,      "start",  offsetof(command_options, start), &df::coord::_identity,                   0, 0 },
    { struct_field_info::PRIMITIVE,      "format", offsetof(command_options, format), df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::STL_VECTOR_PTR, "list",   offsetof(command_options, list),   df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::END }
};
struct_identity command_options::_identity(sizeof(command_options), &df::allocator_fn<command_options>, NULL, "command_options", NULL, command_options_fields);

static command_result do_command(color_ostream &out, vector<string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(log,out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        plugin_name,
        "Short (~54 character) description of command.",
        do_command));

    return CR_OK;
}

// load the lua module associated with the plugin and parse the commandline
// in lua (which has better facilities than C++ for string parsing).
static bool get_options(color_ostream &out,
                        command_options &opts,
                        const vector<string> &parameters)
{
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, parameters.size() + 2) ||
        !Lua::PushModulePublic(
            out, L, ("plugins." + string(plugin_name)).c_str(),
            "parse_commandline")) {
        out.printerr("Failed to load %s Lua code\n", plugin_name);
        return false;
    }

    Lua::Push(L, &opts);
    for (const string &param : parameters)
        Lua::Push(L, param);

    if (!Lua::SafeCall(out, L, parameters.size() + 1, 0))
        return false;

    return true;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    // be sure to suspend the core if any DF state is read or modified
    CoreSuspender suspend;

    command_options opts;
    if (!get_options(out, opts, parameters) || opts.help)
        return CR_WRONG_USAGE;

    // TODO: command logic

    return CR_OK;
}
