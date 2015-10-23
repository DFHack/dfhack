#include "DataDefs.h"
#include "MiscUtils.h"
#include "modules/Renderer.h"

using namespace DFHack;
using df::global::enabler;
using df::global::gps;
using DFHack::Renderer::renderer_wrap;

static renderer_wrap *original_renderer = NULL;

bool init()
{
    if (!original_renderer)
        original_renderer = Renderer::GetRenderer();
    return original_renderer;
}

renderer_wrap *Renderer::AddRenderer (renderer_wrap *r, bool refresh_screen)
{
    if (!init())
    {
        delete r;
        return NULL;
    }
    renderer_wrap *cur = GetRenderer();
    r->parent = cur;
    r->child = NULL;
    r->copy_from_parent();
    if (cur != original_renderer)
    {
        cur->child = original_renderer;
    }
    r->copy_from_parent();
    enabler->renderer = r;
    if (refresh_screen && gps)
        gps->force_full_display_count++;
    return r;
}

void Renderer::RemoveRenderer (renderer_wrap *r)
{
    if (r)
    {
        if (original_renderer)
        {
            r->parent->child = r->child;
            if (r->child)
            {
                r->child->parent = r->parent;
            }
            enabler->renderer = r->parent;
        }
        delete r;
    }
}

bool Renderer::RendererExists (renderer_wrap *r)
{
    renderer_wrap *cur = GetRenderer();
    while (cur && cur != r && cur != original_renderer)
        cur = cur->parent;
    return cur == r;
}

void Renderer::renderer_wrap::set_to_null() {
    screen = NULL;
    screentexpos = NULL;
    screentexpos_addcolor = NULL;
    screentexpos_grayscale = NULL;
    screentexpos_cf = NULL;
    screentexpos_cbr = NULL;
    screen_old = NULL;
    screentexpos_old = NULL;
    screentexpos_addcolor_old = NULL;
    screentexpos_grayscale_old = NULL;
    screentexpos_cf_old = NULL;
    screentexpos_cbr_old = NULL;
}

void Renderer::renderer_wrap::copy_from_parent() {
    screen = parent->screen;
    screentexpos = parent->screentexpos;
    screentexpos_addcolor = parent->screentexpos_addcolor;
    screentexpos_grayscale = parent->screentexpos_grayscale;
    screentexpos_cf = parent->screentexpos_cf;
    screentexpos_cbr = parent->screentexpos_cbr;
    screen_old = parent->screen_old;
    screentexpos_old = parent->screentexpos_old;
    screentexpos_addcolor_old = parent->screentexpos_addcolor_old;
    screentexpos_grayscale_old = parent->screentexpos_grayscale_old;
    screentexpos_cf_old = parent->screentexpos_cf_old;
    screentexpos_cbr_old = parent->screentexpos_cbr_old;
}

void Renderer::renderer_wrap::copy_to_parent() {
    parent->screen = screen;
    parent->screentexpos = screentexpos;
    parent->screentexpos_addcolor = screentexpos_addcolor;
    parent->screentexpos_grayscale = screentexpos_grayscale;
    parent->screentexpos_cf = screentexpos_cf;
    parent->screentexpos_cbr = screentexpos_cbr;
    parent->screen_old = screen_old;
    parent->screentexpos_old = screentexpos_old;
    parent->screentexpos_addcolor_old = screentexpos_addcolor_old;
    parent->screentexpos_grayscale_old = screentexpos_grayscale_old;
    parent->screentexpos_cf_old = screentexpos_cf_old;
    parent->screentexpos_cbr_old = screentexpos_cbr_old;
}

void Renderer::renderer_wrap::update_tile(int32_t x, int32_t y) {
    copy_to_parent();
    parent->update_tile(x,y);
};

void Renderer::renderer_wrap::update_all() {
    copy_to_parent();
    parent->update_all();
};

void Renderer::renderer_wrap::render() {
    copy_to_parent();
    parent->render();
};

void Renderer::renderer_wrap::set_fullscreen() {
    copy_to_parent();
    parent->set_fullscreen();
    copy_from_parent();
};

void Renderer::renderer_wrap::zoom(df::zoom_commands z) {
    copy_to_parent();
    parent->zoom(z);
    copy_from_parent();
};

void Renderer::renderer_wrap::resize(int32_t w, int32_t h) {
    copy_to_parent();
    parent->resize(w,h);
    copy_from_parent();
};

void Renderer::renderer_wrap::grid_resize(int32_t w, int32_t h) {
    copy_to_parent();
    parent->grid_resize(w,h);
    copy_from_parent();
};

bool Renderer::renderer_wrap::get_mouse_coords(int32_t* x, int32_t* y) {
    return parent->get_mouse_coords(x,y);
};

bool Renderer::renderer_wrap::uses_opengl() {
    return parent->uses_opengl();
};
