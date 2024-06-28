-- tests string functions added by dfhack.lua

config.target = 'core'

function test.startswith()
    expect.true_(('abcd'):startswith(''))
    expect.true_(('abcd'):startswith('abc'))
    expect.false_(('abcd'):startswith('bcd'))
    expect.false_(('abcd'):startswith('abcde'))

    expect.true_((''):startswith(''))
    expect.false_((''):startswith('a'))

    expect.false_(('str'):startswith('.'),
                  'ensure we match literals, not patterns')
end

function test.endswith()
    expect.true_(('abcd'):endswith(''))
    expect.true_(('abcd'):endswith('bcd'))
    expect.false_(('abcd'):endswith('abc'))
    expect.false_(('abcd'):endswith('zabcd'))

    expect.true_((''):endswith(''))
    expect.false_((''):endswith('a'))

    expect.false_(('str'):endswith('.'),
                  'ensure we match literals, not patterns')
end

function test.split()
    expect.table_eq({'hello','world'}, ('hello world'):split())
    expect.table_eq({'hello','','world'}, ('hello  world'):split())
    expect.table_eq({'hello','world'}, ('hello   world'):split(' +'))

    expect.table_eq({'hello','world'}, ('hello.world'):split('.', true),
                    'ensure literal interpretation when plain is true')
    expect.table_eq({'', '', '', ''}, ('abc'):split('.'),
                    'ensure pattern interpretation when plain is false')

    -- we don't actually care what this returns, just that it does return
    expect.true_(('hello world'):split('.*'), 'ensure no infinite loop')

    expect.table_eq({'hello ', ' world'}, ('hello , world'):split(','),
                    'ensure spaces are kept when they are not the delimiter')
    expect.table_eq({'hello'}, ('hello'):split(), 'no delimiter')
end

function test.trim()
    expect.eq('hello', ('hello'):trim())
    expect.eq('hello', (' hello'):trim())
    expect.eq('hello', ('hello '):trim())
    expect.eq('hello', ('  hello  '):trim())
    expect.eq('', (''):trim())
    expect.eq('', (' '):trim())
    expect.eq('', ('  \t  \n  \v  '):trim())
    expect.eq('hel  lo', ('  hel  lo  '):trim(), 'keep interior spaces')
    expect.eq('hel \n lo', ('  hel \n lo  '):trim(),
              'keep interior spaces across newlines')
end

function test.wrap()
    expect.eq('', (''):wrap())
    expect.eq('  ', ('  '):wrap())
    expect.eq('   \n ', ('    '):wrap(3))
    expect.eq('   \n   \n ', ('       '):wrap(3))

    expect.eq('hello world', ('hello world'):wrap(20))
    expect.eq('hello   world', ('hello   world'):wrap(20))
    expect.eq('  hello world  ', ('  hello world  '):wrap(20))
    expect.eq('  hello   world  ', ('  hello   world  '):wrap(20))
    expect.eq('hello world \nhow are you?',('hello world how are you?'):wrap(12))
    expect.eq('hello\n \nworld', ('hello world'):wrap(5))
    expect.eq('hello\n     \n   \nworld', ('hello        world'):wrap(5))
    expect.eq('  \nhello\n  \nworld\n  ', ('  hello  world  '):wrap(5))
    expect.eq('hello \nworld', ('hello world'):wrap(8))
    expect.eq('hel\nlo \nwor\nld', ('hello world'):wrap(3))
    expect.eq('hel\nloo\n  \nwor\nldo', ('helloo  worldo'):wrap(3))

    expect.table_eq({'hel', 'lo ', 'wor', 'ld'}, ('hello world'):wrap(3, true))

    expect.error_match('expected width > 0', function() ('somestr'):wrap(0) end)
end

function test.escape_pattern()
    -- no change expected
    expect.eq('', (''):escape_pattern())
    expect.eq(' ', (' '):escape_pattern())
    expect.eq('abc', ('abc'):escape_pattern())
    expect.eq('a,b', ('a,b'):escape_pattern())
    expect.eq('"a,b"', ('"a,b"'):escape_pattern())

    -- excape regex chars
    expect.eq('iz for me%?', ('iz for me?'):escape_pattern())
    expect.eq('%.%*', ('.*'):escape_pattern())
    expect.eq('%( %) %. %% %+ %- %* %? %[ %] %^ %$',
              ('( ) . % + - * ? [ ] ^ $'):escape_pattern())
end
