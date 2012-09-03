-- Prints memory ranges of the process.

for _,v in ipairs(dfhack.internal.getMemRanges()) do
    local access = { '-', '-', '-', 'p' }
    if v.read then access[1] = 'r' end
    if v.write then access[2] = 'w' end
    if v.execute then access[3] = 'x' end
    if not v.valid then
        access[4] = '?'
    elseif v.shared then
        access[4] = 's'
    end
    print(string.format('%08x-%08x %s %s', v.start_addr, v.end_addr, table.concat(access), v.name))
end
