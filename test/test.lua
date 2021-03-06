function test.internal_in_test()
    expect.true_(dfhack.internal.IN_TEST)
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
