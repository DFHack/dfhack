--set target unit as king/queen
--[====[

make-monarch
============
Make the selected unit King or Queen of your civilisation.

]====]

local unit=dfhack.gui.getSelectedUnit()
if not unit then qerror("No unit selected") end
local newfig=dfhack.units.getNemesis(unit).figure
local my_entity=df.historical_entity.find(df.global.plotinfo.civ_id)
local monarch_id
for k,v in pairs(my_entity.positions.own) do
    if v.code=="MONARCH" then
        monarch_id=v.id
        break
    end
end
if not monarch_id then qerror("No monarch found!") end
local old_id
for assignment_idx,assignment in ipairs(my_entity.positions.assignments) do
    if assignment.position_id==monarch_id then
        old_id=assignment.histfig
        assignment.histfig=newfig.id
        local oldfig=df.historical_figure.find(old_id)

        for k,v in pairs(oldfig.entity_links) do
            if df.histfig_entity_link_positionst:is_instance(v) and v.assignment_id==assignment.id and v.entity_id==df.global.plotinfo.civ_id then --hint:df.histfig_entity_link_positionst
                oldfig.entity_links:erase(k)
                break
            end
        end
        newfig.entity_links:insert("#",{new=df.histfig_entity_link_positionst,entity_id=df.global.plotinfo.civ_id,
            link_strength=100,assignment_id=assignment.id,assignment_vector_idx=assignment_idx,start_year=df.global.cur_year})
        break
    end
end
