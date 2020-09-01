function test.get()
    dfhack.with_temp_object(df.unit:new(), function(unit)
        expect.eq(unit:_field('hist_figure_id').ref_target, df.historical_figure)
    end)
end

function test.get_nil()
    dfhack.with_temp_object(df.coord:new(), function(coord)
        expect.nil_(coord:_field('x').ref_target)
    end)
end

function test.get_non_primitive()
    dfhack.with_temp_object(df.unit:new(), function(unit)
        expect.error_match(function()
            return unit:_field('status').ref_target
        end, 'not found')
    end)
end

function test.set()
    dfhack.with_temp_object(df.unit:new(), function(unit)
        expect.error_match(function()
            unit:_field('hist_figure_id').ref_target = df.coord
        end, 'builtin property or method')
    end)
end

function test.set_non_primitive()
    dfhack.with_temp_object(df.unit:new(), function(unit)
        expect.error_match(function()
            unit:_field('status').ref_target = df.coord
        end, 'not found')
    end)
end
