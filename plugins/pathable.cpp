#include "Debug.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/Screen.h"
#include "modules/Textures.h"

#include "df/init.h"
#include "df/map_block.h"
#include "df/tile_designation.h"

#include <functional>

using namespace DFHack;

DFHACK_PLUGIN("pathable");

REQUIRE_GLOBAL(init);
REQUIRE_GLOBAL(window_x);
REQUIRE_GLOBAL(window_y);
REQUIRE_GLOBAL(window_z);

namespace DFHack {
    DBG_DECLARE(pathable, log, DebugCategory::LINFO);
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return CR_OK;
}

static void paintScreenPathable(df::coord target, bool show_hidden = false) {
    DEBUG(log).print("entering paintScreen\n");

    bool use_graphics = Screen::inGraphicsMode();

    int selected_tile_texpos = 0;
    Screen::findGraphicsTile("CURSORS", 4, 3, &selected_tile_texpos);

    long pathable_tile_texpos = init->load_bar_texpos[1];
    long unpathable_tile_texpos = init->load_bar_texpos[4];
    long on_off_texpos = Textures::getMapPathableTexposStart();
    if (on_off_texpos > 0) {
        pathable_tile_texpos = on_off_texpos + 0;
        unpathable_tile_texpos = on_off_texpos + 1;
    }

    auto dims = Gui::getDwarfmodeViewDims().map();
    for (int y = dims.first.y; y <= dims.second.y; ++y) {
        for (int x = dims.first.x; x <= dims.second.x; ++x) {
            df::coord map_pos(*window_x + x, *window_y + y, *window_z);

            if (!Maps::isValidTilePos(map_pos))
                continue;

            // don't overwrite the target tile
            if (!use_graphics && map_pos == target) {
                TRACE(log).print("skipping target tile\n");
                continue;
            }

            if (!show_hidden && !Maps::isTileVisible(map_pos)) {
                TRACE(log).print("skipping hidden tile\n");
                continue;
            }

            DEBUG(log).print("scanning map tile at offset %d, %d\n", x, y);
            Screen::Pen cur_tile = Screen::readTile(x, y, true);
            DEBUG(log).print("tile data: ch=%d, fg=%d, bg=%d, bold=%s\n",
                    cur_tile.ch, cur_tile.fg, cur_tile.bg, cur_tile.bold ? "true" : "false");
            DEBUG(log).print("tile data: tile=%d, tile_mode=%d, tile_fg=%d, tile_bg=%d\n",
                    cur_tile.tile, cur_tile.tile_mode, cur_tile.tile_fg, cur_tile.tile_bg);

            if (!cur_tile.valid()) {
                DEBUG(log).print("cannot read tile at offset %d, %d\n", x, y);
                continue;
            }

            bool can_walk = Maps::canWalkBetween(target, map_pos);
            DEBUG(log).print("tile is %swalkable at offset %d, %d\n",
                             can_walk ? "" : "not ", x, y);

            if (use_graphics) {
                if (map_pos == target) {
                    cur_tile.tile = selected_tile_texpos;
                } else{
                    cur_tile.tile = can_walk ?
                            pathable_tile_texpos : unpathable_tile_texpos;
                }
            } else {
                int color = can_walk ? COLOR_GREEN : COLOR_RED;
                if (cur_tile.fg && cur_tile.ch != ' ') {
                    cur_tile.fg = color;
                    cur_tile.bg = 0;
                } else {
                    cur_tile.fg = 0;
                    cur_tile.bg = color;
                }

                cur_tile.bold = false;

                if (cur_tile.tile)
                    cur_tile.tile_mode = Screen::Pen::CharColor;
            }

            Screen::paintTile(cur_tile, x, y, true);
        }
    }
}

static bool is_warm(const df::coord &pos) {
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return false;
    return block->temperature_1[pos.x&15][pos.y&15] >= 10075;
}

static bool is_rough_wall(int16_t x, int16_t y, int16_t z) {
    df::tiletype *tt = Maps::getTileType(x, y, z);
    if (!tt)
        return false;

    return tileShape(*tt) == df::tiletype_shape::WALL &&
        tileSpecial(*tt) != df::tiletype_special::SMOOTH;
}

static bool will_leak(int16_t x, int16_t y, int16_t z) {
    auto des = Maps::getTileDesignation(x, y, z);
    if (!des)
        return false;
    if (des->bits.liquid_type == df::tile_liquid::Water && des->bits.flow_size >= 1)
        return true;
    if (des->bits.water_table && is_rough_wall(x, y, z))
        return true;
    return false;
}

static bool is_damp(const df::coord &pos) {
    return will_leak(pos.x-1, pos.y-1, pos.z) ||
        will_leak(pos.x, pos.y-1, pos.z) ||
        will_leak(pos.x+1, pos.y-1, pos.z) ||
        will_leak(pos.x-1, pos.y, pos.z) ||
        will_leak(pos.x+1, pos.y, pos.z) ||
        will_leak(pos.x-1, pos.y+1, pos.z) ||
        will_leak(pos.x, pos.y+1, pos.z) ||
        will_leak(pos.x+1, pos.y+1, pos.z);
        will_leak(pos.x, pos.y+1, pos.z+1);
}

static void paintScreenWarmDamp(bool show_hidden = false) {
    DEBUG(log).print("entering paintScreenDampWarm\n");

    if (Screen::inGraphicsMode())
        return;

    auto dims = Gui::getDwarfmodeViewDims().map();
    for (int y = dims.first.y; y <= dims.second.y; ++y) {
        for (int x = dims.first.x; x <= dims.second.x; ++x) {
            df::coord map_pos(*window_x + x, *window_y + y, *window_z);

            if (!Maps::isValidTilePos(map_pos))
                continue;

            if (!show_hidden && !Maps::isTileVisible(map_pos)) {
                TRACE(log).print("skipping hidden tile\n");
                continue;
            }

            TRACE(log).print("scanning map tile at (%d, %d, %d) screen offset (%d, %d)\n",
                map_pos.x, map_pos.y, map_pos.z, x, y);

            Screen::Pen cur_tile = Screen::readTile(x, y, true);
            if (!cur_tile.valid()) {
                DEBUG(log).print("cannot read tile at offset %d, %d\n", x, y);
                continue;
            }

            int color = is_warm(map_pos) ? COLOR_RED : is_damp(map_pos) ? COLOR_BLUE : COLOR_BLACK;
            if (color == COLOR_BLACK) {
                TRACE(log).print("skipping non-warm, non-damp tile\n");
                continue;
            }

            // this will also change the color of the cursor or any selection box that overlaps
            // the tile. this is intentional since it makes the UI more readable
            if (cur_tile.fg && cur_tile.ch != ' ') {
                cur_tile.fg = color;
                cur_tile.bg = 0;
            } else {
                cur_tile.fg = 0;
                cur_tile.bg = color;
            }

            cur_tile.bold = false;

            if (cur_tile.tile)
                cur_tile.tile_mode = Screen::Pen::CharColor;

            Screen::paintTile(cur_tile, x, y, true);
        }
    }
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(paintScreenPathable),
    DFHACK_LUA_FUNCTION(paintScreenWarmDamp),
    DFHACK_LUA_END
};
