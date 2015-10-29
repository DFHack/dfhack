-- Adjust all preferences of all dwarves in play
-- by vjek
--[[=begin

pref-adjust
===========
A two-stage script:  ``pref-adjust clear`` removes preferences from all dwarves,
and ``pref-adjust`` inserts an 'ideal' set which is easy to satisfy::

    Feb Idashzefon likes wild strawberries for their vivid red color,
    fisher berries for their round shape, prickle berries for their
    precise thorns, plump helmets for their rounded tops, prepared meals,
    plants, drinks, doors, thrones, tables and beds. When possible, she
    prefers to consume wild strawberries, fisher berries, prickle
    berries, plump helmets, strawberry wine, fisher berry wine, prickle
    berry wine, and dwarven wine.

=end]]

-- ---------------------------------------------------------------------------
function brainwash_unit(unit)

    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end

    local pss_counter=31415926

    local prefcount = #(unit.status.current_soul.preferences)
    print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

    utils = require 'utils'
    -- below populates an array with all creature names and id's, used for 'detests...'
    rtbl={}
    vec=df.global.world.raws.creatures.all
    for k=0,#vec-1 do
            local name=vec[k].creature_id
            rtbl[name]=k
    end

    -- Now iterate through for the type 3 detests...
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 3 , item_type = rtbl.TROLL , creature_id = rtbl.TROLL , color_id = rtbl.TROLL , shape_id = rtbl.TROLL , plant_id = rtbl.TROLL , item_subtype = -1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 3 , item_type = rtbl.BIRD_BUZZARD , creature_id = rtbl.BIRD_BUZZARD , color_id = rtbl.BIRD_BUZZARD , shape_id = rtbl.BIRD_BUZZARD , plant_id = rtbl.BIRD_BUZZARD , item_subtype = -1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 3 , item_type = rtbl.BIRD_VULTURE , creature_id = rtbl.BIRD_VULTURE , color_id = rtbl.BIRD_VULTURE , shape_id = rtbl.BIRD_VULTURE , plant_id = rtbl.BIRD_VULTURE , item_subtype = -1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 3 , item_type = rtbl.CRUNDLE , creature_id = rtbl.CRUNDLE , color_id = rtbl.CRUNDLE , shape_id = rtbl.CRUNDLE , plant_id = rtbl.CRUNDLE , item_subtype = -1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    -- and the type 4 likes
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = -1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.ARMOR , creature_id = df.item_type.ARMOR , color_id = df.item_type.ARMOR , shape_id = df.item_type.ARMOR , plant_id = df.item_type.ARMOR , item_subtype = -1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.SHIELD , creature_id = df.item_type.SHIELD , color_id = df.item_type.SHIELD , shape_id = df.item_type.SHIELD , plant_id = df.item_type.SHIELD , item_subtype = -1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    -- prefers plump helmets for their ...
    local ph_mat_type=dfhack.matinfo.find("MUSHROOM_HELMET_PLUMP:STRUCTURAL").index
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 5 , item_type = ph_mat_type , creature_id = ph_mat_type , color_id = ph_mat_type , shape_id = ph_mat_type , plant_id = ph_mat_type , item_subtype = -1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    -- prefers to consume dwarven wine:
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 2 , item_type = 68 , creature_id = 68 , color_id = 68 , shape_id = 68 , plant_id = 68 , item_subtype = -1 , mattype = dfhack.matinfo.find("MUSHROOM_HELMET_PLUMP:DRINK").type , matindex = dfhack.matinfo.find("MUSHROOM_HELMET_PLUMP:DRINK").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    -- likes iron, steel (0,8) adam is 25
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("IRON").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
    pss_counter = pss_counter + 1
    utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

    prefcount = #(unit.status.current_soul.preferences)
    print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function clear_preferences(v)
    unit=v

    local prefs=unit.status.current_soul.preferences
    for index,pref in ipairs(prefs) do
        pref:delete()
    end
    prefs:resize(0)
end
-- ---------------------------------------------------------------------------
function clearpref_all_dwarves()
    for _,v in ipairs(df.global.world.units.active) do
        if v.race == df.global.ui.race_id then
            print("Clearing Preferences for "..dfhack.TranslateName(dfhack.units.getVisibleName(v)))
            clear_preferences(v)
        end
    end
end
-- ---------------------------------------------------------------------------
function adjust_all_dwarves()
    for _,v in ipairs(df.global.world.units.active) do
        if v.race == df.global.ui.race_id then
            print("Adjusting "..dfhack.TranslateName(dfhack.units.getVisibleName(v)))
            brainwash_unit(v)
        end
    end
end
-- ---------------------------------------------------------------------------
-- main script operation starts here
-- ---------------------------------------------------------------------------
clearpref_all_dwarves()
adjust_all_dwarves()
