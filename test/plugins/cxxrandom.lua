config.target = 'cxxrandom'

local rng = require('plugins.cxxrandom')

function test.cxxrandom_distributions()
    rng.normal_distribution:new(0,5)
    rng.real_distribution:new(-1,1)
    rng.int_distribution:new(-20,20)
    rng.bool_distribution:new(0.00000000001)
    rng.num_sequence:new(-1000,1000)
    -- no errors, no problem
end

--[[
The below tests pass with their given seeds, if they begin failing
for a given platform, or all around, new seeds should be found.

Note: these tests which assert RNG, are mere sanity checks
to ensure things haven't been severely broken by any changes
]]

function test.cxxrandom_seed()
    local nd = rng.normal_distribution:new(0,500000)
    local e1 = rng.MakeNewEngine(1)
    local e2 = rng.MakeNewEngine(1)
    local e3 = rng.MakeNewEngine(2)
    local g1 = rng.crng:new(e1, true, nd)
    local g2 = rng.crng:new(e2, true, nd)
    local g3 = rng.crng:new(e3, true, nd)
    local v1 = g1:next()
    expect.eq(v1, g2:next())
    expect.ne(v1, g3:next())
end

function test.cxxrandom_ranges()
    local e1 = rng.MakeNewEngine(1)
    local g1 = rng.crng:new(e1, true, rng.normal_distribution:new(0,1))
    local g2 = rng.crng:new(e1, true, rng.real_distribution:new(-5,5))
    local g3 = rng.crng:new(e1, true, rng.int_distribution:new(-5,5))
    local g4 = rng.crng:new(e1, true, rng.num_sequence:new(-5,5))
    for i = 1, 10 do
        local a = g1:next()
        local b = g2:next()
        local c = g3:next()
        local d = g4:next()
        expect.ge(a, -5)
        expect.ge(b, -5)
        expect.ge(c, -5)
        expect.ge(d, -5)
        expect.le(a, 5)
        expect.le(b, 5)
        expect.le(c, 5)
        expect.le(d, 5)
    end
    local gb = rng.crng:new(e1, true, rng.bool_distribution:new(0.00000000001))
    for i = 1, 10 do
        expect.false_(gb:next())
    end
end

function test.cxxrandom_exports()
    local id = rng.GenerateEngine(0)
    rng.NewSeed(id, 2022)
    expect.ge(rng.rollInt(id, 0, 1000), 0)
    expect.ge(rng.rollDouble(id, 0, 1), 0)
    expect.ge(rng.rollNormal(id, 5, 1), 0)
    expect.true_(rng.rollBool(id, 0.9999999999))
    local sid = rng.MakeNumSequence(0,8)
    rng.AddToSequence(sid, 9)
    rng.ShuffleSequence(sid, id)
    for i = 1, 10 do
        local v = rng.NextInSequence(sid)
        expect.ge(v, 0)
        expect.le(v, 9)
    end
    rng.DestroyNumSequence(sid)
    rng.DestroyEngine(id)
end
