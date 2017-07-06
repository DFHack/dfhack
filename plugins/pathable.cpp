#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "DataFuncs.h"
#include "DataIdentity.h"
#include "Export.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/Screen.h"
#include "df/world.h"

using namespace DFHack;

DFHACK_PLUGIN("pathable");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(window_x);
REQUIRE_GLOBAL(window_y);
REQUIRE_GLOBAL(window_z);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands)
{
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    return CR_OK;
}

static void paintScreen(df::coord cursor, bool skip_unrevealed = false)
{
    auto dims = Gui::getDwarfmodeViewDims();
    for (int y = dims.map_y1; y <= dims.map_y2; y++)
    {
        for (int x = dims.map_x1; x <= dims.map_x2; x++)
        {
            Screen::Pen cur_tile = Screen::readTile(x, y, true);
            if (!cur_tile.valid())
                continue;

            df::coord map_pos(
                *window_x + x - dims.map_x1,
                *window_y + y - dims.map_y1,
                *window_z
            );

            // Keep yellow cursor
            if (map_pos == cursor)
                continue;

            if (map_pos.x < 0 || map_pos.x >= world->map.x_count ||
                map_pos.y < 0 || map_pos.y >= world->map.y_count ||
                map_pos.z < 0 || map_pos.z >= world->map.z_count)
            {
                continue;
            }

            if (skip_unrevealed && !Maps::isTileVisible(map_pos))
                continue;

            int color = Maps::canWalkBetween(cursor, map_pos) ? COLOR_GREEN : COLOR_RED;

            if (cur_tile.fg && cur_tile.ch != ' ')
            {
                cur_tile.fg = color;
                cur_tile.bg = 0;
            }
            else
            {
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
    DFHACK_LUA_FUNCTION(paintScreen),
    DFHACK_LUA_END
};

