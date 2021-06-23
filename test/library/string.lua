-- tests string functions added by dfhack.lua

function test.startswith()
    expect.true_(('abcd'):startswith(''))
    expect.true_(('abcd'):startswith('abc'))
    expect.false_(('abcd'):startswith('bcd'))
end

function test.endswith()
    expect.true_(('abcd'):endswith(''))
    expect.true_(('abcd'):endswith('bcd'))
    expect.false_(('abcd'):endswith('abc'))
end

function test.wrap()
    expect.eq('hello world', ('hello world'):wrap(20))
    expect.eq('hello   world', ('hello   world'):wrap(20))
    expect.eq('hello world\nhow are you?',('hello world how are you?'):wrap(12))
    expect.eq('hello\nworld', ('hello world'):wrap(5))
    expect.eq('hello\nworld', ('hello        world'):wrap(5))
    expect.eq('hello\nworld', ('hello world'):wrap(8))
    expect.eq('hel\nlo\nwor\nld', ('hello world'):wrap(3))
    expect.eq('hel\nloo\nwor\nldo', ('helloo  worldo'):wrap(3))
end
