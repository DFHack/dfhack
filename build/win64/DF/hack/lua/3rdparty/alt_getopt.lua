-- Copyright (c) 2009 Aleksey Cheusov <vle@gmx.net>
--
-- Permission is hereby granted, free of charge, to any person obtaining
-- a copy of this software and associated documentation files (the
-- "Software"), to deal in the Software without restriction, including
-- without limitation the rights to use, copy, modify, merge, publish,
-- distribute, sublicense, and/or sell copies of the Software, and to
-- permit persons to whom the Software is furnished to do so, subject to
-- the following conditions:
--
-- The above copyright notice and this permission notice shall be
-- included in all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
-- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
-- NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
-- LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
-- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
-- WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

-- based on https://github.com/LuaDist/alt-getopt/blob/master/alt_getopt.lua
-- MIT licence
-- modified to support negative numbers as non-options, to aggregate non-options
-- for return, and to call error/qerror instead of os.exit on error. can be used
-- directly or via the utils.processArgsGetopt() wrapper.
--
-- sh_opts should be in standard getopt format: a string of letters that
-- represent options, each followed by a colon if that option takes an argument.
-- e.g.: 'ak:hv' has three flags (options with no arguments): 'a', 'h', and 'v'
-- and one option that takes an argument: 'k'.
--
-- Options passed to the module to parse can be in any of the following formats:
--   -kVALUE, -k VALUE, --key=VALUE, --key VALUE
--   -abcd is equivalent to -a -b -c -d if none of them accept arguments.
--   -abckVALUE and -abck VALUE are also acceptable (where k is the only option
--      in the string that takes a value).
--
-- Note that arguments that have a number as the second character are
-- interpreted as positional parameters and not options. For example, the
-- following strings are never interpreted as options:
--   -10
--   -0
--   -1a

local _ENV = mkmodule('3rdparty.alt_getopt')

local function get_opt_map(opts)
    local i = 1
    local len = #opts
    local options = {}

    for short_opt, accept_arg in opts:gmatch('([%w%?])(:?)') do
        options[short_opt] = #accept_arg
    end

    return options
end

local function err_unknown_opt(opt)
    qerror(string.format('Unknown option "-%s%s"', #opt > 1 and '-' or '', opt))
end

-- resolve aliases into their canonical forms
local function canonicalize(options, opt)
    if not options[opt] then
        err_unknown_opt(opt)
    end

    while type(options[opt]) == 'string' do
        opt = options[opt]

        if not options[opt] then
             err_unknown_opt(opt)
        end
    end

    if type(options[opt]) ~= 'number' then
        error(string.format(
                'Option "%s" resolves to non-number for has_arg flag', opt))
    end

    return opt
end

local function has_arg(options, opt)
    return options[canonicalize(options, opt)] == 1
end

-- returns vectors for opts, optargs, and nonoptions
function get_ordered_opts(args, sh_opts, long_opts)
    local optind, count, opts, optargs, nonoptions = 1, 1, {}, {}, {}

    local options = get_opt_map(sh_opts)
    for k,v in pairs(long_opts) do
        options[k] = v
    end

    while optind <= #args do
        local a = args[optind]
        if a == '--' then
            optind = optind + 1
            break
        elseif a:sub(1, 2) == '--' then
            local pos = a:find('=', 1, true)
            if pos then
                local opt = a:sub(3, pos-1)
                if not has_arg(options, opt) then
                    qerror(string.format('Bad usage of option "%s"', a))
                end
                opts[count] = opt
                optargs[count] = a:sub(pos+1)
            else
                local opt = a:sub(3)
                opts[count] = opt
                if has_arg(options, opt) then
                    if optind == #args then
                        qerror(string.format(
                                'Missing value for option "%s"', a))
                    end
                    optargs[count] = args[optind+1]
                    optind = optind + 1
                end
            end
            count = count + 1
        elseif a:sub(1, 1) == '-' and not tonumber(a:sub(2, 2)) then
            local j
            for j=2,#a do
                local opt = canonicalize(options, a:sub(j, j))
                if not has_arg(options, opt) then
                    opts[count] = opt
                    count = count + 1
                elseif j == #a then
                    if optind == #args then
                        qerror(string.format(
                                'Missing value for option "-%s"', opt))
                    end
                    opts[count] = opt
                    optargs[count] = args[optind+1]
                    optind = optind + 1
                    count = count + 1
                else
                    opts[count] = opt
                    optargs[count] = a:sub(j+1)
                    count = count + 1
                    break
                end
            end
        else
            table.insert(nonoptions, args[optind])
        end
        optind = optind + 1
    end
    for i=optind,#args do
        table.insert(nonoptions, args[i])
    end
    return opts, optargs, nonoptions
end

-- returns a map of options to their optargs (or 1 if the option doesn't take an
-- argument), and a vector for nonoptions
function get_opts(args, sh_opts, long_opts)
    local ret = {}

    local opts,optargs,nonoptions = get_ordered_opts(args, sh_opts, long_opts)
    for i,v in ipairs(opts) do
        if optarg[i] then
            ret[v] = optarg[i]
        else
            ret[v] = 1
        end
    end

    return ret, nonoptions
end

return _ENV
