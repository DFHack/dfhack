-- Dismisses stuck merchants that haven't entered the map yet
-- Based on "dismissmerchants" by PatrikLundell:
-- http://www.bay12forums.com/smf/index.php?topic=159297.msg7257447#msg7257447

local help = [====[

fix/stuck-merchants
===================

Dismisses merchants that haven't entered the map yet. This can fix :bug:`9593`.
This script should probably not be run if any merchants are on the map, so using
it with `repeat` is not recommended.

Run ``fix/stuck-merchants -n`` or ``fix/stuck-merchants --dry-run`` to list all
merchants that would be dismissed but make no changes.

]====]


function getEntityName(u)
    local civ = df.historical_entity.find(u.civ_id)
    if not civ then return 'unknown civ' end
    return dfhack.TranslateName(civ.name)
end

function getEntityRace(u)
    local civ = df.historical_entity.find(u.civ_id)
    if civ then
        local craw = df.creature_raw.find(civ.race)
        if craw then
            return craw.name[0]
        end
    end
    return 'unknown race'
end

function dismissMerchants(args)
    local dry_run = false
    for _, arg in pairs(args) do
        if args[1]:match('-h') or args[1]:match('help') then
            print(help)
            return
        elseif args[1]:match('-n') or args[1]:match('dry') then
            dry_run = true
        end
    end
    for _,u in pairs(df.global.world.units.active) do
        if u.flags1.merchant and not dfhack.units.isActive (u) then
            print(('%s unit %d: %s (%s), civ %d (%s, %s)'):format(
                dry_run and 'Would remove' or 'Removing',
                u.id,
                dfhack.df2console(dfhack.TranslateName(dfhack.units.getVisibleName(u))),
                df.creature_raw.find(u.race).name[0],
                u.civ_id,
                dfhack.df2console(getEntityName(u)),
                getEntityRace(u)
            ))
            if not dry_run then
                u.flags1.left = true
            end
        end
    end
end

if not dfhack_flags.module then
    dismissMerchants{...}
end
