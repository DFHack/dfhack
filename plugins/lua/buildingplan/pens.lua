local _ENV = mkmodule('plugins.buildingplan.pens')

GOOD_TILE_PEN, BAD_TILE_PEN = nil, nil
SELECTED_ITEM_PEN = nil
MINI_TEXT_PEN, MINI_TEXT_HPEN, MINI_BUTT_PEN, MINI_BUTT_HPEN = nil, nil, nil, nil

local to_pen = dfhack.pen.parse

function reload_pens()
    GOOD_TILE_PEN = to_pen{ch='o', fg=COLOR_GREEN, tile=dfhack.screen.findGraphicsTile('CURSORS', 1, 2)}
    BAD_TILE_PEN = to_pen{ch='X', fg=COLOR_RED, tile=dfhack.screen.findGraphicsTile('CURSORS', 3, 0)}
    SELECTED_ITEM_PEN = to_pen{ch=string.char(251), fg=COLOR_YELLOW}
    MINI_TEXT_PEN = to_pen{fg=COLOR_BLACK, bg=COLOR_GREY}
    MINI_TEXT_HPEN = to_pen{fg=COLOR_BLACK, bg=COLOR_WHITE}
    MINI_BUTT_PEN = to_pen{fg=COLOR_BLACK, bg=COLOR_LIGHTRED}
    MINI_BUTT_HPEN = to_pen{fg=COLOR_WHITE, bg=COLOR_RED}
end
reload_pens()

return _ENV
