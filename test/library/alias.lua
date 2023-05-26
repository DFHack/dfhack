-- Simple tests to ensure the alias functions work as expected
function test.aliases()
    expect.eq(false, dfhack.internal.isAlias("foo"))
    expect.eq(true, dfhack.internal.addAlias("foo help"))
    expect.eq(true, dfhack.internal.isAlias("foo"))
    expect.eq("help", dfhack.internal.getAliasCommand("foo"))
    expect.eq("this is not an alias", dfhack.internal.getAliasCommand("this is not an alias"))
    expect.eq("help", dfhack.internal.listAliases()["foo"][1])
    expect.eq(true, dfhack.internal.removeAlias("foo"))
    expect.eq(false, dfhack.internal.isAlias("foo"))
end
