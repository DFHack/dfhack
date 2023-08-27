-- Checks the validity of "other" vectors in the world global

--[====[

devel/check-other-ids
=====================

This script runs through all world.items.other and world.buildings.other vectors
and verifies that the items contained in them have the expected types.

]====]

local errors = 0
function err(msg)
    errors = errors + 1
    if errors < 1000 then
        dfhack.printerr(msg)
    elseif errors == 1000 then
        qerror('too many errors')
    end
end

function check_other_ids(field, id_enum, id_enum_attr, item_field_enum)
    local vectors = df.global.world[field].other
    for id, vec in pairs(vectors) do
        local correct_item_type = id_enum.attrs[id][id_enum_attr]
        if correct_item_type ~= -1 then
            correct_item_type = correct_item_type
            for index, item in pairs(vec) do
                if item:getType() ~= correct_item_type then
                    err(('world.%s.other.%s[%i]: incorrect type: expected %s, got %s'):format(
                        field, id, index, item_field_enum[correct_item_type], item_field_enum[item:getType()]
                    ))
                end
            end
        end
    end
end

check_other_ids('buildings', df.buildings_other_id, 'building', df.building_type)
check_other_ids('items', df.items_other_id, 'item', df.item_type)

if errors == 0 then
    print('passed')
else
    qerror('failed')
end
