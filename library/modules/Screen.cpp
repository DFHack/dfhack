/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include <set>
using namespace std;

#include "modules/ImTuiImpl.h"
#include "modules/Renderer.h"
#include "modules/Screen.h"
#include "modules/GuiHooks.h"
#include "Debug.h"
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
#include "df/texture_handlerst.h"
#include "df/tile_pagest.h"
#include "df/interfacest.h"
#include "df/enabler.h"
#include "df/graphic_viewportst.h"
#include "df/unit.h"
#include "df/item.h"
#include "df/job.h"
#include "df/building.h"
#include "df/renderer.h"
#include "df/plant.h"

using namespace df::enums;
using df::global::init;
using df::global::gps;
using df::global::texture;
using df::global::gview;
using df::global::enabler;

using Screen::Pen;
using Screen::PenArray;

using std::string;

namespace DFHack {
    DBG_DECLARE(core, screen, DebugCategory::LINFO);
}


/*
 * Screen painting API.
 */

// returns ui grid coordinates, even if the game map is scaled differently
df::coord2d Screen::getMousePos()
{
    if (!gps)
        return df::coord2d(-1, -1);
    return df::coord2d(gps->mouse_x, gps->mouse_y);
}

// returns the screen pixel coordinates
df::coord2d Screen::getMousePixels()
{
    if (!gps)
        return df::coord2d(-1, -1);
    return df::coord2d(gps->precise_mouse_x, gps->precise_mouse_y);
}

df::coord2d Screen::getWindowSize()
{
    if (!gps) return df::coord2d(80, 25);

    return df::coord2d(gps->dimx, gps->dimy);
}

void Screen::zoom(df::zoom_commands cmd) {
    enabler->zoom_display(cmd);
}

bool Screen::inGraphicsMode()
{
    return init && init->display.flag.is_set(init_display_flags::USE_GRAPHICS);
}

static bool doSetTile_map(const Pen &pen, int x, int y) {
    auto &vp = gps->main_viewport;

    if (x < 0 || x >= vp->dim_x || y < 0 || y >= vp->dim_y)
        return false;

    size_t max_index = vp->dim_x * vp->dim_y - 1;
    size_t index = (x * vp->dim_y) + y;

    if (index > max_index)
        return false;

    long texpos = pen.tile;
    if (!texpos && pen.ch)
        texpos = init->font.large_font_texpos[(uint8_t)pen.ch];
    if (texpos)
        vp->screentexpos_interface[index] = texpos;
    return true;
}

static bool doSetTile_default(const Pen &pen, int x, int y, bool map)
{
    bool use_graphics = Screen::inGraphicsMode();

    if (map && use_graphics)
        return doSetTile_map(pen, x, y);

    if (x < 0 || x >= gps->dimx || y < 0 || y >= gps->dimy)
        return false;

    size_t index = (x * gps->dimy) + y;
    uint8_t *screen = &gps->screen[index * 8];

    if (screen > gps->screen_limit)
        return false;

    long *texpos = &gps->screentexpos[index];
    long *texpos_lower = &gps->screentexpos_lower[index];
    uint32_t *flag = &gps->screentexpos_flag[index];

    // keep SCREENTEXPOS_FLAG_ANCHOR_SUBORDINATE so occluded anchored textures
    // don't appear corrupted
    uint32_t flag_mask = 0x4;
    if (pen.write_to_lower)
        flag_mask |= 0x18;

    *screen = 0;
    *texpos = 0;
    if (!pen.keep_lower)
        *texpos_lower = 0;
    gps->screentexpos_anchored[index] = 0;
    *flag &= flag_mask;

    if (gps->top_in_use) {
        screen = &gps->screen_top[index * 8];
        texpos = &gps->screentexpos_top[index];
        texpos_lower = &gps->screentexpos_top_lower[index];
        flag = &gps->screentexpos_top_flag[index];

        *screen = 0;
        *texpos = 0;
        if (!pen.keep_lower)
            *texpos_lower = 0;
        gps->screentexpos_top_anchored[index] = 0;
        *flag &= flag_mask;
    }

    uint8_t fg = pen.fg | (pen.bold << 3);
    uint8_t bg = pen.bg;

    if (pen.tile_mode == Screen::Pen::CharColor)
        *flag |= 2; // SCREENTEXPOS_FLAG_ADDCOLOR
    else if (pen.tile_mode == Screen::Pen::TileColor) {
        *flag |= 1; // SCREENTEXPOS_FLAG_GRAYSCALE
        if (pen.tile_fg)
            fg = pen.tile_fg;
        if (pen.tile_bg)
            bg = pen.tile_bg;
    }

    if (pen.tile && use_graphics) {
        if (pen.write_to_lower)
            *texpos_lower = pen.tile;
        else
            *texpos = pen.tile;

        if (pen.top_of_text || pen.bottom_of_text) {
            screen[0] = uint8_t(pen.ch);
            if (pen.top_of_text)
                *flag |= 0x8;
            if (pen.bottom_of_text)
                *flag |= 0x10;
        }
    } else if (pen.ch) {
        screen[0] = uint8_t(pen.ch);
        *texpos_lower = 909; // basic black background
    }

    auto rgb_fg = &gps->uccolor[fg][0];
    auto rgb_bg = &gps->uccolor[bg][0];
    screen[1] = rgb_fg[0];
    screen[2] = rgb_fg[1];
    screen[3] = rgb_fg[2];
    screen[4] = rgb_bg[0];
    screen[5] = rgb_bg[1];
    screen[6] = rgb_bg[2];

    return true;
}

GUI_HOOK_DEFINE(Screen::Hooks::set_tile, doSetTile_default);
static bool doSetTile(const Pen &pen, int x, int y, bool map)
{
    return GUI_HOOK_TOP(Screen::Hooks::set_tile)(pen, x, y, map);
}

bool Screen::paintTile(const Pen &pen, int x, int y, bool map)
{
    if (!gps || !pen.valid()) return false;

    doSetTile(pen, x, y, map);
    return true;
}

static Pen doGetTile_map(int x, int y) {
    auto &vp = gps->main_viewport;

    if (x < 0 || x >= vp->dim_x || y < 0 || y >= vp->dim_y)
        return Pen(0, 0, 0, -1);

    size_t max_index = vp->dim_x * vp->dim_y - 1;
    size_t index = (x * vp->dim_y) + y;

    if (index < 0 || index > max_index)
        return Pen(0, 0, 0, -1);

    int tile = vp->screentexpos[index];
    if (tile == 0)
        tile = vp->screentexpos_item[index];
    if (tile == 0)
        tile = vp->screentexpos_building_one[index];
    if (tile == 0)
        tile = vp->screentexpos_background_two[index];
    if (tile == 0)
        tile = vp->screentexpos_background[index];

    char ch = 0;
    uint8_t fg = 0;
    uint8_t bg = 0;
    return Pen(ch, fg, bg, tile, false);
}

static uint8_t to_16_bit_color(uint8_t  *rgb) {
    for (uint8_t c = 0; c < 16; ++c) {
        if (rgb[0] == gps->uccolor[c][0] &&
                rgb[1] == gps->uccolor[c][1] &&
                rgb[2] == gps->uccolor[c][2]) {
            return c;
        }
    }
    return 0;
}

static Pen doGetTile_default(int x, int y, bool map) {
    bool use_graphics = Screen::inGraphicsMode();

    if (map && use_graphics)
        return doGetTile_map(x, y);

    if (x < 0 || x >= gps->dimx || y < 0 || y >= gps->dimy)
        return Pen(0, 0, 0, -1);

    size_t index = (x * gps->dimy) + y;
    uint8_t *screen = &gps->screen[index * 8];

    if (screen > gps->screen_limit)
        return Pen(0, 0, 0, -1);

    long *texpos = &gps->screentexpos[index];
    uint32_t *flag = &gps->screentexpos_flag[index];

    if (gps->top_in_use &&
            (gps->screen_top[index * 8] ||
             (use_graphics && gps->screentexpos_top[index]))) {
        screen = &gps->screen_top[index * 8];
        texpos = &gps->screentexpos_top[index];
        flag = &gps->screentexpos_top_flag[index];
    }

    char ch = *screen;
    uint8_t fg = to_16_bit_color(&screen[1]);
    uint8_t bg = to_16_bit_color(&screen[4]);
    int tile = 0;
    if (use_graphics)
        tile = *texpos;

    if (*flag & 1) {
        // TileColor
        return Pen(ch, fg&7, bg, !!(fg&8), tile, fg, bg);
    } else if (*flag & 2) {
        // CharColor
        return Pen(ch, fg, bg, tile, true);
    }

    // AsIs
    return Pen(ch, fg, bg, tile, false);
}

GUI_HOOK_DEFINE(Screen::Hooks::get_tile, doGetTile_default);
static Pen doGetTile(int x, int y, bool map)
{
    return GUI_HOOK_TOP(Screen::Hooks::get_tile)(x, y, map);
}

Pen Screen::readTile(int x, int y, bool map)
{
    if (!gps) return Pen(0,0,0,-1);

    return doGetTile(x, y, map);
}

bool Screen::paintString(const Pen &pen, int x, int y, const std::string &text, bool map)
{
    auto dim = getWindowSize();
    if (!gps || y < 0 || y >= dim.y) return false;

    Pen tmp(pen);
    bool ok = false;

    for (size_t i = -std::min(0,x); i < text.size(); i++)
    {
        if (x + i >= size_t(dim.x))
            break;

        tmp.ch = text[i];
        tmp.tile = (pen.tile ? pen.tile + uint8_t(text[i]) : 0);
        paintTile(tmp, x+i, y, map);
        ok = true;
    }

    return ok;
}

bool Screen::fillRect(const Pen &pen, int x1, int y1, int x2, int y2, bool map)
{
    auto dim = getWindowSize();
    if (!gps || !pen.valid()) return false;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= dim.x) x2 = dim.x-1;
    if (y2 >= dim.y) y2 = dim.y-1;
    if (x1 > x2 || y1 > y2) return false;

    for (int x = x1; x <= x2; x++)
    {
        for (int y = y1; y <= y2; y++)
            doSetTile(pen, x, y, map);
    }

    return true;
}

bool Screen::drawBorder(const std::string &title)
{
    if (!gps) return false;

    auto dim = getWindowSize();
    Pen border('\xDB', 8);
    Pen text(0, 0, 7);
    Pen signature(0, 0, 8);

    for (int x = 0; x < dim.x; x++)
    {
        doSetTile(border, x, 0, false);
        doSetTile(border, x, dim.y - 1, false);
    }
    for (int y = 0; y < dim.y; y++)
    {
        doSetTile(border, 0, y, false);
        doSetTile(border, dim.x - 1, y, false);
    }

    paintString(signature, dim.x-8, dim.y-1, "DFHack");

    return paintString(text, (dim.x - title.length()) / 2, 0, title);
}

bool Screen::clear()
{
    if (!gps) return false;

    auto dim = getWindowSize();
    return fillRect(Pen(' ',0,0,false), 0, 0, dim.x-1, dim.y-1);
}

bool Screen::invalidate()
{
    if (!enabler) return false;

    enabler->flag.bits.render = true;
    return true;
}

const Pen Screen::Painter::default_pen(0,COLOR_GREY,0);
const Pen Screen::Painter::default_key_pen(0,COLOR_LIGHTGREEN,0);

void Screen::Painter::do_paint_string(const std::string &str, const Pen &pen, bool map)
{
    if (gcursor.y < clip.first.y || gcursor.y > clip.second.y)
        return;

    int dx = std::max(0, int(clip.first.x - gcursor.x));
    int len = std::min((int)str.size(), int(clip.second.x - gcursor.x + 1));

    if (len > dx)
        paintString(pen, gcursor.x + dx, gcursor.y, str.substr(dx, len-dx), map);
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

static std::map<df::viewscreen*, Plugin*> plugin_screens;

bool Screen::show(std::unique_ptr<df::viewscreen> screen, df::viewscreen *before, Plugin *plugin)
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
    df::viewscreen* s = screen.release();
    parent->child = s;
    if (s->child)
        s->child->parent = s;

    if (dfhack_viewscreen::is_instance(s))
        static_cast<dfhack_viewscreen*>(s)->onShow();

    if (plugin)
        plugin_screens[s] = plugin;

    return true;
}

void Screen::dismiss(df::viewscreen *screen, bool to_first)
{
    CHECK_NULL_POINTER(screen);

    auto it = plugin_screens.find(screen);
    if (it != plugin_screens.end())
        plugin_screens.erase(it);

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

bool Screen::hasActiveScreens(Plugin *plugin)
{
    if (plugin_screens.empty())
        return false;
    df::viewscreen *screen = &gview->view;
    while (screen)
    {
        auto it = plugin_screens.find(screen);
        if (it != plugin_screens.end() && it->second == plugin)
            return true;
        screen = screen->child;
    }
    return false;
}

void Screen::raise(df::viewscreen *screen) {
    Hide swapper(screen, Screen::Hide::RESTORE_AT_TOP);
}

namespace DFHack { namespace Screen {

Hide::Hide(df::viewscreen* screen, int flags) :
    screen{screen}, prev_parent{nullptr}, flags{flags} {
    if (!screen)
        return;

    // don't extract a screen that's not even in the stack
    if (screen->parent)
        extract();
}

Hide::~Hide() {
    if (screen)
        merge();
}

void Hide::extract() {
    prev_parent = screen->parent;
    df::viewscreen *prev_child = screen->child;

    screen->parent = nullptr;
    screen->child = nullptr;

    prev_parent->child = prev_child;
    if (prev_child) prev_child->parent = prev_parent;
    else Core::getInstance().top_viewscreen = prev_parent;
}

void Hide::merge() {
    if (screen->parent) {
        // we're somehow back on the stack; do nothing
        return;
    }

    if (flags & RESTORE_AT_TOP) {
        Screen::show(std::unique_ptr<df::viewscreen>(screen));
        return;
    }

    df::viewscreen* new_child = prev_parent->child;

    prev_parent->child = screen;
    screen->child = new_child;
    if (new_child) new_child->parent = screen;
    else Core::getInstance().top_viewscreen = screen;
}
} }

string Screen::getKeyDisplay(df::interface_key key)
{
    if (enabler)
        return enabler->GetKeyDisplay(key);

    return "?";
}

int Screen::keyToChar(df::interface_key key)
{
    if (key < interface_key::STRING_A000 ||
        key > interface_key::STRING_A255)
        return -1;

    if (key < interface_key::STRING_A128)
        return key - interface_key::STRING_A000;

    return key - interface_key::STRING_A128 + 128;
}

df::interface_key Screen::charToKey(char code)
{
    int val = (unsigned char)code;
    if (val < 127)
        return df::interface_key(interface_key::STRING_A000 + val);
    else if (val == 127)
        return interface_key::NONE;
    else
        return df::interface_key(interface_key::STRING_A128 + (val-128));
}

/*
 * Pen array
 */

PenArray::PenArray(unsigned int bufwidth, unsigned int bufheight)
    :dimx(bufwidth), dimy(bufheight), static_alloc(false)
{
    buffer = new Pen[bufwidth * bufheight];
    clear();
}

PenArray::PenArray(unsigned int bufwidth, unsigned int bufheight, void *buf)
    :dimx(bufwidth), dimy(bufheight), static_alloc(true)
{
    buffer = (Pen*)((PenArray*)buf + 1);
    clear();
}

PenArray::~PenArray()
{
    if (!static_alloc)
        delete[] buffer;
}

void PenArray::clear()
{
    for (unsigned int x = 0; x < dimx; x++)
    {
        for (unsigned int y = 0; y < dimy; y++)
        {
            set_tile(x, y, Screen::Pen(0, 0, 0, 0, false));
        }
    }
}

Pen PenArray::get_tile(unsigned int x, unsigned int y)
{
    if (x < dimx && y < dimy)
        return buffer[(y * dimx) + x];
    return Pen(0, 0, 0, 0, false);
}

void PenArray::set_tile(unsigned int x, unsigned int y, Screen::Pen pen)
{
    if (x < dimx && y < dimy)
        buffer[(y * dimx) + x] = pen;
}

void PenArray::draw(unsigned int x, unsigned int y, unsigned int width, unsigned int height,
                    unsigned int bufx, unsigned int bufy)
{
    if (!gps)
        return;
    for (unsigned int gridx = x; gridx < x + width; gridx++)
    {
        for (unsigned int gridy = y; gridy < y + height; gridy++)
        {
            if (gridx >= unsigned(gps->dimx) ||
                gridy >= unsigned(gps->dimy) ||
                gridx - x + bufx >= dimx ||
                gridy - y + bufy >= dimy)
                continue;
            Screen::paintTile(buffer[((gridy - y + bufy) * dimx) + (gridx - x + bufx)], gridx, gridy);
        }
    }
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

dfhack_viewscreen *dfhack_viewscreen::try_cast(df::viewscreen *screen)
{
    return is_instance(screen) ? static_cast<dfhack_viewscreen*>(screen) : NULL;
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

    // if the DF screen immediately beneath the DFHack viewscreens is waiting to
    // be dismissed, raise it to the top so DF never gets stuck
    auto *p = parent;
    while (p) {
        bool is_df_screen = !is_instance(p);
        auto *next_p = p->parent;
        if (is_df_screen && Screen::isDismissed(p)) {
            DEBUG(screen).print("raising dismissed DF viewscreen %p\n", p);
            Screen::raise(p);
        }
        if (is_df_screen)
            break;
        p = next_p;
    }
}

void dfhack_viewscreen::render()
{
    check_resize();
}

bool dfhack_viewscreen::key_conflict(df::interface_key key)
{
    if (key == interface_key::OPTIONS)
        return true;

/* TODO: understand how this changes for v50
    if (text_input_mode)
    {
        if (key == interface_key::HELP || key == interface_key::MOVIES)
            return true;
    }
*/

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

    if (!Screen::isDismissed(self)) {
        WARN(screen).print("DFHack screen was destroyed before it was dismissed\n");
        WARN(screen).print("Please tell the DFHack team which DF screen you were just viewing\n");
        // run skipped onDismiss cleanup logic
        Screen::dismiss(self);
    }

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
    lua_getfield(L, idx, "allow_options");
    allow_options = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, idx, "defocused");
    defocused = lua_toboolean(L, -1);
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
    Lua::PushInterfaceKeys(L, *keys);

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

    ImTuiInterop::viewscreen::register_viewscreen(this);
}

dfhack_lua_viewscreen::~dfhack_lua_viewscreen()
{
    safe_call_lua(do_destroy, 0, 0);

    ImTuiInterop::viewscreen::unregister_viewscreen(this);
}

void dfhack_lua_viewscreen::render()
{
    if (Screen::isDismissed(this))
    {
        if (parent)
            parent->render();
        return;
    }

    int id = ImTuiInterop::viewscreen::on_render_start(this);

    dfhack_viewscreen::render();

    safe_call_lua(do_render, 0, 0);

    ImTuiInterop::viewscreen::on_render_end(this, id);
}

void dfhack_lua_viewscreen::logic()
{
    if (Screen::isDismissed(this)) return;

    dfhack_viewscreen::logic();

    lua_pushstring(Lua::Core::State, "onIdle");
    safe_call_lua(do_notify, 1, 0);
}

bool dfhack_lua_viewscreen::key_conflict(df::interface_key key)
{
    if (key == df::interface_key::OPTIONS)
        return !allow_options;
    return dfhack_viewscreen::key_conflict(key);
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

    ImTuiInterop::viewscreen::on_feed_start(this, keys);

    lua_pushlightuserdata(Lua::Core::State, keys);
    safe_call_lua(do_input, 1, 0);

    if(ImTuiInterop::viewscreen::on_feed_end(keys))
        parent->feed(keys);
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

    ImTuiInterop::viewscreen::on_dismiss();
}

df::unit *dfhack_lua_viewscreen::getSelectedUnit()
{
    Lua::StackUnwinder frame(Lua::Core::State);
    lua_pushstring(Lua::Core::State, "onGetSelectedUnit");
    safe_call_lua(do_notify, 1, 1);
    return Lua::GetDFObject<df::unit>(Lua::Core::State, -1);
}

df::item *dfhack_lua_viewscreen::getSelectedItem()
{
    Lua::StackUnwinder frame(Lua::Core::State);
    lua_pushstring(Lua::Core::State, "onGetSelectedItem");
    safe_call_lua(do_notify, 1, 1);
    return Lua::GetDFObject<df::item>(Lua::Core::State, -1);
}

df::job *dfhack_lua_viewscreen::getSelectedJob()
{
    Lua::StackUnwinder frame(Lua::Core::State);
    lua_pushstring(Lua::Core::State, "onGetSelectedJob");
    safe_call_lua(do_notify, 1, 1);
    return Lua::GetDFObject<df::job>(Lua::Core::State, -1);
}

df::building *dfhack_lua_viewscreen::getSelectedBuilding()
{
    Lua::StackUnwinder frame(Lua::Core::State);
    lua_pushstring(Lua::Core::State, "onGetSelectedBuilding");
    safe_call_lua(do_notify, 1, 1);
    return Lua::GetDFObject<df::building>(Lua::Core::State, -1);
}

df::plant *dfhack_lua_viewscreen::getSelectedPlant()
{
    Lua::StackUnwinder frame(Lua::Core::State);
    lua_pushstring(Lua::Core::State, "onGetSelectedPlant");
    safe_call_lua(do_notify, 1, 1);
    return Lua::GetDFObject<df::plant>(Lua::Core::State, -1);
}

#define STATIC_FIELDS_GROUP
#include "../DataStaticsFields.cpp"

using df::identity_traits;

#define CUR_STRUCT dfhack_viewscreen
static const struct_field_info dfhack_viewscreen_fields[] = {
    { METHOD(OBJ_METHOD, is_lua_screen), 0, 0 },
    { METHOD(OBJ_METHOD, isFocused), 0, 0 },
    { METHOD(OBJ_METHOD, getFocusString), 0, 0 },
    { METHOD(OBJ_METHOD, onShow), 0, 0 },
    { METHOD(OBJ_METHOD, onDismiss), 0, 0 },
    { METHOD(OBJ_METHOD, getSelectedUnit), 0, 0 },
    { METHOD(OBJ_METHOD, getSelectedItem), 0, 0 },
    { METHOD(OBJ_METHOD, getSelectedJob), 0, 0 },
    { METHOD(OBJ_METHOD, getSelectedBuilding), 0, 0 },
    { METHOD(OBJ_METHOD, getSelectedPlant), 0, 0 },
    { FLD_END }
};
#undef CUR_STRUCT
virtual_identity dfhack_viewscreen::_identity(sizeof(dfhack_viewscreen), nullptr, "dfhack_viewscreen", nullptr, &df::viewscreen::_identity, dfhack_viewscreen_fields);

#define CUR_STRUCT dfhack_lua_viewscreen
static const struct_field_info dfhack_lua_viewscreen_fields[] = {
    { FLD_END }
};
#undef CUR_STRUCT
virtual_identity dfhack_lua_viewscreen::_identity(sizeof(dfhack_lua_viewscreen), nullptr, "dfhack_lua_viewscreen", nullptr, &dfhack_viewscreen::_identity, dfhack_lua_viewscreen_fields);
