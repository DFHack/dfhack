local _ENV = mkmodule('json')
local internal = require 'json.internal'

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

function encode_file(data, path, overwrite, ...)
    if dfhack.filesystem.exists(path) and not overwrite then
        error('File exists: ' .. path)
    end
    if dfhack.filesystem.isdir(path) then
        error('Is a directory: ' .. path)
    end
    local contents = encode(data, ...)
    local f = io.open(path, 'w')
    f:write(contents)
    f:close()
end

function decode(data, msg)
    return internal:decode(data, msg)
end

function decode_file(path, ...)
    local f = io.open(path)
    if not f then
        error('Could not open ' .. path)
    end
    local contents = f:read('*all')
    f:close()
    return decode(contents, ...)
end

return _ENV
