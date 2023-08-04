config.target = 'core'

local expect_raw = require('test_util.expect')

function test.str_find()
    expect.true_(expect_raw.str_find('a ', 'a str', 'a comment'))

    local ok, comment, msg = expect_raw.str_find('ab', 'a str', 'a comment')
    expect.false_(ok)
    expect.eq('a comment', comment)
    expect.eq('pattern "ab" not matched in "a str"', msg)

    ok, _, msg = expect_raw.str_find('pattern', nil)
    expect.false_(ok)
    expect.eq('expected string, got nil', msg)

    ok, _, msg = expect_raw.str_find('pattern', {})
    expect.false_(ok)
    expect.eq('expected string, got table', msg)
end

function test.table_eq()
    expect.true_(expect_raw.table_eq({}, {}))
    expect.true_(expect_raw.table_eq({'a'}, {'a'}))
    expect.true_(expect_raw.table_eq({{'a', k='val'}, 'b'},
                                     {{'a', k='val'}, 'b'}))

    expect.false_(expect_raw.table_eq(nil, nil)) -- operands must be non-nil
    expect.false_(expect_raw.table_eq({}, nil))
    expect.false_(expect_raw.table_eq(nil, {}))
    expect.false_(expect_raw.table_eq({}, {''}))
    expect.false_(expect_raw.table_eq({''}, {}))
    expect.false_(expect_raw.table_eq({'a', {}}, {'a'}))
    expect.false_(expect_raw.table_eq({{'a', k='val1'}, 'b'},
                                      {{'a', k='val2'}, 'b'}))

    local tab = {a='a', b='b'}
    expect.true_(expect_raw.table_eq(tab, tab))
    expect.true_(expect_raw.table_eq({tab}, {tab}))

    local tab1, tab2 = {'a'}, {'a'}
    tab1.self, tab2.self = tab1, tab2
    expect.true_(expect_raw.table_eq(tab1, tab2))

    tab1.other, tab2.other = tab2, tab1
    expect.true_(expect_raw.table_eq(tab1, tab2))

    local tabA, tabB, tabC, tabD = {k='a'}, {k='a'}, {k='a'}, {k='a'}
    tabA.next, tabB.next, tabC.next, tabD.next = tabB, tabC, tabD, tabA
    expect.true_(expect_raw.table_eq(tabA, tabB))
end

function test.error_match()
    expect.true_(expect_raw.error_match('err', function() error('err0r') end))
    expect.false_(expect_raw.error_match('err', function() end))
    expect.false_(expect_raw.error_match('00', function() error('err0r') end))

    expect.true_(expect_raw.error_match(
            function() return true end, function() error('err0r') end))
    expect.false_(expect_raw.error_match(
            function() return false end, function() error('err0r') end))
end

function test.error()
    expect.true_(expect_raw.error(function() error('err0r') end))
    expect.true_(expect_raw.error(function() qerror('err0r') end))
    expect.false_(expect_raw.error(function() end))
end

function test.printerr_match_printerr_restored()
    local prev_printerr = dfhack.printerr
    expect_raw.printerr_match('.*', function() dfhack.printerr('a') end)
    expect.eq(prev_printerr, dfhack.printerr)
end

function test.printerr_match()
    local noprint = function() end
    local oneprint = function()
        dfhack.printerr('a')
    end

    expect.true_(expect_raw.printerr_match(nil, noprint))
    expect.true_(expect_raw.printerr_match({}, noprint))
    expect.false_(expect_raw.printerr_match(nil, oneprint))
    expect.false_(expect_raw.printerr_match({}, oneprint))

    expect.true_(expect_raw.printerr_match('a', oneprint))
    expect.true_(expect_raw.printerr_match({[1]='a'}, oneprint))
    expect.true_(expect_raw.printerr_match({a=1}, oneprint))
    expect.true_(expect_raw.printerr_match('.', oneprint))
    expect.true_(expect_raw.printerr_match({['.']=1}, oneprint))
    expect.true_(expect_raw.printerr_match('.*', oneprint))
    expect.true_(expect_raw.printerr_match({['.*']=1}, oneprint))
    expect.false_(expect_raw.printerr_match('b', oneprint))
    expect.false_(expect_raw.printerr_match({b=1}, oneprint))
    expect.false_(expect_raw.printerr_match({a=1,b=1}, oneprint))

    local twoprint = function()
        dfhack.printerr('a')
        dfhack.printerr('b')
    end
    expect.true_(expect_raw.printerr_match({'a','b'}, twoprint))
    expect.true_(expect_raw.printerr_match({a=1,b=1}, twoprint))
    expect.false_(expect_raw.printerr_match({'b','a'}, twoprint))
    expect.false_(expect_raw.printerr_match({a=1}, twoprint))
    expect.false_(expect_raw.printerr_match({b=1}, twoprint))
    expect.false_(expect_raw.printerr_match({a=1,b=2}, twoprint))
    expect.false_(expect_raw.printerr_match({a=1,b=1,c=1}, twoprint))

    local threeprint = function()
        dfhack.printerr('a')
        dfhack.printerr('b')
        dfhack.printerr('c')
    end
    expect.true_(expect_raw.printerr_match({a=1,b=1,c=1}, threeprint))

    local multiprint = function()
        dfhack.printerr('a')
        dfhack.printerr('b')
        dfhack.printerr('a')
    end
    expect.true_(expect_raw.printerr_match({a=2,b=1}, multiprint))
    expect.false_(expect_raw.printerr_match({a=1,b=1}, multiprint))

    local nilprint = function()
        dfhack.printerr()
    end
    expect.true_(expect_raw.printerr_match({}, nilprint))

    local nilaprint = function()
        dfhack.printerr()
        dfhack.printerr('a')
    end
    expect.true_(expect_raw.printerr_match({'a'}, nilaprint))
end
