-- On map load writes information about the loaded region to gamelog.txt
-- By Kurik Amudnil and Warmist (http://www.bay12forums.com/smf/index.php?topic=91166.msg4467072#msg4467072)

local function write_gamelog(msg)
    local log = io.open('gamelog.txt', 'a')
    log:write(msg.."\n")
    log:close()
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
            local worldname = df.global.world.world_data.name
                -- site positions
                -- site  .pos.x  .pos.y
                -- site  .rgn_min_x  .rgn_min_y  .rgn_max_x  .rgn_max.y
                -- site  .global_min_x  .global_min_y  .global_max_x  .global_max_y
                --site.name
                --fort_ent.name
                --civ_ent.name
                
                write_gamelog('Loaded '..df.global.world.cur_savegame.save_dir..', '..dfhack.TranslateName(worldname)..' ('..dfhack.TranslateName(worldname ,true)..') at coordinates ('..site.pos.x..','..site.pos.y..')'..NEWLINE..
                  'Loaded the fortress '..dfhack.TranslateName(site.name)..' ('..dfhack.TranslateName(site.name, true)..'), colonized by the group '..dfhack.TranslateName(fort_ent.name)..' ('..dfhack.TranslateName(fort_ent.name,true)..
                      ') of the civilization '..dfhack.TranslateName(civ_ent.name)..' ('..dfhack.TranslateName(civ_ent.name,true)..').'..NEWLINE)
            end
    end
    end
end
