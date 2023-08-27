-- Unforbid all items

local argparse = require('argparse')

local function unforbid_all(include_unreachable, quiet)
    if not quiet then print('Unforbidding all items...') end

    local citizens = dfhack.units.getCitizens()
    local count = 0
    for _, item in pairs(df.global.world.items.all) do
        if item.flags.forbid then
            if not include_unreachable then
                local reachable = false

                for _, unit in pairs(citizens) do
                    if dfhack.maps.canWalkBetween(xyz2pos(dfhack.items.getPosition(item)), unit.pos) then
                        reachable = true
                    end
                end

                if not reachable then
                    if not quiet then print(('  unreachable: %s (skipping)'):format(item)) end
                    goto skipitem
                end
            end

            if not quiet then print(('  unforbid: %s'):format(item)) end
            item.flags.forbid = false
            count = count + 1

            ::skipitem::
        end
    end

    if not quiet then print(('%d items unforbidden'):format(count)) end
end

-- let the common --help parameter work, even though it's undocumented
local options, args = {
    help = false,
    quiet = false,
    include_unreachable = false,
}, { ... }

local positionals = argparse.processArgsGetopt(args, {
    { 'h', 'help', handler = function() options.help = true end },
    { 'q', 'quiet', handler = function() options.quiet = true end },
    { 'u', 'include-unreachable', handler = function() options.include_unreachable = true end },
})

if positionals[1] == nil or positionals[1] == 'help' or options.help then
    print(dfhack.script_help())
    return
end

if positionals[1] == 'all' then
    unforbid_all(options.include_unreachable, options.quiet)
end
