#include <bitset>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ColorText.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "df/graphic_viewportst.h"
#include "df/world.h"
#include "modules/Gui.h"
#include "modules/Persistence.h"
#include "modules/Screen.h"
#include "modules/World.h"

DFHACK_PLUGIN("design");
using DFHack::color_value;

REQUIRE_GLOBAL(window_x);
REQUIRE_GLOBAL(window_y);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(plotinfo);
using namespace DFHack;
using namespace df::enums;

namespace DFHack {
DBG_DECLARE(design, log, DebugCategory::LINFO);
}

DFhackCExport command_result plugin_init(color_ostream &out,
                                         std::vector<PluginCommand> &commands) {
    return CR_OK;
}

std::map<uint32_t, Screen::Pen> PENS;

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        DEBUG(log,out).print("clearing PENS\n");
        PENS.clear();
    }
    return CR_OK;
}

struct DrawingPoint {
    uint32_t penKey = 0;
    std::pair<int, int> cursor_coords;

    DrawingPoint() : penKey(0), cursor_coords({-1, -1}) {}
};

typedef std::map<int, std::map<int, DrawingPoint>> ShapeMap;
ShapeMap arr;

bool has_point(int x, int y) {
    return arr.count(x) != 0 && arr.at(x).count(y) != 0;
};

// Key tuple is N, W, E, S
typedef std::tuple<bool, bool, bool, bool> DirectionKey;
std::map<DirectionKey, std::pair<int, int>> CURSORS_MAP = {
    {{false, false, false, false}, {1, 2}},  // INSIDE
    {{true, true, false, false}, {0, 1}},    // NW
    {{true, false, false, false}, {1, 1}},   // NORTH
    {{true, false, true, false}, {2, 1}},    // NE
    {{false, true, false, false}, {0, 2}},   // WEST
    {{false, false, true, false}, {2, 2}},   // EAST
    {{false, true, false, true}, {0, 3}},    // SW
    {{false, false, false, true}, {1, 3}},   // SOUTH
    {{false, false, true, true}, {2, 3}},    // SE
    {{true, true, true, false}, {3, 2}},     // N_NUB
    {{true, false, true, true}, {5, 1}},     // E_NUB
    {{true, true, false, true}, {3, 1}},     // W_NUB
    {{false, true, true, true}, {4, 2}},     // S_NUB
    {{false, true, true, false}, {3, 3}},    // VERT_NS
    {{true, false, false, true}, {4, 1}},    // VERT_EW
    {{true, true, true, true}, {4, 3}},      // POINT
};

enum PenMask {
    North = 0,
    South,
    East,
    West,
    DragPoint,
    MouseOver,
    InShape,
    ExtraPoint,
    NumFlags
};

uint32_t gen_pen_key(bool n, bool s, bool e, bool w, bool is_drag_point,
                     bool is_mouse_over, bool inshape, bool extra_point) {
    std::bitset<static_cast<size_t>(PenMask::NumFlags)> ret;
    ret[PenMask::North] = n;
    ret[PenMask::South] = s;
    ret[PenMask::East] = e;
    ret[PenMask::West] = w;
    ret[PenMask::DragPoint] = is_drag_point;
    ret[PenMask::MouseOver] = is_mouse_over;
    ret[PenMask::InShape] = inshape;
    ret[PenMask::ExtraPoint] = extra_point;

    return ret.to_ulong();
}

Screen::Pen make_pen(const std::pair<int, int> &direction, bool is_drag_point,
                     bool is_mouse_over, bool inshape, bool extra_point) {
    color_value color = COLOR_GREEN;
    int ycursor_mod = 0;

    if (!extra_point) {
        if (is_drag_point) {
            color = COLOR_CYAN;
            ycursor_mod += 6;
            if (is_mouse_over) {
                color = COLOR_MAGENTA;
                ycursor_mod += 3;
            }
        }
    } else {
        ycursor_mod += 15;
        color = COLOR_LIGHTRED;

        if (is_mouse_over) {
            color = COLOR_RED;
            ycursor_mod += 3;
        }
    }

    Screen::Pen pen;
    pen.ch = inshape ? 'X' : 'o';
    pen.fg = color;
    int selected_tile_texpos = 0;
    Screen::findGraphicsTile("CURSORS", direction.first,
                             direction.second + ycursor_mod,
                             &selected_tile_texpos);
    pen.tile = selected_tile_texpos;

    return pen;
}

Screen::Pen get_pen(int x, int y, ShapeMap &arr, const std::string &type = "") {
    bool n = false, w = false, e = false, s = false;
    if (has_point(x, y)) {
        if (y == 0 || !has_point(x, y - 1)) n = true;
        if (x == 0 || !has_point(x - 1, y)) w = true;
        if (!has_point(x + 1, y)) e = true;  // TODO check map size
        if (!has_point(x, y + 1)) s = true;  // TODO check map size
    }

    bool is_drag_point = type == "drag_point";
    bool is_extra = type == "extra_point";
    bool is_in_shape = has_point(x, y);
    auto mouse_pos = Gui::getMousePos();
    bool mouse_over = mouse_pos.x == x && mouse_pos.y == y;

    uint32_t pen_key = gen_pen_key(n, s, e, w, is_drag_point, mouse_over,
                                   is_in_shape, is_extra);

    if (CURSORS_MAP.count({n, w, e, s}) > 0 && has_point(x, y)) {
        arr[x][y].cursor_coords = CURSORS_MAP.at({n, w, e, s});
    }

    if (PENS.find(pen_key) == PENS.end()) {
        std::pair<int, int> cursor{-1, -1};

        if (type != "") {
            return make_pen(CURSORS_MAP.at({n, w, e, s}), is_drag_point,
                            mouse_over, is_in_shape, is_extra);
        }

        if (CURSORS_MAP.count({n, w, e, s}) > 0) {
            PENS.emplace(pen_key,
                         make_pen(CURSORS_MAP.at({n, w, e, s}), is_drag_point,
                                  mouse_over, is_in_shape, is_extra));
            if (type == "" && has_point(x, y)) {
                arr[x][y].penKey = pen_key;
            }
        }
    }

    return PENS.at(pen_key);
}

static int design_load_shape(lua_State *L) {
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            int x = lua_tointeger(L, -2);

            if (lua_istable(L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    int y = lua_tointeger(L, -2);
                    bool value = lua_toboolean(L, -1);

                    if (value) {
                        arr[x][y] = DrawingPoint();
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }
    }

    return 0;
}

static int design_clear_shape(lua_State *L) {
    arr.clear();

    return 0;
}

static int design_draw_shape(lua_State *L) {
    if (arr.size() == 0) {
        design_load_shape(L);
    }

    for (auto x : arr) {
        for (auto y : x.second) {
            Screen::Pen cur_tile = Screen::readTile(x.first, y.first, true);
            Screen::Pen pen = get_pen(x.first, y.first, arr);
            cur_tile.tile = pen.tile;
            Screen::paintTile(cur_tile, x.first - *window_x,
                              y.first - *window_y, true);
        }
    }

    return 0;
}

static int design_draw_points(lua_State *L) {
    if (lua_istable(L, -1)) {
        const char *str;
        lua_rawgeti(L, -1, 2);
        str = lua_tostring(L, -1);
        lua_pop(L, 1);

        lua_rawgeti(L, -1, 1);
        int n = luaL_len(L, -1);
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, -1, i);
            int x, y;
            lua_getfield(L, -1, "y");
            y = lua_tointeger(L, -1);
            lua_getfield(L, -2, "x");
            x = lua_tointeger(L, -1);
            lua_pop(L, 3);

            Screen::Pen cur_tile = Screen::readTile(x, y, true);
            Screen::Pen pen = get_pen(x, y, arr, str);
            cur_tile.tile = pen.tile;
            Screen::paintTile(cur_tile, x - *window_x, y - *window_y, true);
        }
        lua_pop(L, 1);
    }

    return 0;
}

DFHACK_PLUGIN_LUA_COMMANDS{DFHACK_LUA_COMMAND(design_draw_shape),
                           DFHACK_LUA_COMMAND(design_draw_points),
                           DFHACK_LUA_COMMAND(design_clear_shape),
                           DFHACK_LUA_END};
