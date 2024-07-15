config.target = 'core'

config.wrapper = function(test_fn)
    -- backup and restore clipboard contents
    local str = dfhack.internal.getClipboardTextCp437Multiline()
    return dfhack.with_finalize(
        curry(dfhack.internal.setClipboardTextCp437Multiline, table.concat(str, NEWLINE)),
        test_fn)
end

function test.clipboard()
    dfhack.internal.setClipboardTextCp437('a test string')
    expect.eq('a test string', dfhack.internal.getClipboardTextCp437(),
        'failed to retrieve simple clipboard text')

    dfhack.internal.setClipboardTextCp437('special char '..string.char(15))
    expect.eq('special char '..string.char(15), dfhack.internal.getClipboardTextCp437(),
        'failed to retrieve special char clipboard text')

    dfhack.internal.setClipboardTextCp437('embedded newline as symbol'..NEWLINE)
    expect.eq('embedded newline as symbol'..string.char(10), dfhack.internal.getClipboardTextCp437(),
        'failed to retrieve clipboard text with embedded char 10')
end

function test.clipboard_multiline()
    dfhack.internal.setClipboardTextCp437Multiline('a test string')
    expect.table_eq({'a test string'}, dfhack.internal.getClipboardTextCp437Multiline(),
        'failed to retrieve single line')

    dfhack.internal.setClipboardTextCp437Multiline('special char '..string.char(15))
    expect.table_eq({'special char '..string.char(15)}, dfhack.internal.getClipboardTextCp437Multiline(),
        'failed to retrieve single line with special char')

    dfhack.internal.setClipboardTextCp437Multiline('multi line'..NEWLINE..'string'..NEWLINE)
    expect.table_eq({'multi line', 'string', ''}, dfhack.internal.getClipboardTextCp437Multiline(),
        'failed to retrieve multi line text')

    dfhack.internal.setClipboardTextCp437Multiline('multi line'..NEWLINE..'string')
    expect.table_eq({'multi line', 'string'}, dfhack.internal.getClipboardTextCp437Multiline(),
        'failed to retrieve multi line text with no trailing newline')
end
