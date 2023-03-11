local _ENV = mkmodule('plugins.buildingplan.pens')

GOOD_TILE_PEN, BAD_TILE_PEN = nil, nil
VERT_TOP_PEN, VERT_MID_PEN, VERT_BOT_PEN = nil, nil, nil
BUTTON_START_PEN, BUTTON_END_PEN = nil, nil
SELECTED_ITEM_PEN = nil

local to_pen = dfhack.pen.parse

local tp = function(base, offset)
    if base == -1 then return nil end
    return base + offset
end

function reload_pens()
    GOOD_TILE_PEN = to_pen{ch='o', fg=COLOR_GREEN, tile=dfhack.screen.findGraphicsTile('CURSORS', 1, 2)}
    BAD_TILE_PEN = to_pen{ch='X', fg=COLOR_RED, tile=dfhack.screen.findGraphicsTile('CURSORS', 3, 0)}

    local tb_texpos = dfhack.textures.getThinBordersTexposStart()
    VERT_TOP_PEN = to_pen{tile=tp(tb_texpos, 10), ch=194, fg=COLOR_GREY, bg=COLOR_BLACK}
    VERT_MID_PEN = to_pen{tile=tp(tb_texpos, 4), ch=192, fg=COLOR_GREY, bg=COLOR_BLACK}
    VERT_BOT_PEN = to_pen{tile=tp(tb_texpos, 11), ch=179, fg=COLOR_GREY, bg=COLOR_BLACK}

    local cp_texpos = dfhack.textures.getControlPanelTexposStart()
    BUTTON_START_PEN = to_pen{tile=tp(cp_texpos, 13), ch='[', fg=COLOR_YELLOW}
    BUTTON_END_PEN = to_pen{tile=tp(cp_texpos, 15), ch=']', fg=COLOR_YELLOW}
    SELECTED_ITEM_PEN = to_pen{tile=tp(cp_texpos, 9), ch=string.char(251), fg=COLOR_YELLOW}
end
reload_pens()

return _ENV
