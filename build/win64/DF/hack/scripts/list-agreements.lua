-- list location agreements with guilds or religions

--[====[

list-agreements
===============

Lists Guildhall and Temple agreements in fortress mode.
In addition:

* Translates names of the associated Guilds and Orders respectively
* Displays worshiped Professions and Deities respectively
* Petition age and status satisfied, denied or expired, or blank for outstanding

Usage::

    list-agreements [options]

Examples:

    ``list-agreements``
        List outstanding, unfullfilled location agreements.

    ``list-agreements all``
        Lists all location agreements, whether satisfied, denied, or expired.

Options:

:all:   list all agreements; past and present
:help:  shows this help screen

]====]
local playerfortid = df.global.plotinfo.site_id -- Player fortress id
local templeagreements = {} -- Table of agreements for temples in player fort
local guildhallagreements = {} -- Table of agreements for guildhalls in player fort

function get_location_name(loctier,loctype)
    local locstr = "Unknown Location"
    if loctype == df.abstract_building_type.TEMPLE and loctier == 1 then
        locstr = "Temple"
    elseif loctype == df.abstract_building_type.TEMPLE and loctier == 2 then
        locstr = "Temple Complex"
    elseif loctype == df.abstract_building_type.GUILDHALL and loctier == 1 then
        locstr = "Guildhall"
    elseif loctype == df.abstract_building_type.GUILDHALL and loctier == 2 then
        locstr = "Grand Guildhall"
    end
    return locstr
end

function get_location_type(agr)
    loctype = agr.details[0].data.Location.type
    return loctype
end

function get_petition_date(agr)
    local agr_year = agr.details[0].year
    local agr_year_tick = agr.details[0].year_tick
    local julian_day = math.floor(agr_year_tick / 1200) + 1
    local agr_month = math.floor(julian_day / 28) + 1
    local agr_day = julian_day % 28
    return string.format("%03d-%02d-%02d",agr_year, agr_month, agr_day)
end

function get_petition_age(agr)
    local agr_year_tick = agr.details[0].year_tick
    local agr_year = agr.details[0].year
    local cur_year_tick = df.global.cur_year_tick
    local cur_year = df.global.cur_year
    local del_year, del_year_tick
    --delta, check to prevent off by 1 error, not validated
    if cur_year_tick > agr_year_tick then
        del_year = cur_year - agr_year
        del_year_tick = cur_year_tick - agr_year_tick
    else
        del_year = cur_year - agr_year - 1
        del_year_tick = agr_year_tick - cur_year_tick
    end
    local julian_day = math.floor(del_year_tick / 1200) + 1
    local del_month = math.floor(julian_day / 28)
    local del_day = julian_day % 28
    return {del_year,del_month,del_day}
end

function get_guildhall_profession(agr)
    local prof = agr.details[0].data.Location.profession
    local profname = string.lower(df.profession[prof])
    -- *VERY* important code follows
    if string.find(profname, "man") then
        profname = string.gsub(profname,"man",string.lower(dfhack.units.getRaceNameById(df.global.plotinfo.race_id)))
    end
    if not profname then
        profname = 'profession-less'
    end
    return profname:gsub("_", " ")
end

function get_agr_party_name(agr)
    --assume party 0 is guild/order, 1 is local government as siteid = playerfortid
    local party_id = agr.parties[0].entity_ids[0]
    local party_name = dfhack.TranslateName(df.global.world.entities.all[party_id].name, true)
    if not party_name then
        party_name = 'An Unknown Entity or Group'
    end
    return party_name
end

function get_deity_name(agr)
    local religion_id = agr.details[0].data.Location.deity_data.Religion
    local deities = df.global.world.entities.all[religion_id].relations.deities
    if #deities == 0 then return 'An Unknown Deity' end
    return dfhack.TranslateName(df.global.world.history.figures[deities[0]].name,true)
end

--get resolution status, and string
function is_resolved(agr)
    local res = false
    local res_str = 'outstanding'

    if agr.flags.convicted_accepted then
        res = true
        res_str = 'satisfied'
    elseif agr.flags.petition_not_accepted then
        res = true
        res_str = 'denied'
    elseif get_petition_age(agr)[1] ~= 0 then
        res = true
        res_str = 'expired'
    end
    return res,res_str
end

--universal handler
function generate_output(agr,loctype)
    local loc_name = get_location_name(agr.details[0].data.Location.tier,loctype)
    local agr_age = get_petition_age(agr)
    local output_str = 'Establish a '.. loc_name..' for "'..get_agr_party_name(agr)
    local resolved, resolution_string = is_resolved(agr)

    if loctype == df.abstract_building_type.TEMPLE then
        output_str = output_str..'" worshiping "'..get_deity_name(agr)..',"'
    elseif loctype == df.abstract_building_type.GUILDHALL then
        output_str = output_str..'", a '..get_guildhall_profession(agr)..' guild,'
    else
        print("Agreement with unknown org")
        return
    end

    output_str = output_str..'\n\tas agreed on '..get_petition_date(agr)..'. \t'..agr_age[1]..'y, '..agr_age[2]..'m, '..agr_age[3]..'d ago'

    -- can print '(outstanding)' status here, but imho none is cleaner
    if not resolved then
        print(output_str)
    else
        print(output_str..' ('..resolution_string..')')
    end
end

---------------------------------------------------------------------------
-- Main Script operation
---------------------------------------------------------------------------

local args = {...}
local cmd = args[1]
local cull_resolved = true

-- args handler
if cmd then
    if cmd == "all" then
        cull_resolved = false
    elseif cmd == "help" then
        print(dfhack.script_help())
        return
    else
        print('argument ``'..cmd..'`` not supported')
        print(dfhack.script_help())
        return
    end
end

for _, agr in pairs(df.agreement.get_vector()) do
    if agr.details[0].data.Location.site == playerfortid then
        if not is_resolved(agr) or not cull_resolved then
            if get_location_type(agr) == df.abstract_building_type.TEMPLE then
                table.insert(templeagreements, agr)
            elseif get_location_type(agr) == df.abstract_building_type.GUILDHALL then
                table.insert(guildhallagreements, agr)
            end
        end
    end
end

print "-----------------------"
print "Agreements for Temples:"
print "-----------------------"
if next(templeagreements) == nil then
    if cull_resolved then
        print "No outstanding agreements"
    else
        print "No agreements"
    end
else
    for _, agr in pairs(templeagreements) do
        generate_output(agr,df.abstract_building_type.TEMPLE)
    end
end

print ""
print "--------------------------"
print "Agreements for Guildhalls:"
print "--------------------------"
if next(guildhallagreements) == nil then
    if cull_resolved then
        print "No outstanding agreements"
    else
        print "No agreements"
    end
else
    for _, agr in pairs(guildhallagreements) do
        generate_output(agr,df.abstract_building_type.GUILDHALL)
    end
end
