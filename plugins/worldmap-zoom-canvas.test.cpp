// SPDX-License-Identifier: Zlib
// Exercise the production canvas state machine without a game or GPU.
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

struct SDL_Rect {
    int x, y, w, h;
};
struct SDL_Window {};
struct SDL_Renderer {};
struct SDL_RendererInfo {
    const char *name;
    unsigned flags;
    int max_texture_width, max_texture_height;
};
namespace df {
struct graphic_map_portst {
    int top_left_corner_x = 16, top_left_corner_y = 32;
    int dim_x = 32, dim_y = 24;
    struct {
        struct {
            bool active = true;
        } bits;
    } flag;
};
struct renderer_2d_base {
    int origin_x = 0, origin_y = 0;
    SDL_Renderer *sdl_renderer = nullptr;
    SDL_Window *window = nullptr;
    int full_draws = 0;
    void update_full_map_port(graphic_map_portst *) {
        ++full_draws;
    }
};
} // namespace df
struct Camera {
    bool enabled = true;
    int context = 1;
    double zoom = 2, x = 4, y = 3;
};
static std::mutex state_mutex;
static Camera camera;
static std::string last_error, renderer_name;
static std::atomic_uint64_t camera_generation{1}, target_generation{1};
static std::atomic_uint64_t canvas_creations{0}, canvas_composites{0}, composited_generation{0};
static std::atomic_uint64_t restore_checks{0}, restore_errors{0};
static bool hooks_ready = true;
static thread_local int transform_depth = 0;
static int context = 1;
static int current_context() {
    return context;
}
static struct {
    df::graphic_map_portst *main_map_port;
} graphics;
static auto gps = &graphics;
static struct {
    df::renderer_2d_base *renderer;
} engine;
static auto enabler = &engine;

struct MockState {
    void *target = nullptr;
    SDL_Rect viewport{3, 5, 900, 700}, clip{7, 9, 600, 400};
    float sx = 1.5f, sy = 2.f;
    bool clipped = true, fail_target = false, fail_copy = false;
    std::array<unsigned char, 4> color{11, 22, 33, 44};
    SDL_Rect copied_source{}, copied_destination{};
    unsigned window_id = 1;
    int clears = 0, copies = 0, destroys = 0, textures = 0;
} mock;
static int texture;

struct MockSdl {
    unsigned window_id(SDL_Window *) {
        return mock.window_id;
    }
    int renderer_info(SDL_Renderer *, SDL_RendererInfo *info) {
        *info = {"mock", 8, 16384, 16384};
        return 0;
    }
    void *create_texture(SDL_Renderer *, unsigned, int, int, int) {
        ++mock.textures;
        return &texture;
    }
    void destroy_texture(void *) {
        ++mock.destroys;
    }
    int texture_blend(void *, int) {
        return 0;
    }
    int texture_scale(void *, int) {
        return 0;
    }
    void *get_target(SDL_Renderer *) {
        return mock.target;
    }
    int set_target(SDL_Renderer *, void *target) {
        if (mock.fail_target)
            return -1;
        mock.target = target;
        return 0;
    }
    void get_scale(SDL_Renderer *, float *x, float *y) {
        *x = mock.sx;
        *y = mock.sy;
    }
    int set_scale(SDL_Renderer *, float x, float y) {
        mock.sx = x;
        mock.sy = y;
        return 0;
    }
    void get_viewport(SDL_Renderer *, SDL_Rect *rect) {
        *rect = mock.viewport;
    }
    int set_viewport(SDL_Renderer *, const SDL_Rect *rect) {
        mock.viewport = rect ? *rect : SDL_Rect{0, 0, 1024, 768};
        return 0;
    }
    void get_clip(SDL_Renderer *, SDL_Rect *rect) {
        *rect = mock.clip;
    }
    int set_clip(SDL_Renderer *, const SDL_Rect *rect) {
        mock.clipped = rect != nullptr;
        mock.clip = rect ? *rect : SDL_Rect{};
        return 0;
    }
    int clip_enabled(SDL_Renderer *) {
        return mock.clipped;
    }
    int get_color(SDL_Renderer *, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a) {
        *r = mock.color[0];
        *g = mock.color[1];
        *b = mock.color[2];
        *a = mock.color[3];
        return 0;
    }
    int set_color(SDL_Renderer *, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
        mock.color = {r, g, b, a};
        return 0;
    }
    int clear(SDL_Renderer *) {
        ++mock.clears;
        return 0;
    }
    int fill(SDL_Renderer *, const SDL_Rect *) {
        return 0;
    }
    int copy(SDL_Renderer *, void *, const SDL_Rect *source, const SDL_Rect *dest) {
        if (mock.fail_copy)
            return -1;
        ++mock.copies;
        mock.copied_source = *source;
        mock.copied_destination = *dest;
        return 0;
    }
} sdl;

#include "worldmap-zoom-canvas.h"

static SDL_Renderer renderer;
static SDL_Window window;
static df::renderer_2d_base native;
static df::graphic_map_portst port;

static void reset() {
    mock = {};
    camera = {};
    context = 1;
    ++camera_generation;
    ++target_generation;
    last_error.clear();
    native = {};
    native.sdl_renderer = &renderer;
    native.window = &window;
    gps->main_map_port = &port;
    enabler->renderer = &native;
}

static bool equal(const SDL_Rect &a, const SDL_Rect &b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

static void crop_and_restore(double zoom) {
    reset();
    camera.zoom = zoom;
    const auto before = mock;
    MapCanvas canvas;
    assert(canvas.begin(&native, &port, camera, true));
    assert(mock.target == &texture);
    assert(mock.viewport.x == 0 && mock.viewport.y == 0);
    assert(mock.sx == 1 && mock.sy == 1);
    canvas.valid = true;
    canvas.finish();
    assert(mock.copies == 1);
    assert(mock.copied_source.x == 16 + 4 * 16 && mock.copied_source.y == 32 + 3 * 16);
    assert(mock.copied_source.w == 512 / zoom && mock.copied_source.h == 384 / zoom);
    assert(equal(mock.copied_destination, SDL_Rect{16, 32, 512, 384}));
    assert(mock.target == before.target && mock.sx == before.sx && mock.sy == before.sy);
    assert(equal(mock.viewport, before.viewport) && equal(mock.clip, before.clip));
    assert(mock.clipped == before.clipped && mock.color == before.color);
    canvas.release(&native);
    assert(mock.destroys == 1);
}

int main() {
    crop_and_restore(2);
    crop_and_restore(4);

    // Partial tile batches must finish with a full native draw before copying.
    reset();
    MapCanvas partial;
    assert(partial.begin(&native, &port, camera, false));
    partial.painted_tile(partial.x0, partial.y0);
    partial.finish();
    assert(native.full_draws == 1 && mock.copies == 1);
    partial.release(&native);

    // A complete tile batch needs no redundant full redraw.
    reset();
    MapCanvas complete;
    assert(complete.begin(&native, &port, camera, false));
    for (int y = complete.y0; y <= complete.y1; ++y)
        for (int x = complete.x0; x <= complete.x1; ++x)
            complete.painted_tile(x, y);
    complete.finish();
    assert(native.full_draws == 0 && mock.copies == 1);
    ++target_generation;
    assert(complete.begin(&native, &port, camera, true));
    assert(mock.textures == 2 && mock.destroys == 1);
    complete.release(&native);

    // Leaving the supported screen must never composite stale geographic data.
    reset();
    MapCanvas stale;
    assert(stale.begin(&native, &port, camera, false));
    context = 0;
    stale.finish();
    assert(mock.copies == 0 && native.full_draws == 0 && !stale.active);
    stale.release(&native);

    // SDL failures disable zoom instead of leaving altered graphics state active.
    reset();
    MapCanvas failed;
    mock.fail_target = true;
    assert(!failed.begin(&native, &port, camera, true));
    assert(!camera.enabled && !last_error.empty() && !failed.active && mock.copies == 0);
    mock.fail_target = false;
    failed.release(&native);

    reset();
    MapCanvas copy_failed;
    assert(copy_failed.begin(&native, &port, camera, true));
    copy_failed.valid = true;
    mock.fail_copy = true;
    copy_failed.finish();
    assert(!camera.enabled && !last_error.empty() && !copy_failed.active);
    copy_failed.release(&native);

    reset();
    Camera selected;
    assert(canvas_camera(&native, &port, selected));
    camera.zoom = 1;
    assert(!canvas_camera(&native, &port, selected));
    std::cout << "PASS: crop at 2x/4x, state restoration, partial/complete tile batches, "
                 "device reset, screen changes, SDL failure recovery, and map gating\n";
}
