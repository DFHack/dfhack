config.target = 'core'

-- nil "base" pen

function test.make_base_nil()
    local pen = dfhack.pen.make(nil)
    expect.nil_(pen)
end

function test.make_base_empty()
    local pen = dfhack.pen.make{}
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.make_base_nil_fg()
    local pen = dfhack.pen.make(nil, COLOR_RED)
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)

    pen = dfhack.pen.make(nil, COLOR_LIGHTRED)
    expect.eq(pen.fg, COLOR_RED)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.make_base_fg_nil_bg()
    local pen = dfhack.pen.make(nil, nil, COLOR_BLUE)
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
end

function test.make_base_fg_bg_nil_bold()
    local pen = dfhack.pen.make(nil, nil, nil, true)
    expect.eq(pen.fg, COLOR_GREY)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)

    pen = dfhack.pen.make(nil, nil, nil, false)
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
end

-- color number "base" pen

function test.make_base_color()
    local pen = dfhack.pen.make(COLOR_RED)
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)

    pen = dfhack.pen.make(COLOR_LIGHTRED)
    expect.eq(pen.fg, COLOR_RED)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.make_base_color_fg()
    local pen = dfhack.pen.make(COLOR_RED, COLOR_GREEN)
    expect.eq(pen.fg, COLOR_GREEN)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)

    pen = dfhack.pen.make(COLOR_RED, COLOR_LIGHTGREEN)
    expect.eq(pen.fg, COLOR_GREEN)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.make_base_color_fg_bg()
    local pen = dfhack.pen.make(COLOR_RED, COLOR_GREEN, COLOR_BLUE)
    expect.eq(pen.fg, COLOR_GREEN)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)

    pen = dfhack.pen.make(COLOR_RED, COLOR_LIGHTGREEN, COLOR_BLUE)
    expect.eq(pen.fg, COLOR_GREEN)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
end

function test.make_base_color_fg_bg_bold()
    local pen = dfhack.pen.make(COLOR_RED, COLOR_GREEN, COLOR_BLUE, true)
    expect.eq(pen.fg, COLOR_GREEN)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)

    -- maybe a bit unexpected? bold=true does not mask off "light" bit in fg
    pen = dfhack.pen.make(COLOR_RED, COLOR_LIGHTGREEN, COLOR_BLUE, true)
    expect.eq(pen.fg, COLOR_LIGHTGREEN)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
end

function test.make_base_color_fg_bg_nonbold()
    local pen = dfhack.pen.make(COLOR_RED, COLOR_GREEN, COLOR_BLUE, false)
    expect.eq(pen.fg, COLOR_GREEN)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)

    -- maybe a bit unexpected? bold=false does not mask off "light" bit in fg
    pen = dfhack.pen.make(COLOR_RED, COLOR_LIGHTGREEN, COLOR_BLUE, false)
    expect.eq(pen.fg, COLOR_LIGHTGREEN)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
end

-- table "base" pen

function test.make_base_table_fg()
    local pen = dfhack.pen.make{ fg = COLOR_RED }
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
    expect.false_(pen.tile_color)

    pen = dfhack.pen.make{ fg = COLOR_LIGHTRED }
    expect.eq(pen.fg, COLOR_RED)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
    expect.false_(pen.tile_color)

    pen = dfhack.pen.make{ fg = COLOR_LIGHTRED, tile_color = true }
    expect.eq(pen.fg, COLOR_RED)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
    expect.true_(pen.tile_color)
end

function test.make_base_table_bg()
    local pen = dfhack.pen.make{ bg = COLOR_BLUE }
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    expect.false_(pen.tile_color)

    pen = dfhack.pen.make{ bg = COLOR_LIGHTBLUE, tile_color = true }
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_LIGHTBLUE)
    expect.true_(pen.tile_color)
end

function test.make_base_table_fg_bold()
    local pen = dfhack.pen.make{ fg = COLOR_RED, bold = true }
    expect.eq(pen.fg, COLOR_RED)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)

    -- maybe a bit unexpected? bold=true does not mask off "light" bit in fg
    pen = dfhack.pen.make{ fg = COLOR_LIGHTRED, bold = true }
    expect.eq(pen.fg, COLOR_LIGHTRED)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.make_base_table_fg_nonbold()
    local pen = dfhack.pen.make{ fg = COLOR_RED, bold = false }
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)

    -- maybe a bit unexpected? bold=false does not mask off "light" bit in fg
    pen = dfhack.pen.make{ fg = COLOR_LIGHTRED, bold = false }
    expect.eq(pen.fg, COLOR_LIGHTRED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
end

-- first pen is ignored if second is a table or a Pen

function test.make_base_table_pen()
    local pen = dfhack.pen.make({ fg = COLOR_LIGHTRED, bg = COLOR_BLUE }, { fg = COLOR_GREEN, bg = COLOR_BROWN })
    expect.eq(pen.fg, COLOR_GREEN)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BROWN)

    local pen2 = dfhack.pen.make{ fg = COLOR_GREEN, bg = COLOR_BROWN }
    pen = dfhack.pen.make({ fg = COLOR_LIGHTRED, bg = COLOR_BLUE }, pen2)
    expect.eq(pen.fg, COLOR_GREEN)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BROWN)
end

function test.make_base_pen_pen()
    local base = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE }
    local pen_table = { fg = COLOR_LIGHTGREEN, bg = COLOR_BROWN }
    local pen = dfhack.pen.make(base, pen_table)
    -- base is unchanged
    expect.eq(base.fg, COLOR_RED)
    expect.false_(base.bold)
    expect.eq(base.bg, COLOR_BLUE)
    -- pen has new values
    expect.eq(pen.fg, COLOR_GREEN)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BROWN)

    pen = dfhack.pen.make(base, dfhack.pen.make(pen_table))
    expect.eq(pen.fg, COLOR_GREEN)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BROWN)
end

-- tile_fg, tile_bg

function test.make_base_table_tile_fg()
    local pen = dfhack.pen.make{ tile_fg = COLOR_RED }
    expect.eq(pen.tile_fg, COLOR_RED)
    expect.eq(pen.tile_bg, COLOR_BLACK)
    expect.nil_(pen.tile_color)
end

function test.make_base_table_tile_bg()
    local pen = dfhack.pen.make{ tile_bg = COLOR_BLUE }
    expect.eq(pen.tile_fg, COLOR_GREY)
    expect.eq(pen.tile_bg, COLOR_BLUE)
    expect.nil_(pen.tile_color)
end

-- assign fields

function test.assign_fg()
    local pen = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE }
    pen.fg = COLOR_GREEN
    expect.eq(pen.fg, COLOR_GREEN)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)

    -- assigning a "light" fg does not change bold
    pen = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE }
    pen.fg = COLOR_LIGHTGREEN
    expect.eq(pen.fg, COLOR_LIGHTGREEN)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
end

function test.assign_bg()
    local pen = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE }
    pen.bg = COLOR_GREEN
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_GREEN)
end

function test.assign_bold()
    local pen = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE }
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    pen.bold = true
    expect.eq(pen.fg, COLOR_RED)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)

    pen = dfhack.pen.make{ fg = COLOR_LIGHTRED, bg = COLOR_BLUE }
    expect.eq(pen.fg, COLOR_RED)
    expect.true_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    pen.bold = false
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
end

function test.assign_tile_fg()
    local pen = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE, tile_color = true }
    pen.tile_fg = COLOR_GREEN
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    expect.eq(pen.tile_fg, COLOR_GREEN)
    expect.eq(pen.tile_bg, COLOR_BLACK)
    expect.nil_(pen.tile_color)

    pen = dfhack.pen.make{ tile_fg = COLOR_RED, tile_bg = COLOR_BLUE, tile_color = true }
    pen.tile_fg = COLOR_GREEN
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
    expect.eq(pen.tile_fg, COLOR_GREEN)
    expect.eq(pen.tile_bg, COLOR_BLUE)
    expect.nil_(pen.tile_color)
end

function test.assign_tile_bg()
    local pen = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE, tile_color = true }
    pen.tile_bg = COLOR_GREEN
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    expect.eq(pen.tile_fg, COLOR_GREY)
    expect.eq(pen.tile_bg, COLOR_GREEN)
    expect.nil_(pen.tile_color)

    pen = dfhack.pen.make{ tile_fg = COLOR_RED, tile_bg = COLOR_BLUE, tile_color = true }
    pen.tile_bg = COLOR_GREEN
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
    expect.eq(pen.tile_fg, COLOR_RED)
    expect.eq(pen.tile_bg, COLOR_GREEN)
    expect.nil_(pen.tile_color)
end

function test.assign_tile_color()
    local tile = { tile_fg = COLOR_RED, tile_bg = COLOR_BLUE }
    local pen = dfhack.pen.make(tile)
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
    expect.eq(pen.tile_fg, COLOR_RED)
    expect.eq(pen.tile_bg, COLOR_BLUE)
    expect.nil_(pen.tile_color)

    pen.tile_color = true
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
    expect.nil_(pen.tile_fg)
    expect.nil_(pen.tile_bg)
    expect.true_(pen.tile_color)


    pen = dfhack.pen.make(tile)
    pen.tile_color = false
    expect.eq(pen.fg, COLOR_GREY)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLACK)
    expect.nil_(pen.tile_fg)
    expect.nil_(pen.tile_bg)
    expect.false_(pen.tile_color)


    pen = dfhack.pen.make({ fg = COLOR_RED, bg = COLOR_BLUE, tile_color = true })
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    expect.nil_(pen.tile_fg)
    expect.nil_(pen.tile_bg)
    expect.true_(pen.tile_color)

    pen.tile_color = false
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    expect.nil_(pen.tile_fg)
    expect.nil_(pen.tile_bg)
    expect.false_(pen.tile_color)


    pen = dfhack.pen.make({ fg = COLOR_RED, bg = COLOR_BLUE, tile_color = false })
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    expect.nil_(pen.tile_fg)
    expect.nil_(pen.tile_bg)
    expect.false_(pen.tile_color)

    pen.tile_color = true
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    expect.nil_(pen.tile_fg)
    expect.nil_(pen.tile_bg)
    expect.true_(pen.tile_color)


    -- a bit odd w.r.t usual Lua semantics: assigning nil to tile_color works like false
    pen = dfhack.pen.make({ fg = COLOR_RED, bg = COLOR_BLUE, tile_color = true })
    pen.tile_color = nil
    expect.eq(pen.fg, COLOR_RED)
    expect.false_(pen.bold)
    expect.eq(pen.bg, COLOR_BLUE)
    expect.nil_(pen.tile_fg)
    expect.nil_(pen.tile_bg)
    expect.false_(pen.tile_color)
end

-- COLOR_RESET should translate to "default" colors

function test.make_base_reset()
    local pen = dfhack.pen.make(COLOR_RESET)
    expect.eq(pen.fg, COLOR_GREY)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.make_pen_reset()
    local base = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE }
    local pen = dfhack.pen.make(base, COLOR_RESET)
    expect.eq(pen.fg, COLOR_GREY)
    expect.eq(pen.bg, COLOR_BLUE)
end

function test.make_bg_reset()
    local base = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE }
    local pen = dfhack.pen.make(base, nil, COLOR_RESET)
    expect.eq(pen.fg, COLOR_RED)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.make_pen_table_fg_reset()
    local pen = dfhack.pen.make{ fg = COLOR_RESET }
    expect.eq(pen.fg, COLOR_GREY)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.make_pen_table_bg_reset()
    local pen = dfhack.pen.make{ bg = COLOR_RESET }
    expect.eq(pen.fg, COLOR_GREY)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.assign_fg_reset()
    local pen = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE }
    pen.fg = COLOR_RESET
    expect.eq(pen.fg, COLOR_GREY)
    expect.eq(pen.bg, COLOR_BLUE)
end

function test.assign_bg_reset()
    local pen = dfhack.pen.make{ fg = COLOR_RED, bg = COLOR_BLUE }
    pen.bg = COLOR_RESET
    expect.eq(pen.fg, COLOR_RED)
    expect.eq(pen.bg, COLOR_BLACK)
end

function test.make_pen_table_tile_fg_reset()
    local pen = dfhack.pen.make{ tile_fg = COLOR_RESET }
    expect.eq(pen.tile_fg, COLOR_GREY)
    expect.eq(pen.tile_bg, COLOR_BLACK)
end

function test.make_pen_table_tile_bg_reset()
    local pen = dfhack.pen.make{ tile_bg = COLOR_RESET }
    expect.eq(pen.tile_fg, COLOR_GREY)
    expect.eq(pen.tile_bg, COLOR_BLACK)
end

function test.assign_tile_fg_reset()
    local pen = dfhack.pen.make{ tile_fg = COLOR_RED, tile_bg = COLOR_BLUE }
    pen.tile_fg = COLOR_RESET
    expect.eq(pen.tile_fg, COLOR_GREY)
    expect.eq(pen.tile_bg, COLOR_BLUE)
end

function test.assign_tile_bg_reset()
    local pen = dfhack.pen.make{ tile_fg = COLOR_RED, tile_bg = COLOR_BLUE }
    pen.tile_bg = COLOR_RESET
    expect.eq(pen.tile_fg, COLOR_RED)
    expect.eq(pen.tile_bg, COLOR_BLACK)
end
