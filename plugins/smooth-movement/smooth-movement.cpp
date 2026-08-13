// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <SDL_render.h>

#include "Core.h"
#include "MemAccess.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

#include "modules/DFSDL.h"

#include "df/coord2d.h"
#include "df/enabler.h"
#include "df/graphic.h"
#include "df/graphic_viewportst.h"
#include "df/renderer_2d_base.h"
#include "df/texture_fullid.h"

#include "visual_animation.h"

using namespace DFHack;

DFHACK_PLUGIN("smooth-movement");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(enabler);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(window_x);
REQUIRE_GLOBAL(window_y);
REQUIRE_GLOBAL(window_z);

namespace {

using tile_coveragest = std::set<df::coord2d>;

// Runtime harness for the engine-owned visual state; gameplay data is never
// read.
decltype(&SDL_RenderCopyF) render_copy_f = nullptr;
decltype(&SDL_RenderCopyExF) render_copy_ex_f = nullptr;
decltype(&SDL_RenderFillRect) render_fill_rect = nullptr;
decltype(&SDL_RenderSetClipRect) render_set_clip_rect = nullptr;
decltype(&SDL_GetRenderDrawColor) get_render_draw_color = nullptr;
decltype(&SDL_SetRenderDrawColor) set_render_draw_color = nullptr;

visual_animation_managerst animation_manager;
tile_coveragest previous_coverage;
uint64_t visual_context_revision = 0;
const void *previous_viewport = nullptr;
struct visual_contextst {
    int32_t z_level;
    df::coord2d viewport_dimensions;
    df::coord2d clip_min;
    df::coord2d clip_max;
    int32_t zoom;
    df::coord2d origin;
    df::coord2d screen_dimensions;

    bool operator==(const visual_contextst &) const = default;
};
visual_contextst previous_visual_context{};
bool has_visual_context = false;
// Map scroll (window_x/window_y) is tracked separately from the reset
// signature: a pure pan is followed (movements are translated) instead of
// triggering a full reset, so it must NOT bump the context revision. It only
// invalidates the viewport-space blackout coverage from the prior frame.
df::coord2d previous_pan = df::coord2d(0, 0);
bool has_pan_context = false;
bool flip_enabled = false;
bool zlevel_enabled = false;

// --- free camera
// -------------------------------------------------------------------------------
// The camera is visually unbound from the tile grid. Two layered offsets:
//   rest      -- a PERSISTENT sub-tile offset in tiles: the free camera. Set by
//   pixel-perfect
//                middle-mouse drag panning (the view rests wherever released,
//                mid-tile or not) and by the `smooth-movement camera <fx> <fy>`
//                console command. Survives zoom and z-level changes. Kept in
//                [-0.5,0.5] by normalization: whole-tile parts are folded into
//                window_x/window_y (a plain UI scroll write -- NEVER the
//                viewport dims, which crash DF; the sub-tile strip this leaves
//                at one screen edge has no buffer data and stays black).
//   transient -- the decaying scroll glide from before, in pixels, layered on
//   top.
// Render offset = transient + rest*tile. window_x/window_y remain the game's
// own tile camera.
bool camera_enabled = false;                  // OFF by default: plain `enable smooth-movement`
                                              // keeps upstream behavior (creature interpolation
                                              // only); `smooth-movement camera on` opts in.
constexpr int32_t camera_max_glide_tiles = 3; // per-jump: farther than this snaps instantly
constexpr double camera_tau_ms = 35.0;        // transient catch-up (~95% done after 100ms)
point2dst<double> transient;                  // decaying glide offset, pixels
point2dst<double> rest;                       // persistent free-camera offset, tiles
df::coord2d camera_pending = df::coord2d(0, 0); // scroll delta not yet in the buffers
int32_t camera_pending_frames = 0;
df::coord2d self_scroll = df::coord2d(0, 0); // window deltas WE wrote: visual no-ops when landing
bool drag_active = false;
point2dst<double> drag_anchor_view;   // visual camera at drag start, tiles
df::coord2d drag_anchor_mouse = df::coord2d(0, 0); // precise mouse at drag start, pixels
bool camera_was_offset = false;       // edge-detects offset->0 for one cleanup redraw
df::coord2d camera_previous_window = df::coord2d(0, 0); // window-scroll observation baseline
bool camera_has_prev = false;

double tile_px(const df::renderer_2d_base *renderer) {
    const int32_t zoom = renderer->viewport_zoom_factor;
    return double(zoom == 128 ? 32 : std::max(1, zoom * 32 / 128));
}

// Match ratio of "buffers shifted by (dwx,dwy)" on the background layer: 0..1,
// or -1 when there is nothing to compare (empty background).
double background_match_ratio(const df::graphic_viewportst *vp, int32_t dwx, int32_t dwy) {
    int32_t considered = 0;
    int32_t matches = 0;
    for (int32_t x = 0; x < vp->dim_x; ++x) {
        const int32_t sx = x + dwx;
        if (sx < 0 || sx >= vp->dim_x)
            continue;
        for (int32_t y = 0; y < vp->dim_y; ++y) {
            const int32_t sy = y + dwy;
            if (sy < 0 || sy >= vp->dim_y)
                continue;
            const int32_t cur = vp->screentexpos_background[x * vp->dim_y + y];
            if (cur == 0)
                continue;
            ++considered;
            if (vp->screentexpos_background_old[sx * vp->dim_y + sy] == cur)
                ++matches;
        }
    }
    if (considered == 0)
        return -1.0;
    return double(matches) / double(considered);
}

// Cancel everything except the persistent rest offset (the camera keeps its
// sub-tile position across zoom/z/resize; only the in-flight animation state is
// unfollowable).
void clear_camera_pending() {
    camera_pending.x = 0;
    camera_pending.y = 0;
    camera_pending_frames = 0;
    self_scroll.x = 0;
    self_scroll.y = 0;
}

void cancel_camera_transients() {
    transient.x = 0.0;
    transient.y = 0.0;
    clear_camera_pending();
    drag_active = false;
}

void set_camera_enabled(bool enable) {
    if (camera_enabled == enable)
        return;
    camera_enabled = enable;
    cancel_camera_transients();
    rest.x = 0.0;
    rest.y = 0.0;
    camera_has_prev = false; // fresh observation baseline; no phantom scroll on re-enable
    // camera_was_offset stays: the render path issues one cleanup redraw if we
    // were mid-offset.
}

// Fold whole tiles of rest into window_x/window_y so |rest| <= 0.5 (minimal
// edge strip). The visual position is unchanged: the window write is attributed
// via self_scroll when it lands.
void normalize_rest() {
    const int32_t kx = int32_t(-std::llround(rest.x));
    const int32_t ky = int32_t(-std::llround(rest.y));
    if (kx != 0 && window_x != nullptr && *window_x + kx >= 0) {
        *window_x += kx;
        self_scroll.x += kx;
    }
    if (ky != 0 && window_y != nullptr && *window_y + ky >= 0) {
        *window_y += ky;
        self_scroll.y += ky;
    }
}

// A scroll of (ax,ay) tiles has landed in the buffers: our own normalization
// writes are visual no-ops (they move into rest); the remainder is a real
// scroll and glides -- unless a drag is driving the position directly, in which
// case it folds into rest wholesale.
void attribute_landed(int32_t ax, int32_t ay, double tile) {
    int32_t sx = 0;
    if (self_scroll.x != 0 && (self_scroll.x > 0) == (ax > 0) && ax != 0)
        sx = (std::abs(self_scroll.x) <= std::abs(ax)) ? self_scroll.x : ax;
    int32_t sy = 0;
    if (self_scroll.y != 0 && (self_scroll.y > 0) == (ay > 0) && ay != 0)
        sy = (std::abs(self_scroll.y) <= std::abs(ay)) ? self_scroll.y : ay;
    self_scroll.x -= sx;
    self_scroll.y -= sy;
    rest.x += sx;
    rest.y += sy;
    const int32_t gx = ax - sx;
    const int32_t gy = ay - sy;
    if (drag_active) {
        rest.x += gx;
        rest.y += gy;
    } else {
        transient.x += gx * tile;
        transient.y += gy * tile;
        const double cap = tile * (camera_max_glide_tiles + 0.5);
        transient.x = std::clamp(transient.x, -cap, cap);
        transient.y = std::clamp(transient.y, -cap, cap);
    }
}

// Per-frame camera bookkeeping: observe window scrolls, attribute them when the
// buffers apply them (glide vs our own normalization writes), drive the drag,
// decay the transient.
void update_camera(df::renderer_2d_base *renderer, const df::graphic_viewportst *vp,
                   uint32_t delta_ms) {
    if (!camera_enabled)
        return;
    const double tile = tile_px(renderer);
    const int32_t wx = window_x ? *window_x : 0;
    const int32_t wy = window_y ? *window_y : 0;
    if (camera_has_prev && (wx != camera_previous_window.x || wy != camera_previous_window.y)) {
        const int32_t dx = wx - camera_previous_window.x;
        const int32_t dy = wy - camera_previous_window.y;
        if ((std::abs(dx) > camera_max_glide_tiles || std::abs(dy) > camera_max_glide_tiles) &&
            !drag_active)
            cancel_camera_transients(); // teleport-like jump (recenter/minimap): snap
        else {
            camera_pending.x += dx;
            camera_pending.y += dy;
            camera_pending_frames = 0;
        }
    }
    camera_previous_window.x = wx;
    camera_previous_window.y = wy;
    camera_has_prev = true;

    if (camera_pending.x != 0 || camera_pending.y != 0) {
        if (std::abs(camera_pending.x) > 6 || std::abs(camera_pending.y) > 6) {
            // Scrolling far outran detection: snap (keep rest, drop the animation
            // debt).
            transient.x = 0.0;
            transient.y = 0.0;
            clear_camera_pending();
        } else {
            // Fast scrolling applies the pending delta PIECEMEAL: the buffers may
            // hold +1 of a pending +3 this frame. Testing only the total made
            // landings miss, time out, and snap -- the fast-scroll jitter. Instead,
            // find the LARGEST applied prefix of the pending scroll and attribute
            // just that; the rest keeps pending. Ties between qualifying shifts only
            // happen on uniform terrain, where mistiming is invisible.
            const int32_t stepx = (camera_pending.x > 0) - (camera_pending.x < 0);
            const int32_t stepy = (camera_pending.y > 0) - (camera_pending.y < 0);
            int32_t best_ax = 0, best_ay = 0, best_mag = -1;
            double best_score = -1.0;
            bool no_data = false;
            for (int32_t ix = 0; ix <= std::abs(camera_pending.x); ++ix) {
                for (int32_t iy = 0; iy <= std::abs(camera_pending.y); ++iy) {
                    const double score = background_match_ratio(vp, ix * stepx, iy * stepy);
                    if (score < 0.0) {
                        no_data = true;
                        break;
                    }
                    const int32_t mag = ix + iy;
                    if (score >= 0.6 &&
                        (mag > best_mag || (mag == best_mag && score > best_score))) {
                        best_mag = mag;
                        best_score = score;
                        best_ax = ix * stepx;
                        best_ay = iy * stepy;
                    }
                }
                if (no_data)
                    break;
            }
            if (no_data) {
                // Nothing to compare against (empty background): give up on
                // attribution.
                clear_camera_pending();
            } else if (best_mag > 0) {
                attribute_landed(best_ax, best_ay, tile);
                camera_pending.x -= best_ax;
                camera_pending.y -= best_ay;
                camera_pending_frames = 0;
            } else if (best_mag == 0) {
                // Content demonstrably hasn't moved yet: keep waiting, no timeout
                // pressure.
                camera_pending_frames = 0;
            } else if (++camera_pending_frames > 4) {
                // Neither static nor any prefix recognizable (heavy simultaneous
                // change): drop the debt without touching the in-flight glide.
                clear_camera_pending();
            }
        }
    }

    // --- pixel-perfect middle-mouse drag: the view follows the mouse 1:1 and
    // rests where released. DF's own drag still moves window in tile steps; rest
    // carries the remainder. Positions are tracked against the CONTENT window
    // (window minus unlanded jumps) so the buffer lag never causes a visible
    // stutter.
    const bool mbut = enabler != nullptr && enabler->mouse_mbut;
    const double content_wx = double(wx - camera_pending.x);
    const double content_wy = double(wy - camera_pending.y);
    if (mbut && !drag_active && gps != nullptr) {
        drag_active = true;
        drag_anchor_view.x = content_wx - rest.x - transient.x / tile;
        drag_anchor_view.y = content_wy - rest.y - transient.y / tile;
        drag_anchor_mouse.x = gps->precise_mouse_x;
        drag_anchor_mouse.y = gps->precise_mouse_y;
        transient.x = 0.0;
        transient.y = 0.0;
    }
    if (drag_active) {
        if (!mbut) {
            drag_active = false;
            normalize_rest();
        } else if (gps != nullptr) {
            double vx =
                drag_anchor_view.x - double(gps->precise_mouse_x - drag_anchor_mouse.x) / tile;
            double vy =
                drag_anchor_view.y - double(gps->precise_mouse_y - drag_anchor_mouse.y) / tile;
            rest.x = content_wx - vx;
            rest.y = content_wy - vy;
            // If DF's own drag disagrees by more than a tile and a half, rebase on
            // its view.
            const double lim = 1.5;
            if (rest.x < -lim || rest.x > lim || rest.y < -lim || rest.y > lim) {
                rest.x = std::clamp(rest.x, -lim, lim);
                rest.y = std::clamp(rest.y, -lim, lim);
                drag_anchor_view.x =
                    content_wx - rest.x + double(gps->precise_mouse_x - drag_anchor_mouse.x) / tile;
                drag_anchor_view.y =
                    content_wy - rest.y + double(gps->precise_mouse_y - drag_anchor_mouse.y) / tile;
            }
        }
    }

    if (transient.x != 0.0 || transient.y != 0.0) {
        const double k = std::exp(-double(delta_ms) / camera_tau_ms);
        transient.x *= k;
        transient.y *= k;
        if (std::abs(transient.x) < 0.5 && std::abs(transient.y) < 0.5) {
            transient.x = 0.0;
            transient.y = 0.0;
        }
    }
}

constexpr uint32_t fire_bits = 0x70000000U;

void update_visual_context(const df::renderer_2d_base *renderer, const df::graphic_viewportst *vp) {
    // window_x/window_y are deliberately excluded: a horizontal/vertical scroll
    // is followed, not reset. window_z (z-level) stays, since a z change is not
    // followable.
    const visual_contextst context = {
        .z_level = window_z ? *window_z : 0,
        .viewport_dimensions = df::coord2d(vp->dim_x, vp->dim_y),
        .clip_min = df::coord2d(vp->clipx[0], vp->clipy[0]),
        .clip_max = df::coord2d(vp->clipx[1], vp->clipy[1]),
        .zoom = renderer->viewport_zoom_factor,
        .origin = df::coord2d(renderer->origin_x, renderer->origin_y),
        .screen_dimensions = df::coord2d(gps->dimx, gps->dimy),
    };
    const bool changed =
        !has_visual_context || previous_viewport != vp || previous_visual_context != context;
    if (changed) {
        ++visual_context_revision;
        previous_coverage.clear();
        cancel_camera_transients();
    }
    previous_viewport = vp;
    previous_visual_context = context;
    has_visual_context = true;

    // On a pure pan the reset signature is unchanged, but last frame's blackout
    // coverage is in the old viewport frame, so discard it (the engine repaints
    // the whole scrolled viewport anyway).
    const int32_t pan_x = window_x ? *window_x : 0;
    const int32_t pan_y = window_y ? *window_y : 0;
    if (!has_pan_context || previous_pan.x != pan_x || previous_pan.y != pan_y)
        previous_coverage.clear();
    previous_pan.x = pan_x;
    previous_pan.y = pan_y;
    has_pan_context = true;
}

using viewport_layer_memberst = int32_t *df::graphic_viewportst::*;

struct visual_layer_bufferst {
    viewport_visual_layer layer;
    viewport_layer_memberst current;
    viewport_layer_memberst previous;
};

constexpr size_t visual_layer_count = static_cast<size_t>(viewport_visual_layer::count);
constexpr std::array visual_layer_buffers = {
    visual_layer_bufferst{viewport_visual_layer::right,
                          &df::graphic_viewportst::screentexpos_right_creature,
                          &df::graphic_viewportst::screentexpos_right_creature_old},
    visual_layer_bufferst{viewport_visual_layer::center, &df::graphic_viewportst::screentexpos,
                          &df::graphic_viewportst::screentexpos_old},
    visual_layer_bufferst{viewport_visual_layer::left,
                          &df::graphic_viewportst::screentexpos_left_creature,
                          &df::graphic_viewportst::screentexpos_left_creature_old},
    visual_layer_bufferst{viewport_visual_layer::upright,
                          &df::graphic_viewportst::screentexpos_upright_creature,
                          &df::graphic_viewportst::screentexpos_upright_creature_old},
    visual_layer_bufferst{viewport_visual_layer::up,
                          &df::graphic_viewportst::screentexpos_up_creature,
                          &df::graphic_viewportst::screentexpos_up_creature_old},
    visual_layer_bufferst{viewport_visual_layer::upleft,
                          &df::graphic_viewportst::screentexpos_upleft_creature,
                          &df::graphic_viewportst::screentexpos_upleft_creature_old},
    visual_layer_bufferst{viewport_visual_layer::vehicle,
                          &df::graphic_viewportst::screentexpos_vehicle,
                          &df::graphic_viewportst::screentexpos_vehicle_old},
    visual_layer_bufferst{viewport_visual_layer::item, &df::graphic_viewportst::screentexpos_item,
                          &df::graphic_viewportst::screentexpos_item_old},
    visual_layer_bufferst{viewport_visual_layer::designation,
                          &df::graphic_viewportst::screentexpos_designation,
                          &df::graphic_viewportst::screentexpos_designation_old}};

constexpr bool valid_visual_layer_buffers() {
    uint16_t layers = 0;
    for (const auto &buffer : visual_layer_buffers) {
        const uint16_t layer = uint16_t(1U << static_cast<uint8_t>(buffer.layer));
        if (layers & layer)
            return false;
        layers |= layer;
    }
    return layers == uint16_t((1U << visual_layer_count) - 1);
}

static_assert(valid_visual_layer_buffers());

template <typename Viewport> auto visual_layers(Viewport *vp, bool previous = false) {
    using layer_pointer = std::conditional_t<std::is_const_v<Viewport>, const int32_t *, int32_t *>;
    std::array<layer_pointer, visual_layer_count> layers{};
    for (const auto &buffer : visual_layer_buffers)
        layers[static_cast<size_t>(buffer.layer)] =
            vp->*(previous ? buffer.previous : buffer.current);
    return layers;
}

viewport_visual_animation_inputst animation_input(df::graphic_viewportst *vp) {
    const df::graphic_viewportst *const_viewport = vp;
    return {vp,
            df::coord2d(vp->dim_x, vp->dim_y),
            visual_context_revision,
            visual_layers(const_viewport),
            visual_layers(const_viewport, true),
            df::coord2d(window_x ? *window_x : 0, window_y ? *window_y : 0)};
}

// The layer buffers are freed and nulled without clearing the active flag.
bool viewport_readable(df::graphic_viewportst *vp) {
    return vp != nullptr && vp->flag.bits.active && animation_input(vp).valid();
}

int32_t tile_pixel(int32_t tile, int32_t origin, int32_t zoom) {
    return zoom == 128 ? 32 * tile + origin : (zoom * 32 * tile) / 128 + origin;
}

bool inside_clip(const df::graphic_viewportst *vp, int32_t x, int32_t y) {
    return x >= vp->clipx[0] && x <= vp->clipx[1] && y >= vp->clipy[0] && y <= vp->clipy[1];
}

bool has_fire(const df::graphic_viewportst *vp, int32_t x, int32_t y) {
    return vp->screentexpos_spatter_flag != nullptr &&
           (vp->screentexpos_spatter_flag[x * vp->dim_y + y] & fire_bits) != 0;
}

template <typename T> class scoped_value_restorest {
    T &value;
    T saved;

  public:
    explicit scoped_value_restorest(T &value, T replacement = T{})
        : value(value), saved(std::exchange(value, std::move(replacement))) {
        static_assert(std::is_nothrow_move_assignable_v<T>);
    }

    ~scoped_value_restorest() noexcept { value = std::move(saved); }

    scoped_value_restorest(const scoped_value_restorest &) = delete;
    scoped_value_restorest &operator=(const scoped_value_restorest &) = delete;
    scoped_value_restorest(scoped_value_restorest &&) = delete;
    scoped_value_restorest &operator=(scoped_value_restorest &&) = delete;
};

template <typename Callback> void with_zeroed_values(const Callback &callback) { callback(); }

template <typename Callback, typename T, typename... Values>
void with_zeroed_values(const Callback &callback, T &value, Values &...values) {
    scoped_value_restorest<T> zero(value);
    with_zeroed_values(callback, values...);
}

struct render_proxyst {
    viewport_visual_layer layer;
    point2dst<float> source;
    df::coord2d target;
    int32_t texpos;
    float progress;
    SDL_Texture *texture;
    bool mirrored = false;
    int32_t mirror_shift = 0;
    tile_coveragest coverage;
};

struct render_coveragest {
    tile_coveragest all;
    std::array<tile_coveragest, static_cast<size_t>(visual_render_groupst::count)> groups;
    std::unordered_map<int32_t, uint16_t> selected;
};

struct viewport_renderst {
    df::graphic_viewportst *viewport;
    std::vector<render_proxyst> proxies;
    render_coveragest coverage;
};

constexpr uint16_t visual_layer_bit(viewport_visual_layer layer) {
    return uint16_t(1U << static_cast<uint8_t>(layer));
}

uint16_t selected_mask(const std::unordered_map<int32_t, uint16_t> &selected, int32_t index) {
    const auto found = selected.find(index);
    return found == selected.end() ? 0 : found->second;
}

template <size_t Layer = 0, typename Callback>
void with_suppressed_visual_layers(const std::array<int32_t *, visual_layer_count> &layers,
                                   int32_t index, uint16_t mask, const Callback &callback) {
    if constexpr (Layer == visual_layer_count)
        callback();
    else if (mask & (1U << Layer)) {
        scoped_value_restorest<int32_t> zero(layers[Layer][index]);
        with_suppressed_visual_layers<Layer + 1>(layers, index, mask, callback);
    } else
        with_suppressed_visual_layers<Layer + 1>(layers, index, mask, callback);
}

template <typename Callback>
void with_base_suppressed(df::graphic_viewportst *vp, int32_t index, const Callback &callback) {
    with_zeroed_values(callback, vp->screentexpos_background[index],
                       vp->screentexpos_floor_flag[index], vp->screentexpos_background_two[index],
                       vp->screentexpos_liquid_flag[index], vp->screentexpos_spatter_flag[index],
                       vp->screentexpos_spatter[index], vp->screentexpos_ramp_flag[index],
                       vp->screentexpos_shadow_flag[index], vp->screentexpos_building_one[index]);
}

template <typename Callback>
void with_main_suppressed(df::graphic_viewportst *vp, int32_t index, const Callback &callback) {
    with_base_suppressed(vp, index,
                         [&] { with_zeroed_values(callback, vp->screentexpos_vermin[index]); });
}

template <typename Callback>
void with_upper_suppressed(df::graphic_viewportst *vp, int32_t index, const Callback &callback) {
    with_main_suppressed(vp, index, [&] {
        with_zeroed_values(callback, vp->screentexpos_building_two[index],
                           vp->screentexpos_projectile[index], vp->screentexpos_high_flow[index],
                           vp->screentexpos_top_shadow[index], vp->screentexpos_signpost[index]);
    });
}

void redraw_viewport_tile(df::renderer_2d_base *renderer, const viewport_renderst &viewport,
                          int32_t x, int32_t y, bool defer_interface) {
    df::graphic_viewportst *vp = viewport.viewport;
    const int32_t index = x * vp->dim_y + y;
    const auto redraw = [&] { renderer->update_viewport_tile(vp, x, y); };
    const auto stage = [&] {
        with_suppressed_visual_layers(visual_layers(vp), index,
                                      selected_mask(viewport.coverage.selected, index), redraw);
    };
    // The interface layer is the shading for levels below the camera.
    // A staged tile has a sprite drawn over it afterwards, so draw_interface_only
    // places it instead.
    if (!defer_interface || vp->screentexpos_interface == nullptr)
        stage();
    else
        with_zeroed_values(stage, vp->screentexpos_interface[index]);
}

// Every buffer the interface-only pass zeroes has to exist before it can be
// zeroed.
bool interface_pass_readable(const df::graphic_viewportst *vp) {
    return vp != nullptr && vp->screentexpos_interface != nullptr &&
           vp->screentexpos_background != nullptr && vp->screentexpos_floor_flag != nullptr &&
           vp->screentexpos_background_two != nullptr && vp->screentexpos_liquid_flag != nullptr &&
           vp->screentexpos_spatter_flag != nullptr && vp->screentexpos_spatter != nullptr &&
           vp->screentexpos_ramp_flag != nullptr && vp->screentexpos_shadow_flag != nullptr &&
           vp->screentexpos_building_one != nullptr && vp->screentexpos_vermin != nullptr &&
           vp->screentexpos_building_two != nullptr && vp->screentexpos_projectile != nullptr &&
           vp->screentexpos_high_flow != nullptr && vp->screentexpos_signpost != nullptr;
}

// Runs after the proxies so the shading covers them rather than sitting
// underneath.
void draw_interface_only(df::renderer_2d_base *renderer, df::graphic_viewportst *vp, int32_t x,
                         int32_t y) {
    if (!interface_pass_readable(vp))
        return;
    const int32_t index = x * vp->dim_y + y;
    const auto redraw = [&] { renderer->update_viewport_tile(vp, x, y); };
    const auto without_visuals = [&] {
        with_suppressed_visual_layers(visual_layers(vp), index,
                                      uint16_t((1U << visual_layer_count) - 1), redraw);
    };
    with_zeroed_values(without_visuals, vp->screentexpos_background[index],
                       vp->screentexpos_floor_flag[index], vp->screentexpos_background_two[index],
                       vp->screentexpos_liquid_flag[index], vp->screentexpos_spatter_flag[index],
                       vp->screentexpos_spatter[index], vp->screentexpos_ramp_flag[index],
                       vp->screentexpos_shadow_flag[index], vp->screentexpos_building_one[index],
                       vp->screentexpos_vermin[index], vp->screentexpos_building_two[index],
                       vp->screentexpos_projectile[index], vp->screentexpos_high_flow[index],
                       vp->screentexpos_signpost[index]);
}

void redraw_world_tile(df::renderer_2d_base *renderer,
                       const std::vector<viewport_renderst> &viewports,
                       const tile_coveragest &staged, int32_t x, int32_t y) {
    // The stage pass repaints everything above the lowest across the staged
    // tiles, after the proxies.
    const bool staged_tile = staged.count(df::coord2d(x, y)) != 0;
    for (const viewport_renderst &viewport : viewports) {
        if (inside_clip(viewport.viewport, x, y))
            redraw_viewport_tile(renderer, viewport, x, y, staged_tile);
        if (staged_tile)
            break;
    }
}

constexpr uint16_t visual_layers_through_group(visual_render_groupst group) {
    uint16_t mask = 0;
    for (const auto &descriptor : visual_layer_descriptors)
        if (descriptor.render_group != visual_render_groupst::designation &&
            static_cast<uint8_t>(descriptor.render_group) <= static_cast<uint8_t>(group))
            mask |= visual_layer_bit(descriptor.layer);
    return mask;
}

void redraw_above(df::renderer_2d_base *renderer, df::graphic_viewportst *vp, int32_t x, int32_t y,
                  visual_render_groupst group,
                  const std::unordered_map<int32_t, uint16_t> &selected) {
    const int32_t index = x * vp->dim_y + y;
    const auto redraw = [&] { renderer->update_viewport_tile(vp, x, y); };
    const auto suppress_visuals = [&] {
        const auto stage = [&] {
            with_suppressed_visual_layers(
                visual_layers(vp), index,
                selected_mask(selected, index) | visual_layers_through_group(group), redraw);
        };
        // The interface layer sits above every group, so each group's redraw would
        // paint it again. draw_interface_only places it once, after the sprites.
        if (vp->screentexpos_interface == nullptr)
            stage();
        else
            with_zeroed_values(stage, vp->screentexpos_interface[index]);
    };
    if (group == visual_render_groupst::item || group == visual_render_groupst::vehicle)
        with_base_suppressed(vp, index, suppress_visuals);
    else if (group == visual_render_groupst::main)
        with_main_suppressed(vp, index, suppress_visuals);
    else
        with_upper_suppressed(vp, index, suppress_visuals);
}

SDL_Texture *cached_texture(df::renderer_2d_base *renderer, int32_t texpos,
                            bool transparent_background = true) {
    if (texpos == 0)
        return nullptr;
    df::texture_fullid texture_id;
    texture_id.texpos = texpos;
    texture_id.r = texture_id.g = texture_id.b = 1.0f;
    texture_id.br = texture_id.bg = texture_id.bb = 0.0f;
    texture_id.flag =
        transparent_background ? df::texture_fullid_flag::mask_transparent_background : 0;
    const auto texture = renderer->tile_cache.tile_cache.find(texture_id);
    return texture == renderer->tile_cache.tile_cache.end()
               ? nullptr
               : static_cast<SDL_Texture *>(texture->second);
}

// The render_copy_ex_f null check is defensive only, not a graceful-degradation
// path. `bind` aborts load_sdl on any missing symbol and plugin_enable then
// refuses the render hook.
void render_copy_maybe_mirrored(SDL_Renderer *renderer, SDL_Texture *texture,
                                const SDL_FRect &destination, bool mirrored) {
    if (mirrored && render_copy_ex_f != nullptr) {
        render_copy_ex_f(renderer, texture, nullptr, &destination, 0.0, nullptr,
                         SDL_FLIP_HORIZONTAL);
        return;
    }
    render_copy_f(renderer, texture, nullptr, &destination);
}

void draw_proxy(df::renderer_2d_base *renderer, const render_proxyst &proxy) {
    const int32_t zoom = renderer->viewport_zoom_factor;
    const int32_t target_x = tile_pixel(proxy.target.x, renderer->origin_x, zoom);
    const int32_t target_y = tile_pixel(proxy.target.y, renderer->origin_y, zoom);
    const float tile_size = float(zoom == 128 ? 32 : std::max(1, zoom * 32 / 128));
    const float source_x = target_x + (proxy.source.x - proxy.target.x) * tile_size;
    const float source_y = target_y + (proxy.source.y - proxy.target.y) * tile_size;
    const float mirror_offset = float(proxy.mirror_shift) * tile_size;
    const SDL_FRect destination = {
        source_x + (target_x - source_x) * proxy.progress + mirror_offset,
        source_y + (target_y - source_y) * proxy.progress, tile_size, tile_size};
    render_copy_maybe_mirrored(static_cast<SDL_Renderer *>(renderer->sdl_renderer), proxy.texture,
                               destination, proxy.mirrored);
}

std::vector<render_proxyst> collect_proxies(df::renderer_2d_base *renderer,
                                            df::graphic_viewportst *vp, bool movement_enabled) {
    std::vector<render_proxyst> proxies;
    auto layers = visual_layers(vp);
    auto previous_layers = visual_layers(vp, true);
    for (const auto visual_layer : visual_layer_draw_order) {
        const size_t layer = static_cast<size_t>(visual_layer);
        for (int32_t y = 0; y < vp->dim_y; ++y) {
            for (int32_t x = 0; x < vp->dim_x; ++x) {
                const int32_t index = x * vp->dim_y + y;
                const int32_t texpos = layers[layer][index];
                if (texpos == 0)
                    continue;
                if (!movement_enabled)
                    continue;
                const auto movement = animation_manager.get_movement(
                    vp, static_cast<viewport_visual_layer>(layer), x, y);
                if (!movement.active)
                    continue;
                const int32_t inherited_source_x =
                    inherited_visual_source_tile(x, movement.source.x, x);
                const int32_t inherited_source_y =
                    inherited_visual_source_tile(y, movement.source.y, y);
                const bool inherited_source_in_bounds =
                    inherited_source_x >= 0 && inherited_source_x < vp->dim_x &&
                    inherited_source_y >= 0 && inherited_source_y < vp->dim_y;
                if (!visual_layer_moves_independently(visual_layer)) {
                    bool anchored = false;
                    for (const render_proxyst &anchor : proxies) {
                        if (anchor.layer == viewport_visual_layer::center &&
                            std::abs(anchor.target.x - x) <= 1 &&
                            std::abs(anchor.target.y - y) <= 1 &&
                            anchor.source.x - anchor.target.x == movement.source.x - x &&
                            anchor.source.y - anchor.target.y == movement.source.y - y &&
                            anchor.progress == movement.progress)
                            anchored = true;
                    }
                    if (!anchored)
                        continue;
                }
                if ((visual_layer == viewport_visual_layer::item ||
                     visual_layer == viewport_visual_layer::designation) &&
                    movement.inherited) {
                    if (visual_layer == viewport_visual_layer::item &&
                        vp->screentexpos_old[index] != 0)
                        continue;
                    if (!inherited_source_in_bounds)
                        continue;
                    const int32_t source = inherited_source_x * vp->dim_y + inherited_source_y;
                    if (!visual_moved_between_tiles(visual_layer, layers[layer],
                                                    previous_layers[layer], source, index))
                        continue;
                }
                if (!visual_layer_moves_independently(visual_layer) &&
                    visual_layer != viewport_visual_layer::designation && movement.inherited) {
                    const bool fragment_moved =
                        inherited_source_in_bounds &&
                        visual_moved_between_tiles(
                            visual_layer, layers[layer], previous_layers[layer],
                            inherited_source_x * vp->dim_y + inherited_source_y, index);
                    if (!fragment_moved) {
                        const auto &descriptor = visual_layer_descriptor(visual_layer);
                        bool owns_fragment = false;
                        for (const render_proxyst &anchor : proxies)
                            if (anchor.layer == viewport_visual_layer::center &&
                                anchor.target.x == x + descriptor.anchor_offset.x &&
                                anchor.target.y == y + descriptor.anchor_offset.y &&
                                anchor.source.x - anchor.target.x == movement.source.x - x &&
                                anchor.source.y - anchor.target.y == movement.source.y - y &&
                                anchor.progress == movement.progress)
                                owns_fragment = true;
                        if (!owns_fragment)
                            continue;
                    }
                }

                // Items, vehicles and designations keep their vanilla orientation.
                const auto &mirror_descriptor = visual_layer_descriptor(visual_layer);
                const visual_render_groupst group = visual_render_group(visual_layer);
                const bool mirror_eligible =
                    flip_enabled &&
                    (group == visual_render_groupst::main || group == visual_render_groupst::upper);
                // Facing is read from the anchor tile so every fragment of one creature
                // agrees.
                const bool mirrored =
                    mirror_eligible && animation_manager.get_facing(
                                           vp, x + mirror_descriptor.anchor_offset.x,
                                           y + mirror_descriptor.anchor_offset.y) != native_sprite_facing;
                // The anchor's own layer has no offset, so it flips in place.
                const int32_t mirror_shift =
                    mirrored ? mirrored_tile_x(x, x + mirror_descriptor.anchor_offset.x) - x : 0;
                render_proxyst proxy = {static_cast<viewport_visual_layer>(layer),
                                        movement.source,
                                        df::coord2d(x, y),
                                        texpos,
                                        movement.progress,
                                        nullptr,
                                        mirrored,
                                        mirror_shift,
                                        {}};
                bool blocked = false;
                for (int32_t coverage_x = int32_t(std::floor(std::min(proxy.source.x, float(x))));
                     coverage_x <= int32_t(std::ceil(std::max(proxy.source.x, float(x))));
                     ++coverage_x) {
                    for (int32_t coverage_y =
                             int32_t(std::floor(std::min(proxy.source.y, float(y))));
                         coverage_y <= int32_t(std::ceil(std::max(proxy.source.y, float(y))));
                         ++coverage_y) {
                        if (!inside_clip(vp, coverage_x, coverage_y)) {
                            blocked = true;
                            break;
                        }
                        if (visual_render_group(proxy.layer) == visual_render_groupst::main &&
                            has_fire(vp, coverage_x, coverage_y)) {
                            blocked = true;
                            break;
                        }
                        proxy.coverage.emplace(coverage_x, coverage_y);
                    }
                    if (blocked)
                        break;
                }
                if (blocked)
                    continue;
                if (proxy.mirror_shift != 0) {
                    tile_coveragest mirrored_coverage;
                    for (const auto &tile : proxy.coverage)
                        mirrored_coverage.emplace(tile.x + proxy.mirror_shift, tile.y);
                    for (const auto &tile : mirrored_coverage) {
                        if (!inside_clip(vp, tile.x, tile.y)) {
                            blocked = true;
                            break;
                        }
                        if (visual_render_group(proxy.layer) == visual_render_groupst::main &&
                            has_fire(vp, tile.x, tile.y)) {
                            blocked = true;
                            break;
                        }
                        proxy.coverage.insert(tile);
                    }
                    if (blocked)
                        continue;
                }

                proxy.texture = cached_texture(renderer, texpos);
                if (proxy.texture == nullptr)
                    continue;
                proxies.push_back(std::move(proxy));
            }
        }
    }

    // A creature that has stopped still needs its mirrored sprite painted each
    // frame. Otherwise the engine repaints it natively and the two orientations
    // alternate between steps. A fragment's tile is its anchor minus the layer's
    // centre offset, inverting the moving path.
    if (flip_enabled) {
        for (int32_t anchor_x = 0; anchor_x < vp->dim_x; ++anchor_x) {
            for (int32_t anchor_y = 0; anchor_y < vp->dim_y; ++anchor_y) {
                if (animation_manager.get_facing(vp, anchor_x, anchor_y) == native_sprite_facing)
                    continue;
                for (const auto visual_layer : visual_layer_draw_order) {
                    const visual_render_groupst group = visual_render_group(visual_layer);
                    if (group != visual_render_groupst::main &&
                        group != visual_render_groupst::upper)
                        continue;
                    const auto &descriptor = visual_layer_descriptor(visual_layer);
                    const int32_t x = anchor_x - descriptor.anchor_offset.x;
                    const int32_t y = anchor_y - descriptor.anchor_offset.y;
                    if (x < 0 || x >= vp->dim_x || y < 0 || y >= vp->dim_y)
                        continue;
                    const size_t layer = static_cast<size_t>(visual_layer);
                    const int32_t texpos = layers[layer][x * vp->dim_y + y];
                    if (texpos == 0)
                        continue;
                    bool already_drawn = false;
                    for (const render_proxyst &existing : proxies)
                        if (existing.layer == visual_layer && existing.target.x == x &&
                            existing.target.y == y)
                            already_drawn = true;
                    if (already_drawn)
                        continue;

                    // source == target at progress 1.0 draws in place, moved only by
                    // mirror_shift.
                    render_proxyst proxy = {visual_layer,
                                            {float(x), float(y)},
                                            df::coord2d(x, y),
                                            texpos,
                                            1.0f,
                                            nullptr,
                                            true,
                                            mirrored_tile_x(x, anchor_x) - x,
                                            {}};
                    // The sprite lands on x+mirror_shift, so that interval must be
                    // repaintable. The shift has either sign, so order the interval ends
                    // first.
                    const int32_t coverage_first = std::min(x, x + proxy.mirror_shift);
                    const int32_t coverage_last = std::max(x, x + proxy.mirror_shift);
                    bool blocked = false;
                    for (int32_t coverage_x = coverage_first; coverage_x <= coverage_last;
                         ++coverage_x) {
                        if (!inside_clip(vp, coverage_x, y) ||
                            (group == visual_render_groupst::main && has_fire(vp, coverage_x, y))) {
                            blocked = true;
                            break;
                        }
                        proxy.coverage.emplace(coverage_x, y);
                    }
                    if (blocked)
                        continue;

                    proxy.texture = cached_texture(renderer, texpos);
                    if (proxy.texture == nullptr)
                        continue;
                    proxies.push_back(std::move(proxy));
                }
            }
        }
    }
    return proxies;
}

render_coveragest collect_coverage(const std::vector<render_proxyst> &proxies, int32_t dim_y) {
    render_coveragest coverage;
    for (const render_proxyst &proxy : proxies) {
        coverage.all.insert(proxy.coverage.begin(), proxy.coverage.end());
        coverage.selected[proxy.target.x * dim_y + proxy.target.y] |= visual_layer_bit(proxy.layer);
        auto &group = coverage.groups[static_cast<size_t>(visual_render_group(proxy.layer))];
        group.insert(proxy.coverage.begin(), proxy.coverage.end());
    }
    return coverage;
}

std::vector<df::graphic_viewportst *> active_viewports() {
    std::vector<df::graphic_viewportst *> viewports;
    if (gps == nullptr)
        return viewports;
    for (int32_t lower = 7; lower >= 0; --lower) {
        df::graphic_viewportst *vp = gps->lower_viewport[lower];
        if (viewport_readable(vp))
            viewports.push_back(vp);
    }
    if (viewport_readable(gps->main_viewport))
        viewports.push_back(gps->main_viewport);
    return viewports;
}

std::vector<viewport_renderst>
collect_viewport_renders(df::renderer_2d_base *renderer,
                         const std::vector<df::graphic_viewportst *> &viewports) {
    std::vector<viewport_renderst> renders;
    renders.reserve(viewports.size());
    for (df::graphic_viewportst *vp : viewports) {
        const bool movement_enabled = zlevel_enabled || vp == gps->main_viewport;
        viewport_renderst render = {vp, collect_proxies(renderer, vp, movement_enabled), {}};
        render.coverage = collect_coverage(render.proxies, vp->dim_y);
        renders.push_back(std::move(render));
    }
    return renders;
}

tile_coveragest collect_viewport_coverage(const std::vector<viewport_renderst> &viewports) {
    tile_coveragest coverage;
    for (const viewport_renderst &viewport : viewports)
        coverage.insert(viewport.coverage.all.begin(), viewport.coverage.all.end());
    return coverage;
}

void draw_interpolation_stages(df::renderer_2d_base *renderer, df::graphic_viewportst *vp,
                               const std::vector<render_proxyst> &proxies,
                               const render_coveragest &coverage) {
    for (size_t index = 0; index < coverage.groups.size(); ++index) {
        const auto group = static_cast<visual_render_groupst>(index);
        for (const render_proxyst &proxy : proxies)
            if (visual_render_group(proxy.layer) == group)
                draw_proxy(renderer, proxy);
        if (group == visual_render_groupst::designation)
            continue;
        for (const auto &[x, y] : coverage.groups[index])
            redraw_above(renderer, vp, x, y, group, coverage.selected);
    }
}

void redraw_viewport_tiles(df::renderer_2d_base *renderer, const viewport_renderst &viewport,
                           const tile_coveragest &coverage) {
    df::graphic_viewportst *vp = viewport.viewport;
    for (const auto &[x, y] : coverage) {
        if (!inside_clip(vp, x, y))
            continue;
        redraw_viewport_tile(renderer, viewport, x, y, true);
    }
}

void draw_viewport_interpolation_stages(df::renderer_2d_base *renderer,
                                        const std::vector<viewport_renderst> &viewports,
                                        const tile_coveragest &coverage) {
    for (size_t index = 0; index < viewports.size(); ++index) {
        // A lower z-level's proxy must be covered by the next viewport's fog and
        // terrain. Reapply that viewport before its own proxies, matching DF's
        // lower-to-main draw order.
        if (index > 0)
            redraw_viewport_tiles(renderer, viewports[index], coverage);
        const viewport_renderst &viewport = viewports[index];
        draw_interpolation_stages(renderer, viewport.viewport, viewport.proxies, viewport.coverage);
        // A viewport shades everything drawn beneath it, so this covers every
        // staged tile. Restricting it to the tiles this viewport has sprites on
        // would not deepen with distance.
        for (const auto &[x, y] : coverage) {
            if (inside_clip(viewport.viewport, x, y))
                draw_interface_only(renderer, viewport.viewport, x, y);
        }
    }
}

bool has_mirrored_viewport_facing(const std::vector<df::graphic_viewportst *> &viewports) {
    for (const df::graphic_viewportst *vp : viewports)
        if (animation_manager.has_mirrored_facing(vp))
            return true;
    return false;
}

void render_interpolated_world(df::renderer_2d_base *renderer) {
    df::graphic_viewportst *vp = gps ? gps->main_viewport : nullptr;
    const std::vector<df::graphic_viewportst *> viewports = active_viewports();

    if (vp != nullptr)
        update_visual_context(renderer, vp);
    const uint32_t now_ms = Core::getInstance().p->getTickCount();
    animation_manager.begin_frame(now_ms);
    for (df::graphic_viewportst *viewport : viewports)
        animation_manager.synchronize_viewport(animation_input(viewport));
    animation_manager.end_frame();

    if (!viewport_readable(vp) || renderer->sdl_renderer == nullptr)
        return;
    update_camera(renderer, vp, animation_manager.get_frame_delta_ms());
    const double cam_tile = tile_px(renderer);
    const int32_t glide_x = int32_t(std::lround(transient.x + rest.x * cam_tile));
    const int32_t glide_y = int32_t(std::lround(transient.y + rest.y * cam_tile));
    const bool glide = glide_x != 0 || glide_y != 0;
    if (!glide && camera_was_offset) {
        // The camera just re-joined the grid: one engine redraw replaces the last
        // shifted frame.
        camera_was_offset = false;
        if (gps != nullptr)
            ++gps->force_full_display_count;
    }
    if (glide)
        camera_was_offset = true;
    const bool animation_redraw =
        zlevel_enabled ? animation_manager.requires_full_redraw()
                       : animation_manager.has_active_movement(vp) || !previous_coverage.empty();
    if (!glide && !animation_redraw && (!flip_enabled || !has_mirrored_viewport_facing(viewports)))
        return;

    std::vector<viewport_renderst> viewport_renders = collect_viewport_renders(renderer, viewports);
    tile_coveragest coverage = collect_viewport_coverage(viewport_renders);

    SDL_Renderer *sdl_renderer = static_cast<SDL_Renderer *>(renderer->sdl_renderer);
    const int32_t zoom = renderer->viewport_zoom_factor;
    const int32_t tile_size = zoom == 128 ? 32 : std::max(1, zoom * 32 / 128);

    if (glide) {
        // Camera mid-glide: repaint the WHOLE map rect at the shifted origin so the
        // world (and the creature proxies, which read origin at draw time) renders
        // between tiles. The engine already drew this frame at the snapped
        // position; everything here overdraws it, clipped to the map rect so
        // shifted tiles never spill over the UI. The uncovered strip on the
        // trailing edge stays black until the glide lands.
        const SDL_Rect map_rect = {tile_pixel(vp->clipx[0], renderer->origin_x, zoom),
                                   tile_pixel(vp->clipy[0], renderer->origin_y, zoom),
                                   tile_pixel(vp->clipx[1] + 1, renderer->origin_x, zoom) -
                                       tile_pixel(vp->clipx[0], renderer->origin_x, zoom),
                                   tile_pixel(vp->clipy[1] + 1, renderer->origin_y, zoom) -
                                       tile_pixel(vp->clipy[0], renderer->origin_y, zoom)};
        render_set_clip_rect(sdl_renderer, &map_rect);
        Uint8 old_r = 0, old_g = 0, old_b = 0, old_a = 255;
        get_render_draw_color(sdl_renderer, &old_r, &old_g, &old_b, &old_a);
        set_render_draw_color(sdl_renderer, 0, 0, 0, 255);
        render_fill_rect(sdl_renderer, &map_rect);
        set_render_draw_color(sdl_renderer, old_r, old_g, old_b, old_a);

        const int32_t saved_origin_x = renderer->origin_x;
        const int32_t saved_origin_y = renderer->origin_y;
        renderer->origin_x += glide_x;
        renderer->origin_y += glide_y;
        for (int32_t x = vp->clipx[0]; x <= vp->clipx[1]; ++x) {
            for (int32_t y = vp->clipy[0]; y <= vp->clipy[1]; ++y)
                redraw_world_tile(renderer, viewport_renders, coverage, x, y);
        }
        draw_viewport_interpolation_stages(renderer, viewport_renders, coverage);
        renderer->origin_x = saved_origin_x;
        renderer->origin_y = saved_origin_y;
        render_set_clip_rect(sdl_renderer, nullptr);

        // Everything was repainted; per-tile coverage bookkeeping restarts after
        // the glide.
        previous_coverage.clear();
        return;
    }

    tile_coveragest redraw_coverage = coverage;
    redraw_coverage.insert(previous_coverage.begin(), previous_coverage.end());
    Uint8 old_r = 0, old_g = 0, old_b = 0, old_a = 255;
    get_render_draw_color(sdl_renderer, &old_r, &old_g, &old_b, &old_a);
    set_render_draw_color(sdl_renderer, 0, 0, 0, 255);
    for (const auto &[x, y] : redraw_coverage) {
        if (!inside_clip(vp, x, y))
            continue;
        const SDL_Rect tile_rect = {tile_pixel(x, renderer->origin_x, zoom),
                                    tile_pixel(y, renderer->origin_y, zoom), tile_size, tile_size};
        render_fill_rect(sdl_renderer, &tile_rect);
    }
    set_render_draw_color(sdl_renderer, old_r, old_g, old_b, old_a);

    for (const auto &[x, y] : redraw_coverage) {
        if (inside_clip(vp, x, y))
            redraw_world_tile(renderer, viewport_renders, coverage, x, y);
    }
    draw_viewport_interpolation_stages(renderer, viewport_renders, coverage);

    previous_coverage = std::move(coverage);
}

struct renderer_hook : df::renderer_2d_base {
    typedef df::renderer_2d_base interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, update_all, ());
};

IMPLEMENT_VMETHOD_INTERPOSE(renderer_hook, update_all);

void renderer_hook::interpose_fn_update_all() {
    // update_all is the existing UI stage, so world correction must run first.
    render_interpolated_world(this);
    INTERPOSE_NEXT(update_all)();
}

void clear_sdl_bindings() {
    render_copy_f = nullptr;
    render_copy_ex_f = nullptr;
    render_fill_rect = nullptr;
    render_set_clip_rect = nullptr;
    get_render_draw_color = nullptr;
    set_render_draw_color = nullptr;
}

bool load_sdl(color_ostream &out) {
    clear_sdl_bindings();
    DFLibrary *sdl_handle = DFSDL::obtain_library_handle();
#define bind(name, target)                                                                         \
    target = reinterpret_cast<decltype(target)>(LookupPlugin(sdl_handle, #name));                  \
    if (target == nullptr) {                                                                       \
        out.printerr("smooth-movement: SDL2 function unavailable: " #name "\n");                   \
        clear_sdl_bindings();                                                                      \
        return false;                                                                              \
    }
    bind(SDL_RenderCopyF, render_copy_f);
    bind(SDL_RenderCopyExF, render_copy_ex_f);
    bind(SDL_RenderFillRect, render_fill_rect);
    bind(SDL_RenderSetClipRect, render_set_clip_rect);
    bind(SDL_GetRenderDrawColor, get_render_draw_color);
    bind(SDL_SetRenderDrawColor, set_render_draw_color);
#undef bind
    return true;
}

void reset_state() {
    animation_manager = visual_animation_managerst();
    previous_coverage.clear();
    visual_context_revision = 0;
    previous_viewport = nullptr;
    previous_visual_context = {};
    has_visual_context = false;
    previous_pan.x = 0;
    previous_pan.y = 0;
    has_pan_context = false;
    cancel_camera_transients();
    rest.x = 0.0;
    rest.y = 0.0;
    camera_enabled = false;
    camera_has_prev = false;
    camera_was_offset = false;
    flip_enabled = false;
    zlevel_enabled = false;
}

command_result status_command(color_ostream &out, std::vector<std::string> &parameters) {
    if (parameters.empty()) {
        out.print("smooth-movement: {}\n", is_enabled ? "enabled" : "disabled");
        out.print("free camera: {}, offset {:.3f} {:.3f} (tiles east/south of the "
                  "grid)\n",
                  camera_enabled ? "on" : "off", -rest.x, -rest.y);
        out.print("sprite flipping: {}\n", flip_enabled ? "on" : "off");
        out.print("lower z-level animation: {}\n", zlevel_enabled ? "on" : "off");
        return CR_OK;
    }
    if (parameters[0] == "camera") {
        if (parameters.size() == 1) {
            out.print("free camera: {}, offset {:.3f} {:.3f}\n", camera_enabled ? "on" : "off",
                      -rest.x, -rest.y);
            return CR_OK;
        }
        if (parameters.size() == 2 && parameters[1] == "on") {
            set_camera_enabled(true);
            return CR_OK;
        }
        if (parameters.size() == 2 && parameters[1] == "off") {
            set_camera_enabled(false);
            return CR_OK;
        }
        if (parameters.size() == 2 && parameters[1] == "reset") {
            rest.x = 0.0;
            rest.y = 0.0;
            return CR_OK;
        }
        if (parameters.size() == 3) {
            try {
                const double fx = std::stod(parameters[1]);
                const double fy = std::stod(parameters[2]);
                if (fx < -0.99 || fx > 0.99 || fy < -0.99 || fy > 0.99) {
                    out.printerr("offsets must be within -0.99..0.99 tiles\n");
                    return CR_FAILURE;
                }
                // User-facing: positive = view sits east/south of the grid position.
                set_camera_enabled(true);
                rest.x = -fx;
                rest.y = -fy;
                normalize_rest();
                return CR_OK;
            } catch (...) {
                return CR_WRONG_USAGE;
            }
        }
        return CR_WRONG_USAGE;
    }
    if (parameters[0] == "flip") {
        if (parameters.size() == 1) {
            out.print("sprite flipping: {}\n", flip_enabled ? "on" : "off");
            return CR_OK;
        }
        // A toggle changes the screen without changing anything DF knows, so DF
        // will not repaint. OFF matters most: the render path stops touching tiles
        // it painted every frame. The last mirrored frame would persist. Same flush
        // plugin_enable(false) uses.
        if (parameters.size() == 2 && parameters[1] == "on") {
            flip_enabled = true;
            if (gps != nullptr)
                ++gps->force_full_display_count;
            out.print("smooth-movement: sprite flipping enabled\n");
            return CR_OK;
        }
        if (parameters.size() == 2 && parameters[1] == "off") {
            flip_enabled = false;
            if (gps != nullptr)
                ++gps->force_full_display_count;
            out.print("smooth-movement: sprite flipping disabled\n");
            return CR_OK;
        }
        return CR_WRONG_USAGE;
    }
    if (parameters[0] == "zlevel") {
        if (parameters.size() == 1) {
            out.print("lower z-level animation: {}\n", zlevel_enabled ? "on" : "off");
            return CR_OK;
        }
        if (parameters.size() == 2 && parameters[1] == "on")
            zlevel_enabled = true;
        else if (parameters.size() == 2 && parameters[1] == "off")
            zlevel_enabled = false;
        else
            return CR_WRONG_USAGE;
        if (gps != nullptr)
            ++gps->force_full_display_count;
        out.print("smooth-movement: lower z-level animation {}\n",
                  zlevel_enabled ? "enabled" : "disabled");
        return CR_OK;
    }
    return CR_WRONG_USAGE;
}

} // namespace

DFhackCExport command_result plugin_init(color_ostream &, std::vector<PluginCommand> &commands) {
    commands.emplace_back("smooth-movement", "Smoothly animate movement in the fortress viewport.",
                          status_command);
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (is_enabled == enable)
        return CR_OK;
    if (enable) {
        reset_state();
        if (!load_sdl(out))
            return CR_FAILURE;
        if (!INTERPOSE_HOOK(renderer_hook, update_all).apply()) {
            out.printerr("smooth-movement: could not hook the 2D renderer\n");
            clear_sdl_bindings();
            return CR_FAILURE;
        }
    } else {
        INTERPOSE_HOOK(renderer_hook, update_all).remove();
        reset_state();
        clear_sdl_bindings();
        if (gps != nullptr)
            ++gps->force_full_display_count;
    }
    is_enabled = enable;
    out.print("smooth-movement: {}\n", enable ? "enabled" : "disabled");
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}
