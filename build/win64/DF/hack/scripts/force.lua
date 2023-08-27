-- Forces an event (wrapper for modtools/force)

local utils = require 'utils'
local args = {...}
if #args < 1 then qerror('missing event type') end
if args[1]:find('help') then
    print(dfhack.script_help())
    return
end
local eventType = nil
for _, type in ipairs(df.timed_event_type) do
    if type:lower() == args[1]:lower() then
        eventType = type
    end
end
if not eventType then
    qerror('unknown event type: ' .. args[1])
end

local newArgs = {'--eventType', eventType}
if eventType == 'Caravan' or eventType == 'Diplomat' then
    table.insert(newArgs, '--civ')
    if not args[2] then
        table.insert(newArgs, 'player')
    else
        table.insert(newArgs, args[2])
    end
end

dfhack.run_script('modtools/force', table.unpack(newArgs))
