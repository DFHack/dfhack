-- Driver for the end-to-end test (e2e.py). Runs the real
-- plugins/lua/luadebugger.lua under a plain lua 5.3 interpreter and
-- transports DAP message bodies over stdin/stdout, one message per line
-- (json bodies never contain raw newlines).
--
-- A stdin line that does not parse as a DAP request is treated as a driver
-- command:
--   RUN <path>   loadfile() the target and resume it in a new coroutine

local DIR = arg[0]:match('^(.*)[/\\][^/\\]*$') or '.'

local real_print = print
local real_write = io.write
dfhack = { BASE_G=_G, is_core_context=true, run_script=function(n) return n end,
    enable_script=function(n,e) return e end, interpreter=function() return nil end }
mkmodule = function(n) return setmetatable({},{__index=_G}) end
local json = dofile(DIR .. '/json_stub.lua')
local real_require = require
require = function(n) if n=='json' then return json end return real_require(n) end

local inbound = {}
local connected = true

function debugger_recv(timeout_ms)
    if timeout_ms and timeout_ms < 0 then
        -- blocking read used by the debug loop
        return io.stdin:read('*l') or ''
    end
    return table.remove(inbound, 1) or ''
end
function debugger_send(msg)
    io.stdout:write(msg, '\n')
    io.stdout:flush()
    return true
end
function debugger_connected() return connected end
function debugger_port() return 43030 end

local mod = dofile(DIR .. '/../../plugins/lua/luadebugger.lua')

local function run_target(path)
    local fn, err = loadfile(path)
    if not fn then
        real_write('driver error: ', tostring(err), '\n')
        return
    end
    local co = coroutine.create(fn)
    local ok, res = coroutine.resume(co)
    real_write('driver: target finished, ok=', tostring(ok),
               ' res=', tostring(res), '\n')
end

while true do
    local line = io.stdin:read('*l')
    if line == nil then break end
    if line:sub(1, 4) == 'RUN ' then
        run_target(line:sub(5))
    elseif line ~= '' then
        inbound[#inbound + 1] = line
        mod.poll()
    end
end
