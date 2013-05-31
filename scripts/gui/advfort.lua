-- allows to do jobs in adv. mode.
keybinds={
nextJob={key="CUSTOM_SHIFT_T",desc="Next job in the list"},
prevJob={key="CUSTOM_SHIFT_R",desc="Previous job in the list"},
continue={key="A_WAIT",desc="Continue job if available"},
down_alt1={key="CUSTOM_CTRL_D",desc="Use job down"},
down_alt2={key="CURSOR_DOWN_Z_AUX",desc="Use job down"},
up_alt1={key="CUSTOM_CTRL_E",desc="Use job up"}, 
up_alt2={key="CURSOR_UP_Z_AUX",desc="Use job up"},
use_same={key="A_MOVE_SAME_SQUARE",desc="Use job at the tile you are standing"},
workshop={key="CHANGETAB",desc="Show building menu"},
}


local gui = require 'gui'
local wid=require 'gui.widgets'
local dialog=require 'gui.dialogs'
local buildings=require 'dfhack.buildings'
local bdialog=require 'gui.buildings'
local workshopJobs=require 'dfhack.workshops'
local utils=require 'utils'

local tile_attrs = df.tiletype.attrs

settings={build_by_items=false,check_inv=false,df_assign=true}



local mode_name
for k,v in ipairs({...}) do --setting parsing
    if v=="-c" or v=="--cheat" then
        settings.build_by_items=true
        settings.df_assign=false
    elseif v=="-i" or v=="--inventory" then
        settings.check_inv=true
        settings.df_assign=false
    elseif v=="-a" or v=="--nodfassign" then
        settings.df_assign=false
    else
        mode_name=v
       
    end
end

mode=mode or 0
last_building=last_building or {}

function Disclaimer(tlb)
    local dsc={"The Gathering Against ",{text="Goblin ",pen=dfhack.pen.parse{fg=COLOR_GREEN,bg=0}}, "Oppresion ",
        "(TGAGO) is not responsible for all ",NEWLINE,"the damage that this tool can (and will) cause to you and your loved worlds",NEWLINE,"and/or sanity.Please use with caution.",NEWLINE,{text="Magma not included.",pen=dfhack.pen.parse{fg=COLOR_LIGHTRED,bg=0}}}
    if tlb then
        for _,v in ipairs(dsc) do
            table.insert(tlb,v)
        end
    end
    return dsc
end
function showHelp()
    local helptext={
    "This tool allow you to perform jobs as a dwarf would in dwarf mode. When ",NEWLINE,
    "cursor is available you can press ",{key="SELECT", text="select",key_sep="()"},
    " to enqueue a job from",NEWLINE,"pointer location. If job is 'Build' and there is no planed construction",NEWLINE,
    "at cursor this tool show possible building choices.",NEWLINE,NEWLINE,{text="Keybindings:",pen=dfhack.pen.parse{fg=COLOR_CYAN,bg=0}},NEWLINE
    }
    for k,v in pairs(keybinds) do
        table.insert(helptext,{key=v.key,text=v.desc,key_sep=":"})
        table.insert(helptext,NEWLINE)
    end
    table.insert(helptext,{text="CAREFULL MOVE",pen=dfhack.pen.parse{fg=COLOR_LIGHTGREEN,bg=0}})
    table.insert(helptext,": use job in that direction")
    table.insert(helptext,NEWLINE)
    table.insert(helptext,NEWLINE)
    Disclaimer(helptext)
    dialog.showMessage("Help!?!",helptext)
end
--[[    Util functions ]]--
function advGlobalPos()
    local map=df.global.world.map
    local wd=df.global.world.world_data
    local adv=df.global.world.units.active[0]
    --wd.adv_region_x*16+wd.adv_emb_x,wd.adv_region_y*16+wd.adv_emb_y
    --return wd.adv_region_x*16+wd.adv_emb_x,wd.adv_region_y*16+wd.adv_emb_y
    --return wd.adv_region_x*16+wd.adv_emb_x+adv.pos.x/16,wd.adv_region_y*16+wd.adv_emb_y+adv.pos.y/16
    --print(map.region_x,map.region_y,adv.pos.x,adv.pos.y)
    --print(map.region_x+adv.pos.x/48, map.region_y+adv.pos.y/48,wd.adv_region_x*16+wd.adv_emb_x,wd.adv_region_y*16+wd.adv_emb_y)
    return math.floor(map.region_x+adv.pos.x/48), math.floor(map.region_y+adv.pos.y/48)
end
function inSite()
    local tx,ty=advGlobalPos()
    --print(tx,ty)
    
    for k,v in pairs(df.global.world.world_data.sites) do
        local tp={v.pos.x,v.pos.y}
        if tx>=tp[1]*16+v.rgn_min_x and tx<=tp[1]*16+v.rgn_max_x and
            ty>=tp[2]*16+v.rgn_min_y and ty<=tp[2]*16+v.rgn_max_y then
            --print(k)
            return v
        end
    end
end
--[[    low level job management    ]]--
function getLastJobLink()
    local st=df.global.world.job_list
    while st.next~=nil do
        st=st.next
    end
    return st
end
function addNewJob(job)
    local lastLink=getLastJobLink()
    local newLink=df.job_list_link:new()
    newLink.prev=lastLink
    lastLink.next=newLink
    newLink.item=job
    job.list_link=newLink
end
function makeJob(args)
    local newJob=df.job:new()
    newJob.id=df.global.job_next_id
    df.global.job_next_id=df.global.job_next_id+1
    --newJob.flags.special=true
    newJob.job_type=args.job_type
    newJob.completion_timer=-1

    newJob.pos:assign(args.pos)
    args.job=newJob
    local failed
    for k,v in ipairs(args.pre_actions or {}) do
        local ok,msg=v(args)
        if not ok then
            failed=msg
            break
        end
    end
    if failed==nil then
        AssignUnitToJob(newJob,args.unit,args.from_pos)
        for k,v in ipairs(args.post_actions or {}) do
            local ok,msg=v(args)
            if not ok then
                failed=msg
                break
            end
        end
        if failed then
            UnassignJob(newJob,args.unit)
        end
    end
    if failed==nil then
        addNewJob(newJob)
        return newJob
    else
        newJob:delete()
        return false,failed
    end
    
end

function UnassignJob(job,unit,unit_pos)
    unit.job.current_job=nil
end
function AssignUnitToJob(job,unit,unit_pos)
    job.general_refs:insert("#",{new=df.general_ref_unit_workerst,unit_id=unit.id})
    unit.job.current_job=job
    unit_pos=unit_pos or {x=job.pos.x,y=job.pos.y,z=job.pos.z}
    unit.path.dest:assign(unit_pos)
    return true
end
function SetCreatureRef(args)
    local job=args.job
    local pos=args.pos
    for k,v in pairs(df.global.world.units.active) do
        if v.pos.x==pos.x and v.pos.y==pos.y and v.pos.z==pos.z then
            job.general_refs:insert("#",{new=df.general_ref_unit_cageest,unit_id=v.id})
            return
        end
    end
end

function SetPatientRef(args)
    local job=args.job
    local pos=args.pos
    for k,v in pairs(df.global.world.units.active) do
        if v.pos.x==pos.x and v.pos.y==pos.y and v.pos.z==pos.z then
            job.general_refs:insert("#",{new=df.general_ref_unit_patientst,unit_id=v.id})
            return
        end
    end
end
function SetCarveDir(args)
    local job=args.job
    local pos=args.pos
    local from_pos=args.from_pos
    local dirs={up=18,down=19,right=20,left=21}
    if pos.x>from_pos.x then
        job.item_category[dirs.right]=true
    elseif pos.x<from_pos.x then
        job.item_category[dirs.left]=true
    elseif pos.y>from_pos.y then
        job.item_category[dirs.up]=true
    elseif pos.y<from_pos.y then
        job.item_category[dirs.down]=true
    end
end
function MakePredicateWieldsItem(item_skill)
    local pred=function(args)
        local inv=args.unit.inventory
        for k,v in pairs(inv) do
            if v.mode==1 and v.item:getMeleeSkill()==item_skill and args.unit.body.weapon_bp==v.body_part_id then
                return true
            end
        end
        return false,"Correct tool not equiped"
    end
    return pred
end

function makeset(args)
    local tbl={}
    for k,v in pairs(args) do
        tbl[v]=true
    end
    return tbl
end
function NotConstruct(args)
    local tt=dfhack.maps.getTileType(args.pos)
    if tile_attrs[tt].material~=df.tiletype_material.CONSTRUCTION and dfhack.buildings.findAtTile(args.pos)==nil then
        return true
    else
        return false, "Can only do it on non constructions"
    end
end
function NoConstructedBuilding(args)
    local bld=dfhack.buildings.findAtTile(args.pos)
    if bld and bld.construction_stage==3 then
        return false, "Can only do it on clear area or non-finished buildings"
    end
    return true
end
function IsBuilding(args)
    if dfhack.buildings.findAtTile(args.pos) then
        return true
    end
    return false, "Can only do it on buildings"
end
function IsConstruct(args)
    local tt=dfhack.maps.getTileType(args.pos)
    if tile_attrs[tt].material==df.tiletype_material.CONSTRUCTION then
        return true
    else
        return false, "Can only do it on constructions"
    end
end
function SameSquare(args)
    local pos1=args.pos
    local pos2=args.from_pos
    if pos1.x==pos2.x and pos1.y==pos2.y and pos1.z==pos2.z then
       return true
    else
        return false, "Can only do it on same square"
    end
end
function IsHardMaterial(args)
    local tt=dfhack.maps.getTileType(args.pos)
    local mat=tile_attrs[tt].material
    local hard_materials=makeset{df.tiletype_material.STONE,df.tiletype_material.FEATURE,
        df.tiletype_material.LAVA_STONE,df.tiletype_material.MINERAL,df.tiletype_material.FROZEN_LIQUID,}
    if hard_materials[mat] then
        return true
    else
        return false, "Can only do it on hard materials"
    end
end
function IsStairs(args)
    local tt=dfhack.maps.getTileType(args.pos)
    local shape=tile_attrs[tt].shape
    if shape==df.tiletype_shape.STAIR_UP or shape==df.tiletype_shape.STAIR_DOWN or shape==df.tiletype_shape.STAIR_UPDOWN or shape==df.tiletype_shape.RAMP then
        return true
    else
        return false,"Can only do it on stairs/ramps"
    end
end
function IsFloor(args)
    local tt=dfhack.maps.getTileType(args.pos)
    local shape=tile_attrs[tt].shape
    if shape==df.tiletype_shape.FLOOR or shape==df.tiletype_shape.BOULDER or shape==df.tiletype_shape.PEBBLES then
        return true
    else
        return false,"Can only do it on floors"
    end
end
function IsWall(args)
    local tt=dfhack.maps.getTileType(args.pos)
    if tile_attrs[tt].shape==df.tiletype_shape.WALL then
        return true
    else
        return false, "Can only do it on walls"
    end
end
function IsTree(args)
    local tt=dfhack.maps.getTileType(args.pos)
    if tile_attrs[tt].shape==df.tiletype_shape.TREE then
        return true
    else
        return false, "Can only do it on trees"
    end
end
function IsPlant(args)
    local tt=dfhack.maps.getTileType(args.pos)
    if tile_attrs[tt].shape==df.tiletype_shape.SHRUB then
        return true
    else
        return false, "Can only do it on plants"
    end
end
function IsWater(args)
    return true
end

function IsUnit(args)
    local pos=args.pos
    for k,v in pairs(df.global.world.units.active) do
        if v.pos.x==pos.x and v.pos.y==pos.y and v.pos.z==pos.z then
            return true
        end
    end
    return false,"Unit must be present"
end
function itemsAtPos(pos,tbl)
    local ret=tbl or {}
    for k,v in pairs(df.global.world.items.all) do
        if v.pos.x==pos.x and v.pos.y==pos.y and v.pos.z==pos.z and v.flags.on_ground then
            table.insert(ret,v)
        end
    end
    return ret
end
function AssignBuildingRef(args)
    local bld=dfhack.buildings.findAtTile(args.pos)
    args.job.general_refs:insert("#",{new=df.general_ref_building_holderst,building_id=bld.id})
    bld.jobs:insert("#",args.job)
    return true
end
function chooseBuildingWidthHeightDir(args) --TODO nicer selection dialog
    local btype=df.building_type
    local area=makeset{"w","h"}
    local all=makeset{"w","h","d"}
    local needs={[btype.FarmPlot]=area,[btype.Bridge]=all,
        [btype.RoadDirt]=area,[btype.RoadPaved]=area,[btype.ScrewPump]=makeset{"d"},
        [btype.AxleHorizontal]=makeset{"w","h"},[btype.WaterWheel]=makeset{"d"},[btype.Rollers]=makeset{"d"}}
    local myneeds=needs[args.type]
    if myneeds==nil then return end
    if args.width==nil and myneeds.w then
        --args.width=3
        dialog.showInputPrompt("Building size:", "Input building width:", nil, "1", 
            function(txt) args.width=tonumber(txt);BuildingChosen(args) end)
        return true
    end
    if args.height==nil and myneeds.h then
        --args.height=4
        dialog.showInputPrompt("Building size:", "Input building height:", nil, "1", 
            function(txt) args.height=tonumber(txt);BuildingChosen(args) end)
        return true
    end
    if args.direction==nil and myneeds.d then
        --args.direction=0--?
        dialog.showInputPrompt("Building size:", "Input building direction:", nil, "0", 
            function(txt) args.direction=tonumber(txt);BuildingChosen(args) end)
        return true
    end
    return false
    --width = ..., height = ..., direction = ...
end

function BuildingChosen(inp_args,type_id,subtype_id,custom_id)
    local args=inp_args or {}
    
    args.type=type_id or args.type
    args.subtype=subtype_id or args.subtype
    args.custom=custom_id or args.custom_id
    if inp_args then
        args.pos=inp_args.pos or args.pos
    end
    last_building.type=args.type
    last_building.subtype=args.subtype
    last_building.custom=args.custom
    
    if chooseBuildingWidthHeightDir(args) then
        
        return
    end
    --if settings.build_by_items then
    --    args.items=itemsAtPos(inp_args.from_pos)
    --end
    buildings.constructBuilding(args)
end


function RemoveBuilding(args)
    local bld=dfhack.buildings.findAtTile(args.pos)
    if bld~=nil then
        bld:queueDestroy()
        for k,v in ipairs(bld.jobs) do
            if v.job_type==df.job_type.DestroyBuilding then
                AssignUnitToJob(v,args.unit,args.from_pos)
                return true
            end
        end
        return false,"Building removal job failed to be created"
    else
        return false,"No building to remove"
    end
end

function isSuitableItem(job_item,item)
    --todo butcher test
    if job_item.item_type~=-1 then
        if item:getType()~= job_item.item_type then
        
            return false, "type"
        elseif job_item.item_subtype~=-1 then
            if item:getSubtype()~=job_item.item_subtype then
                return false,"subtype"
            end
        end
    end
    
    if job_item.mat_type~=-1 then
        if item:getActualMaterial()~= job_item.mat_type then --unless we would want to make hist-fig specific reactions
            return false, "material"
        elseif job_item.mat_index~=-1 then
            if item:getActualMaterialIndex()~=job_item.mat_index then
                return false,"material index"
            end
        end
    end
    if job_item.flags1.sand_bearing and not item:isSandBearing() then
        return false,"not sand bearing"
    end
    if job_item.flags1.butcherable and not (item:getType()== df.item_type.CORPSE or item:getType()==df.item_type.CORPSEPIECE) then
        return false,"not butcherable"
    end
    local matinfo=dfhack.matinfo.decode(item)
    --print(matinfo:getCraftClass())
    --print("Matching ",item," vs ",job_item)
    if not matinfo:matches(job_item) then
        return false,"matinfo"
    end
    -- some bonus checks:
    if job_item.flags2.building_material and not item:isBuildMat() then
        return false,"non-build mat"
    end
    -- *****************
    --print("--Matched")
    --reagen_index?? reaction_id??
    if job_item.metal_ore~=-1 and not item:isMetalOre(job_item.metal_ore) then
        return false,"metal ore"
    end
    if job_item.min_dimension~=-1 then
    end
    if #job_item.contains~=0 then
    end
    if job_item.has_tool_use~=-1 then
        if not item:hasToolUse(job_item.has_tool_use) then
            return false,"tool use"
        end
    end
    if job_item.has_material_reaction_product~="" then
        local ok=false
        for k,v in pairs(matinfo.material.reaction_product.id) do
            if v.value==job_item.has_material_reaction_product then
                ok=true
                break
            end
        end
        if not ok then
            return false, "no material reaction product"
        end
    end
    if job_item.reaction_class~="" then
        local ok=false
        for k,v in pairs(matinfo.material.reaction_class) do
            if v.value==job_item.reaction_class then
                ok=true
                break
            end
        end
        if not ok then
            return false, "no material reaction class"
        end
    end
    return true
end
function getItemsUncollected(job)
    local ret={}
    for id,jitem in pairs(job.items) do
        local x,y,z=dfhack.items.getPosition(jitem.item)
        if x~=job.pos.x or y~=job.pos.y or z~=job.pos.z then
            table.insert(ret,jitem)
        end
    end
    return ret
end
function AddItem(tbl,item,recurse,skip_add)
    if not skip_add then
        table.insert(tbl,item)
    end
    if recurse then
        local subitems=dfhack.items.getContainedItems(item)
        if subitems~=nil then
            for k,v in pairs(subitems) do
                AddItem(tbl,v,recurse)
            end
        end
    end
end
function EnumItems(args)
    local ret=args.table or {}
    if args.all then
        for k,v in pairs(df.global.world.items.all) do
            if v.flags.on_ground then
                AddItem(ret,v,args.deep)
            end
        end
    elseif args.pos~=nil then
        for k,v in pairs(df.global.world.items.all) do
            if v.pos.x==args.pos.x and v.pos.y==args.pos.y and v.pos.z==args.pos.z and v.flags.on_ground then
                AddItem(ret,v,args.deep)
            end
        end
    end
    if args.unit~=nil then
        for k,v in pairs(args.unit.inventory) do
            if args.inv[v.mode] then
                AddItem(ret,v.item,args.deep)
            elseif args.deep then
                AddItem(ret,v.item,args.deep,true)
            end
        end
    end
    return ret
end
jobitemEditor=defclass(jobitemEditor,gui.FramedScreen)
jobitemEditor.ATTRS{
    frame_style = gui.GREY_LINE_FRAME,
    frame_inset = 1,
    allow_add=false,
    allow_remove=false,
    allow_any_item=false,
    job=DEFAULT_NIL,
    items=DEFAULT_NIL,
}
function jobitemEditor:init(args)
    --self.job=args.job
    if self.job==nil then qerror("This screen must have job target") end
    if self.items==nil then qerror("This screen must have item list") end
    local itemChoices={}
    table.insert(itemChoices,{text="<nothing>"})
    for k,v in pairs(self.items) do
        table.insert(itemChoices,{item=v,text=dfhack.items.getDescription(v, 0)})
    end
    self.itemChoices=itemChoices
    self:addviews{
            wid.Label{
                view_id = 'label',
                text = args.prompt,
                text_pen = args.text_pen,
                frame = { l = 0, t = 0 },
            },
            wid.List{
                view_id = 'itemList',
                frame = { l = 0, t = 2 ,b=2},
                on_submit =  self:callback("selectJobItem")
            },
            wid.Label{
                frame = { b=1,l=1},
                text ={{text= ": cancel",
                    key  = "LEAVESCREEN",
                    on_activate= self:callback("dismiss")
                    },
                    {
                    gap=3,
                    text= ": accept",
                    key  = "SEC_SELECT",
                    on_activate= self:callback("commit"),
                    enabled=self:callback("jobValid")
                    },
                     {
                    gap=3,
                    text= ": add",
                    key  = "CUSTOM_A",
                    enabled=false,
                    --on_activate= self:callback("commit")
                    },
                     {
                    gap=3,
                    text= ": remove",
                    key  = "CUSTOM_R",
                    enabled=false,
                    --on_activate= self:callback("commit")
                    },}
            },
    }
    self.assigned={}
    self:fill(self.job)
end

function jobitemEditor:fill(job)
    local choices={}
    for item_id, trg_job_item in ipairs(job.job_items) do
        local str="<nothing>"
        if self.assigned[item_id] ~=nil then
            str=dfhack.items.getDescription(self.assigned[item_id],0)
        end
        local text={string.format("%3d:",item_id)}
        if self.assigned[item_id]~=nil then
            for subid,assigned_item in ipairs(self.assigned[item_id]) do
                table.insert(text,"   "..dfhack.items.getDescription(assigned_item,0))
                table.insert(text,"\n")
            end
        else
            table.insert(text,"<nothing>")
        end
            table.insert(choices,{text=text,
            job_item=trg_job_item,job_item_idx=item_id})
    end
    self.choices=choices
    self.subviews.itemList:setChoices(choices)
end
function jobitemEditor:countMatched(job_item_idx,job_item)
    local sum=0
    if self.assigned[job_item_idx]==nil then return false end
    for k,v in pairs(self.assigned[job_item_idx]) do
        sum=sum+v:getTotalDimension()
    end
    return job_item.quantity<=sum
end
function jobitemEditor:jobValid()
    for k,v in pairs(self.job.job_items) do
        if not self:countMatched(k,v) then
            return false
        end
    end
    return true
end
function jobitemEditor:itemChosen(job_choice,index,choice)
    if choice.item==nil then
        self.assigned[job_choice.job_item_idx]=nil
    else
        self.assigned[job_choice.job_item_idx]=self.assigned[job_choice.job_item_idx] or {}
        table.insert(self.assigned[job_choice.job_item_idx],choice.item)
        if not self:countMatched(job_choice.job_item_idx,job_choice.job_item) then
            self:selectJobItem(nil,job_choice)
        end
    end
    self:fill(self.job)
end
function jobitemEditor:selectJobItem(index,choice)
    local filtered_items={}
    for k,v in pairs(self.itemChoices) do
        if (v.text=="<nothing>" or isSuitableItem(choice.job_item,v.item)) and (v.item==nil or not self:isAssigned(v.item)) then
            table.insert(filtered_items,v)
        end
    end
    dialog.showListPrompt("Item choice", "Choose item:", nil, filtered_items, self:callback("itemChosen",choice), nil,nil,true) --custom item choice dialog req
end
function jobitemEditor:isAssigned(item)
    for k,v in pairs(self.assigned) do
        for subid, aitem in pairs(v) do
            if aitem.id==item.id then
                return true
            end
        end
    end
end
function jobitemEditor:commit()
    
    for job_item_id,v in pairs(self.assigned) do
        for sub_id,cur_item in pairs(v) do
            self.job.items:insert("#",{new=true,item=cur_item,role=df.job_item_ref.T_role.Reagent,job_item_idx=job_item_id})
        end
    end
    local uncollected = getItemsUncollected(job)
    if #uncollected == 0 then
        self.job.flags.working=true
    else
        uncollected[1].is_fetching=1
        self.job.flags.fetching=true
    end
    self:dismiss()
end
function AssignJobItems(args)
    
    if settings.df_assign then --use df default logic and hope that it would work
        return true
    end
    -- first find items that you want to use for the job
    local job=args.job
    local its=EnumItems{pos=args.from_pos,unit=args.unit,
        inv={[df.unit_inventory_item.T_mode.Hauled]=settings.check_inv,[df.unit_inventory_item.T_mode.Worn]=settings.check_inv,
             [df.unit_inventory_item.T_mode.Weapon]=settings.check_inv,},deep=true}
    --[[ job item editor...
    jobitemEditor{job=args.job,items=its}:show()
    local ok=job.flags.working or job.flags.fetching
    if not ok then 
        return ok, "Stuff"
    else
        return ok
    end
    --]]
    -- [=[
    --[[while(#job.items>0) do --clear old job items
        job.items[#job.items-1]:delete()
        job.items:erase(#job.items-1)
    end]]
    local item_counts={}
    for job_id, trg_job_item in ipairs(job.job_items) do
        item_counts[job_id]=trg_job_item.quantity
    end
    local used_item_id={}
    for job_id, trg_job_item in ipairs(job.job_items) do
        for _,cur_item in pairs(its) do 
            if not used_item_id[cur_item.id] then
                
                local item_suitable,msg=isSuitableItem(trg_job_item,cur_item) 
                --if msg then
                --    print(cur_item,msg)
                --end
                
                if (item_counts[job_id]>0 and item_suitable) or settings.build_by_items then
                    --cur_item.flags.in_job=true
                    job.items:insert("#",{new=true,item=cur_item,role=df.job_item_ref.T_role.Reagent,job_item_idx=job_id})
                    item_counts[job_id]=item_counts[job_id]-cur_item:getTotalDimension()
                    --print(string.format("item added, job_item_id=%d, item %s, quantity left=%d",job_id,tostring(cur_item),item_counts[job_id]))
                    used_item_id[cur_item.id]=true
                end
            end
        end
    end
    
    if not settings.build_by_items then
        for job_id, trg_job_item in ipairs(job.job_items) do
            if item_counts[job_id]>0 then
                return false, "Not enough items for this job"
            end
        end
    end
    local uncollected = getItemsUncollected(job)
    if #uncollected == 0 then
        job.flags.working=true
    else
        job.flags.fetching=true
        uncollected[1].is_fetching=1
        
    end
    --todo set working for workshops if items are present, else set fetching (and at least one item to is_fetching=1)
    --job.flags.working=true
    return true
    --]=]
end
function CheckAndFinishBuilding(args,bld)
    for idx,job in pairs(bld.jobs) do
        if job.job_type==df.job_type.ConstructBuilding then
            args.job=job
            break
        end
    end
    
    if args.job~=nil then
        local ok,msg=AssignJobItems(args)
        if not ok then
            return false,msg
        else
            AssignUnitToJob(args.job,args.unit,args.from_pos) 
        end
    else
        local t={items=buildings.getFiltersByType({},bld:getType(),bld:getSubtype(),bld:getCustomType())}
        args.pre_actions={dfhack.curry(setFiltersUp,t),AssignJobItems,AssignBuildingRef}
        local ok,msg=makeJob(args)
        return ok,msg
    end
end
function AssignJobToBuild(args)
    local bld=dfhack.buildings.findAtTile(args.pos)
    args.job_type=df.job_type.ConstructBuilding
    if bld~=nil then
        CheckAndFinishBuilding(args,bld)
    else
        bdialog.BuildingDialog{on_select=dfhack.curry(BuildingChosen,args),hide_none=true}:show()
    end
    return true
end
function BuildLast(args)
    local bld=dfhack.buildings.findAtTile(args.pos)
    args.job_type=df.job_type.ConstructBuilding
    if bld~=nil then
        CheckAndFinishBuilding(args,bld)
    else
        --bdialog.BuildingDialog{on_select=dfhack.curry(BuildingChosen,args),hide_none=true}:show()
        if last_building and last_building.type then
            BuildingChosen(args,last_building.type,last_building.subtype,last_building.custom)
        end
    end
    return true
end
function CancelJob(unit)
    local c_job=unit.job.current_job 
    if c_job then
        unit.job.current_job =nil --todo add real cancelation   
        for k,v in pairs(c_job.general_refs) do
            if df.general_ref_unit_workerst:is_instance(v) then
                v:delete()
                c_job.general_refs:erase(k)
                return
            end
        end
    end
end
function ContinueJob(unit)
    local c_job=unit.job.current_job 
    if c_job then
        c_job.flags.suspend=false
        for k,v in pairs(c_job.items) do
            if v.is_fetching==1 then
                unit.path.dest:assign(v.item.pos)
                return
            end
        end
        unit.path.dest:assign(c_job.pos)
    end
end

actions={
    {"CarveFortification"   ,df.job_type.CarveFortification,{IsWall,IsHardMaterial}},
    {"DetailWall"           ,df.job_type.DetailWall,{MakePredicateWieldsItem(df.job_skill.MINING),IsWall,IsHardMaterial}},
    {"DetailFloor"          ,df.job_type.DetailFloor,{MakePredicateWieldsItem(df.job_skill.MINING),IsFloor,IsHardMaterial,SameSquare}},
    {"CarveTrack"           ,df.job_type.CarveTrack,{MakePredicateWieldsItem(df.job_skill.MINING),IsFloor,IsHardMaterial}
                            ,{SetCarveDir}}, 
    {"Dig"                  ,df.job_type.Dig,{MakePredicateWieldsItem(df.job_skill.MINING),IsWall}},
    {"CarveUpwardStaircase" ,df.job_type.CarveUpwardStaircase,{MakePredicateWieldsItem(df.job_skill.MINING),IsWall}},
    {"CarveDownwardStaircase",df.job_type.CarveDownwardStaircase,{MakePredicateWieldsItem(df.job_skill.MINING)}},
    {"CarveUpDownStaircase" ,df.job_type.CarveUpDownStaircase,{MakePredicateWieldsItem(df.job_skill.MINING)}},
    {"CarveRamp"            ,df.job_type.CarveRamp,{MakePredicateWieldsItem(df.job_skill.MINING),IsWall}},
    {"DigChannel"           ,df.job_type.DigChannel,{MakePredicateWieldsItem(df.job_skill.MINING)}},
    {"FellTree"             ,df.job_type.FellTree,{MakePredicateWieldsItem(df.job_skill.AXE),IsTree}},
    {"Fish"                 ,df.job_type.Fish,{IsWater}},
    --{"Diagnose Patient"     ,df.job_type.DiagnosePatient,{IsUnit},{SetPatientRef}},
    --{"Surgery"              ,df.job_type.Surgery,{IsUnit},{SetPatientRef}},
    {"TameAnimal"           ,df.job_type.TameAnimal,{IsUnit},{SetCreatureRef}}, 
    {"GatherPlants"         ,df.job_type.GatherPlants,{IsPlant,SameSquare}},
    {"RemoveConstruction"   ,df.job_type.RemoveConstruction,{IsConstruct}},
    {"RemoveBuilding"       ,RemoveBuilding,{IsBuilding}},
    {"RemoveStairs"         ,df.job_type.RemoveStairs,{IsStairs,NotConstruct}},
    --{"HandleLargeCreature"   ,df.job_type.HandleLargeCreature,{isUnit},{SetCreatureRef}},
    {"Build"                ,AssignJobToBuild,{NoConstructedBuilding}},
    {"BuildLast"                ,BuildLast,{NoConstructedBuilding}},
    {"Clean"                ,df.job_type.Clean,{}},
    
}

for id,action in pairs(actions) do
    if action[1]==mode_name then
        mode=id-1
        break
    end
end
usetool=defclass(usetool,gui.Screen)
usetool.focus_path = 'advfort'
function usetool:getModeName()
    local adv=df.global.world.units.active[0]
    if adv.job.current_job then
        return string.format("%s working(%d) ",(actions[(mode or 0)+1][1] or ""),adv.job.current_job.completion_timer)
    else
        return actions[(mode or 0)+1][1] or " "
    end
    
end

function usetool:init(args)
    self:addviews{
        wid.Label{
            view_id="mainLabel",
            frame = {xalign=0,yalign=0},
            text={{key=keybinds.prevJob.key},{gap=1,text=dfhack.curry(usetool.getModeName,self)},{gap=1,key=keybinds.nextJob.key},
                  }
                  },
            

        wid.Label{
            view_id="shopLabel",
            frame = {l=35,xalign=0,yalign=0},
            visible=false,
            text={
                {id="text1",gap=1,key=keybinds.workshop.key,key_sep="()", text="Workshop menu",pen=dfhack.pen.parse{fg=COLOR_YELLOW,bg=0}}}
                  },
            
        wid.Label{
            view_id="siteLabel",
            frame = {t=1,xalign=-1,yalign=0},
            visible=false,
            text={
                {id="text1", text="Site:"},{id="site", text="name"}
                  }
            }
            }
end

function usetool:onIdle()
    
    self._native.parent:logic()
end
MOVEMENT_KEYS = {
    A_CARE_MOVE_N = { 0, -1, 0 }, A_CARE_MOVE_S = { 0, 1, 0 },
    A_CARE_MOVE_W = { -1, 0, 0 }, A_CARE_MOVE_E = { 1, 0, 0 },
    A_CARE_MOVE_NW = { -1, -1, 0 }, A_CARE_MOVE_NE = { 1, -1, 0 },
    A_CARE_MOVE_SW = { -1, 1, 0 }, A_CARE_MOVE_SE = { 1, 1, 0 },
    --[[A_MOVE_N = { 0, -1, 0 }, A_MOVE_S = { 0, 1, 0 },
    A_MOVE_W = { -1, 0, 0 }, A_MOVE_E = { 1, 0, 0 },
    A_MOVE_NW = { -1, -1, 0 }, A_MOVE_NE = { 1, -1, 0 },
    A_MOVE_SW = { -1, 1, 0 }, A_MOVE_SE = { 1, 1, 0 },--]]
    A_CUSTOM_CTRL_D = { 0, 0, -1 },
    A_CUSTOM_CTRL_E = { 0, 0, 1 },
    CURSOR_UP_Z_AUX = { 0, 0, 1 }, CURSOR_DOWN_Z_AUX = { 0, 0, -1 },
    A_MOVE_SAME_SQUARE={0,0,0},
    SELECT={0,0,0},
}
ALLOWED_KEYS={
    A_MOVE_N=true,A_MOVE_S=true,A_MOVE_W=true,A_MOVE_E=true,A_MOVE_NW=true,
    A_MOVE_NE=true,A_MOVE_SW=true,A_MOVE_SE=true,A_STANCE=true,SELECT=true,A_MOVE_DOWN_AUX=true,
    A_MOVE_UP_AUX=true,A_LOOK=true,CURSOR_DOWN=true,CURSOR_UP=true,CURSOR_LEFT=true,CURSOR_RIGHT=true,
    CURSOR_UPLEFT=true,CURSOR_UPRIGHT=true,CURSOR_DOWNLEFT=true,CURSOR_DOWNRIGHT=true,A_CLEAR_ANNOUNCEMENTS=true,
    CURSOR_UP_Z=true,CURSOR_DOWN_Z=true,
}
function moddedpos(pos,delta)
    return {x=pos.x+delta[1],y=pos.y+delta[2],z=pos.z+delta[3]}
end
function usetool:onHelp()
    showHelp()
end
function setFiltersUp(specific,args)
    local job=args.job
    if specific.job_fields~=nil then
        job:assign(specific.job_fields)
    end
    --printall(specific)
    for _,v in ipairs(specific.items) do
        --printall(v)
        local filter=v
        filter.new=true
        job.job_items:insert("#",filter)
    end
    return true
end
function onWorkShopJobChosen(args,idx,choice)
    args.pos=args.from_pos
    args.job_type=choice.job_id
    args.post_actions={AssignBuildingRef}
    args.pre_actions={dfhack.curry(setFiltersUp,choice.filter),AssignJobItems}
    local job,msg=makeJob(args)
    if not job then
        dfhack.gui.showAnnouncement(msg,5,1)
    end
    args.job=job
    
    --[[for _,v in ipairs(choice.filter) do
        local filter=require("utils").clone(args.common)
        filter.new=true
        require("utils").assign(filter,v)
        --printall(filter)
        job.job_items:insert("#",filter)
    end--]]
    --local ok,msg=AssignJobItems(args)
    --print(ok,msg)
end
function siegeWeaponActionChosen(building,actionid)
    local args
    if actionid==1 then
        building.facing=(building.facing+1)%4
    elseif actionid==2 then
        local action=df.job_type.LoadBallista
        if building:getSubtype()==df.siegeengine_type.Catapult then
            action=df.job_type.LoadCatapult
        end
        args={}
        args.job_type=action
        args.unit=df.global.world.units.active[0]
        local from_pos={x=args.unit.pos.x,y=args.unit.pos.y, z=args.unit.pos.z}
        args.from_pos=from_pos
        args.pos=from_pos
        args.pre_actions={dfhack.curry(setFiltersUp,{items={{}}})}
        --issue a job...
    elseif actionid==3 then
        local action=df.job_type.FireBallista
        if building:getSubtype()==df.siegeengine_type.Catapult then
            action=df.job_type.FireCatapult
        end
        args={}
        args.job_type=action
        args.unit=df.global.world.units.active[0]
        local from_pos={x=args.unit.pos.x,y=args.unit.pos.y, z=args.unit.pos.z}
        args.from_pos=from_pos
        args.pos=from_pos
        --another job?
    end
    if args~=nil then
        args.post_actions={AssignBuildingRef}
        local ok,msg=makeJob(args)
        if not ok then
            dfhack.gui.showAnnouncement(msg,5,1)
        end
    end
end
function putItemToBuilding(building,item)
    if building:getType()==df.building_type.Table then
        dfhack.items.moveToBuilding(item,building,0)
    else
        local container=building.contained_items[0].item --todo maybe iterate over all, add if usemode==2?
        dfhack.items.moveToContainer(item,container)
    end
end
function usetool:openPutWindow(building)
    
    local adv=df.global.world.units.active[0]
    local items=EnumItems{pos=adv.pos,unit=adv,
        inv={[df.unit_inventory_item.T_mode.Hauled]=true,--[df.unit_inventory_item.T_mode.Worn]=true,
             [df.unit_inventory_item.T_mode.Weapon]=true,},deep=true}
    local choices={}
    for k,v in pairs(items) do
        table.insert(choices,{text=dfhack.items.getDescription(v,0),item=v})
    end
    dialog.showListPrompt("Item choice", "Choose item to put into:", COLOR_WHITE,choices,function (idx,choice) putItemToBuilding(building,choice.item) end)
end
function usetool:openSiegeWindow(building)
    dialog.showListPrompt("Engine job choice", "Choose what to do:",COLOR_WHITE,{"Turn","Load","Fire"},
        dfhack.curry(siegeWeaponActionChosen,building))
end
function usetool:onWorkShopButtonClicked(building,index,choice)
    local adv=df.global.world.units.active[0]
    if df.interface_button_building_new_jobst:is_instance(choice.button) then
        choice.button:click()
        if #building.jobs>0 then
            local job=building.jobs[#building.jobs-1]
            AssignUnitToJob(job,adv,adv.pos)
            AssignJobItems{job=job,from_pos=adv.pos,pos=adv.pos,unit=adv}
        end
    elseif df.interface_button_building_category_selectorst:is_instance(choice.button) or
        df.interface_button_building_material_selectorst:is_instance(choice.button) then
        choice.button:click()
        self:openShopWindowButtoned(building,true)
    end
end

function usetool:openShopWindowButtoned(building,no_reset)
    self:setupFields()
    local wui=df.global.ui_sidebar_menus.workshop_job
    if not no_reset then
        -- [[ manual reset incase the df-one does not exist?
        wui:assign{category_id=-1,mat_type=-1,mat_index=-1}
        for k,v in pairs(wui.material_category) do
            wui.material_category[k]=false
        end
        --]]
        --[[building:fillSidebarMenu()
        if #wui.choices_all>0 then
            wui.choices_all[#wui.choices_all-1]:click()
        end
        --]]
    end
    building:fillSidebarMenu()
    
    local list={}
    for id,choice in pairs(wui.choices_visible) do
        table.insert(list,{text=utils.call_with_string(choice,"getLabel"),button=choice})
    end
    if #list ==0 and not no_reset then
        print("Fallback")
        self:openShopWindow(building)
        return
        --qerror("No jobs for this workshop")
    end
    dialog.showListPrompt("Workshop job choice", "Choose what to make",COLOR_WHITE,list,self:callback("onWorkShopButtonClicked",building)
            ,nil, nil,true)
end
function usetool:openShopWindow(building)
    local adv=df.global.world.units.active[0]
    
    local filter_pile=workshopJobs.getJobs(building:getType(),building:getSubtype(),building:getCustomType())
    if filter_pile then
        local state={unit=adv,from_pos={x=adv.pos.x,y=adv.pos.y, z=adv.pos.z}
        ,screen=self,bld=building,common=filter_pile.common}
        choices={}
        for k,v in pairs(filter_pile) do
            table.insert(choices,{job_id=0,text=v.name:lower(),filter=v})
        end
        dialog.showListPrompt("Workshop job choice", "Choose what to make",COLOR_WHITE,choices,dfhack.curry(onWorkShopJobChosen,state)
            ,nil, nil,true)
    else
        qerror("No jobs for this workshop")
    end
end
function usetool:armCleanTrap(building)
    local adv=df.global.world.units.active[0]
    --[[
    Lever,
        PressurePlate,
        CageTrap,
        StoneFallTrap,
        WeaponTrap,
        TrackStop
    --]]
    if building.state==0 then
        --CleanTrap
        --[[        LoadCageTrap,
        LoadStoneTrap,
        LoadWeaponTrap,
        ]]
        if building.trap_type==df.trap_type.Lever then 
            --link
            return
        end
        --building.trap_type==df.trap_type.PressurePlate then
        --settings/link
        local args={unit=adv,post_actions={AssignBuildingRef,AssignJobItems},pos=adv.pos,from_pos=adv.pos,job_type=df.job_type.CleanTrap}
        if building.trap_type==df.trap_type.CageTrap then
            args.job_type=df.job_type.LoadCageTrap
            local job_filter={items={{quantity=1,item_type=df.item_type.CAGE}} }
            args.pre_actions={dfhack.curry(setFiltersUp,job_filter)}

        elseif building.trap_type==df.trap_type.StoneFallTrap then
            args.job_type=df.job_type.LoadStoneTrap
            local job_filter={items={{quantity=1,item_type=df.item_type.BOULDER}} }
            args.pre_actions={dfhack.curry(setFiltersUp,job_filter)}
        elseif building.trap_type==df.trap_type.WeaponTrap then
            qerror("TODO")
        else
            return
        end
        local job,msg=makeJob(args)
        if not job then
            print(msg)
        end
    end
end
function usetool:hiveActions(building)
    local adv=df.global.world.units.active[0]
    local args={unit=adv,post_actions={AssignBuildingRef,AssignJobItems},pos=adv.pos,from_pos=adv.pos,job_type=df.job_type.InstallColonyInHive}
    local job_filter={items={{quantity=1,item_type=df.item_type.VERMIN}} }
            args.pre_actions={dfhack.curry(setFiltersUp,job_filter)}
    local job,msg=makeJob(args)
    if not job then
        print(msg)
    end
    --InstallColonyInHive,
    --CollectHiveProducts,
end
function usetool:operatePump(building)
    
    local adv=df.global.world.units.active[0]
    makeJob{unit=adv,post_actions={AssignBuildingRef},pos=adv.pos,from_pos=adv.pos,job_type=df.job_type.OperatePump}
end
function usetool:farmPlot(building)
    local adv=df.global.world.units.active[0]
    local do_harvest=false
    for id, con_item in pairs(building.contained_items) do
        if con_item.use_mode==2 and con_item.item:getType()==df.item_type.PLANT then
            if same_xyz(adv.pos,con_item.item.pos) then
                do_harvest=true
            end
        end
    end
    --check if there tile is without plantseeds,add job
    
    local args={unit=adv,pos=adv.pos,from_pos=adv.pos,
        }
    if not do_harvest then
        local seedjob={items={{quantity=1,item_type=df.item_type.SEEDS}}}
        args.job_type=df.job_type.PlantSeeds
        args.pre_actions={dfhack.curry(setFiltersUp,seedjob)}
        args.post_actions={AssignBuildingRef}
        
    else
        args.job_type=df.job_type.HarvestPlants
        args.post_actions={AssignBuildingRef}
    end
    local job,msg=makeJob(args)
    if not job then
        print(msg)
    else
        --print(AssignJobItems(args)) --WHY U NO WORK?
        
    end
end
MODES={
    [df.building_type.Table]={ --todo filters...
        name="Put items",
        input=usetool.openPutWindow,
    },
    [df.building_type.Coffin]={
        name="Put items",
        input=usetool.openPutWindow,
    },
    [df.building_type.Box]={
        name="Put items",
        input=usetool.openPutWindow,
    },
    [df.building_type.Weaponrack]={
        name="Put items",
        input=usetool.openPutWindow,
    },
    [df.building_type.Armorstand]={
        name="Put items",
        input=usetool.openPutWindow,
    },
    [df.building_type.Cabinet]={
        name="Put items",
        input=usetool.openPutWindow,
    },
    [df.building_type.Workshop]={
        name="Workshop menu",
        input=usetool.openShopWindowButtoned,
    },
    [df.building_type.Furnace]={
        name="Workshop menu",
        input=usetool.openShopWindowButtoned,
    },
    [df.building_type.SiegeEngine]={
        name="Siege menu",
        input=usetool.openSiegeWindow,
    },
    [df.building_type.FarmPlot]={
        name="Plant/Harvest",
        input=usetool.farmPlot,
    },
    [df.building_type.ScrewPump]={
        name="Operate Pump",
        input=usetool.operatePump,
    },
    [df.building_type.Trap]={
        name="Interact",
        input=usetool.armCleanTrap,
    },
    [df.building_type.Hive]={
        name="Hive actions",
        input=usetool.hiveActions,
    }
}
function usetool:shopMode(enable,mode,building)    
    self.subviews.shopLabel.visible=enable
    if mode then
        self.subviews.shopLabel:itemById("text1").text=mode.name
        self.building=building
    end
    self.mode=mode
end
function usetool:shopInput(keys)
    if keys[keybinds.workshop.key] then
        self:openShopWindowButtoned(self.in_shop)
    end
end

function usetool:setupFields()
    local adv=df.global.world.units.active[0]
    local civ_id=df.global.world.units.active[0].civ_id
    local ui=df.global.ui
    ui.civ_id = civ_id
    ui.main.fortress_entity=df.historical_entity.find(civ_id)
    ui.race_id=adv.race
    local nem=dfhack.units.getNemesis(adv)
    if nem then
        local links=nem.figure.entity_links
        for _,link in ipairs(links) do
            local hist_entity=df.historical_entity.find(link.entity_id)
            if hist_entity and hist_entity.type==df.historical_entity_type.SiteGovernment then
                ui.group_id=link.entity_id
                break
            end
        end
    end
    local site= inSite()
    if site then
        ui.site_id=site.id
    end
end
function usetool:fieldInput(keys)
    local adv=df.global.world.units.active[0]
    local cur_mode=actions[(mode or 0)+1]
    local failed=false
    for code,_ in pairs(keys) do
        --print(code)
        if MOVEMENT_KEYS[code] then
            local state={unit=adv,pos=moddedpos(adv.pos,MOVEMENT_KEYS[code]),dir=MOVEMENT_KEYS[code],
                    from_pos={x=adv.pos.x,y=adv.pos.y, z=adv.pos.z},post_actions=cur_mode[4],pre_actions=cur_mode[5],job_type=cur_mode[2],screen=self}
            if code=="SELECT" then
                if df.global.cursor.x~=-30000 then
                    state.pos={x=df.global.cursor.x,y=df.global.cursor.y,z=df.global.cursor.z}
                else
                    break
                end
            end
           
            for _,p in pairs(cur_mode[3] or {}) do
                local ok,msg=p(state)
                if ok==false then
                    dfhack.gui.showAnnouncement(msg,5,1)
                    failed=true
                end
            end
            
            if not failed then
                local ok,msg
                if type(cur_mode[2])=="function" then
                    ok,msg=cur_mode[2](state)
                else
                    ok,msg=makeJob(state)
                    --(adv,moddedpos(adv.pos,MOVEMENT_KEYS[code]),cur_mode[2],adv.pos,cur_mode[4])
                    
                end
                
                if code=="SELECT" then
                    self:sendInputToParent("LEAVESCREEN")
                end
                if ok then
                    self:sendInputToParent("A_WAIT")
                else
                    dfhack.gui.showAnnouncement(msg,5,1)
                end
            end
            return code
        end
        if code~="_STRING" and code~="_MOUSE_L" and code~="_MOUSE_R" then
            if ALLOWED_KEYS[code] then
                self:sendInputToParent(code)
            end
        end
    end
    
end
function usetool:onInput(keys)
    local adv=df.global.world.units.active[0]
    
    if keys.LEAVESCREEN  then
        if df.global.cursor.x~=-30000 then
            self:sendInputToParent("LEAVESCREEN")
        else
            self:dismiss()
            CancelJob(adv)
        end
    elseif keys[keybinds.nextJob.key] then
        mode=(mode+1)%#actions
    elseif keys[keybinds.prevJob.key] then
        mode=mode-1
        if mode<0 then mode=#actions-1 end
    --elseif keys.A_LOOK then
    --    self:sendInputToParent("A_LOOK")
    elseif keys[keybinds.continue.key] then
        ContinueJob(adv)
        self:sendInputToParent("A_WAIT")
    else
        if self.mode~=nil then
            if keys[keybinds.workshop.key] then
                self.mode.input(self,self.building)
            end
            self:fieldInput(keys)    
        else
            self:fieldInput(keys)
        end
    end
    local site=inSite()
    
    if site then
        self.subviews.siteLabel.visible=true
        self.subviews.siteLabel:itemById("site").text=dfhack.TranslateName(site.name)
    else
        self.subviews.siteLabel.visible=false
    end
end
function usetool:isOnBuilding()
    local adv=df.global.world.units.active[0]
    local bld=dfhack.buildings.findAtTile(adv.pos)
    if bld and MODES[bld:getType()]~=nil and bld:getBuildStage()==bld:getMaxBuildStage() then
        return true,MODES[bld:getType()],bld
    else
        return false
    end
end
function usetool:onRenderBody(dc)
    self:shopMode(self:isOnBuilding())
    self:renderParent()
end
if not (dfhack.gui.getCurFocus()=="dungeonmode/Look" or dfhack.gui.getCurFocus()=="dungeonmode/Default") then
    qerror("This script requires an adventurer mode with (l)ook or default mode.")
end
usetool():show()