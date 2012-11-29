-- allows to do jobs in adv. mode.
local gui = require 'gui'
local wid=require 'gui.widgets'
local dialog=require 'gui.dialogs'
local buildings=require 'dfhack.buildings'

local tile_attrs = df.tiletype.attrs

mode=mode or 0
keybinds={
key_next={key="CUSTOM_SHIFT_T",desc="Next job in the list"},
key_prev={key="CUSTOM_SHIFT_R",desc="Previous job in the list"},
key_continue={key="A_WAIT",desc="Continue job if available"},
key_down_alt1={key="CUSTOM_CTRL_D",desc="Use job down"},--does not work?
key_down_alt2={key="CURSOR_DOWN_Z_AUX",desc="Use job down"},
key_up_alt1={key="CUSTOM_CTRL_E",desc="Use job up"}, --does not work?
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
function MakeJob(unit,pos,job_type,unit_pos,post_actions)
    local nj=df.job:new()
    nj.id=df.global.job_next_id
    df.global.job_next_id=df.global.job_next_id+1
    --nj.flags.special=true
    nj.job_type=job_type
    nj.completion_timer=-1
    --nj.unk4a=12
    --nj.unk4b=0
    nj.pos:assign(pos)
    AssignUnitToJob(nj,unit,unit_pos)
    for k,v in ipairs(post_actions or {}) do
        v{job=nj,pos=pos,old_pos=unit_pos,unit=unit}
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
function AssignItems(items,args)
    
end
--[[ building submodule... ]]--
function DialogBuildingChoose(on_select, on_cancel)
    blist={}
    for i=df.building_type._first_item,df.building_type._last_item do
        table.insert(blist,df.building_type[i])
    end
    dialog.showListPrompt("Building list", "Choose building:", COLOR_WHITE, blist, on_select, on_cancel, nil, true)
end
function DialogSubtypeChoose(subtype,on_select, on_cancel)
    blist={}
    for i=subtype._first_item,subtype._last_item do
        table.insert(blist,subtype[i])
    end
    dialog.showListPrompt("Subtype", "Choose subtype:", COLOR_WHITE, blist, on_select, on_cancel, nil, true)
end
--workshop, furnaces, traps
invalid_buildings={}
function SubtypeChosen(args,index)
    args.subtype=index-1
    buildings.constructBuilding(args)
end
function BuildingChosen(st_pos,pos,index)
    local b_type=index-2
    local args={}
    args.type=b_type
    args.pos=pos
    args.items=itemsAtPos(st_pos)
    if invalid_buildings[b_type] then
        return 
    elseif b_type==df.building_type.Construction then
        DialogSubtypeChoose(df.construction_type,dfhack.curry(SubtypeChosen,args))
        return
    elseif b_type==df.building_type.Furnace then
        DialogSubtypeChoose(df.furnace_type,dfhack.curry(SubtypeChosen,args))
        return
    elseif b_type==df.building_type.Trap then
        DialogSubtypeChoose(df.trap_type,dfhack.curry(SubtypeChosen,args))
        return
    elseif b_type==df.building_type.Workshop then
        DialogSubtypeChoose(df.workshop_type,dfhack.curry(SubtypeChosen,args))
        return
    else
        buildings.constructBuilding(args)
    end
end

--[[ end of buildings ]]--
function RemoveBuilding(args)
    local bld=dfhack.buildings.findAtTile(args.pos)
    if bld~=nil then
        bld:queueDestroy()
        for k,v in ipairs(bld.jobs) do
            if v.job_type==df.job_type.DestroyBuilding then
                AssignUnitToJob(v,args.unit,args.old_pos)
                return
            end
        end
    end
end

function AssignJobToBuild(args)
    local bld=dfhack.buildings.findAtTile(args.pos)
    if bld~=nil then
        if #bld.jobs>0 then
            AssignUnitToJob(bld.jobs[0],args.unit,args.old_pos)
        else
            local jb=MakeJob(args.unit,args.pos,df.job_type.ConstructBuilding,args.old_pos,{AssignBuildingRef})
            local its=itemsAtPos(args.old_pos)
            for k,v in pairs(its) do
                jb.items:insert("#",{new=true,item=v,role=2})
            end
            
        end
    else
        DialogBuildingChoose(dfhack.curry(BuildingChosen,args.old_pos,args.pos))
    end
end
function ContinueJob(unit)
    local c_job=unit.job.current_job 
    if c_job then
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
    {"RemoveStairs"         ,df.job_type.RemoveStairs,{IsStairs,NotConstruct}},
    
}


usetool=defclass(usetool,gui.Screen)
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
            frame = {xalign=0,yalign=0},
            text={{key=keybinds.key_prev.key},{gap=1,text=dfhack.curry(usetool.getModeName,self)},{gap=1,key=keybinds.key_next.key}}
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

    if keys.LEAVESCREEN  then
        if df.global.cursor.x~=-30000 then
            self:sendInputToParent("LEAVESCREEN")
        else
            self:dismiss()
        end
    elseif keys[keybinds.key_next.key] then
        mode=(mode+1)%#dig_modes
    elseif keys[keybinds.key_prev.key] then
        mode=mode-1
        if mode<0 then mode=#dig_modes-1 end
    --elseif keys.A_LOOK then
    --    self:sendInputToParent("A_LOOK")
    elseif keys[keybinds.key_continue.key] then
        ContinueJob(df.global.world.units.active[0])
        self:sendInputToParent("A_WAIT")
    else
        local adv=df.global.world.units.active[0]
        local cur_mode=dig_modes[(mode or 0)+1]
        local failed=false
        for code,_ in pairs(keys) do
            --print(code)
            if MOVEMENT_KEYS[code] then
                local state={unit=adv,pos=moddedpos(adv.pos,MOVEMENT_KEYS[code]),dir=MOVEMENT_KEYS[code],
                        old_pos={x=adv.pos.x,y=adv.pos.y, z=adv.pos.z}}
                if code=="SELECT" then
                    if df.global.cursor.x~=-30000 then
                        state.pos={x=df.global.cursor.x,y=df.global.cursor.y,z=df.global.cursor.z}
                    else
                        break
                    end
                end
               
                for _,p in pairs(cur_mode[3] or {}) do
                    local t,v=p(state)
                    if t==false then
                        dfhack.gui.showAnnouncement(v,5,1)
                        failed=true
                    end
                end
                if not failed then
                    if type(cur_mode[2])=="function" then
                        cur_mode[2](state)
                    else
                        MakeJob(adv,moddedpos(adv.pos,MOVEMENT_KEYS[code]),cur_mode[2],adv.pos,cur_mode[4])
                    end
                    
                    if code=="SELECT" then
                        self:sendInputToParent("LEAVESCREEN")
                    end
                    
                    self:sendInputToParent("A_WAIT")
                    
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