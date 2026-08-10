// SPDX-License-Identifier: MIT

#include <string>
#include <vector>

#include "PluginManager.h"
#include "VTableInterpose.h"

#include "df/renderer_2d_base.h"

#include "visual_renderer.h"

using namespace DFHack;

DFHACK_PLUGIN("smooth-movement");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL_NO_USE(gps);
REQUIRE_GLOBAL_NO_USE(window_x);
REQUIRE_GLOBAL_NO_USE(window_y);
REQUIRE_GLOBAL_NO_USE(window_z);

namespace {

struct renderer_hook : df::renderer_2d_base {
    typedef df::renderer_2d_base interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, update_all, ());
};

IMPLEMENT_VMETHOD_INTERPOSE(renderer_hook, update_all);

void renderer_hook::interpose_fn_update_all() {
    smooth_movement::render(this);
    INTERPOSE_NEXT(update_all)();
}

command_result status_command(color_ostream &out, std::vector<std::string> &parameters) {
    if (parameters.empty()) {
        out.print("smooth-movement: {}\n", is_enabled ? "enabled" : "disabled");
        out.print("sprite flipping: {}\n", smooth_movement::flip_enabled() ? "on" : "off");
        out.print("lower z-level animation: {}\n",
                  smooth_movement::zlevel_enabled() ? "on" : "off");
        return CR_OK;
    }
    if (parameters[0] == "flip") {
        if (parameters.size() == 1) {
            out.print("sprite flipping: {}\n", smooth_movement::flip_enabled() ? "on" : "off");
            return CR_OK;
        }
        if (parameters.size() == 2 && parameters[1] == "on")
            smooth_movement::set_flip_enabled(true);
        else if (parameters.size() == 2 && parameters[1] == "off")
            smooth_movement::set_flip_enabled(false);
        else
            return CR_WRONG_USAGE;
        out.print("smooth-movement: sprite flipping {}\n",
                  smooth_movement::flip_enabled() ? "enabled" : "disabled");
        return CR_OK;
    }
    if (parameters[0] == "zlevel") {
        if (parameters.size() == 1) {
            out.print("lower z-level animation: {}\n",
                      smooth_movement::zlevel_enabled() ? "on" : "off");
            return CR_OK;
        }
        if (parameters.size() == 2 && parameters[1] == "on")
            smooth_movement::set_zlevel_enabled(true);
        else if (parameters.size() == 2 && parameters[1] == "off")
            smooth_movement::set_zlevel_enabled(false);
        else
            return CR_WRONG_USAGE;
        out.print("smooth-movement: lower z-level animation {}\n",
                  smooth_movement::zlevel_enabled() ? "enabled" : "disabled");
        return CR_OK;
    }
    return CR_WRONG_USAGE;
}

} // namespace

DFhackCExport command_result plugin_init(color_ostream &, std::vector<PluginCommand> &commands) {
    commands.emplace_back("smooth-movement", "Smoothly animate movement in the fortress viewport.",
                          status_command);
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (is_enabled == enable)
        return CR_OK;
    if (enable) {
        if (!smooth_movement::initialize(out))
            return CR_FAILURE;
        if (!INTERPOSE_HOOK(renderer_hook, update_all).apply()) {
            out.printerr("smooth-movement: could not hook the 2D renderer\n");
            smooth_movement::shutdown();
            return CR_FAILURE;
        }
    } else {
        INTERPOSE_HOOK(renderer_hook, update_all).remove();
        smooth_movement::shutdown();
    }
    is_enabled = enable;
    out.print("smooth-movement: {}\n", enable ? "enabled" : "disabled");
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}
