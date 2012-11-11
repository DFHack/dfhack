-- Apply or remove binary patches at runtime.

local utils = require('utils')

function load_patch(name)
    local filename = name
    local auto = false
    if not string.match(filename, '[./\\]') then
        auto = true
        filename = dfhack.getHackPath()..'/patches/'..dfhack.getDFVersion()..'/'..name..'.dif'
    end

    local file, err = io.open(filename, 'r')
    if not file then
        if auto and string.match(err, ': No such file or directory') then
            return nil, 'no patch '..name..' for '..dfhack.getDFVersion()
        else
            return nil, err
        end
    end

    local old_bytes = {}
    local new_bytes = {}

    for line in file:lines() do
        if string.match(line, '^%x+:') then
            local offset, oldv, newv = string.match(line, '^(%x+):%s*(%x+)%s+(%x+)%s*$')
            if not offset then
                file:close()
                return nil, 'Could not parse: '..line
            end

            offset, oldv, newv = tonumber(offset,16), tonumber(oldv,16), tonumber(newv,16)
            if oldv > 255 or newv > 255 then
                file:close()
                return nil, 'Invalid byte values: '..line
            end

            old_bytes[offset] = oldv
            new_bytes[offset] = newv
        end
    end

    return { name = name, old_bytes = old_bytes, new_bytes = new_bytes }
end

function rebase_table(input)
    local output = {}
    local base = dfhack.internal.getImageBase()
    for k,v in pairs(input) do
        local offset = dfhack.internal.adjustOffset(k)
        if not offset then
            return nil, string.format('invalid offset: %x', k)
        end
        output[base + offset] = v
    end
    return output
end

function rebase_patch(patch)
    local nold, err = rebase_table(patch.old_bytes)
    if not nold then return nil, err end
    local nnew, err = rebase_table(patch.new_bytes)
    if not nnew then return nil, err end
    return { name = patch.name, old_bytes = nold, new_bytes = nnew }
end

function run_command(cmd,name)
    local patch, err = load_patch(name)
    if not patch then
        dfhack.printerr('Could not load: '..err)
        return
    end

    local rpatch, err = rebase_patch(patch)
    if not rpatch then
        dfhack.printerr(name..': '..err)
        return
    end

    if cmd == 'check' then
        local old_ok, err, addr = dfhack.internal.patchBytes({}, rpatch.old_bytes)
        if old_ok then
            print(name..': patch is not applied.')
        elseif dfhack.internal.patchBytes({}, rpatch.new_bytes) then
            print(name..': patch is applied.')
        else
            dfhack.printerr(string.format('%s: conflict at address %x', name, addr))
        end
    elseif cmd == 'apply' then
        local ok, err, addr = dfhack.internal.patchBytes(rpatch.new_bytes, rpatch.old_bytes)
        if ok then
            print(name..': applied the patch.')
        elseif dfhack.internal.patchBytes({}, rpatch.new_bytes) then
            print(name..': patch is already applied.')
        else
            dfhack.printerr(string.format('%s: conflict at address %x', name, addr))
        end
    elseif cmd == 'remove' then
        local ok, err, addr = dfhack.internal.patchBytes(rpatch.old_bytes, rpatch.new_bytes)
        if ok then
            print(name..': removed the patch.')
        elseif dfhack.internal.patchBytes({}, rpatch.old_bytes) then
            print(name..': patch is already removed.')
        else
            dfhack.printerr(string.format('%s: conflict at address %x', name, addr))
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
