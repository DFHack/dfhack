-- Regularly writes extra info to gamelog.txt
local help = [[=begin

modtools/extra-gamelog
======================
This script writes extra information to the gamelog.
This is useful for tools like :forums:`Soundsense <106497>`.

=end]]

msg = dfhack.gui.writeToGamelog

function log_on_load(op)
    if op ~= SC_WORLD_LOADED then return end

    -- Seasons fix for Soundsense
    local seasons = {
        [-1] = 'Nothing', -- worldgen
        'Spring',
        'Summer',
        'Autumn',
        'Winter'}
    msg(seasons[df.global.cur_season]..' has arrived on the calendar.')

    -- Weather fix for Soundsense
    local raining = false
    local snowing = false
    for _, row in ipairs(df.global.current_weather) do
        for _, weather in ipairs(row) do
            raining = raining or weather == 1
            snowing = snowing or weather == 2
        end
    end
    if (not snowing and not raining) then msg("The weather has cleared.")
    elseif raining then msg("It has started raining.")
    elseif snowing then msg("A snow storm has come.")
    end

    -- Log site information for forts
    if df.world_site.find(df.global.ui.site_id) == nil then return end
    local site = df.world_site.find(df.global.ui.site_id)
    local fort_ent = df.global.ui.main.fortress_entity
    local civ_ent = df.historical_entity.find(df.global.ui.civ_id)
    local function fullname(item)
        return dfhack.TranslateName(item.name)..' ('..dfhack.TranslateName(item.name ,true)..')'
    end
    msg('Loaded '..df.global.world.cur_savegame.save_dir..', '..fullname(df.global.world.world_data)..
        ' at coordinates ('..site.pos.x..','..site.pos.y..')')
    msg('Loaded the fortress '..fullname(site)..
        (fort_ent and ', colonized by the group '..fullname(fort_ent) or '')..
        (civ_ent and ' of the civilization '..fullname(civ_ent)..'.' or '.'))
end


old_expedition_leader = nil
old_mayor = nil
function log_nobles()
    local expedition_leader = nil
    local mayor = nil
    local function check(unit)
        if not dfhack.units.isCitizen(unit) then return end
        for _, pos in ipairs(dfhack.units.getNoblePositions(unit) or {}) do
            if pos.position.name[0] == "expedition leader" then
                expedition_leader = unit
            elseif pos.position.name[0] == "mayor" then
                mayor = unit
            end
        end
    end
    for _, unit in ipairs(df.global.world.units.active) do
        check(unit)
    end

    if old_mayor == nil and expedition_leader == nil and mayor ~= nil and old_expedition_leader ~= nil then
        msg("Expedition leader was replaced by mayor.")
    end

    if expedition_leader ~= old_expedition_leader then
        if expedition_leader == nil then
            msg("Expedition leader position is now vacant.")
        else
            msg(dfhack.TranslateName(dfhack.units.getVisibleName(expedition_leader)).." became expedition leader.")
        end
    end

    if mayor ~= old_mayor then
        if mayor == nil then
            msg("Mayor position is now vacant.")
        else
            msg(dfhack.TranslateName(dfhack.units.getVisibleName(mayor)).." became mayor.")
        end
    end
    old_mayor = mayor
    old_expedition_leader = expedition_leader
end

siege = false
function log_siege()
    local function cur_siege()
        for _, unit in ipairs(df.global.world.units.active) do
            if unit.flags1.active_invader then return true end
        end
        return false
    end
    local old_siege = siege
    siege = cur_siege()
    if siege ~= old_siege and siege then
        msg("A vile force of darkness has arrived!")
    elseif siege ~= old_siege and not siege then
        msg("Siege was broken.")
    end
end


local workshopTypes = {
    [0]="Carpenters Workshop",
    "Farmers Workshop",
    "Masons Workshop",
    "Craftsdwarfs Workshop",
    "Jewelers Workshop",
    "Metalsmiths Forge",
    "Magma Forge",
    "Bowyers Workshop",
    "Mechanics Workshop",
    "Siege Workshop",
    "Butchers Workshop",
    "Leatherworks Workshop",
    "Tanners Workshop",
    "Clothiers Workshop",
    "Fishery",
    "Still",
    "Loom",
    "Quern",
    "Kennels",
    "Kitchen",
    "Ashery",
    "Dyers Workshop",
    "Millstone",
    "Custom",
    "Tool",
    }

local furnaceTypes = {
    [0]="Wood Furnace",
    "Smelter",
    "Glass Furnace",
    "Kiln",
    "Magma Smelter",
    "Magma Glass Furnace",
    "Magma Kiln",
    "Custom Furnace",
    }

buildStates = {}

function log_buildings()
    for _, building in ipairs(df.global.world.buildings.all) do
        if getmetatable(building) == "building_workshopst" or getmetatable(building) == "building_furnacest" then
            buildStates[building.id] = buildStates[building.id] or building.flags.exists
            if buildStates[building.id] ~= building.flags.exists then
                buildStates[building.id] = building.flags.exists
                if building.flags.exists then
                    if getmetatable(building) == "building_workshopst" then
                        msg(workshopTypes[building.type].." was built.")
                    elseif getmetatable(building) == "building_furnacest" then
                        msg(furnaceTypes[building.type].." was built.")
                    end
                end
            end
        end
    end
end

local function event_loop()
    log_nobles()
    log_siege()
    log_buildings()
    dfhack.timeout(50, 'ticks', event_loop)
end

local args = {...}
if args[1] == 'disable' then
    dfhack.onStateChange[_ENV] = nil
elseif args[1] == 'enable' then
    dfhack.onStateChange[_ENV] = log_on_load
    event_loop()
else
    print(help)
end
