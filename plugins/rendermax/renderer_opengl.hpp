//original file from https://github.com/Baughn/Dwarf-Fortress--libgraphics-
#pragma once

#include "Core.h"
#include <VTableInterpose.h>
#include "df/renderer.h"
#include "df/init.h"
#include "df/enabler.h"
#include "df/zoom_commands.h"
#include "df/texture_handler.h"
#include "df/graphic.h"
#include <math.h>
#include <cmath>
#include <mutex>

using df::renderer;
using df::init;
using df::enabler;

struct old_opengl:public renderer
{
    void* sdlSurface;
    int32_t dispx,dispy;
    float *vertexes, *fg, *bg, *tex;
    int32_t zoom_steps,forced_steps,natural_w,natural_h;
    int32_t off_x,off_y,size_x,size_y;
};
struct renderer_wrap : public renderer {
private:
    void set_to_null() {
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

    void copy_from_inner() {
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

    void copy_to_inner() {
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
public:
    renderer_wrap(renderer* parent):parent(parent)
    {
        copy_from_inner();
    }
    virtual void update_tile(int32_t x, int32_t y) {

        copy_to_inner();
        parent->update_tile(x,y);
    };
    virtual void update_all() {
        copy_to_inner();
        parent->update_all();
    };
    virtual void render() {
        copy_to_inner();
        parent->render();
    };
    virtual void set_fullscreen() {
        copy_to_inner();
        parent->set_fullscreen();
        copy_from_inner();
    };
    virtual void zoom(df::zoom_commands z) {
        copy_to_inner();
        parent->zoom(z);
        copy_from_inner();
    };
    virtual void resize(int32_t w, int32_t h) {
        copy_to_inner();
        parent->resize(w,h);
        copy_from_inner();
    };
    virtual void grid_resize(int32_t w, int32_t h) {
        copy_to_inner();
        parent->grid_resize(w,h);
        copy_from_inner();
    };
    virtual ~renderer_wrap() {
        df::global::enabler->renderer=parent;
    };
    virtual bool get_mouse_coords(int32_t* x, int32_t* y) {
        return parent->get_mouse_coords(x,y);
    };
    virtual bool uses_opengl() {
        return parent->uses_opengl();
    };
    void invalidateRect(int32_t x,int32_t y,int32_t w,int32_t h)
    {
        for(int i=x;i<x+w;i++)
        for(int j=y;j<y+h;j++)
        {
            int index=i*df::global::gps->dimy + j;
            screen_old[index*4]=screen[index*4]+1;//ensure tile is different
        }
    };
    void invalidate()
    {
        invalidateRect(0,0,df::global::gps->dimx,df::global::gps->dimy);
        //df::global::gps->force_full_display_count++;
    };
protected:
    renderer* parent;
};
struct renderer_trippy : public renderer_wrap {
private:
    float rFloat()
    {
        return rand()/(float)RAND_MAX;
    }
    void colorizeTile(int x,int y)
    {
        const int tile = x*(df::global::gps->dimy) + y;
        old_opengl* p=reinterpret_cast<old_opengl*>(parent);
        float *fg = p->fg + tile * 4 * 6;
        float *bg = p->bg + tile * 4 * 6;
        // float *tex = p->tex + tile * 2 * 6;
        const float val=1/2.0;

        float r=rFloat()*val - val/2;
        float g=rFloat()*val - val/2;
        float b=rFloat()*val - val/2;

        float backr=rFloat()*val - val/2;
        float backg=rFloat()*val - val/2;
        float backb=rFloat()*val - val/2;
        for (int i = 0; i < 6; i++) {
            *(fg++) += r;
            *(fg++) += g;
            *(fg++) += b;
            *(fg++) = 1;

            *(bg++) += backr;
            *(bg++) += backg;
            *(bg++) += backb;
            *(bg++) = 1;
        }
    }
public:
    renderer_trippy(renderer* parent):renderer_wrap(parent)
    {
    }
    virtual void update_tile(int32_t x, int32_t y) {
        renderer_wrap::update_tile(x,y);
        colorizeTile(x,y);
    };
    virtual void update_all() {
        renderer_wrap::update_all();
        for (int x = 0; x < df::global::gps->dimx; x++)
            for (int y = 0; y < df::global::gps->dimy; y++)
                colorizeTile(x,y);
    };
};

struct rgbf
{
    float r,g,b;
    rgbf():r(0),g(0),b(0)
    {

    }
    rgbf(float r,float g,float b):r(r),g(g),b(b)
    {

    }
    rgbf operator-(const rgbf& cell) const
    {
        return rgbf(r-cell.r,g-cell.g,b-cell.b);
    }
    rgbf operator*(float val)const
    {
        return rgbf(r*val,g*val,b*val);
    }
    rgbf operator/(float val) const
    {
        return rgbf(r/val,g/val,b/val);
    }
    rgbf operator*(const rgbf& cell) const
    {
        return rgbf(r*cell.r,g*cell.g,b*cell.b);
    }
    rgbf operator*=(float val)
    {
        r*=val;
        g*=val;
        b*=val;
        return *this;
    }
    rgbf operator*=(const rgbf& cell)
    {
        r*=cell.r;
        g*=cell.g;
        b*=cell.b;
        return *this;
    }
    rgbf operator+=(const rgbf& cell)
    {
        r+=cell.r;
        g+=cell.g;
        b+=cell.b;
        return *this;
    }
    rgbf operator+(const rgbf& other) const
    {
        return rgbf(r+other.r,g+other.g,b+other.b);
    }
    bool operator<=(const rgbf& other) const
    {
        return r<=other.r && g<=other.g && b<=other.b;
    }
    float dot(const rgbf& other) const
    {
        return r*other.r+g*other.g+b*other.b;
    }
    rgbf pow(const float exp) const
    {
        return rgbf(std::pow(r, exp), std::pow(g, exp), std::pow(b, exp));
    }
    rgbf pow(const int exp) const
    {
        return rgbf(std::pow(r, exp), std::pow(g, exp), std::pow(b, exp));
    }
};
struct renderer_test : public renderer_wrap {
private:
    void colorizeTile(int x,int y)
    {
        const int tile = x*(df::global::gps->dimy) + y;
        old_opengl* p=reinterpret_cast<old_opengl*>(parent);
        float *fg = p->fg + tile * 4 * 6;
        float *bg = p->bg + tile * 4 * 6;
        // float *tex = p->tex + tile * 2 * 6;
        rgbf light=lightGrid[tile];
        for (int i = 0; i < 6; i++) {
            *(fg++) *= light.r;
            *(fg++) *= light.g;
            *(fg++) *= light.b;
            *(fg++) = 1;

            *(bg++) *= light.r;
            *(bg++) *= light.g;
            *(bg++) *= light.b;
            *(bg++) = 1;
        }
    }
    void reinitLightGrid(int w,int h)
    {
        std::lock_guard<std::mutex> guard{dataMutex};
        lightGrid.resize(w*h);
    }
    void reinitLightGrid()
    {
        reinitLightGrid(df::global::gps->dimy,df::global::gps->dimx);
    }
public:
    std::mutex dataMutex;
    std::vector<rgbf> lightGrid;
    renderer_test(renderer* parent):renderer_wrap(parent)
    {
        reinitLightGrid();
    }
    virtual void update_tile(int32_t x, int32_t y) {
        renderer_wrap::update_tile(x,y);
        std::lock_guard<std::mutex> guard{dataMutex};
        colorizeTile(x,y);
        //some sort of mutex or sth?
        //and then map read
    };
    virtual void update_all() {
        renderer_wrap::update_all();
        std::lock_guard<std::mutex> guard{dataMutex};
        for (int x = 0; x < df::global::gps->dimx; x++)
            for (int y = 0; y < df::global::gps->dimy; y++)
                colorizeTile(x,y);
        //some sort of mutex or sth?
        //and then map read
        //same stuff for all of them i guess...
    };
    virtual void set_fullscreen()
    {
        renderer_wrap::set_fullscreen();
        reinitLightGrid();
    }
    virtual void zoom(df::zoom_commands z)
    {
        renderer_wrap::zoom(z);
        reinitLightGrid();
    }
    virtual void grid_resize(int32_t w, int32_t h) {
        renderer_wrap::grid_resize(w,h);
        reinitLightGrid(w,h);
    };
    virtual void resize(int32_t w, int32_t h) {
        renderer_wrap::resize(w,h);
        reinitLightGrid(w,h);
    }
};

struct rgba
{
    float r,g,b,a;
};
struct renderer_lua : public renderer_wrap {
private:
    void overwriteTile(int x,int y)
    {
        const int tile = xyToTile(x,y);
        old_opengl* p=reinterpret_cast<old_opengl*>(parent);
        float *fg = p->fg + tile * 4 * 6;
        float *bg = p->bg + tile * 4 * 6;
        // float *tex = p->tex + tile * 2 * 6;
        rgbf fm=foreMult[tile];
        rgbf fo=foreOffset[tile];

        rgbf bm=backMult[tile];
        rgbf bo=backOffset[tile];
        for (int i = 0; i < 6; i++) {
            rgba* fore=reinterpret_cast<rgba*>(fg);
            fore->r=fore->r*fm.r+fo.r;
            fore->g=fore->g*fm.g+fo.g;
            fore->b=fore->b*fm.b+fo.b;

            fg+=4;
            rgba* back=reinterpret_cast<rgba*>(bg);
            back->r=back->r*bm.r+bo.r;
            back->g=back->g*bm.g+bo.g;
            back->b=back->b*bm.b+bo.b;
            bg+=4;
        }
    }
    void reinitGrids(int w,int h)
    {
        std::lock_guard<std::mutex> guard{dataMutex};
        foreOffset.resize(w*h);
        foreMult.resize(w*h);
        backOffset.resize(w*h);
        backMult.resize(w*h);
    }
    void reinitGrids()
    {
        reinitGrids(df::global::gps->dimy,df::global::gps->dimx);
    }
public:
    std::mutex dataMutex;
    std::vector<rgbf> foreOffset,foreMult;
    std::vector<rgbf> backOffset,backMult;
    inline int xyToTile(int x, int y)
    {
       return x*(df::global::gps->dimy) + y;
    }
    renderer_lua(renderer* parent):renderer_wrap(parent)
    {
        reinitGrids();
    }
    virtual void update_tile(int32_t x, int32_t y) {
        renderer_wrap::update_tile(x,y);
        std::lock_guard<std::mutex> guard{dataMutex};
        overwriteTile(x,y);
        //some sort of mutex or sth?
        //and then map read
    };
    virtual void update_all() {
        renderer_wrap::update_all();
        std::lock_guard<std::mutex> guard{dataMutex};
        for (int x = 0; x < df::global::gps->dimx; x++)
            for (int y = 0; y < df::global::gps->dimy; y++)
                overwriteTile(x,y);
        //some sort of mutex or sth?
        //and then map read
        //same stuff for all of them i guess...
    };
    virtual void grid_resize(int32_t w, int32_t h) {
        renderer_wrap::grid_resize(w,h);
        reinitGrids(w,h);
    };
    virtual void resize(int32_t w, int32_t h) {
        renderer_wrap::resize(w,h);
        reinitGrids(w,h);
    }
};
