-- show death cause of a creature
local utils = require('utils')
local DEATH_TYPES = reqscript('gui/unit-info-viewer').DEATH_TYPES

-- Creates a table of all items at the given location optionally matching a given item type
function getItemsAtPosition(pos_x, pos_y, pos_z, item_type)
    local items = {}
    for _, item in ipairs(df.global.world.items.all) do
        if item.pos.x == pos_x and item.pos.y == pos_y and item.pos.z == pos_z then
            if not item_type or item:getType() == item_type then
                table.insert(items, item)
            end
        end
    end
    return items
end

-- Finds a unit with the given id or nil if no unit is found
function findUnit(unit_id)
    if not unit_id or unit_id == -1 then
        return nil
    end

    return utils.binsearch(df.global.world.units.all, unit_id, 'id')
end

-- Find a histfig with the given id or nil if no unit is found
function findHistFig(histfig_id)
    if not histfig_id or histfig_id == -1 then
        return nil
    end

    return utils.binsearch(df.global.world.history.figures, histfig_id, 'id')
end

function getRace(race_id)
    return df.global.world.raws.creatures.all[race_id]
end

function getRaceNameSingular(race_id)
    return getRace(race_id).name[0]
end

function getDeathStringFromCause(cause)
    if cause == -1 then
        return "died"
    else
        return DEATH_TYPES[cause]:trim()
    end
end

function displayDeathUnit(unit)
    local str = ("The %s"):format(getRaceNameSingular(unit.race))
    if unit.name.has_name then
        str = str .. (" %s"):format(dfhack.TranslateName(unit.name))
    end

    if not unit.flags2.killed and not unit.flags3.ghostly then
        str = str .. " is not dead yet!"
        print(str)
        return
    end

    str = str .. (" %s"):format(getDeathStringFromCause(unit.counters.death_cause))

    local incident = df.incident.find(unit.counters.death_id)
    if incident then
        str = str .. (" in year %d"):format(incident.event_year)

        if incident.criminal then
            local killer = df.unit.find(incident.criminal)
            if killer then
                str = str .. (" killed by the %s"):format(getRaceNameSingular(killer.race))
                if killer.name.has_name then
                    str = str .. (" %s"):format(dfhack.TranslateName(killer.name))
                end
            end
        end
    end

    print(str .. '.')
end

-- returns the item description if the item still exists; otherwise
-- returns the weapon name
function getWeaponName(item_id, subtype)
    local item = df.item.find(item_id)
    if not item then
        return df.global.world.raws.itemdefs.weapons[subtype].name
    end
    return dfhack.items.getDescription(item, 0, false)
end

function displayDeathEventHistFigUnit(histfig_unit, event)
    local str = ("The %s %s %s in year %d"):format(
            getRaceNameSingular(histfig_unit.race),
            dfhack.TranslateName(histfig_unit.name),
            getDeathStringFromCause(event.death_cause),
            event.year
    )

    local slayer_histfig = findHistFig(event.slayer_hf)
    if slayer_histfig then
        str = str .. (", killed by the %s %s"):format(
                getRaceNameSingular(slayer_histfig.race),
                dfhack.TranslateName(slayer_histfig.name)
        )
    end

    if event.weapon then
        if event.weapon.item_type == df.item_type.WEAPON then
            str = str .. (", using a %s"):format(getWeaponName(event.weapon.item, event.weapon.item_subtype))
        elseif event.weapon.shooter_item_type == df.item_type.WEAPON then
            str = str .. (", shot by a %s"):format(getWeaponName(event.weapon.shooter_item, event.weapon.shooter_item_subtype))
        end
    end

    print(str .. '.')
end

-- Returns the death event for the given histfig or nil if not found
function getDeathEventForHistFig(histfig_id)
    for i = #df.global.world.history.events - 1, 0, -1 do
        local event = df.global.world.history.events[i]
        if event:getType() == df.history_event_type.HIST_FIGURE_DIED then
            if event.victim_hf == histfig_id then
                return event
            end
        end
    end

    return nil
end

function displayDeathHistFig(histfig)
    local histfig_unit = findUnit(histfig.unit_id)
    if not histfig_unit then
        qerror(("Failed to retrieve unit for histfig [histfig_id: %d, histfig_unit_id: %d"):format(
                histfig.id,
                tostring(histfig.unit_id)
        ))
    end

    if not histfig_unit.flags2.killed and not histfig_unit.flags3.ghostly then
        print(("%s is not dead yet!"):format(dfhack.TranslateName(histfig_unit.name)))
    else
        local death_event = getDeathEventForHistFig(histfig.id)
        displayDeathEventHistFigUnit(histfig_unit, death_event)
    end
end

local selected_item = dfhack.gui.getSelectedItem(true)
local selected_unit = dfhack.gui.getSelectedUnit(true)
local hist_figure_id

if not selected_unit and (not selected_item or selected_item:getType() ~= df.item_type.CORPSE) then
    -- if there isn't a selected unit and we don't have a selected item or the selected item is not a corpse
    -- let's try to look for corpses under the cursor because it's probably what the user wants
    -- we will just grab the first one as it's the best we can do
    local items = getItemsAtPosition(df.global.cursor.x, df.global.cursor.y, df.global.cursor.z, df.item_type.CORPSE)
    if #items > 0 then
        print("Automatically chose first corpse under cursor.")
        selected_item = items[1]
    end
end

if not selected_unit and not selected_item then
    qerror("Please select a corpse")
end

if selected_item then
    hist_figure_id = selected_item.hist_figure_id
elseif selected_unit then
    hist_figure_id = selected_unit.hist_figure_id
end

if not hist_figure_id then
    qerror("Failed to find hist_figure_id. This is not user error")
elseif hist_figure_id == -1 then
    if not selected_unit then
        selected_unit = findUnit(selected_item.unit_id)
        if not selected_unit then
            qerror("Not a historical figure, cannot find death info")
        end
    end

    displayDeathUnit(selected_unit)
else
    displayDeathHistFig(findHistFig(hist_figure_id))
end
