local _ENV = mkmodule('argparse')

local getopt = require('3rdparty.alt_getopt')
local guidm = require('gui.dwarfmode')

function processArgs(args, validArgs)
    --[[
    standardized argument processing for scripts
    -argName value
    -argName [list of values]
    -argName [list of [nested values] -that can be [whatever] format of matched square brackets]
    -arg1 \-arg3
        escape sequences
    --]]
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
            elseif string.sub(arg,1,1) == '\\' then
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
        elseif string.sub(arg,1,1) == '-' then
            argName = string.sub(arg,2)
            if validArgs and not validArgs[argName] then
                error('error: invalid arg: ' .. i .. ': ' .. argName)
            end
            if result[argName] then
                error('duplicate arg: ' .. i .. ': ' .. argName)
            end
            if i+1 > #args or string.sub(args[i+1],1,1) == '-' then
                result[argName] = ''
                argName = nil
            else
                result[argName] = {}
            end
        else
            error('error parsing arg ' .. i .. ': ' .. arg)
        end
    end
    return result
end

-- processes commandline options according to optionActions and returns all
-- argument strings that are not options. Options and non-option strings can
-- appear in any order, and single-letter options that do not take arguments
-- can be combined into a single option string (e.g. '-abc' is the same as
-- '-a -b -c' if options 'a' and 'b' do not take arguments.
--
-- Numbers cannot be options and negative numbers (e.g. -10) will be interpreted
-- as positional parameters and returned in the nonoptions list.
--
-- optionActions is a vector with elements in the following format:
-- {shortOptionName, longOptionAlias, hasArg=boolean, handler=fn}
-- shortOptionName and handler are required. If the option takes an argument,
-- it will be passed to the handler function.
-- longOptionAlias is optional.
-- hasArgument defaults to false.
--
-- example usage:
--
-- local filename = nil
-- local open_readonly = false
-- local nonoptions = processArgsGetopt(args, {
--   {'r', handler=function() open_readonly = true end},
--   {'f', 'filename', hasArg=true,
--    handler=function(optarg) filename = optarg end}
-- })
--
-- when args is {'first', '-f', 'fname', 'second'} or, equivalently,
-- {'first', '--filename', 'fname', 'second'} (note the double dash in front of
-- the long option alias), then filename will be fname and nonoptions will
-- contain {'first', 'second'}.
function processArgsGetopt(args, optionActions)
    local sh_opts, long_opts = '', {}
    local handlers = {}
    for _,optionAction in ipairs(optionActions) do
        local sh_opt,long_opt = optionAction[1], optionAction[2]
        if not sh_opt or type(sh_opt) ~= 'string' or #sh_opt ~= 1 then
            error('optionAction missing option letter at index 1')
        end
        if not optionAction.handler then
            error(string.format('handler missing for option "%s"', sh_opt))
        end
        sh_opts = sh_opts .. sh_opt
        if optionAction.hasArg then sh_opts = sh_opts .. ':' end
        handlers[sh_opt] = optionAction.handler
        if long_opt then
            long_opts[long_opt] = sh_opt
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

-- Parses a comma-separated sequence of strings and returns a lua list. Spaces
-- are trimmed from the strings. If <arg_name> is specified, it is used to make
-- error messages more useful. If <list_length> is specified and greater than 0,
-- exactly that number of elements must be found or the function will error.
-- Example:
--   stringSequence('hello , world,list', 'words') => {'hello', 'world', 'list'}
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

-- Parses a comma-separated sequence of numeric strings and returns a list of
-- the discovered numbers (as numbers, not strings). If <arg_name> is specified,
-- it is used to make error messages more useful. If <list_length> is specified
-- and greater than 0, exactly that number of elements must be found or the
-- function will error. Example:
--   numericSequence('10, -20 ,  30.5') => {10, -20, 30.5}
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

-- Parses a comma-separated coordinate string and returns a coordinate table of
-- {x=x, y=y, z=z}. If the string 'here' is passed, returns the coordinates of
-- the active game cursor, or throws an error if the cursor is not active. This
-- function also verifies that the coordinates are valid for the current map and
-- throws if they are not (unless <skip_validation> is set to true).
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
