local _ENV = mkmodule('plugins.timestream')

function migrate_old_config()
    local GLOBAL_KEY = 'timestream'
    local old_config = dfhack.persistent.getSiteData(GLOBAL_KEY)
    if not old_config then return end
    if old_config.enabled then dfhack.run_command('enable', GLOBAL_KEY) end
    if old_config.settings and type(old_config.settings) == 'table' and tonumber(old_config.settings.fps) then
        timestream_setFps(tonumber(old_config.settings.fps))
    end
end

local function do_set(setting_name, arg)
    local numarg = tonumber(arg)
    if setting_name ~= 'fps' or not numarg then
        qerror('must specify setting and value')
    end
    timestream_setFps(arg)
    print(('set %s to %s'):format(setting_name, timestream_getFps()))
end

local function do_reset()
    timestream_resetSettings()
end

local function print_status()
    print(GLOBAL_KEY .. ' is ' .. (state.enabled and 'enabled' or 'not enabled'))
    print()
    print('settings:')
    for _,v in ipairs(SETTINGS) do
        print(('  %15s: %s'):format(v.name, state.settings[v.internal_name or v.name]))
    end
    if DEBUG < 2 then return end
    print()
    print(('cur_year_tick:    %d'):format(df.global.cur_year_tick))
    print(('timeskip_deficit: %.2f'):format(timeskip_deficit))
    if DEBUG < 3 then return end
    print()
    print('tick coverage:')
    for coverage_slot=0,49 do
        print(('  slot %2d: %scovered'):format(coverage_slot, tick_coverage[coverage_slot] and '' or 'NOT '))
    end
    print()
    local bdays, bdays_list = {}, {}
    for _, next_bday in pairs(birthday_triggers) do
        if not bdays[next_bday] then
            bdays[next_bday] = true
            table.insert(bdays_list, next_bday)
        end
    end
    print(('%d birthdays:'):format(#bdays_list))
    table.sort(bdays_list)
    for _,bday in ipairs(bdays_list) do
        print(('  year tick: %d'):format(bday))
    end
end

function parse_commandline(args)
    local command = table.remove(args, 1)

    if command == 'help' then
        return false
    elseif command == 'set' then
        do_set(args[1], args[2])
    elseif command == 'reset' then
        do_reset()
    elseif not command or command == 'status' then
        print_status()
    else
        return false
    end

    return true
end

return _ENV
