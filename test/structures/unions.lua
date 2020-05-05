local utils = require('utils')

function test.unit_action_fields()
    dfhack.with_temp_object(df.unit_action:new(), function(action)
        for k in pairs(action.data) do
            expect.eq(utils.addressof(action.data.raw_data), utils.addressof(action.data[k]),
                'address of ' .. k .. ' does not match')
        end
    end)
end

function test.unit_action_type()
    dfhack.with_temp_object(df.unit_action:new(), function(action)
        for k, v in ipairs(df.unit_action_type) do
            expect.true_(action.data[df.unit_action_type.attrs[k].tag])
            expect.true_(action.data[df.unit_action_type.attrs[v].tag])
        end
    end)
end
