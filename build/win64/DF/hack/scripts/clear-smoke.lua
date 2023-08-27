-- Removes all smoke from the map

--[====[

clear-smoke
===========

Removes all smoke from the map. Note that this can leak memory and should be
used sparingly.

]====]

function clearSmoke(flows)
    for i = #flows - 1, 0, -1 do
        if flows[i].type == df.flow_type.Smoke then
            flows:erase(i)
        end
    end
end

clearSmoke(df.global.flows)

for _, block in pairs(df.global.world.map.map_blocks) do
    clearSmoke(block.flows)
end
