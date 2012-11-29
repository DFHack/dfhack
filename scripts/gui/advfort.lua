-- allows to do jobs in adv. mode.
local gui = require 'gui'
local wid=require 'gui.widgets'
local dialog=require 'gui.dialogs'
local buildings=require 'dfhack.buildings'
local bdialog=require 'gui.buildings'

local tile_attrs = df.tiletype.attrs
settings={build_by_items=false,check_inv=false,df_assign=true}
for k,v in ipairs({...}) do
    if v=="-c" or v=="--cheat" then
        settings.build_by_items=true
        settings.df_assign=false
    elseif v=="-i" or v=="--inventory" then
        settings.check_inv=true
        settings.df_assign=false
    elseif v=="-a" or v=="--nodfassign" then
        settings.df_assign=false
    end
end
mode=mode or 0
keybinds={
key_next={key="CUSTOM_SHIFT_T",desc="Next job in the list"},
key_prev={key="CUSTOM_SHIFT_R",desc="Previous job in the list"},
key_continue={key="A_WAIT",desc="Continue job if available"},
key_down_alt1={key="CUSTOM_CTRL_D",desc="Use job down"},
key_down_alt2={key="CURSOR_DOWN_Z_AUX",desc="Use job down"},
key_up_alt1={key="CUSTOM_CTRL_E",desc="Use job up"}, 
key_up_alt2={key="CURSOR_UP_Z_AUX",desc="Use job up"},
key_use_same={key="A_MOVE_SAME_SQUARE",desc="Use job at the tile you are standing"},
}
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
    require("gui.dialogs").showMessage("Help!?!",helptext)
end
function getLastJobLink()
    local st=df.global.world.job_list
    while st.next~=nil do
        st=st.next
    end
    return st
end
function AddNewJob(job)
    local nn=getLastJobLink()
    local nl=df.job_list_link:new()
    nl.prev=nn
    nn.next=nl
    nl.item=job
    job.list_link=nl
end
function MakeJob(args)
    local nj=df.job:new()
    nj.id=df.global.job_next_id
    df.global.job_next_id=df.global.job_next_id+1
    --nj.flags.special=true
    nj.job_type=args.job_type
    nj.completion_timer=-1

    nj.pos:assign(args.pos)
    args.job=nj
    local failed=false
    for k,v in ipairs(args.pre_actions or {}) do
        local ok,msg=v(args)
        if not ok then
            failed=true
            return false,msg
        end
    end
    if not failed then
        AssignUnitToJob(nj,args.unit,args.from_pos)
    end
    for k,v in ipairs(args.post_actions or {}) do
        v(args)
    end
    AddNewJob(nj)
    return nj
end

function AssignUnitToJob(job,unit,unit_pos)
    job.general_refs:insert("#",{new=df.general_ref_unit_workerst,unit_id=unit.id})
    unit.job.current_job=job
    unit_pos=unit_pos or {x=job.pos.x,y=job.pos.y,z=job.pos.z}
    unit.path.dest:assign(unit_pos)
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

function MakePredicateWieldsItem(item_skill)
    local pred=function(args)
        local inv=args.unit.inventory
        for k,v in pairs(inv) do
            if v.mode==1 and df.item_weaponst:is_instance(v.item) then
                if v.item.subtype.skill_melee==item_skill and args.unit.body.weapon_bp==v.body_part_id then
                    return true
                end
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
function IsHardMaterial(args)
    local tt=dfhack.maps.getTileType(args.pos)
    local mat=tile_attrs[tt].material
    local hard_materials={df.tiletype_material.STONE,df.tiletype_material.FEATURE,
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
    if tile_attrs[tt].shape==df.tiletype_shape.PLANT then
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
function itemsAtPos(pos)
    local ret={}
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
end

function BuildingChosen(inp_args,type_id,subtype_id,custom_id)
    local args={}
    args.type=type_id
    args.subtype=subtype_id
    args.custom=custom_id
    args.pos=inp_args.pos
    --if settings.build_by_items then
    --    args.items=itemsAtPos(inp_args.from_pos)
    --end
    --printall(args.items)
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
function AssignJobItems(args)
    if settings.df_assign then
        return true
    end
    --printall(args)
    local job=args.job
    local its=itemsAtPos(args.from_pos)
    if settings.check_inv then
        for k,v in pairs(args.unit.inventory) do
            table.insert(its,v.item)
        end
    end
    --[[while(#job.items>0) do --clear old job items
        job.items[#job.items-1]:delete()
        job.items:erase(#job.items-1)
    end]]
    local used_item_id={}
    for job_id, trg_job_item in ipairs(job.job_items) do
        for _,cur_item in pairs(its) do 
            if not used_item_id[cur_item.id] then
                if (trg_job_item.quantity>0 and dfhack.job.isSuitableItem(trg_job_item, cur_item:getType(), cur_item:getSubtype()) and
                    dfhack.job.isSuitableMaterial(trg_job_item, cur_item:getMaterial(), cur_item:getMaterialIndex())) or settings.build_by_items then
                    job.items:insert("#",{new=true,item=cur_item,role=2,job_item_idx=job_id})
                    trg_job_item.quantity=trg_job_item.quantity-1
                    --print(string.format("item added, job_item_id=%d, item %s, quantity left=%d",job_id,tostring(cur_item),trg_job_item.quantity))
                    used_item_id[cur_item.id]=true
                end
            end
        end
    end
    --[[print("+============+")
    printall(job.items)
    print("-============-")
    printall(job.job_items)
    print("+============+")]]--
    if not settings.build_by_items then
        for job_id, trg_job_item in ipairs(job.job_items) do
            if trg_job_item.quantity>0 then
                return false, "Not enough items for this job"
            end
        end
    end
    return true
end
function AssignJobToBuild(args)
    local bld=dfhack.buildings.findAtTile(args.pos)
    args.job_type=df.job_type.ConstructBuilding
    if bld~=nil then
        if #bld.jobs>0 then
            args.job=bld.jobs[0]
            local ok,msg=AssignJobItems(args)
            if not ok then
                return false,msg
            else
                AssignUnitToJob(bld.jobs[0],args.unit,args.from_pos) --todo check if correct job type
            end
        else
            args.pre_action=AssignJobItems
            local ok,msg=MakeJob(args)
            return ok,msg
        end
    else
        bdialog.BuildingDialog{on_select=dfhack.curry(BuildingChosen,args),hide_none=true}:show()
    end
    return true
end
function CancelJob(unit)
    local c_job=unit.job.current_job 
    if c_job then
        unit.job.current_job =nil --todo add real cancelation        
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

dig_modes={
    {"CarveFortification"   ,df.job_type.CarveFortification,{IsWall,IsHardMat}},
    {"DetailWall"           ,df.job_type.DetailWall,{IsWall,IsHardMat}},
    {"DetailFloor"          ,df.job_type.DetailFloor,{IsFloor,IsHardMat}},
    --{"CarveTrack"           ,df.job_type.CarveTrack}, -- does not work??
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
    --{"TameAnimal"           ,df.job_type.TameAnimal,{IsUnit},{SetCreatureRef}}, 
    {"GatherPlants"         ,df.job_type.GatherPlants,{IsPlant}},
    {"RemoveConstruction"   ,df.job_type.RemoveConstruction,{IsConstruct}},
    {"RemoveBuilding"       ,RemoveBuilding,{IsBuilding}},
    --{"HandleLargeCreature"   ,df.job_type.HandleLargeCreature,{isUnit},{SetCreatureRef}},
    {"Build"                ,AssignJobToBuild},
    --{"ConstructBlocks"         ,df.job_type.ConstructBlocks,{},{SetBuildingRef},{AssignJobItems}},
    {"RemoveStairs"         ,df.job_type.RemoveStairs,{IsStairs,NotConstruct}},
    --{"LoadCageTrap"         ,df.job_type.LoadCageTrap,{},{SetBuildingRef},{AssignJobItems}},
    
}


usetool=defclass(usetool,gui.Screen)
function usetool:getShopMode()
    return "In shop"
end
function usetool:getModeName()
    local adv=df.global.world.units.active[0]
    if adv.job.current_job then
        return string.format("%s working(%d) ",(dig_modes[(mode or 0)+1][1] or ""),adv.job.current_job.completion_timer)
    else
        return dig_modes[(mode or 0)+1][1] or " "
    end
    
end

function usetool:init(args)
    self:addviews{
        wid.Label{
            view_id="main_label",
            frame = {xalign=0,yalign=0},
            text={{key=keybinds.key_prev.key},{gap=1,text=dfhack.curry(usetool.getModeName,self)},{gap=1,key=keybinds.key_next.key}}
                  },
        wid.Label{
            view_id="shop_label",
            visible=false,
            frame = {xalign=0,yalign=0},
            text={{key=keybinds.key_prev.key},{gap=1,text=dfhack.curry(usetool.getShopMode,self),pen=dfhack.pen.parse{fg=COLOR_YELLOW,bg=0}},{gap=1,key=keybinds.key_next.key}}
                  }
            }
end
function usetool:onRenderBody(dc)
    
    self:renderParent()
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
function usetool:onDismiss()
    local adv=df.global.world.units.active[0]
    --TODO: cancel job
    --[[if adv and adv.job.current_job then
        local cj=adv.job.current_job
        adv.jobs.current_job=nil
        cj:delete()
    end]]
end
function usetool:onHelp()
    showHelp()
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
    elseif keys[keybinds.key_next.key] then
        mode=(mode+1)%#dig_modes
    elseif keys[keybinds.key_prev.key] then
        mode=mode-1
        if mode<0 then mode=#dig_modes-1 end
    --elseif keys.A_LOOK then
    --    self:sendInputToParent("A_LOOK")
    elseif keys[keybinds.key_continue.key] then
        ContinueJob(adv)
        self:sendInputToParent("A_WAIT")
    else
        
        local cur_mode=dig_modes[(mode or 0)+1]
        local failed=false
        for code,_ in pairs(keys) do
            --print(code)
            if MOVEMENT_KEYS[code] then
                local state={unit=adv,pos=moddedpos(adv.pos,MOVEMENT_KEYS[code]),dir=MOVEMENT_KEYS[code],
                        from_pos={x=adv.pos.x,y=adv.pos.y, z=adv.pos.z},post_action=cur_mode[4],pre_action=cur_mode[5],job_type=cur_mode[2],screen=self}
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
                        ok,msg=MakeJob(state)
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
end
usetool():show()