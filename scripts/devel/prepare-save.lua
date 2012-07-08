-- Prepare the current save for use with devel/find-offsets.

local utils = require 'utils'

df.global.pause_state = true

print[[
WARNING: THIS SCRIPT IS STRICTLY FOR DFHACK DEVELOPERS.

This script prepares the current savegame to be used
with devel/find-offsets. It CHANGES THE GAME STATE
to predefined values, and initiates an immediate
quicksave, thus PERMANENTLY MODIFYING the save.
]]

if not utils.prompt_yes_no('Proceed?') then
    return
end

--[[print('Placing anchor...')

do
    local wp = df.global.ui.waypoints

    for _,pt in ipairs(wp.points) do
        if pt.name == 'dfhack_anchor' then
            print('Already placed.')
            goto found
        end
    end

    local x,y,z = pos2xyz(df.global.cursor)

    if not x then
        error("Place cursor at your preferred anchor point.")
    end

    local id = wp.next_point_id
    wp.next_point_id = id + 1

    wp.points:insert('#',{
        new = true, id = id, name = 'dfhack_anchor',
        comment=(x..','..y..','..z),
        tile = string.byte('!'), fg_color = COLOR_LIGHTRED, bg_color = COLOR_BLUE,
        pos = xyz2pos(x,y,z)
    })

::found::
end]]

print('Nicknaming units...')

for i,unit in ipairs(df.global.world.units.active) do
    dfhack.units.setNickname(unit, i..':'..unit.id)
end

print('Setting weather...')

local wbytes = {
    2, 1, 0, 2, 0,
    1, 2, 1, 0, 0,
    2, 0, 2, 1, 2,
    1, 2, 0, 1, 1,
    2, 0, 1, 0, 2
}

for i=0,4 do
    for j = 0,4 do
        df.global.current_weather[i][j] = (wbytes[i*5+j+1] or 2)
    end
end

local yearstr = df.global.cur_year..','..df.global.cur_year_tick

print('Cur year and tick: '..yearstr)

dfhack.persistent.save{
    key='prepare-save/cur_year',
    value=yearstr,
    ints={df.global.cur_year, df.global.cur_year_tick}
}

-- Save

dfhack.run_script('quicksave')

