-- helper functions for the quickfort API
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

require('dfhack.buildings') -- loads additional functions into dfhack.buildings
local utils = require('utils')
local quickfort_command = reqscript('internal/quickfort/command')
local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_building = reqscript('internal/quickfort/building')
local quickfort_map = reqscript('internal/quickfort/map')
local quickfort_parse = reqscript('internal/quickfort/parse')

function normalize_data(data, pos)
    pos = pos or {}
    pos.x, pos.y, pos.z = pos.x or 0, pos.y or 0, pos.z or 0

    if type(data) == 'string' then
        data = {[0]={[0]={[0]=data}}}
    end

    local shifted, min = {}, {x=30000, y=30000, z=30000}
    for z,grid in pairs(data) do
        local shiftedz, shiftedgrid = z+pos.z, {}
        if shiftedz < min.z then min.z = shiftedz end
        for y,row in pairs(grid) do
            local shiftedy, shiftedrow = y+pos.y, {}
            if shiftedy < min.y then min.y = shiftedy end
            for x,text in pairs(row) do
                local shiftedx = x + pos.x
                if shiftedx < min.x then min.x = shiftedx end
                shiftedrow[shiftedx] = {cell=('%d,%d,%d'):format(x,y,z),
                                        text=text}
            end
            shiftedgrid[shiftedy] = shiftedrow
        end
        shifted[shiftedz] = shiftedgrid
    end
    return shifted, min
end

-- wraps quickfort_command.init_ctx() and sets API-specific settings
function init_api_ctx(params, cursor)
    local p = copyall(params)

    -- fix up API params so they can be used to initialize a quickfort ctx
    if not p.command then p.command = 'run' end
    p.blueprint_name = 'API'
    p.cursor = cursor
    p.quiet = not p.verbose
    p.preview = false
    if p.preserve_engravings then
        p.preserve_engravings = quickfort_parse.parse_preserve_engravings(
                params.preserve_engravings, true)
    end

    return quickfort_command.init_ctx(p)
end
function clean_stats(stats)
    for _,stat in pairs(stats) do
        -- remove internal markers
        stat.always = nil
    end
    return stats
end
