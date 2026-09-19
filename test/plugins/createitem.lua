config.mode = 'fortress'
config.target = 'createitem'

local dwarfmode = require('gui.dwarfmode')

local function find_floor_pos()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        for x = 0, 15 do
            for y = 0, 15 do
                local tt = block.tiletype[x][y]
                local des = block.designation[x][y]
                local occ = block.occupancy[x][y]
                if df.tiletype.attrs[tt].shape == df.tiletype_shape.FLOOR
                        and not des.hidden and occ.building == 0 then
                    return block.map_pos.x + x, block.map_pos.y + y,
                           block.map_pos.z
                end
            end
        end
    end
end

local function set_cursor(x, y, z)
    dwarfmode.setCursorPos(xyz2pos(x, y, z))
end

local function clear_cursor()
    dwarfmode.clearCursorPos()
end

local function count_items_at(x, y, z)
    local n = 0
    for _, item in ipairs(df.global.world.items.other.IN_PLAY) do
        -- item.pos is unreliable for contained items; getPosition is not
        local ix, iy, iz = dfhack.items.getPosition(item)
        if ix == x and iy == y and iz == z then
            n = n + 1
        end
    end
    return n
end

function test.creates_item_at_cursor()
    local x, y, z = find_floor_pos()
    expect.ne(nil, x, 'test needs a revealed floor tile')
    local before = count_items_at(x, y, z)

    local created
    return dfhack.with_finalize(function()
        if created then dfhack.items.remove(created) end
        clear_cursor()
        dfhack.run_command_silent('createitem', 'floor')
    end, function()
        set_cursor(x, y, z)
        local _, status = dfhack.run_command_silent('createitem',
            'BOULDER', 'INORGANIC:GRANITE')
        expect.eq(CR_OK, status)
        expect.eq(before + 1, count_items_at(x, y, z))

        for _, item in ipairs(df.global.world.items.other.IN_PLAY) do
            local ix, iy, iz = dfhack.items.getPosition(item)
            if ix == x and iy == y and iz == z
                    and item:getType() == df.item_type.BOULDER then
                created = item
                break
            end
        end
        expect.ne(nil, created, 'created boulder should exist')
    end)
end

function test.floor_mode_sets_destination()
    local output, status = dfhack.run_command_silent('createitem', 'floor')
    expect.eq(CR_OK, status)
    expect.str_find('placed on the floor', output)
end

function test.no_args_is_wrong_usage()
    local _, status = dfhack.run_command_silent('createitem')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.bad_item_type_is_failure()
    local output, status = dfhack.run_command_silent('createitem',
        'BOGUS', 'INORGANIC:GRANITE')
    expect.eq(CR_FAILURE, status)
    expect.str_find('valid item type', output)
end

function test.bad_material_is_failure()
    local output, status = dfhack.run_command_silent('createitem',
        'BOULDER', 'BOGUSMAT')
    expect.eq(CR_FAILURE, status)
    expect.str_find('Unrecognized material', output)
end
