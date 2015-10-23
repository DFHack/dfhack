#include "Export.h"
#include "df/enabler.h"
#include "df/graphic.h"
#include "df/renderer.h"
#pragma once

namespace DFHack { namespace Renderer {
    struct DFHACK_EXPORT renderer_wrap : public df::renderer {
        void set_to_null();
        void copy_from_parent();
        void copy_to_parent();
        renderer_wrap *parent;
        renderer_wrap *child;

        virtual void update_tile(int32_t x, int32_t y);
        virtual void update_all();
        virtual void render();
        virtual void set_fullscreen();
        virtual void zoom(df::zoom_commands z);
        virtual void resize(int32_t w, int32_t h);
        virtual void grid_resize(int32_t w, int32_t h);
        virtual ~renderer_wrap() {
            // All necessary cleanup should be handled by RemoveRenderer()
        };
        virtual bool get_mouse_coords(int32_t *x, int32_t *y);
        virtual bool uses_opengl();
    };
    // Returns the renderer instance given on success, or deletes it and returns NULL on failure
    // Usage: renderer_foo *r = AddRenderer(new renderer_foo)
    DFHACK_EXPORT renderer_wrap *AddRenderer(renderer_wrap*, bool refresh_screen = false);
    // Removes and deletes the given renderer instance
    DFHACK_EXPORT void RemoveRenderer(renderer_wrap*);
    DFHACK_EXPORT bool RendererExists(renderer_wrap*);
    inline renderer_wrap *GetRenderer()
    {
        return (renderer_wrap*)(df::global::enabler ? df::global::enabler->renderer : NULL);
    }
}}
