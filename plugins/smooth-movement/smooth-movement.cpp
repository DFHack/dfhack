// SPDX-License-Identifier: Zlib

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
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
#include "df/global_objects.h"
#include "df/graphic.h"
#include "df/graphic_viewportst.h"
#include "df/renderer_2d_base.h"
#include "df/texture_fullid.h"
#include "df/viewport_spatter_flag.h"

#include "visual_animation.h"

using namespace DFHack;

DFHACK_PLUGIN("smooth-movement");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

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

// DF's native renderer draws 32-pixel tiles at viewport zoom 128.
constexpr int32_t native_tile_pixels = 32;
constexpr int32_t native_viewport_zoom = 128;

void update_visual_context(const df::renderer_2d_base *renderer, const df::graphic_viewportst *vp) {
    // window_x/window_y are deliberately excluded: a horizontal/vertical scroll
    // is followed, not reset. window_z (z-level) stays, since a z change is not
    // followable.
    const visual_contextst context = {
        .z_level = window_z ? *window_z : 0,
        .viewport_dimensions = df::coord2d(vp->dim_x, vp->dim_y),
        // DF clip arrays hold inclusive minimum and maximum coordinates.
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
    }
    previous_viewport = vp;
    previous_visual_context = context;
    has_visual_context = true;

    // On a pure pan the reset signature is unchanged, but last frame's blackout
    // coverage is in the old viewport frame, so discard it (the engine repaints
    // the whole scrolled viewport anyway).
    const df::coord2d pan(window_x ? *window_x : 0, window_y ? *window_y : 0);
    if (!has_pan_context || previous_pan != pan)
        previous_coverage.clear();
    previous_pan = pan;
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

int32_t tile_size(int32_t zoom) {
    return std::max(1, zoom * native_tile_pixels / native_viewport_zoom);
}

int32_t tile_pixel(int32_t tile, int32_t origin, int32_t zoom) {
    return tile_size(zoom) * tile + origin;
}

bool inside_clip(const df::graphic_viewportst *vp, df::coord2d pos) {
    return pos.x >= vp->clipx[0] && pos.x <= vp->clipx[1] && pos.y >= vp->clipy[0] &&
           pos.y <= vp->clipy[1];
}

bool inside_clip(const df::graphic_viewportst *vp, int32_t x, int32_t y) {
    return inside_clip(vp, df::coord2d(x, y));
}

bool has_fire(const df::graphic_viewportst *vp, int32_t x, int32_t y) {
    return vp->screentexpos_spatter_flag != nullptr &&
           vp->screentexpos_spatter_flag[x * vp->dim_y + y].bits.fire_frame_type != 0;
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
    coord2dst<float> source;
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
void draw_interface_only(df::renderer_2d_base *renderer, df::graphic_viewportst *vp,
                         df::coord2d pos) {
    if (!interface_pass_readable(vp))
        return;
    const int32_t index = pos.x * vp->dim_y + pos.y;
    const auto redraw = [&] { renderer->update_viewport_tile(vp, pos.x, pos.y); };
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
    const coord2dst<int32_t> target{
        tile_pixel(proxy.target.x, renderer->origin_x, zoom),
        tile_pixel(proxy.target.y, renderer->origin_y, zoom)};
    const float tile_size = float(::tile_size(zoom));
    const float source_x = target.x + (proxy.source.x - proxy.target.x) * tile_size;
    const float source_y = target.y + (proxy.source.y - proxy.target.y) * tile_size;
    const float mirror_offset = float(proxy.mirror_shift) * tile_size;
    const SDL_FRect destination = {
        source_x + (target.x - source_x) * proxy.progress + mirror_offset,
        source_y + (target.y - source_y) * proxy.progress, tile_size, tile_size};
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
                if (texpos == 0 || !movement_enabled)
                    continue;
                const auto movement = animation_manager.get_movement(
                    vp, static_cast<viewport_visual_layer>(layer), x, y);
                if (!movement.active)
                    continue;
                const coord2dst<float> target{float(x), float(y)};
                const auto movement_delta = movement.source - target;
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
                        // Creature fragments lie no more than one tile from their center layer.
                        if (anchor.layer == viewport_visual_layer::center &&
                            std::abs(anchor.target.x - x) <= 1 &&
                            std::abs(anchor.target.y - y) <= 1 &&
                            anchor.source -
                                    coord2dst<float>{float(anchor.target.x), float(anchor.target.y)} ==
                                movement_delta &&
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
                                anchor.source -
                                        coord2dst<float>{float(anchor.target.x), float(anchor.target.y)} ==
                                    movement_delta &&
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
                const df::coord2d anchor(x + mirror_descriptor.anchor_offset.x,
                                         y + mirror_descriptor.anchor_offset.y);
                const bool mirrored = mirror_eligible &&
                                      animation_manager.get_facing(vp, anchor) != native_sprite_facing;
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
                const int32_t coverage_min_x = int32_t(std::floor(std::min(proxy.source.x, float(x))));
                const int32_t coverage_max_x = int32_t(std::ceil(std::max(proxy.source.x, float(x))));
                const int32_t coverage_min_y = int32_t(std::floor(std::min(proxy.source.y, float(y))));
                const int32_t coverage_max_y = int32_t(std::ceil(std::max(proxy.source.y, float(y))));
                for (int32_t coverage_x = coverage_min_x; coverage_x <= coverage_max_x;
                     ++coverage_x) {
                    for (int32_t coverage_y = coverage_min_y; coverage_y <= coverage_max_y;
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
                const df::coord2d anchor(anchor_x, anchor_y);
                if (animation_manager.get_facing(vp, anchor) == native_sprite_facing)
                    continue;
                for (const auto visual_layer : visual_layer_draw_order) {
                    const visual_render_groupst group = visual_render_group(visual_layer);
                    if (group != visual_render_groupst::main &&
                        group != visual_render_groupst::upper)
                        continue;
                    const auto &descriptor = visual_layer_descriptor(visual_layer);
                    const int32_t x = anchor.x - descriptor.anchor_offset.x;
                    const int32_t y = anchor.y - descriptor.anchor_offset.y;
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
                                            mirrored_tile_x(x, anchor.x) - x,
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
        // TODO: use std::mdspan when C++23 available
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
    for (size_t lower = std::size(gps->lower_viewport); lower-- > 0;) {
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
    for (const df::coord2d &tile : coverage) {
        if (!inside_clip(vp, tile))
            continue;
        redraw_viewport_tile(renderer, viewport, tile.x, tile.y, true);
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
        for (const df::coord2d &tile : coverage) {
            if (inside_clip(viewport.viewport, tile))
                draw_interface_only(renderer, viewport.viewport, tile);
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
    const bool animation_redraw =
        zlevel_enabled ? animation_manager.requires_full_redraw()
                       : animation_manager.has_active_movement(vp) || !previous_coverage.empty();
    if (!animation_redraw && (!flip_enabled || !has_mirrored_viewport_facing(viewports)))
        return;

    std::vector<viewport_renderst> viewport_renders = collect_viewport_renders(renderer, viewports);
    tile_coveragest coverage = collect_viewport_coverage(viewport_renders);

    SDL_Renderer *sdl_renderer = static_cast<SDL_Renderer *>(renderer->sdl_renderer);
    const int32_t zoom = renderer->viewport_zoom_factor;
    const int32_t tile_size = ::tile_size(zoom);

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
    previous_pan = {};
    has_pan_context = false;
    flip_enabled = false;
    zlevel_enabled = false;
}

command_result status_command(color_ostream &out, std::vector<std::string> &parameters) {
    if (parameters.empty()) {
        out.print("smooth-movement: {}\n", is_enabled ? "enabled" : "disabled");
        out.print("sprite flipping: {}\n", flip_enabled ? "on" : "off");
        out.print("lower z-level animation: {}\n", zlevel_enabled ? "on" : "off");
        return CR_OK;
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
