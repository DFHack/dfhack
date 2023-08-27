-- Displays changes in the key modifier state
--@ enable = true

active = active or false

if dfhack.internal.getModstate == nil or dfhack.internal.getModifiers == nil then
    qerror('Required internal functions are missing')
end

function usage()
    print(dfhack.script_help())
end

function set_timeout()
    dfhack.timeout(1, 'frames', check)
end

last_msg = last_msg or nil --as:string
function log(s, color)
    -- prevent duplicate output
    if s ~= last_msg then
        dfhack.color(color)
        print(s)
        last_msg = s
    end
end

last_modstate = last_modstate or nil --as:number
function check()
    local msg = ''
    local modstate = dfhack.internal.getModstate()
    if modstate ~= last_modstate then
        last_modstate = modstate
        for k, v in pairs(dfhack.internal.getModifiers()) do
            msg = msg .. k .. '=' .. (v and 1 or 0) .. ' '
        end
        log(msg)
    end
    if active then set_timeout() end
end

local args = {...}
if dfhack_flags and dfhack_flags.enable then
    table.insert(args, dfhack_flags.enable_state and 'enable' or 'disable')
end

if #args == 1 then
    if args[1] == 'enable' or args[1] == 'start' then
        set_timeout()
        active = true
    elseif args[1] == 'disable' or args[1] == 'stop' then
        active = false
    else
        usage()
    end
else
    usage()
end
