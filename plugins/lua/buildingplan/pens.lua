local _ENV = mkmodule('plugins.buildingplan.pens')

GOOD_TILE_PEN, BAD_TILE_PEN = nil, nil
VERT_TOP_PEN, VERT_MID_PEN, VERT_BOT_PEN = nil, nil, nil
HORI_LEFT_PEN, HORI_MID_PEN, HORI_RIGHT_PEN = nil, nil, nil
BUTTON_START_PEN, BUTTON_END_PEN = nil, nil
SELECTED_ITEM_PEN = nil
MINI_TEXT_PEN, MINI_TEXT_HPEN, MINI_BUTT_PEN, MINI_BUTT_HPEN = nil, nil, nil, nil

local to_pen = dfhack.pen.parse

local tp = function(asset, offset)
    local texpos = dfhack.textures.getAsset(asset, offset)
    if texpos == -1 then return nil end
    return texpos
end

function reload_pens()
    GOOD_TILE_PEN = to_pen{ch='o', fg=COLOR_GREEN, tile=dfhack.screen.findGraphicsTile('CURSORS', 1, 2)}
    BAD_TILE_PEN = to_pen{ch='X', fg=COLOR_RED, tile=dfhack.screen.findGraphicsTile('CURSORS', 3, 0)}

    local tb = "hack/data/art/border-thin.png"
    VERT_TOP_PEN = to_pen { tile = tp(tb, 10), ch = 194, fg = COLOR_GREY, bg = COLOR_BLACK }
    VERT_MID_PEN = to_pen { tile = tp(tb, 4), ch = 179, fg = COLOR_GREY, bg = COLOR_BLACK }
    VERT_BOT_PEN = to_pen { tile = tp(tb, 11), ch = 193, fg = COLOR_GREY, bg = COLOR_BLACK }

    local mb = "hack/data/art/border-medium.png"
    HORI_LEFT_PEN = to_pen { tile = tp(mb, 12), ch = 195, fg = COLOR_GREY, bg = COLOR_BLACK }
    HORI_MID_PEN = to_pen { tile = tp(mb, 5), ch = 196, fg = COLOR_GREY, bg = COLOR_BLACK }
    HORI_RIGHT_PEN = to_pen { tile = tp(mb, 13), ch = 180, fg = COLOR_GREY, bg = COLOR_BLACK }

    local cp = "hack/data/art/control-panel.png"
    BUTTON_START_PEN = to_pen { tile = tp(cp, 13), ch = '[', fg = COLOR_YELLOW }
    BUTTON_END_PEN = to_pen { tile = tp(cp, 15), ch = ']', fg = COLOR_YELLOW }
    SELECTED_ITEM_PEN = to_pen { tile = tp(cp, 9), ch = string.char(251), fg = COLOR_YELLOW }

    MINI_TEXT_PEN = to_pen{fg=COLOR_BLACK, bg=COLOR_GREY}
    MINI_TEXT_HPEN = to_pen{fg=COLOR_BLACK, bg=COLOR_WHITE}
    MINI_BUTT_PEN = to_pen{fg=COLOR_BLACK, bg=COLOR_LIGHTRED}
    MINI_BUTT_HPEN = to_pen{fg=COLOR_WHITE, bg=COLOR_RED}
end
reload_pens()

return _ENV
