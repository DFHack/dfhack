/*
Zlib License

Copyright (c) 2026 World Map Zoom contributors

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Adapted for the DFHack build from World Map Zoom 1.0.0.
*/

// Map-only native composition transform, built with DFHack's matching headers.
#define NO_DFHACK_VERSION_MACROS

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <SDL.h>
#include <lauxlib.h>
#include <lua.h>

#include "Core.h"
#include "DFHackVersion.h"
#include "PluginManager.h"
#include "VTableInterpose.h"
#include "modules/DFSDL.h"
#include "modules/Gui.h"

#include "df/enabler.h"
#include "df/graphic.h"
#include "df/graphic_map_portst.h"
#include "df/renderer_2d.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/viewscreen_worldst.h"

#include "worldmap-zoom-canvas.h"
#include "worldmap-zoom-sdl.h"

using namespace DFHack;
DFHACK_PLUGIN("worldmap-zoom-render");
REQUIRE_GLOBAL(enabler);
REQUIRE_GLOBAL(gps);

static WorldMapSdl sdl;
struct Camera {
    bool enabled = false;
    int context = 0;
    double zoom = 1, x = 0, y = 0;
    const void *port = nullptr;
    const void *screen = nullptr;
    uint64_t generation = 0;
};
static std::mutex state_mutex;
static Camera camera;
// Lua and rendering exchange snapshots/diagnostics here. Profiling vectors are
// bounded and mutex-protected; frame_start/frame_started are render-thread-only.
static std::string capture_path, last_capture, last_error, capture_error;
static std::atomic_bool hooks_ready{false};
static std::atomic_bool build_verified{false}, prepare_pending{false};
static std::atomic_uint64_t full_calls{0}, tile_calls{0}, transformed_calls{0};
static std::atomic_ulong render_thread{0};
static std::atomic_uint64_t restore_checks{0}, restore_errors{0};
static std::atomic_bool profiling{false};
static std::vector<double> profile_us;
static std::vector<double> frame_profile_us;
static std::chrono::steady_clock::time_point frame_start;
static bool frame_started = false;
static void start_frame() {
    if (profiling && !frame_started) {
        frame_start = std::chrono::steady_clock::now();
        frame_started = true;
    }
}
static std::vector<unsigned char> native_ui;
// UI occupancy is captured/read only by the simulation-side Lua client. It is
// separate from the SDL canvas, whose map pixels must not count as controls.
static int ui_cols = 0, ui_rows = 0;
static std::atomic_int desktop_width{0}, desktop_height{0}, output_width{0}, output_height{0};
static std::atomic_bool window_focused{false};
static std::atomic_uint64_t focus_epoch{0};
static std::atomic_uint32_t watched_window{0};
static bool event_watch_installed = false; // graphics thread only
static std::string renderer_name;
static std::atomic_uint64_t camera_generation{0}, target_generation{0}, canvas_creations{0},
    canvas_composites{0};
static std::atomic_uint64_t composited_generation{0}, presented_generation{0};
static thread_local int transform_depth = 0;
static worldmap_zoom::MapCanvas<WorldMapSdl> map_canvas(sdl);
static df::renderer_2d_base *batch_renderer = nullptr;
static df::graphic_map_portst *batch_port = nullptr;
// Admission closes before unload cancels queued work. The core task handle
// acknowledges destruction of plugin callbacks, not merely their last write.
static std::recursive_mutex jobs_mutex;
static std::vector<std::shared_ptr<RenderThreadTask>> render_jobs;
static std::shared_ptr<RenderThreadTask> cleanup_job;
static std::atomic_bool closing{false};
static bool stopped_inline = false; // only used during render-thread process shutdown

static void remove_hooks();
static int lua_prepare(lua_State *L);
static int lua_stop(lua_State *L);
static bool stopped();

static std::shared_ptr<RenderThreadTask> queue_job(std::function<void()> callback) {
    std::lock_guard lock(jobs_mutex);
    if (closing)
        return {};
    std::erase_if(render_jobs, [](const auto &job) { return job->finished(); });
    // Allocate tracking space before making a callback reachable from core.
    // A failed vector allocation must not leave untracked plugin code queued.
    render_jobs.reserve(render_jobs.size() + 1);
    auto job = queueOnRenderThread(std::move(callback));
    render_jobs.push_back(job);
    return job;
}

static Camera camera_snapshot() {
    std::lock_guard lock(state_mutex);
    return camera;
}

// This is the only canvas-to-Lua publication path. All canvas fields are owned
// by the render thread; Lua reads the copied counters and error under the mutex.
static void publish_canvas_status() {
    const auto &stats = map_canvas.stats();
    canvas_creations = stats.creations;
    canvas_composites = stats.composites;
    composited_generation = stats.composited_generation;
    restore_checks = stats.restore_checks;
    restore_errors = stats.restore_errors;
    if (!map_canvas.error().empty()) {
        std::lock_guard lock(state_mutex);
        camera.enabled = false;
        last_error = map_canvas.error();
    }
}

static void finish_canvas(bool composite = true) {
    if (!map_canvas.active())
        return;
    const auto live = camera_snapshot();
    const bool current_view = live.enabled && live.generation == map_canvas.generation() && !closing;
    map_canvas.finish(composite && current_view, [] {
        // Native rendering re-enters our vmethods. The depth guard routes
        // nested calls directly to DF while this target pass remains active.
        ++transform_depth;
        batch_renderer->update_full_map_port(batch_port);
        --transform_depth;
    });
    publish_canvas_status();
}

static void release_canvas(df::renderer_2d_base *renderer) {
    auto native = renderer ? reinterpret_cast<SDL_Renderer *>(renderer->sdl_renderer) : nullptr;
    auto window = renderer ? sdl.window_id(reinterpret_cast<SDL_Window *>(renderer->window)) : 0;
    map_canvas.release(native, window);
    batch_renderer = nullptr;
    batch_port = nullptr;
    publish_canvas_status();
}

static bool begin_canvas(df::renderer_2d_base *renderer, df::graphic_map_portst *port, const Camera &c,
                         bool full_draw) {
    // Only display-side map-port/renderer fields are read here. Screen mode and
    // active map identity were already checked and published on the sim side.
    constexpr int cell = worldmap_zoom::map_cell_pixels;
    const SDL_Rect rect{renderer->origin_x + port->top_left_corner_x,
                        renderer->origin_y + port->top_left_corner_y, port->dim_x * cell, port->dim_y * cell};
    const worldmap_zoom::CanvasView view{c.zoom, c.x, c.y, c.generation};
    const bool begun = map_canvas.begin(reinterpret_cast<SDL_Renderer *>(renderer->sdl_renderer),
                                        sdl.window_id(reinterpret_cast<SDL_Window *>(renderer->window)), rect,
                                        view, target_generation.load(), full_draw);
    if (begun) {
        batch_renderer = renderer;
        batch_port = port;
    }
    publish_canvas_status();
    return begun;
}

static bool canvas_camera(df::graphic_map_portst *port, Camera &c) {
    constexpr int max_cells = worldmap_zoom::max_canvas_pixels / worldmap_zoom::map_cell_pixels;
    if (transform_depth || !hooks_ready || closing || !port || !port->flag.bits.active || port->dim_x < 1 ||
        port->dim_y < 1 || port->dim_x > max_cells || port->dim_y > max_cells)
        return false;
    c = camera_snapshot();
    return c.enabled && c.zoom > 1 && c.port == port;
}

static bool bind_sdl() {
    auto h = DFSDL::obtain_library_handle();
    if (!h)
        return false;
#define BIND(member, name)                                                                                   \
    sdl.member = reinterpret_cast<decltype(sdl.member)>(LookupPlugin(h, name));                              \
    if (!sdl.member)                                                                                         \
    return false
    BIND(thread_id, "SDL_ThreadID");
    BIND(get_scale, "SDL_RenderGetScale");
    BIND(set_scale, "SDL_RenderSetScale");
    BIND(get_viewport, "SDL_RenderGetViewport");
    BIND(set_viewport, "SDL_RenderSetViewport");
    BIND(get_clip, "SDL_RenderGetClipRect");
    BIND(set_clip, "SDL_RenderSetClipRect");
    BIND(clip_enabled, "SDL_RenderIsClipEnabled");
    BIND(read_pixels, "SDL_RenderReadPixels");
    BIND(output_size, "SDL_GetRendererOutputSize");
    BIND(window_size, "SDL_SetWindowSize");
    BIND(fullscreen, "SDL_SetWindowFullscreen");
    BIND(display_index, "SDL_GetWindowDisplayIndex");
    BIND(desktop_mode, "SDL_GetDesktopDisplayMode");
    BIND(window_id, "SDL_GetWindowID");
    BIND(keyboard_focus, "SDL_GetKeyboardFocus");
    BIND(add_event_watch, "SDL_AddEventWatch");
    BIND(del_event_watch, "SDL_DelEventWatch");
    BIND(renderer_info, "SDL_GetRendererInfo");
    BIND(get_target, "SDL_GetRenderTarget");
    BIND(set_target, "SDL_SetRenderTarget");
    BIND(create_texture, "SDL_CreateTexture");
    BIND(destroy_texture, "SDL_DestroyTexture");
    BIND(texture_blend, "SDL_SetTextureBlendMode");
    BIND(texture_scale, "SDL_SetTextureScaleMode");
    BIND(get_color, "SDL_GetRenderDrawColor");
    BIND(set_color, "SDL_SetRenderDrawColor");
    BIND(clear, "SDL_RenderClear");
    BIND(fill, "SDL_RenderFillRect");
    BIND(copy, "SDL_RenderCopy");
#undef BIND
    return true;
}

static int watch_focus(void *, SDL_Event *event) {
    // SDL_WINDOWEVENT / SDL_WINDOWEVENT_FOCUS_LOST. An epoch also catches
    // loss followed by regain between two Lua updates. No mapped DF access.
    if (event && event->type == SDL_WINDOWEVENT && event->window.windowID == watched_window.load()) {
        if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            window_focused = false;
            ++focus_epoch;
        } else if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
            window_focused = true;
    }
    if (event && (event->type == SDL_RENDER_TARGETS_RESET || event->type == SDL_RENDER_DEVICE_RESET))
        ++target_generation;
    return 1;
}

// Simulation/core-lock context only. Renderer hooks must use the published camera.
static int current_context() {
    auto screen = Gui::getDFViewscreen(true);
    if (auto s = virtual_cast<df::viewscreen_choose_start_sitest>(screen))
        return s->zoomed_in || s->page != df::choose_start_site_view_mode::Biome ? 0 : 1;
    if (auto s = virtual_cast<df::viewscreen_worldst>(screen))
        return s->view_mode == df::world_view_mode_type::NORMAL ? 2 : 0;
    return 0;
}

static void capture(df::renderer_2d_base *r) {
    // A diagnostic capture reads the composed backbuffer just before present.
    // File I/O is opt-in; ordinary rendering never writes screenshots.
    std::string path;
    {
        std::lock_guard lock(state_mutex);
        path.swap(capture_path);
    }
    if (path.empty())
        return;
    int w = 0, h = 0;
    auto sr = reinterpret_cast<SDL_Renderer *>(r->sdl_renderer);
    sdl.output_size(sr, &w, &h);
    std::string error;
    if (w <= 0 || h <= 0 || w > 16384 || h > 16384)
        error = "Invalid capture dimensions";
    else {
        std::vector<unsigned char> pixels(static_cast<size_t>(w) * h * 4);
        if (sdl.read_pixels(sr, nullptr, SDL_PIXELFORMAT_ABGR8888, pixels.data(), w * 4) != 0)
            error = "SDL_RenderReadPixels failed";
        else
            try {
                std::ofstream f(std::filesystem::path(std::u8string(path.begin(), path.end())),
                                std::ios::binary);
                f << "P6\n" << w << ' ' << h << "\n255\n";
                for (size_t i = 0; i < pixels.size(); i += 4)
                    f.write(reinterpret_cast<char *>(pixels.data() + i), 3);
                if (!f)
                    error = "Could not write capture";
            } catch (const std::exception &e) {
                error = std::string("Could not write capture: ") + e.what();
            }
    }
    std::lock_guard lock(state_mutex);
    capture_error = error;
    if (error.empty())
        last_capture = path;
}

struct map_hook : df::renderer_2d_base {
    using interpose_base = df::renderer_2d_base;
    DEFINE_VMETHOD_INTERPOSE(void, update_full_map_port, (df::graphic_map_portst * port)) {
        start_frame();
        render_thread = sdl.thread_id();
        ++full_calls;
        const bool measure = profiling.load();
        const auto start =
            measure ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
        Camera c;
        if (!transform_depth && canvas_camera(port, c) && begin_canvas(this, port, c, true)) {
            ++transform_depth;
            INTERPOSE_NEXT(update_full_map_port)(port);
            --transform_depth;
            map_canvas.markFullDraw();
            ++transformed_calls;
            finish_canvas();
        } else {
            if (!transform_depth)
                finish_canvas();
            INTERPOSE_NEXT(update_full_map_port)(port);
        }
        if (measure) {
            const auto us =
                std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
            std::lock_guard lock(state_mutex);
            if (profile_us.size() < 256)
                profile_us.push_back(us);
        }
    }
    DEFINE_VMETHOD_INTERPOSE(void, update_map_port_tile,
                             (df::graphic_map_portst * port, int32_t x, int32_t y)) {
        start_frame();
        ++tile_calls;
        Camera c;
        if (!transform_depth && canvas_camera(port, c) && begin_canvas(this, port, c, false)) {
            ++transform_depth;
            map_canvas.clearTile(x, y);
            INTERPOSE_NEXT(update_map_port_tile)(port, x, y);
            map_canvas.paintedTile(x, y);
            --transform_depth;
            ++transformed_calls;
            // Keep consecutive tile updates in one target pass. The next UI
            // draw or present composites it once, below the native interface.
        } else {
            if (!transform_depth)
                finish_canvas();
            INTERPOSE_NEXT(update_map_port_tile)(port, x, y);
        }
    }
    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        render_thread = sdl.thread_id();
        finish_canvas();
        int w = 0, h = 0;
        sdl.output_size(reinterpret_cast<SDL_Renderer *>(sdl_renderer), &w, &h);
        output_width = w;
        output_height = h;
        capture(this);
        INTERPOSE_NEXT(render)();
        presented_generation = composited_generation.load();
        if (profiling && frame_started) {
            auto us =
                std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - frame_start)
                    .count();
            std::lock_guard lock(state_mutex);
            if (frame_profile_us.size() < 256)
                frame_profile_us.push_back(us);
            if (frame_profile_us.size() == 256 && profile_us.size() == 256)
                profiling = false;
        }
        frame_started = false;
    }
    // Any of these native interface/fortress-viewport draws may follow a
    // batch of world-map tile updates. Composite the map before that UI draw.
    // The same boundary is skipped during our recursive native map fallback.
    DEFINE_VMETHOD_INTERPOSE(void, update_all, ()) {
        start_frame();
        if (!transform_depth)
            finish_canvas();
        INTERPOSE_NEXT(update_all)();
    }
    DEFINE_VMETHOD_INTERPOSE(void, update_tile, (int32_t x, int32_t y)) {
        start_frame();
        if (!transform_depth)
            finish_canvas();
        INTERPOSE_NEXT(update_tile)(x, y);
    }
    DEFINE_VMETHOD_INTERPOSE(void, update_anchor_tile, (int32_t x, int32_t y)) {
        start_frame();
        if (!transform_depth)
            finish_canvas();
        INTERPOSE_NEXT(update_anchor_tile)(x, y);
    }
    DEFINE_VMETHOD_INTERPOSE(void, update_top_tile, (int32_t x, int32_t y)) {
        start_frame();
        if (!transform_depth)
            finish_canvas();
        INTERPOSE_NEXT(update_top_tile)(x, y);
    }
    DEFINE_VMETHOD_INTERPOSE(void, update_top_anchor_tile, (int32_t x, int32_t y)) {
        start_frame();
        if (!transform_depth)
            finish_canvas();
        INTERPOSE_NEXT(update_top_anchor_tile)(x, y);
    }
    DEFINE_VMETHOD_INTERPOSE(void, update_viewport_tile,
                             (df::graphic_viewportst * port, int32_t x, int32_t y)) {
        start_frame();
        if (!transform_depth)
            finish_canvas();
        INTERPOSE_NEXT(update_viewport_tile)(port, x, y);
    }
    DEFINE_VMETHOD_INTERPOSE(void, update_full_viewport, (df::graphic_viewportst * port)) {
        start_frame();
        if (!transform_depth)
            finish_canvas();
        INTERPOSE_NEXT(update_full_viewport)(port);
    }
    DEFINE_VMETHOD_INTERPOSE(void, do_blank_screen_fill, ()) {
        start_frame();
        finish_canvas(false);
        INTERPOSE_NEXT(do_blank_screen_fill)();
    }
    DEFINE_VMETHOD_INTERPOSE(void, resize, (int32_t w, int32_t h)) {
        release_canvas(this);
        INTERPOSE_NEXT(resize)(w, h);
    }
    DEFINE_VMETHOD_INTERPOSE(void, grid_resize, (int32_t w, int32_t h)) {
        release_canvas(this);
        INTERPOSE_NEXT(grid_resize)(w, h);
    }
    DEFINE_VMETHOD_INTERPOSE(void, set_fullscreen, ()) {
        release_canvas(this);
        INTERPOSE_NEXT(set_fullscreen)();
    }
    DEFINE_VMETHOD_INTERPOSE(bool, init_video, (int32_t w, int32_t h)) {
        release_canvas(this);
        return INTERPOSE_NEXT(init_video)(w, h);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_full_map_port);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_map_port_tile);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, render);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_all);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_tile);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_anchor_tile);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_top_tile);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_top_anchor_tile);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_viewport_tile);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_full_viewport);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, do_blank_screen_fill);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, resize);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, grid_resize);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, set_fullscreen);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, init_video);

// One explicit registration list is shared by installation and removal.
static const std::array<VMethodInterposeLinkBase *, 15> renderer_hooks{
    &INTERPOSE_HOOK(map_hook, update_full_map_port),
    &INTERPOSE_HOOK(map_hook, update_map_port_tile),
    &INTERPOSE_HOOK(map_hook, render),
    &INTERPOSE_HOOK(map_hook, update_all),
    &INTERPOSE_HOOK(map_hook, update_tile),
    &INTERPOSE_HOOK(map_hook, update_anchor_tile),
    &INTERPOSE_HOOK(map_hook, update_top_tile),
    &INTERPOSE_HOOK(map_hook, update_top_anchor_tile),
    &INTERPOSE_HOOK(map_hook, update_viewport_tile),
    &INTERPOSE_HOOK(map_hook, update_full_viewport),
    &INTERPOSE_HOOK(map_hook, do_blank_screen_fill),
    &INTERPOSE_HOOK(map_hook, resize),
    &INTERPOSE_HOOK(map_hook, grid_resize),
    &INTERPOSE_HOOK(map_hook, set_fullscreen),
    &INTERPOSE_HOOK(map_hook, init_video),
};

static void remove_hooks() {
    if (enabler && enabler->renderer)
        release_canvas(virtual_cast<df::renderer_2d_base>(enabler->renderer));
    if (event_watch_installed) {
        // SDL2 invokes/removes event watches under the same internal mutex.
        // We are outside event dispatch, so return from DelEventWatch also
        // excludes an event-watch invocation on a foreign SDL_PushEvent thread.
        sdl.del_event_watch(watch_focus, nullptr);
        event_watch_installed = false;
    }
    watched_window = 0;
    window_focused = false;
    hooks_ready = false;
    for (auto *hook : renderer_hooks)
        hook->remove();
}

static int lua_camera(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    if (closing)
        return luaL_error(L, "Renderer is stopping");
    Camera c;
    c.enabled = lua_toboolean(L, 1) != 0;
    const auto context = luaL_checkinteger(L, 2);
    if (context < 0 || context > 2 || (c.enabled && context == 0))
        return luaL_error(L, "Expected a supported context, 1x/2x/4x zoom, and finite coordinates");
    c.context = static_cast<int>(context);
    c.zoom = luaL_checknumber(L, 3);
    c.x = luaL_checknumber(L, 4);
    c.y = luaL_checknumber(L, 5);
    if (!std::isfinite(c.zoom) || !std::isfinite(c.x) || !std::isfinite(c.y) ||
        (c.zoom != 1 && c.zoom != 2 && c.zoom != 4) || c.context < 0 || c.context > 2)
        return luaL_error(L, "Expected a supported context, 1x/2x/4x zoom, and finite coordinates");
    if (c.enabled && (!build_verified || !hooks_ready || c.context != current_context()))
        return luaL_error(L, "Renderer is not ready or the high-level map is no longer open");
    {
        std::lock_guard lock(state_mutex);
        if (closing)
            return luaL_error(L, "Renderer is stopping");
        c.port = c.enabled ? gps->main_map_port : nullptr;
        c.screen = c.enabled ? Gui::getDFViewscreen(true) : nullptr;
        c.generation = ++camera_generation;
        camera = c;
    }
    if (!c.enabled && build_verified)
        queue_job([] { release_canvas(virtual_cast<df::renderer_2d_base>(enabler->renderer)); });
    if (build_verified && gps)
        gps->force_full_display_count = 2;
    return 0;
}

static int zoom_status(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    lua_newtable(L);
    const auto int_field = [L](const char *name, auto value) {
        lua_pushinteger(L, static_cast<lua_Integer>(value));
        lua_setfield(L, -2, name);
    };
    lua_pushboolean(L, hooks_ready);
    lua_setfield(L, -2, "ready");
    lua_pushboolean(L, stopped());
    lua_setfield(L, -2, "stopped");
    int_field("full_calls", full_calls.load());
    int_field("tile_calls", tile_calls.load());
    int_field("transformed_calls", transformed_calls.load());
    int_field("render_thread", render_thread.load());
    int_field("context", build_verified ? current_context() : 0);
    int_field("restore_checks", restore_checks.load());
    int_field("restore_errors", restore_errors.load());
    int_field("desktop_width", desktop_width.load());
    int_field("desktop_height", desktop_height.load());
    int_field("output_width", output_width.load());
    int_field("output_height", output_height.load());
    int_field("focus_epoch", focus_epoch.load());
    int_field("canvas_creations", canvas_creations.load());
    int_field("canvas_composites", canvas_composites.load());
    int_field("camera_generation", camera_generation.load());
    int_field("presented_generation", presented_generation.load());
    lua_pushboolean(L, window_focused.load());
    lua_setfield(L, -2, "window_focused");
    {
        std::lock_guard lock(state_mutex);
        int_field("profile_samples", profile_us.size());
        int_field("frame_profile_samples", frame_profile_us.size());
        lua_pushstring(L, last_error.c_str());
        lua_setfield(L, -2, "error");
        lua_pushstring(L, renderer_name.c_str());
        lua_setfield(L, -2, "sdl_backend");
        lua_pushstring(L, capture_error.c_str());
        lua_setfield(L, -2, "capture_error");
        lua_pushstring(L, last_capture.c_str());
        lua_setfield(L, -2, "last_capture");
    }
    return 1;
}

static int lua_profile(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    if (closing)
        return luaL_error(L, "Renderer is stopping");
    if (!build_verified || !hooks_ready)
        return luaL_error(L, "Renderer is not ready");
    std::lock_guard lock(state_mutex);
    if (lua_toboolean(L, 1)) {
        profile_us.clear();
        frame_profile_us.clear();
        profiling = true;
    }
    lua_newtable(L);
    int i = 0;
    for (double sample : profile_us) {
        lua_pushnumber(L, sample);
        lua_rawseti(L, -2, ++i);
    }
    return 1;
}

static int lua_frame_profile(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    if (closing)
        return luaL_error(L, "Renderer is stopping");
    if (!build_verified || !hooks_ready)
        return luaL_error(L, "Renderer is not ready");
    std::lock_guard lock(state_mutex);
    lua_newtable(L);
    int i = 0;
    for (double sample : frame_profile_us) {
        lua_pushnumber(L, sample);
        lua_rawseti(L, -2, ++i);
    }
    return 1;
}

// Called after native parent rendering and BEFORE drawing the Lua controls.
// UI lives in the text buffers; geographic layers live in main_map_port.
static int lua_snapshot_ui(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    if (closing)
        return luaL_error(L, "Renderer is stopping");
    if (!build_verified || !hooks_ready)
        return luaL_error(L, "Renderer is not ready");
    ui_cols = gps->dimx;
    ui_rows = gps->dimy;
    const size_t count = static_cast<size_t>(ui_cols) * ui_rows;
    if (!ui_cols || !ui_rows || count > 4000000)
        return luaL_error(L, "Invalid UI dimensions");
    native_ui.assign(count, 0);
    for (size_t i = 0; i < count; ++i) {
        const auto occupied = [](unsigned char ch) { return ch != 0 && ch != 32; };
        native_ui[i] =
            gps->screentexpos[i] || gps->screentexpos_lower[i] || gps->screentexpos_anchored[i] ||
            occupied(gps->screen[i * 8]) ||
            (gps->top_in_use && (gps->screentexpos_top[i] || gps->screentexpos_top_lower[i] ||
                                 gps->screentexpos_top_anchored[i] || occupied(gps->screen_top[i * 8])));
    }
    return 0;
}

static int lua_ui_hit(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    if (closing)
        return luaL_error(L, "Renderer is stopping");
    if (!build_verified || !hooks_ready)
        return luaL_error(L, "Renderer is not ready");
    const int x = static_cast<int>(luaL_checkinteger(L, 1)), y = static_cast<int>(luaL_checkinteger(L, 2));
    if (ui_cols != gps->dimx || ui_rows != gps->dimy || native_ui.empty()) {
        lua_pushnil(L);
        return 1;
    }
    if (x < 0 || y < 0 || x >= ui_cols || y >= ui_rows) {
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, native_ui[static_cast<size_t>(x) * ui_rows + y]);
    return 1;
}

static int lua_window(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    if (closing)
        return luaL_error(L, "Renderer is stopping");
    if (!build_verified || !hooks_ready)
        return luaL_error(L, "Renderer is not ready");
    const int w = static_cast<int>(luaL_checkinteger(L, 1)), h = static_cast<int>(luaL_checkinteger(L, 2));
    const bool full = lua_toboolean(L, 3) != 0;
    if (full || enabler->fullscreen_state.bits.fullscreen)
        return luaL_error(L, "Use native DF settings for fullscreen tests");
    if (w < 640 || h < 480 || w > 7680 || h > 4320)
        return luaL_error(L, "Invalid test window dimensions");
    queue_job([w, h] {
        auto r = virtual_cast<df::renderer_2d>(enabler->renderer);
        if (!r)
            return;
        auto window = reinterpret_cast<SDL_Window *>(r->window);
        sdl.window_size(window, w, h);
        // Programmatic SDL resizing emits SIZE_CHANGED, while DF handles
        // RESIZED. Let the native event loop pause simulation and reshape its
        // buffers, exactly as for an ordinary user window resize.
        SDL_Event event{};
        event.window.type = SDL_WINDOWEVENT;
        event.window.windowID = sdl.window_id(window);
        event.window.event = SDL_WINDOWEVENT_RESIZED;
        event.window.data1 = w;
        event.window.data2 = h;
        DFSDL::DFSDL_PushEvent(&event);
    });
    return 0;
}

static int lua_capture(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    if (closing)
        return luaL_error(L, "Renderer is stopping");
    if (!build_verified || !hooks_ready)
        return luaL_error(L, "Renderer is not ready");
    std::string path = luaL_checkstring(L, 1);
    {
        std::lock_guard lock(state_mutex);
        capture_path = path;
    }
    gps->force_full_display_count = 2;
    return 0;
}

static int lua_test_focus(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    if (closing)
        return luaL_error(L, "Renderer is stopping");
    if (!build_verified || !hooks_ready)
        return luaL_error(L, "Renderer is not ready");
    const bool focused = lua_toboolean(L, 1) != 0;
    queue_job([focused] {
        SDL_Event event{};
        event.window.type = SDL_WINDOWEVENT;
        event.window.windowID = watched_window.load();
        event.window.event = focused ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST;
        DFSDL::DFSDL_PushEvent(&event);
    });
    return 0;
}

DFhackCExport CommandReg plugin_lua_commands[] = {{"stop", lua_stop},
                                                  {"prepare", lua_prepare},
                                                  {"setCamera", lua_camera},
                                                  {"status", zoom_status},
                                                  {"capture", lua_capture},
                                                  {"profile", lua_profile},
                                                  {"snapshotUi", lua_snapshot_ui},
                                                  {"uiHit", lua_ui_hit},
                                                  {"testWindow", lua_window},
                                                  {"frameProfile", lua_frame_profile},
                                                  {"testFocus", lua_test_focus},
                                                  {nullptr, nullptr}};

static int lua_prepare(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    if (closing)
        return luaL_error(L, "Renderer is stopping; reload the plugin before preparing again");
    // Validate runtime metadata BEFORE reading mapped renderer fields or
    // installing hooks. Compare with the metadata linked into this build, not a fixed release.
    // Plugin metadata alone only checks the release/ABI.
    for (auto [function, expected] : {std::pair{"getGitCommit", Version::git_commit()},
                                      std::pair{"getGitXmlCommit", Version::git_xml_commit()}}) {
        lua_getglobal(L, "dfhack");
        lua_getfield(L, -1, function);
        if (lua_pcall(L, 0, 1, 0) != LUA_OK)
            return luaL_error(L, "Cannot verify DFHack build");
        const char *value = lua_tostring(L, -1);
        if (!value || std::string(value) != expected)
            return luaL_error(L, "Unsupported DFHack build/structures");
        lua_pop(L, 2);
    }
    if (!enabler || !virtual_cast<df::renderer_2d>(enabler->renderer)) {
        return luaL_error(L, "Unsupported renderer: requires the tested SDL2 renderer");
    }
    if (hooks_ready && build_verified) {
        lua_pushboolean(L, true);
        return 1;
    }
    if (prepare_pending.exchange(true)) {
        lua_pushboolean(L, false);
        return 1;
    }
    {
        std::lock_guard lock(state_mutex);
        last_error.clear();
    }
    build_verified = true;
    queue_job([] {
        if (!build_verified) {
            prepare_pending = false;
            return;
        }
        map_canvas.clearError();
        auto r = virtual_cast<df::renderer_2d>(enabler->renderer);
        if (r) {
            SDL_RendererInfo info{};
            if (sdl.renderer_info(reinterpret_cast<SDL_Renderer *>(r->sdl_renderer), &info) == 0) {
                std::lock_guard lock(state_mutex);
                renderer_name = info.name ? info.name : "unknown";
            }
            auto window = reinterpret_cast<SDL_Window *>(r->window);
            watched_window = sdl.window_id(window);
            window_focused = sdl.keyboard_focus() == window;
            if (!event_watch_installed) {
                sdl.add_event_watch(watch_focus, nullptr);
                event_watch_installed = true;
            }
            SDL_DisplayMode mode{};
            if (sdl.desktop_mode(sdl.display_index(reinterpret_cast<SDL_Window *>(r->window)), &mode) == 0) {
                desktop_width = mode.w;
                desktop_height = mode.h;
            }
        }
        bool ok = true;
        for (auto *hook : renderer_hooks) {
            if (!hook->apply())
                ok = false;
        }
        if (!ok) {
            remove_hooks();
            std::lock_guard lock(state_mutex);
            last_error = "Could not attach renderer hooks";
        }
        hooks_ready = ok;
        prepare_pending = false;
        if (gps)
            gps->force_full_display_count = 2;
    });
    lua_pushboolean(L, false);
    return 1;
}

// Start teardown without waiting. This can be requested from Lua, where
// blocking the sim/core-lock context would deadlock a render thread waiting for
// the next simulation acknowledgement. status().stopped is the completion fence.
static void request_stop() {
    std::lock_guard lock(jobs_mutex);
    const bool retry_failed_cleanup = !cleanup_job || (cleanup_job->finished() && !cleanup_job->succeeded());
    if (closing.exchange(true) && !retry_failed_cleanup)
        return;
    for (const auto &job : render_jobs)
        job->cancel();
    {
        std::lock_guard state_lock(state_mutex);
        camera = {};
    }
    cleanup_job = queueOnRenderThread([] {
        remove_hooks();
        build_verified = false;
        prepare_pending = false;
        if (gps)
            gps->force_full_display_count = 2;
    });
}

static bool stopped() {
    std::lock_guard lock(jobs_mutex);
    return stopped_inline || (cleanup_job && cleanup_job->finished() && cleanup_job->succeeded());
}

static int lua_stop(lua_State *L) {
    std::lock_guard admission(jobs_mutex);
    request_stop();
    lua_pushboolean(L, stopped());
    return 1;
}

DFhackCExport command_result plugin_init(color_ostream &, std::vector<PluginCommand> &) {
    // Function pointers are immutable once hooks can run on the render thread.
    return bind_sdl() ? CR_OK : CR_FAILURE;
}

DFhackCExport command_result plugin_onupdate(color_ostream &) {
    // This callback runs on simulation, where inspecting viewscreens is legal.
    // Publish only a disable decision; rendering never follows a screen pointer.
    std::lock_guard lock(state_mutex);
    if (camera.enabled &&
        (closing || camera.context != current_context() || camera.port != gps->main_map_port ||
         camera.screen != Gui::getDFViewscreen(true))) {
        camera.enabled = false;
        camera.generation = ++camera_generation;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == SC_WORLD_UNLOADED || event == SC_MAP_UNLOADED) {
        std::lock_guard lock(state_mutex);
        camera = {};
    }
    if (event != SC_BEGIN_UNLOAD)
        return CR_OK;
    auto &core = Core::getInstance();
    if (core.isRenderThread()) {
        // Core::Shutdown runs here after the SDL loop, before DF destroys its
        // renderer. There can be no renderer hook in flight on this thread.
        std::lock_guard lock(jobs_mutex);
        closing = true;
        for (const auto &job : render_jobs)
            job->cancel();
        if (cleanup_job)
            cleanup_job->cancel();
        remove_hooks();
        build_verified = false;
        stopped_inline = true;
        return CR_OK;
    }
    if (stopped())
        return CR_OK;
    if (!build_verified && !prepare_pending)
        return CR_OK; // never prepared: no callbacks, event watch, or hooks
    if (core.isSuspended()) {
        out.printerr("Cannot wait for renderer cleanup while holding the core lock. "
                     "Call stop(), wait for status().stopped on later frames, then unload; "
                     "or run unload from the DFHack console.\n");
        return CR_FAILURE;
    }
    request_stop();
    // The console path owns no core lock. A bounded wait avoids hanging unload
    // if the game has stopped servicing SDL. A timeout REFUSES unload while
    // keeping the module and cleanup task alive; it never unloads pending code.
    if (!cleanup_job->waitFor(std::chrono::milliseconds(2000)) || !stopped()) {
        out.printerr(
            "Renderer cleanup is not complete; unload refused. Retry after the next rendered frame.\n");
        return CR_FAILURE;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &) {
    // SC_BEGIN_UNLOAD established that every callback/capture and renderer
    // entry point is gone. No plugin lambda is queued after this return.
    profiling = false;
    native_ui.clear();
    render_jobs.clear();
    cleanup_job.reset();
    return CR_OK;
}
