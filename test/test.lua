config.target = 'core'

function test.internal_in_test()
    expect.true_(dfhack.internal.IN_TEST)
end
