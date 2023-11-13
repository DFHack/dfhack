local _ENV = mkmodule('plugins.buildingplan')

--[[

 Public native functions:

 * bool isPlannableBuilding(df::building_type type, int16_t subtype, int32_t custom)
 * bool isPlannedBuilding(df::building *bld)
 * void addPlannedBuilding(df::building *bld)
 * void doCycle()
 * void scheduleCycle()

--]]

local argparse = require('argparse')
local inspector = require('plugins.buildingplan.inspectoroverlay')
local mechanisms = require('plugins.buildingplan.mechanisms')
local pens = require('plugins.buildingplan.pens')
local planner = require('plugins.buildingplan.planneroverlay')
require('dfhack.buildings')

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    return argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
        })
end

function parse_commandline(...)
    local args, opts = {...}, {}
    local positionals = process_args(opts, args)

    if opts.help then
        return false
    end

    local command = table.remove(positionals, 1)
    if not command or command == 'status' then
        printStatus()
    elseif command == 'set' and positionals then
        setSetting(positionals[1], positionals[2] == 'true')
    elseif command == 'reset' then
        resetFilters()
    else
        return false
    end

    return true
end

function is_suspendmanager_enabled()
    local ok, sm = pcall(reqscript, 'suspendmanager')
    return ok and sm.isEnabled()
end

function get_num_filters(btype, subtype, custom)
    local filters = dfhack.buildings.getFiltersByType({}, btype, subtype, custom)
    return filters and #filters or 0
end

function get_job_item(btype, subtype, custom, index)
    local filters = dfhack.buildings.getFiltersByType({}, btype, subtype, custom)
    if not filters or not filters[index] then return nil end
    local obj = df.job_item:new()
    obj:assign(filters[index])
    return obj
end

local function to_title_case(str)
    str = str:gsub('(%a)([%w_]*)',
        function (first, rest) return first:upper()..rest:lower() end)
    str = str:gsub('_', ' ')
    return str
end

function get_desc(filter)
    local desc = 'Unknown'
    if filter.has_tool_use and filter.has_tool_use > -1 then
        desc = to_title_case(df.tool_uses[filter.has_tool_use])
    elseif filter.flags2 and filter.flags2.screw then
        desc = 'Screw'
    elseif filter.item_type and filter.item_type > -1 then
        desc = to_title_case(df.item_type[filter.item_type])
    elseif filter.vector_id and filter.vector_id > -1 then
        desc = to_title_case(df.job_item_vector_id[filter.vector_id])
    elseif filter.flags2 and filter.flags2.building_material then
        if filter.flags2.magma_safe then
            desc = 'Magma-safe material'
        elseif filter.flags2.fire_safe then
            desc = 'Fire-safe material'
        else
            desc = 'Building material'
        end
    end

    if desc:endswith('s') then
        desc = desc:sub(1,-2)
    end
    if desc == 'Trappart' then
        desc = 'Mechanism'
    elseif desc == 'Wood' then
        desc = 'Log'
    elseif desc == 'Any weapon' then
        desc = 'Weapon'
    elseif desc == 'Any spike' then
        desc = 'Spike'
    elseif desc == 'Ballistapart' then
        desc = 'Ballista part'
    elseif desc == 'Catapultpart' then
        desc = 'Catapult part'
    elseif desc == 'Smallgem' then
        desc = 'Small, cut gem'
    end

    return desc
end

function reload_pens()
    pens.reload_pens()
end

function signal_reset()
    planner.reset_counts_flag = true
    inspector.reset_inspector_flag = true
end

-- for use during development to reload all buildingplan modules
function reload_modules()
    reload('plugins.buildingplan.pens')
    reload('plugins.buildingplan.filterselection')
    reload('plugins.buildingplan.itemselection')
    reload('plugins.buildingplan.planneroverlay')
    reload('plugins.buildingplan.inspectoroverlay')
    reload('plugins.buildingplan.mechanisms')
    reload('plugins.buildingplan')
end

OVERLAY_WIDGETS = {
    planner=planner.PlannerOverlay,
    inspector=inspector.InspectorOverlay,
    mechanisms=mechanisms.MechanismOverlay,
}

return _ENV
