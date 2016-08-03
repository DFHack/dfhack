#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Screen.h"
#include "modules/Renderer.h"

#include "df/enabler.h"
#include "df/renderer.h"

//#include "df/world.h"

using namespace DFHack;

DFHACK_PLUGIN("renderer-msg");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(enabler);
REQUIRE_GLOBAL(gps);

struct scdata {
    uint8_t ch, fg, bg, bold;
};

struct renderer_msg : public Renderer::renderer_wrap {
    virtual void update_tile (int32_t x, int32_t y) {
        static std::string str = std::string("DFHack: ") + plugin_name + " active";
        Screen::paintString(Screen::Pen(' ', 9, 0), 0, gps->dimy - 1, str);
        for (size_t i = 0; i < gps->dimx; ++i)
            ((scdata*)screen)[i * gps->dimy + gps->dimy - 1].bg = 2;
        renderer_wrap::update_tile(x, y);
    };
};

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    return CR_OK;
}

static Renderer::renderer_wrap *w = NULL;
DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    if (is_enabled == enable)
        return CR_OK;
    CoreSuspender s;
    is_enabled = enable;
    if (enable)
        w = Renderer::AddRenderer(new renderer_msg, true);
    else
        Renderer::RemoveRenderer(w);
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    plugin_enable(out, false);
    return plugin_enable(out, false);
}
