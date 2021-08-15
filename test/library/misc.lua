-- tests misc functions added by dfhack.lua

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

    local iterated = 0
    local t = {a='hello', b='world', [1]='extra'}
    for k,v in safe_pairs(t) do
        expect.eq(t[k], v)
        iterated = iterated + 1
    end
    expect.eq(3, iterated)
end
