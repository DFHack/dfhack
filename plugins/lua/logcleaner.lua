local _ENV = mkmodule('plugins.logcleaner')

local function print_status()
    print(('logcleaner is %s'):format(isEnabled() and "enabled" or "disabled"))
    print('  Combat: ' .. (logcleaner_getCombat() and 'enabled' or 'disabled'))
    print('  Sparring: ' .. (logcleaner_getSparring() and 'enabled' or 'disabled'))
    print('  Hunting: ' .. (logcleaner_getHunting() and 'enabled' or 'disabled'))
end

function parse_commandline(...)
    local args = {...}
    local command = args[1]

    -- Show status if no command or "status"
    if not command or command == 'status' then
        print_status()
        return true
    end

    -- Handle enable/disable commands
    if command == 'enable' then
        if isEnabled() then
            print('logcleaner is already enabled')
        else
            dfhack.run_command('enable', 'logcleaner')
            print('logcleaner enabled')
        end
        return true
    end

    if command == 'disable' then
        if not isEnabled() then
            print('logcleaner is already disabled')
        else
            dfhack.run_command('disable', 'logcleaner')
            print('logcleaner disabled')
        end
        return true
    end

    -- Start with all disabled, enable only what's specified
    local new_combat, new_sparring, new_hunting = false, false, false
    local has_filter = false

    if command == 'all' then
        new_combat, new_sparring, new_hunting = true, true, true
        has_filter = true
    elseif command == 'none' then
        new_combat, new_sparring, new_hunting = false, false, false
    else
        -- Split by comma for multiple options in one parameter
        for token in command:gmatch('([^,]+)') do
            if token == 'combat' then
                new_combat = true
                has_filter = true
            elseif token == 'sparring' then
                new_sparring = true
                has_filter = true
            elseif token == 'hunting' then
                new_hunting = true
                has_filter = true
            else
                dfhack.printerr('Unknown option: ' .. token)
                return false
            end
        end
    end

    -- Auto-enable plugin when filters are being configured
    if has_filter and not isEnabled() then
        dfhack.run_command('enable', 'logcleaner')
        print('logcleaner enabled')
    end

    logcleaner_setCombat(new_combat)
    logcleaner_setSparring(new_sparring)
    logcleaner_setHunting(new_hunting)

    print('Log cleaning config updated:')
    print_status()

    return true
end

return _ENV
