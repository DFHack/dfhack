config.target = 'core'

local utils = require 'utils'

function test.OrderedTable()
    local t = utils.OrderedTable()
    local keys = {'a', 'c', 'e', 'd', 'b', 'q', 58, -1.2}
    for i = 1, #keys do
        t[keys[i]] = i
    end
    local i = 1
    for k, v in pairs(t) do
        expect.eq(k, keys[i], 'key order')
        expect.eq(v, i, 'correct value')
        i = i + 1
    end
end

function test.normalizePath()
    expect.eq('imapath/file.csv', utils.normalizePath('imapath/file.csv'))
    expect.eq('/ima/path', utils.normalizePath('/ima/path'))
    expect.eq('ima/path', utils.normalizePath('ima//path'))

    expect.eq('imapath', utils.normalizePath('imapath'))
    expect.eq('/imapath', utils.normalizePath('/imapath'))
    expect.eq('/imapath', utils.normalizePath('//imapath'))
    expect.eq('/imapath', utils.normalizePath('///imapath'))

    expect.eq('imapath/', utils.normalizePath('imapath/'))
    expect.eq('imapath/', utils.normalizePath('imapath//'))
end

function test.invert()
    local t = {}
    local i = utils.invert{'a', 4.4, false, true, 5, t}
    expect.eq(i.a, 1)
    expect.eq(i[4.4], 2)
    expect.eq(i[false], 3)
    expect.eq(i[true], 4)
    expect.eq(i[5], 5)
    expect.eq(i[t], 6)
    expect.eq(i[700], nil)
    expect.eq(i.foo, nil)
    expect.eq(i[{}], nil)
end

function test.invert_nil()
    local i = utils.invert{'a', nil, 'b'}
    expect.eq(i.a, 1)
    expect.eq(i[nil], nil)
    expect.eq(i.b, 3)
end

function test.invert_overwrite()
    local i = utils.invert{'a', 'b', 'a'}
    expect.eq(i.b, 2)
    expect.eq(i.a, 3)
end

function test.df_expr_to_ref()
    -- userdata field
    expect.eq(utils.df_expr_to_ref('df.global.world.event.engravings'), df.global.world.event.engravings)
    expect.eq(utils.df_expr_to_ref('df.global.world.event.engravings'), df.global.world.event:_field('engravings'))
    -- primitive field
    expect.eq(utils.df_expr_to_ref('df.global.world.original_save_version'), df.global.world:_field('original_save_version'))
    -- table field
    expect.eq(utils.df_expr_to_ref('df.global.world'), df.global.world)
    expect.eq(utils.df_expr_to_ref('df.global'), df.global)
    -- table
    expect.eq(utils.df_expr_to_ref('df'), df)

    -- userdata object
    expect.eq(utils.df_expr_to_ref('scr'), dfhack.gui.getCurViewscreen())

    local fake_unit
    mock.patch(dfhack.gui, 'getSelectedUnit', function() return fake_unit end, function()
        -- lightuserdata field
        fake_unit = {
           null_field=df.NULL,
        }
        expect.eq(utils.df_expr_to_ref('unit'), fake_unit)
        expect.eq(utils.df_expr_to_ref('unit.null_field'), fake_unit.null_field)

        dfhack.with_temp_object(df.unit:new(), function(u)
            fake_unit = u

            -- userdata field
            expect.eq(utils.df_expr_to_ref('unit.name'), fake_unit.name)
            expect.eq(utils.df_expr_to_ref('unit.name'), fake_unit:_field('name'))

            -- primitive field
            expect.eq(utils.df_expr_to_ref('unit.profession'), fake_unit:_field('profession'))
        end)

        -- vector items
        dfhack.with_temp_object(df.new('ptr-vector'), function(vec)
            fake_unit = vec
            vec:insert('#', df.global.world)
            vec:insert('#', df.global.plotinfo)

            expect.eq(utils.df_expr_to_ref('unit'), vec)

            expect.eq(utils.df_expr_to_ref('unit[0]'), utils.df_expr_to_ref('unit.0'))
            expect.eq(df.reinterpret_cast(df.world, utils.df_expr_to_ref('unit[0]').value), df.global.world)

            expect.eq(utils.df_expr_to_ref('unit[1]'), utils.df_expr_to_ref('unit.1'))
            expect.eq(df.reinterpret_cast(df.plotinfost, utils.df_expr_to_ref('unit[1]').value), df.global.plotinfo)

            expect.error_match('index out of bounds', function() utils.df_expr_to_ref('unit.2') end)
            expect.error_match('index out of bounds', function() utils.df_expr_to_ref('unit[2]') end)
            expect.error_match('index out of bounds', function() utils.df_expr_to_ref('unit.-1') end)
            expect.error_match('index out of bounds', function() utils.df_expr_to_ref('unit[-1]') end)

            expect.error_match('not found', function() utils.df_expr_to_ref('unit.a') end)
        end)
    end)
end
