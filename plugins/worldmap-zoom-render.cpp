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

#include "lauxlib.h"
#include "lua.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

using namespace DFHack;
DFHACK_PLUGIN("worldmap-zoom-render");
REQUIRE_GLOBAL(enabler);
REQUIRE_GLOBAL(gps);

// Resolve SDL functions from DFHack's SDL handle; no separate SDL runtime.
struct SdlApi {
    void (*get_scale)(SDL_Renderer *, float *, float *) = nullptr;
    int (*set_scale)(SDL_Renderer *, float, float) = nullptr;
    void (*get_viewport)(SDL_Renderer *, SDL_Rect *) = nullptr;
    int (*set_viewport)(SDL_Renderer *, const SDL_Rect *) = nullptr;
    void (*get_clip)(SDL_Renderer *, SDL_Rect *) = nullptr;
    int (*set_clip)(SDL_Renderer *, const SDL_Rect *) = nullptr;
    int (*clip_enabled)(SDL_Renderer *) = nullptr;
    int (*read_pixels)(SDL_Renderer *, const SDL_Rect *, unsigned, void *, int) = nullptr;
    int (*output_size)(SDL_Renderer *, int *, int *) = nullptr;
    void (*window_size)(SDL_Window *, int, int) = nullptr;
    int (*fullscreen)(SDL_Window *, unsigned) = nullptr;
    int (*display_index)(SDL_Window *) = nullptr;
    int (*desktop_mode)(int, SDL_DisplayMode *) = nullptr;
    unsigned (*window_id)(SDL_Window *) = nullptr;
    SDL_Window *(*keyboard_focus)() = nullptr;
    void (*add_event_watch)(int (*)(void *, SDL_Event *), void *) = nullptr;
    void (*del_event_watch)(int (*)(void *, SDL_Event *), void *) = nullptr;
    int (*renderer_info)(SDL_Renderer *, SDL_RendererInfo *) = nullptr;
    void *(*get_target)(SDL_Renderer *) = nullptr;
    int (*set_target)(SDL_Renderer *, void *) = nullptr;
    void *(*create_texture)(SDL_Renderer *, unsigned, int, int, int) = nullptr;
    void (*destroy_texture)(void *) = nullptr;
    int (*texture_blend)(void *, int) = nullptr;
    int (*texture_scale)(void *, int) = nullptr;
    int (*get_color)(SDL_Renderer *, unsigned char *, unsigned char *, unsigned char *,
                     unsigned char *) = nullptr;
    int (*set_color)(SDL_Renderer *, unsigned char, unsigned char, unsigned char, unsigned char) = nullptr;
    int (*clear)(SDL_Renderer *) = nullptr;
    int (*fill)(SDL_Renderer *, const SDL_Rect *) = nullptr;
    int (*copy)(SDL_Renderer *, void *, const SDL_Rect *, const SDL_Rect *) = nullptr;
} sdl;

struct Camera {
    bool enabled = false;
    int context = 0;
    double zoom = 1, x = 0, y = 0;
};
static std::mutex state_mutex;
static Camera camera;
static std::string capture_path, last_capture, last_error, capture_error;
static std::atomic_bool hooks_ready{false};
static std::atomic_bool build_verified{false}, prepare_pending{false};
static std::atomic_uint64_t full_calls{0}, tile_calls{0}, transformed_calls{0};
static std::atomic_uint32_t render_thread{0};
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
static void remove_hooks();
static int lua_prepare(lua_State *L);

static bool pin_module() {
    // DFHack's render callback queue may outlive a logical plugin unload. Pin
    // code until process exit, while detaching hooks and releasing all activity.
    // This also closes the fetched-vtable-call/unload race on the graphics thread.
    static HMODULE pinned = nullptr;
    if (!pinned && !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                                       reinterpret_cast<LPCWSTR>(&pin_module), &pinned))
        return false;
    return true;
}

static bool bind_sdl() {
    auto h = DFSDL::obtain_library_handle();
#define BIND(member, name)                                                                                   \
    sdl.member = reinterpret_cast<decltype(sdl.member)>(LookupPlugin(h, name));                              \
    if (!sdl.member)                                                                                         \
    return false
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
    if (event && event->window.type == 0x200 && event->window.windowID == watched_window.load()) {
        if (event->window.event == 13) {
            window_focused = false;
            ++focus_epoch;
        } else if (event->window.event == 12)
            window_focused = true;
    }
    if (event && (event->window.type == 0x2000 || event->window.type == 0x2001))
        ++target_generation;
    return 1;
}

static int current_context() {
    auto screen = Gui::getDFViewscreen(true);
    if (auto s = virtual_cast<df::viewscreen_choose_start_sitest>(screen))
        return s->zoomed_in || s->page != df::choose_start_site_view_mode::Biome ? 0 : 1;
    if (auto s = virtual_cast<df::viewscreen_worldst>(screen))
        return s->view_mode == df::world_view_mode_type::NORMAL ? 2 : 0;
    return 0;
}

#include "worldmap-zoom-canvas.h"

static void capture(df::renderer_2d_base *r) {
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
        if (sdl.read_pixels(sr, nullptr, 0x16762004, pixels.data(), w * 4) != 0)
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
        render_thread = GetCurrentThreadId();
        ++full_calls;
        const bool measure = profiling.load();
        const auto start =
            measure ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
        Camera c;
        if (!transform_depth && canvas_camera(this, port, c) && map_canvas.begin(this, port, c, true)) {
            ++transform_depth;
            INTERPOSE_NEXT(update_full_map_port)(port);
            --transform_depth;
            map_canvas.valid = true;
            ++transformed_calls;
            map_canvas.finish();
        } else {
            if (!transform_depth)
                map_canvas.finish();
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
        if (!transform_depth && canvas_camera(this, port, c) && map_canvas.begin(this, port, c, false)) {
            ++transform_depth;
            map_canvas.clear_tile(x, y);
            INTERPOSE_NEXT(update_map_port_tile)(port, x, y);
            map_canvas.painted_tile(x, y);
            --transform_depth;
            ++transformed_calls;
            // Keep consecutive tile updates in one target pass. The next UI
            // draw or present composites it once, below the native interface.
        } else {
            if (!transform_depth)
                map_canvas.finish();
            INTERPOSE_NEXT(update_map_port_tile)(port, x, y);
        }
    }
    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        render_thread = GetCurrentThreadId();
        map_canvas.finish();
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
#define UI_HOOK(method, signature, args)                                                                     \
    DEFINE_VMETHOD_INTERPOSE(void, method, signature) {                                                      \
        start_frame();                                                                                       \
        if (!transform_depth)                                                                                \
            map_canvas.finish();                                                                             \
        INTERPOSE_NEXT(method) args;                                                                         \
    }
    UI_HOOK(update_all, (), ())
    UI_HOOK(update_tile, (int32_t x, int32_t y), (x, y))
    UI_HOOK(update_anchor_tile, (int32_t x, int32_t y), (x, y))
    UI_HOOK(update_top_tile, (int32_t x, int32_t y), (x, y))
    UI_HOOK(update_top_anchor_tile, (int32_t x, int32_t y), (x, y))
    UI_HOOK(update_viewport_tile, (df::graphic_viewportst * p, int32_t x, int32_t y), (p, x, y))
    UI_HOOK(update_full_viewport, (df::graphic_viewportst * p), (p))
#undef UI_HOOK
    DEFINE_VMETHOD_INTERPOSE(void, do_blank_screen_fill, ()) {
        start_frame();
        map_canvas.finish(false);
        INTERPOSE_NEXT(do_blank_screen_fill)();
    }
    DEFINE_VMETHOD_INTERPOSE(void, resize, (int32_t w, int32_t h)) {
        map_canvas.release(this);
        INTERPOSE_NEXT(resize)(w, h);
    }
    DEFINE_VMETHOD_INTERPOSE(void, grid_resize, (int32_t w, int32_t h)) {
        map_canvas.release(this);
        INTERPOSE_NEXT(grid_resize)(w, h);
    }
    DEFINE_VMETHOD_INTERPOSE(void, set_fullscreen, ()) {
        map_canvas.release(this);
        INTERPOSE_NEXT(set_fullscreen)();
    }
    DEFINE_VMETHOD_INTERPOSE(bool, init_video, (int32_t w, int32_t h)) {
        map_canvas.release(this);
        return INTERPOSE_NEXT(init_video)(w, h);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_full_map_port);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, update_map_port_tile);
IMPLEMENT_VMETHOD_INTERPOSE(map_hook, render);
#define CANVAS_BOUNDARIES(X)                                                                                 \
    X(update_all)                                                                                            \
    X(update_tile)                                                                                           \
    X(update_anchor_tile)                                                                                    \
    X(update_top_tile) X(update_top_anchor_tile) X(update_viewport_tile) X(update_full_viewport)             \
        X(do_blank_screen_fill) X(resize) X(grid_resize) X(set_fullscreen) X(init_video)
#define DECLARE_BOUNDARY(name) IMPLEMENT_VMETHOD_INTERPOSE(map_hook, name);
CANVAS_BOUNDARIES(DECLARE_BOUNDARY)
#undef DECLARE_BOUNDARY

static void remove_hooks() {
    if (enabler && enabler->renderer)
        map_canvas.release(virtual_cast<df::renderer_2d_base>(enabler->renderer));
    if (event_watch_installed) {
        sdl.del_event_watch(watch_focus, nullptr);
        event_watch_installed = false;
    }
    watched_window = 0;
    window_focused = false;
    hooks_ready = false;
    INTERPOSE_HOOK(map_hook, update_full_map_port).remove();
    INTERPOSE_HOOK(map_hook, update_map_port_tile).remove();
    INTERPOSE_HOOK(map_hook, render).remove();
#define REMOVE_BOUNDARY(name) INTERPOSE_HOOK(map_hook, name).remove();
    CANVAS_BOUNDARIES(REMOVE_BOUNDARY)
#undef REMOVE_BOUNDARY
}

static int lua_camera(lua_State *L) {
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
        camera = c;
    }
    ++camera_generation;
    if (!c.enabled && build_verified)
        runOnRenderThread([] { map_canvas.release(virtual_cast<df::renderer_2d_base>(enabler->renderer)); });
    if (build_verified && gps)
        gps->force_full_display_count = 2;
    return 0;
}

static int zoom_status(lua_State *L) {
    lua_newtable(L);
#define INT_FIELD(name, value)                                                                               \
    lua_pushinteger(L, static_cast<lua_Integer>(value));                                                     \
    lua_setfield(L, -2, name)
    lua_pushboolean(L, hooks_ready);
    lua_setfield(L, -2, "ready");
    INT_FIELD("full_calls", full_calls.load());
    INT_FIELD("tile_calls", tile_calls.load());
    INT_FIELD("transformed_calls", transformed_calls.load());
    INT_FIELD("render_thread", render_thread.load());
    INT_FIELD("context", build_verified ? current_context() : 0);
    INT_FIELD("restore_checks", restore_checks.load());
    INT_FIELD("restore_errors", restore_errors.load());
    INT_FIELD("desktop_width", desktop_width.load());
    INT_FIELD("desktop_height", desktop_height.load());
    INT_FIELD("output_width", output_width.load());
    INT_FIELD("output_height", output_height.load());
    INT_FIELD("focus_epoch", focus_epoch.load());
    INT_FIELD("canvas_creations", canvas_creations.load());
    INT_FIELD("canvas_composites", canvas_composites.load());
    INT_FIELD("camera_generation", camera_generation.load());
    INT_FIELD("presented_generation", presented_generation.load());
    lua_pushboolean(L, window_focused.load());
    lua_setfield(L, -2, "window_focused");
    {
        std::lock_guard lock(state_mutex);
        INT_FIELD("profile_samples", profile_us.size());
        INT_FIELD("frame_profile_samples", frame_profile_us.size());
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
    if (!build_verified || !hooks_ready)
        return luaL_error(L, "Renderer is not ready");
    const int w = static_cast<int>(luaL_checkinteger(L, 1)), h = static_cast<int>(luaL_checkinteger(L, 2));
    const bool full = lua_toboolean(L, 3) != 0;
    if (full || enabler->fullscreen_state.bits.fullscreen)
        return luaL_error(L, "Use native DF settings for fullscreen tests");
    if (w < 640 || h < 480 || w > 7680 || h > 4320)
        return luaL_error(L, "Invalid test window dimensions");
    runOnRenderThread([w, h] {
        auto r = virtual_cast<df::renderer_2d>(enabler->renderer);
        if (!r)
            return;
        auto window = reinterpret_cast<SDL_Window *>(r->window);
        sdl.window_size(window, w, h);
        // Programmatic SDL resizing emits SIZE_CHANGED, while DF handles
        // RESIZED. Let the native event loop pause simulation and reshape its
        // buffers, exactly as for an ordinary user window resize.
        SDL_Event event{};
        event.window.type = 0x200;
        event.window.windowID = sdl.window_id(window);
        event.window.event = 5;
        event.window.data1 = w;
        event.window.data2 = h;
        DFSDL::DFSDL_PushEvent(&event);
    });
    return 0;
}

static int lua_capture(lua_State *L) {
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
    if (!build_verified || !hooks_ready)
        return luaL_error(L, "Renderer is not ready");
    const bool focused = lua_toboolean(L, 1) != 0;
    runOnRenderThread([focused] {
        SDL_Event event{};
        event.window.type = 0x200;
        event.window.windowID = watched_window.load();
        event.window.event = focused ? 12 : 13;
        DFSDL::DFSDL_PushEvent(&event);
    });
    return 0;
}

DFhackCExport CommandReg plugin_lua_commands[] = {
    {"prepare", lua_prepare},      {"setCamera", lua_camera},  {"status", zoom_status},
    {"capture", lua_capture},      {"profile", lua_profile},   {"snapshotUi", lua_snapshot_ui},
    {"uiHit", lua_ui_hit},         {"testWindow", lua_window}, {"frameProfile", lua_frame_profile},
    {"testFocus", lua_test_focus}, {nullptr, nullptr}};

static int lua_prepare(lua_State *L) {
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
    if (!enabler || !virtual_cast<df::renderer_2d>(enabler->renderer) || !bind_sdl()) {
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
    if (!pin_module()) {
        prepare_pending = false;
        return luaL_error(L, "Could not retain the renderer module safely");
    }
    {
        std::lock_guard lock(state_mutex);
        last_error.clear();
    }
    build_verified = true;
    runOnRenderThread([] {
        if (!build_verified) {
            prepare_pending = false;
            return;
        }
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
        bool ok = INTERPOSE_HOOK(map_hook, update_full_map_port).apply() &&
                  INTERPOSE_HOOK(map_hook, update_map_port_tile).apply() &&
                  INTERPOSE_HOOK(map_hook, render).apply();
#define APPLY_BOUNDARY(name) ok = INTERPOSE_HOOK(map_hook, name).apply() && ok;
        CANVAS_BOUNDARIES(APPLY_BOUNDARY)
#undef APPLY_BOUNDARY
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

DFhackCExport command_result plugin_init(color_ostream &, std::vector<PluginCommand> &) {
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &, state_change_event event) {
    if (event == SC_WORLD_UNLOADED || event == SC_MAP_UNLOADED) {
        std::lock_guard lock(state_mutex);
        camera = {};
        if (build_verified)
            runOnRenderThread(
                [] { map_canvas.release(virtual_cast<df::renderer_2d_base>(enabler->renderer)); });
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &) {
    {
        std::lock_guard lock(state_mutex);
        camera = {};
    }
    profiling = false;
    native_ui.clear();
    ui_cols = ui_rows = 0;
    if (build_verified.exchange(false))
        runOnRenderThread([] {
            remove_hooks();
            if (gps)
                gps->force_full_display_count = 2;
        });
    return CR_OK;
}
