// A container for random minor tweaks that don't fit anywhere else,
// in order to avoid creating lots of plugins and polluting the namespace.

#include "LuaTools.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

DFHACK_PLUGIN("tweak");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

using std::vector;
using std::string;
using namespace DFHack;

#include "tweaks/adamantine-cloth-wear.h"
#include "tweaks/craft-age-wear.h"
#include "tweaks/eggs-fertile.h"
#include "tweaks/fast-heat.h"
#include "tweaks/flask-contents.h"
#include "tweaks/named-codices.h"
#include "tweaks/partial-items.h"
#include "tweaks/reaction-gloves.h"
#include "tweaks/material-size-for-melting.h"

class tweak_onupdate_hookst {
public:
    typedef void(*T_callback)(void);
    tweak_onupdate_hookst(std::string name_, T_callback cb)
        : enabled(false), name(name_), callback(cb) {}
    bool enabled;
    const std::string name;
    const T_callback callback;
};
static command_result tweak(color_ostream &out, vector<string> & parameters);
static std::multimap<string, VMethodInterposeLinkBase> tweak_hooks;
static std::multimap<string, tweak_onupdate_hookst> tweak_onupdate_hooks;

#define TWEAK_HOOK(tweak, cls, func) tweak_hooks.insert(std::pair<std::string, VMethodInterposeLinkBase>\
    (tweak, INTERPOSE_HOOK(cls, func)))
#define TWEAK_ONUPDATE_HOOK(tweak, func) tweak_onupdate_hooks.insert(\
    std::pair<std::string, tweak_onupdate_hookst>(tweak, tweak_onupdate_hookst(#func, func)))

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "tweak",
        "A collection of tweaks and bugfixes.",
        tweak));

    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_armor_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_helm_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_gloves_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_shoes_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_pants_hook, incWearTimer);

    TWEAK_HOOK("craft-age-wear", craft_age_wear_hook, ageItem);

    TWEAK_HOOK("eggs-fertile", eggs_fertile_hook, getItemDescription);

    TWEAK_HOOK("fast-heat", fast_heat_hook, updateTempFromMap);
    TWEAK_HOOK("fast-heat", fast_heat_hook, updateTemperature);
    TWEAK_HOOK("fast-heat", fast_heat_hook, adjustTemperature);

    TWEAK_HOOK("flask-contents", flask_contents_hook, getItemDescription);

    TWEAK_HOOK("named-codices", book_hook, getItemDescription);

    TWEAK_HOOK("partial-items", partial_items_hook_bar, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_drink, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_glob, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_liquid_misc, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_powder_misc, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_cloth, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_sheet, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_thread, getItemDescription);

    TWEAK_HOOK("reaction-gloves", reaction_gloves_hook, produce);

    TWEAK_HOOK("material-size-for-melting", material_size_for_melting_armor_hook, getMaterialSizeForMelting);
    TWEAK_HOOK("material-size-for-melting", material_size_for_melting_gloves_hook, getMaterialSizeForMelting);
    TWEAK_HOOK("material-size-for-melting", material_size_for_melting_shoes_hook, getMaterialSizeForMelting);
    TWEAK_HOOK("material-size-for-melting", material_size_for_melting_helm_hook, getMaterialSizeForMelting);
    TWEAK_HOOK("material-size-for-melting", material_size_for_melting_pants_hook, getMaterialSizeForMelting);
    TWEAK_HOOK("material-size-for-melting", material_size_for_melting_weapon_hook, getMaterialSizeForMelting);
    TWEAK_HOOK("material-size-for-melting", material_size_for_melting_trapcomp_hook, getMaterialSizeForMelting);
    TWEAK_HOOK("material-size-for-melting", material_size_for_melting_tool_hook, getMaterialSizeForMelting);
    rng.init();

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    for (auto &entry : tweak_onupdate_hooks) {
        tweak_onupdate_hookst &hook = entry.second;
        if (hook.enabled)
            hook.callback();
    }
    return CR_OK;
}

static void enable_hook(color_ostream &out, VMethodInterposeLinkBase &hook, vector<string> &parameters) {
    bool disable = vector_get(parameters, 1) == "disable" || vector_get(parameters, 2) == "disable";
    bool quiet = vector_get(parameters, 1) == "quiet" || vector_get(parameters, 2) == "quiet";
    if (disable) {
        hook.remove();
        if (!quiet)
            out.print("Disabled tweak %s (%s)\n", parameters[0].c_str(), hook.name());
    } else {
        if (hook.apply()) {
            if (!quiet)
                out.print("Enabled tweak %s (%s)\n", parameters[0].c_str(), hook.name());
        }
        else
            out.printerr("Could not activate tweak %s (%s)\n", parameters[0].c_str(), hook.name());
    }
}

static command_result enable_tweak(string tweak, color_ostream &out, vector<string> &parameters) {
    bool recognized = false;
    string cmd = parameters[0];
    for (auto &entry : tweak_hooks) {
        if (entry.first == cmd) {
            recognized = true;
            enable_hook(out, entry.second, parameters);
        }
    }
    is_enabled = false;
    for (auto & entry : tweak_onupdate_hooks) {
        tweak_onupdate_hookst &hook = entry.second;
        if (entry.first == cmd) {
            bool state = (vector_get(parameters, 1) != "disable");
            recognized = true;
            hook.enabled = state;
            out.print("%s tweak %s (%s)\n", state ? "Enabled" : "Disabled", cmd.c_str(), hook.name.c_str());
        }
        if (hook.enabled)
            is_enabled = true;
    }
    if (!recognized) {
        out.printerr("Unrecognized tweak: %s\n", cmd.c_str());
        out.print("Run 'help tweak' to display a full list\n");
        return CR_FAILURE; // Avoid dumping usage information
    }
    return CR_OK;
}

static std::map<string, bool> get_status() {
    std::map<string, bool> list;
    for (auto & entry : tweak_hooks)
        list[entry.first] = entry.second.is_applied();
    for (auto & entry : tweak_onupdate_hooks)
        list[entry.first] = entry.second.enabled;

    return list;
}

static command_result tweak(color_ostream &out, vector <string> &parameters) {
    CoreSuspender suspend;

    if (parameters.empty() || parameters[0] == "list") {
        out.print("tweaks:\n");
        for (auto & entry : get_status())
            out.print("  %25s: %s\n", entry.first.c_str(), entry.second ? "enabled" : "disabled");
        return CR_OK;
    }

    string cmd = parameters[0];
    return enable_tweak(cmd, out, parameters);
}

static int tweak_get_status(lua_State *L) {
    Lua::Push(L, get_status());
    return 1;
}

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(tweak_get_status),
    DFHACK_LUA_END
};
