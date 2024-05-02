local _ENV = mkmodule('plugins.buildingplan.pens')

local gui = require('gui')
local textures = require('gui.textures')

GOOD_TILE_PEN, BAD_TILE_PEN = nil, nil
SELECTED_ITEM_PEN = nil
MINI_TEXT_PEN, MINI_TEXT_HPEN, MINI_BUTT_PEN, MINI_BUTT_HPEN = nil, nil, nil, nil

local to_pen = dfhack.pen.parse

function reload_pens()
    GOOD_TILE_PEN = to_pen{ch='o', fg=COLOR_GREEN, tile=dfhack.screen.findGraphicsTile('CURSORS', 1, 2)}
    BAD_TILE_PEN = to_pen{ch='X', fg=COLOR_RED, tile=dfhack.screen.findGraphicsTile('CURSORS', 3, 0)}

    VERT_TOP_PEN = to_pen { tile = curry(textures.tp_border_thin, 11), ch = 194, fg = COLOR_GREY, bg = COLOR_BLACK }
    VERT_MID_PEN = to_pen { tile = curry(textures.tp_border_thin, 5), ch = 179, fg = COLOR_GREY, bg = COLOR_BLACK }
    VERT_BOT_PEN = to_pen { tile = curry(textures.tp_border_thin, 12), ch = 193, fg = COLOR_GREY, bg = COLOR_BLACK }

    HORI_LEFT_PEN = to_pen { tile = curry(textures.tp_border_medium, 13), ch = 195, fg = COLOR_GREY, bg = COLOR_BLACK }
    HORI_MID_PEN = to_pen { tile = curry(textures.tp_border_medium, 6), ch = 196, fg = COLOR_GREY, bg = COLOR_BLACK }
    HORI_RIGHT_PEN = to_pen { tile = curry(textures.tp_border_medium, 14), ch = 180, fg = COLOR_GREY, bg = COLOR_BLACK }

    BUTTON_START_PEN = to_pen { tile = curry(textures.tp_control_panel, 14), ch = '[', fg = COLOR_YELLOW }
    BUTTON_END_PEN = to_pen { tile = curry(textures.tp_control_panel, 16), ch = ']', fg = COLOR_YELLOW }
    SELECTED_ITEM_PEN = to_pen { tile = curry(textures.tp_control_panel, 10), ch = string.char(251), fg = COLOR_YELLOW }

    MINI_TEXT_PEN = to_pen{fg=COLOR_BLACK, bg=COLOR_GREY}
    MINI_TEXT_HPEN = to_pen{fg=COLOR_BLACK, bg=COLOR_WHITE}
    MINI_BUTT_PEN = to_pen{fg=COLOR_BLACK, bg=COLOR_LIGHTRED}
    MINI_BUTT_HPEN = to_pen{fg=COLOR_WHITE, bg=COLOR_RED}
end
reload_pens()

return _ENV
