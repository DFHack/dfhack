config.target = 'core'

function test.toSearchNormalized()
    expect.eq(dfhack.toSearchNormalized(''), '')
    expect.eq(dfhack.toSearchNormalized('abcd'), 'abcd')
    expect.eq(dfhack.toSearchNormalized('ABCD'), 'abcd')
    expect.eq(dfhack.toSearchNormalized(dfhack.utf2df('áçèîöü')), 'aceiou')
    expect.eq(dfhack.toSearchNormalized(dfhack.utf2df('ÄÇÉÖÜÿ')), 'aceouy')
    expect.eq(dfhack.toSearchNormalized(dfhack.utf2df('æÆ')), 'aeae')
end
