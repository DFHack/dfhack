-- handles automatic fishing jobs to limit the number of fish the fortress keeps on hand
-- autofish [enable | disable] <max> [min] [--include-raw | -r]

--@ enable=true
--@ module=true

local json = require("json")
local persist = require("persist-table")
local argparse = require("argparse")
local repeatutil = require("repeat-util")

local GLOBAL_KEY = "autofish"

-- set default enabled state
enabled = enabled or false
s_maxFish = s_maxFish or 100
s_minFish = s_minFish or 75
s_useRaw = s_useRaw or true
isFishing = isFishing or true

--- Check if the script is enabled.
-- @return true if the script is enabled, otherwise false.
function isEnabled()
    return enabled
end

--- Save the current state of the script
local function persist_state()
    persist.GlobalTable[GLOBAL_KEY] = json.encode({enabled=enabled,
        s_maxFish=s_maxFish, s_minFish=s_minFish, s_useRaw=s_useRaw,
        isFishing=isFishing
    })
end

--- Load the saved state of the script
local function load_state()
    -- load persistent data
    local persisted_data = json.decode(persist.GlobalTable[GLOBAL_KEY] or "") or {}
    enabled = persisted_data.enabled or false
    s_maxFish = persisted_data.s_maxFish or 100
    s_minFish = persisted_data.s_minFish or 75
    s_useRaw = persisted_data.s_useRaw or (persisted_data.s_useRaw == nil)
    isFishing = persisted_data.isFishing or (persisted_data.isFishing == nil)
end

--- Set the maximum fish threshold.
-- @param val The value to set s_maxFish to. (number)
function set_minFish(val)
    s_minFish = val
    -- min fish cannot exceed max fish
    if s_minFish >= s_maxFish then s_minFish = s_maxFish end
    persist_state()
end

--- Set the minimum fish threshold.
-- @param val The value to set s_minFish to. (number)
function set_maxFish(val)
    s_maxFish = val
    -- max fish cannot be lower than min fish
    if s_maxFish <= s_minFish then s_minFish = s_maxFish end
    persist_state()
end

--- Set the raw fish toggle
-- @param val The value to set s_useRaw to. (boolean)
function set_useRaw(val)
    s_useRaw = val
    persist_state()
end

--- Toggle all work details (and fishing labours)
-- @param state What state to toggle the labours to.
function toggle_fishing_labour(state)
    -- pass true to state to turn on, otherwise disable
    -- find all work details that have fishing enabled:
    local work_details = df.global.plotinfo.hauling.work_details
    for _,v in pairs(work_details) do
        if v.allowed_labors.FISH then
            -- set limited to true just in case a custom work detail is being
            -- changed, to prevent *all* dwarves from fishing.
            v.work_detail_flags.limited = true
            v.work_detail_flags.enabled = state

            -- workaround to actually enable labours
            for _,v2 in ipairs(v.assigned_units) do
                -- find unit by ID and toggle fishing
                local unit = df.unit.find(v2)
                if unit then
                    unit.status.labors.FISH = state
                end
            end
        end
    end
    isFishing = state -- save current state

    -- let the user know we've got enough, or run out of fish
    if isFishing then
        print("autofish: Re-enabling fishing, fallen below minimum.")
    else
        print("autofish: Disabling fishing, reached desired quota.")
    end
end

--- Checks several item flags to see if a given item should be considered good
-- @param item: a valid dwarf fortress item.
function isValidItem(item)
    local flags = item.flags
    if flags.rotten or flags.trader or flags.hostile or flags.forbid
        or flags.dump or flags.on_fire or flags.garbage_collect or flags.owned
        or flags.removed or flags.encased or flags.spider_web then
        return false
    end
    return true
end


--- Counts the number of available fish in a fortress
-- @return prepared (number): count of prepared fish available.
-- @return raw (number): count of raw fish available.
function count_fish()
    local world = df.global.world

    -- count the number of valid fish we have. (not rotten, forbidden, on fire, dumping...)
    local prepared, raw = 0, 0
    for k,v in pairs(world.items.other[df.items_other_id.IN_PLAY]) do
        if v:getType() == df.item_type.FISH and isValidItem(v) then
            prepared = prepared + v:getStackSize()
        end
        if (v:getType() == df.item_type.FISH_RAW and isValidItem(v)) and s_useRaw then
            raw = raw + v:getStackSize()
        end
    end
    return prepared, raw
end

--- The main event loop of the script.
function event_loop()
    if not enabled then return end

    local prepared, raw = count_fish()

    -- handle pausing/resuming labour
    local numFish = s_useRaw and (prepared+raw) or prepared
    if isFishing and (numFish >= s_maxFish) then
        toggle_fishing_labour(false)
    elseif not isFishing and (numFish < s_minFish) then
        toggle_fishing_labour(true)
    end

    persist_state()

    -- check weekly
    repeatutil.scheduleUnlessAlreadyScheduled(GLOBAL_KEY, 7, "days", event_loop)
end


--- Print a status output, showing the current state of the script and options.
local function print_status()
    print(string.format("autofish is currently %s.\n", (enabled and "enabled" or "disabled")))
    if enabled then
        local rfs
        rfs = s_useRaw and "raw & prepared" or "prepared"

        print(string.format("Stopping at %s %s fish.", s_maxFish, rfs))
        print(string.format("Restarting at %s %s fish.", s_minFish, rfs))
        if isFishing then
            print("\nCurrently allowing fishing.")
        else
            print("\nCurrently not allowing fishing.")
        end
    end
end

--- Handles automatic loading
dfhack.onStateChange[GLOBAL_KEY] = function(sc)
    -- unload with game
    if sc == SC_MAP_UNLOADED then
        enabled = false
        return
    end

    if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
        return
    end

    load_state()

    -- run the main code
    event_loop()
end


-- sanity checks?
if dfhack_flags.module then
    return
end

if df.global.gamemode ~= df.game_mode.DWARF or not dfhack.isMapLoaded() then
    dfhack.printerr("autofish needs a loaded fortress to work")
    return
end

-- argument handling
local args = {...}
if dfhack_flags and dfhack_flags.enable then
    args = {dfhack_flags.enable_state and "enable" or "disable"}
end

-- find flags in args:
local positionals = argparse.processArgsGetopt(args,
    {{"r", "toggle-raw",
    handler=function() s_useRaw = not s_useRaw end}
})

load_state()
if positionals[1] == "enable" then
    enabled = true

elseif positionals[1] == "disable" then
    enabled = false
    persist_state()
    repeatutil.cancel(GLOBAL_KEY)
    return

elseif positionals[1] == "status" then
    print_status()
    return

elseif positionals ~= nil then
    -- positionals is a number?
    if positionals[1] and tonumber(positionals[1]) then
        -- assume we're changing setting:
        local newval = tonumber(positionals[1])
        set_maxFish(newval)
        if not positionals[2] then
            set_minFish(math.floor(newval * 0.75))
        end
    else
        -- invalid or no argument
        return
    end

    if positionals[2] and tonumber(positionals[2]) then
        set_minFish(tonumber(positionals[2]))
    end

    -- a setting probably changed, save & show the updated settings.
    persist_state()
    print_status()
    return
end

event_loop()
persist_state()
