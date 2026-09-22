config.mode = 'fortress'
config.target = 'burrow'

local burrow = require('plugins.burrow')

local function with_burrow(fn, name)
    local burrows = df.global.plotinfo.burrows
    -- burrow name lookups return the first match, so remove any test burrows
    -- leaked by previous runs before creating ours
    name = name or 'DFHACK_TEST_BURROW'
    for i = #burrows.list - 1, 0, -1 do
        if burrows.list[i].name == name then
            dfhack.burrows.clearTiles(burrows.list[i])
            burrows.list[i]:delete()
            burrows.list:erase(i)
        end
    end
    local b = df.burrow:new()
    b.id = burrows.next_id
    burrows.next_id = burrows.next_id + 1
    b.name = name
    burrows.list:insert('#', b)
    return dfhack.with_finalize(
        function()
            for i = #burrows.list - 1, 0, -1 do
                if burrows.list[i] == b then
                    burrows.list:erase(i)
                end
            end
            dfhack.burrows.clearTiles(b)
            dfhack.burrows.clearUnits(b)
            b:delete()
        end,
        function() fn(b) end)
end

local function assigned(b, x, y, z)
    return dfhack.burrows.isAssignedTile(b, xyz2pos(x, y, z))
end

function test.tiles_box_add_and_remove()
    with_burrow(function(b)
        expect.true_(burrow.parse_commandline(
            'tiles', 'box-add', b.name, '10,10,120', '12,12,120'))
        expect.true_(assigned(b, 10, 10, 120))
        expect.true_(assigned(b, 12, 12, 120))
        expect.false_(assigned(b, 9, 10, 120))
        expect.false_(assigned(b, 10, 10, 119))

        expect.true_(burrow.parse_commandline(
            'tiles', 'box-remove', b.name, '11,11,120', '12,12,120'))
        expect.true_(assigned(b, 10, 10, 120))
        expect.false_(assigned(b, 12, 12, 120))
    end)
end

function test.tiles_clear()
    with_burrow(function(b)
        burrow.parse_commandline('tiles', 'box-add', b.name, '10,10,120', '12,12,120')
        expect.true_(assigned(b, 10, 10, 120))
        expect.true_(burrow.parse_commandline('tiles', 'clear', b.name))
        expect.false_(assigned(b, 10, 10, 120))
    end)
end

function test.tiles_add_copies_from_other_burrow()
    with_burrow(function(src)
        burrow.parse_commandline('tiles', 'box-add', src.name, '20,20,120', '21,21,120')
        with_burrow(function(dst)
            expect.true_(burrow.parse_commandline(
                'tiles', 'add', dst.name, src.name))
            expect.true_(assigned(dst, 20, 20, 120))
            expect.true_(assigned(src, 20, 20, 120))
        end, 'DFHACK_TEST_BURROW_DST')
    end, 'DFHACK_TEST_BURROW_SRC')
end

function test.units_add_copies_from_other_burrow()
    -- create a source burrow with a unit assigned directly
    with_burrow(function(src)
        local unit = dfhack.units.getCitizens()[1]
        if not unit then return end
        dfhack.burrows.setAssignedUnit(src, unit, true)
        expect.true_(dfhack.burrows.isAssignedUnit(src, unit))
        with_burrow(function(dst)
            expect.true_(burrow.parse_commandline(
                'units', 'add', dst.name, src.name))
            expect.true_(dfhack.burrows.isAssignedUnit(dst, unit))
            expect.true_(burrow.parse_commandline(
                'units', 'remove', dst.name, src.name))
            expect.false_(dfhack.burrows.isAssignedUnit(dst, unit))
            expect.true_(dfhack.burrows.isAssignedUnit(src, unit))
        end, 'DFHACK_TEST_BURROW_DST')
    end, 'DFHACK_TEST_BURROW_SRC')
end

function test.help_and_bad_mode_return_false()
    expect.false_(burrow.parse_commandline('help'))
    expect.false_(burrow.parse_commandline('--help'))
    expect.false_(burrow.parse_commandline('bogus', 'clear'))
    expect.false_(burrow.parse_commandline('tiles', 'bogus'))
end
