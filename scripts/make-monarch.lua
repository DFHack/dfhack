--set target unit as king/queen
local unit=dfhack.gui.getSelectedUnit()
if not unit then qerror("No unit selected") end
local newfig=dfhack.units.getNemesis(unit).figure
local my_entity=df.historical_entity.find(df.global.ui.civ_id)
local monarch_id
for k,v in pairs(my_entity.positions.own) do
    if v.code=="MONARCH" then
        monarch_id=v.id
        break
    end
end
if not monarch_id then qerror("No monarch found!") end
local old_id
for pos_id,v in pairs(my_entity.positions.assignments) do
    if v.position_id==monarch_id then
        old_id=v.histfig
        v.histfig=newfig.id
        local oldfig=df.historical_figure.find(old_id)
        
        for k,v in pairs(oldfig.entity_links) do
            if df.histfig_entity_link_positionst:is_instance(v) and v.assignment_id==pos_id and v.entity_id==df.global.ui.civ_id then
                oldfig.entity_links:erase(k)
                break
            end
        end
        newfig.entity_links:insert("#",{new=df.histfig_entity_link_positionst,entity_id=df.global.ui.civ_id,
            link_strength=100,assignment_id=pos_id,start_year=df.global.cur_year})
        break
    end
end
