
local gui = require 'gui'
local dlg = require 'gui.dialogs'
local args={...}
local is_cheat=(#args>0 and args[1]=="-c")
local cursor=xyz2pos(df.global.cursor.x,df.global.cursor.y,df.global.cursor.z)
local permited_equips={}

permited_equips[df.item_backpackst]="UPPERBODY"
permited_equips[df.item_quiverst]="UPPERBODY"
permited_equips[df.item_flaskst]="UPPERBODY"
permited_equips[df.item_armorst]="UPPERBODY"
permited_equips[df.item_shoesst]="STANCE"
permited_equips[df.item_glovesst]="GRASP"
permited_equips[df.item_helmst]="HEAD"
permited_equips[df.item_pantsst]="LOWERBODY"
function DoesHaveSubtype(item)
    if df.item_backpackst:is_instance(item) or df.item_flaskst:is_instance(item) or df.item_quiverst:is_instance(item) then
        return false
    end
    return true
end
function CheckCursor(p)
    if p.x==-30000 then
        dlg.showMessage(
                'Companion orders',
                'You must have a cursor on some tile!', COLOR_LIGHTRED
            )
        return false
    end
    return true
end
function getxyz() -- this will return pointers x,y and z coordinates.
	local x=df.global.cursor.x
	local y=df.global.cursor.y
	local z=df.global.cursor.z	
	return x,y,z -- return the coords
end

function GetCaste(race_id,caste_id)
    local race=df.creature_raw.find(race_id)
    return race.caste[caste_id]
end

function EnumBodyEquipable(race_id,caste_id)
    local caste=GetCaste(race_id,caste_id)
    local bps=caste.body_info.body_parts
    local ret={}
    for k,v in pairs(bps) do
        ret[k]={bp=v,layers={[0]={size=0,permit=0},[1]={size=0,permit=0},[2]={size=0,permit=0},[3]={size=0,permit=0} } }
    end
    return ret
end
function ReadCurrentEquiped(body_equip,unit)
    for k,v in pairs(unit.inventory) do
        if v.mode==2 then
            local bpid=v.body_part_id
            if DoesHaveSubtype(v.item) then
                local sb=v.item.subtype.props
                local trg=body_equip[bpid]
                local trg_layer=trg.layers[sb.layer]
                
                if trg_layer.permit==0 then
                    trg_layer.permit=sb.layer_permit
                else
                    if trg_layer.permit>sb.layer_permit then
                        trg_layer.permit=sb.layer_permit
                    end
                end
                trg_layer.size=trg_layer.size+sb.layer_size
            end
        end
    end
end
function LayeringPermits(body_part,item)
    if not DoesHaveSubtype(item) then
        return true
    end
    local sb=item.subtype.props
    local trg_layer=body_part.layers[sb.layer]
    if math.min(trg_layer.permit ,sb.layer_permit)<trg_layer.size+sb.layer_size then
        return true
    end
    return false
end
function AddLayering(body_part,item)
    if not DoesHaveSubtype(item) then
        return
    end
    local sb=item.subtype.props
    local trg_layer=body_part.layers[sb.layer]
    trg_layer.permit=math.min(trg_layer.permit,sb.layer_permit)
    trg_layer.size=trg_layer.size+sb.layer_size
end
function AddIfFits(body_equip,unit,item)
    --TODO shaped items
    
    local need_flag
    for k,v in pairs(permited_equips) do
        if k:is_instance(item) then
            need_flag=v
            break
        end
    end
    if need_flag==nil then
        return false
    end

   
    for k,bp in pairs(body_equip) do
        local handedness_ok=true
        if df.item_glovesst:is_instance(item) then
            if bp.bp.flags["LEFT"]~=item.handedness[1] then
                handedness_ok=false
            end
        end
        if bp.bp.flags[need_flag] and LayeringPermits(bp,item) and handedness_ok then
            if dfhack.items.moveToInventory(item,unit,2,k) then
                AddLayering(bp,item)
                return true
            end
        end
    end
    return false
end
function EnumGrasps(race_id,caste_id)
    local caste=GetCaste(race_id,caste_id)
    local bps=caste.body_info.body_parts
    local ret={}
    for k,v in pairs(bps) do
        if v.flags.GRASP then
            --table.insert(ret,{k,v})
            ret[k]={v}
        end
    end
    return ret
end
function EnumEmptyGrasps(unit)
    local grasps=EnumGrasps(unit.race,unit.caste)
    local ret={}
    for k,v in pairs(unit.inventory) do
        if grasps[v.body_part_id] and v.mode==1 then
            grasps[v.body_part_id][2]=true
        end
    end
    for k,v in pairs(grasps) do
        if not v[2] then
            table.insert(ret,k)
        end
    end
    return ret
end

function GetBackpack(unit)
    for k,v in pairs(unit.inventory) do
       
        if df.item_backpackst:is_instance(v.item) then
            return v.item
        end
    end
end
function AddBackpackItems(backpack,items)
    if backpack then
        local bitems=dfhack.items.getContainedItems(backpack)
        for k,v in pairs(bitems) do
            table.insert(items,v)
        end
    end
end
function GetItemsAtPos(pos)
    local ret={}
    for k,v in pairs(df.global.world.items.all) do
        if v.flags.on_ground and v.pos.x==pos.x and v.pos.y==pos.y and v.pos.z==pos.z then
            table.insert(ret,v)
        end
    end
    return ret
end
function isAnyOfEquipable(item)
    for k,v in pairs(permited_equips) do
        if k:is_instance(item) then
            return true
        end
    end
    return false
end
function FilterByEquipable(items)
    local ret={}
    for k,v in pairs(items) do
        if isAnyOfEquipable(v) then
            table.insert(ret,v)
        end
    end
    return ret
end
function FilterBySize(items,race_id) --TODO add logic for compatible races
    local ret={}
    for k,v in pairs(items) do
        if v.maker_race==race_id then
            table.insert(ret,v)
        end
    end
    return ret
end
--local companions=??
local orders={
{name="move",f=function (unit_list,pos)
    if not CheckCursor(pos) then
        return false
    end
    for k,v in pairs(unit_list) do
        v.path.dest:assign(pos)
    end

    return true
end},
{name="equip",f=function (unit_list)
    --search in inventory(hands/backpack/ground) and equip everything
    --lot's of stuff to think: layering, which item goes where, which goes first, which body parts are missing, body sizes etc...
    --dfhack.items.moveToInventory(item,unit,use_mode,body_part)
    for k,unit in pairs(unit_list) do
        local items=GetItemsAtPos(unit.pos)
        --todo make a table join function or sth... submit it to the lua list!
        AddBackpackItems(GetBackpack(unit),items)
        items=FilterByEquipable(items)
        FilterBySize(items,unit.race) 
        local body_parts=EnumBodyEquipable(unit.race,unit.caste)
        ReadCurrentEquiped(body_parts,unit)
        for it_num,item in pairs(items) do
            AddIfFits(body_parts,unit,item)
        end
    end
end},
{name="pick-up",f=function (unit_list)
    --pick everything up (first into hands (if empty) then backpack (if have and has space?))
    for k,v in pairs(unit_list) do
        local items=GetItemsAtPos(v.pos)
        local grasps=EnumEmptyGrasps(v)
        -- TODO sort with weapon/shield on top of list!
        --add to grasps, then add to backpack (sanely? i.e. weapons/shields into hands then stuff)
        --or add to backpack if have, only then check grasps (faster equiping)
        while #grasps >0 and #items>0 do 
            if(dfhack.items.moveToInventory(items[#items],v,1,grasps[#grasps])) then
                table.remove(grasps)
            end
            table.remove(items)
        end
        local backpack=GetBackpack(v)
        if backpack then
            while #items>0 do
                dfhack.items.moveToContainer(items[#items],backpack)
                table.remove(items)
            end
        end
    end
    return true
end},
{name="unequip",f=function (unit_list)
    --remove and drop all the stuff (todo maybe a gui too?)
    for k,v in pairs(unit_list) do
        while #v.inventory ~=0 do
            dfhack.items.moveToGround(v.inventory[0].item,v.pos)
        end
    end
    return true
end},
{name="unwield",f=function (unit_list)
    
    for k,v in pairs(unit_list) do
        local wep_count=0
        for _,it in pairs(v.inventory) do
            if it.mode==1 then
                wep_count=wep_count+1
            end
        end
        for i=1,wep_count do
            for _,it in pairs(v.inventory) do
                if it.mode==1 then
                    dfhack.items.moveToGround(it.item,v.pos)
                    break
                end
            end    
        end
    end
    return true
end},
--[=[
{name="roam not working :<",f=function (unit_list,pos,dist) --does not work
    if not CheckCursor(pos) then
        return false
    end
    dist=dist or 5
    for k,v in pairs(unit_list) do
        v.idle_area:assign(pos)
        v.idle_area_threshold=dist
    end
    return true
end},
--]=]
{name="wait",f=function (unit_list)
    for k,v in pairs(unit_list) do
        v.relations.group_leader_id=-1
    end
    return true
end},
{name="follow",f=function (unit_list)
    local adv=df.global.world.units.active[0]
    for k,v in pairs(unit_list) do
        v.relations.group_leader_id=adv.id
    end
    return true
end},
{name="leave",f=function (unit_list)
    local adv=df.global.world.units.active[0]
    local t_nem=dfhack.units.getNemesis(adv)
    for k,v in pairs(unit_list) do
        
        v.relations.group_leader_id=-1
        local u_nem=dfhack.units.getNemesis(v)
        if u_nem then
            u_nem.group_leader_id=-1
        end
        if t_nem and u_nem then
            for k,v in pairs(t_nem.companions) do
                if v==u_nem.id then
                    t_nem.companions:erase(k)
                    break
                end
            end
        end
    end
    return true
end},

}
local cheats={
{name="Patch up",f=function (unit_list)
    local dft=require("plugins.dfusion.tools")
    for k,v in pairs(unit_list) do 
        dft.healunit(v)
 	end
	return true
end},
{name="Power up",f=function (unit_list)
    local dft=require("plugins.dfusion.tools")
    for k,d in pairs(unit_list) do 
        dft.powerup(d)
    end
    return true
end},
{name="get in",f=function (unit_list,pos)
    if not CheckCursor(pos) then
        return false
    end
	adv=df.global.world.units.active[0]
	item=getItemsAtPos(getxyz())[1]
	print(item.id)
    for k,v in pairs(unit_list) do
        v.riding_item_id=item.id
        local ref=df.general_ref_unit_riderst:new()
        ref.unit_id=v.id
        item.general_refs:insert("#",ref)
	end
	return true
end},
}
--[[ todo: add cheats...]]--
function getCompanions(unit)
    unit=unit or df.global.world.units.active[0]
    local t_nem=dfhack.units.getNemesis(unit)
    if t_nem==nil then
        qerror("Invalid unit! No nemesis record")
    end
    local ret={}
    for k,v in pairs(t_nem.companions) do
        local u=df.nemesis_record.find(v)
        if u.unit then
            table.insert(ret,u.unit)
        end
    end
    return ret
end


CompanionUi=defclass(CompanionUi,gui.FramedScreen)
CompanionUi.ATTRS{
    frame_title = "Companions",
}
function CompanionUi:init(args)
    self.unit_list=args.unit_list
    self.selected={}
    for i=0,26 do
        self.selected[i]=true
    end
end
function CompanionUi:GetSelectedUnits()
    local ret={}
    for k,v in pairs(self.unit_list) do
        if self.selected[k] then
            table.insert(ret,v)
        end
    end
    return ret
end
function CompanionUi:onInput(keys)

   
    if keys.LEAVESCREEN then
        self:dismiss()
    elseif keys._STRING then
        local s=keys._STRING
        if s==string.byte('*') then
            local v=self.selected[1] or false
            for i=0,26 do
                
                self.selected[i]=not v
            end
        end
        if s>=string.byte('a') and s<=string.byte('z') then
            local idx=s-string.byte('a')+1
            if self.selected[idx] then 
                self.selected[idx]=false
            else
                self.selected[idx]=true
            end
        end
        if s>=string.byte('A') and s<=string.byte('Z') then
            local idx=s-string.byte('A')+1
            if orders[idx] and orders[idx].f then
                if orders[idx].f(self:GetSelectedUnits(),cursor) then
                    self:dismiss()
                end
            end
            if is_cheat then
                idx=idx-#orders
                if cheats[idx] and cheats[idx].f then
                    if cheats[idx].f(self:GetSelectedUnits(),cursor) then
                        self:dismiss()
                    end
                end
            end
        end
    end
end
function CompanionUi:onRenderBody( dc)
    --list widget goes here...
    local char_a=string.byte('a')-1
    dc:newline(1):string("*. All")
    for k,v in ipairs(self.unit_list) do
        if self.selected[k] then
            dc:pen(COLOR_GREEN)
        else
            dc:pen(COLOR_GREY)
        end
        dc:newline(1):string(string.char(k+char_a)..". "):string(dfhack.TranslateName(v.name));
    end
    dc:pen(COLOR_GREY)
    local w,h=self:getWindowSize()
    local char_A=string.byte('A')-1
    for k,v in ipairs(orders) do
        dc:seek(w/2,k):string(string.char(k+char_A)..". "):string(v.name);
    end
    if is_cheat then
        for k,v in ipairs(cheats) do
            dc:seek(w/2,k+#orders):string(string.char(k+#orders+char_A)..". "):string(v.name);
        end
    end
end
local screen=CompanionUi{unit_list=getCompanions()}
screen:show()