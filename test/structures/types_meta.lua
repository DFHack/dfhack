config.target = 'core'

function test.struct()
    expect.eq(df.coord._kind, 'struct-type')
    expect.eq(tostring(df.coord), '<type: coord>')
    expect.eq(getmetatable(df.coord), 'coord')
    expect.pairs_contains(df.coord, 'new')
    expect.pairs_contains(df.coord, 'is_instance')
    expect.pairs_contains(df.coord, 'sizeof')
end

function test.class()
    expect.eq(df.viewscreen._kind, 'class-type')
    expect.eq(tostring(df.viewscreen), '<type: viewscreen>')
    expect.eq(getmetatable(df.viewscreen), 'viewscreen')
    expect.pairs_contains(df.viewscreen, 'new')
    expect.pairs_contains(df.viewscreen, 'is_instance')
    expect.pairs_contains(df.viewscreen, 'sizeof')
end

function test.enum()
    expect.eq(df.interface_key._kind, 'enum-type')
    expect.eq(tostring(df.interface_key), '<type: interface_key>')
    expect.eq(getmetatable(df.interface_key), 'interface_key')
    expect.pairs_contains(df.interface_key, 'new')
    expect.pairs_contains(df.interface_key, 'is_instance')
    expect.pairs_contains(df.interface_key, 'sizeof')
end

function test.bitfield()
    expect.eq(df.item_flags._kind, 'bitfield-type')
    expect.eq(tostring(df.item_flags), '<type: item_flags>')
    expect.eq(getmetatable(df.item_flags), 'item_flags')
    expect.pairs_contains(df.item_flags, 'new')
    expect.pairs_contains(df.item_flags, 'is_instance')
    expect.pairs_contains(df.item_flags, 'sizeof')
end

function test.global()
    expect.eq(df.global._kind, 'global')
    expect.eq(tostring(df.global), '<type: global>')
    expect.eq(getmetatable(df.global), 'global')
    expect.not_pairs_contains(df.global, 'new')
    expect.not_pairs_contains(df.global, 'is_instance')
    expect.not_pairs_contains(df.global, 'sizeof')
end

function test.unit()
    expect.pairs_contains(df.unit, 'new')
    expect.pairs_contains(df.unit, 'is_instance')
    expect.pairs_contains(df.unit, 'sizeof')
    expect.pairs_contains(df.unit, 'find')
    expect.pairs_contains(df.unit, 'get_vector')
    expect.pairs_contains(df.unit, '_kind')

    if df.unit.T_job then
        expect.pairs_contains(df.unit, 'T_job')
        expect.pairs_contains(df.unit.T_job, 'new')
        expect.pairs_contains(df.unit.T_job, 'is_instance')
        expect.pairs_contains(df.unit.T_job, 'sizeof')
        expect.not_pairs_contains(df.unit.T_job, 'find')
        expect.not_pairs_contains(df.unit.T_job, 'get_vector')
        expect.pairs_contains(df.unit.T_job, '_kind')
    else
        expect.fail('unit.T_job not defined; unit has changed')
    end
end
