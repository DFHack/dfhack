-- Blocks most types of visitors (caravans, migrants, etc.)
--@ enable=true
--@ module=true

local argparse = require('argparse')
local json = require('json')
local persist = require('persist-table')
local repeatutil = require('repeat-util')

local GLOBAL_KEY = 'hermit'
local timed_events = df.global.timed_events
local allowlist = {
    [df.timed_event_type.WildlifeCurious] = true,
    [df.timed_event_type.WildlifeMischievous] = true,
    [df.timed_event_type.WildlifeFlier] = true,
}

enabled = enabled or false

function isEnabled()
    return enabled
end

local function persist_state()
    persist.GlobalTable[GLOBAL_KEY] = json.encode{enabled=enabled}
end

local function load_state()
    local persisted_data = json.decode(persist.GlobalTable[GLOBAL_KEY] or '') or {}
    enabled = persisted_data.enabled or false
end

function event_loop()
    if not enabled then return end

    local tmp_events = {} --as:df.timed_event[]
    for _, event in pairs(timed_events) do
        table.insert(tmp_events, event)
    end
    timed_events:resize(0)

    for _, event in pairs(tmp_events) do
        if allowlist[event.type] then
            timed_events:insert('#', event)
        else
            event:delete()
        end
    end

    repeatutil.scheduleUnlessAlreadyScheduled(GLOBAL_KEY, 1, 'days', event_loop)
end

local function print_status()
    print(('hermit is currently %s.'):format(enabled and 'enabled' or 'disabled'))
end

dfhack.onStateChange[GLOBAL_KEY] = function(sc)
    if sc == SC_MAP_UNLOADED then
        enabled = false
        return
    end

    if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
        return
    end

    load_state()
    event_loop()
end

if dfhack_flags.module then
    return
end

if df.global.gamemode ~= df.game_mode.DWARF or not dfhack.isMapLoaded() then
    dfhack.printerr('hermit needs a loaded fortress to work')
    return
end

local args, opts = {...}, {}
if dfhack_flags and dfhack_flags.enable then
    args = {dfhack_flags.enable_state and 'enable' or 'disable'}
end

local positionals = argparse.processArgsGetopt(args, {
    {'h', 'help', handler=function() opts.help = true end},
})

local command = positionals[1]
if command == 'help' or opts.help then
    print(dfhack.script_help())
elseif command == 'enable' then
    enabled = true
    persist_state()
    event_loop()
elseif command == 'disable' then
    enabled = false
    persist_state()
    repeatutil.cancel(GLOBAL_KEY)
elseif not command or command == 'status' then
    print_status()
else
    print(dfhack.script_help())
end
