#include "Console.h"
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
    bool message_dirty = true;  // force redraw when renderer is installed

    virtual void update_tile(int32_t x, int32_t y) override {
        draw_message();
        renderer_wrap::update_tile(x, y);
    }

    virtual void update_all() override {
        draw_message();
        renderer_wrap::update_all();
    }

    virtual void render() override {
        message_dirty = true;
        renderer_wrap::render();
    }

    void draw_message() {
        if (message_dirty) {
            static std::string str = std::string("DFHack: ") + plugin_name + " active";
            Screen::paintString(Screen::Pen(' ', COLOR_LIGHTCYAN, COLOR_GREEN), 0, gps->dimy - 1, str);
            for (int32_t i = 0; i < gps->dimx; ++i)
                ((scdata*)screen)[i * gps->dimy + gps->dimy - 1].bg = 2;

            message_dirty = false;
        }
    }
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
