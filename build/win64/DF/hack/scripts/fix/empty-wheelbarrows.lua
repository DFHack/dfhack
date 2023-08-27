--checks all wheelbarrows on map for rocks stuck in them. If a wheelbarrow isn't in use for a job (hauling) then there should be no rocks in them
--rocks will occasionally get stuck in wheelbarrows, and accumulate if the wheelbarrow gets used.
--this script empties all wheelbarrows which have rocks stuck in them.

local argparse = require("argparse")

local args = {...}

local quiet = false
local dryrun = false

local cmds = argparse.processArgsGetopt(args, {
    {'q', 'quiet', handler=function() quiet = true end},
    {'d', 'dry-run', handler=function() dryrun = true end},
})

local i_count = 0
local e_count = 0

local function emptyContainedItems(e, outputCallback)
    local items = dfhack.items.getContainedItems(e)
    if #items > 0 then
        outputCallback('Emptying wheelbarrow: ' .. dfhack.items.getDescription(e, 0))
        e_count = e_count + 1
        for _,i in ipairs(items) do
            outputCallback('  ' .. dfhack.items.getDescription(i, 0))
            if (not dryrun) then dfhack.items.moveToGround(i, e.pos) end
            i_count = i_count + 1
        end
    end
end

local function emptyWheelbarrows(outputCallback)
    for _,e in ipairs(df.global.world.items.other.TOOL) do
        -- wheelbarrow must be on ground and not in a job
        if ((not e.flags.in_job) and e.flags.on_ground and e:isWheelbarrow()) then
           emptyContainedItems(e, outputCallback)
        end
    end
end

local output
if (quiet) then output = (function(...) end) else output = print end

emptyWheelbarrows(output)

if (i_count > 0 or (not quiet)) then
    print(("fix/empty-wheelbarrows - removed %d items from %d wheelbarrows."):format(i_count, e_count))
end
