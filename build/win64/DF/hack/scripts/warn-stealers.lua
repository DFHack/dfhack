--@ enable = true

local eventful = require("plugins.eventful")
local repeatUtil = require("repeat-util")

local eventfulKey = "warn-stealers"
local numTicksBetweenChecks = 100

function gamemodeCheck()
    if df.global.gamemode == df.game_mode.DWARF then
        return true
    end
    cache = nil
    print("warn-stealers must be run in fort mode")
    disable()
    return false
end

cache = cache or {}

local races = df.global.world.raws.creatures.all

function addToCacheIfStealer(unitId)
    if not gamemodeCheck() then
        return
    end
    local unit = df.unit.find(unitId)
    assert(unit, "New active unit detected with id " .. unitId .. " was not found!")
    local casteFlags = races[unit.race].caste[unit.caste].flags
    if casteFlags.CURIOUS_BEAST_EATER or casteFlags.CURIOUS_BEAST_GUZZLER or casteFlags.CURIOUS_BEAST_ITEM then
        cache[unitId] = true
    end
end

function announce(unit)
    local caste = races[unit.race].caste[unit.caste]
    local casteFlags = caste.flags
    local desires = {}
    if casteFlags.CURIOUS_BEAST_EATER then
        table.insert(desires, "eat food")
    end
    if casteFlags.CURIOUS_BEAST_GUZZLER then
        table.insert(desires, "guzzle drinks")
    end
    if casteFlags.CURIOUS_BEAST_ITEM then
        table.insert(desires, "steal items")
    end
    local str = table.concat(desires, " and ")
    dfhack.gui.showZoomAnnouncement(-1, unit.pos, "A " .. caste.caste_name[0] .. " has appeared, it may " .. str .. ".", COLOR_RED, true)
end

function checkCache()
    if not gamemodeCheck() then
        return
    end
    for unitId in pairs(cache) do
        local unit = df.unit.find(unitId)
        if unit then
            if unit.flags1.inactive then
                cache[unitId] = nil
            elseif not dfhack.units.isHidden(unit) then
                announce(unit)
                cache[unitId] = nil
            end
        else
            cache[unitId] = nil
        end
    end
end

function help()
    print(dfhack.script_help())
end

function enable()
    if not gamemodeCheck() then
        return
    end
    eventful.enableEvent(eventful.eventType.UNIT_NEW_ACTIVE, numTicksBetweenChecks)
    eventful.onUnitNewActive[eventfulKey] = addToCacheIfStealer
    repeatUtil.scheduleEvery(eventfulKey, numTicksBetweenChecks, "ticks", checkCache)
    -- in case any units were missed
    for _, unit in ipairs(df.global.world.units.active) do
        addToCacheIfStealer(unit.id)
    end
    print("warn-stealers running")
end

function disable()
    eventful.onUnitNewActive[eventfulKey] = nil
    repeatUtil.cancel(eventfulKey)
    print("warn-stealers stopped")
end

local action_switch = {enable = enable, disable = disable}
setmetatable(action_switch, {__index = function() return help end})

args = {...}
if dfhack_flags and dfhack_flags.enable then
    args = {dfhack_flags.enable_state and "enable" or "disable"}
end
action_switch[args[1] or "help"]()
