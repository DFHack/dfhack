local script = require("gui.script")
local persistTable = require("persist-table")

-- (Hopefully) get original settings
originalPopCap = originalPopCap or df.global.d_init.population_cap
originalStrictPopCap = originalStrictPopCap or df.global.d_init.strict_population_cap
originalVisitorCap = originalVisitorCap or df.global.d_init.visitor_cap

local function popControl(forceEnterSettings)
    if df.global.gamemode ~= df.game_mode.DWARF then
        if forceEnterSettings then
            -- did reenter-settings, show an error
            qerror("Not in fort mode")
            return
        else
            -- silent automatic behaviour
            return
        end
    end

    if not persistTable.GlobalTable.fortPopInfo then
        persistTable.GlobalTable.fortPopInfo = {}
    end

    local siteId = df.global.plotinfo.site_id

    script.start(function()
        local siteInfo = persistTable.GlobalTable.fortPopInfo[siteId]
        if not siteInfo or forceEnterSettings then
            -- get new settings
            persistTable.GlobalTable.fortPopInfo[siteId] = nil -- i don't know if persist-table works well with reassignent
            persistTable.GlobalTable.fortPopInfo[siteId] = {}
            siteInfo = persistTable.GlobalTable.fortPopInfo[siteId]
            if script.showYesNoPrompt("Hermit", "Hermit mode?") then
                siteInfo.hermit = "true"
            else
                siteInfo.hermit = "false"
                local _ -- ignore
                -- migrant cap
                local migrantCapInput
                while not tonumber(migrantCapInput) do
                    _, migrantCapInput = script.showInputPrompt("Migrant cap", "Maximum migrants per wave?")
                end
                siteInfo.migrantCap = migrantCapInput
                -- pop cap
                local popCapInput
                while not tonumber(popCapInput) or popCapInput == "" do
                    _, popCapInput = script.showInputPrompt("Population cap", "Maximum population? Settings population cap: " .. originalPopCap .. "\n(assuming wasn't changed before first call of this script)")
                end
                siteInfo.popCap = tostring(tonumber(popCapInput) or originalPopCap)
                -- strict pop cap
                local strictPopCapInput
                while not tonumber(strictPopCapInput) or strictPopCapInput == "" do
                    _, strictPopCapInput = script.showInputPrompt("Strict population cap", "Strict maximum population? Settings strict population cap " .. originalStrictPopCap .. "\n(assuming wasn't changed before first call of this script)")
                end
                siteInfo.strictPopCap = tostring(tonumber(strictPopCapInput) or originalStrictPopCap)
                -- visitor cap
                local visitorCapInput
                while not tonumber(visitorCapInput) or visitorCapInput == "" do
                    _, visitorCapInput = script.showInputPrompt("Visitors", "Vistitor cap? Settings visitor cap " .. originalVisitorCap .. "\n(assuming wasn't changed before first call of this script)")
                end
                siteInfo.visitorCap = tostring(tonumber(visitorCap) or originalVisitorCap)
            end
        end
        -- use settings
        if siteInfo.hermit == "true" then
            dfhack.run_command("hermit enable")
            -- NOTE: could, maybe should cancel max-wave repeat here
        else
            dfhack.run_command("hermit disable")
            dfhack.run_command("repeat -name max-wave -timeUnits months -time 1 -command [ max-wave " .. siteInfo.migrantCap .. " " .. siteInfo.popCap .. " ]")
            df.global.d_init.strict_population_cap = tonumber(siteInfo.strictPopCap)
            df.global.d_init.visitor_cap = tonumber(siteInfo.visitorCap)
        end
    end)
end

local function viewSettings()
    local siteId = df.global.plotinfo.site_id
    if not persistTable.GlobalTable.fortPopInfo or not persistTable.GlobalTable.fortPopInfo[siteId] then
        print("Could not find site information")
        return
    end
    local siteInfo = persistTable.GlobalTable.fortPopInfo[siteId]
    if siteInfo.hermit == "true" then
       print("Hermit: true")
       return
    end
    print("Hermit: false")
    print("Migrant cap: " .. siteInfo.migrantCap)
    print("Population cap: " .. siteInfo.popCap)
    print("Strict population cap: " .. siteInfo.strictPopCap)
    print("Visitor cap: " .. siteInfo.visitorCap)
end

local function help()
    print("syntax: pop-control [on-load|reenter-settings|view-settings]")
end

local action_switch = {
    ["on-load"] = function() popControl(false) end,
    ["reenter-settings"] = function() popControl(true) end,
    ["view-settings"] = function() viewSettings() end
}
setmetatable(action_switch, {__index = function() return help end})

local args = {...}
action_switch[args[1] or "help"]()
