-- Simple binary patch with IDA dif file support.

local _ENV = mkmodule('binpatch')

local function load_patch(name)
    local filename = name
    if not string.match(filename, '[./\\]') then
        filename = dfhack.getHackPath()..'/patches/'..dfhack.getDFVersion()..'/'..name..'.dif'
    end

    local file, err = io.open(filename, 'r')
    if not file then
        if string.match(err, ': No such file or directory') then
            return nil, 'patch not found'
        end
    end

    local old_bytes = {}
    local new_bytes = {}

    for line in file:lines() do
        if string.match(line, '^%x+:') then
            local offset, oldv, newv = string.match(line, '^(%x+):%s*(%x+)%s+(%x+)%s*$')
            if not offset then
                file:close()
                return nil, 'could not parse: '..line
            end

            offset, oldv, newv = tonumber(offset,16), tonumber(oldv,16), tonumber(newv,16)
            if oldv > 255 or newv > 255 then
                file:close()
                return nil, 'invalid byte values: '..line
            end

            old_bytes[offset] = oldv
            new_bytes[offset] = newv
        end
    end

    return { name = name, old_bytes = old_bytes, new_bytes = new_bytes }
end

local function rebase_table(input)
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

local function rebase_patch(patch)
    local nold, err = rebase_table(patch.old_bytes)
    if not nold then return nil, err end
    local nnew, err = rebase_table(patch.new_bytes)
    if not nnew then return nil, err end
    return { name = patch.name, old_bytes = nold, new_bytes = nnew }
end

BinaryPatch = defclass(BinaryPatch)

BinaryPatch.ATTRS {
    name = DEFAULT_NIL,
    old_bytes = DEFAULT_NIL,
    new_bytes = DEFAULT_NIL,
}

function load_dif_file(name)
    local patch, err = load_patch(name)
    if not patch then return nil, err end

    local rpatch, err = rebase_patch(patch)
    if not rpatch then return nil, err end

    return BinaryPatch(rpatch)
end

function BinaryPatch:status()
    local old_ok, err, addr = dfhack.internal.patchBytes({}, self.old_bytes)
    if old_ok then
        return 'removed'
    elseif dfhack.internal.patchBytes({}, self.new_bytes) then
        return 'applied'
    else
        return 'conflict', addr
    end
end

function BinaryPatch:isApplied()
    return dfhack.internal.patchBytes({}, self.new_bytes)
end

function BinaryPatch:apply()
    local ok, err, addr = dfhack.internal.patchBytes(self.new_bytes, self.old_bytes)
    if ok then
        return true, 'applied the patch'
    elseif dfhack.internal.patchBytes({}, self.new_bytes) then
        return true, 'patch is already applied'
    else
        return false, string.format('conflict at address %x', addr)
    end
end

function BinaryPatch:isRemoved()
    return dfhack.internal.patchBytes({}, self.old_bytes)
end

function BinaryPatch:remove()
    local ok, err, addr = dfhack.internal.patchBytes(self.old_bytes, self.new_bytes)
    if ok then
        return true, 'removed the patch'
    elseif dfhack.internal.patchBytes({}, self.old_bytes) then
        return true, 'patch is already removed'
    else
        return false, string.format('conflict at address %x', addr)
    end
end

return _ENV
