// SPDX-License-Identifier: Zlib
// The production header is included normally. Tests inject SDL operations, not
// fake DF classes or globals; no renderer/GPU/game is required.
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <array>
#include <cassert>
#include <iostream>

#include <SDL.h>

#include "worldmap-zoom-canvas.h"

struct MockState {
    SDL_Texture *target = nullptr;
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
        *info = {};
        info->name = "mock";
        info->flags = SDL_RENDERER_TARGETTEXTURE;
        info->max_texture_width = info->max_texture_height = 16384;
        return 0;
    }
    SDL_Texture *create_texture(SDL_Renderer *, unsigned, int, int, int) {
        ++mock.textures;
        return reinterpret_cast<SDL_Texture *>(&texture);
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
    SDL_Texture *get_target(SDL_Renderer *) {
        return mock.target;
    }
    int set_target(SDL_Renderer *, SDL_Texture *target) {
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

static auto *renderer = reinterpret_cast<SDL_Renderer *>(1);
static constexpr uint32_t window_id = 1;
static constexpr SDL_Rect map{16, 32, 512, 384};
static worldmap_zoom::CanvasView view{2, 4, 3, 1};

static bool equal(const SDL_Rect &a, const SDL_Rect &b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

static void crop_and_restore(double zoom) {
    mock = {};
    view = {zoom, 4, 3, 1};
    const auto before = mock;
    worldmap_zoom::MapCanvas<MockSdl> canvas(sdl);
    assert(canvas.begin(renderer, window_id, map, view, 1, true));
    assert(mock.target == reinterpret_cast<SDL_Texture *>(&texture));
    assert(mock.viewport.x == 0 && mock.viewport.y == 0 && mock.sx == 1 && mock.sy == 1);
    canvas.markFullDraw();
    canvas.finish(true, [] { assert(false && "A full draw does not need fallback"); });
    assert(mock.copies == 1);
    assert(mock.copied_source.x == 80 && mock.copied_source.y == 80);
    assert(mock.copied_source.w == 512 / zoom && mock.copied_source.h == 384 / zoom);
    assert(equal(mock.copied_destination, map));
    assert(mock.target == before.target && mock.sx == before.sx && mock.sy == before.sy);
    assert(equal(mock.viewport, before.viewport) && equal(mock.clip, before.clip));
    assert(mock.clipped == before.clipped && mock.color == before.color);
    assert(canvas.stats().restore_errors == 0);
    canvas.release(renderer, window_id);
    assert(mock.destroys == 1);
}

int main() {
    crop_and_restore(2);
    crop_and_restore(4);

    mock = {};
    view = {2, 4, 3, 1};
    int redraws = 0;
    worldmap_zoom::MapCanvas<MockSdl> partial(sdl);
    assert(partial.begin(renderer, window_id, map, view, 1, false));
    partial.paintedTile(4, 3);
    partial.finish(true, [&] { ++redraws; });
    assert(redraws == 1 && mock.copies == 1);
    partial.release(renderer, window_id);

    mock = {};
    redraws = 0;
    worldmap_zoom::MapCanvas<MockSdl> complete(sdl);
    assert(complete.begin(renderer, window_id, map, view, 1, false));
    // Crop: columns [4,19], rows [3,14]. Repeated dirty cells must not count twice.
    complete.paintedTile(4, 3);
    complete.paintedTile(4, 3);
    for (int y = 3; y <= 14; ++y)
        for (int x = 4; x <= 19; ++x)
            complete.paintedTile(x, y);
    complete.finish(true, [&] { ++redraws; });
    assert(redraws == 0 && mock.copies == 1);
    assert(complete.begin(renderer, window_id, map, view, 2, true));
    assert(mock.textures == 2 && mock.destroys == 1);
    complete.release(renderer, window_id);

    mock = {};
    worldmap_zoom::MapCanvas<MockSdl> stale(sdl);
    assert(stale.begin(renderer, window_id, map, view, 1, false));
    stale.finish(false, [] { assert(false && "A disabled/stale view must not redraw"); });
    assert(mock.copies == 0 && !stale.active());
    stale.release(renderer, window_id);

    mock = {};
    worldmap_zoom::MapCanvas<MockSdl> failed(sdl);
    mock.fail_target = true;
    assert(!failed.begin(renderer, window_id, map, view, 1, true));
    assert(!failed.error().empty() && !failed.active() && mock.copies == 0);
    mock.fail_target = false;
    failed.release(renderer, window_id);

    mock = {};
    const auto before = mock;
    worldmap_zoom::MapCanvas<MockSdl> copy_failed(sdl);
    assert(copy_failed.begin(renderer, window_id, map, view, 1, true));
    copy_failed.markFullDraw();
    mock.fail_copy = true;
    copy_failed.finish(true, [] {});
    assert(!copy_failed.error().empty() && !copy_failed.active());
    assert(equal(mock.viewport, before.viewport) && mock.color == before.color);
    copy_failed.release(renderer, window_id);

    mock = {};
    worldmap_zoom::MapCanvas<MockSdl> invalid(sdl);
    assert(!invalid.begin(renderer, window_id, SDL_Rect{-1, 0, 512, 384}, view, 1, true));
    assert(!invalid.begin(renderer, window_id, SDL_Rect{0, 0, 32768, 384}, view, 1, true));
    assert(mock.textures == 0);

    mock = {};
    worldmap_zoom::MapCanvas<MockSdl> replaced(sdl);
    assert(replaced.begin(renderer, window_id, map, view, 1, true));
    // SDL already freed this texture with the old renderer. Do not free it again.
    replaced.release(reinterpret_cast<SDL_Renderer *>(2), window_id + 1);
    assert(mock.destroys == 0 && !replaced.active());
    std::cout << "PASS: 2x/4x crops, SDL state, tile coverage, device reset, stale camera, "
                 "failure recovery, geometry bounds, and renderer replacement\n";
}
