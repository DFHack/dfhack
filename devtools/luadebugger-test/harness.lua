-- Standalone test harness for devtools/luadebugger-test/dbg_instrumented.lua.
--
-- Runs the debuggee under a plain lua 5.3 interpreter (e.g. one built from
-- depends/lua) with the dfhack environment stubbed out:
--   * mkmodule / dfhack.BASE_G / dfhack.{run_script,enable_script,interpreter}
--   * the plugin-exported transport functions (debugger_recv/send/connected)
--   * require('json'), backed by the minimal implementation below
--
-- The transport is driven by a pre-enqueued inbound message queue. Messages
-- flagged 'stopped' are only delivered to the blocking recv used inside the
-- debug loop, mimicking the fact that a real client only sends stack/eval/
-- continue traffic while the debuggee is stopped.
--
-- Usage: lua53 harness.lua [--verbose]

local DIR = arg[0]:match('^(.*)[/\\][^/\\]*$') or '.'
local VERBOSE = arg[1] == '--verbose'

-- minimal JSON (encode/decode) for test purposes, API-compatible with
-- dfhack's json module
local json = dofile(DIR .. '/json_stub.lua')

------------------------------------------------------------------------
-- dfhack environment stubs
------------------------------------------------------------------------
local inbound = {}      -- {msg=string, when='any'|'stopped'}
local outbound = {}     -- raw JSON strings sent by the debuggee
local conn = {connected = true}
local results = {}

local function note(s)
    results[#results + 1] = s
    if VERBOSE then io.write('  | ' .. s .. '\n') end
end

local function enqueue(when, tbl)
    tbl.seq = tbl.seq or 0
    tbl.type = 'request'
    inbound[#inbound + 1] = {msg = json.encode(tbl), when = when}
end

-- drop any unconsumed queued messages (e.g. a continue/disconnect whose
-- corresponding stop never happened); scenarios are meant to be independent
local function clear_inbound()
    for k in pairs(inbound) do inbound[k] = nil end
end

local function dequeue_cmd(m)
    local ok, r = pcall(json.decode, m.msg)
    return ok and r.command or '?'
end
function debugger_recv(timeout_ms)
    if timeout_ms and timeout_ms < 0 then
        local m = table.remove(inbound, 1)
        if VERBOSE and m then
            io.write('  >> recv(-1): ', tostring(dequeue_cmd(m)), '\n')
        end
        return m and m.msg or ''
    end
    local m = inbound[1]
    if m and m.when ~= 'stopped' then
        table.remove(inbound, 1)
        if VERBOSE then
            io.write('  >> recv(0): ', tostring(dequeue_cmd(m)), '\n')
        end
        return m.msg
    end
    return ''
end
function debugger_send(msg)
    outbound[#outbound + 1] = msg
    return true
end
function debugger_connected() return conn.connected end
function debugger_port() return 43030 end

local dfhack = {
    BASE_G = _G,
    is_core_context = true,
    run_script = function(name) return ('ran %s'):format(name) end,
    enable_script = function(name, en) return en end,
    interpreter = function() return 'interp' end,
}
_G.dfhack = dfhack

local function mkmodule(name)
    return setmetatable({}, {__index = _G})
end
_G.mkmodule = mkmodule

local real_require = require
local function require_stub(name)
    if name == 'json' then return json end
    return real_require(name)
end
_G.require = require_stub

------------------------------------------------------------------------
-- load the debuggee
------------------------------------------------------------------------
local mod = assert(loadfile(DIR .. '/../../plugins/lua/luadebugger.lua'))()
assert(type(mod) == 'table' and type(mod.poll) == 'function',
       'debuggee module did not load')

------------------------------------------------------------------------
-- assertions
------------------------------------------------------------------------
local n_pass, n_fail = 0, 0
local function check(cond, label)
    if cond then
        n_pass = n_pass + 1
        if VERBOSE then io.write('PASS ' .. label .. '\n') end
    else
        n_fail = n_fail + 1
        io.write('FAIL ' .. label .. '\n')
    end
end

local function drain_outbound()
    local msgs = {}
    for _, raw in ipairs(outbound) do
        msgs[#msgs + 1] = json.decode(raw)
    end
    outbound = {}
    return msgs
end

local function find(msgs, pred)
    for _, m in ipairs(msgs) do
        if pred(m) then return m end
    end
end

------------------------------------------------------------------------
-- scenario 1: attach, breakpoint, inspect, step, continue, disconnect
------------------------------------------------------------------------
io.write('--- scenario 1: breakpoint + inspect + step ---\n')

local target_path = DIR .. '/target.lua'
local dap_seq = 0
local function req(command, args, when)
    dap_seq = dap_seq + 1
    enqueue(when or 'any', {seq = dap_seq, command = command,
                            arguments = args})
    return dap_seq
end

req('initialize', {adapterID = 'harness'})
req('attach', {})
req('setBreakpoints', {source = {path = target_path},
                       breakpoints = {{line = 10}}})
req('configurationDone')
-- queued for consumption while stopped at the breakpoint. Thread ids are
-- assigned in registration order: 1 = main thread, 2 = the target coroutine.
req('stackTrace', {threadId = 2}, 'stopped')
req('scopes', {frameId = 1}, 'stopped')
req('variables', {variablesReference = 1}, 'stopped')   -- Locals scope ref
req('evaluate', {expression = 'acc', frameId = 1}, 'stopped')
req('evaluate', {expression = 'i', frameId = 1}, 'stopped')
req('next', {threadId = 2}, 'stopped')
-- after 'next' stops, frame ids were invalidated: re-fetch the stack before
-- evaluating in the new frame context. 'next' lands on the `for` line (9),
-- where the body-scoped `i` is not yet visible; `acc` is (==3 after iter 1).
req('stackTrace', {threadId = 2}, 'stopped')
req('evaluate', {expression = 'acc', frameId = 1}, 'stopped')
req('continue', {threadId = 2}, 'stopped')
req('disconnect')

mod.poll()

do
    local msgs = drain_outbound()
    check(find(msgs, function(m)
        return m.type == 'response' and m.command == 'initialize' and m.success
    end) ~= nil, 'initialize response')
    check(find(msgs, function(m)
        return m.type == 'event' and m.event == 'initialized'
    end) ~= nil, 'initialized event')
    check(find(msgs, function(m)
        return m.type == 'response' and m.command == 'setBreakpoints' and
               m.success and m.body.breakpoints[1].verified
    end) ~= nil, 'setBreakpoints verified')
end

-- run the target; it should hit the breakpoint at line 10 (first hit: i=1)
local target_fn = assert(loadfile(target_path))
local co = coroutine.create(target_fn)
local co_ok, co_res = coroutine.resume(co)

do
    local msgs = drain_outbound()
    for _, m in ipairs(msgs) do
        note(json.encode(m))
    end
    local stopped = find(msgs, function(m)
        return m.type == 'event' and m.event == 'stopped'
    end)
    check(stopped ~= nil, 'stopped event emitted')
    check(stopped and stopped.body.reason == 'breakpoint',
          'stopped reason == breakpoint')
    local trace = find(msgs, function(m)
        return m.command == 'stackTrace' and m.success
    end)
    check(trace ~= nil, 'stackTrace response')
    if trace then
        local f = trace.body.stackFrames[1]
        check(f.source and f.source.path and
              f.source.path:gsub('\\', '/'):find('target%.lua$') ~= nil,
              'top frame is target.lua')
        check(f.line == 10, 'top frame line == 10 (got ' ..
              tostring(f.line) .. ')')
    end
    local vars = find(msgs, function(m)
        return m.command == 'variables' and m.success
    end)
    check(vars ~= nil, 'variables response')
    if vars then
        local names = {}
        for _, v in ipairs(vars.body.variables) do names[v.name] = v end
        check(names.acc ~= nil, 'local acc visible')
        check(names.i ~= nil, 'local i visible')
        check(names.n ~= nil, 'upvalue/arg n visible')
    end
    local eval_acc = find(msgs, function(m)
        return m.command == 'evaluate' and m.success and
               m.body.result == '0'
    end)
    check(eval_acc ~= nil, 'evaluate acc == 0 on first hit')
    local eval_i = find(msgs, function(m)
        return m.command == 'evaluate' and m.success and
               m.body.result == '1'
    end)
    check(eval_i ~= nil, 'evaluate i == 1 on first hit')
    local eval_acc2 = find(msgs, function(m)
        return m.command == 'evaluate' and m.success and
               m.body.result == '3'
    end)
    check(eval_acc2 ~= nil, 'evaluate acc == 3 after next')
    check(co_ok == true and co_res ~= nil,
          'target coroutine completed after continue')
end
clear_inbound()

------------------------------------------------------------------------
-- scenario 2: breakpoint inside a coroutine + stepIn/stepOut
------------------------------------------------------------------------
io.write('--- scenario 2: coroutine coverage + stepIn/stepOut ---\n')

-- The debuggee deactivated on disconnect; start a fresh session.
conn.connected = true
dap_seq = 0
req('initialize', {adapterID = 'harness'})
req('attach', {})
-- breakpoint inside the coroutine body (line 17: return helper(10) + #mine)
req('setBreakpoints', {source = {path = target_path},
                       breakpoints = {{line = 17}}})
req('configurationDone')
req('threads', nil, 'stopped')
-- thread 3 = the coroutine spawned inside target.lua (1=main, 2=outer)
req('stackTrace', {threadId = 3}, 'stopped')
req('stepIn', {threadId = 3}, 'stopped')    -- into helper() at line 4
req('stackTrace', {threadId = 3}, 'stopped')
req('stepOut', {threadId = 3}, 'stopped')
-- helper() is called in a return position, so stepping out of it leaves no
-- further line events in the coroutine: the thread just ends. The queued
-- continue/disconnect below are therefore never consumed; clear_inbound()
-- after the scenario drops them.
req('continue', {threadId = 3}, 'stopped')
req('disconnect')

mod.poll()
drain_outbound()

co = coroutine.create(target_fn)
co_ok, co_res = coroutine.resume(co)

do
    local msgs = drain_outbound()
    for _, m in ipairs(msgs) do note(json.encode(m)) end
    local stops = {}
    for _, m in ipairs(msgs) do
        if m.type == 'event' and m.event == 'stopped' then
            stops[#stops + 1] = m
        end
    end
    check(#stops == 2, 'stopped at coroutine bp + stepIn (' ..
          tostring(#stops) .. ' stops)')
    local traces = {}
    for _, m in ipairs(msgs) do
        if m.command == 'stackTrace' and m.success then
            traces[#traces + 1] = m
        end
    end
    if traces[1] then
        local f = traces[1].body.stackFrames[1]
        check(f.line == 17, 'coroutine bp at line 17 (got ' ..
              tostring(f.line) .. ')')
    end
    if traces[2] then
        local f = traces[2].body.stackFrames[1]
        check(f.line == 4 or f.line == 5,
              'stepIn landed inside helper (line ' .. tostring(f.line) .. ')')
    end
    local threads_resp = find(msgs, function(m)
        return m.command == 'threads' and m.success
    end)
    check(threads_resp and #threads_resp.body.threads >= 2,
          'threads lists main + coroutine')
    check(co_ok == true, 'target coroutine completed')
end
clear_inbound()

------------------------------------------------------------------------
-- scenario 3: pause while running
------------------------------------------------------------------------
io.write('--- scenario 3: pause ---\n')

conn.connected = true
dap_seq = 0
req('initialize', {adapterID = 'harness'})
req('attach', {})
req('configurationDone')
req('pause')
req('continue', {threadId = 1}, 'stopped')
req('disconnect')

mod.poll()

co = coroutine.create(target_fn)
co_ok = coroutine.resume(co)

do
    local msgs = drain_outbound()
    for _, m in ipairs(msgs) do note(json.encode(m)) end
    -- the stale line-17 breakpoint from scenario 2 (breakpoints persist
    -- across sessions) may fire first; just require that a pause stop exists
    local stopped = find(msgs, function(m)
        return m.type == 'event' and m.event == 'stopped' and
               m.body.reason == 'pause'
    end)
    check(stopped ~= nil, 'stopped event with reason == pause')
end

io.write(('=== %d passed, %d failed ===\n'):format(n_pass, n_fail))
os.exit(n_fail > 0 and 1 or 0)
