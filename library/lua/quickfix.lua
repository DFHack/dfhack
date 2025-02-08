-- short-term urgent bugfixes

local _ENV = mkmodule("quickfix")

local function get_dwarf_race()
    for k,v in ipairs(df.global.world.raws.creatures.all) do
        -- dwarves are always the first entity race
        if v.flags.OCCURS_AS_ENTITY_RACE then return k end
    end
end

-- entities can send caravans, and if the race is -1, then DF crashes
-- called from onLoad.default.init
function set_entity_race_references()
    local race = get_dwarf_race()
    if not race then
        return
    end

    local count = 0
    for _,v in ipairs(df.global.world.entities.all) do
        if v.race == -1 then
            count = count + 1
            v.race = race
        end
    end
    print(('quickfix: fixed %d unset entity race reference(s)'):format(count))
end

return _ENV
