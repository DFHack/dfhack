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

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <SDL.h>

namespace worldmap_zoom {

// World-map cells are fixed at 16 pixels in DF's map-port renderer. This is
// independent of the text font size and of the requested magnification.
constexpr int map_cell_pixels = 16;
constexpr int max_canvas_pixels = 16384; // bound allocations even if native geometry is corrupt

struct CanvasView {
    double zoom = 1, x = 0, y = 0;
    uint64_t generation = 0;
};

struct CanvasStats {
    uint64_t creations = 0, composites = 0, restore_checks = 0, restore_errors = 0;
    uint64_t composited_generation = 0;
};

// A render-thread-only target-texture pass. This class has no DF globals,
// viewscreens, callback queue, or implicit dependencies on its includer.
// Api supplies the SDL operations, allowing tests to inject failures without
// redefining DF/SDL types. See the methodology section in the plugin docs.
template <class Api> class MapCanvas {
public:
    explicit MapCanvas(Api &api) : sdl(api) {}
    const CanvasStats &stats() const {
        return counters;
    }
    const std::string &error() const {
        return failure;
    }
    uint64_t generation() const {
        return view.generation;
    }
    bool active() const {
        return pass_active;
    }
    void markFullDraw() {
        valid = true;
    }
    void clearError() {
        failure.clear();
    }

    bool begin(SDL_Renderer *renderer, uint32_t window_id, const SDL_Rect &map_rect,
               const CanvasView &requested_view, uint64_t device_generation, bool full_draw) {
        if (!failure.empty())
            return false;
        // The texture uses native output coordinates: DF's unmodified draw
        // methods can draw at their usual positive origin. A negative/scaled
        // viewport clips incorrectly on some Direct3D backends.
        if (!validGeometry(map_rect, requested_view))
            return fail("Invalid map canvas geometry or zoom");
        if (!ensureTexture(renderer, window_id, map_rect, device_generation))
            return false;
        const bool new_view = view.generation != requested_view.generation || !valid;
        if (pass_active && view.generation != requested_view.generation)
            finish(false, [] {});
        if (!failure.empty())
            return false;
        view = requested_view;
        map = map_rect;
        configureCrop();
        const bool starting_pass = !pass_active;
        if (starting_pass && new_view) {
            valid = false;
            resetTileCoverage();
        }
        if (starting_pass && !beginTargetPass())
            return false;

        // A full rebuild overwrites the whole crop. Incremental updates must
        // preserve the existing texture between batches, but a *new* crop has
        // no valid pixels yet and must start from a known background.
        const bool needs_clear = full_draw || (starting_pass && !valid);
        if (needs_clear && !clearTarget()) {
            finish(false, [] {});
            return fail("Could not clear map canvas");
        }
        if (sdl.set_clip(owner, &source) != 0) {
            finish(false, [] {});
            return fail("SDL rejected the map crop");
        }
        return true;
    }

    template <class Redraw> void finish(bool may_composite, Redraw redraw_full_map) {
        if (!pass_active)
            return;
        // DF may supply only changed cells. If those cells did not cover the
        // newly selected crop, redraw the native map once before enlarging it.
        // The adapter owns the native pointers; this class only invokes it.
        if (may_composite && !valid) {
            if (!clearTarget()) {
                may_composite = false;
                fail("Could not clear incomplete map canvas");
            } else {
                redraw_full_map();
                valid = true;
            }
        }
        pass_active = false;
        if (!restoreDestinationState())
            return;
        // Restore *before* copying: the map belongs in the same native output
        // rectangle as before, and subsequent UI draws must use DF's state.
        if (may_composite && valid) {
            if (sdl.copy(owner, texture, &source, &map) != 0)
                fail("SDL map composition failed");
            else {
                ++counters.composites;
                counters.composited_generation = view.generation;
            }
        }
    }

    void release(SDL_Renderer *current_renderer, uint32_t current_window) {
        // SDL_DestroyRenderer also destroys its textures. If DF replaced the
        // renderer/window, forget the old pointer instead of double-freeing it.
        const bool same_owner = texture && owner == current_renderer && window == current_window;
        if (same_owner) {
            finish(false, [] {});
            sdl.destroy_texture(texture);
        } else {
            pass_active = false;
        }
        texture = nullptr;
        owner = nullptr;
        valid = false;
        width = height = 0;
    }

    void clearTile(int x, int y) {
        // A changed native cell can become transparent/empty. Clear just that
        // cell so stale river/site/cloud pixels do not survive in our cache.
        SDL_Rect cell{map.x + x * map_cell_pixels, map.y + y * map_cell_pixels, map_cell_pixels,
                      map_cell_pixels};
        const bool cleared =
            sdl.set_color(owner, 0, 0, 0, SDL_ALPHA_OPAQUE) == 0 && sdl.fill(owner, &cell) == 0;
        const bool color_restored = restoreDrawColor();
        if (!cleared || !color_restored)
            fail("Could not clear a changed map cell");
    }

    void paintedTile(int x, int y) {
        if (valid || x < first_col || x > last_col || y < first_row || y > last_row)
            return;
        const auto index = static_cast<size_t>(y) * columns + x;
        if (!painted[index]) {
            painted[index] = true;
            valid = ++painted_count == required_cells;
        }
    }

private:
    Api &sdl;
    SDL_Renderer *owner = nullptr;
    SDL_Texture *texture = nullptr;
    uint32_t window = 0;
    int width = 0, height = 0;
    uint64_t device_epoch = 0;
    bool pass_active = false, valid = false;
    CanvasView view;
    CanvasStats counters;
    std::string failure;
    SDL_Rect map{}, source{}, saved_viewport{}, saved_clip{};
    SDL_Texture *saved_target = nullptr;
    float saved_scale_x = 1, saved_scale_y = 1;
    bool saved_clipped = false;
    std::array<unsigned char, 4> saved_color{};
    std::vector<bool> painted;
    int columns = 0, first_col = 0, last_col = 0, first_row = 0, last_row = 0;
    int painted_count = 0, required_cells = 0;

    bool fail(const char *message) {
        failure = message;
        return false;
    }

    bool validGeometry(const SDL_Rect &rect, const CanvasView &v) const {
        return rect.x >= 0 && rect.y >= 0 && rect.w > 0 && rect.h > 0 &&
               static_cast<int64_t>(rect.x) + rect.w <= max_canvas_pixels &&
               static_cast<int64_t>(rect.y) + rect.h <= max_canvas_pixels && rect.w % map_cell_pixels == 0 &&
               rect.h % map_cell_pixels == 0 && (v.zoom == 2 || v.zoom == 4) && std::isfinite(v.x) &&
               std::isfinite(v.y);
    }

    bool ensureTexture(SDL_Renderer *renderer, uint32_t window_id, const SDL_Rect &rect, uint64_t epoch) {
        const int wanted_width = rect.x + rect.w, wanted_height = rect.y + rect.h;
        if (texture && (owner != renderer || window != window_id || width != wanted_width ||
                        height != wanted_height || device_epoch != epoch))
            release(renderer, window_id);
        if (texture)
            return true;
        SDL_RendererInfo info{};
        if (sdl.renderer_info(renderer, &info) != 0 || !(info.flags & SDL_RENDERER_TARGETTEXTURE) ||
            (info.max_texture_width > 0 && wanted_width > info.max_texture_width) ||
            (info.max_texture_height > 0 && wanted_height > info.max_texture_height))
            return fail("Renderer cannot create the required map target texture");
        texture = sdl.create_texture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
                                     wanted_width, wanted_height);
        if (!texture)
            return fail("Could not create map target texture");
        owner = renderer;
        window = window_id;
        width = wanted_width;
        height = wanted_height;
        device_epoch = epoch;
        valid = false;
        ++counters.creations;
        // The native map is opaque and nearest-neighbour magnification keeps
        // the original pixel art sharp. Do not blend in old window contents.
        if (sdl.texture_blend(texture, SDL_BLENDMODE_NONE) != 0 ||
            sdl.texture_scale(texture, SDL_ScaleModeNearest) != 0) {
            release(renderer, window_id);
            return fail("Could not configure map target texture");
        }
        return true;
    }

    void configureCrop() {
        const double cols = static_cast<double>(map.w) / map_cell_pixels;
        const double rows = static_cast<double>(map.h) / map_cell_pixels;
        const int offset_x = static_cast<int>(
            std::round(std::clamp(view.x, 0.0, cols * (1 - 1 / view.zoom)) * map_cell_pixels));
        const int offset_y = static_cast<int>(
            std::round(std::clamp(view.y, 0.0, rows * (1 - 1 / view.zoom)) * map_cell_pixels));
        source = {map.x + offset_x, map.y + offset_y, map.w / static_cast<int>(view.zoom),
                  map.h / static_cast<int>(view.zoom)};
    }

    void resetTileCoverage() {
        columns = map.w / map_cell_pixels;
        painted.assign(static_cast<size_t>(columns) * (map.h / map_cell_pixels), false);
        painted_count = 0;
        first_col = (source.x - map.x) / map_cell_pixels;
        last_col = (source.x - map.x + source.w - 1) / map_cell_pixels;
        first_row = (source.y - map.y) / map_cell_pixels;
        last_row = (source.y - map.y + source.h - 1) / map_cell_pixels;
        required_cells = (last_col - first_col + 1) * (last_row - first_row + 1);
    }

    bool beginTargetPass() {
        saved_target = sdl.get_target(owner);
        sdl.get_scale(owner, &saved_scale_x, &saved_scale_y);
        sdl.get_viewport(owner, &saved_viewport);
        sdl.get_clip(owner, &saved_clip);
        saved_clipped = sdl.clip_enabled(owner) != 0;
        sdl.get_color(owner, &saved_color[0], &saved_color[1], &saved_color[2], &saved_color[3]);
        pass_active = true; // every failure below must restore the saved state
        if (sdl.set_target(owner, texture) != 0 || sdl.set_scale(owner, 1, 1) != 0 ||
            sdl.set_viewport(owner, nullptr) != 0 || sdl.set_clip(owner, nullptr) != 0) {
            finish(false, [] {});
            return fail("SDL rejected the map target pass");
        }
        return true;
    }

    bool restoreDrawColor() {
        return sdl.set_color(owner, saved_color[0], saved_color[1], saved_color[2], saved_color[3]) == 0;
    }

    bool clearTarget() {
        // SDL_RenderClear clears the entire target regardless of its clip.
        // Preserve DF's draw colour so native drawing observes its usual state.
        const bool cleared = sdl.set_color(owner, 0, 0, 0, SDL_ALPHA_OPAQUE) == 0 && sdl.clear(owner) == 0;
        return restoreDrawColor() && cleared;
    }

    bool restoreDestinationState() {
        bool ok = sdl.set_target(owner, saved_target) == 0;
        ok = (sdl.set_scale(owner, saved_scale_x, saved_scale_y) == 0) && ok;
        ok = (sdl.set_viewport(owner, &saved_viewport) == 0) && ok;
        ok = (sdl.set_clip(owner, saved_clipped ? &saved_clip : nullptr) == 0) && ok;
        ok = restoreDrawColor() && ok;
        // Check the state we restored, not just SDL's return values. A bad
        // restore would otherwise corrupt all later interface drawing.
        float x = 0, y = 0;
        SDL_Rect viewport{}, clip{};
        std::array<unsigned char, 4> color{};
        sdl.get_scale(owner, &x, &y);
        sdl.get_viewport(owner, &viewport);
        sdl.get_clip(owner, &clip);
        sdl.get_color(owner, &color[0], &color[1], &color[2], &color[3]);
        const auto same_rect = [](const SDL_Rect &a, const SDL_Rect &b) {
            return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
        };
        ok = ok && sdl.get_target(owner) == saved_target && x == saved_scale_x && y == saved_scale_y &&
             same_rect(viewport, saved_viewport) && (sdl.clip_enabled(owner) != 0) == saved_clipped &&
             (!saved_clipped || same_rect(clip, saved_clip)) && color == saved_color;
        ++counters.restore_checks;
        if (!ok) {
            ++counters.restore_errors;
            return fail("SDL canvas state restoration failed");
        }
        return true;
    }
};

} // namespace worldmap_zoom
