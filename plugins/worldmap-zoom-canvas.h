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

// Included after the verified SDL bindings and state in worldmap-zoom-render.cpp.
// All canvas operations run on the native graphics thread.
struct MapCanvas {
    SDL_Renderer *owner = nullptr;
    void *texture = nullptr;
    unsigned window = 0;
    int width = 0, height = 0;
    bool active = false, valid = false;
    uint64_t camera_epoch = 0, device_epoch = 0;
    Camera view{};
    SDL_Rect map{}, source{}, viewport{}, clip{};
    void *previous_target = nullptr;
    float sx = 1, sy = 1;
    bool clipped = false;
    unsigned char color[4]{};
    df::renderer_2d_base *native_renderer = nullptr;
    df::graphic_map_portst *native_port = nullptr;
    std::vector<unsigned char> painted;
    int columns = 0, x0 = 0, x1 = 0, y0 = 0, y1 = 0, painted_count = 0, required = 0;

    void error(const char *message) {
        std::lock_guard lock(state_mutex);
        camera.enabled = false;
        last_error = message;
    }
    bool restore() {
        bool ok = sdl.set_target(owner, previous_target) == 0;
        ok = (sdl.set_scale(owner, sx, sy) == 0) && ok;
        ok = (sdl.set_viewport(owner, &viewport) == 0) && ok;
        ok = (sdl.set_clip(owner, clipped ? &clip : nullptr) == 0) && ok;
        ok = (sdl.set_color(owner, color[0], color[1], color[2], color[3]) == 0) && ok;
        float x, y;
        SDL_Rect v{}, c{};
        unsigned char rgba[4]{};
        sdl.get_scale(owner, &x, &y);
        sdl.get_viewport(owner, &v);
        sdl.get_clip(owner, &c);
        sdl.get_color(owner, &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
        ok = ok && sdl.get_target(owner) == previous_target && x == sx && y == sy && v.x == viewport.x &&
             v.y == viewport.y && v.w == viewport.w && v.h == viewport.h &&
             (sdl.clip_enabled(owner) != 0) == clipped &&
             (!clipped || (c.x == clip.x && c.y == clip.y && c.w == clip.w && c.h == clip.h)) &&
             std::equal(color, color + 4, rgba);
        ++restore_checks;
        if (!ok) {
            ++restore_errors;
            error("SDL canvas state restoration failed; zoom disabled");
        }
        return ok;
    }
    void finish(bool composite = true) {
        if (!active)
            return;
        Camera live;
        {
            std::lock_guard lock(state_mutex);
            live = camera;
        }
        // A camera change can arrive as individual tile updates. Let that
        // batch fill the visible crop before falling back to a full native
        // draw, avoiding a redundant full draw at the start of every drag.
        if (composite && !valid && live.enabled && live.context == view.context &&
            current_context() == view.context) {
            sdl.set_color(owner, 0, 0, 0, 255);
            sdl.clear(owner);
            sdl.set_color(owner, color[0], color[1], color[2], color[3]);
            ++transform_depth;
            native_renderer->update_full_map_port(native_port);
            --transform_depth;
            valid = true;
        }
        active = false;
        if (!restore())
            return;
        if (composite && valid && live.enabled && live.context == view.context &&
            current_context() == view.context) {
            if (sdl.copy(owner, texture, &source, &map) != 0)
                error("SDL map composition failed; zoom disabled");
            else {
                ++canvas_composites;
                composited_generation = camera_epoch;
            }
        }
    }
    void release(df::renderer_2d_base *r) {
        // SDL_DestroyRenderer frees its textures. Never dereference a texture
        // belonging to a renderer/window that has already been replaced.
        bool same = texture && r && owner == reinterpret_cast<SDL_Renderer *>(r->sdl_renderer) &&
                    window == sdl.window_id(reinterpret_cast<SDL_Window *>(r->window));
        if (same) {
            finish(false);
            sdl.destroy_texture(texture);
        } else
            active = false;
        texture = nullptr;
        owner = nullptr;
        valid = false;
        width = height = 0;
    }
    bool begin(df::renderer_2d_base *r, df::graphic_map_portst *port, const Camera &c, bool full) {
        auto sr = reinterpret_cast<SDL_Renderer *>(r->sdl_renderer);
        auto wid = sdl.window_id(reinterpret_cast<SDL_Window *>(r->window));
        int w = r->origin_x + port->top_left_corner_x + port->dim_x * 16;
        int h = r->origin_y + port->top_left_corner_y + port->dim_y * 16;
        if (w < 1 || h < 1 || w > 16384 || h > 16384) {
            error("Invalid map canvas dimensions");
            return false;
        }
        if (texture && (owner != sr || window != wid || width != w || height != h ||
                        device_epoch != target_generation.load()))
            release(r);
        if (!texture) {
            SDL_RendererInfo info{};
            if (sdl.renderer_info(sr, &info) != 0 || !(info.flags & 8) ||
                (info.max_texture_width > 0 && w > info.max_texture_width) ||
                (info.max_texture_height > 0 && h > info.max_texture_height)) {
                error("Renderer cannot create the required map target texture");
                return false;
            }
            {
                std::lock_guard lock(state_mutex);
                renderer_name = info.name ? info.name : "unknown";
            }
            texture = sdl.create_texture(sr, 0x16362004, 2, w, h); // ARGB8888 / TARGET
            if (!texture) {
                error("Could not create map target texture");
                return false;
            }
            owner = sr;
            window = wid;
            width = w;
            height = h;
            valid = false;
            ++canvas_creations;
            if (sdl.texture_blend(texture, 0) != 0 || sdl.texture_scale(texture, 0) != 0) {
                release(r);
                error("Could not configure map target texture");
                return false;
            }
        }
        auto cg = camera_generation.load(), dg = target_generation.load();
        if (active && (camera_epoch != cg || device_epoch != dg))
            finish(false);
        bool invalidated = camera_epoch != cg || device_epoch != dg || !valid;
        if (camera_epoch != cg || device_epoch != dg)
            valid = false;
        camera_epoch = cg;
        device_epoch = dg;
        view = c;
        map = {r->origin_x + port->top_left_corner_x, r->origin_y + port->top_left_corner_y, port->dim_x * 16,
               port->dim_y * 16};
        source = {
            map.x + static_cast<int>(std::round(std::clamp(c.x, 0.0, port->dim_x * (1 - 1 / c.zoom)) * 16)),
            map.y + static_cast<int>(std::round(std::clamp(c.y, 0.0, port->dim_y * (1 - 1 / c.zoom)) * 16)),
            map.w / static_cast<int>(c.zoom), map.h / static_cast<int>(c.zoom)};
        native_renderer = r;
        native_port = port;
        bool started = !active;
        if (started && invalidated) {
            columns = port->dim_x;
            painted.assign(static_cast<size_t>(columns) * port->dim_y, 0);
            painted_count = 0;
            x0 = (source.x - map.x) / 16;
            x1 = (source.x - map.x + source.w - 1) / 16;
            y0 = (source.y - map.y) / 16;
            y1 = (source.y - map.y + source.h - 1) / 16;
            required = (x1 - x0 + 1) * (y1 - y0 + 1);
        }
        if (!active) {
            previous_target = sdl.get_target(sr);
            sdl.get_scale(sr, &sx, &sy);
            sdl.get_viewport(sr, &viewport);
            sdl.get_clip(sr, &clip);
            clipped = sdl.clip_enabled(sr) != 0;
            sdl.get_color(sr, &color[0], &color[1], &color[2], &color[3]);
            active = true;
            if (sdl.set_target(sr, texture) != 0 || sdl.set_scale(sr, 1, 1) != 0 ||
                sdl.set_viewport(sr, nullptr) != 0 || sdl.set_clip(sr, nullptr) != 0) {
                finish(false);
                error("SDL rejected the map canvas; zoom disabled");
                return false;
            }
        }
        if (full || (!valid && started)) {
            if (sdl.set_color(sr, 0, 0, 0, 255) != 0 || sdl.clear(sr) != 0 ||
                sdl.set_color(sr, color[0], color[1], color[2], color[3]) != 0) {
                finish(false);
                error("Could not clear map canvas; zoom disabled");
                return false;
            }
        }
        if (sdl.set_clip(sr, &source) != 0) {
            finish(false);
            error("SDL rejected map canvas clipping");
            return false;
        }
        return true;
    }
    void clear_tile(int x, int y) {
        SDL_Rect cell{map.x + x * 16, map.y + y * 16, 16, 16};
        if (sdl.set_color(owner, 0, 0, 0, 255) != 0 || sdl.fill(owner, &cell) != 0 ||
            sdl.set_color(owner, color[0], color[1], color[2], color[3]) != 0)
            error("Could not clear map tile; zoom disabled");
    }
    void painted_tile(int x, int y) {
        if (valid || x < x0 || x > x1 || y < y0 || y > y1)
            return;
        auto index = static_cast<size_t>(y) * columns + x;
        if (!painted[index]) {
            painted[index] = 1;
            if (++painted_count == required)
                valid = true;
        }
    }
};
static MapCanvas map_canvas;
static bool canvas_camera(df::renderer_2d_base *r, df::graphic_map_portst *p, Camera &c) {
    if (transform_depth || !hooks_ready || !p || p != gps->main_map_port || r != enabler->renderer ||
        !p->flag.bits.active || p->dim_x < 1 || p->dim_y < 1)
        return false;
    {
        std::lock_guard lock(state_mutex);
        c = camera;
    }
    return c.enabled && c.zoom > 1 && c.context == current_context();
}
