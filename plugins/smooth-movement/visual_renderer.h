// SPDX-License-Identifier: MIT

#pragma once

namespace DFHack {
class color_ostream;
}

namespace df {
struct renderer_2d_base;
}

namespace smooth_movement {

bool initialize(DFHack::color_ostream &out);
void shutdown();
void render(df::renderer_2d_base *renderer);

bool flip_enabled();
void set_flip_enabled(bool enable);
bool zlevel_enabled();
void set_zlevel_enabled(bool enable);

} // namespace smooth_movement
