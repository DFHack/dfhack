-- Apply or remove binary patches at runtime.

local bp = require('binpatch')

function run_command(cmd,name)
    local pfix = name..': '

    local patch, err = bp.load_dif_file(name)
    if not patch then
        dfhack.printerr(pfix..err)
        return
    end

    if cmd == 'check' then
        local status, addr = patch:status()
        if status == 'conflict' then
            dfhack.printerr(string.format('%sconflict at address %x', pfix, addr))
        else
            print(pfix..'patch is '..status)
        end
    elseif cmd == 'apply' then
        local ok, msg = patch:apply()
        if ok then
            print(pfix..msg)
        else
            dfhack.printerr(pfix..msg)
        end
    elseif cmd == 'remove' then
        local ok, msg = patch:remove()
        if ok then
            print(pfix..msg)
        else
            dfhack.printerr(pfix..msg)
        end
    else
        qerror('Invalid command: '..cmd)
    end
end

local cmd,name = ...
if not cmd or not name then
    qerror('Usage: binpatch check/apply/remove <patchname>')
end
run_command(cmd, name)
