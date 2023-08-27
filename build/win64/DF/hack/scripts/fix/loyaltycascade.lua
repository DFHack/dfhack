-- Prevents a "loyalty cascade" (civil war) when a citizen is killed.

-- Checks if a unit is a former member of a given entity as well as it's
-- current enemy.
local function getUnitRenegade(unit, entity_id)
    local unit_entity_links = df.global.world.history.figures[unit.hist_figure_id].entity_links
    local former_index = nil
    local enemy_index = nil

    for index, link in pairs(unit_entity_links) do
        local link_type = link:getType()

        if link.entity_id ~= entity_id then
            goto skipentity
        end

        if link_type ==  df.histfig_entity_link_type.FORMER_MEMBER then
            former_index = index
        elseif link_type ==  df.histfig_entity_link_type.ENEMY then
            enemy_index = index
        end

        :: skipentity ::
    end

    return former_index, enemy_index
end

local function convertUnit(unit, entity_id, former_index, enemy_index)
    local unit_entity_links = df.global.world.history.figures[unit.hist_figure_id].entity_links

    unit_entity_links:erase(math.max(former_index, enemy_index))
    unit_entity_links:erase(math.min(former_index, enemy_index))

    -- Creates a new entity link to the player's civilization/group.
    unit_entity_links:insert('#', df.histfig_entity_link_memberst{
        entity_id = entity_id,
        link_strength = 100
    })
end

local function fixUnit(unit)
    local fixed = false

    if not dfhack.units.isOwnCiv(unit) or not dfhack.units.isDwarf(unit) then
        return fixed
    end

    local unit_name = dfhack.TranslateName(dfhack.units.getVisibleName(unit))
    local former_civ_index, enemy_civ_index = getUnitRenegade(unit, df.global.plotinfo.civ_id)
    local former_group_index, enemy_group_index = getUnitRenegade(unit, df.global.plotinfo.group_id)

    -- If the unit is a former member of your civilization, as well as now an
    -- enemy of it, we make it become a member again.
    if former_civ_index and enemy_civ_index then
        local civ_name = dfhack.TranslateName(df.global.world.entities.all[df.global.plotinfo.civ_id].name)

        convertUnit(unit, df.global.plotinfo.civ_id, former_civ_index, enemy_civ_index)

        dfhack.gui.showAnnouncement(([[loyaltycascade: %s is now a member of %s again]]):format(unit_name, civ_name), COLOR_WHITE)

        fixed = true
    end

    if former_group_index and enemy_group_index then
        local group_name = dfhack.TranslateName(df.global.world.entities.all[df.global.plotinfo.group_id].name)

        convertUnit(unit, df.global.plotinfo.group_id, former_group_index, enemy_group_index)

        dfhack.gui.showAnnouncement(([[loyaltycascade: %s is now a member of %s again]]):format(unit_name, group_name), COLOR_WHITE)

        fixed = true
    end

    if fixed and unit.enemy.enemy_status_slot ~= -1 then
        local status_cache = unit.enemy.enemy_status_cache
        local status_slot = unit.enemy.enemy_status_slot

        unit.enemy.enemy_status_slot = -1
        status_cache.slot_used[status_slot] = false

        for index, _ in pairs(status_cache.rel_map[status_slot]) do
            status_cache.rel_map[status_slot][index] = -1
        end

        for index, _ in pairs(status_cache.rel_map) do
            status_cache.rel_map[index] = -1
        end

        -- TODO: what if there were status slots taken above status_slot?
        -- does everything need to be moved down by one?
        if cache.next_slot > status_slot then
            cache.next_slot = status_slot
        end
    end

    return false
end

local count = 0
for _, unit in pairs(df.global.world.units.all) do
    if dfhack.units.isCitizen(unit) and fixUnit(unit) then
        count = count + 1
    end
end

if count > 0 then
    print(([[Fixed %s units from a loyalty cascade.]]):format(count))
else
    print("No loyalty cascade found.")
end
