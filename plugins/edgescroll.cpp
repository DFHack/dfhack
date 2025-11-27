#include "ColorText.h"
#include "PluginManager.h"
#include "MemAccess.h"

#include "df/world_generatorst.h"
#include "modules/Gui.h"

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

template<typename T>
static void apply_scroll(T* out, T diff, T min, T max) {
    *out = std::min(std::max(*out + diff, min), max);
}

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
static void scroll_world(T* screen, int xdiff, int ydiff) {
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
        [xdiff, ydiff](df::viewscreen_choose_start_sitest* s) {scroll_world(s, xdiff, ydiff);},
        [xdiff, ydiff](df::viewscreen_worldst* s) {scroll_world(s, xdiff, ydiff);},
        [xdiff, ydiff](df::viewscreen_new_regionst* s) {scroll_world(s, xdiff, ydiff);},
    };
    std::visit(visitor, screen);
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {

    // Ensure either a map viewscreen or the main viewport are visible
    auto worldmap = get_map();
    if (!worldmap.has_value() && (!gps->main_viewport || !gps->main_viewport->flag.bits.active))
        return CR_OK;

    // FIXME: Once dfhooks_sdl_loop is hooked up in Core, use SDL_GetMouseState
    // to determine the correct mouse position without forcing the screen to
    // render slightly un-centered.
    // origin_x/y are already zero if not fitting the interface to the grid

    // Force the origin_x/y values to zero to workaround df marking any
    // mouse position within the margin register as invalid
    auto renderer = virtual_cast<df::renderer_2d>(enabler->renderer);
    if (renderer && (renderer->origin_x != 0 || renderer->origin_y != 0)) {
        renderer->origin_x = 0;
        renderer->origin_y = 0;
    }

    auto dim_x = gps->screen_pixel_x;
    auto dim_y = gps->screen_pixel_y;
    auto x = gps->precise_mouse_x;
    auto y = gps->precise_mouse_y;
    if (x == -1 || y == -1)
        return CR_OK; // Invalid mouse position

    // Apply a cooldown to any potential edgescrolls
    auto& core = Core::getInstance();
    static uint32_t last_action = 0;
    uint32_t now = core.p->getTickCount();
    if (now < last_action + cooldown_ms)
        return CR_OK;


    int xdiff = 0;
    int ydiff = 0;
    if (x <= border_range) {
        xdiff--;
    } else if (x >= dim_x - border_range) {
        xdiff++;
    }
    if (y <= border_range) {
        ydiff--;
    } else if (y >= dim_y - border_range) {
        ydiff++;
    }

    if (xdiff == 0 && ydiff == 0)
        return CR_OK; // No work to do

    // Dispatch scrolling to active scrollables
    if (worldmap.has_value())
        scroll_world(worldmap.value(), xdiff, ydiff);
    else if (gps->main_viewport->flag.bits.active)
        scroll_dwarfmode(xdiff, ydiff);

    // Update cooldown
    last_action = now;

    return CR_OK;
}
