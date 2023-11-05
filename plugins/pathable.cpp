#include "Debug.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/Screen.h"
#include "modules/Textures.h"

#include "df/init.h"

using namespace DFHack;

DFHACK_PLUGIN("pathable");

REQUIRE_GLOBAL(init);
REQUIRE_GLOBAL(window_x);
REQUIRE_GLOBAL(window_y);
REQUIRE_GLOBAL(window_z);

namespace DFHack {
    DBG_DECLARE(pathable, log, DebugCategory::LINFO);
}

static std::vector<TexposHandle> textures;

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    textures = Textures::loadTileset("hack/data/art/pathable.png", 32, 32, true);
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
    long on_off_texpos = Textures::getTexposByHandle(textures[0]);
    if (on_off_texpos > 0) {
        pathable_tile_texpos = on_off_texpos;
        unpathable_tile_texpos = Textures::getTexposByHandle(textures[1]);
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

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(paintScreenPathable),
    DFHACK_LUA_END
};
