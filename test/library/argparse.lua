config.target = 'core'

local argparse = require('argparse')
local guidm = require('gui.dwarfmode')

function test.processArgs()
    local validArgs = {opt1=true, opt2=true}

    expect.table_eq({}, argparse.processArgs({}, validArgs))
    expect.table_eq({opt1=''}, argparse.processArgs({'-opt1'}, validArgs))
    expect.table_eq({opt1=''}, argparse.processArgs({'--opt1'}, validArgs))

    expect.table_eq({opt1='arg'},
                    argparse.processArgs({'-opt1', 'arg'}, validArgs))
    expect.table_eq({opt1='arg'},
                    argparse.processArgs({'--opt1', 'arg'}, validArgs))

    expect.table_eq({opt1='', opt2=''},
                    argparse.processArgs({'--opt1', '-opt2'}, validArgs))
    expect.table_eq({opt1='', opt2=''},
                    argparse.processArgs({'--opt1', '--opt2'},validArgs))

    expect.table_eq({opt1='', opt2='arg'},
                    argparse.processArgs({'--opt1', '-opt2', 'arg'}, validArgs))
    expect.table_eq({opt1='', opt2='arg'},
                    argparse.processArgs({'--opt1', '--opt2', 'arg'},validArgs))

    expect.table_eq({opt1={}},
                    argparse.processArgs({'-opt1', '[', ']'}, validArgs))
    expect.table_eq({opt1={'a'}},
                    argparse.processArgs({'--opt1', '[', 'a', ']'}, validArgs))
    expect.table_eq({opt1={'a', '[', 'nested', 'string', ']'}},
                    argparse.processArgs({'-opt1', '[', 'a', '[', 'nested',
                                          'string', ']', ']'},
                                         validArgs))

    expect.table_eq({opt1='-value'},
                     argparse.processArgs({'-opt1', '\\-value'}, validArgs))
    expect.table_eq({opt1='--value'},
                     argparse.processArgs({'-opt1', '\\--value'}, validArgs))

    expect.table_eq({unvalidated_opt='value'},
                     argparse.processArgs({'-unvalidated_opt', 'value'}, nil))

    expect.error_match('invalid arg',
            function() argparse.processArgs({'-opt3'}, validArgs) end)
    expect.error_match('duplicate arg',
            function() argparse.processArgs({'-opt1', '--opt1'}, validArgs) end)
    expect.error_match('error parsing arg',
            function() argparse.processArgs({'justastring'}, validArgs) end)
end

function test.processArgsGetopt_happy_path()
    local quiet, verbose, name

    local function process(args, expected_q, expected_v, expected_n)
        quiet, verbose, name = false, false, nil
        local nonoptions = argparse.processArgsGetopt(args, {
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
    expect.error_match('at least one of sh_opt and long_opt',
        function()
            argparse.processArgsGetopt({}, {{handler=function() end}})
        end)

    expect.error_match('option letter not found',
        function() argparse.processArgsGetopt({}, {{'notoneletter'}}) end)

    expect.error_match('option letter not found',
        function() argparse.processArgsGetopt({}, {{function() end}}) end)

    expect.error_match('long option name',
        function() argparse.processArgsGetopt({}, {{'', ''}}) end)

    expect.error_match('long option name',
        function() argparse.processArgsGetopt({}, {{nil, ''}}) end)

    expect.error_match('long option name',
        function() argparse.processArgsGetopt({}, {{'a', ''}}) end)

    expect.error_match('handler missing',
        function() argparse.processArgsGetopt({}, {{'r'}}) end)
end

function test.processArgsGetopt_parsing_errors()
    expect.error_match('Unknown option',
                       function() argparse.processArgsGetopt({'-abc'},
                                    {{'a', handler=function() end}})
                       end,
                       'use undefined short option')

    expect.error_match('Unknown option',
                       function() argparse.processArgsGetopt({'--abc'},
                                    {{'a', handler=function() end}})
                       end,
                       'use undefined long option')

    expect.error_match('Bad usage',
                       function() argparse.processArgsGetopt({'--ab=c'},
                                    {{'a', 'ab', handler=function() end}})
                       end,
                       'pass value to param that does not take one')

    expect.error_match('Missing value',
                       function() argparse.processArgsGetopt({'-a'},
                                    {{'a', 'ab', hasArg=true,
                                      handler=function() end}})
                       end,
                       'fail to pass value to short param that requires one')

    expect.error_match('Missing value',
                       function() argparse.processArgsGetopt({'--ab'},
                                    {{'a', 'ab', hasArg=true,
                                      handler=function() end}})
                       end,
                       'fail to pass value to long param that requires one')
end

function test.processArgsGetopt_long_opt_without_short_opt()
    local var = false
    local nonoptions = argparse.processArgsGetopt(
                            {'--long'},
                            {{'', 'long', handler=function() var = true end}})
    expect.true_(var)
    expect.table_eq({}, nonoptions)

    var = false
    nonoptions = argparse.processArgsGetopt(
                    {'--long'},
                    {{nil, 'long', handler=function() var = true end}})
    expect.true_(var)
    expect.table_eq({}, nonoptions)
end

function test.stringList()
    expect.table_eq({'happy', 'path'}, argparse.stringList(' happy  ,   path'),
                    'ensure elements are trimmed')
    expect.table_eq({'empty', '', 'elem'}, argparse.stringList('empty,,elem'),
                    'ensure empty elements are preserved')

    expect.error_match('expected 5 elements',
                       function() argparse.stringList('a,b,c,d', '', 5) end,
                       'too few elements')
    expect.error_match('expected 5 elements',
                       function() argparse.stringList('a,b,c,d,e,f', '', 5) end,
                       'too many elements')
    expect.error_match('^expected',
                       function() argparse.stringList('', '', 5) end,
                       'no arg name printed when none supplied')
    expect.error_match('^argname',
                       function() argparse.stringList('', 'argname', 5) end,
                       'arg name printed when supplied')
end

function test.numberList()
    expect.table_eq({5, 4, 0, -1, 0.5, -0.5},
                    argparse.numberList(' 5,4,  0,  -1 , 000.5000, -0.50'),
                    'happy path')

    expect.error_match('invalid number',
                       function() argparse.numberList('1,b,3') end,
                       'letter not number')
    expect.error_match('invalid number',
                       function() argparse.numberList('1,,3') end,
                       'blank number')
    expect.error_match('invalid number',
                       function() argparse.numberList('1,2,') end,
                       'blank number at end')
    expect.error_match('invalid number',
                       function() argparse.numberList('1-1') end,
                       'bad number format')
    expect.error_match('^expected',
                       function() argparse.numberList('', '', 5) end,
                       'no arg name printed when none supplied')
    expect.error_match('^argname',
                       function() argparse.numberList('', 'argname', 5) end,
                       'arg name printed when supplied')
end

function test.coords()
    mock.patch(dfhack.maps, "isValidTilePos", mock.func(true),
        function()
            expect.table_eq({x=0, y=4, z=3}, argparse.coords('0,4 , 3'),
                            'happy path')

            expect.error_match('expected non%-negative integer',
                            function() argparse.coords('1,-2,3') end,
                            'negative coordinate')

            mock.patch(guidm, 'getCursorPos', mock.func({x=1, y=2, z=3}),
                function()
                    expect.table_eq({x=1, y=2, z=3}, argparse.coords('here'))
                end)
            mock.patch(guidm, 'getCursorPos', mock.func(),
                function()
                    expect.error_match('cursor is not active',
                                    function() argparse.coords('here') end,
                                    'inactive cursor')
                end)
        end)

    mock.patch(dfhack.maps, "isValidTilePos", mock.func(false),
        function()
            expect.error_match('not on current map',
                               function() argparse.coords('0,4,300') end)
            expect.table_eq({x=0, y=4, z=300},
                            argparse.coords('0,4,300', nil, true))

            mock.patch(guidm, 'getCursorPos', mock.func({x=1, y=2, z=300}),
                function()
                    expect.error_match('not on current map',
                                    function() argparse.coords('here') end)
                end)
            mock.patch(guidm, 'getCursorPos', mock.func({x=1, y=2, z=300}),
                function()
                    expect.table_eq({x=1, y=2, z=300},
                                    argparse.coords('here', nil, true))
                end)
        end)
end

function test.positiveInt()
    expect.eq(5, argparse.positiveInt(5))
    expect.eq(5, argparse.positiveInt('5'))
    expect.eq(5, argparse.positiveInt('5.0'))
    expect.eq(1, argparse.positiveInt('1'))

    expect.error_match('expected positive integer',
                       function() argparse.positiveInt('0') end)
    expect.error_match('expected positive integer',
                       function() argparse.positiveInt('5.01') end)
    expect.error_match('expected positive integer',
                       function() argparse.positiveInt(-1) end)
end

function test.nonnegativeInt()
    expect.eq(5, argparse.nonnegativeInt(5))
    expect.eq(5, argparse.nonnegativeInt('5'))
    expect.eq(5, argparse.nonnegativeInt('5.0'))
    expect.eq(1, argparse.nonnegativeInt('1'))
    expect.eq(0, argparse.nonnegativeInt('0'))
    expect.eq(0, argparse.nonnegativeInt('-0'))

    expect.error_match('expected non%-negative integer',
                       function() argparse.nonnegativeInt('-0.01') end)
    expect.error_match('expected non%-negative integer',
                       function() argparse.nonnegativeInt(-5) end)
    expect.error_match('expected non%-negative integer',
                       function() argparse.nonnegativeInt(-1) end)
end
