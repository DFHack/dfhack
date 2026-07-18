#include "ColorText.h"
#include "MemAccess.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/DFSDL.h"

#include "df/enabler.h"
#include "df/gamest.h"
#include "df/graphic.h"
#include "df/graphic_viewportst.h"
#include "df/renderer_2d.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/viewscreen_worldst.h"
#include "df/viewscreen_new_regionst.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_generatorst.h"

#include <cstdint>
#include <optional>

using namespace DFHack;

DFHACK_PLUGIN("edgescroll");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(enabler);
REQUIRE_GLOBAL(game);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(gps);

// Cooldown between edge scroll actions
constexpr uint32_t cooldown_ms = 100;
// Number of pixels from border to trigger edgescroll
constexpr int border_range = 5;

// Controls how much edge scroll moves
constexpr int map_scroll_pixels = 100;
constexpr int world_scroll_tiles = 3;
constexpr int world_scroll_tiles_zoomed = 6;

DFhackCExport command_result plugin_init([[maybe_unused]]color_ostream &out, [[maybe_unused]] std::vector<PluginCommand> &commands) {
    return CR_OK;
}

DFhackCExport command_result plugin_enable([[maybe_unused]]color_ostream &out, bool enable) {
    is_enabled = enable;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown([[maybe_unused]] color_ostream &out) {
    return CR_OK;
}

static std::atomic_bool callback_queued = false;

struct scroll_state {
    int8_t xdiff = 0;
    int8_t ydiff = 0;
};

static scroll_state state;
static scroll_state queued;

static void render_thread_cb() {
    queued = {};
    // Ignore the mouse if outside the window
    if (!enabler->mouse_focus) {
        callback_queued.store(false);
        return;
    }

    // Calculate the render rect in window coordinates
    auto* renderer = virtual_cast<df::renderer_2d>(enabler->renderer);
    int origin_x, origin_y;
    int end_x, end_y;
    DFSDL::DFSDL_RenderLogicalToWindow(
        (SDL_Renderer*)renderer->sdl_renderer, (float)renderer->origin_x,
        (float)renderer->origin_y, &origin_x, &origin_y);
    DFSDL::DFSDL_RenderLogicalToWindow(
        (SDL_Renderer*)renderer->sdl_renderer,
        (float)renderer->cur_w - (float)renderer->origin_x,
        (float)(renderer->cur_h - renderer->origin_y), &end_x, &end_y);

    // Get the mouse location in window coordinates
    int mx, my;
    DFSDL::DFSDL_GetMouseState(&mx, &my);

    if (mx <= origin_x + border_range) {
        queued.xdiff--;
    } else if (mx >= end_x - border_range) {
        queued.xdiff++;
    }
    if (my <= origin_y + border_range) {
        queued.ydiff--;
    } else if (my >= end_y - border_range) {
        queued.ydiff++;
    }

    callback_queued.store(false);
}

static bool update_mouse_pos() {
    if (callback_queued.load())
        return false; // Queued callback not complete, check back later

    // Queued callback complete, save the results and enqueue again
    state = queued;
    queued = {};
    DFHack::runOnRenderThread(render_thread_cb);
    callback_queued.store(true);
    return true;
}

// Apply scroll whilst maintaining boundaries
template<typename T>
static void apply_scroll(T* out, T diff, T min, T max) {
    *out = std::min(std::max(*out + diff, min), max);
}

// Scroll main fortress/adventure world views
static void scroll_dwarfmode(int xdiff, int ydiff) {
    using df::global::window_x;
    using df::global::window_y;
    using df::global::game;
    // Scale the movement by pixels, to keep scroll speeds visually consistent
    int tilesize = gps->viewport_zoom_factor / 4;
    int width = gps->main_viewport->dim_x;
    int height = gps->main_viewport->dim_y;

    // Ensure the map doesn't go fully off-screen
    int min_x = -width / 2;
    int min_y = -height / 2;
    int max_x = world->map.x_count - (width / 2);
    int max_y = world->map.y_count - (height / 2);
    apply_scroll(window_x, xdiff * std::max(1, map_scroll_pixels / tilesize), min_x, max_x);
    apply_scroll(window_y, ydiff * std::max(1, map_scroll_pixels / tilesize), min_y, max_y);

    // Force a minimap update
    game->minimap.update = 1;
    game->minimap.mustmake = 1;
}

template<typename T>
static void scroll_world_internal(T* screen, int xdiff, int ydiff) {
    if constexpr(std::is_same_v<T, df::viewscreen_choose_start_sitest>) {
        if (screen->zoomed_in) {
            int max_x = (world->world_data->world_width * 16)-1;
            int max_y = (world->world_data->world_height * 16)-1;
            apply_scroll(&screen->zoom_cent_x, xdiff * world_scroll_tiles_zoomed, 0, max_x);
            apply_scroll(&screen->zoom_cent_y, ydiff * world_scroll_tiles_zoomed, 0, max_y);
            return;
        }
    }

    int32_t *x, *y;
    if constexpr(std::is_same_v<T, df::viewscreen_new_regionst>) {
        x = &world->worldgen_status.cursor_x;
        y = &world->worldgen_status.cursor_y;
    } else {
        x = &screen->region_cent_x;
        y = &screen->region_cent_y;
    }
    int max_x = world->world_data->world_width-1;
    int max_y = world->world_data->world_height-1;
    apply_scroll(x, xdiff * world_scroll_tiles, 0, max_x);
    apply_scroll(y, ydiff * world_scroll_tiles, 0, max_y);
}

template<typename... Ts>
struct overloads : Ts... { using Ts::operator()...; };

using world_map = std::variant<df::viewscreen_choose_start_sitest*, df::viewscreen_worldst*, df::viewscreen_new_regionst*>;
static std::optional<world_map> get_map() {
    df::viewscreen* screen = Gui::getCurViewscreen(true);
    screen = Gui::getDFViewscreen(true, screen); // Get the first non-dfhack viewscreen
    if(!screen)
        return {};

    if (auto start_site = virtual_cast<df::viewscreen_choose_start_sitest>(screen))
            return start_site;
    if (auto world_map = virtual_cast<df::viewscreen_worldst>(screen))
        return world_map;
    if (auto worldgen_map = virtual_cast<df::viewscreen_new_regionst>(screen)) {
        if (!world || world->worldgen_status.state <= df::world_generatorst::Initializing)
            return {}; // Map isn't displayed yet
        return worldgen_map;
    }
    return {};
}

static void scroll_world(world_map screen, int xdiff, int ydiff) {
    const auto visitor = overloads {
        [xdiff, ydiff](df::viewscreen_choose_start_sitest* s) {scroll_world_internal(s, xdiff, ydiff);},
        [xdiff, ydiff](df::viewscreen_worldst* s) {scroll_world_internal(s, xdiff, ydiff);},
        [xdiff, ydiff](df::viewscreen_new_regionst* s) {scroll_world_internal(s, xdiff, ydiff);},
    };
    std::visit(visitor, screen);
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    // Apply a cooldown to any potential edgescrolls
    auto& core = Core::getInstance();
    static uint32_t last_action = 0;
    uint32_t now = core.p->getTickCount();
    if (now < last_action + cooldown_ms)
        return CR_OK;

    // Update mouse_x/y from values read in render thread callback
    if (!update_mouse_pos())
        return CR_OK; // No new input to process

    if (state.xdiff == 0 && state.ydiff == 0)
        return CR_OK; // No work to do

    // Ensure either a map viewscreen or the main viewport are visible
    auto worldmap = get_map();
    if (!worldmap.has_value() && (!gps->main_viewport || !gps->main_viewport->flag.bits.active))
        return CR_OK;

    // Dispatch scrolling to active scrollables
    if (worldmap.has_value())
        scroll_world(worldmap.value(), state.xdiff, state.ydiff);
    else if (gps->main_viewport->flag.bits.active)
        scroll_dwarfmode(state.xdiff, state.ydiff);

    // Update cooldown
    last_action = now;

    return CR_OK;
}
