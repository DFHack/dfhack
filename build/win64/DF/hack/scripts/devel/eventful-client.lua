-- Testing script for eventful events

--[====[
devel/eventful-client
=====================

Usage::

    devel/eventful-client help
    devel/eventful-client add <event type> <frequency>
    devel/eventful-client add all <frequency>
    devel/eventful-client list
    devel/eventful-client clear

:help:  shows this help text and a list of valid event types
:add:   add a handler for the named event type at the requested tick frequency
:list:  lists active handlers and their metadata
:clear: unregisters all handlers

Note this script does not handle the eventful reaction or workshop events.
]====]

local eventful = require('plugins.eventful')

local utils = require('utils')

rng = rng or dfhack.random.new()

-- maps handler name to {count, freq}
handlers = handlers or {}

local function get_event_fn_name(event_type_name)
    local fn_name = 'on'
    for word in event_type_name:gmatch('[^_]+') do
        fn_name = fn_name .. word:sub(1,1) .. word:sub(2):lower()
    end
    if eventful[fn_name] then return fn_name end
    fn_name = fn_name .. 'CreatedDestroyed'
    if eventful[fn_name] then return fn_name end
    return nil
end

local function get_event_registry()
    local registry = {}
    local event_list = utils.invert(eventful.eventType)
    for i=0,#event_list do
        local event_type_name = event_list[i]
        if event_type_name == 'EVENT_MAX' then
            return registry
        end
        registry[i+1] = {etype=event_type_name,
                         fn=get_event_fn_name(event_type_name)}
    end
end

local function help()
    print(dfhack.script_help())
    print()
    print('Event types (and the eventful event functions they map to):')
    for _,v in ipairs(get_event_registry()) do
        print(('  %s -> %s'):format(v.etype, v.fn))
    end
end

local function make_handler_fn(registry_entry, handler_name, freq)
    return function(...)
        local handler = handlers[handler_name]
        handler.count = handler.count + 1
        print(('eventful-client: %s received %s event (freq=%d)')
              :format(os.date("%X"), registry_entry.etype, freq))
        print('  params:', ...)
    end
end

-- adds a new handler for the specified event type. uses a unique handler name
-- each time so multiple handlers can be registered for the same event.
local function add_one(registry_entry, freq)
    if not registry_entry.fn then
        print(('no eventful event function for %s events; skipping')
              :format(registry_entry.etype))
        return
    end
    eventful.enableEvent(eventful.eventType[registry_entry.etype], freq)
    local handler_name = 'eventful-client.'..tostring(rng:random())
    local handler = make_handler_fn(registry_entry, handler_name, freq)
    print(('eventful-client registering new %s handler at freq %d: %s')
          :format(registry_entry.etype, freq, handler_name))
    eventful[registry_entry.fn][handler_name] = handler
    handlers[handler_name] = {count=0, freq=freq}
end

local function add(spec, freq)
    for _,v in ipairs(get_event_registry()) do
        if spec == 'all' or spec == v.etype then
            add_one(v, freq)
        end
    end
end

local function iterate_handlers(fn)
    local count = 0
    for _,v in ipairs(get_event_registry()) do
        if not eventful[v.fn] then goto continue end
        for k in pairs(eventful[v.fn]) do
            if k:startswith('eventful-client.') then
                count = count + 1
                fn(v, k)
            end
        end
        ::continue::
    end
    return count
end

local function list_fn(registry_entry, handler_name)
    local hdata = handlers[handler_name]
    local metadata = 'handler metadata not found'
    if hdata then
        metadata = ('freq=%s, call count=%s'):format(hdata.freq, hdata.count)
    end
    print(('  %s -> %s (%s)')
          :format(registry_entry.etype, handler_name, metadata))
end

local function list()
    print('Active handlers:')
    if 0 == iterate_handlers(list_fn) then
        print('  None')
    end
end

local function clear_fn(registry_entry, handler_name)
    list_fn(registry_entry, handler_name)
    eventful[registry_entry.fn][handler_name] = nil
end

local function clear()
    print('Clearing handlers:')
    iterate_handlers(clear_fn)
    handlers = {}
end

local action_switch = {
    add=add,
    list=list,
    clear=clear,
}
setmetatable(action_switch, {__index=function() return help end})

local args = {...}
local action = table.remove(args, 1) or 'help'

action_switch[action](table.unpack(args))
