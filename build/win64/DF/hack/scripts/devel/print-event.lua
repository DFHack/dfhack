-- Prints the description of an event by ID or index.
--author BenLubar
--[====[

devel/print-event
=================
Prints the description of an event by ID or index.

]====]
local utils = require "utils"

local validArgs = utils.invert({
    "id",
    "index"
})

local args = utils.processArgs({...})

if type(args.id) == "string" then
    args.id = {args.id}
elseif args.id == nil then
    args.id = {}
end
if type(args.index) == "string" then
    args.index = {args.index}
elseif args.index == nil then
    args.index = {}
end

if #args.id == 0 and #args.index == 0 then
    qerror("requires at least one event ID or index")
end

local function print_event(event)
    local str = df.new("string")
    local ctx = df.history_event_context:new()
    event:getSentence(str, ctx)
    ctx:delete()
    print(str.value)
    str:delete()
end

for _, id in ipairs(args.id) do
    local event = df.history_event.find(tonumber(id))
    if not event then
        qerror("event ID not found: " .. id)
    end
    print_event(event)
end

for _, idx in ipairs(args.index) do
    local event = df.global.world.history.events[tonumber(idx)]
    print_event(event)
end
