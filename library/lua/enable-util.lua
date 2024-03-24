
local _ENV = mkmodule("enable-util")

local repeatUtil = require('repeat-util')

function assignPersistentGlobal(global_key, global_store, key, value)
    if global_store[key] == value then
        return
    end
    global_store[key] = value
    dfhack.persistent.saveSiteData(global_key, global_store)
end

function enableEvery(flags, global_store, global_key, time, unit, action)

    local function assign_persistent(key,value)
        assignPersistentGlobal(global_key, global_store, key, value)
    end

    dfhack.onStateChange[global_key] = function(sc)
        if sc == SC_MAP_UNLOADED then
            global_store.enabled = false
            -- repeat-util will cancel on its own
            return
        end

        if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
            return
        end

        -- map was loaded, restore global data and reschedule
        global_store = dfhack.persistent.getSiteData(global_key, {enabled=false})
        if global_store.enabled then
            print ('reenabling ', global_key)
            repeatUtil.scheduleEvery(global_key, time, unit, action)
        end
    end

    -- react to enable/disable commands
    if flags.enable then
        if flags.enable_state then
            assign_persistent('enabled', true)
            repeatUtil.scheduleEvery(global_key, time, unit, action)
        else
            assign_persistent('enabled', false)
            repeatUtil.cancel(global_key)
        end
    end
end




return _ENV
