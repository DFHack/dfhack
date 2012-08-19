/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "modules/Screen.h"
#include "MemAccess.h"
#include "VersionInfo.h"
#include "Types.h"
#include "Error.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "PluginManager.h"
#include "MiscUtils.h"
using namespace DFHack;

#include "DataDefs.h"
#include "df/init.h"
#include "df/texture_handler.h"
#include "df/tile_page.h"
#include "df/interfacest.h"

using namespace df::enums;
using df::global::init;
using df::global::gps;
using df::global::texture;
using df::global::gview;

using Screen::Pen;

df::coord2d Screen::getMousePos()
{
    if (!gps) return df::coord2d();

    return df::coord2d(gps->mouse_x, gps->mouse_y);
}

df::coord2d Screen::getWindowSize()
{
    if (!gps) return df::coord2d();

    return df::coord2d(gps->dimx, gps->dimy);
}

static void doSetTile(const Pen &pen, int index)
{
    auto screen = gps->screen + index*4;
    screen[0] = uint8_t(pen.ch);
    screen[1] = uint8_t(pen.fg) & 15;
    screen[2] = uint8_t(pen.bg) & 15;
    screen[3] = uint8_t(pen.bold) & 1;
    gps->screentexpos[index] = pen.tile;
    gps->screentexpos_addcolor[index] = (pen.tile_mode == Screen::Pen::CharColor);
    gps->screentexpos_grayscale[index] = (pen.tile_mode == Screen::Pen::TileColor);
    gps->screentexpos_cf[index] = pen.tile_fg;
    gps->screentexpos_cbr[index] = pen.tile_bg;
}

bool Screen::paintTile(const Pen &pen, int x, int y)
{
    if (!gps) return false;

    int dimx = gps->dimx, dimy = gps->dimy;
    if (x < 0 || x >= dimx || y < 0 || y >= dimy) return false;

    doSetTile(pen, x*dimy + y);
    return true;
}

bool Screen::paintString(const Pen &pen, int x, int y, const std::string &text)
{
    if (!gps || y < 0 || y >= gps->dimy) return false;

    Pen tmp(pen);
    bool ok = false;

    for (size_t i = -std::min(0,x); i < text.size(); i++)
    {
        if (x + i >= size_t(gps->dimx))
            break;

        tmp.ch = text[i];
        tmp.tile = (pen.tile ? pen.tile + uint8_t(text[i]) : 0);
        paintTile(tmp, x+i, y);
        ok = true;
    }

    return ok;
}

bool Screen::fillRect(const Pen &pen, int x1, int y1, int x2, int y2)
{
    if (!gps) return false;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= gps->dimx) x2 = gps->dimx-1;
    if (y2 >= gps->dimy) y2 = gps->dimy-1;
    if (x1 > x2 || y1 > y2) return false;

    for (int x = x1; x <= x2; x++)
    {
        int index = x*gps->dimy;

        for (int y = y1; y <= y2; y++)
            doSetTile(pen, index+y);
    }

    return true;
}

bool Screen::findGraphicsTile(const std::string &pagename, int x, int y, int *ptile, int *pgs)
{
    if (!gps || !texture || x < 0 || y < 0) return false;

    for (size_t i = 0; i < texture->page.size(); i++)
    {
        auto page = texture->page[i];
        if (!page->loaded || page->token != pagename) continue;

        if (x >= page->page_dim_x || y >= page->page_dim_y)
            break;
        int idx = y*page->page_dim_x + x;
        if (size_t(idx) >= page->texpos.size())
            break;

        if (ptile) *ptile = page->texpos[idx];
        if (pgs) *pgs = page->texpos_gs[idx];
        return true;
    }

    return false;
}

bool Screen::show(df::viewscreen *screen, df::viewscreen *before)
{
    CHECK_NULL_POINTER(screen);
    CHECK_INVALID_ARGUMENT(!screen->parent && !screen->child);

    if (!gps || !gview) return false;

    df::viewscreen *parent = &gview->view;
    while (parent && parent->child != before)
        parent = parent->child;

    if (!parent) return false;

    gps->force_full_display_count += 2;

    screen->child = parent->child;
    screen->parent = parent;
    parent->child = screen;
    if (screen->child)
        screen->child->parent = screen;

    return true;
}

void Screen::dismiss(df::viewscreen *screen)
{
    CHECK_NULL_POINTER(screen);

    screen->breakdown_level = interface_breakdown_types::STOPSCREEN;
}

bool Screen::isDismissed(df::viewscreen *screen)
{
    CHECK_NULL_POINTER(screen);

    return screen->breakdown_level != interface_breakdown_types::NONE;
}

static std::set<df::viewscreen*> dfhack_screens;

dfhack_viewscreen::dfhack_viewscreen()
{
    dfhack_screens.insert(this);
}

dfhack_viewscreen::~dfhack_viewscreen()
{
    dfhack_screens.erase(this);
}

bool dfhack_viewscreen::is_instance(df::viewscreen *screen)
{
    return dfhack_screens.count(screen) != 0;
}
