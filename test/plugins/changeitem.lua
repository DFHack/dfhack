config.mode = 'fortress'
config.target = 'changeitem'

local dwarfmode = require('gui.dwarfmode')

local function find_floor_item_pos()
    for _, item in ipairs(df.global.world.items.other.IN_PLAY) do
        if not item.flags.hidden and not item.flags.in_inventory
                and not item.flags.in_job and not item.flags.construction then
            local x, y, z = dfhack.items.getPosition(item)
            if x then return item, x, y, z end
        end
    end
end

local function set_cursor(x, y, z)
    dwarfmode.setCursorPos(xyz2pos(x, y, z))
end

local function clear_cursor()
    dwarfmode.clearCursorPos()
end

function test.here_quality_changes_item()
    local item, x, y, z = find_floor_item_pos()
    expect.ne(nil, item, 'test needs an item on the ground')
    local orig_quality = item.quality

    return dfhack.with_finalize(function()
        item.quality = orig_quality
        clear_cursor()
    end, function()
        set_cursor(x, y, z)
        local output, status = dfhack.run_command_silent('changeitem',
            'here', 'q', '4')
        expect.eq(CR_OK, status)
        expect.str_find('items processed', output)
        expect.eq(4, item.quality)
    end)
end

function test.here_empty_tile_processes_nothing()
    -- find a floor tile and put the cursor on it; if it happens to hold
    -- items the count is still reported
    local block = df.global.world.map.map_blocks[0]
    local x, y, z = block.map_pos.x, block.map_pos.y, block.map_pos.z

    return dfhack.with_finalize(clear_cursor, function()
        set_cursor(x, y, z)
        local output, status = dfhack.run_command_silent('changeitem', 'here')
        expect.eq(CR_OK, status)
        expect.str_find('items processed', output)
    end)
end

function test.here_no_cursor_is_failure()
    return dfhack.with_finalize(clear_cursor, function()
        clear_cursor()
        local output, status = dfhack.run_command_silent('changeitem', 'here')
        expect.eq(CR_FAILURE, status)
        expect.str_find('Cursor position not found', output)
    end)
end

function test.no_selection_is_failure()
    local output, status = dfhack.run_command_silent('changeitem')
    expect.eq(CR_FAILURE, status)
    expect.str_find('No item selected', output)
end

function test.material_missing_arg_is_wrong_usage()
    local output, status = dfhack.run_command_silent('changeitem', 'm')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('no material specified', output)
end

function test.bad_quality_is_wrong_usage()
    local _, status = dfhack.run_command_silent('changeitem', 'q', '9')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.bad_option_is_wrong_usage()
    local _, status = dfhack.run_command_silent('changeitem', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end
