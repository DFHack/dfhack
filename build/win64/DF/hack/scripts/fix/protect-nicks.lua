-- Workaround for the v50.x bug where Dwarf Fortress doesn't set the creature nicknames everywhere.
-- It leads the nickname to be sometimes lost, but also not displayed in legends, engravings, etc...
--@ enable = true
--@ module = true

local json = require('json')
local persist = require('persist-table')

local GLOBAL_KEY = 'fix-protect-nicks'

enabled = enabled or false

function isEnabled()
    return enabled
end

local function persist_state()
    persist.GlobalTable[GLOBAL_KEY] = json.encode({enabled=enabled})
end

-- Reassign all the units nicknames with "dfhack.units.setNickname"
local function save_nicks()
    for _,unit in pairs(df.global.world.units.active) do
        dfhack.units.setNickname(unit, unit.name.nickname)
    end
end

local function event_loop()
    if enabled then
        save_nicks()
        dfhack.timeout(1, 'days', event_loop)
    end
end

dfhack.onStateChange[GLOBAL_KEY] = function(sc)
    if sc == SC_MAP_UNLOADED then
        enabled = false
        return
    end

    if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
        return
    end

    local persisted_data = json.decode(persist.GlobalTable[GLOBAL_KEY] or '')
    enabled = (persisted_data or {enabled=false})['enabled']
    event_loop()
end

if dfhack_flags.module then
    return
end

if df.global.gamemode ~= df.game_mode.DWARF or not dfhack.isMapLoaded() then
    -- Possibly to review with adventure mode
    dfhack.printerr('fix/protect-nicks only works in fortress mode')
    return
end

local args = {...}
if dfhack_flags and dfhack_flags.enable then
    args = {dfhack_flags.enable_state and 'enable' or 'disable'}
end

if args[1] == "enable" then
    enabled = true
elseif args[1] == "disable" then
    enabled = false
elseif args[1] == "now" then
    print("Restoring and saving nicknames")
    save_nicks()
    return
else
    local enabled_str = enabled and "enabled" or "disabled"
    print("fix/protect-nicks is currently " .. enabled_str)
    return
end

event_loop()
persist_state()
