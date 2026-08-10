// SPDX-License-Identifier: MIT

#ifndef SMOOTH_MOVEMENT_VISUAL_ANIMATION_H
#define SMOOTH_MOVEMENT_VISUAL_ANIMATION_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

enum class viewport_visual_layer : uint8_t {
    right,
    center,
    left,
    upright,
    up,
    upleft,
    vehicle,
    item,
    designation,
    count
};

enum class visual_render_groupst : uint8_t { item, vehicle, main, upper, designation, count };

struct visual_layer_descriptorst {
    viewport_visual_layer layer;
    visual_render_groupst render_group;
    bool moves_independently;
    bool matches_any_previous;
    uint8_t draw_order;
    int8_t center_x;
    int8_t center_y;
};

constexpr std::array visual_layer_descriptors = {
    visual_layer_descriptorst{viewport_visual_layer::right, visual_render_groupst::main, false,
                              false, 3, -1, 0},
    visual_layer_descriptorst{viewport_visual_layer::center, visual_render_groupst::main, true,
                              false, 0, 0, 0},
    visual_layer_descriptorst{viewport_visual_layer::left, visual_render_groupst::main, false,
                              false, 4, 1, 0},
    visual_layer_descriptorst{viewport_visual_layer::upright, visual_render_groupst::upper, false,
                              false, 5, -1, 1},
    visual_layer_descriptorst{viewport_visual_layer::up, visual_render_groupst::upper, false, false,
                              6, 0, 1},
    visual_layer_descriptorst{viewport_visual_layer::upleft, visual_render_groupst::upper, false,
                              false, 7, 1, 1},
    visual_layer_descriptorst{viewport_visual_layer::vehicle, visual_render_groupst::vehicle, true,
                              true, 2, 0, 0},
    visual_layer_descriptorst{viewport_visual_layer::item, visual_render_groupst::item, true, false,
                              1, 0, 0},
    visual_layer_descriptorst{viewport_visual_layer::designation,
                              visual_render_groupst::designation, false, true, 8, 0, 0}};

constexpr bool valid_visual_layer_descriptors() {
    uint16_t draw_orders = 0;
    for (size_t i = 0; i < visual_layer_descriptors.size(); ++i) {
        const auto &descriptor = visual_layer_descriptors[i];
        if (static_cast<size_t>(descriptor.layer) != i ||
            descriptor.draw_order >= visual_layer_descriptors.size() ||
            (draw_orders & (1U << descriptor.draw_order)))
            return false;
        draw_orders |= uint16_t(1U << descriptor.draw_order);
    }
    return true;
}

static_assert(valid_visual_layer_descriptors());

constexpr const visual_layer_descriptorst &visual_layer_descriptor(viewport_visual_layer layer) {
    return visual_layer_descriptors[static_cast<size_t>(layer)];
}

constexpr viewport_visual_layer visual_layer_at_draw_order(uint8_t draw_order) {
    for (const auto &descriptor : visual_layer_descriptors)
        if (descriptor.draw_order == draw_order)
            return descriptor.layer;
    return viewport_visual_layer::count;
}

constexpr visual_render_groupst visual_render_group(viewport_visual_layer layer) {
    return visual_layer_descriptor(layer).render_group;
}

constexpr bool visual_layer_moves_independently(viewport_visual_layer layer) {
    return visual_layer_descriptor(layer).moves_independently;
}

constexpr bool visual_layer_tracks_own_movement(viewport_visual_layer layer) {
    const auto &descriptor = visual_layer_descriptor(layer);
    return descriptor.moves_independently ||
           descriptor.render_group == visual_render_groupst::designation;
}

constexpr bool visual_layer_matches(viewport_visual_layer layer, int32_t current,
                                    int32_t previous) {
    return visual_layer_descriptor(layer).matches_any_previous ? previous != 0
                                                               : previous == current;
}

struct viewport_visual_animation_inputst {
    const void *viewport = nullptr;
    int32_t dim_x = 0;
    int32_t dim_y = 0;
    uint64_t context_revision = 0;
    std::array<const int32_t *, static_cast<size_t>(viewport_visual_layer::count)> current{};
    std::array<const int32_t *, static_cast<size_t>(viewport_visual_layer::count)> previous{};
    // Current map-scroll offset (window_x/window_y). A pure pan does not bump
    // context_revision. Only a hint: it changes at input time, the buffers shift
    // on a later render frame.
    int32_t pan_x = 0;
    int32_t pan_y = 0;

    bool valid() const {
        if (viewport == nullptr || dim_x <= 0 || dim_y <= 0)
            return false;
        for (size_t layer = 0; layer < current.size(); ++layer) {
            if (current[layer] == nullptr || previous[layer] == nullptr)
                return false;
        }
        return true;
    }
};

struct visual_movement_renderst {
    bool active = false;
    float source_x = 0.0f;
    float source_y = 0.0f;
    float progress = 1.0f;
    bool inherited = false;
};

inline float animation_progress(uint32_t now_ms, uint32_t start_time_ms, uint32_t duration_ms) {
    const float linear = std::min(1.0f, float(now_ms - start_time_ms) / duration_ms);
    return linear * linear * (3.0f - 2.0f * linear);
}

inline bool visual_moved_between_tiles(viewport_visual_layer layer, const int32_t *current,
                                       const int32_t *previous, int32_t source, int32_t target) {
    return previous[target] == 0 && current[source] == 0 &&
           (layer == viewport_visual_layer::designation || previous[source] != 0);
}

inline int32_t inherited_visual_source_tile(int32_t overlay_target, float center_source,
                                            float center_target) {
    return overlay_target + int32_t(std::lround(center_source - center_target));
}

enum class visual_facingst : int8_t { east = 0, west = 1 };

// DF creature art faces west, so only east needs flipping. Also the default and
// cleared value.
constexpr visual_facingst native_sprite_facing = visual_facingst::west;

// Sticky facing: only a horizontal component changes it.
constexpr visual_facingst facing_after_move(int32_t dx, visual_facingst previous) {
    if (dx > 0)
        return visual_facingst::east;
    if (dx < 0)
        return visual_facingst::west;
    return previous;
}

// center_x is only -1, 0 or +1, so a creature is at most three columns wide
// here.
constexpr int32_t mirrored_tile_x(int32_t piece_x, int32_t anchor_x) {
    return anchor_x - (piece_x - anchor_x);
}

class visual_animation_managerst {
    struct movementst {
        viewport_visual_layer layer;
        int32_t texpos;
        float source_x;
        float source_y;
        int32_t target_x;
        int32_t target_y;
        uint32_t start_time_ms;
    };

    struct viewport_animationst {
        const void *viewport = nullptr;
        int32_t dim_x = 0;
        int32_t dim_y = 0;
        uint64_t context_revision = 0;
        bool has_context = false;
        bool seen = false;
        std::vector<movementst> movements;
        // One facing per tile, not per unit: the viewport exposes one creature
        // texpos per tile.
        std::vector<int8_t> facing;
        // Stationary mirrored creatures are repainted every frame; this is the
        // cheap pre-check.
        bool has_mirrored = false;
        int32_t pan_x = 0;
        int32_t pan_y = 0;
        bool has_pan = false;
        // Window scrolls not yet observed in the buffers, oldest first.
        // A signed total would cancel on a reversing drag while both shifts are
        // still owed.
        std::vector<std::array<int32_t, 2>> pending;
        // Redraws no prefix has matched.
        int32_t pending_frames = 0;
        // Redraws spent waiting for the buffers to move at all.
        int32_t pending_age = 0;
        // Redraws left in which new-movement detection stays suppressed after
        // scroll activity.
        int32_t suppress_frames = 0;
        // Buffer contents last seen, to recognize a repeat of them.
        uint64_t buffer_signature = 0;
        bool has_buffer_signature = false;
        // Set while the previous buffer still belongs to a view that has been left
        // behind.
        bool previous_view_stale = false;
    };

    uint32_t frame_time_ms = 0;
    uint32_t frame_delta_ms = 0;
    bool has_frame = false;
    bool force_full_redraw = false;
    std::vector<viewport_animationst> viewports;

    static constexpr uint32_t movement_duration_ms = 100;
    // Scrolling faster than detection keeps up: give up rather than test ever
    // more prefixes.
    static constexpr size_t max_pending_shifts = 8;
    // Bounds the wait on a scroll that never lands, so suppression cannot stick
    // forever.
    static constexpr int32_t max_pending_age_frames = 120;

    static void clear_pending(viewport_animationst &state);

    static void abandon_pending(viewport_animationst &state);

    static void reset_facing(viewport_animationst &state);

    static void reset_tracking(viewport_animationst &state);

    // Identifies the buffer contents this frame, to tell a redrawn viewport from
    // a repeated one.
    static uint64_t compute_buffer_signature(const viewport_visual_animation_inputst &input);

    // Fraction of tracked sprites consistent with a buffer shift:
    // current[x]==previous[x+dwx]. Negative when there is nothing to compare.
    static double shift_match_ratio(const viewport_visual_animation_inputst &input, int32_t dwx,
                                    int32_t dwy);

    static std::array<int32_t, 2> shared_movement_delta(const int32_t *current,
                                                        const int32_t *previous, int32_t dim_x,
                                                        int32_t dim_y);

    viewport_animationst &get_viewport(const viewport_visual_animation_inputst &input);

    float movement_progress(uint32_t start_time_ms) const;

  public:
    visual_animation_managerst() = default;

    void begin_frame(uint32_t now_ms);

    void synchronize_viewport(const viewport_visual_animation_inputst &input);

    void end_frame();

    uint32_t get_frame_time_ms() const;

    uint32_t get_frame_delta_ms() const;

    visual_facingst get_facing(const void *viewport, int32_t x, int32_t y) const;

    bool has_mirrored_facing(const void *viewport) const;

    bool requires_full_redraw() const;

    bool has_active_movement(const void *viewport) const;

    visual_movement_renderst get_movement(const void *viewport, viewport_visual_layer layer,
                                          int32_t target_x, int32_t target_y) const;
};

#endif // SMOOTH_MOVEMENT_VISUAL_ANIMATION_H
