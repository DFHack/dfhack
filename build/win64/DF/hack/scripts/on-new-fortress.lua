-- runs dfhack commands only in a newborn fortress

if not (...) then
    print(dfhack.script_help())
    return
end

if not (dfhack.world.isFortressMode() and df.global.plotinfo.fortress_age == 0) then return end

for cmd in table.concat({...}, ' '):gmatch("%s*([^;]+);?%s*") do
    dfhack.run_command(cmd)
end
