-- tests misc functions added by dfhack.lua

config.target = 'core'

function test.safe_pairs()
    for k,v in safe_pairs(nil) do
        expect.fail('nil should not be iterable')
    end
    for k,v in safe_pairs('a') do
        expect.fail('a string should not be iterable')
    end
    for k,v in safe_pairs({}) do
        expect.fail('an empty table should not be iterable')
    end
    for k,v in safe_pairs(df.item._identity) do
        expect.fail('a non-iterable light userdata var should not be iterable')
    end

    local iterated = 0
    local t = {a='hello', b='world', [1]='extra'}
    for k,v in safe_pairs(t) do
        expect.eq(t[k], v)
        iterated = iterated + 1
    end
    expect.eq(3, iterated)
end

function test.safe_pairs_ipairs()
    local t = {1, 2}
    setmetatable(t, {
        __pairs = function()
            expect.fail('pairs() should not be called')
        end,
    })

    local iterated = 0
    for k,v in safe_pairs(t, ipairs) do
        expect.eq(k, v)
        iterated = iterated + 1
    end
    expect.eq(#t, iterated)
end
