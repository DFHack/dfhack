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
#include "modules/Persistence.h"
#include "modules/Screen.h"
#include "modules/World.h"

DFHACK_PLUGIN("design");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
using DFHack::color_value;

REQUIRE_GLOBAL(window_x);
REQUIRE_GLOBAL(window_y);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(plotinfo);
using namespace DFHack;
using namespace df::enums;

enum ConfigValues {
  CONFIG_IS_ENABLED = 0,
};
namespace DFHack {
// // for configuration-related logging
DBG_DECLARE(design, status, DebugCategory::LDEBUG);
// for logging during the periodic scan
DBG_DECLARE(design, cycle, DebugCategory::LDEBUG);
}  // namespace DFHack
static const std::string CONFIG_KEY = std::string(plugin_name) + "/config";
static PersistentDataItem config;
static const int32_t CYCLE_TICKS = 1200;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out,
                                 std::vector<std::string> &parameters);
static int32_t do_cycle(color_ostream &out, bool force_designate = false);

DFhackCExport command_result plugin_init(color_ostream &out,
                                         std::vector<PluginCommand> &commands) {
  DEBUG(status, out).print("initializing %s\n", plugin_name);

  // provide a configuration interface for the plugin
  commands.push_back(
      PluginCommand(plugin_name, "Designs stuff TBD", do_command));

  return CR_OK;
}

static int get_config_val(PersistentDataItem &c, int index) {
  if (!c.isValid()) return -1;
  return c.ival(index);
}

static bool get_config_bool(PersistentDataItem &c, int index) {
  return get_config_val(c, index) == 1;
}

static void set_config_val(PersistentDataItem &c, int index, int value) {
  if (c.isValid()) c.ival(index) = value;
}

static void set_config_bool(PersistentDataItem &c, int index, bool value) {
  set_config_val(c, index, value ? 1 : 0);
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
  if (!Core::getInstance().isWorldLoaded()) {
    out.printerr("Cannot enable %s without a loaded world.\n", plugin_name);

    return CR_FAILURE;
  }

  if (enable != is_enabled) {
    is_enabled = enable;
    DEBUG(status, out)
        .print("%s from the API; persisting\n",
               is_enabled ? "enabled" : "disabled");
    set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
    if (enable) do_cycle(out, true);
  } else {
    DEBUG(status, out)
        .print("%s from the API, but already %s; no action\n",
               is_enabled ? "enabled" : "disabled",
               is_enabled ? "enabled" : "disabled");
  }
  return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
  DEBUG(status, out).print("shutting down %s\n", plugin_name);

  return CR_OK;
}

DFhackCExport command_result plugin_load_data(color_ostream &out) {
  cycle_timestamp = 0;
  config = World::GetPersistentData(CONFIG_KEY);

  if (!config.isValid()) {
    DEBUG(status, out).print("no config found in this save; initializing\n");
    config = World::AddPersistentData(CONFIG_KEY);
    set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
  }

  // we have to copy our enabled flag into the global plugin variable, but
  // all the other state we can directly read/modify from the persistent
  // data structure.
  is_enabled = get_config_bool(config, CONFIG_IS_ENABLED);
  DEBUG(status, out)
      .print("loading persisted enabled state: %s\n",
             is_enabled ? "true" : "false");

  return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out,
                                                  state_change_event event) {
  if (event == DFHack::SC_WORLD_UNLOADED) {
    if (is_enabled) {
      DEBUG(status, out).print("world unloaded; disabling %s\n", plugin_name);
      is_enabled = false;
    }
  }

  return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
  if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS) {
    int32_t ret = do_cycle(out);
  }

  return CR_OK;
}
int selected_tile_texpos = 0;
const static bool hi =
    Screen::findGraphicsTile("CURSORS", 4, 3, &selected_tile_texpos);

static command_result do_command(color_ostream &out,
                                 std::vector<std::string> &parameters) {
  return CR_OK;
}

static int32_t do_cycle(color_ostream &out, bool force_designate) { return 0; }

// Assuming the existence of a class named Point, similar to the one in Lua
class Point {
 public:
  int x;
  int y;

  Point(int x, int y) : x(x), y(y) {}

  bool operator==(const Point &other) const {
    return x == other.x && y == other.y;
  }
};

// Assuming the existence of a class named Color, similar to the one in Lua
class Color {
 public:
  // Define your color values here
};

// Assuming the existence of a class named Pen, similar to the one in Lua
class Pen {
 public:
  std::string ch;
  int tile;
  color_value fg;
  // Define your pen properties and methods here
};

class Design {
 public:
  std::map<uint32_t, Pen> PENS;

  enum PEN_MASK {
    NORTH = 0,
    SOUTH,
    EAST,
    WEST,
    DRAG_POINT,
    MOUSEOVER,
    INSHAPE,
    EXTRA_POINT,
    NUM_FLAGS
  };

  // Define the function similar to the Lua version

  uint32_t gen_pen_key(bool n, bool s, bool e, bool w, bool is_corner,
                       bool is_mouse_over, bool inshape, bool extra_point) {
    std::bitset<NUM_FLAGS> ret;
    ret[NORTH] = n;
    ret[SOUTH] = s;
    ret[EAST] = e;
    ret[WEST] = w;
    ret[DRAG_POINT] = is_corner;
    ret[MOUSEOVER] = is_mouse_over;
    ret[INSHAPE] = inshape;
    ret[EXTRA_POINT] = extra_point;

    return static_cast<uint32_t>(ret.to_ulong());
  }

  Pen Design::get_pen(int x, int y,
                      const std::map<int, std::map<int, bool>> &arr);
  // Define the function similar to the Lua version
};

Design design;

// Add other methods and member variables needed for the class

static int design_getPen(lua_State *L) {
  std::map<int, std::map<int, bool>> arr;
  if (lua_istable(L, -1)) {
    // Iterate over the outer table
    lua_pushnil(L);  // First key
    while (lua_next(L, -2) != 0) {
      int x = lua_tointeger(L, -2);  // Convert key to an integer

      if (lua_istable(L, -1)) {
        // Iterate over the inner table
        lua_pushnil(L);  // First key
        while (lua_next(L, -2) != 0) {
          int y = lua_tointeger(L, -2);  // Convert key to an integer
          bool value = lua_toboolean(L, -1);

          if (value) {
            if (arr.count(x) == 0) arr[x] = {};
            arr[x][y] = value;
          }
          lua_pop(L, 1);  // Remove value, keep the key for the next iteration
        }
      }
      lua_pop(L, 1);  // Remove inner table, keep the key for the next iteration
    }
  }

  for (auto x : arr) {
    for (auto y : x.second) {
      Screen::Pen cur_tile = Screen::readTile(x.first, y.first, true);
      // cur_tile.tile = selected_tile_texpos;
      Pen pen = design.get_pen(x.first, y.first, arr);
      cur_tile.tile = pen.tile;
      Screen::paintTile(cur_tile, x.first - *window_x, y.first - *window_y,
                        true);
    }
  }
  return 0;
}
enum class CURSORS {
  INSIDE,
  NORTH,
  N_NUB,
  S_NUB,
  W_NUB,
  E_NUB,
  NE,
  NW,
  WEST,
  EAST,
  SW,
  SOUTH,
  SE,
  VERT_NS,
  VERT_EW,
  POINT
};

std::map<CURSORS, std::pair<int, int>> CURSORS_MAP = {
    {CURSORS::INSIDE, {1, 2}},  {CURSORS::NORTH, {1, 1}},
    {CURSORS::N_NUB, {3, 2}},   {CURSORS::S_NUB, {4, 2}},
    {CURSORS::W_NUB, {3, 1}},   {CURSORS::E_NUB, {5, 1}},
    {CURSORS::NE, {2, 1}},      {CURSORS::NW, {0, 1}},
    {CURSORS::WEST, {0, 2}},    {CURSORS::EAST, {2, 2}},
    {CURSORS::SW, {0, 3}},      {CURSORS::SOUTH, {1, 3}},
    {CURSORS::SE, {2, 3}},      {CURSORS::VERT_NS, {3, 3}},
    {CURSORS::VERT_EW, {4, 1}}, {CURSORS::POINT, {4, 3}},
};
Pen make_pen(const std::pair<int, int> &direction, bool is_corner,
             bool is_mouse_over, bool inshape, bool extra_point) {
  color_value color = COLOR_GREEN;
  int ycursor_mod = 0;

  if (!extra_point) {
    if (is_corner) {
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

  Pen pen;
  pen.ch = inshape ? "X" : "o";
  pen.fg = color;
  int selected_tile_texpos = 0;
  Screen::findGraphicsTile("CURSORS", direction.first, direction.second, &selected_tile_texpos);
  pen.tile = selected_tile_texpos;

  // Assuming dfhack.screen.findGraphicsTile is replaced with a custom function
  // findGraphicsTile pen.tile = findGraphicsTile("CURSORS", direction.first,
  // direction.second + ycursor_mod);

  return pen;
}
Pen Design::get_pen(int x, int y,
                    const std::map<int, std::map<int, bool>> &arr) {
  auto has_point = [&arr](int _x, int _y) {
    return arr.count(_x) != 0 && arr.at(_x).count(_y) != 0 && arr.at(_x).at(_y);
  };
  bool get_point = has_point(x, y);

  // Basic shapes are bounded by rectangles and therefore can have corner drag
  // points even if they're not real points in the shape if (marks.size() >=
  // shape.min_points && shape.basic_shape) {
  //     Point shape_top_left, shape_bot_right;
  //     shape.get_point_dims(shape_top_left, shape_bot_right);

  //     if (x == shape_top_left.x && y == shape_top_left.y &&
  //     shape.drag_corners.nw) {
  //         drag_point = true;
  //     } else if (x == shape_bot_right.x && y == shape_top_left.y &&
  //     shape.drag_corners.ne) {
  //         drag_point = true;
  //     } else if (x == shape_top_left.x && y == shape_bot_right.y &&
  //     shape.drag_corners.sw) {
  //         drag_point = true;
  //     } else if (x == shape_bot_right.x && y == shape_bot_right.y &&
  //     shape.drag_corners.se) {
  //         drag_point = true;
  //     }
  // }

  // for (const auto& mark : marks) {
  //     if (mark == Point(x, y)) {
  //         drag_point = true;
  //     }
  // }

  // if (mirror_point && *mirror_point == Point(x, y)) {
  //     drag_point = true;
  // }

  // // Check for an extra point
  // bool extra_point = false;
  // for (const auto& point : extra_points) {
  //     if (x == point.x && y == point.y) {
  //         extra_point = true;
  //         break;
  //     }
  // }

  // // Show center point if both marks are set
  // if ((shape.basic_shape && marks.size() == shape.max_points) ||
  //     (!shape.basic_shape && !placing_mark.active && !marks.empty())) {
  //     int center_x, center_y;
  //     shape.get_center(center_x, center_y);

  //     if (x == center_x && y == center_y) {
  //         extra_point = true;
  //     }
  // }

  bool n = false, w = false, e = false, s = false;
  if (get_point) {
    if (y == 0 || !has_point(x, y - 1)) n = true;
    if (x == 0 || !has_point(x - 1, y)) w = true;
    if (!has_point(x + 1, y)) e = true;
    if (!has_point(x, y + 1)) s = true;
  }
  // DEBUG(status).print("jcosker %d %d %d %d\n", n, s, e, w);

  // Get the bit field to use as a key for the PENS map
  uint32_t pen_key = gen_pen_key(n, s, e, w, false, false, get_point, false);
  // DEBUG(status).print("jcosker %zu\n", pen_key);

  if (PENS.find(pen_key) == PENS.end()) {
    std::pair<int, int> cursor{-1, -1};
    // int cursor = -1; // Assuming -1 is an invalid cursor value

    // Determine the cursor to use based on the input parameters
    // The CURSORS enum or equivalent should be defined in your code
    if (get_point && !n && !w && !e && !s)
      cursor = CURSORS_MAP.at(CURSORS::INSIDE);
    else if (get_point && n && w && !e && !s)
      cursor = CURSORS_MAP.at(CURSORS::NW);
    else if (get_point && n && !w && !e && !s)
      cursor = CURSORS_MAP.at(CURSORS::NORTH);
    else if (get_point && n && e && !w && !s)
      cursor = CURSORS_MAP.at(CURSORS::NE);
    else if (get_point && !n && w && !e && !s)
      cursor = CURSORS_MAP.at(CURSORS::WEST);
    else if (get_point && !n && !w && e && !s)
      cursor = CURSORS_MAP.at(CURSORS::EAST);
    else if (get_point && !n && w && !e && s)
      cursor = CURSORS_MAP.at(CURSORS::SW);
    else if (get_point && !n && !w && !e && s)
      cursor = CURSORS_MAP.at(CURSORS::SOUTH);
    else if (get_point && !n && !w && e && s)
      cursor = CURSORS_MAP.at(CURSORS::SE);
    else if (get_point && n && w && e && !s)
      cursor = CURSORS_MAP.at(CURSORS::N_NUB);
    else if (get_point && n && !w && e && s)
      cursor = CURSORS_MAP.at(CURSORS::E_NUB);
    else if (get_point && n && w && !e && s)
      cursor = CURSORS_MAP.at(CURSORS::W_NUB);
    else if (get_point && !n && w && e && s)
      cursor = CURSORS_MAP.at(CURSORS::S_NUB);
    else if (get_point && !n && w && e && !s)
      cursor = CURSORS_MAP.at(CURSORS::VERT_NS);
    else if (get_point && n && !w && !e && s)
      cursor = CURSORS_MAP.at(CURSORS::VERT_EW);
    else if (get_point && n && w && e && s)
      cursor = CURSORS_MAP.at(CURSORS::POINT);
    // else if (drag_point && !get_point) cursor = CURSORS::INSIDE;
    // else if (extra_point) cursor = CURSORS::INSIDE;
    // Create the pen if the cursor is set

    DEBUG(status).print("jcosker %d, %d\n", cursor.first, cursor.second);
    if (cursor.first != -1) {
      PENS[pen_key] = make_pen(cursor, false, false, get_point, false);
    }
  }

  // Return the pen for the caller
  return PENS.at(pen_key);
}

DFHACK_PLUGIN_LUA_COMMANDS{DFHACK_LUA_COMMAND(design_getPen), DFHACK_LUA_END};
