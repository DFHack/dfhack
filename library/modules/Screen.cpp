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
#include "LuaTools.h"

#include "MiscUtils.h"

using namespace DFHack;

#include "DataDefs.h"
#include "df/init.h"
#include "df/texture_handler.h"
#include "df/tile_page.h"
#include "df/interfacest.h"
#include "df/enabler.h"

using namespace df::enums;
using df::global::init;
using df::global::gps;
using df::global::texture;
using df::global::gview;
using df::global::enabler;

using Screen::Pen;

/*
 * Screen painting API.
 */

df::coord2d Screen::getMousePos()
{
    if (!gps || (enabler && !enabler->tracking_on))
        return df::coord2d(-1, -1);

    return df::coord2d(gps->mouse_x, gps->mouse_y);
}

df::coord2d Screen::getWindowSize()
{
    if (!gps) return df::coord2d(80, 25);

    return df::coord2d(gps->dimx, gps->dimy);
}

bool Screen::inGraphicsMode()
{
    return init && init->display.flag.is_set(init_display_flags::USE_GRAPHICS);
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
    if (!gps || !pen.valid()) return false;

    int dimx = gps->dimx, dimy = gps->dimy;
    if (x < 0 || x >= dimx || y < 0 || y >= dimy) return false;

    doSetTile(pen, x*dimy + y);
    return true;
}

Pen Screen::readTile(int x, int y)
{
    if (!gps) return Pen(0,0,0,-1);

    int dimx = gps->dimx, dimy = gps->dimy;
    if (x < 0 || x >= dimx || y < 0 || y >= dimy)
        return Pen(0,0,0,-1);

    int index = x*dimy + y;
    auto screen = gps->screen + index*4;
    if (screen[3] & 0x80)
        return Pen(0,0,0,-1);

    Pen pen(
        screen[0], screen[1], screen[2], screen[3]?true:false,
        gps->screentexpos[index]
    );

    if (pen.tile)
    {
        if (gps->screentexpos_grayscale[index])
        {
            pen.tile_mode = Screen::Pen::TileColor;
            pen.tile_fg = gps->screentexpos_cf[index];
            pen.tile_bg = gps->screentexpos_cbr[index];
        }
        else if (gps->screentexpos_addcolor[index])
        {
            pen.tile_mode = Screen::Pen::CharColor;
        }
    }

    return pen;
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
    if (!gps || !pen.valid()) return false;

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

bool Screen::drawBorder(const std::string &title)
{
    if (!gps) return false;

    int dimx = gps->dimx, dimy = gps->dimy;
    Pen border(0xDB, 8);
    Pen text(0, 0, 7);
    Pen signature(0, 0, 8);

    for (int x = 0; x < dimx; x++)
    {
        doSetTile(border, x * dimy + 0);
        doSetTile(border, x * dimy + dimy - 1);
    }
    for (int y = 0; y < dimy; y++)
    {
        doSetTile(border, 0 * dimy + y);
        doSetTile(border, (dimx - 1) * dimy + y);
    }

    paintString(signature, dimx-8, dimy-1, "DFHack");

    return paintString(text, (dimx - title.length()) / 2, 0, title);
}

bool Screen::clear()
{
    if (!gps) return false;

    return fillRect(Pen(' ',0,0,false), 0, 0, gps->dimx-1, gps->dimy-1);
}

bool Screen::invalidate()
{
    if (!enabler) return false;

    enabler->flag.bits.render = true;
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

    if (dfhack_viewscreen::is_instance(screen))
        static_cast<dfhack_viewscreen*>(screen)->onShow();

    return true;
}

void Screen::dismiss(df::viewscreen *screen, bool to_first)
{
    CHECK_NULL_POINTER(screen);

    if (screen->breakdown_level != interface_breakdown_types::NONE)
        return;

    if (to_first)
        screen->breakdown_level = interface_breakdown_types::TOFIRST;
    else
        screen->breakdown_level = interface_breakdown_types::STOPSCREEN;

    if (dfhack_viewscreen::is_instance(screen))
        static_cast<dfhack_viewscreen*>(screen)->onDismiss();
}

bool Screen::isDismissed(df::viewscreen *screen)
{
    CHECK_NULL_POINTER(screen);

    return screen->breakdown_level != interface_breakdown_types::NONE;
}

/*
 * Base DFHack viewscreen.
 */

static std::set<df::viewscreen*> dfhack_screens;

dfhack_viewscreen::dfhack_viewscreen() : text_input_mode(false)
{
    dfhack_screens.insert(this);

    last_size = Screen::getWindowSize();
}

dfhack_viewscreen::~dfhack_viewscreen()
{
    dfhack_screens.erase(this);
}

bool dfhack_viewscreen::is_instance(df::viewscreen *screen)
{
    return dfhack_screens.count(screen) != 0;
}

void dfhack_viewscreen::check_resize()
{
    auto size = Screen::getWindowSize();

    if (size != last_size)
    {
        last_size = size;
        resize(size.x, size.y);
    }
}

void dfhack_viewscreen::logic()
{
    check_resize();

    // Various stuff works poorly unless always repainting
    Screen::invalidate();
}

void dfhack_viewscreen::render()
{
    check_resize();
}

bool dfhack_viewscreen::key_conflict(df::interface_key key)
{
    if (key == interface_key::OPTIONS)
        return true;

    if (text_input_mode)
    {
        if (key == interface_key::HELP || key == interface_key::MOVIES)
            return true;
    }

    return false;
}

/*
 * Lua-backed viewscreen.
 */

static int DFHACK_LUA_VS_TOKEN = 0;

df::viewscreen *dfhack_lua_viewscreen::get_pointer(lua_State *L, int idx, bool make)
{
    df::viewscreen *screen;

    if (lua_istable(L, idx))
    {
        if (!Lua::IsCoreContext(L))
            luaL_error(L, "only the core context can create lua screens");

        lua_rawgetp(L, idx, &DFHACK_LUA_VS_TOKEN);

        if (!lua_isnil(L, -1))
        {
            if (make)
                luaL_error(L, "this screen is already on display");

            screen = (df::viewscreen*)lua_touserdata(L, -1);
        }
        else
        {
            if (!make)
                luaL_error(L, "this screen is not on display");

            screen = new dfhack_lua_viewscreen(L, idx);
        }

        lua_pop(L, 1);
    }
    else
        screen = Lua::CheckDFObject<df::viewscreen>(L, idx);

    return screen;
}

bool dfhack_lua_viewscreen::safe_call_lua(int (*pf)(lua_State *), int args, int rvs)
{
    CoreSuspendClaimer suspend;
    color_ostream_proxy out(Core::getInstance().getConsole());

    auto L = Lua::Core::State;
    lua_pushcfunction(L, pf);
    if (args > 0) lua_insert(L, -args-1);
    lua_pushlightuserdata(L, this);
    if (args > 0) lua_insert(L, -args-1);

    return Lua::Core::SafeCall(out, args+1, rvs);
}

dfhack_lua_viewscreen *dfhack_lua_viewscreen::get_self(lua_State *L)
{
    auto self = (dfhack_lua_viewscreen*)lua_touserdata(L, 1);
    lua_rawgetp(L, LUA_REGISTRYINDEX, self);
    if (!lua_istable(L, -1)) return NULL;
    return self;
}

int dfhack_lua_viewscreen::do_destroy(lua_State *L)
{
    auto self = get_self(L);
    if (!self) return 0;

    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, self);

    lua_pushnil(L);
    lua_rawsetp(L, -2, &DFHACK_LUA_VS_TOKEN);
    lua_pushnil(L);
    lua_setfield(L, -2, "_native");

    lua_getfield(L, -1, "onDestroy");
    if (lua_isnil(L, -1))
        return 0;

    lua_pushvalue(L, -2);
    lua_call(L, 1, 0);
    return 0;
}

void dfhack_lua_viewscreen::update_focus(lua_State *L, int idx)
{
    lua_getfield(L, idx, "text_input_mode");
    text_input_mode = lua_toboolean(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, idx, "focus_path");
    auto str = lua_tostring(L, -1);
    if (!str) str = "";
    focus = str;
    lua_pop(L, 1);

    if (focus.empty())
        focus = "lua";
    else
        focus = "lua/"+focus;
}

int dfhack_lua_viewscreen::do_render(lua_State *L)
{
    auto self = get_self(L);
    if (!self) return 0;

    lua_getfield(L, -1, "onRender");

    if (lua_isnil(L, -1))
    {
        Screen::clear();
        return 0;
    }

    lua_pushvalue(L, -2);
    lua_call(L, 1, 0);
    return 0;
}

int dfhack_lua_viewscreen::do_notify(lua_State *L)
{
    int args = lua_gettop(L);

    auto self = get_self(L);
    if (!self) return 0;

    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
    if (lua_isnil(L, -1))
        return 0;

    // self field args table fn -> table fn table args
    lua_replace(L, 1);
    lua_copy(L, -1, 2);
    lua_insert(L, 1);
    lua_call(L, args-1, 1);

    self->update_focus(L, 1);
    return 1;
}

int dfhack_lua_viewscreen::do_input(lua_State *L)
{
    auto self = get_self(L);
    if (!self) return 0;

    auto keys = (std::set<df::interface_key>*)lua_touserdata(L, 2);

    lua_getfield(L, -1, "onInput");

    if (lua_isnil(L, -1))
    {
        if (keys->count(interface_key::LEAVESCREEN))
            Screen::dismiss(self);

        return 0;
    }

    lua_pushvalue(L, -2);

    lua_createtable(L, 0, keys->size()+3);

    for (auto it = keys->begin(); it != keys->end(); ++it)
    {
        auto key = *it;

        if (auto name = enum_item_raw_key(key))
            lua_pushstring(L, name);
        else
            lua_pushinteger(L, key);

        lua_pushboolean(L, true);
        lua_rawset(L, -3);

        if (key >= interface_key::STRING_A000 &&
            key <= interface_key::STRING_A255)
        {
            lua_pushinteger(L, key - interface_key::STRING_A000);
            lua_setfield(L, -2, "_STRING");
        }
    }

    if (enabler && enabler->tracking_on)
    {
        if (enabler->mouse_lbut) {
            lua_pushboolean(L, true);
            lua_setfield(L, -2, "_MOUSE_L");
        }
        if (enabler->mouse_rbut) {
            lua_pushboolean(L, true);
            lua_setfield(L, -2, "_MOUSE_R");
        }
    }

    lua_call(L, 2, 0);
    self->update_focus(L, -1);
    return 0;
}

dfhack_lua_viewscreen::dfhack_lua_viewscreen(lua_State *L, int table_idx)
{
    assert(Lua::IsCoreContext(L));

    Lua::PushDFObject(L, (df::viewscreen*)this);
    lua_setfield(L, table_idx, "_native");
    lua_pushlightuserdata(L, this);
    lua_rawsetp(L, table_idx, &DFHACK_LUA_VS_TOKEN);

    lua_pushvalue(L, table_idx);
    lua_rawsetp(L, LUA_REGISTRYINDEX, this);

    update_focus(L, table_idx);
}

dfhack_lua_viewscreen::~dfhack_lua_viewscreen()
{
    safe_call_lua(do_destroy, 0, 0);
}

void dfhack_lua_viewscreen::render()
{
    if (Screen::isDismissed(this)) return;

    dfhack_viewscreen::render();

    safe_call_lua(do_render, 0, 0);
}

void dfhack_lua_viewscreen::logic()
{
    if (Screen::isDismissed(this)) return;

    dfhack_viewscreen::logic();

    lua_pushstring(Lua::Core::State, "onIdle");
    safe_call_lua(do_notify, 1, 0);
}

void dfhack_lua_viewscreen::help()
{
    if (Screen::isDismissed(this)) return;

    lua_pushstring(Lua::Core::State, "onHelp");
    safe_call_lua(do_notify, 1, 0);
}

void dfhack_lua_viewscreen::resize(int w, int h)
{
    if (Screen::isDismissed(this)) return;

    auto L = Lua::Core::State;
    lua_pushstring(L, "onResize");
    lua_pushinteger(L, w);
    lua_pushinteger(L, h);
    safe_call_lua(do_notify, 3, 0);
}

void dfhack_lua_viewscreen::feed(std::set<df::interface_key> *keys)
{
    if (Screen::isDismissed(this)) return;

    lua_pushlightuserdata(Lua::Core::State, keys);
    safe_call_lua(do_input, 1, 0);
}

void dfhack_lua_viewscreen::onShow()
{
    lua_pushstring(Lua::Core::State, "onShow");
    safe_call_lua(do_notify, 1, 0);
}

void dfhack_lua_viewscreen::onDismiss()
{
    lua_pushstring(Lua::Core::State, "onDismiss");
    safe_call_lua(do_notify, 1, 0);
}
