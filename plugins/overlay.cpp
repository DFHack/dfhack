#include "df/viewscreen_titlest.h"

#include "modules/Gui.h"

#include "PluginManager.h"
#include "VTableInterpose.h"
#include "uicommon.h"

using namespace DFHack;

DFHACK_PLUGIN("overlay");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(enabler);
REQUIRE_GLOBAL(gps);

static const std::string button_text = "[ DFHack Launcher ]";
static bool clicked = false;

static bool handle_click() {
    int32_t x, y;
    if (!enabler->tracking_on || !enabler->mouse_lbut || clicked ||
            !Gui::getMousePos(x, y))
        return false;
    if (y == gps->dimy - 1 && x >= 1 && (size_t)x <= button_text.size()) {
        clicked = true;
        Core::getInstance().setHotkeyCmd("gui/launcher");
        return true;
    }
    return false;
}

static void draw_widgets() {
    int x = 1;
    int y = gps->dimy - 1;
    OutputString(COLOR_GREEN, x, y, button_text);
}

struct title_hook : df::viewscreen_titlest {
    typedef df::viewscreen_titlest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input)) {
        if (!handle_click())
            INTERPOSE_NEXT(feed)(input);
    }
    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        INTERPOSE_NEXT(render)();
        draw_widgets();
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(title_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(title_hook, render);

DFhackCExport command_result plugin_onstatechange(color_ostream &,
                                                  state_change_event evt) {
    if (evt == SC_VIEWSCREEN_CHANGED) {
        clicked = false;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &, bool enable) {
    if (is_enabled == enable)
        return CR_OK;
    if (enable != is_enabled) {
        if (!INTERPOSE_HOOK(title_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(title_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &) {
    return plugin_enable(out, true);
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}
