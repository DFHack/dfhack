-- Docs at https://docs.dfhack.org/en/stable/docs/Lua%20API.html#argparse

local _ENV = mkmodule('argparse')

local getopt = require('3rdparty.alt_getopt')
local guidm = require('gui.dwarfmode')

function processArgs(args, validArgs)
    local result = {}
    local argName
    local bracketDepth = 0
    for i,arg in ipairs(args) do
        if argName then
            if arg == '[' then
                if bracketDepth > 0 then
                    table.insert(result[argName], arg)
                end
                bracketDepth = bracketDepth+1
            elseif arg == ']' then
                bracketDepth = bracketDepth-1
                if bracketDepth > 0 then
                    table.insert(result[argName], arg)
                else
                    argName = nil
                end
            elseif arg:startswith('\\') then
                if bracketDepth == 0 then
                    result[argName] = string.sub(arg,2)
                    argName = nil
                else
                    table.insert(result[argName], string.sub(arg,2))
                end
            else
                if bracketDepth == 0 then
                    result[argName] = arg
                    argName = nil
                else
                    table.insert(result[argName], arg)
                end
            end
        elseif arg:startswith('-') then
            argName = string.sub(arg, arg:startswith('--') and 3 or 2)
            if validArgs and not validArgs[argName] then
                qerror('error: invalid arg: ' .. i .. ': ' .. argName)
            end
            if result[argName] then
                qerror('duplicate arg: ' .. i .. ': ' .. argName)
            end
            if i+1 > #args or args[i+1]:startswith('-') then
                result[argName] = ''
                argName = nil
            else
                result[argName] = {}
            end
        else
            qerror('error parsing arg ' .. i .. ': ' .. arg)
        end
    end
    return result
end

-- See online docs for full usage info.
--
-- Quick example:
--
-- local args = {...}
-- local open_readonly, filename = false, nil     -- set defaults
--
-- local positionals = argparse.processArgsGetopt(args, {
--   {'r', handler=function() open_readonly = true end},
--   {'f', 'filename', hasArg=true,
--    handler=function(optarg) filename = optarg end}
--   })
--
-- In this example, if args is {'first', '-rf', 'fname', 'second'} or,
-- equivalently, {'first', '-r', '--filename', 'myfile.txt', 'second'} (note the
-- double dash in front of the long option alias), then:
--   open_readonly == true
--   filename == 'myfile.txt'
--   positionals == {'first', 'second'}.
function processArgsGetopt(args, optionActions)
    local sh_opts, long_opts = '', {}
    local handlers = {}
    for _,optionAction in ipairs(optionActions) do
        local sh_opt,long_opt = optionAction[1], optionAction[2]
        if sh_opt and (type(sh_opt) ~= 'string'  or #sh_opt > 1) then
            error('option letter not found')
        end
        if long_opt and (type(long_opt) ~= 'string' or #long_opt == 0) then
            error('long option name must be a string with length >0')
        end
        if not sh_opt then
            sh_opt = ''
        end
        if not long_opt and #sh_opt == 0 then
            error('at least one of sh_opt and long_opt must be specified')
        end
        if not optionAction.handler then
            error(string.format('handler missing for option "%s"',
                                #sh_opt > 0 and sh_opt or long_opt))
        end
        if #sh_opt > 0 then
            sh_opts = sh_opts .. sh_opt
            if optionAction.hasArg then sh_opts = sh_opts .. ':' end
            handlers[sh_opt] = optionAction.handler
        end
        if long_opt then
            if #sh_opt > 0 then
                long_opts[long_opt] = sh_opt
            else
                long_opts[long_opt] = optionAction.hasArg and 1 or 0
            end
            handlers[long_opt] = optionAction.handler
        end
    end
    local opts, optargs, nonoptions =
            getopt.get_ordered_opts(args, sh_opts, long_opts)
    for i,v in ipairs(opts) do
        handlers[v](optargs[i])
    end
    return nonoptions
end

local function arg_error(arg_name, fmt, ...)
    local prefix = ''
    if arg_name and #arg_name > 0 then
        prefix = arg_name .. ': '
    end
    qerror(('%s'..fmt):format(prefix, ...))
end

function stringList(arg, arg_name, list_length)
    if not list_length then list_length = 0 end
    local list = arg:split(',')
    if list_length > 0 and #list ~= list_length then
        arg_error(arg_name,
                  'expected %d elements; found %d', list_length, #list)
    end
    for i,element in ipairs(list) do
        list[i] = element:trim()
    end
    return list
end

function numberList(arg, arg_name, list_length)
    local strings = stringList(arg, arg_name, list_length)
    for i,str in ipairs(strings) do
        local num = tonumber(str)
        if not num then
            arg_error(arg_name, 'invalid number: "%s"', str)
        end
        strings[i] = num
    end
    return strings
end

-- throws if val is not a nonnegative integer; otherwise returns val
local function check_nonnegative_int(val, arg_name)
    if not val or val < 0 or val ~= math.floor(val) then
        arg_error(arg_name,
                  'expected non-negative integer; got "%s"', tostring(val))
    end
    return val
end

function coords(arg, arg_name, skip_validation)
    if arg == 'here' then
        local cursor = guidm.getCursorPos()
        if not cursor then
            arg_error(arg_name,
                      '"here" was specified for coordinates, but the game' ..
                      ' cursor is not active!')
        end
        if not skip_validation and not dfhack.maps.isValidTilePos(cursor) then
            arg_error(arg_name, 'cursor coordinates not on current map!')
        end
        return cursor
    end
    local numbers = numberList(arg, arg_name, 3)
    local pos = xyz2pos(check_nonnegative_int(numbers[1]),
                        check_nonnegative_int(numbers[2]),
                        check_nonnegative_int(numbers[3]))
    if not skip_validation and not dfhack.maps.isValidTilePos(pos) then
        arg_error(arg_name,
                  'specified coordinates not on current map: "%s"', arg)
    end
    return pos
end

return _ENV
