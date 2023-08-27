-- Reverts a civil war state

local civ = df.historical_entity.find(df.global.plotinfo.civ_id)

local fixed = false
for _, entity in pairs(civ.relations.diplomacy) do
    if entity.group_id == civ.id and entity.relation > 0 then
        entity.relation = 0
        fixed = true
    end
end

if fixed then
    print("Civil war removed!")
else
    print("No civil war found.")
end
