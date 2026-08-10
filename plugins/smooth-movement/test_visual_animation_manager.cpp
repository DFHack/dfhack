// SPDX-License-Identifier: MIT

#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>

#include "visual_animation.h"

namespace {

viewport_visual_animation_inputst make_input(const void *viewport, int32_t dimension,
                                             const int32_t *empty) {
    viewport_visual_animation_inputst input;
    input.viewport = viewport;
    input.dim_x = dimension;
    input.dim_y = dimension;
    input.context_revision = 1;
    input.current.fill(empty);
    input.previous.fill(empty);
    return input;
}

void set_layer(viewport_visual_animation_inputst &input, viewport_visual_layer layer,
               const int32_t *current, const int32_t *previous) {
    const size_t index = static_cast<size_t>(layer);
    input.current[index] = current;
    input.previous[index] = previous;
}

void run_frame(visual_animation_managerst &manager, const viewport_visual_animation_inputst &input,
               uint32_t now_ms) {
    manager.begin_frame(now_ms);
    manager.synchronize_viewport(input);
    manager.end_frame();
}

} // namespace

int main() {
    visual_animation_managerst manager;
    manager.begin_frame(1000);
    assert(manager.get_frame_time_ms() == 1000);
    assert(manager.get_frame_delta_ms() == 0);

    manager.begin_frame(1020);
    assert(manager.get_frame_time_ms() == 1020);
    assert(manager.get_frame_delta_ms() == 20);

    manager.begin_frame(1020);
    assert(manager.get_frame_delta_ms() == 0);

    visual_animation_managerst rollover;
    rollover.begin_frame(std::numeric_limits<uint32_t>::max() - 5);
    rollover.begin_frame(3);
    assert(rollover.get_frame_delta_ms() == 9);
    // Facing rule: only a horizontal component changes facing.
    assert(facing_after_move(1, visual_facingst::west) == visual_facingst::east);
    assert(facing_after_move(-1, visual_facingst::east) == visual_facingst::west);
    // dy is not an input, so a diagonal is only ever the sign of dx.
    // The four real diagonals run end to end through the manager further down.
    assert(facing_after_move(1, visual_facingst::east) == visual_facingst::east);
    assert(facing_after_move(-1, visual_facingst::west) == visual_facingst::west);
    // Pure vertical and idle carry the previous facing (sticky).
    assert(facing_after_move(0, visual_facingst::west) == visual_facingst::west);
    assert(facing_after_move(0, visual_facingst::east) == visual_facingst::east);

    assert(mirrored_tile_x(5, 5) == 5); // anchor reflects to itself
    assert(mirrored_tile_x(6, 5) == 4); // right spill -> left
    assert(mirrored_tile_x(4, 5) == 6); // left spill -> right
    // The formula is a general reflection, so it holds for offsets no layer can express.
    assert(mirrored_tile_x(8, 5) == 2);
    // center_x is only ever -1, 0 or +1, so the real mirror shift is only ever -2, 0 or +2.
    for (const auto &descriptor : visual_layer_descriptors)
        assert(descriptor.center_x >= -1 && descriptor.center_x <= 1);
    // Reflection is self-inverse.
    assert(mirrored_tile_x(mirrored_tile_x(6, 5), 5) == 6);
    assert(mirrored_tile_x(mirrored_tile_x(8, 5), 5) == 8);

    {
        constexpr int32_t dim = 4;
        int32_t empty[dim * dim] = {};
        int32_t before[dim * dim] = {};
        int32_t west_after[dim * dim] = {};
        const int viewport_token = 0;
        const void *viewport = &viewport_token;

        before[2 * dim + 2] = 77;
        west_after[1 * dim + 2] = 77; // moved west: x 2 -> 1

        // Moving west sets west facing on the target tile.
        {
            visual_animation_managerst manager;
            auto input = make_input(viewport, dim, empty);
            set_layer(input, viewport_visual_layer::center, before, empty);
            run_frame(manager, input, 1000);
            assert(!manager.has_active_movement(viewport));
            set_layer(input, viewport_visual_layer::center, west_after, before);
            run_frame(manager, input, 1016);
            assert(manager.has_active_movement(viewport));
            assert(manager.get_facing(viewport, 1, 2) == visual_facingst::west);
        }

        // Moving east sets east facing.
        // East is neither the grid default nor the source facing, so the assertion is not vacuous.
        {
            visual_animation_managerst manager;
            auto input = make_input(viewport, dim, empty);
            set_layer(input, viewport_visual_layer::center, before, empty);
            run_frame(manager, input, 1000);
            set_layer(input, viewport_visual_layer::center, west_after, before);
            run_frame(manager, input, 1016);
            assert(manager.get_facing(viewport, 1, 2) == visual_facingst::west);
            set_layer(input, viewport_visual_layer::center, before, west_after);
            run_frame(manager, input, 1032);
            assert(manager.get_facing(viewport, 2, 2) == visual_facingst::east);
        }

        // The mirrored flag gates the render path's early return.
        // It must rise only for a genuinely mirrored creature and fall when that tile empties.
        {
            visual_animation_managerst manager;
            auto input = make_input(viewport, dim, empty);
            set_layer(input, viewport_visual_layer::center, before, empty);
            run_frame(manager, input, 1000);
            assert(!manager.has_mirrored_facing(viewport));
            // Moving west matches the art, so nothing is mirrored yet.
            set_layer(input, viewport_visual_layer::center, west_after, before);
            run_frame(manager, input, 1016);
            assert(manager.get_facing(viewport, 1, 2) == visual_facingst::west);
            assert(!manager.has_mirrored_facing(viewport));
            // Moving east faces away from the art and raises the flag.
            set_layer(input, viewport_visual_layer::center, before, west_after);
            run_frame(manager, input, 1032);
            assert(manager.get_facing(viewport, 2, 2) == visual_facingst::east);
            assert(manager.has_mirrored_facing(viewport));
            // The creature leaves: the tile clears and so does the flag.
            set_layer(input, viewport_visual_layer::center, empty, before);
            run_frame(manager, input, 1048);
            assert(manager.get_facing(viewport, 2, 2) == native_sprite_facing);
            assert(!manager.has_mirrored_facing(viewport));
            assert(!manager.has_mirrored_facing(nullptr));
        }

        // Pure vertical movement carries the existing facing to the new tile.
        {
            visual_animation_managerst manager;
            auto input = make_input(viewport, dim, empty);
            set_layer(input, viewport_visual_layer::center, before, empty);
            run_frame(manager, input, 1000);
            set_layer(input, viewport_visual_layer::center, west_after, before);
            run_frame(manager, input, 1016);
            assert(manager.get_facing(viewport, 1, 2) == visual_facingst::west);
            int32_t up[dim * dim] = {};
            up[1 * dim + 1] = 77; // north: (1,2) -> (1,1), no horizontal component
            set_layer(input, viewport_visual_layer::center, up, west_after);
            run_frame(manager, input, 1032);
            assert(manager.get_facing(viewport, 1, 1) == visual_facingst::west);
        }

        // Out-of-range and unknown viewports fall back to the native facing.
        {
            visual_animation_managerst manager;
            auto input = make_input(viewport, dim, empty);
            set_layer(input, viewport_visual_layer::center, before, empty);
            run_frame(manager, input, 1000);
            assert(manager.get_facing(viewport, -1, 0) == native_sprite_facing);
            assert(manager.get_facing(viewport, dim, 0) == native_sprite_facing);
            assert(manager.get_facing(nullptr, 0, 0) == native_sprite_facing);
        }

        // Each rendered z-level has its own viewport buffers; tracking one must not suppress
        // another.
        {
            const int lower_token = 0;
            const int main_token = 0;
            const void *lower_viewport = &lower_token;
            const void *main_viewport = &main_token;
            visual_animation_managerst z_levels;
            auto lower_input = make_input(lower_viewport, dim, empty);
            auto main_input = make_input(main_viewport, dim, empty);
            set_layer(lower_input, viewport_visual_layer::center, before, empty);
            set_layer(main_input, viewport_visual_layer::center, before, empty);
            z_levels.begin_frame(1000);
            z_levels.synchronize_viewport(lower_input);
            z_levels.synchronize_viewport(main_input);
            z_levels.end_frame();
            set_layer(lower_input, viewport_visual_layer::center, west_after, before);
            set_layer(main_input, viewport_visual_layer::center, west_after, before);
            z_levels.begin_frame(1016);
            z_levels.synchronize_viewport(lower_input);
            z_levels.synchronize_viewport(main_input);
            z_levels.end_frame();
            assert(
                z_levels.get_movement(lower_viewport, viewport_visual_layer::center, 1, 2).active);
            assert(
                z_levels.get_movement(main_viewport, viewport_visual_layer::center, 1, 2).active);
        }
    }

    // All four diagonals through the manager, so every step carries a real dy as well as a dx.
    // The chain alternates direction, so no assertion can pass by inheriting the previous facing.
    {
        constexpr int32_t diag_dim = 5;
        int32_t diag_empty[diag_dim * diag_dim] = {};
        int32_t start[diag_dim * diag_dim] = {};
        int32_t north_east[diag_dim * diag_dim] = {};
        int32_t south_west[diag_dim * diag_dim] = {};
        int32_t south_east[diag_dim * diag_dim] = {};
        int32_t north_west[diag_dim * diag_dim] = {};
        const int diag_token = 0;
        const void *diag_viewport = &diag_token;

        start[2 * diag_dim + 2] = 77;      // (2,2)
        north_east[3 * diag_dim + 1] = 77; // (2,2) -> (3,1): dx +1, dy -1
        south_west[2 * diag_dim + 2] = 77; // (3,1) -> (2,2): dx -1, dy +1
        south_east[3 * diag_dim + 3] = 77; // (2,2) -> (3,3): dx +1, dy +1
        north_west[2 * diag_dim + 2] = 77; // (3,3) -> (2,2): dx -1, dy -1

        visual_animation_managerst diagonal;
        auto input = make_input(diag_viewport, diag_dim, diag_empty);
        set_layer(input, viewport_visual_layer::center, start, diag_empty);
        run_frame(diagonal, input, 1000);
        assert(diagonal.get_facing(diag_viewport, 2, 2) == native_sprite_facing);

        set_layer(input, viewport_visual_layer::center, north_east, start);
        run_frame(diagonal, input, 1016);
        assert(diagonal.get_facing(diag_viewport, 3, 1) == visual_facingst::east);

        // West is the grid default, so the westward legs also assert a movement was registered.
        // Otherwise an untracked step leaving the tile at its default would pass.
        set_layer(input, viewport_visual_layer::center, south_west, north_east);
        run_frame(diagonal, input, 1032);
        assert(diagonal.get_movement(diag_viewport, viewport_visual_layer::center, 2, 2).active);
        assert(diagonal.get_facing(diag_viewport, 2, 2) == visual_facingst::west);

        set_layer(input, viewport_visual_layer::center, south_east, south_west);
        run_frame(diagonal, input, 1048);
        assert(diagonal.get_facing(diag_viewport, 3, 3) == visual_facingst::east);

        set_layer(input, viewport_visual_layer::center, north_west, south_east);
        run_frame(diagonal, input, 1064);
        assert(diagonal.get_movement(diag_viewport, viewport_visual_layer::center, 2, 2).active);
        assert(diagonal.get_facing(diag_viewport, 2, 2) == visual_facingst::west);
    }

    // Facing is keyed by SCREEN tile, so a scroll moves the creatures out from under it.
    // The two outcomes differ fundamentally: a landed shift has a known delta and is translated.
    // An abandoned shift never identifies a delta, so the grid can only be dropped.
    {
        constexpr int32_t pan_dim = 4;
        int32_t pan_empty[pan_dim * pan_dim] = {};
        int32_t at_one[pan_dim * pan_dim] = {};
        int32_t at_two[pan_dim * pan_dim] = {};
        int32_t unmatched_a[pan_dim * pan_dim] = {};
        int32_t unmatched_b[pan_dim * pan_dim] = {};
        const int pan_token = 0;
        const void *pan_viewport = &pan_token;

        at_one[1 * pan_dim + 1] = 77;
        at_two[2 * pan_dim + 1] = 77; // steps east, x 1 -> 2, so it faces east
        unmatched_a[2 * pan_dim + 1] = 78;
        unmatched_b[2 * pan_dim + 1] = 79;

        // LANDED: the shift is recognized, so facing follows the buffers.
        {
            visual_animation_managerst landed;
            auto input = make_input(pan_viewport, pan_dim, pan_empty);
            set_layer(input, viewport_visual_layer::center, at_one, pan_empty);
            run_frame(landed, input, 1000);
            set_layer(input, viewport_visual_layer::center, at_two, at_one);
            run_frame(landed, input, 1016);
            assert(landed.get_facing(pan_viewport, 2, 1) == visual_facingst::east);
            assert(landed.has_mirrored_facing(pan_viewport));

            // Frame A: the scroll is announced but the buffers have not moved yet.
            input.pan_x = 1;
            set_layer(input, viewport_visual_layer::center, at_two, at_two);
            run_frame(landed, input, 1032);
            assert(landed.get_facing(pan_viewport, 2, 1) == visual_facingst::east);

            // Frame B: the buffers shift east by one and the majority-match test recognizes it.
            set_layer(input, viewport_visual_layer::center, at_one, at_two);
            run_frame(landed, input, 1048);
            assert(landed.get_facing(pan_viewport, 1, 1) == visual_facingst::east);
            assert(landed.get_facing(pan_viewport, 2, 1) == native_sprite_facing);
            assert(landed.has_mirrored_facing(pan_viewport));
        }

        // ABANDONED: the shift never shows up in the buffers, so no delta is ever identified.
        // The grid must be back at the default while the tile is still OCCUPIED.
        // The empty-tile sweep cannot reach that case, so only an explicit reset clears it.
        {
            visual_animation_managerst abandoned;
            auto input = make_input(pan_viewport, pan_dim, pan_empty);
            set_layer(input, viewport_visual_layer::center, at_one, pan_empty);
            run_frame(abandoned, input, 2000);
            set_layer(input, viewport_visual_layer::center, at_two, at_one);
            run_frame(abandoned, input, 2016);
            assert(abandoned.get_facing(pan_viewport, 2, 1) == visual_facingst::east);

            // Changed buffers keep the failed majority-match test running every frame.
            // It tolerates four before giving up on the fifth.
            input.pan_x = 1;
            for (int32_t frame = 0; frame < 4; ++frame) {
                set_layer(input, viewport_visual_layer::center, at_two,
                          frame % 2 == 0 ? unmatched_a : unmatched_b);
                run_frame(abandoned, input, 2032 + uint32_t(frame) * 16);
                // Still pending, so the facing survives.
                // The assertion after the giving-up frame therefore tests the reset, not an empty
                // grid.
                assert(abandoned.get_facing(pan_viewport, 2, 1) == visual_facingst::east);
                assert(abandoned.has_mirrored_facing(pan_viewport));
            }
            set_layer(input, viewport_visual_layer::center, at_two, unmatched_a);
            run_frame(abandoned, input, 2096);
            assert(abandoned.get_facing(pan_viewport, 2, 1) == native_sprite_facing);
            assert(!abandoned.has_mirrored_facing(pan_viewport));
        }
    }

    // Regression: A and B move in the same frame, chained -- A's target tile is B's source tile.
    // A's write must not corrupt B's read of its own pre-frame facing.
    // A is sent east first, so its facing differs from the default B carries and a swap shows.
    {
        constexpr int32_t chase_dim = 5;
        int32_t chase_empty[chase_dim * chase_dim] = {};
        int32_t frame_a[chase_dim * chase_dim] = {};
        int32_t frame_b[chase_dim * chase_dim] = {};
        int32_t frame_c[chase_dim * chase_dim] = {};
        const int chase_token = 0;
        const void *chase_viewport = &chase_token;

        frame_a[1 * chase_dim + 1] = 77; // A at (1,1)
        frame_a[2 * chase_dim + 2] = 88; // B at (2,2), stationary, keeps the default facing
        frame_b[2 * chase_dim + 1] = 77; // A steps east: (1,1) -> (2,1), picks up an east facing
        frame_b[2 * chase_dim + 2] = 88; // B unchanged

        // Both step south in the same frame: shared_movement_delta needs two tiles on the same
        // delta.
        frame_c[2 * chase_dim + 2] = 77; // A: (2,1) -> (2,2)
        frame_c[2 * chase_dim + 3] = 88; // B: (2,2) -> (2,3)

        visual_animation_managerst manager;
        auto input = make_input(chase_viewport, chase_dim, chase_empty);
        set_layer(input, viewport_visual_layer::center, frame_a, chase_empty);
        run_frame(manager, input, 1000);
        set_layer(input, viewport_visual_layer::center, frame_b, frame_a);
        run_frame(manager, input, 1016);
        assert(manager.get_facing(chase_viewport, 2, 1) == visual_facingst::east);
        assert(manager.get_facing(chase_viewport, 2, 2) == native_sprite_facing);

        set_layer(input, viewport_visual_layer::center, frame_c, frame_b);
        run_frame(manager, input, 1032);
        assert(manager.get_facing(chase_viewport, 2, 2) == visual_facingst::east);
        // B carries its own default forward, not A's, though A wrote (2,2) earlier in the same
        // pass.
        assert(manager.get_facing(chase_viewport, 2, 3) == native_sprite_facing);
    }

    // Regression: a vacated source tile is reoccupied the same frame by an UNTRACKED creature.
    // Nothing targets that tile, so no target write clears it, and it is not empty either.
    // Only an explicit, order-independent source clear restores the default there.
    {
        constexpr int32_t gap_dim = 5;
        int32_t gap_empty[gap_dim * gap_dim] = {};
        int32_t frame_a[gap_dim * gap_dim] = {};
        int32_t frame_b[gap_dim * gap_dim] = {};
        int32_t frame_c[gap_dim * gap_dim] = {};
        const int gap_token = 0;
        const void *gap_viewport = &gap_token;

        // E is a companion so that D's later departure shares a delta with E's own move.
        // shared_movement_delta engages only once two tiles move on the same delta.
        frame_a[1 * gap_dim + 1] = 77; // D at (1,1)
        frame_a[2 * gap_dim + 2] = 88; // E at (2,2), stationary companion
        frame_b[2 * gap_dim + 1] =
            77; // D steps east: (1,1) -> (2,1), facing differs from the default
        frame_b[2 * gap_dim + 2] = 88; // E unchanged

        // D and E both step south, chained, and F appears at D's just-vacated tile the same frame.
        // previous[(2,1)] was occupied by D, so the empty-cell fallback cannot see F.
        // No shared-delta source matches F's texpos either, so its arrival registers no movement.
        frame_c[2 * gap_dim + 2] = 77; // D: (2,1) -> (2,2)
        frame_c[2 * gap_dim + 3] = 88; // E: (2,2) -> (2,3)
        frame_c[2 * gap_dim + 1] = 55; // F appears at (2,1), untracked

        visual_animation_managerst manager;
        auto input = make_input(gap_viewport, gap_dim, gap_empty);
        set_layer(input, viewport_visual_layer::center, frame_a, gap_empty);
        run_frame(manager, input, 1000);
        set_layer(input, viewport_visual_layer::center, frame_b, frame_a);
        run_frame(manager, input, 1016);
        assert(manager.get_facing(gap_viewport, 2, 1) == visual_facingst::east);

        set_layer(input, viewport_visual_layer::center, frame_c, frame_b);
        run_frame(manager, input, 1032);
        assert(!manager.get_movement(gap_viewport, viewport_visual_layer::center, 2, 1).active);
        // F must not inherit D's stale east facing, though the tile is occupied rather than empty.
        assert(manager.get_facing(gap_viewport, 2, 1) == native_sprite_facing);
        assert(manager.get_facing(gap_viewport, 2, 2) == visual_facingst::east);
        assert(manager.get_facing(gap_viewport, 2, 3) == native_sprite_facing);
    }

    assert(animation_progress(100, 0, 100) == 1.0f);
    assert(inherited_visual_source_tile(0, 0, 1) == -1);
    assert(inherited_visual_source_tile(2, 0, 1) == 1);
    assert(visual_layer_descriptor(viewport_visual_layer::right).center_x == -1);
    assert(visual_layer_descriptor(viewport_visual_layer::left).center_x == 1);
    assert(visual_layer_descriptor(viewport_visual_layer::upright).center_x == -1 &&
           visual_layer_descriptor(viewport_visual_layer::upright).center_y == 1);
    assert(visual_layer_descriptor(viewport_visual_layer::up).center_x == 0 &&
           visual_layer_descriptor(viewport_visual_layer::up).center_y == 1);
    assert(visual_layer_descriptor(viewport_visual_layer::upleft).center_x == 1 &&
           visual_layer_descriptor(viewport_visual_layer::upleft).center_y == 1);

    std::array<int32_t, 9> empty{};
    std::array<int32_t, 9> current{};
    std::array<int32_t, 9> previous{};
    const void *viewport = reinterpret_cast<const void *>(uintptr_t(1));
    auto input = make_input(viewport, 3, empty.data());
    set_layer(input, viewport_visual_layer::center, current.data(), previous.data());

    visual_animation_managerst movement;
    run_frame(movement, input, 1990);
    assert(!movement.requires_full_redraw());

    previous[0 * 3 + 1] = 42;
    current[1 * 3 + 1] = 42;
    run_frame(movement, input, 2000);
    auto render = movement.get_movement(viewport, viewport_visual_layer::center, 1, 1);
    assert(render.active);
    assert(render.source_x == 0 && render.source_y == 1);
    assert(render.progress == 0.0f);
    assert(movement.requires_full_redraw());

    previous = current;
    set_layer(input, viewport_visual_layer::center, current.data(), previous.data());
    run_frame(movement, input, 2050);
    render = movement.get_movement(viewport, viewport_visual_layer::center, 1, 1);
    assert(render.active);
    assert(render.progress == 0.5f);

    run_frame(movement, input, 2100);
    assert(!movement.get_movement(viewport, viewport_visual_layer::center, 1, 1).active);
    assert(movement.requires_full_redraw());

    run_frame(movement, input, 2120);
    assert(!movement.requires_full_redraw());

    visual_animation_managerst ambiguous;
    run_frame(ambiguous, input, 2990);
    previous.fill(0);
    previous[0 * 3 + 1] = 42;
    previous[1 * 3 + 0] = 42;
    set_layer(input, viewport_visual_layer::center, current.data(), previous.data());
    run_frame(ambiguous, input, 3000);
    assert(!ambiguous.get_movement(viewport, viewport_visual_layer::center, 1, 1).active);

    // A handler and led animal form an occupied chain: each enters the other's old space.
    visual_animation_managerst convoy;
    current.fill(0);
    previous.fill(0);
    run_frame(convoy, input, 3490);
    previous[0 * 3 + 1] = 41;
    previous[1 * 3 + 1] = 42;
    current[1 * 3 + 1] = 41;
    current[2 * 3 + 1] = 42;
    run_frame(convoy, input, 3500);
    const auto animal = convoy.get_movement(viewport, viewport_visual_layer::center, 1, 1);
    const auto handler = convoy.get_movement(viewport, viewport_visual_layer::center, 2, 1);
    assert(animal.active && animal.source_x == 0 && animal.source_y == 1);
    assert(handler.active && handler.source_x == 1 && handler.source_y == 1);

    // A multi-tile fragment can use its own movement or its mapped center tile as proof of
    // ownership.
    for (const auto layer :
         {viewport_visual_layer::right, viewport_visual_layer::left, viewport_visual_layer::upright,
          viewport_visual_layer::up, viewport_visual_layer::upleft}) {
        current.fill(0);
        previous.fill(0);
        previous[0 * 3 + 1] = 50;
        current[1 * 3 + 1] = 50;
        assert(visual_moved_between_tiles(layer, current.data(), previous.data(), 0 * 3 + 1,
                                          1 * 3 + 1));
        previous[1 * 3 + 1] = 50;
        assert(!visual_moved_between_tiles(layer, current.data(), previous.data(), 0 * 3 + 1,
                                           1 * 3 + 1));
    }

    visual_animation_managerst context;
    current.fill(0);
    previous.fill(0);
    run_frame(context, input, 4000);
    previous[0 * 3 + 1] = 42;
    current[1 * 3 + 1] = 42;
    run_frame(context, input, 4010);
    assert(context.get_movement(viewport, viewport_visual_layer::center, 1, 1).active);

    ++input.context_revision;
    run_frame(context, input, 4020);
    assert(!context.get_movement(viewport, viewport_visual_layer::center, 1, 1).active);

    // Camera-pan handling. window_x/window_y change at input time but the buffers shift on a later
    // render frame, so the manager must (a) NOT create movements from the buffer shift itself (the
    // floating-sprite bug), and (b) translate in-flight movements on the frame the shift lands.
    std::array<int32_t, 9> pan_current{};
    std::array<int32_t, 9> pan_previous{};
    std::array<int32_t, 9> pan_empty{};
    auto pan_input = make_input(viewport, 3, pan_empty.data());
    set_layer(pan_input, viewport_visual_layer::center, pan_current.data(), pan_previous.data());

    // FLOAT REGRESSION: a stationary creature, pan announced at frame A, buffers shift at frame B.
    // Frame B's buffers look exactly like a real move ((1,1)->(0,1) with a unique source) — the
    // manager must recognize it as the pending pan and create NO movement.
    visual_animation_managerst floaty;
    pan_previous[1 * 3 + 1] = 42;
    pan_current[1 * 3 + 1] = 42;
    run_frame(floaty, pan_input, 4990);
    pan_input.pan_x = 1; // frame A: window scrolled, buffers unchanged
    run_frame(floaty, pan_input, 5000);
    assert(!floaty.get_movement(viewport, viewport_visual_layer::center, 1, 1).active);
    pan_previous[1 * 3 + 1] = 42; // frame B: buffers apply the shift
    pan_current.fill(0);
    pan_current[0 * 3 + 1] = 42;
    run_frame(floaty, pan_input, 5010);
    assert(!floaty.get_movement(viewport, viewport_visual_layer::center, 0, 1).active);

    // FOLLOW: an in-flight movement survives the announce frame untouched and is translated on the
    // frame the buffers shift, so the sprite tracks the scrolled world.
    visual_animation_managerst panner;
    pan_current.fill(0);
    pan_previous.fill(0);
    pan_input.pan_x = 0;
    run_frame(panner, pan_input, 5990);
    pan_previous[0 * 3 + 1] = 42; // creature steps (0,1) -> (1,1)
    pan_current[1 * 3 + 1] = 42;
    run_frame(panner, pan_input, 6000);
    auto moved = panner.get_movement(viewport, viewport_visual_layer::center, 1, 1);
    assert(moved.active && moved.source_x == 0 && moved.source_y == 1);

    pan_input.pan_x = 1; // frame A: pan announced, buffers unchanged
    pan_previous[0 * 3 + 1] = 0;
    pan_previous[1 * 3 + 1] = 42; // previous now matches current (stationary at (1,1))
    run_frame(panner, pan_input, 6010);
    moved = panner.get_movement(viewport, viewport_visual_layer::center, 1, 1);
    assert(moved.active && moved.source_x == 0); // untouched: still anchored to the old frame

    pan_current.fill(0); // frame B: buffers shift east by one
    pan_current[0 * 3 + 1] = 42;
    run_frame(panner, pan_input, 6020);
    assert(!panner.get_movement(viewport, viewport_visual_layer::center, 1, 1).active);
    auto followed = panner.get_movement(viewport, viewport_visual_layer::center, 0, 1);
    assert(followed.active && followed.source_x == -1 && followed.source_y == 1);

    // SAME-FRAME: pan announced and buffers shifted in the same call — translated immediately.
    visual_animation_managerst same_frame;
    pan_current.fill(0);
    pan_previous.fill(0);
    pan_input.pan_x = 0;
    run_frame(same_frame, pan_input, 6990);
    pan_previous[0 * 3 + 1] = 42;
    pan_current[1 * 3 + 1] = 42;
    run_frame(same_frame, pan_input, 7000);
    assert(same_frame.get_movement(viewport, viewport_visual_layer::center, 1, 1).active);
    pan_input.pan_x = 1;
    pan_previous = pan_current;
    pan_current.fill(0);
    pan_current[0 * 3 + 1] = 42;
    run_frame(same_frame, pan_input, 7010);
    followed = same_frame.get_movement(viewport, viewport_visual_layer::center, 0, 1);
    assert(followed.active && followed.source_x == -1);

    // A change that is NOT a pure pan (context revision bump) still resets, even with in-flight
    // work.
    visual_animation_managerst reset_on_zoom;
    pan_current.fill(0);
    pan_previous.fill(0);
    pan_input.pan_x = 0;
    pan_input.context_revision = 1;
    run_frame(reset_on_zoom, pan_input, 8000);
    pan_previous[0 * 3 + 1] = 42;
    pan_current[1 * 3 + 1] = 42;
    run_frame(reset_on_zoom, pan_input, 8010);
    assert(reset_on_zoom.get_movement(viewport, viewport_visual_layer::center, 1, 1).active);
    pan_input.context_revision = 2; // e.g. zoom / z-level / resize
    run_frame(reset_on_zoom, pan_input, 8020);
    assert(!reset_on_zoom.get_movement(viewport, viewport_visual_layer::center, 1, 1).active);

    // Status fragments inherit nearby center motion even while their texture flashes.
    std::array<int32_t, 9> status_current{};
    std::array<int32_t, 9> status_previous{};
    current.fill(0);
    previous.fill(0);
    input.context_revision = 1;
    input.current.fill(empty.data());
    input.previous.fill(empty.data());
    set_layer(input, viewport_visual_layer::center, current.data(), previous.data());
    set_layer(input, viewport_visual_layer::item, status_current.data(), status_previous.data());
    set_layer(input, viewport_visual_layer::designation, status_current.data(),
              status_previous.data());
    visual_animation_managerst companion;
    run_frame(companion, input, 8990);
    previous[0 * 3 + 1] = 42;
    current[1 * 3 + 1] = 42;
    status_previous[0 * 3 + 0] = 90;
    status_current[1 * 3 + 0] = 91;
    assert(visual_moved_between_tiles(viewport_visual_layer::designation, status_current.data(),
                                      status_previous.data(), 0 * 3 + 0, 1 * 3 + 0));
    status_previous[0 * 3 + 0] = 0;
    assert(visual_moved_between_tiles(viewport_visual_layer::designation, status_current.data(),
                                      status_previous.data(), 0 * 3 + 0, 1 * 3 + 0));
    assert(!visual_moved_between_tiles(viewport_visual_layer::item, status_current.data(),
                                       status_previous.data(), 0 * 3 + 0, 1 * 3 + 0));
    status_previous[1 * 3 + 0] = 80;
    assert(!visual_moved_between_tiles(viewport_visual_layer::designation, status_current.data(),
                                       status_previous.data(), 0 * 3 + 0, 1 * 3 + 0));
    status_previous[0 * 3 + 0] = 90;
    status_previous[1 * 3 + 0] = 0;
    run_frame(companion, input, 9000);
    auto status = companion.get_movement(viewport, viewport_visual_layer::designation, 1, 0);
    assert(status.active && !status.inherited && status.source_x == 0 && status.source_y == 0);
    const auto carried_item = companion.get_movement(viewport, viewport_visual_layer::item, 1, 0);
    assert(carried_item.active && carried_item.inherited);
    previous = current;
    status_current[1 * 3 + 0] = 92;
    run_frame(companion, input, 9050);
    status = companion.get_movement(viewport, viewport_visual_layer::designation, 1, 0);
    assert(status.active && status.progress == 0.5f);

    // Divergent nearby creature movements make companion ownership ambiguous, so the overlay snaps.
    current.fill(0);
    previous.fill(0);
    status_current.fill(0);
    status_previous.fill(0);
    visual_animation_managerst crowd;
    run_frame(crowd, input, 9990);
    previous[0 * 3 + 0] = 41;
    current[0 * 3 + 1] = 41;
    previous[2 * 3 + 2] = 42;
    current[2 * 3 + 1] = 42;
    status_current[1 * 3 + 1] = 99;
    run_frame(crowd, input, 10000);
    assert(!crowd.get_movement(viewport, viewport_visual_layer::designation, 1, 1).active);
    status_previous[1 * 3 + 0] = 90;
    status_current[1 * 3 + 1] = 91;
    run_frame(crowd, input, 10010);
    status = crowd.get_movement(viewport, viewport_visual_layer::designation, 1, 1);
    assert(status.active);
    assert(!status.inherited);
    assert(status.source_x == 1 && status.source_y == 0);

    // Wheelbarrows use the item layer and the same independent adjacent-movement detection.
    current.fill(0);
    previous.fill(0);
    input.current.fill(empty.data());
    input.previous.fill(empty.data());
    set_layer(input, viewport_visual_layer::item, current.data(), previous.data());
    visual_animation_managerst item;
    run_frame(item, input, 10990);
    previous[0 * 3 + 1] = 77;
    current[1 * 3 + 1] = 77;
    run_frame(item, input, 11000);
    const auto item_move = item.get_movement(viewport, viewport_visual_layer::item, 1, 1);
    assert(item_move.active && item_move.source_x == 0 && item_move.source_y == 1);

    // Minecart graphics can change texpos while moving; vehicle identity is tile occupancy.
    current.fill(0);
    previous.fill(0);
    input.current.fill(empty.data());
    input.previous.fill(empty.data());
    set_layer(input, viewport_visual_layer::vehicle, current.data(), previous.data());
    visual_animation_managerst vehicle;
    run_frame(vehicle, input, 11990);
    previous[0 * 3 + 1] = 77;
    current[1 * 3 + 1] = 78;
    run_frame(vehicle, input, 12000);
    assert(vehicle.get_movement(viewport, viewport_visual_layer::vehicle, 1, 1).active);
    previous = current;
    current[1 * 3 + 1] = 79;
    run_frame(vehicle, input, 12050);
    const auto cart = vehicle.get_movement(viewport, viewport_visual_layer::vehicle, 1, 1);
    assert(cart.active && cart.progress == 0.5f);
    previous = current;
    current.fill(0);
    current[2 * 3 + 1] = 80;
    run_frame(vehicle, input, 12060);
    const auto chained = vehicle.get_movement(viewport, viewport_visual_layer::vehicle, 2, 1);
    assert(chained.active && chained.source_x > 0.0f && chained.source_x < 1.0f &&
           chained.progress == 0.0f);
}
