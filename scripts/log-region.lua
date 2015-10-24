-- On map load writes information about the loaded region to gamelog.txt
-- By Kurik Amudnil and Warmist (http://www.bay12forums.com/smf/index.php?topic=91166.msg4467072#msg4467072)
--[[=begin

log-region
==========
When enabled in :file:`dfhack.init`, each time a fort is loaded identifying information
will be written to the gamelog.  Assists in parsing the file if you switch
between forts, and adds information for story-building.

=end]]

local function write_gamelog(msg)
    local log = io.open('gamelog.txt', 'a')
    log:write(msg.."\n")
    log:close()
end

local function fullname(item)
    return dfhack.TranslateName(item.name)..' ('..dfhack.TranslateName(item.name ,true)..')'
end

local args = {...}
if args[1] == 'disable' then
    dfhack.onStateChange[_ENV] = nil
else
    dfhack.onStateChange[_ENV] = function(op)
        if op == SC_WORLD_LOADED then
            if df.world_site.find(df.global.ui.site_id) ~= nil then -- added this check, now only attempts write in fort mode
                local site = df.world_site.find(df.global.ui.site_id)
                local fort_ent = df.global.ui.main.fortress_entity
                local civ_ent = df.historical_entity.find(df.global.ui.civ_id)
                local world = df.global.world
                -- site positions
                -- site  .pos.x  .pos.y
                -- site  .rgn_min_x  .rgn_min_y  .rgn_max_x  .rgn_max.y
                -- site  .global_min_x  .global_min_y  .global_max_x  .global_max_y
                --site.name
                --fort_ent.name
                --civ_ent.name

                write_gamelog('Loaded '..world.cur_savegame.save_dir..', '..fullname(world.world_data)..
                  ' at coordinates ('..site.pos.x..','..site.pos.y..')'..NEWLINE..
                  'Loaded the fortress '..fullname(site)..
                  (fort_ent and ', colonized by the group '..fullname(fort_ent) or '')..
                  (civ_ent and ' of the civilization '..fullname(civ_ent) or '')..'.'..NEWLINE)
            end
        end
    end
end
