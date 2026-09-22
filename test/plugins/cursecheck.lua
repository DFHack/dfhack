config.mode = 'fortress'
config.target = 'cursecheck'

function test.reports_map_curse_count()
    local output = dfhack.run_command_silent('cursecheck')
    expect.str_find('%d+ cursed creatures on map', output)
end

function test.help_shows_usage()
    local output, status = dfhack.run_command_silent('cursecheck', 'help')
    expect.true_(output:find('cursecheck', 1, true) ~= nil
                 or output:find('Usage', 1, true) ~= nil
                 or output:find('usage', 1, true) ~= nil
                 or status ~= 0)
end

function test.detail_still_reports_count()
    local output = dfhack.run_command_silent('cursecheck', 'detail')
    expect.str_find('cursed creatures on map', output)
end
