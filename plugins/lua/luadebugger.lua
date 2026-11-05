-- Debug Adapter Protocol (DAP) debuggee for DFHack Lua.
--
-- This module implements the "debug adapter" side of a DAP session in pure
-- Lua. It talks to the remote client through whole DAP message bodies
-- exchanged with the luadebugger plugin's mailboxes (debugger_recv /
-- debugger_send); the plugin owns the TCP listener and the transport threads.
--
-- Execution model: debug.sethook installs a line hook on the main state and
-- on every coroutine created after session start (coroutine entry points and
-- the dfhack.* entry points that core C++ resumes in fresh threads are
-- wrapped). When a breakpoint is hit the hook enters a command loop that
-- blocks the simulation thread in debugger_recv() until the client resumes
-- execution. The game therefore freezes while stopped, which is the intended
-- "paused" semantics and needs no cooperation from DF's event loop.
--
-- poll() is invoked once per frame by plugin_onupdate whenever the plugin has
-- inbound traffic or a client state change, so connection setup and commands
-- that don't require a stopped thread are serviced even when no Lua code is
-- running.

local _ENV = mkmodule('plugins.luadebugger')

local json = require('json')

local recv_raw = debugger_recv
local send_raw = debugger_send
local is_connected = debugger_connected

local DBG_SOURCE = debug.getinfo(1, 'S').source
local main_thread = coroutine.running()

-- tunables
local POLL_INTERVAL = 128     -- line-hook events between inbound mailbox drains
local MAX_STRING = 1024       -- max string length reported in variables
local MAX_CHILDREN = 1000     -- max table children per variables page

------------------------------------------------------------------------
-- session state
------------------------------------------------------------------------
local active = false            -- session active (hooks installed)
local attach_args = nil         -- arguments from the attach request
local stopped_co = nil          -- coroutine currently blocked in debug_loop
local stopped_depth = 0         -- stack depth of stopped_co measured in hook
local pause_requested = false
local stepping = nil            -- {mode='in'|'over'|'out', co=thread, depth=int}
local seq_counter = 0

local breakpoints = {}          -- normalized client path -> {line -> true}
local chunk_to_path = {}        -- chunk source -> normalized client path|false
local seen_sources = {}         -- all chunk sources observed by the hook

local thread_ids = setmetatable({}, {__mode='k'})  -- thread -> int id
local next_thread_id = 0

local var_refs = {}             -- variablesReference -> descriptor
local next_var_ref = 1
local frame_refs = {}           -- stackFrame id -> {co, rel}
local next_frame_id = 1

local saved = {}                -- original functions, for unpatch

------------------------------------------------------------------------
-- small utilities
------------------------------------------------------------------------
local function next_seq()
    seq_counter = seq_counter + 1
    return seq_counter
end

local function send_response(req, body, ok, message)
    send_raw(json.encode{
        seq = next_seq(),
        type = 'response',
        request_seq = req.seq,
        command = req.command,
        success = ok ~= false,
        message = message,
        body = body,
    })
end

local function send_event(event, body)
    send_raw(json.encode{
        seq = next_seq(),
        type = 'event',
        event = event,
        body = body,
    })
end

local function tostring_safe(v)
    local ok, s = pcall(tostring, v)
    if ok then return s end
    return '(' .. type(v) .. ': tostring failed)'
end

local function normalize_path(p)
    return tostring(p or ''):gsub('\\', '/'):lower()
end

local function thread_id(co)
    local id = thread_ids[co]
    if not id then
        next_thread_id = next_thread_id + 1
        id = next_thread_id
        thread_ids[co] = id
    end
    return id
end

local function thread_of_id(id)
    for co, tid in pairs(thread_ids) do
        if tid == id then return co end
    end
end

local function thread_name(co)
    if co == main_thread then return 'main thread' end
    local info = debug.getinfo(co, 0, 'Sl')
    if info then
        return ('coroutine %s:%d (%s)'):format(
            info.short_src, info.currentline, coroutine.status(co))
    end
    return 'coroutine (' .. coroutine.status(co) .. ')'
end

local function register_thread(co)
    thread_id(co)
end

------------------------------------------------------------------------
-- stack inspection
------------------------------------------------------------------------
-- Stack layout while the hook is executing (levels as debug.getinfo counts
-- them, from inside hook_impl):
--     0  the debug.getinfo C call itself
--     1  hook_impl                     (this file)
--     2  pcall                         (C)
--     3  hook_fn                       (this file)
--     4  the frame that fired the event
--     5+ user call frames
-- Debugger frames appear only above the firing frame, and user_base_for must
-- therefore subtract one frame per helper function on the call path: a level
-- number is only valid for the frame that computes it.

-- Returns the getinfo level of co's first non-debugger frame, adjusted so the
-- result is valid when used by this function's caller.
local function user_frame_level(co)
    local m = 2   -- 0 = the getinfo call, 1 = this frame; start at the caller
    while true do
        local info = debug.getinfo(co, m, 'S')
        if not info or
           (info.source ~= DBG_SOURCE and info.source ~= '=[C]') then
            return m - 1
        end
        m = m + 1
    end
end

local function stack_depth(co)
    local n = 0
    while debug.getinfo(co, n) do n = n + 1 end
    return n
end

-- getinfo level of a frame in the stopped thread, adjusted for this
-- function's own frame. rel=0 is the user frame that fired the hook; larger
-- rel values walk towards the stack bottom.
local function user_base_for(co)
    if co == stopped_co then
        return user_frame_level(co) - 1
    end
    return 0
end

local function frame_source(info)
    local src = {name = info.short_src}
    if info.source:sub(1, 1) == '@' then
        src.path = info.source:sub(2)
    end
    return src
end

local function frame_func_env(co, level)
    local info = debug.getinfo(co, level, 'f')
    if info and info.func then
        local i = 1
        while true do
            local name, v = debug.getupvalue(info.func, i)
            if not name then break end
            if name == '_ENV' then return v end
            i = i + 1
        end
    end
end

------------------------------------------------------------------------
-- breakpoint bookkeeping
------------------------------------------------------------------------
local function chunk_key(source)
    if type(source) ~= 'string' or source:sub(1, 1) ~= '@' then
        return nil
    end
    return normalize_path(source:sub(2))
end

-- Suffix-match a chunk path against the paths clients have breakpoints on.
-- VSCode sends absolute filesystem paths; DFHack chunks may be absolute or
-- relative (e.g. @./hack/lua/foo.lua), so match on trailing path segments.
local function suffix_match(key, path)
    if #key >= #path then
        return key:sub(-#path) == path and
               (#key == #path or key:sub(-#path - 1, -#path - 1) == '/')
    else
        return path:sub(-#key) == key and
               (#path == #key or path:sub(-#key - 1, -#key - 1) == '/')
    end
end

local function match_bp_path(chunk_src)
    local cached = chunk_to_path[chunk_src]
    if cached ~= nil then return cached end
    local key = chunk_key(chunk_src)
    local found = false
    if key then
        local best = 0
        for bp_path in pairs(breakpoints) do
            if suffix_match(key, bp_path) and #bp_path > best then
                best = #bp_path
                found = bp_path
            end
        end
    end
    chunk_to_path[chunk_src] = found
    return found
end

------------------------------------------------------------------------
-- variable materialization
------------------------------------------------------------------------
local function make_var_ref(entry)
    local id = next_var_ref
    next_var_ref = id + 1
    var_refs[id] = entry
    return id
end

local function var_entry(name, v)
    local t = type(v)
    local e = {name = tostring(name), type = t, variablesReference = 0}
    if t == 'string' then
        local s = v
        if #s > MAX_STRING then s = s:sub(1, MAX_STRING) .. '...' end
        e.value = '"' .. s .. '"'
    elseif t == 'table' then
        e.value = tostring_safe(v)
        local named, indexed = 0, 0
        for k in next, v do
            if type(k) == 'number' then indexed = indexed + 1
            else named = named + 1 end
        end
        e.namedVariables = named
        e.indexedVariables = indexed
        e.variablesReference = make_var_ref{kind = 'table', t = v}
    elseif t == 'function' then
        local info = debug.getinfo(v, 'nS')
        if info then
            e.value = ('function %s:%d'):format(info.short_src, info.linedefined)
        else
            e.value = tostring_safe(v)
        end
    else
        e.value = tostring_safe(v)
    end
    return e
end

local function table_children(t, args)
    local vars = {}
    local start = args.start or 0
    local count = args.count
    local filter = args.filter
    local idx = 0
    for k, v in next, t do
        local indexed = type(k) == 'number'
        if not filter or (filter == 'indexed') == indexed then
            idx = idx + 1
            if idx > start and (not count or #vars < count) then
                vars[#vars + 1] = var_entry(k, v)
            end
        end
        if idx > start + MAX_CHILDREN then
            vars[#vars + 1] = {name = '...', value = '(truncated)',
                               type = 'string', variablesReference = 0}
            break
        end
    end
    return vars
end

------------------------------------------------------------------------
-- request handlers; a handler returns 'continue' to resume execution
------------------------------------------------------------------------
local request_handlers = {}

request_handlers.initialize = function(req, args)
    send_response(req, {
        supportsConfigurationDoneRequest = true,
        supportsEvaluateForHovers = true,
        supportsSetVariable = true,
        supportsLoadedSourcesRequest = true,
    })
    send_event('initialized')
end

request_handlers.attach = function(req, args)
    attach_args = args
    send_response(req)
end

request_handlers.launch = function(req)
    send_response(req, nil, false,
                  'luadebugger only supports attach requests')
end

request_handlers.setBreakpoints = function(req, args)
    local src = args.source or {}
    local path = src.path or src.name
    local resp = {}
    if path then
        local lines = {}
        for _, bp in ipairs(args.breakpoints or {}) do
            if bp.line then lines[bp.line] = true end
            resp[#resp + 1] = {verified = true, line = bp.line}
        end
        breakpoints[normalize_path(path)] = lines
        chunk_to_path = {}    -- invalidate the chunk->client path cache
    end
    send_response(req, {breakpoints = resp})
end

request_handlers.setExceptionBreakpoints = function(req)
    send_response(req, {breakpoints = {}})
end

request_handlers.setFunctionBreakpoints = function(req)
    send_response(req, {breakpoints = {}})
end

request_handlers.configurationDone = function(req)
    send_response(req)
    if attach_args and attach_args.stopOnEntry then
        pause_requested = true
    end
end

request_handlers.pause = function(req)
    pause_requested = true
    send_response(req)
end

request_handlers.threads = function(req)
    local list, seen = {}, {}
    local function add(co)
        if co and not seen[co] and coroutine.status(co) ~= 'dead' then
            seen[co] = true
            list[#list + 1] = {id = thread_id(co), name = thread_name(co)}
        end
    end
    add(stopped_co)
    for co in pairs(thread_ids) do add(co) end
    add(main_thread)
    send_response(req, {threads = list})
end

request_handlers.stackTrace = function(req, args)
    local co = thread_of_id(args.threadId) or stopped_co
    local frames, total = {}, 0
    if co and coroutine.status(co) ~= 'dead' then
        local base = user_base_for(co)
        local startf = args.startFrame or 0
        local maxn = args.levels
        local rel = 0
        while true do
            local info = debug.getinfo(co, base + rel, 'nSl')
            if not info then break end
            total = total + 1
            if rel >= startf and (not maxn or #frames < maxn) then
                local fid = next_frame_id
                next_frame_id = fid + 1
                frame_refs[fid] = {co = co, rel = rel}
                frames[#frames + 1] = {
                    id = fid,
                    name = info.name or
                           ('%s:%d'):format(info.short_src, info.currentline),
                    source = frame_source(info),
                    line = info.currentline,
                    column = 0,
                }
            end
            rel = rel + 1
        end
    end
    send_response(req, {stackFrames = frames, totalFrames = total})
end

request_handlers.scopes = function(req, args)
    local fr = frame_refs[args.frameId]
    local scopes = {}
    if fr then
        scopes[#scopes + 1] = {name = 'Locals', expensive = false,
            variablesReference = make_var_ref{kind = 'locals',
                                              co = fr.co, rel = fr.rel}}
        scopes[#scopes + 1] = {name = 'Upvalues', expensive = false,
            variablesReference = make_var_ref{kind = 'upvalues',
                                              co = fr.co, rel = fr.rel}}
        scopes[#scopes + 1] = {name = 'Globals', expensive = true,
            variablesReference = make_var_ref{kind = 'globals',
                                              co = fr.co, rel = fr.rel}}
    end
    send_response(req, {scopes = scopes})
end

request_handlers.variables = function(req, args)
    local entry = var_refs[args.variablesReference]
    local vars = {}
    if entry then
        if entry.kind == 'table' then
            vars = table_children(entry.t, args)
        elseif entry.kind == 'globals' then
            local level = user_base_for(entry.co) + entry.rel
            local fenv = frame_func_env(entry.co, level) or dfhack.BASE_G
            vars = table_children(fenv, args)
        else
            local level = user_base_for(entry.co) + entry.rel
            if entry.kind == 'locals' then
                local i = 1
                while true do
                    local n, v = debug.getlocal(entry.co, level, i)
                    if not n then break end
                    vars[#vars + 1] = var_entry(n, v)
                    i = i + 1
                end
                i = -1
                while true do
                    local n, v = debug.getlocal(entry.co, level, i)
                    if not n then break end
                    vars[#vars + 1] = var_entry(n, v)
                    i = i - 1
                end
            elseif entry.kind == 'upvalues' then
                local info = debug.getinfo(entry.co, level, 'f')
                if info and info.func then
                    local i = 1
                    while true do
                        local n, v = debug.getupvalue(info.func, i)
                        if not n then break end
                        vars[#vars + 1] = var_entry(n, v)
                        i = i + 1
                    end
                end
            end
        end
    end
    send_response(req, {variables = vars})
end

local function eval_env(co, rel)
    local env = {}
    local fenv = dfhack.BASE_G
    if co and rel then
        local level = user_base_for(co) + rel
        local i = 1
        while true do
            local n, v = debug.getlocal(co, level, i)
            if not n then break end
            env[n] = v
            i = i + 1
        end
        local info = debug.getinfo(co, level, 'f')
        if info and info.func then
            i = 1
            while true do
                local n, v = debug.getupvalue(info.func, i)
                if not n then break end
                env[n] = v
                i = i + 1
            end
        end
        fenv = frame_func_env(co, level) or fenv
    end
    return setmetatable(env, {__index = fenv})
end

request_handlers.evaluate = function(req, args)
    local fr = args.frameId and frame_refs[args.frameId]
    local env = fr and eval_env(fr.co, fr.rel) or eval_env()
    local chunk = load('return ' .. args.expression, '=(debug eval)', 't', env)
    if not chunk then
        chunk = load(args.expression, '=(debug eval)', 't', env)
    end
    if not chunk then
        send_response(req, nil, false,
                      'cannot parse expression: ' .. args.expression)
        return
    end
    local ok, res = pcall(chunk)
    if not ok then
        send_response(req, nil, false, tostring_safe(res))
        return
    end
    local e = var_entry('result', res)
    send_response(req, {
        result = e.value, type = e.type,
        variablesReference = e.variablesReference,
        namedVariables = e.namedVariables,
        indexedVariables = e.indexedVariables,
    })
end

request_handlers.setVariable = function(req, args)
    local entry = var_refs[args.variablesReference]
    if not entry then
        send_response(req, nil, false, 'unknown variable')
        return
    end
    local chunk = load('return ' .. args.value, '=(debug setvar)', 't', {})
    local ok, parsed = chunk and pcall(chunk)
    if not ok then
        send_response(req, nil, false, 'cannot parse value: ' .. args.value)
        return
    end
    if entry.kind == 'table' then
        entry.t[tonumber(args.name) or args.name] = parsed
    else
        local level = user_base_for(entry.co) + entry.rel
        if entry.kind == 'locals' then
            local i = 1
            while true do
                local n = debug.getlocal(entry.co, level, i)
                if not n then break end
                if n == args.name then
                    debug.setlocal(entry.co, level, i, parsed)
                    break
                end
                i = i + 1
            end
        elseif entry.kind == 'upvalues' then
            local info = debug.getinfo(entry.co, level, 'f')
            if info and info.func then
                local i = 1
                while true do
                    local n = debug.getupvalue(info.func, i)
                    if not n then break end
                    if n == args.name then
                        debug.setupvalue(info.func, i, parsed)
                        break
                    end
                    i = i + 1
                end
            end
        else
            (frame_func_env(entry.co, level) or dfhack.BASE_G)[args.name] = parsed
        end
    end
    local e = var_entry(args.name, parsed)
    send_response(req, {value = e.value, type = e.type,
                        variablesReference = e.variablesReference})
end

request_handlers['continue'] = function(req)
    send_response(req, {allThreadsContinued = true})
    send_event('continued', {
        threadId = stopped_co and thread_id(stopped_co) or 1,
        allThreadsContinued = true})
    return 'continue'
end

local function step(mode)
    return function(req)
        stepping = {mode = mode, co = stopped_co, depth = stopped_depth}
        send_response(req)
        return 'continue'
    end
end
request_handlers.next = step('over')
request_handlers.stepIn = step('in')
request_handlers.stepOut = step('out')

request_handlers.disconnect = function(req)
    send_response(req)
    send_event('terminated')
    deactivate()
    return 'continue'
end
request_handlers.terminate = request_handlers.disconnect

request_handlers.loadedSources = function(req)
    local srcs = {}
    for s in pairs(seen_sources) do
        local e = {name = s}
        if s:sub(1, 1) == '@' then
            e.name = s:sub(2):match('[^/\\]+$') or s:sub(2)
            e.path = s:sub(2)
        end
        srcs[#srcs + 1] = e
    end
    send_response(req, {sources = srcs})
end

------------------------------------------------------------------------
-- message dispatch
------------------------------------------------------------------------
local function handle_message(data)
    local ok, req = pcall(json.decode, data)
    if not ok or type(req) ~= 'table' or req.type ~= 'request' then
        return
    end
    local handler = request_handlers[req.command]
    if not handler then
        send_response(req, nil, false,
                      'unsupported request: ' .. tostring(req.command))
        return
    end
    local ok2, resume = pcall(handler, req, req.arguments or {})
    if not ok2 then
        send_response(req, nil, false,
                      'handler error: ' .. tostring_safe(resume))
        return
    end
    return resume
end

local function drain_messages()
    for _ = 1, 32 do
        local data = recv_raw(0)
        if data == '' then break end
        if not active then activate() end
        handle_message(data)
    end
end

------------------------------------------------------------------------
-- the debug loop: entered from the hook while stopped
------------------------------------------------------------------------
local function debug_loop(reason, co)
    stopped_co = co
    var_refs = {}
    next_var_ref = 1
    frame_refs = {}
    next_frame_id = 1

    send_event('stopped', {
        reason = reason,
        threadId = thread_id(co),
        allThreadsStopped = true,
    })

    while active and is_connected() do
        local data = recv_raw(-1)
        if data == '' then break end   -- disconnect or plugin shutdown
        if handle_message(data) then break end
    end

    stopped_co = nil
end

------------------------------------------------------------------------
-- the hook
------------------------------------------------------------------------
local in_hook = false
local poll_counter = 0

local function hook_impl(event, line, info)
    local co = coroutine.running()
    -- info describes the frame that fired this event; if it is debugger code
    -- (e.g. the coroutine.create wrapper or captured_print running under a
    -- user call), skip the event entirely.
    if not info or info.source == DBG_SOURCE then return end
    seen_sources[info.source] = true

    poll_counter = poll_counter + 1
    if poll_counter >= POLL_INTERVAL then
        poll_counter = 0
        drain_messages()
        if not active then return end
    end

    if pause_requested then
        pause_requested = false
        stopped_depth = stack_depth(co)
        debug_loop('pause', co)
        return
    end

    if stepping and stepping.co == co then
        -- depths compared here are measured with hook_impl on the stack, so
        -- stopped_depth is recorded at this same call site for consistency
        local d = stack_depth(co)
        local hit = (stepping.mode == 'in') or
                    (stepping.mode == 'over' and d <= stepping.depth) or
                    (stepping.mode == 'out' and d < stepping.depth)
        if hit then
            stepping = nil
            stopped_depth = d
            debug_loop('step', co)
            return
        end
    end

    if event == 'line' and next(breakpoints) ~= nil then
        local bp_path = match_bp_path(info.source)
        if bp_path and breakpoints[bp_path][line] then
            stopped_depth = stack_depth(co)
            debug_loop('breakpoint', co)
        end
    end
end

local function hook_fn(event, line)
    if in_hook or not active then return end
    in_hook = true
    -- the frame that triggered this event sits directly below this hook
    -- function (level 0 is the getinfo call itself, level 1 is hook_fn)
    local info = debug.getinfo(2, 'nSl')
    local ok, err = pcall(hook_impl, event, line, info)
    in_hook = false
    if not ok then
        pcall(send_event, 'output', {
            category = 'stderr',
            output = 'luadebugger: internal error: ' ..
                     tostring_safe(err) .. '\n',
        })
        deactivate()
    end
end

------------------------------------------------------------------------
-- session lifecycle
------------------------------------------------------------------------
local function install_thread_hook(co)
    co = co or main_thread
    debug.sethook(co, hook_fn, 'l')
    register_thread(co)
end

local function wrap_body(fn)
    return function(...)
        if active then
            install_thread_hook(coroutine.running())
        end
        return fn(...)
    end
end

local function patch_entry_points()
    saved.create = coroutine.create
    coroutine.create = function(f, ...)
        local co = saved.create(function(...)
            install_thread_hook(coroutine.running())
            return f(...)
        end)
        register_thread(co)
        return co
    end
    saved.wrap = coroutine.wrap
    coroutine.wrap = function(f, ...)
        -- wrap() does not expose the thread, so it must self-register inside
        return saved.wrap(function(...)
            local me = coroutine.running()
            install_thread_hook(me)
            register_thread(me)
            return f(...)
        end)
    end
    -- These dfhack functions are the bodies of coroutines created by core C++
    -- (Lua::NewCoroutine via RunCoreQueryLoop); wrapping them hooks those
    -- threads at startup.
    for _, field in ipairs{'run_script', 'enable_script', 'interpreter'} do
        local fn = dfhack[field]
        if type(fn) == 'function' then
            saved['dfhack.' .. field] = fn
            dfhack[field] = wrap_body(fn)
        end
    end
    -- Redirect print output to the debug console while a session is active.
    saved.print = dfhack.BASE_G.print
    saved.dfhack_print = dfhack.print
    local function captured_print(...)
        local parts = {}
        for i = 1, select('#', ...) do
            parts[#parts + 1] = tostring_safe(select(i, ...))
        end
        send_event('output', {
            category = 'stdout',
            output = table.concat(parts, '\t') .. '\n',
        })
        saved.print(...)
    end
    dfhack.BASE_G.print = captured_print
    dfhack.print = captured_print
end

local function unpatch_entry_points()
    if saved.create then coroutine.create = saved.create end
    if saved.wrap then coroutine.wrap = saved.wrap end
    for _, field in ipairs{'run_script', 'enable_script', 'interpreter'} do
        local k = 'dfhack.' .. field
        if saved[k] then dfhack[field] = saved[k] end
    end
    if saved.print then dfhack.BASE_G.print = saved.print end
    if saved.dfhack_print then dfhack.print = saved.dfhack_print end
    saved = {}
end

function activate()
    if active then return end
    active = true
    patch_entry_points()
    install_thread_hook(main_thread)
    send_event('output', {
        category = 'console',
        output = 'luadebugger: debug session started\n',
    })
end

function deactivate()
    if not active then return end
    active = false
    debug.sethook()                       -- clear hook on current (main) thread
    for co in pairs(thread_ids) do
        pcall(debug.sethook, co)          -- clear hooks on coroutines
    end
    unpatch_entry_points()
    pause_requested = false
    stepping = nil
    stopped_co = nil
    -- thread/frame/variable ids are session-scoped
    thread_ids = setmetatable({}, {__mode='k'})
    next_thread_id = 0
end

-- Called once per frame from the plugin's onupdate whenever the transport has
-- inbound traffic or a client state change. in_hook suppresses the line hook
-- for the duration: functions the debuggee itself calls (json decode/encode,
-- client callbacks) are ordinary Lua code and would otherwise trigger hook
-- events that recursively drain the mailbox and reorder message handling.
function poll()
    if in_hook then return end
    in_hook = true
    local ok, err = pcall(function()
        if not is_connected() then
            deactivate()
            return
        end
        for _ = 1, 64 do
            local data = recv_raw(0)
            if data == '' then break end
            if not active then activate() end
            handle_message(data)
        end
    end)
    in_hook = false
    if not ok then
        pcall(send_event, 'output', {
            category = 'stderr',
            output = 'luadebugger: poll error: ' ..
                     tostring_safe(err) .. '\n',
        })
        deactivate()
    end
end

return _ENV
