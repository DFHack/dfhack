-- Allow arena creature spawn after a mode change

for key, _ in pairs(df.global.world.arena.race) do
    df.global.world.arena.race[key] = nil
end

for key, _ in pairs(df.global.world.arena.caste) do
    df.global.world.arena.caste[key] = nil
end

for creature_index, _ in pairs(df.global.world.raws.creatures.all) do
    for caste_index, _ in pairs(df.global.world.raws.creatures.all[creature_index].caste) do
        df.global.world.arena.race:insert('#', creature_index)
        df.global.world.arena.caste:insert('#', caste_index)
    end
end

df.global.world.arena.creature_cnt[#df.global.world.arena.race - 1] = 0
