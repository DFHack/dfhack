config.mode = 'fortress'
config.target = 'overlay'

local overlay = require('plugins.overlay')

-- overlay enable state persists via save_config; restore originals
local saved_enabled

config.wrapper = function(test_fn)
    saved_enabled = {}
    return dfhack.with_finalize(function()
        for name, enabled in pairs(saved_enabled) do
            dfhack.run_command_silent('overlay',
                                      enabled and 'enable' or 'disable',
                                      name)
        end
    end, test_fn)
end

local function save_widget_state(name)
    local cfg = overlay.get_state().config[name]
    saved_enabled[name] = cfg and cfg.enabled or false
end

-- overlay_command suppresses prints under run_command_silent (quiet
-- flag), so verify state changes directly instead of output
function test.list_succeeds()
    local output, status = dfhack.run_command_silent('overlay', 'list')
    expect.eq(0, status)
end

function test.enable_disable_widget()
    overlay.rescan()
    local db = overlay.get_state().db
    local name = next(db)
    expect.ne(nil, name)
    save_widget_state(name)
    dfhack.run_command_silent('overlay', 'disable', name)
    expect.false_(overlay.get_state().config[name].enabled)
    dfhack.run_command_silent('overlay', 'enable', name)
    expect.true_(overlay.get_state().config[name].enabled)
end

function test.unknown_command_shows_usage()
    local output, status = dfhack.run_command_silent('overlay', 'bogus')
    expect.true_(output:find('overlay', 1, true) ~= nil or status ~= 0)
end
