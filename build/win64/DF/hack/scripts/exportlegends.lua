-- Export everything from legends mode
--luacheck-flags: strictsubtype
--@ module=true

local gui = require('gui')
local overlay = require('plugins.overlay')
local script = require('gui.script')
local widgets = require('gui.widgets')

local args = {...}

-- Get the date of the world as a string
-- Format: "YYYYY-MM-DD"
local function get_world_date_str()
    local month = dfhack.world.ReadCurrentMonth() + 1 --days and months are 1-indexed
    local day = dfhack.world.ReadCurrentDay()
    local date_str = string.format('%05d-%02d-%02d', df.global.cur_year, month, day)
    return date_str
end

local function escape_xml(str)
    return str:gsub('&', '&amp;'):gsub('<', '&lt;'):gsub('>', '&gt;')
end

local function getItemSubTypeName(itemType, subType)
    if (dfhack.items.getSubtypeCount(itemType)) <= 0 then
        return tostring(-1)
    end
    local subtype_def = dfhack.items.getSubtypeDef(itemType, subType)
    if (subtype_def == nil) then
        return tostring(-1)
    else
        return escape_xml(dfhack.df2utf(subtype_def.name:lower()))
    end
end

-- is this faster than pcall?
local function table_containskey(self, key)
    for value, _ in pairs(self) do
        if value == key then
            return true
        end
    end
    return false
end

progress_item = progress_item or ''
step_size = step_size or 1
step_percent = -1
progress_percent = progress_percent or -1
last_update_ms = 0

-- should be frequent enough so that user can still effectively use
-- the vanilla legends UI to browse while export is in progress
local YIELD_TIMEOUT_MS = 10

local function yield_if_timeout()
    local now_ms = dfhack.getTickCount()
    if now_ms - last_update_ms > YIELD_TIMEOUT_MS then
        script.sleep(1, 'frames')
        last_update_ms = dfhack.getTickCount()
    end
end

--luacheck: skip
local function progress_ipairs(vector, desc, interval)
    desc = desc or 'item'
    interval = interval or 10000
    local cb = ipairs(vector)
    return function(vector, k, ...)
        if k then
            progress_percent = math.max(progress_percent, step_percent + ((k * step_size) // #vector))
            if #vector >= interval and (k % interval == 0 or k == #vector - 1) then
                print(('        %s %i/%i (%0.f%%)'):format(desc, k, #vector, (k * 100) / #vector))
            end
        end
        yield_if_timeout()
        return cb(vector, k)
    end, vector, nil
end

-- wrapper that returns "unknown N" for df.enum_type[BAD_VALUE],
-- instead of returning nil or causing an error
local df_enums = {} --as:df
setmetatable(df_enums, {
    __index = function(self, enum)
        if not df[enum] or df[enum]._kind ~= 'enum-type' then
            error('invalid enum: ' .. enum)
        end
        local t = {}
        setmetatable(t, {
            __index = function(self, k)
                return df[enum][k] or ('unknown ' .. k)
            end
        })
        return t
    end,
    __newindex = function() error('read-only') end
})

-- prints a line with the value inside the tags if the value isn't -1. Intended to be used
-- for fields where -1 is a known "no info" value. Relies on 'indentation' being set to indicate
-- the current indentation level
local function printifvalue (file, indentation, tag, value)
    if value ~= -1 then
        file:write(string.rep("\t", indentation).."<"..tag..">"..tostring(value).."</"..tag..">\n")
    end
end

-- Export additional legends data, legends_plus.xml
local function export_more_legends_xml()
    local problem_elements = {}

    local filename = df.global.world.cur_savegame.save_dir.."-"..get_world_date_str().."-legends_plus.xml"
    local file = io.open(filename, 'w')
    if not file then
      qerror("could not open file: " .. filename)
      return -- so lint understands what's going on
    end

    file:write("<?xml version=\"1.0\" encoding='UTF-8'?>\n")
    file:write("<df_world>\n")
    file:write("<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(df.global.world.world_data.name))).."</name>\n")
    file:write("<altname>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(df.global.world.world_data.name,1))).."</altname>\n")

    local function write_chunk(name, fn)
        progress_item = name
        yield_if_timeout()
        file:write("<" .. name .. ">\n")
        fn()
        file:write("</" .. name .. ">\n")
    end

    local chunks = {}

    table.insert(chunks, {name='landmasses', fn=function()
    for landmassK, landmassV in progress_ipairs(df.global.world.world_data.landmasses, 'landmass') do
        file:write("\t<landmass>\n")
        file:write("\t\t<id>"..landmassV.index.."</id>\n")
        file:write("\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(landmassV.name,1))).."</name>\n")
        file:write("\t\t<coord_1>"..landmassV.min_x..","..landmassV.min_y.."</coord_1>\n")
        file:write("\t\t<coord_2>"..landmassV.max_x..","..landmassV.max_y.."</coord_2>\n")
        file:write("\t</landmass>\n")
    end
    end})

    table.insert(chunks, {name='mountain_peaks', fn=function()
    for mountainK, mountainV in progress_ipairs(df.global.world.world_data.mountain_peaks, 'mountain') do
        file:write("\t<mountain_peak>\n")
        file:write("\t\t<id>"..mountainK.."</id>\n")
        file:write("\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(mountainV.name,1))).."</name>\n")
        file:write("\t\t<coords>"..mountainV.pos.x..","..mountainV.pos.y.."</coords>\n")
        file:write("\t\t<height>"..mountainV.height.."</height>\n")
        if mountainV.flags.is_volcano then
            file:write("\t\t<is_volcano/>\n")
        end
        file:write("\t</mountain_peak>\n")
    end
    end})

    table.insert(chunks, {name='regions', fn=function()
    for regionK, regionV in progress_ipairs(df.global.world.world_data.regions, 'region') do
        file:write("\t<region>\n")
        file:write("\t\t<id>"..regionV.index.."</id>\n")
        file:write("\t\t<coords>")
        for xK, xVal in ipairs(regionV.region_coords.x) do
           file:write(xVal..","..regionV.region_coords.y[xK].."|")
        end
        file:write("</coords>\n")
        local evilness = "neutral"
        if regionV.evil then
           evilness = "evil"
        elseif regionV.good then
           evilness = "good"
        end
        file:write("\t\t<evilness>"..evilness.."</evilness>\n")
        for forceK, forceVal in ipairs(regionV.forces) do
           file:write("\t\t<force_id>"..forceVal.."</force_id>\n")
        end
        file:write("\t</region>\n")
    end
    end})

    table.insert(chunks, {name='underground_regions', fn=function()
    for regionK, regionV in progress_ipairs(df.global.world.world_data.underground_regions, 'underground region') do
        file:write("\t<underground_region>\n")
        file:write("\t\t<id>"..regionV.index.."</id>\n")
        file:write("\t\t<coords>")
            for xK, xVal in ipairs(regionV.region_coords.x) do
                file:write(xVal..","..regionV.region_coords.y[xK].."|")
            end
        file:write("</coords>\n")
        file:write("\t</underground_region>\n")
    end
    end})

    table.insert(chunks, {name='rivers', fn=function()
    for riverK, riverV in progress_ipairs(df.global.world.world_data.rivers, 'river') do
        file:write("\t<river>\n")
        file:write("\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(riverV.name, 1))).."</name>\n")
        file:write("\t\t<path>")
        for pathK, pathV in progress_ipairs(riverV.path.x, 'river section') do
            file:write(pathV..","..riverV.path.y[pathK]..",")
            file:write(riverV.flow[pathK]..",")
            file:write(riverV.exit_tile[pathK]..",")
            file:write(riverV.elevation[pathK].."|")
        end
        file:write("</path>\n")
        file:write("\t\t<end_pos>"..riverV.end_pos.x..","..riverV.end_pos.y.."</end_pos>\n")
        file:write("\t</river>\n")
    end
    end})

    table.insert(chunks, {name='creature_raw', fn=function()
    for creatureK, creatureV in ipairs (df.global.world.raws.creatures.all) do
        file:write("\t<creature>\n")
        file:write("\t\t<creature_id>"..creatureV.creature_id.."</creature_id>\n")
        file:write("\t\t<name_singular>"..escape_xml(dfhack.df2utf(creatureV.name[0])).."</name_singular>\n")
        file:write("\t\t<name_plural>"..escape_xml(dfhack.df2utf(creatureV.name[1])).."</name_plural>\n")
        for flagK, flagV in ipairs (df.creature_raw_flags) do
            if creatureV.flags[flagV] then
                file:write("\t\t<"..flagV:lower().."/>\n")
            end
        end
        file:write("\t</creature>\n")
    end
    end})

    table.insert(chunks, {name='sites', fn=function()
    for siteK, siteV in progress_ipairs(df.global.world.world_data.sites, 'site') do
        file:write("\t<site>\n")
        for k,v in pairs(siteV) do
            if (k == "id" or k == "civ_id" or k == "cur_owner_id") then
                printifvalue(file, 2, k, v)
                -- file:write("\t\t<"..k..">"..tostring(v).."</"..k..">\n")
            elseif (k == "buildings") then
                if (#siteV.buildings > 0) then
                    file:write("\t\t<structures>\n")
                    for buildingK, buildingV in ipairs(siteV.buildings) do
                        file:write("\t\t\t<structure>\n")
                        file:write("\t\t\t\t<id>"..buildingV.id.."</id>\n")
                        file:write("\t\t\t\t<type>"..df_enums.abstract_building_type[buildingV:getType()]:lower().."</type>\n")
                        if table_containskey(buildingV,"name") then
                            file:write("\t\t\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(buildingV.name, 1))).."</name>\n")
                            file:write("\t\t\t\t<name2>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(buildingV.name))).."</name2>\n")
                        end
                        if df.abstract_building_templest:is_instance(buildingV) then
                            file:write("\t\t\t\t<deity_type>"..buildingV.deity_type.."</deity_type>\n")
                            if buildingV.deity_type == df.temple_deity_type.Deity then
                                file:write("\t\t\t\t<deity>"..buildingV.deity_data.Deity.."</deity>\n")
                            elseif buildingV.deity_type == df.temple_deity_type.Religion then
                                file:write("\t\t\t\t<religion>"..buildingV.deity_data.Religion.."</religion>\n")
                            end
                        end
                        if df.abstract_building_dungeonst:is_instance(buildingV) then
                            file:write("\t\t\t\t<dungeon_type>"..buildingV.dungeon_type.."</dungeon_type>\n")
                        end
                        for inhabitabntK,inhabitabntV in pairs(buildingV.inhabitants) do
                            file:write("\t\t\t\t<inhabitant>"..inhabitabntV.histfig_id.."</inhabitant>\n")
                        end
                        file:write("\t\t\t</structure>\n")
                    end
                    file:write("\t\t</structures>\n")
                end
            end
        end
        file:write("\t</site>\n")
    end
    end})

    table.insert(chunks, {name='world_constructions', fn=function()
    for wcK, wcV in progress_ipairs(df.global.world.world_data.constructions.list, 'construction') do
        file:write("\t<world_construction>\n")
        file:write("\t\t<id>"..wcV.id.."</id>\n")
        file:write("\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(wcV.name,1))).."</name>\n")
        file:write("\t\t<type>"..(df_enums.world_construction_type[wcV:getType()]):lower().."</type>\n")
        file:write("\t\t<coords>")
        for xK, xVal in ipairs(wcV.square_pos.x) do
            file:write(xVal..","..wcV.square_pos.y[xK].."|")
        end
        file:write("</coords>\n")
        file:write("\t</world_construction>\n")
    end
    end})

    table.insert(chunks, {name='artifacts', fn=function()
    for artifactK, artifactV in progress_ipairs(df.global.world.artifacts.all, 'artifact') do
        file:write("\t<artifact>\n")
        file:write("\t\t<id>"..artifactV.id.."</id>\n")
        local item = artifactV.item
        if df.item_constructed:is_instance(item) then
            file:write("\t\t<item_type>"..tostring(df_enums.item_type[item:getType()]):lower().."</item_type>\n")
            if (item:getSubtype() ~= -1) then --luacheck: skip
                file:write("\t\t<item_subtype>"..escape_xml(dfhack.df2utf(item.subtype.name)).."</item_subtype>\n")
            end
            for improvementK,impovementV in pairs(item.improvements) do
                if df.itemimprovement_writingst:is_instance(impovementV) then
                    for writingk,writingV in pairs(impovementV.contents) do
                        file:write("\t\t<writing>"..writingV.."</writing>\n")
                    end
                elseif df.itemimprovement_pagesst:is_instance(impovementV) then
                    file:write("\t\t<page_count>"..impovementV.count.."</page_count>\n")
                    for writingk,writingV in pairs(impovementV.contents) do
                        file:write("\t\t<writing>"..writingV.."</writing>\n")
                    end
                end
            end
        end
        if (table_containskey(item,"description")) then
            file:write("\t\t<item_description>"..dfhack.df2utf(item.description:lower()).."</item_description>\n")
        end
        if item:getMaterial() ~= -1 then
            file:write("\t\t<mat>"..dfhack.df2utf(dfhack.matinfo.toString(dfhack.matinfo.decode(item:getMaterial(), item:getMaterialIndex()))).."</mat>\n")
        end
        file:write("\t</artifact>\n")
    end
    end})

    table.insert(chunks, {name='historical_figures', fn=function()
    for hfK, hfV in progress_ipairs(df.global.world.history.figures, 'historical figure') do
        file:write("\t<historical_figure>\n")
        file:write("\t\t<id>"..hfV.id.."</id>\n")
        file:write("\t\t<sex>"..hfV.sex.."</sex>\n")
        if hfV.race >= 0 then file:write("\t\t<race>"..escape_xml(dfhack.df2utf(df.creature_raw.find(hfV.race).name[0])).."</race>\n") end
        file:write("\t</historical_figure>\n")
    end
    end})

    table.insert(chunks, {name='identities', fn=function()
    for idK, idV in progress_ipairs(df.global.world.identities.all, 'identity') do
        file:write("\t<identity>\n")
        file:write("\t\t<id>"..idV.id.."</id>\n")
        file:write("\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(idV.name,1))).."</name>\n")
        local id_tag = df.identity_type.attrs[idV.type].id_tag
        if id_tag then
            file:write("\t\t<"..id_tag..">"..idV[id_tag].."</"..id_tag..">\n")
        else
            dfhack.printerr ("Unknown df.identity_type value encountered: "..tostring(idV.type)..". Please report to DFHack team.")
        end
        if idV.race >= 0 then file:write("\t\t<race>"..(df.global.world.raws.creatures.all[idV.race].creature_id):lower().."</race>\n") end
        if idV.race >= 0  and idV.caste >= 0 then file:write("\t\t<caste>"..(df.global.world.raws.creatures.all[idV.race].caste[idV.caste].caste_id):lower().."</caste>\n") end
        file:write("\t\t<birth_year>"..idV.birth_year.."</birth_year>\n")
        file:write("\t\t<birth_second>"..idV.birth_second.."</birth_second>\n")
        if idV.profession >= 0 then file:write("\t\t<profession>"..(df_enums.profession[idV.profession]):lower().."</profession>\n") end
        file:write("\t\t<entity_id>"..idV.entity_id.."</entity_id>\n")
        file:write("\t</identity>\n")
    end
    end})

    table.insert(chunks, {name='entity_populations', fn=function()
    for entityPopK, entityPopV in progress_ipairs(df.global.world.entity_populations, 'entity population') do
        file:write("\t<entity_population>\n")
        file:write("\t\t<id>"..entityPopV.id.."</id>\n")
        for raceK, raceV in ipairs(entityPopV.races) do
            local raceName = (df.global.world.raws.creatures.all[raceV].creature_id):lower()
            file:write("\t\t<race>"..raceName..":"..entityPopV.counts[raceK].."</race>\n")
        end
        file:write("\t\t<civ_id>"..entityPopV.civ_id.."</civ_id>\n")
        file:write("\t</entity_population>\n")
    end
    end})

    table.insert(chunks, {name='entities', fn=function()
    for entityK, entityV in progress_ipairs(df.global.world.entities.all, 'entity') do
        file:write("\t<entity>\n")
        file:write("\t\t<id>"..entityV.id.."</id>\n")
        if entityV.race >= 0 then
            file:write("\t\t<race>"..(df.global.world.raws.creatures.all[entityV.race].creature_id):lower().."</race>\n")
        end
        file:write("\t\t<type>"..(df_enums.historical_entity_type[entityV.type]):lower().."</type>\n")
        if entityV.type == df.historical_entity_type.Religion or entityV.type == df.historical_entity_type.MilitaryUnit then -- Get worshipped figures
            if (entityV.relations ~= nil and entityV.relations.deities ~= nil) then
                for k,v in pairs(entityV.relations.deities) do
                    file:write("\t\t<worship_id>"..v.."</worship_id>\n")
                end
            end
        end
        if entityV.type == df.historical_entity_type.MilitaryUnit then -- Get favorite weapons
            if (entityV.resources ~= nil and entityV.resources.weapon_type ~= nil) then
                for weaponK,weaponID in pairs(entityV.resources.weapon_type) do
                    file:write("\t\t<weapon>"..getItemSubTypeName(df.item_type.WEAPON, weaponID).."</weapon>\n")
                end
            end
        end
        if entityV.type == df.historical_entity_type.Guild then -- Get profession
           for professionK,professionV in pairs(entityV.guild_professions) do
              file:write("\t\t<profession>"..df_enums.profession[professionV.profession]:lower().."</profession>\n")
           end
        end
        for id, link in pairs(entityV.entity_links) do
            file:write("\t\t<entity_link>\n")
                for k, v in pairs(link) do
                    if (k == "type") then
                        file:write("\t\t\t<"..k..">"..tostring(df_enums.entity_entity_link_type[v]).."</"..k..">\n")
                    else
                        file:write("\t\t\t<"..k..">"..v.."</"..k..">\n")
                    end
                end
            file:write("\t\t</entity_link>\n")
        end
        for positionK,positionV in pairs(entityV.positions.own) do
            file:write("\t\t<entity_position>\n")
            file:write("\t\t\t<id>"..positionV.id.."</id>\n")
            if positionV.name[0]          ~= "" then file:write("\t\t\t<name>"..escape_xml(positionV.name[0]).."</name>\n") end
            if positionV.name_male[0]     ~= "" then file:write("\t\t\t<name_male>"..escape_xml(positionV.name_male[0]).."</name_male>\n") end
            if positionV.name_female[0]   ~= "" then file:write("\t\t\t<name_female>"..escape_xml(positionV.name_female[0]).."</name_female>\n") end
            if positionV.spouse[0]        ~= "" then file:write("\t\t\t<spouse>"..positionV.spouse[0].."</spouse>\n") end
            if positionV.spouse_male[0]   ~= "" then file:write("\t\t\t<spouse_male>"..positionV.spouse_male[0].."</spouse_male>\n") end
            if positionV.spouse_female[0] ~= "" then file:write("\t\t\t<spouse_female>"..positionV.spouse_female[0].."</spouse_female>\n") end
            file:write("\t\t</entity_position>\n")
        end
        for assignmentK,assignmentV in pairs(entityV.positions.assignments) do
            file:write("\t\t<entity_position_assignment>\n")
            for k, v in pairs(assignmentV) do
                if (k == "id" or k == "histfig" or k == "position_id" or k == "squad_id") then
                    printifvalue(file, 3, k, v)  --  'id' should never be negative, and so won't be suppressed
                end
            end
            file:write("\t\t</entity_position_assignment>\n")
        end
        for idx,id in pairs(entityV.histfig_ids) do
            file:write("\t\t<histfig_id>"..id.."</histfig_id>\n")
        end
        for id, link in ipairs(entityV.children) do
            file:write("\t\t<child>"..link.."</child>\n")
        end

        -- As of version .50, sometimes claim borders have more "x" coordinates than "y", which breaks the below.  Not sure why that would be, so commenting it out.
        -- if #entityV.claims.border.x > 0 then
        --     file:write("\t\t<claims>")
        --     for xK, xVal in ipairs(entityV.claims.border.x) do
        --         file:write(xVal..","..entityV.claims.border.y[xK].."|")
        --     end
        --     file:write("</claims>\n")
        -- end

        if (table_containskey(entityV,"occasion_info") and entityV.occasion_info ~= nil) then
            for occasionK, occasionV in pairs(entityV.occasion_info.occasions) do
                file:write("\t\t<occasion>\n")
                file:write("\t\t\t<id>"..occasionV.id.."</id>\n")
                file:write("\t\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(occasionV.name,1))).."</name>\n")
                file:write("\t\t\t<event>"..occasionV.event.."</event>\n")
                for scheduleK, scheduleV in pairs(occasionV.schedule) do
                    file:write("\t\t\t<schedule>\n")
                    file:write("\t\t\t\t<id>"..scheduleK.."</id>\n")
                    file:write("\t\t\t\t<type>"..df_enums.occasion_schedule_type[scheduleV.type]:lower().."</type>\n")
                    if(scheduleV.type == df.occasion_schedule_type.THROWING_COMPETITION) then
                        file:write("\t\t\t\t<item_type>"..df_enums.item_type[scheduleV.reference]:lower().."</item_type>\n")
                        file:write("\t\t\t\t<item_subtype>"..getItemSubTypeName(scheduleV.reference,scheduleV.reference2).."</item_subtype>\n")
                    else
                        file:write("\t\t\t\t<reference>"..scheduleV.reference.."</reference>\n")
                        file:write("\t\t\t\t<reference2>"..scheduleV.reference2.."</reference2>\n")
                    end
                    for featureK, featureV in pairs(scheduleV.features) do
                        file:write("\t\t\t\t<feature>\n")
                        if(df_enums.occasion_schedule_feature[featureV.feature] ~= nil) then
                            file:write("\t\t\t\t\t<type>"..df_enums.occasion_schedule_feature[featureV.feature]:lower().."</type>\n")
                        else
                            file:write("\t\t\t\t\t<type>"..featureV.feature.."</type>\n")
                        end
                        file:write("\t\t\t\t\t<reference>"..featureV.reference.."</reference>\n")
                        file:write("\t\t\t\t</feature>\n")
                    end
                    file:write("\t\t\t</schedule>\n")
                end
                file:write("\t\t</occasion>\n")
            end
        end
        file:write("\t</entity>\n")
    end
    end})

    table.insert(chunks, {name='historical_events', fn=function()
    for ID, event in progress_ipairs(df.global.world.history.events, 'event') do
        if df.history_event_add_hf_entity_linkst:is_instance(event)
              or df.history_event_add_hf_site_linkst:is_instance(event)
              or df.history_event_add_hf_hf_linkst:is_instance(event)
              or df.history_event_add_hf_entity_linkst:is_instance(event)
              or df.history_event_topicagreement_concludedst:is_instance(event)
              or df.history_event_topicagreement_rejectedst:is_instance(event)
              or df.history_event_topicagreement_madest:is_instance(event)
              or df.history_event_body_abusedst:is_instance(event)
              or df.history_event_change_creature_typest:is_instance(event)
              or df.history_event_change_hf_jobst:is_instance(event)
              or df.history_event_change_hf_statest:is_instance(event)
              or df.history_event_created_buildingst:is_instance(event)
              or df.history_event_creature_devouredst:is_instance(event)
              or df.history_event_hf_does_interactionst:is_instance(event)
              or df.history_event_hf_learns_secretst:is_instance(event)
              or df.history_event_hist_figure_new_petst:is_instance(event)
              or df.history_event_hist_figure_reach_summitst:is_instance(event)
              or df.history_event_item_stolenst:is_instance(event)
              or df.history_event_remove_hf_entity_linkst:is_instance(event)
              or df.history_event_remove_hf_site_linkst:is_instance(event)
              or df.history_event_replaced_buildingst:is_instance(event)
              or df.history_event_masterpiece_created_dye_itemst:is_instance(event)
              or df.history_event_masterpiece_created_arch_constructst:is_instance(event)
              or df.history_event_masterpiece_created_itemst:is_instance(event)
              or df.history_event_masterpiece_created_item_improvementst:is_instance(event)
              or df.history_event_masterpiece_created_foodst:is_instance(event)
              or df.history_event_masterpiece_created_engravingst:is_instance(event)
              or df.history_event_masterpiece_lostst:is_instance(event)
              or df.history_event_entity_actionst:is_instance(event)
              or df.history_event_hf_act_on_buildingst:is_instance(event)
              or df.history_event_artifact_createdst:is_instance(event)
              or df.history_event_assume_identityst:is_instance(event)
              or df.history_event_create_entity_positionst:is_instance(event)
              or df.history_event_diplomat_lostst:is_instance(event)
              or df.history_event_merchantst:is_instance(event)
              or df.history_event_war_peace_acceptedst:is_instance(event)
              or df.history_event_war_peace_rejectedst:is_instance(event)
              or df.history_event_hist_figure_woundedst:is_instance(event)
              or df.history_event_hist_figure_diedst:is_instance(event)
                then
            file:write("\t<historical_event>\n")
            file:write("\t\t<id>"..event.id.."</id>\n")
            file:write("\t\t<type>"..tostring(df_enums.history_event_type[event:getType()]):lower().."</type>\n")
            for k,v in pairs(event) do
                if k == "year" or k == "seconds" or k == "flags" or k == "id"
                    or (k == "region" and not df.history_event_hf_does_interactionst:is_instance(event))
                    or k == "region_pos" or k == "layer" or k == "feature_layer" or k == "subregion"
                    or k == "anon_1" or k == "anon_2" or k == "flags2" or k == "unk1" then
                    -- do notting for these keys
                elseif df.history_event_add_hf_entity_linkst:is_instance(event) and k == "link_type" then
                    file:write("\t\t<"..k..">"..df_enums.histfig_entity_link_type[v]:lower().."</"..k..">\n")
                elseif df.history_event_add_hf_entity_linkst:is_instance(event) and k == "position_id" then
                    local entity = df.historical_entity.find(event.civ)
                    if (entity ~= nil and event.civ > -1 and v > -1) then
                        for entitypositionsK, entityPositionsV in ipairs(entity.positions.own) do
                            if entityPositionsV.id == v then
                                file:write("\t\t<position>"..escape_xml(tostring(entityPositionsV.name[0]):lower()).."</position>\n")
                                break
                            end
                        end
                    else
                        file:write("\t\t<position>-1</position>\n")
                    end
                elseif df.history_event_create_entity_positionst:is_instance(event) and k == "position" then
                    local entity = df.historical_entity.find(event.site_civ)
                    if (entity ~= nil and v > -1) then
                        for entitypositionsK, entityPositionsV in ipairs(entity.positions.own) do
                            if entityPositionsV.id == v then
                                file:write("\t\t<position>"..escape_xml(tostring(entityPositionsV.name[0]):lower()).."</position>\n")
                                break
                            end
                        end
                    else
                        file:write("\t\t<position>-1</position>\n")
                    end
                elseif df.history_event_remove_hf_entity_linkst:is_instance(event) and k == "link_type" then
                    file:write("\t\t<"..k..">"..df_enums.histfig_entity_link_type[v]:lower().."</"..k..">\n")
                elseif df.history_event_remove_hf_entity_linkst:is_instance(event) and k == "position_id" then
                    local entity = df.historical_entity.find(event.civ)
                    if (entity ~= nil and event.civ > -1 and v > -1) then
                        for entitypositionsK, entityPositionsV in ipairs(entity.positions.own) do
                            if entityPositionsV.id == v then
                                file:write("\t\t<position>"..escape_xml(tostring(entityPositionsV.name[0]):lower()).."</position>\n")
                                break
                            end
                        end
                    else
                        file:write("\t\t<position>-1</position>\n")
                    end
                elseif df.history_event_add_hf_hf_linkst:is_instance(event) and k == "type" then
                    file:write("\t\t<link_type>"..df_enums.histfig_hf_link_type[v]:lower().."</link_type>\n")
                elseif df.history_event_add_hf_site_linkst:is_instance(event) and k == "type" then
                    file:write("\t\t<link_type>"..df_enums.histfig_site_link_type[v]:lower().."</link_type>\n")
                elseif df.history_event_remove_hf_site_linkst:is_instance(event) and k == "type" then
                    file:write("\t\t<link_type>"..df_enums.histfig_site_link_type[v]:lower().."</link_type>\n")
                elseif (df.history_event_item_stolenst:is_instance(event) or
                        df.history_event_masterpiece_created_itemst:is_instance(event) or
                        df.history_event_masterpiece_created_item_improvementst:is_instance(event) or
                        df.history_event_masterpiece_created_dye_itemst:is_instance(event)
                        ) and k == "item_type" then
                    file:write("\t\t<item_type>"..df_enums.item_type[v]:lower().."</item_type>\n")
                elseif (df.history_event_item_stolenst:is_instance(event) or
                        df.history_event_masterpiece_created_itemst:is_instance(event) or
                        df.history_event_masterpiece_created_item_improvementst:is_instance(event) or
                        df.history_event_masterpiece_created_dye_itemst:is_instance(event)
                        ) and k == "item_subtype" then
                    if event.item_type > -1 and v > -1 then
                        file:write("\t\t<"..k..">"..getItemSubTypeName(event.item_type,v).."</"..k..">\n")
                    end
                elseif df.history_event_masterpiece_created_foodst:is_instance(event) and k == "item_subtype" then
                    --if event.item_type > -1 and v > -1 then
                        file:write("\t\t<item_type>food</item_type>\n")
                        file:write("\t\t<"..k..">"..getItemSubTypeName(df.item_type.FOOD,v).."</"..k..">\n")
                    --end
                elseif df.history_event_item_stolenst:is_instance(event) and k == "mattype" then
                    if (v > -1) then
                        if (dfhack.matinfo.decode(event.mattype, event.matindex) == nil) then
                            file:write("\t\t<mattype>"..event.mattype.."</mattype>\n")
                            file:write("\t\t<matindex>"..event.matindex.."</matindex>\n")
                        else
                            file:write("\t\t<mat>"..dfhack.df2utf(dfhack.matinfo.toString(dfhack.matinfo.decode(event.mattype, event.matindex))).."</mat>\n")
                        end
                    end
                elseif (df.history_event_artifact_possessedst:is_instance(event) or
                        df.history_event_poetic_form_createdst:is_instance(event) or
                        df.history_event_musical_form_createdst:is_instance(event) or
                        df.history_event_dance_form_createdst:is_instance(event) or
                        df.history_event_written_content_composedst:is_instance(event) or
                        df.history_event_artifact_claim_formedst:is_instance(event) or
                        df.history_event_artifact_givenst:is_instance(event) or
                        df.history_event_entity_dissolvedst:is_instance(event) or
                        df.history_event_item_stolenst:is_instance(event) or
                        df.history_event_artifact_createdst:is_instance(event)) and k == "circumstance" then
                    if event.circumstance.type ~= df.unit_thought_type.None then
                        file:write("\t\t<circumstance>\n")
                        file:write("\t\t\t<type>"..df_enums.unit_thought_type[event.circumstance.type]:lower().."</type>\n")
                        if event.circumstance.type == df.unit_thought_type.Death then
                            printifvalue (file, 3, "death", event.circumstance.data.Death)
                        elseif event.circumstance.type == df.unit_thought_type.Prayer then
                            printifvalue (file, 3, "prayer", event.circumstance.data.Prayer)
                        elseif event.circumstance.type == df.unit_thought_type.DreamAbout then
                            printifvalue (file, 3, "dream_about", event.circumstance.data.DreamAbout)
                        elseif event.circumstance.type == df.unit_thought_type.Defeated then
                            printifvalue (file, 3, "defeated", event.circumstance.data.Defeated)
                        elseif event.circumstance.type == df.unit_thought_type.Murdered then
                            printifvalue (file, 3, "murdered", event.circumstance.data.Murdered)
                        elseif event.circumstance.type == df.unit_thought_type.HistEventCollection then
                            file:write("\t\t\t<hist_event_collection>"..event.circumstance.data.HistEventCollection.."</hist_event_collection>\n")
                        elseif event.circumstance.type == df.unit_thought_type.AfterAbducting then
                            printifvalue (file, 3, "after_abducting", event.circumstance.data.AfterAbducting)
                        end
                        file:write("\t\t</circumstance>\n")
                    end
                elseif (df.history_event_artifact_possessedst:is_instance(event) or
                        df.history_event_poetic_form_createdst:is_instance(event) or
                        df.history_event_musical_form_createdst:is_instance(event) or
                        df.history_event_dance_form_createdst:is_instance(event) or
                        df.history_event_written_content_composedst:is_instance(event) or
                        df.history_event_artifact_claim_formedst:is_instance(event) or
                        df.history_event_artifact_givenst:is_instance(event) or
                        df.history_event_entity_dissolvedst:is_instance(event) or
                        df.history_event_item_stolenst:is_instance(event) or
                        df.history_event_artifact_createdst:is_instance(event)) and k == "reason" then
                    if event.reason.type ~= df.history_event_reason.none then
                        file:write("\t\t<reason>"..df_enums.history_event_reason[event.reason.type]:lower().."</reason>\n")
                        if event.reason.type == df.history_event_reason.glorify_hf then
                            printifvalue (file, 2, "glorify_hf", event.reason.data.glorify_hf)
                        elseif event.reason.type == df.history_event_reason.sanctify_hf then
                            printifvalue (file, 2, "sanctify_hf", event.reason.data.sanctify_hf)
                        elseif event.reason.type == df.history_event_reason.artifact_is_heirloom_of_family_hfid then
                            printifvalue (file, 2, "artifact_is_heirloom_of_family_hfid", event.reason.data.artifact_is_heirloom_of_family_hfid)
                        elseif event.reason.type == df.history_event_reason.artifact_is_symbol_of_entity_position then
                            printifvalue (file, 2, "artifact_is_symbol_of_entity_position", event.reason.data.artifact_is_symbol_of_entity_position)
                        end
                    end
                elseif (df.history_event_masterpiece_created_itemst:is_instance(event) or
                        df.history_event_masterpiece_created_item_improvementst:is_instance(event) or
                        df.history_event_masterpiece_created_foodst:is_instance(event) or
                        df.history_event_masterpiece_created_dye_itemst:is_instance(event)
                        ) and k == "mat_type" then
                    if (v > -1) then
                        if (dfhack.matinfo.decode(event.mat_type, event.mat_index) == nil) then
                            file:write("\t\t<mat_type>"..event.mat_type.."</mat_type>\n")
                            file:write("\t\t<mat_index>"..event.mat_index.."</mat_index>\n")
                        else
                            file:write("\t\t<mat>"..dfhack.df2utf(dfhack.matinfo.toString(dfhack.matinfo.decode(event.mat_type, event.mat_index))).."</mat>\n")
                        end
                    end
                elseif df.history_event_masterpiece_created_item_improvementst:is_instance(event) and k == "imp_mat_type" then
                    if (v > -1) then
                        if (dfhack.matinfo.decode(event.imp_mat_type, event.imp_mat_index) == nil) then
                            file:write("\t\t<imp_mat_type>"..event.imp_mat_type.."</imp_mat_type>\n")
                            file:write("\t\t<imp_mat_index>"..event.imp_mat_index.."</imp_mat_index>\n")
                        else
                            file:write("\t\t<imp_mat>"..dfhack.df2utf(dfhack.matinfo.toString(dfhack.matinfo.decode(event.imp_mat_type, event.imp_mat_index))).."</imp_mat>\n")
                        end
                    end
                elseif df.history_event_masterpiece_created_dye_itemst:is_instance(event) and k == "dye_mat_type" then
                    if (v > -1) then
                        if (dfhack.matinfo.decode(event.dye_mat_type, event.dye_mat_index) == nil) then
                            file:write("\t\t<dye_mat_type>"..event.dye_mat_type.."</dye_mat_type>\n")
                            file:write("\t\t<dye_mat_index>"..event.dye_mat_index.."</dye_mat_index>\n")
                        else
                            file:write("\t\t<dye_mat>"..dfhack.df2utf(dfhack.matinfo.toString(dfhack.matinfo.decode(event.dye_mat_type, event.dye_mat_index))).."</dye_mat>\n")
                        end
                    end

                elseif df.history_event_item_stolenst:is_instance(event) and k == "matindex" then
                    --skip
                elseif df.history_event_item_stolenst:is_instance(event) and k == "item" and v == -1 then
                    --skip
                elseif (df.history_event_masterpiece_created_itemst:is_instance(event) or
                        df.history_event_masterpiece_created_item_improvementst:is_instance(event)
                        ) and k == "mat_index" then
                    --skip
                elseif df.history_event_masterpiece_created_item_improvementst:is_instance(event) and k == "imp_mat_index" then
                    --skip
                elseif (df.history_event_war_peace_acceptedst:is_instance(event) or
                        df.history_event_war_peace_rejectedst:is_instance(event) or
                        df.history_event_topicagreement_concludedst:is_instance(event) or
                        df.history_event_topicagreement_rejectedst:is_instance(event) or
                        df.history_event_topicagreement_madest:is_instance(event)
                        ) and k == "topic" then
                    file:write("\t\t<topic>"..tostring(df_enums.meeting_topic[v]):lower().."</topic>\n")
                elseif df.history_event_masterpiece_created_item_improvementst:is_instance(event) and k == "improvement_type" then
                    file:write("\t\t<improvement_type>"..df_enums.improvement_type[v]:lower().."</improvement_type>\n")
                elseif ((df.history_event_hist_figure_reach_summitst:is_instance(event) and k == "group")
                     or (df.history_event_hist_figure_new_petst:is_instance(event) and k == "group")
                     or (df.history_event_body_abusedst:is_instance(event) and k == "bodies")) then
                    for detailK,detailV in pairs(v) do
                        file:write("\t\t<"..k..">"..detailV.."</"..k..">\n")
                    end
                elseif  df.history_event_hist_figure_new_petst:is_instance(event) and k == "pets" then
                    for detailK,detailV in pairs(v) do
                        file:write("\t\t<"..k..">"..escape_xml(dfhack.df2utf(df.creature_raw.find(detailV).name[0])).."</"..k..">\n")
                    end
                elseif df.history_event_body_abusedst:is_instance(event) and (k == "abuse_data") then
                    if event.abuse_type == df.history_event_body_abusedst.T_abuse_type.Impaled then
                        file:write("\t\t<item_type>"..tostring(df_enums.item_type[event.abuse_data.Impaled.item_type]):lower().."</item_type>\n")
                        file:write("\t\t<item_subtype>"..getItemSubTypeName(event.abuse_data.Impaled.item_type,event.abuse_data.Impaled.item_subtype).."</item_subtype>\n")
                        if (event.abuse_data.Impaled.mat_type > -1) then
                            if (dfhack.matinfo.decode(event.abuse_data.Impaled.mat_type, event.abuse_data.Impaled.mat_index) == nil) then
                                file:write("\t\t<mat_type>"..event.abuse_data.Impaled.mat_type.."</mat_type>\n")
                                file:write("\t\t<mat_index>"..event.abuse_data.Impaled.mat_index.."</mat_index>\n")
                            else
                                file:write("\t\t<item_mat>"..dfhack.df2utf(dfhack.matinfo.toString(dfhack.matinfo.decode(event.abuse_data.Impaled.mat_type, event.abuse_data.Impaled.mat_index))).."</item_mat>\n")
                            end
                        end
                    elseif event.abuse_type == df.history_event_body_abusedst.T_abuse_type.Piled then
                        local val = df.history_event_body_abusedst.T_abuse_data.T_Piled.T_pile_type [event.abuse_data.Piled.pile_type]
                        if not val then
                            file:write("\t\t<pile_type>unknown "..tostring(event.abuse_data.Piled.pile_type).."</pile_type>\n")
                        else
                            file:write("\t\t<pile_type>"..tostring(val):lower().."</pile_type>\n")
                        end
                    elseif event.abuse_type == df.history_event_body_abusedst.T_abuse_type.Flayed then
                        file:write("\t\t<structure>"..tostring(event.abuse_data.Flayed.structure).."</structure>\n")
                    elseif event.abuse_type == df.history_event_body_abusedst.T_abuse_type.Hung then
                        file:write("\t\t<tree>"..tostring(event.abuse_data.Hung.tree).."</tree>\n")
                        if (dfhack.matinfo.decode(event.abuse_data.Hung.mat_type, event.abuse_data.Hung.mat_index) == nil) then
                            file:write("\t\t<mat_type>"..event.abuse_data.Hung.mat_type.."</mat_type>\n")
                            file:write("\t\t<mat_index>"..event.abuse_data.Hung.mat_index.."</mat_index>\n")
                        else
                            file:write("\t\t<item_mat>"..dfhack.df2utf(dfhack.matinfo.toString(dfhack.matinfo.decode(event.abuse_data.Hung.mat_type, event.abuse_data.Hung.mat_index))).."</item_mat>\n")
                        end
                    elseif event.abuse_type == df.history_event_body_abusedst.T_abuse_type.Mutilated then  --  For completeness. No fields
                    elseif event.abuse_type == df.history_event_body_abusedst.T_abuse_type.Animated then
                        file:write("\t\t<interaction>"..tostring(event.abuse_data.Animated.interaction).."</interaction>\n")
                    end
                elseif df.history_event_assume_identityst:is_instance(event) and k == "identity" then
                    local identity = df.identity.find(v)
                    if identity then
                        local id_tag = df.identity_type.attrs[identity.type].id_tag
                        if id_tag == "histfig_id" then
                            printifvalue(file, 2, "identity_histfig_id", identity.histfig_id)
                        elseif id_tag == "nemesis_id" then
                            printifvalue(file, 2, "identity_nemesis_id", identity.nemesis_id)
                        else
                            dfhack.printerr ("Unknown df.identity_type value encountered:"..tostring (identity.type)..". Please report to DFHack team.")
                        end
                        file:write("\t\t<identity_name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(identity.name))).."</identity_name>\n")
                        local craw = df.creature_raw.find(identity.race)
                        if craw then
                            file:write("\t\t<identity_race>"..(craw.creature_id):lower().."</identity_race>\n")
                            file:write("\t\t<identity_caste>"..(craw.caste[identity.caste].caste_id):lower().."</identity_caste>\n")
                        end
                    end
                elseif df.history_event_masterpiece_created_arch_constructst:is_instance(event) and k == "building_type" then
                    file:write("\t\t<building_type>"..df_enums.building_type[v]:lower().."</building_type>\n")
                elseif df.history_event_masterpiece_created_arch_constructst:is_instance(event) and k == "building_subtype" then
                    if (df_enums.building_type[event.building_type]:lower() == "furnace") then
                        file:write("\t\t<building_subtype>"..df_enums.furnace_type[v]:lower().."</building_subtype>\n")
                    elseif v > -1 then
                        file:write("\t\t<building_subtype>"..tostring(v).."</building_subtype>\n")
                    end
                elseif k == "race" then
                    if v > -1 then
                        file:write("\t\t<race>"..escape_xml(dfhack.df2utf(df.global.world.raws.creatures.all[v].name[0])).."</race>\n")
                    end
                elseif k == "caste" then
                    if v > -1 then
                        file:write("\t\t<caste>"..(df.global.world.raws.creatures.all[event.race].caste[v].caste_id):lower().."</caste>\n")
                    end
                elseif k == "interaction" and df.history_event_hf_does_interactionst:is_instance(event) then
                    if #df.global.world.raws.interactions[v].sources > 0 then
                        local str_1 = df.global.world.raws.interactions[v].sources[0].hist_string_1
                        if string.sub (str_1, 1, 1) == " " and string.sub (str_1, string.len (str_1), string.len (str_1)) == " " then
                            str_1 = string.sub (str_1, 2, string.len (str_1) - 1)
                        end
                        file:write("\t\t<interaction_action>"..str_1..df.global.world.raws.interactions[v].sources[0].hist_string_2.."</interaction_action>\n")
                    end
                elseif k == "interaction" and df.history_event_hf_learns_secretst:is_instance(event) then
                    if #df.global.world.raws.interactions[v].sources > 0 then
                        file:write("\t\t<secret_text>"..df.global.world.raws.interactions[v].sources[0].name.."</secret_text>\n")
                    end
                elseif df.history_event_hist_figure_diedst:is_instance(event) and k == "weapon" then
                    for detailK,detailV in pairs(v) do
                        if (detailK == "item") then
                            if detailV > -1 then
                                file:write("\t\t<"..detailK..">"..detailV.."</"..detailK..">\n")
                                local thisItem = df.item.find(detailV)
                                if (thisItem ~= nil) then
                                    if (thisItem.flags.artifact == true) then
                                        for refk,refv in pairs(thisItem.general_refs) do
                                            if (df.general_ref_is_artifactst:is_instance(refv)) then
                                                file:write("\t\t<artifact_id>"..refv.artifact_id.."</artifact_id>\n")
                                                break
                                            end
                                        end
                                    end
                                end
                            end
                        elseif (detailK == "item_type") then
                            if event.weapon.item > -1 then
                                file:write("\t\t<"..detailK..">"..tostring(df_enums.item_type[detailV]):lower().."</"..detailK..">\n")
                            end
                        elseif (detailK == "item_subtype") then
                            if event.weapon.item > -1 and detailV > -1 then
                                file:write("\t\t<"..detailK..">"..getItemSubTypeName(event.weapon.item_type,detailV).."</"..detailK..">\n")
                            end
                        elseif (detailK == "mattype") then
                            if (detailV > -1) then
                                file:write("\t\t<mat>"..dfhack.df2utf(dfhack.matinfo.toString(dfhack.matinfo.decode(event.weapon.mattype, event.weapon.matindex))).."</mat>\n")
                            end
                        elseif (detailK == "matindex") then

                        elseif (detailK == "shooter_item") then
                            if detailV > -1 then
                                file:write("\t\t<"..detailK..">"..detailV.."</"..detailK..">\n")
                                local thisItem = df.item.find(detailV)
                                if  thisItem ~= nil then
                                    if (thisItem.flags.artifact == true) then
                                        for refk,refv in pairs(thisItem.general_refs) do
                                            if (df.general_ref_is_artifactst:is_instance(refv)) then
                                                file:write("\t\t<shooter_artifact_id>"..refv.artifact_id.."</shooter_artifact_id>\n")
                                                break
                                            end
                                        end
                                    end
                                end
                            end
                        elseif (detailK == "shooter_item_type") then
                            if event.weapon.shooter_item > -1 then
                                file:write("\t\t<"..detailK..">"..tostring(df_enums.item_type[detailV]):lower().."</"..detailK..">\n")
                            end
                        elseif (detailK == "shooter_item_subtype") then
                            if event.weapon.shooter_item > -1 and detailV > -1 then
                                file:write("\t\t<"..detailK..">"..getItemSubTypeName(event.weapon.shooter_item_type,detailV).."</"..detailK..">\n")
                            end
                        elseif (detailK == "shooter_mattype") then
                            if (detailV > -1) then
                                file:write("\t\t<shooter_mat>"..dfhack.df2utf(dfhack.matinfo.toString(dfhack.matinfo.decode(event.weapon.shooter_mattype, event.weapon.shooter_matindex))).."</shooter_mat>\n")
                            end
                        elseif (detailK == "shooter_matindex") then
                            --skip
                        elseif detailK == "slayer_race" or detailK == "slayer_caste" then
                            --skip
                        else
                            file:write("\t\t<"..detailK..">"..detailV.."</"..detailK..">\n")
                        end
                    end
                elseif df.history_event_hist_figure_diedst:is_instance(event) and k == "death_cause" then
                    file:write("\t\t<"..k..">"..df_enums.death_type[v]:lower().."</"..k..">\n")
                elseif df.history_event_change_hf_jobst:is_instance(event) and (k == "new_job" or k == "old_job") then
                    file:write("\t\t<"..k..">"..df_enums.profession[v]:lower().."</"..k..">\n")
                elseif df.history_event_change_creature_typest:is_instance(event) and (k == "old_race" or k == "new_race")  and v >= 0 then
                    file:write("\t\t<"..k..">"..escape_xml(dfhack.df2utf(df.global.world.raws.creatures.all[v].name[0])).."</"..k..">\n")
                elseif tostring(v):find ("<") then
                    if not problem_elements[tostring(event._type)] then
                        problem_elements[tostring(event._type)] = {}
                    end
                    if not problem_elements[tostring(event._type)][k] then
                        problem_elements [tostring(event._type)][k] = true
                    end
                    file:write("\t\t<"..k..">please report compound element for correction</"..k..">\n")
                else
                    local enum = event:_field (k)._type
                    if enum._kind == "enum-type" then
                        local val = enum [v]
                        if not val then
                            file:write("\t\t<"..k..">unknown "..tostring(v).."</"..k..">\n")
                        else
                            file:write("\t\t<"..k..">"..tostring(val):lower().."</"..k..">\n")
                        end
                    else
                        file:write("\t\t<"..k..">"..tostring(v).."</"..k..">\n")
                    end
                end
            end
            file:write("\t</historical_event>\n")
        end
    end
    end})

    table.insert(chunks, {name='historical_event_relationships', fn=function()
    for ID, set in progress_ipairs(df.global.world.history.relationship_events, 'relationship_event') do
        for k = 0, set.next_element - 1 do
            file:write("\t<historical_event_relationship>\n")
            file:write("\t\t<event>"..set.event[k].."</event>\n")
            file:write("\t\t<relationship>"..df_enums.vague_relationship_type[set.relationship[k]].."</relationship>\n")
            file:write("\t\t<source_hf>"..set.source_hf[k].."</source_hf>\n")
            file:write("\t\t<target_hf>"..set.target_hf[k].."</target_hf>\n")
            file:write("\t\t<year>"..set.year[k].."</year>\n")
            file:write("\t</historical_event_relationship>\n")
        end
    end
    end})

    table.insert(chunks, {name='historical_event_relationship_supplements', fn=function()
    for ID, event in progress_ipairs(df.global.world.history.relationship_event_supplements, 'relationship_event_supplement') do
        file:write("\t<historical_event_relationship_supplement>\n")
        file:write("\t\t<event>"..event.event.."</event>\n")
        file:write("\t\t<occasion_type>"..event.occasion_type.."</occasion_type>\n")
        file:write("\t\t<site>"..event.site.."</site>\n")
        file:write("\t\t<unk_1>"..event.unk_1.."</unk_1>\n")
        file:write("\t</historical_event_relationship_supplement>\n")
    end
    end})

    table.insert(chunks, {name='historical_event_collections', fn=function() end})

    table.insert(chunks, {name='historical_eras', fn=function() end})

    table.insert(chunks, {name='written_contents', fn=function()
    for wcK, wcV in progress_ipairs(df.global.world.written_contents.all) do
        file:write("\t<written_content>\n")
        file:write("\t\t<id>"..wcV.id.."</id>\n")
        file:write("\t\t<title>"..escape_xml(dfhack.df2utf(wcV.title)).."</title>\n")
        printifvalue(file, 2, "page_start", wcV.page_start)
        printifvalue(file, 2, "page_end", wcV.page_end)
        for refK, refV in pairs(wcV.refs) do
            file:write("\t\t<reference>\n")
            file:write("\t\t\t<type>"..df_enums.general_ref_type[refV:getType()].."</type>\n")
            if df.general_ref_artifact:is_instance(refV) then file:write("\t\t\t<id>"..refV.artifact_id.."</id>\n") -- artifact
            elseif df.general_ref_entity:is_instance(refV) then file:write("\t\t\t<id>"..refV.entity_id.."</id>\n") -- entity
            elseif df.general_ref_historical_eventst:is_instance(refV) then file:write("\t\t\t<id>"..refV.event_id.."</id>\n") -- event
            elseif df.general_ref_sitest:is_instance(refV) then file:write("\t\t\t<id>"..refV.site_id.."</id>\n") -- site
            elseif df.general_ref_subregionst:is_instance(refV) then file:write("\t\t\t<id>"..refV.region_id.."</id>\n") -- region
            elseif df.general_ref_historical_figurest:is_instance(refV) then file:write("\t\t\t<id>"..refV.hist_figure_id.."</id>\n") -- hist figure
            elseif df.general_ref_written_contentst:is_instance(refV) then file:write("\t\t\t<id>"..refV.written_content_id.."</id>\n")
            elseif df.general_ref_poetic_formst:is_instance(refV) then file:write("\t\t\t<id>"..refV.poetic_form_id.."</id>\n") -- poetic form
            elseif df.general_ref_musical_formst:is_instance(refV) then file:write("\t\t\t<id>"..refV.musical_form_id.."</id>\n") -- musical form
            elseif df.general_ref_dance_formst:is_instance(refV) then file:write("\t\t\t<id>"..refV.dance_form_id.."</id>\n") -- dance form
            elseif df.general_ref_interactionst:is_instance(refV) then -- TODO INTERACTION
            elseif df.general_ref_knowledge_scholar_flagst:is_instance(refV) then -- TODO KNOWLEDGE_SCHOLAR_FLAG
            elseif df.general_ref_value_levelst:is_instance(refV) then -- TODO VALUE_LEVEL
            elseif df.general_ref_languagest:is_instance(refV) then -- TODO LANGUAGE
            elseif df.general_ref_abstract_buildingst:is_instance(refV) then -- TODO ABSTRACT_BUILDING
            else
                print("unknown reference",refV:getType(),df_enums.general_ref_type[refV:getType()])
                --for k,v in pairs(refV) do print(k,v) end
            end
            file:write("\t\t</reference>\n")
        end
        file:write("\t\t<type>"..(df_enums.written_content_type[wcV.type] or wcV.type).."</type>\n")
        for styleK, styleV in pairs(wcV.styles) do
            file:write("\t\t<style>"..(df_enums.written_content_style[styleV] or styleV).."</style>\n")
        end
        file:write("\t\t<author>"..wcV.author.."</author>\n")
        file:write("\t</written_content>\n")
    end
    end})

    table.insert(chunks, {name='poetic_forms', fn=function()
    for formK, formV in progress_ipairs(df.global.world.poetic_forms.all, 'poetic form') do
        file:write("\t<poetic_form>\n")
        file:write("\t\t<id>"..formV.id.."</id>\n")
        file:write("\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(formV.name,1))).."</name>\n")
        file:write("\t</poetic_form>\n")
    end
    end})

    table.insert(chunks, {name='musical_forms', fn=function()
    for formK, formV in progress_ipairs(df.global.world.musical_forms.all, 'musical form') do
        file:write("\t<musical_form>\n")
        file:write("\t\t<id>"..formV.id.."</id>\n")
        file:write("\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(formV.name,1))).."</name>\n")
        file:write("\t</musical_form>\n")
    end
    end})

    table.insert(chunks, {name='dance_forms', fn=function()
    for formK, formV in progress_ipairs(df.global.world.dance_forms.all, 'dance form') do
        file:write("\t<dance_form>\n")
        file:write("\t\t<id>"..formV.id.."</id>\n")
        file:write("\t\t<name>"..escape_xml(dfhack.df2utf(dfhack.TranslateName(formV.name,1))).."</name>\n")
        file:write("\t</dance_form>\n")
    end
    end})

    step_size = math.max(1, 100 // #chunks)
    for k, chunk in ipairs(chunks) do
        progress_percent = math.max(progress_percent, (100 * k) // #chunks)
        step_percent = progress_percent
        write_chunk(chunk.name, chunk.fn)
    end

    file:write("</df_world>\n")
    file:close()

    local problem_elements_exist = false
    for i, element in pairs(problem_elements) do
        for k, field in pairs(element) do
          dfhack.printerr(i.." element '"..k.."' attempted to be processed as simple type.")
        end
        problem_elements_exist = true
    end
    if problem_elements_exist then
        dfhack.printerr("Some elements could not be interpreted correctly because they were not simple elements.")
        dfhack.printerr("These elements are reported above. Please notify the DFHack community of these value pairs.")
        dfhack.printerr("Note that these issues have not invalidated the XML file: it ought to still be usable.")
    end

    print("Done exporting extended legends data to: " .. filename)
end

local function wrap_export()
    if progress_percent >= 0 then
        qerror('exportlegends already in progress')
    end
    progress_percent = 0
    step_size = 1
    progress_item = 'basic info'
    yield_if_timeout()
    local ok, err = pcall(export_more_legends_xml)
    if not ok then
        dfhack.printerr(err)
    end
    progress_percent = -1
    step_size = 1
    progress_item = ''
end

-- -------------------
-- LegendsOverlay
--

LegendsOverlay = defclass(LegendsOverlay, overlay.OverlayWidget)
LegendsOverlay.ATTRS{
    default_pos={x=2, y=2},
    default_enabled=true,
    viewscreens='legends/Default',
    frame={w=55, h=5},
}

function LegendsOverlay:init()
    self:addviews{
        widgets.Panel{
            view_id='button_mask',
            frame={t=0, l=0, w=15, h=3},
        },
        widgets.BannerPanel{
            frame={b=0, l=0, r=0, h=1},
            subviews={
                widgets.ToggleHotkeyLabel{
                    view_id='do_export',
                    frame={t=0, l=1, w=53},
                    label='Also export DFHack extended legends data:',
                    key='CUSTOM_CTRL_D',
                    visible=function() return progress_percent < 0 end,
                },
                widgets.Label{
                    frame={t=0, l=1},
                    text={
                        'Exporting ',
                        {text=function() return progress_item end},
                        ' (',
                        {text=function() return progress_percent end, pen=COLOR_YELLOW},
                        '% complete)'
                    },
                    visible=function() return progress_percent >= 0 end,
                },
            },
        },
    }
end

function LegendsOverlay:onInput(keys)
    if keys._MOUSE_L_DOWN and progress_percent < 0 and
        self.subviews.button_mask:getMousePos() and
        self.subviews.do_export:getOptionValue()
    then
        script.start(wrap_export)
    end
    return LegendsOverlay.super.onInput(self, keys)
end

-- -------------------
-- DoneMaskOverlay
--

DoneMaskOverlay = defclass(DoneMaskOverlay, overlay.OverlayWidget)
DoneMaskOverlay.ATTRS{
    default_pos={x=-2, y=2},
    default_enabled=true,
    viewscreens='legends',
    frame={w=9, h=3},
}

function DoneMaskOverlay:init()
    self:addviews{
        widgets.Panel{
            frame_background=gui.CLEAR_PEN,
            visible=function() return progress_percent >= 0 end,
        }
    }
end

function DoneMaskOverlay:onInput(keys)
    if progress_percent >= 0 then
        if keys.LEAVESCREEN or (keys._MOUSE_L_DOWN and self:getMousePos()) then
            return true
        end
    end
    return DoneMaskOverlay.super.onInput(self, keys)
end

OVERLAY_WIDGETS = {
    export=LegendsOverlay,
    mask=DoneMaskOverlay,
}

if dfhack_flags.module then
    return
end

-- Check if on legends screen and trigger the export if so
if not dfhack.gui.matchFocusString('legends') then
    qerror('exportlegends must be run from the main legends view')
end

script.start(wrap_export)
