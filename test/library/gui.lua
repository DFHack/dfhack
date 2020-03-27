local gui = require('gui')

function test.getKeyDisplay()
    expect.eq(gui.getKeyDisplay(df.interface_key.CUSTOM_A), 'a')
    expect.eq(gui.getKeyDisplay('CUSTOM_A'), 'a')
    expect.eq(gui.getKeyDisplay(df.interface_key._first_item - 1), '?')
    expect.eq(gui.getKeyDisplay(df.interface_key._last_item + 1), '?')
    expect.eq(gui.getKeyDisplay(df.interface_key.KEYBINDING_COMPLETE), '?')
end

function test.clear_pen()
    expect.table_eq(gui.CLEAR_PEN, {
        ch = string.byte(' '),
        fg = COLOR_BLACK,
        bg = COLOR_BLACK,
        bold = false,
        tile_color = false,
    })
end
