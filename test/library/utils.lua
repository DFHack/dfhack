local utils = require 'utils'

function test.OrderedTable()
    local t = utils.OrderedTable()
    local keys = {'a', 'c', 'e', 'd', 'b', 'q', 58, -1.2}
    for i = 1, #keys do
        t[keys[i]] = i
    end
    local i = 1
    for k, v in pairs(t) do
        expect.eq(k, keys[i], 'key order')
        expect.eq(v, i, 'correct value')
        i = i + 1
    end
end

function test.invert()
    local t = {}
    local i = utils.invert{'a', 4.4, false, true, 5, t}
    expect.eq(i.a, 1)
    expect.eq(i[4.4], 2)
    expect.eq(i[false], 3)
    expect.eq(i[true], 4)
    expect.eq(i[5], 5)
    expect.eq(i[t], 6)
    expect.eq(i[700], nil)
    expect.eq(i.foo, nil)
    expect.eq(i[{}], nil)
end

function test.invert_nil()
    local i = utils.invert{'a', nil, 'b'}
    expect.eq(i.a, 1)
    expect.eq(i[nil], nil)
    expect.eq(i.b, 3)
end

function test.invert_overwrite()
    local i = utils.invert{'a', 'b', 'a'}
    expect.eq(i.b, 2)
    expect.eq(i.a, 3)
end

function test.processArgsGetopt_happy_path()
    local quiet, verbose, name

    local function process(args, expected_q, expected_v, expected_n)
        quiet, verbose, name = false, false, nil
        local nonoptions = utils.processArgsGetopt(args, {
            {'q', handler=function() quiet = true end},
            {'v', 'verbose', handler=function() verbose = true end},
            {'n', 'name', hasArg=true,
                handler=function(optarg) name = optarg end},
        })
        expect.eq(expected_q, quiet)
        expect.eq(expected_v, verbose)
        expect.eq(expected_n, name)
        return nonoptions
    end

    local args = {}
    expect.table_eq({}, process(args, false, false, nil))

    args = {'-q'}
    expect.table_eq({}, process(args, true, false, nil))

    args = {'-v'}
    expect.table_eq({}, process(args, false, true, nil))

    args = {'--verbose'}
    expect.table_eq({}, process(args, false, true, nil))

    args = {'-n', 'foo'}
    expect.table_eq({}, process(args, false, false, 'foo'))

    args = {'-n', 'foo'}
    expect.table_eq({}, process(args, false, false, 'foo'))

    args = {'-nfoo'}
    expect.table_eq({}, process(args, false, false, 'foo'))

    args = {'--name', 'foo'}
    expect.table_eq({}, process(args, false, false, 'foo'))

    args = {'--name=foo'}
    expect.table_eq({}, process(args, false, false, 'foo'))

    args = {'-vqnfoo'}
    expect.table_eq({}, process(args, true, true, 'foo'))

    args = {'nonopt1', '-nfoo', 'nonopt2', '-1', '-10', '-0v'}
    expect.table_eq({'nonopt1', 'nonopt2', '-1', '-10', '-0v'},
                    process(args, false, false, 'foo'))

    args = {'nonopt1', '--', '-nfoo', '--nonopt2', 'nonopt3'}
    expect.table_eq({'nonopt1', '-nfoo', '--nonopt2', 'nonopt3'},
                    process(args, false, false, nil))
end

function test.processArgsGetopt_action_errors()
    expect.error_match('missing option letter',
        function() utils.processArgsGetopt({}, {{handler=function() end}}) end)

    expect.error_match('missing option letter',
        function() utils.processArgsGetopt({}, {{'notoneletter'}}) end)

    expect.error_match('missing option letter',
        function() utils.processArgsGetopt({}, {{function() end}}) end)

    expect.error_match('handler missing',
        function() utils.processArgsGetopt({}, {{'r'}}) end)
end

function test.processArgsGetopt_parsing_errors()
    expect.error_match('Unknown option',
                       function() utils.processArgsGetopt({'-abc'},
                                    {{'a', handler=function() end}})
                       end,
                       'use undefined short option')

    expect.error_match('Unknown option',
                       function() utils.processArgsGetopt({'--abc'},
                                    {{'a', handler=function() end}})
                       end,
                       'use undefined long option')

    expect.error_match('Bad usage',
                       function() utils.processArgsGetopt({'--ab=c'},
                                    {{'a', 'ab', handler=function() end}})
                       end,
                       'pass value to param that does not take one')

    expect.error_match('Missing value',
                       function() utils.processArgsGetopt({'-a'},
                                    {{'a', 'ab', hasArg=true,
                                      handler=function() end}})
                       end,
                       'fail to pass value to short param that requires one')

    expect.error_match('Missing value',
                       function() utils.processArgsGetopt({'--ab'},
                                    {{'a', 'ab', hasArg=true,
                                      handler=function() end}})
                       end,
                       'fail to pass value to long param that requires one')
end
