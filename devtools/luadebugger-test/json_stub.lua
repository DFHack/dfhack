-- Minimal JSON encode/decode for the luadebugger test harness.
-- API-compatible with dfhack's json module (encode/decode).

local json = {}

local esc_map = {['"'] = '\\"', ['\\'] = '\\\\', ['\n'] = '\\n',
                 ['\r'] = '\\r', ['\t'] = '\\t', ['\b'] = '\\b',
                 ['\f'] = '\\f'}

local function enc_str(s)
    return '"' .. s:gsub('[%z\1-\31\\"]', function(c)
        return esc_map[c] or ('\\u%04x'):format(c:byte())
    end) .. '"'
end

local function enc(v)
    local t = type(v)
    if t == 'nil' then return 'null'
    elseif t == 'boolean' or t == 'number' then return tostring(v)
    elseif t == 'string' then return enc_str(v)
    elseif t == 'table' then
        if #v > 0 or next(v) == nil then
            local parts = {}
            for i = 1, #v do parts[i] = enc(v[i]) end
            return '[' .. table.concat(parts, ',') .. ']'
        end
        local parts = {}
        for k, val in pairs(v) do
            parts[#parts + 1] = enc_str(tostring(k)) .. ':' .. enc(val)
        end
        return '{' .. table.concat(parts, ',') .. '}'
    end
    error('json: cannot encode ' .. t)
end
json.encode = enc

function json.decode(s)
    local pos = 1
    local parse
    local function skipws()
        local _, e = s:find('^[ \t\n\r]+', pos)
        if e then pos = e + 1 end
    end
    local function str()
        local i = pos + 1
        local buf = {}
        while true do
            local c = s:sub(i, i)
            if c == '"' then pos = i + 1; break end
            if c == '\\' then
                local e2 = s:sub(i + 1, i + 1)
                local m = {n = '\n', t = '\t', r = '\r', b = '\b',
                           f = '\f', ['"'] = '"', ['\\'] = '\\',
                           ['/'] = '/'}
                if e2 == 'u' then
                    buf[#buf + 1] = '' -- approximate: drop unicode escapes
                    i = i + 6
                else
                    buf[#buf + 1] = m[e2] or e2
                    i = i + 2
                end
            else
                buf[#buf + 1] = c
                i = i + 1
            end
        end
        return table.concat(buf)
    end
    local function literal(word, val)
        if s:sub(pos, pos + #word - 1) == word then
            pos = pos + #word
            return val
        end
        error('bad literal at ' .. pos)
    end
    function parse()
        skipws()
        local c = s:sub(pos, pos)
        if c == '{' then
            pos = pos + 1
            local t = {}
            skipws()
            if s:sub(pos, pos) == '}' then pos = pos + 1; return t end
            while true do
                skipws()
                local k = str()
                skipws()
                assert(s:sub(pos, pos) == ':', 'expected : at ' .. pos)
                pos = pos + 1
                t[k] = parse()
                skipws()
                local d = s:sub(pos, pos)
                pos = pos + 1
                if d == '}' then return t end
            end
        elseif c == '[' then
            pos = pos + 1
            local t = {}
            skipws()
            if s:sub(pos, pos) == ']' then pos = pos + 1; return t end
            while true do
                t[#t + 1] = parse()
                skipws()
                local d = s:sub(pos, pos)
                pos = pos + 1
                if d == ']' then return t end
            end
        elseif c == '"' then return str()
        elseif c == 't' then return literal('true', true)
        elseif c == 'f' then return literal('false', false)
        elseif c == 'n' then return literal('null', nil)
        else
            local m = s:match('^[%d%.eE%+%-]+', pos)
            pos = pos + #m
            return tonumber(m)
        end
    end
    return parse()
end

return json
