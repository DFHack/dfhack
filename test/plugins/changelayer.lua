config.mode = 'fortress'
config.target = 'changelayer'

local dwarfmode = require('gui.dwarfmode')

function test.no_material_is_wrong_usage()
    local output, status = dfhack.run_command_silent('changelayer')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('specify a material', output)
end

function test.bad_material_is_failure()
    local output, status = dfhack.run_command_silent('changelayer', 'BOGUSMAT')
    expect.eq(CR_FAILURE, status)
    expect.str_find('No such material', output)
end

function test.no_cursor_is_failure()
    return dfhack.with_finalize(function()
        dwarfmode.clearCursorPos()
    end, function()
        dwarfmode.clearCursorPos()
        local output, status = dfhack.run_command_silent('changelayer',
            'GRANITE')
        expect.eq(CR_FAILURE, status)
        expect.str_find('No cursor', output)
    end)
end

function test.help_is_wrong_usage()
    local _, status = dfhack.run_command_silent('changelayer', '?')
    expect.eq(CR_WRONG_USAGE, status)
end
