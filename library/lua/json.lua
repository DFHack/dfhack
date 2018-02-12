local _ENV = mkmodule('json')
local internal = require 'json.internal'
local fs = dfhack.filesystem

encode_defaults = {
    -- For compatibility with jsonxx (C++)
    pretty = true,
    indent = '\t',
}

function encode(data, options, msg)
    local opts = {}
    for k, v in pairs(encode_defaults) do opts[k] = v end
    for k, v in pairs(options or {}) do opts[k] = v end
    return internal:encode(data, msg, opts)
end

function encode_file(data, path, ...)
    if fs.isdir(path) then
        error('Is a directory: ' .. path)
    end
    local contents = encode(data, ...)
    local f = io.open(path, 'w')
    if not f then
        error('Could not write to ' .. tostring(path))
    end
    f:write(contents)
    f:close()
end

function decode(data, msg)
    return internal:decode(data, msg)
end

function decode_file(path, ...)
    local f = io.open(path)
    if not f then
        error('Could not read from ' .. tostring(path))
    end
    local contents = f:read('*all')
    f:close()
    return decode(contents, ...)
end

local _file = defclass()
function _file:init(opts)
    self.path = opts.path
    self.data = {}
    self.exists = false
    self:read(opts.strict)
end

function _file:read(strict)
    if not fs.exists(self.path) then
        if strict then
            error('cannot read file: ' .. self.path)
        else
            self.data = {}
        end
    else
        self.exists = true
        self.data = decode_file(self.path)
    end
    return self.data
end

function _file:write(data)
    if data then
        self.data = data
    end
    encode_file(self.data, self.path)
    self.exists = true
end

function _file:__tostring()
    return ('<json file: %s>'):format(self.path)
end

function open(path, strict)
    return _file{path=path, strict=strict}
end

return _ENV
