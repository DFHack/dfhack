local _ENV = mkmodule('plugins.spectate')

local argparse = require('argparse')
local dlg = require('gui.dialogs')
local json = require('json')
local overlay = require('plugins.overlay')
local utils = require('utils')

-- settings starting with 'tooltip-' are not passed to the C++ plugin
local lua_only_settings_prefix = 'tooltip-'

local function get_default_state()
    return {
        ['auto-disengage']=true,
        ['auto-unpause']=false,
        ['cinematic-action']=true,
        ['follow-seconds']=10,
        ['include-animals']=false,
        ['include-hostiles']=false,
        ['include-visitors']=false,
        ['include-wildlife']=false,
        ['prefer-conflict']=true,
        ['prefer-new-arrivals']=true,
        ['tooltip-follow-job']=true,
        ['tooltip-follow-name']=true,
        ['tooltip-follow-stress']=true,
        ['tooltip-hover-job']=true,
        ['tooltip-hover-name']=true,
        ['tooltip-hover-stress']=true,
    }
end

local function load_state()
    local state = get_default_state()
    local config = json.open('dfhack-config/spectate.json')
    for key in pairs(config.data) do
        if state[key] == nil then
            config.data[key] = nil
        end
    end
    utils.assign(state, config.data)
    config.data = state
    return config
end

local config = load_state()

function refresh_cpp_config()
    for name,value in pairs(config.data) do
        if not name:startswith(lua_only_settings_prefix) then
            if type(value) == 'boolean' then
                value = value and 1 or 0
            end
            spectate_setSetting(name, value)
        end
    end
end

function show_squads_warning()
    local message = {
        'Cannot start spectate mode while auto-disengage is enabled and',
        'the squads panel is open. The auto-disengage feature automatically',
        'stops spectate mode when you open the squads panel.',
        '',
        'Please either close the squads panel or disable auto-disengage by',
        'running the following command:',
        '',
        'spectate set auto-disengage false',
    }
    dlg.showMessage("Spectate", table.concat(message, '\n'))
end

-----------------------------
-- commandline interface

local function print_status()
    print('spectate is:', isEnabled() and 'enabled' or 'disabled')
    print()
    print('settings:')
    for key, value in pairs(config.data) do
        print('  ' .. key .. ': ' .. tostring(value))
    end
end

local function do_toggle()
    if isEnabled() then
        dfhack.run_command('disable', 'spectate')
    else
        dfhack.run_command('enable', 'spectate')
    end
end

local function set_setting(key, value)
    if config.data[key] == nil then
        qerror('unknown setting: ' .. key)
    end
    if key == 'follow-seconds' then
        value = argparse.positiveInt(value, 'follow-seconds')
    else
        value = argparse.boolean(value, key)
    end
    config.data[key] = value
    config:write()
    if not key:startswith(lua_only_settings_prefix) then
        if type(value) == 'boolean' then
            value = value and 1 or 0
        end
        spectate_setSetting(key, value)
    end
end

local function set_overlay(name, value)
    if not name:startswith('spectate.') then
        name = 'spectate.' .. name
    end
    if name ~= 'spectate.follow' and name ~= 'spectate.hover' then
        qerror('unknown overlay: ' .. name)
    end
    value = argparse.boolean(value, name)
    dfhack.run_command('overlay', value and 'enable' or 'disable', name)
end

function parse_commandline(args)
    local command = table.remove(args, 1)
    if not command or command == 'status' then
        print_status()
    elseif command == 'toggle' then
        do_toggle()
    elseif command == 'set' then
        set_setting(args[1], args[2])
    elseif command == 'overlay' then
        set_overlay(args[1], args[2])
    else
        return false
    end

    return true
end

-----------------------------
-- overlays

FollowOverlay = defclass(FollowOverlay, overlay.OverlayWidget)
FollowOverlay.ATTRS{
    desc='Adds info tooltips that follow units on the map.',
    default_pos={x=1,y=1},
    fullscreen=true,
    viewscreens='dwarfmode/Default',
}

HoverOverlay = defclass(HoverOverlay, overlay.OverlayWidget)
HoverOverlay.ATTRS{
    desc='Shows info popup when hovering the mouse over units on the map.',
    default_pos={x=1,y=1},
    fullscreen=true,
    viewscreens='dwarfmode/Default',
}

OVERLAY_WIDGETS = {
    follow=FollowOverlay,
    hover=HoverOverlay,
}

return _ENV
