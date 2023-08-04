config.target = 'core'

local utils = require('utils')

function test.unit_action_fields()
    dfhack.with_temp_object(df.unit_action:new(), function(action)
        for k in pairs(action.data) do
            expect.eq(utils.addressof(action.data.raw_data), utils.addressof(action.data:_field(k)),
                'address of ' .. k .. ' does not match')
        end
    end)
end

function test.unit_action_type()
    dfhack.with_temp_object(df.unit_action:new(), function(action)
        for index, name in ipairs(df.unit_action_type) do
            expect.true_(name, "unit_action_type entry without name: " .. tostring(index))
            local tag = df.unit_action_type.attrs[name].tag
            expect.true_(tag, "unit_action_type entry missing tag: name=" .. name)
            action.type = index
            expect.pairs_contains(action.data, tag,
                "unit_action_type entry missing from unit_action.data: name=" .. name)
        end
    end)
end
