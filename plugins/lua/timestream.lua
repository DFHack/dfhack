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
    print('timestream is ' .. (isEnabled() and 'enabled' or 'not enabled'))
    print()
    print('target FPS is set to: ' .. tostring(timestream_getFps()))
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
