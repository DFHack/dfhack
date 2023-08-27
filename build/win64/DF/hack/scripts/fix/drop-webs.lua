--Cause floating webs to fall
--By Bumber
--@module = true
--[====[

fix/drop-webs
=============
Turns floating webs into projectiles, causing them to fall down to a valid
surface. This addresses :bug:`595`.

Use ``fix/drop-webs -all`` to turn all webs into projectiles, causing webs to
fall out of branches, etc.

Use `clear-webs` to remove webs entirely.

]====]
local utils = require "utils"

function drop_webs(all_webs)
    if not dfhack.isMapLoaded() then
        qerror("Error: Map not loaded!")
    end

    local count = 0
    for i, item in ipairs(df.global.world.items.other.ANY_WEBS) do
        if item.flags.on_ground and not item.flags.in_job then
            local valid_tile = all_webs or (dfhack.maps.getTileType(item.pos) == df.tiletype.OpenSpace)

            if valid_tile then
                local proj = dfhack.items.makeProjectile(item)
                proj.flags.no_impact_destroy = true
                proj.flags.bouncing = true
                proj.flags.piercing = true
                proj.flags.parabolic = true
                proj.flags.unk9 = true
                proj.flags.no_collide = true
                count = count+1
            end
        end
    end
    print(tostring(count).." webs projectilized!")
end

function main(...)
    local validArgs = utils.invert({"all"})
    local args = utils.processArgs({...}, validArgs)

    drop_webs(args.all)
end

if not dfhack_flags.module then
    main(...)
end
