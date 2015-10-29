-- allows to do jobs in adv. mode.

--[[=begin

gui/advfort
===========
This script allows to perform jobs in adventure mode. For more complete help
press :kbd:`?` while script is running. It's most comfortable to use this as a
keybinding. (e.g. ``keybinding set Ctrl-T gui/advfort``). Possible arguments:

:-a, --nodfassign:  uses different method to assign items.
:-i, --inventory:   checks inventory for possible items to use in the job.
:-c, --cheat:       relaxes item requirements for buildings (e.g. walls from bones). Implies -a
:job:               selects that job (e.g. Dig or FellTree)

An example of player digging in adventure mode:

.. image:: /docs/images/advfort.png

**WARNING:**  changes only persist in non procedural sites, namely: player forts, caves, and camps.

=end]]

--[==[
    version: 0.044
    changelog:
        *0.044
        - added output to clear_jobs of number of cleared jobs
        - another failed attempt at gather plants fix
        - added track stop configuration window
        *0.043
        - fixed track carving: up/down was reversed and removed (temp) requirements because they were not working correctly
        - added checks for unsafe conditions (currently quite stupid). Should save few adventurers that are trying to work in dangerous conditions (e.g. fishing)
        - unsafe checks disabled by "-u" ir "--unsafe"
        *0.042
        - fixed (probably for sure now) the crash bug.
        - added --clear_jobs debug option. Will delete ALL JOBS!
        *0.041
        - fixed cooking allowing already cooked meals
        *0.04
        - add (-q)uick mode. Autoselects materials.
        - fixed few(?) crash bugs
        - fixed job errors not being shown in df
        *0.031
        - make forbiding optional (-s)afe mode
        *0.03
        - forbid doing anything in non-sites unless you are (-c)heating
        - a bit more documentation and tidying
        - add a deadlock fix
        *0.021
        - advfort_items now autofills items
        - tried out few things to fix gather plants
        *0.02
        - fixed axles not being able to be placed in other direction (thanks SyrusLD)
        - added lever linking
        - restructured advfort_items, don't forget to update that too!
        - Added clutter view if shop is cluttered.
        *0.013
        - fixed siege weapons and traps (somewhat). Now you can load them with new menu :)
        *0.012
        - fix for some jobs not finding correct building.
        *0.011
        - fixed crash with building jobs (other jobs might have been crashing too!)
        - fixed bug with building asking twice to input items
        *0.01
        - instant job startation
        - item selection screen (!)
        - BUG:custom jobs need stuff on ground to work
        *0.003
        - fixed farms (i think...)
        - added faster time pasing (yay for random deaths from local wildlife)
        - still hasn't fixed gather plants. but you can visit local market, buy a few local fruits/vegetables eat them and use seeds
        - other stuff
        *0.002
        - kind-of fixed the item problem... now they get teleported (if teleport_items=true which should be default for adventurer)
        - gather plants still not working... Other jobs seem to work.
        - added new-and-improved waiting. Interestingly it could be improved to be interuptable.
    todo list:
        - document everything! Maybe somebody would understand what is happening then and help me :<
        - when building trap add to known traps (or known adventurers?) so that it does not trigger on adventurer
    bugs list:
        - items blocking construction stuck the game
        - burning charcoal crashed game
        - gem thingies probably broken
        - custom reactions semibroken
        - gathering plants still broken

--]==]

--keybinding, change to your hearts content. Only the key part.
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
quick={key="CUSTOM_Q",desc="Toggle quick item select"},
}
-- building filters
build_filter={
    forbid_all=false, --this forbits all except the "allow"
    allow={"MetalSmithsForge"}, --ignored if forbit_all=false
    forbid={} --ignored if forbit_all==true
}
build_filter.HUMANish={
    forbid_all=true,
    allow={"Masons"},
    forbid={}
}

--economic stone fix: just disable all of them
--[[ FIXME: maybe let player select which to disable?]]
for k,v in ipairs(df.global.ui.economic_stone) do df.global.ui.economic_stone[k]=0 end

local gui = require 'gui'
local wid=require 'gui.widgets'
local dialog=require 'gui.dialogs'
local buildings=require 'dfhack.buildings'
local bdialog=require 'gui.buildings'
local workshopJobs=require 'dfhack.workshops'
local utils=require 'utils'
local gscript=require 'gui.script'

local tile_attrs = df.tiletype.attrs

settings={build_by_items=false,use_worn=false,check_inv=true,teleport_items=true,df_assign=false,gui_item_select=true,only_in_sites=false}

function hasValue(tbl,val)
    for k,v in pairs(tbl) do
        if v==val then
            return true
        end
    end
    return false
end
function reverseRaceLookup(id)
    return df.global.world.raws.creatures.all[id].creature_id
end
function deon_filter(name,type_id,subtype_id,custom_id, parent)
    --print(name)
    local adv=df.global.world.units.active[0]
    local race_filter=build_filter[reverseRaceLookup(adv.race)]
    if race_filter then
        if race_filter.forbid_all then
            return hasValue(race_filter.allow,name)
        else
            return not hasValue(race_filter.forbid,name)
        end
    else
        if build_filter.forbid_all then
            return hasValue(build_filter.allow,name)
        else
            return not hasValue(build_filter.forbid,name)
        end
    end
end
local mode_name
for k,v in ipairs({...}) do --setting parsing
    if v=="-c" or v=="--cheat" then
        settings.build_by_items=true
        settings.df_assign=false
    elseif v=="-q" or v=="--quick" then
        settings.quick=true
    elseif v=="-u" or v=="--unsafe" then --ignore pain and etc
        settings.unsafe=true
    elseif v=="-s" or v=="--safe" then
        settings.safe=true
    elseif v=="-i" or v=="--inventory" then
        settings.check_inv=true
        settings.df_assign=false
    elseif v=="-a" or v=="--nodfassign" then
        settings.df_assign=false
    elseif v=="-h" or v=="--help" then
        settings.help=true
    elseif v=="--clear_jobs" then
        settings.clear_jobs=true
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

if settings.help then
    showHelp()
    return
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

    for k,v in pairs(df.global.world.world_data.sites) do
        local tp={v.pos.x,v.pos.y}
        if tx>=tp[1]*16+v.rgn_min_x and tx<=tp[1]*16+v.rgn_max_x and
            ty>=tp[2]*16+v.rgn_min_y and ty<=tp[2]*16+v.rgn_max_y then
            return v
        end
    end
end
--[[    low level job management    ]]--
function findAction(unit,ltype)
    ltype=ltype or df.unit_action_type.None
    for i,v in ipairs(unit.actions) do
        if v.type==ltype then
            return v
        end
    end
end
function add_action(unit,action_data)
    local action=findAction(unit) --find empty action
    if action then
        action:assign(action_data)
        action.id=unit.next_action_id
        unit.next_action_id=unit.next_action_id+1
    else
        local tbl=copyall(action_data)
        tbl.new=true
        tbl.id=unit.next_action_id
        unit.actions:insert("#",tbl)
        unit.next_action_id=unit.next_action_id+1
    end
end
function addJobAction(job,unit) --what about job2?
    if job==nil then
        error("invalid job")
    end
    if findAction(unit,df.unit_action_type.Job) or findAction(unit,df.unit_action_type.Job2) then
        print("Already has job action")
        return
    end
    local action=findAction(unit)
    local pos=copyall(unit.pos)
    --local pos=copyall(job.pos)
    unit.path.dest:assign(pos)
    --job
    local data={type=df.unit_action_type.Job,data={job={x=pos.x,y=pos.y,z=pos.z,timer=10}}}
    --job2:
    --local data={type=df.unit_action_type.Job2,data={job2={timer=10}}}
    add_action(unit,data)
    --add_action(unit,{type=df.unit_action_type.Unsteady,data={unsteady={timer=5}}})
end

function make_native_job(args)
    if args.job == nil then
        local newJob=df.job:new()
        newJob.id=df.global.job_next_id
        df.global.job_next_id=df.global.job_next_id+1
        newJob.flags.special=true
        newJob.job_type=args.job_type
        newJob.completion_timer=-1

        newJob.pos:assign(args.pos)
        --newJob.pos:assign(args.unit.pos)
        args.job=newJob
        args.unlinked=true
    end
end
function smart_job_delete( job )
    local gref_types=df.general_ref_type
    --TODO: unmark items as in job
    for i,v in ipairs(job.general_refs) do
        if v:getType()==gref_types.BUILDING_HOLDER then
            local b=v:getBuilding()
            if b then
                --remove from building
                for i,v in ipairs(b.jobs) do
                    if v==job then
                        b.jobs:erase(i)
                        break
                    end
                end
            else
                print("Warning: building holder ref was invalid while deleting job")
            end
        elseif v:getType()==gref_types.UNIT_WORKER then
            local u=v:getUnit()
            if u then
                u.job.current_job =nil
            else
                print("Warning: unit worker ref was invalid while deleting job")
            end
        else
            print("Warning: failed to remove link from job with type:",gref_types[v:getType()])
        end
    end
    --unlink job
    local link=job.list_link
    if link.prev then
        link.prev.next=link.next
    end
    if link.next then
        link.next.prev=link.prev
    end
    link:delete()
    --finally delete the job
    job:delete()
end
--TODO: this logic might be better with other --starting logic--
if settings.clear_jobs then
    print("Clearing job list!")
    local counter=0
    local job_link=df.global.world.job_list.next
    while job_link and job_link.item do
        local job=job_link.item
        job_link=job_link.next
        smart_job_delete(job)
        counter=counter+1
    end
    print("Deleted: "..counter.." jobs")
    return
end
function makeJob(args)
    gscript.start(function ()
        make_native_job(args)
        local failed
        for k,v in ipairs(args.pre_actions or {}) do
            local ok,msg=v(args)
            if not ok then
                failed=msg
                break
            end
        end
        if failed==nil then
            AssignUnitToJob(args.job,args.unit,args.from_pos)
            for k,v in ipairs(args.post_actions or {}) do
                local ok,msg=v(args)
                if not ok then
                    failed=msg
                    break
                end
            end
            if failed then
                UnassignJob(args.job,args.unit)
            end
        end
        if failed==nil then
            if args.unlinked then
                dfhack.job.linkIntoWorld(args.job,true)
                args.unlinked=false
            end
            addJobAction(args.job,args.unit)
            args.screen:wait_tick()
        else
            if not args.no_job_delete then
                smart_job_delete(args.job)
            end
            dfhack.gui.showAnnouncement("Job failed:"..failed,5,1)
        end
    end)
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

function SetWebRef(args)
    local pos=args.pos
    for k,v in pairs(df.global.world.items.other.ANY_WEBS) do
        if v.pos.x==pos.x and v.pos.y==pos.y and v.pos.z==pos.z then
            args.job.general_refs:insert("#",{new=df.general_ref_item,item_id=v.id})
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
        job.item_category[dirs.down]=true
    elseif pos.y<from_pos.y then
        job.item_category[dirs.up]=true
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
    if tile_attrs[tt].material==df.tiletype_material.TREE then
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
    local bld=args.building or dfhack.buildings.findAtTile(args.pos)
    args.job.general_refs:insert("#",{new=df.general_ref_building_holderst,building_id=bld.id})
    bld.jobs:insert("#",args.job)
    args.building=args.building or bld
    return true
end
function chooseBuildingWidthHeightDir(args) --TODO nicer selection dialog
    local btype=df.building_type
    local area=makeset{"w","h"}
    local all=makeset{"w","h","d"}
    local needs={[btype.FarmPlot]=area,[btype.Bridge]=all,
        [btype.RoadDirt]=area,[btype.RoadPaved]=area,[btype.ScrewPump]=makeset{"d"},
        [btype.AxleHorizontal]=all,[btype.WaterWheel]=makeset{"d"},[btype.Rollers]=makeset{"d"}}
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
CheckAndFinishBuilding=nil
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
    args.building=buildings.constructBuilding(args)
    CheckAndFinishBuilding(args,args.building)
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
    if job_item.flags1.cookable and item:getType()==df.item_type.FOOD then
        return false,"already cooked"
    end

    if type(job_item) ~= "table" and not matinfo:matches(job_item) then
        --[[
        local true_flags={}
        for k,v in pairs(job_item.flags1) do
            if v then
                table.insert(true_flags,k)
            end
        end
        for k,v in pairs(job_item.flags2) do
            if v then
                table.insert(true_flags,k)
            end
        end
        for k,v in pairs(job_item.flags3) do
            if v then
                table.insert(true_flags,k)
            end
        end
        for k,v in pairs(true_flags) do
            print(v)
        end
        --]]
        
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
    -- if #job_item.contains~=0 then
    -- end
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
function putItemsInBuilding(building,job_item_refs)
    for k,v in ipairs(job_item_refs) do
        --local pos=dfhack.items.getPosition(v)
        if not dfhack.items.moveToBuilding(v.item,building,0) then
            print("Could not put item:",k,v.item)
        end
        v.is_fetching=0
    end
end
function putItemsInHauling(unit,job_item_refs)
    for k,v in ipairs(job_item_refs) do
        --local pos=dfhack.items.getPosition(v)
        print("moving:",tostring(v),tostring(v.item))
        printall(v)
        if not dfhack.items.moveToInventory(v.item,unit,0,0) then
            print("Could not put item:",k,v.item)
        end
        v.is_fetching=0
    end
end
function finish_item_assign(args)
    local job=args.job
    local item_modes={
        [df.job_type.PlantSeeds]="haul",
        [df.job_type.Eat]="haul",
    }
    local item_mode=item_modes[job.job_type] or "teleport"
    if settings.teleport_items and item_mode=="teleport" then
        putItemsInBuilding(args.building,job.items)
    end

    local uncollected = getItemsUncollected(job)
    if #uncollected == 0 then
        job.flags.working=true
        if item_mode=="haul" then
            putItemsInHauling(args.unit,job.items)
        end
    else
        job.flags.fetching=true
        uncollected[1].is_fetching=1
    end
end
function EnumItems_with_settings( args )
    if settings.check_inv then
        return EnumItems{pos=args.from_pos,unit=args.unit,
                inv={[df.unit_inventory_item.T_mode.Hauled]=settings.use_worn,[df.unit_inventory_item.T_mode.Worn]=settings.use_worn,
                [df.unit_inventory_item.T_mode.Weapon]=settings.use_worn,},deep=true}
    else
        return EnumItems{pos=args.from_pos}
    end
end
function find_suitable_items(job,items,job_items)
    job_items=job_items or job.job_items

    local item_counts={}
    for job_id, trg_job_item in ipairs(job_items) do
        item_counts[job_id]=trg_job_item.quantity
    end

    local item_suitability={}
    local used_item_id={}
    for job_id, trg_job_item in ipairs(job_items) do
        item_suitability[job_id]={}

        for _,cur_item in pairs(items) do
            if not used_item_id[cur_item.id] then
                local item_suitable,msg=isSuitableItem(trg_job_item,cur_item)
                if item_suitable or settings.build_by_items then
                    table.insert(item_suitability[job_id],cur_item)
                end
                --[[
                if msg then
                    print(cur_item,msg)
                else
                    print(cur_item,"ok")
                end
                --]]
                if not settings.gui_item_select then
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
    end

    return item_suitability,item_counts
end
function AssignJobItems(args)
    if settings.df_assign then --use df default logic and hope that it would work
        return true
    end
    -- first find items that you want to use for the job
    local job=args.job
    local its=EnumItems_with_settings(args)
    
    local item_suitability,item_counts=find_suitable_items(job,its)
    --[[while(#job.items>0) do --clear old job items
        job.items[#job.items-1]:delete()
        job.items:erase(#job.items-1)
    end]]

    if settings.gui_item_select and #job.job_items>0 then
        local item_dialog=require('hack.scripts.gui.advfort_items')
       
        if settings.quick then --TODO not so nice hack. instead of rewriting logic for job item filling i'm using one in gui dialog...
            local item_editor=item_dialog.jobitemEditor{
                job = job,
                items = item_suitability,
            }
            if item_editor:jobValid() then
                item_editor:commit()
                finish_item_assign(args)
                return true
            else
                return false, "Quick select items"
            end
        else
            local ret=item_dialog.showItemEditor(job,item_suitability)
            if ret then
                finish_item_assign(args)
                return true
            else
                print("Failed job, i'm confused...")
            end
            --end)
            return false,"Selecting items"
        end
    else
        if not settings.build_by_items then
            for job_id, trg_job_item in ipairs(job.job_items) do
                if item_counts[job_id]>0 then
                    print("Not enough items for this job")
                    return false, "Not enough items for this job"
                end
            end
        end
        finish_item_assign(args)
        return true
    end

end

CheckAndFinishBuilding=function (args,bld)
    args.building=args.building or bld
    for idx,job in pairs(bld.jobs) do
        if job.job_type==df.job_type.ConstructBuilding then
            args.job=job
            args.no_job_delete=true
            break
        end
    end

    if args.job~=nil then
        args.pre_actions={AssignJobItems}
    else
        local t={items=buildings.getFiltersByType({},bld:getType(),bld:getSubtype(),bld:getCustomType())}
        args.pre_actions={dfhack.curry(setFiltersUp,t),AssignBuildingRef}--,AssignJobItems
    end
    args.no_job_delete=true
    makeJob(args)
end
function AssignJobToBuild(args)
    local bld=args.building or dfhack.buildings.findAtTile(args.pos)
    args.building=bld
    args.job_type=df.job_type.ConstructBuilding
    if bld~=nil then
        CheckAndFinishBuilding(args,bld)
    else
        bdialog.BuildingDialog{on_select=dfhack.curry(BuildingChosen,args),hide_none=true,building_filter=deon_filter}:show()
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
    --no job to continue
    if not c_job then return end
    --reset suspends...
    c_job.flags.suspend=false
    for k,v in pairs(c_job.items) do --try fetching missing items
        if v.is_fetching==1 then
            unit.path.dest:assign(v.item.pos)
            return
        end
    end

    --unit.path.dest:assign(c_job.pos) -- FIXME: job pos is not always the target pos!!
    addJobAction(c_job,unit)
end
--TODO: in far far future maybe add real linking?
-- function assign_link_refs(args )
--     local job=args.job
--     --job.general_refs:insert("#",{new=df.general_ref_building_holderst,building_id=args.building.id})
--     job.general_refs:insert("#",{new=df.general_ref_building_triggertargetst,building_id=args.triggertarget.id})
--     printall(job)
-- end
-- function assign_link_roles( args )
--     if #args.job.items~=2 then
--         print("AAA FAILED!")
--         return false
--     end
--     args.job.items[0].role=df.job_item_ref.T_role.LinkToTarget
--     args.job.items[1].role=df.job_item_ref.T_role.LinkToTrigger
-- end
function fake_linking(lever,building,slots)
    local item1=slots[1].items[1]
    local item2=slots[2].items[1]
    if not dfhack.items.moveToBuilding(item1,lever,2) then
        qerror("failed to move item to building")
    end
    if not dfhack.items.moveToBuilding(item2,building,2) then
        qerror("failed to move item2 to building")
    end
    item2.general_refs:insert("#",{new=df.general_ref_building_triggerst,building_id=lever.id})
    item1.general_refs:insert("#",{new=df.general_ref_building_triggertargetst,building_id=building.id})
    
    lever.linked_mechanisms:insert("#",item2)
    --fixes...
    if building:getType()==df.building_type.Door then
        building.door_flags.operated_by_mechanisms=true
    end

    dfhack.gui.showAnnouncement("Linked!",COLOR_YELLOW,true)
end
function LinkBuilding(args)
    local bld=args.building or dfhack.buildings.findAtTile(args.pos)
    args.building=bld

    local lever_bld
    if lever_id then --intentionally global!
        lever_bld=df.building.find(lever_id)
        if lever_bld==nil then
            lever_id=nil
        end
    end
    if lever_bld==nil then
        if bld:getType()==df.building_type.Trap and bld:getSubtype()==df.trap_type.Lever then
            lever_id=bld.id
            dfhack.gui.showAnnouncement("Selected lever for linking",COLOR_YELLOW,true)
            return
        else
            dfhack.gui.showAnnouncement("You first need a lever",COLOR_RED,true)
        end
    else
        if lever_bld==bld then
            dfhack.gui.showAnnouncement("Invalid target",COLOR_RED,true) --todo more invalid targets
            return
        end
        -- args.job_type=df.job_type.LinkBuildingToTrigger
        -- args.building=lever_bld
        -- args.triggertarget=bld
        -- args.pre_actions={
        --     dfhack.curry(setFiltersUp,{items={{quantity=1,item_type=df.item_type.TRAPPARTS},{quantity=1,item_type=df.item_type.TRAPPARTS}}}),
        --     AssignJobItems,
        --     assign_link_refs,}
        -- args.post_actions={AssignBuildingRef,assign_link_roles}
        -- makeJob(args)
        local input_filter_defaults = { --stolen from buildings lua to better customize...
            item_type = df.item_type.TRAPPARTS,
            item_subtype = -1,
            mat_type = -1,
            mat_index = -1,
            flags1 = {},
            flags2 = { allow_artifact = true },
            flags3 = {},
            flags4 = 0,
            flags5 = 0,
            reaction_class = '',
            has_material_reaction_product = '',
            metal_ore = -1,
            min_dimension = -1,
            has_tool_use = -1,
            quantity = 1
        }
        local job_items={copyall(input_filter_defaults),copyall(input_filter_defaults)}
        local its=EnumItems_with_settings(args)
        local suitability=find_suitable_items(nil,its,job_items)
        require('hack.scripts.gui.advfort_items').jobitemEditor{items=suitability,job_items=job_items,on_okay=dfhack.curry(fake_linking,lever_bld,bld)}:show()
        lever_id=nil
    end
    --one item as LinkToTrigger role
    --one item as LinkToTarget
    --genref for holder(lever)
    --genref for triggertarget

end
--[[ Plant gathering attemped fix No. 35]] --[=[ still did not work!]=]
function get_design_block_ev(blk)
    for i,v in ipairs(blk.block_events) do
        if v:getType()==df.block_square_event_type.designation_priority then
            return v
        end
    end
end
function PlantGatherFix(args)
    local pos=args.pos
    --[[args.job.flags[17]=false --??

    
    local block=dfhack.maps.getTileBlock(pos)
    local ev=get_design_block_ev(block)
    if ev==nil then
        block.block_events:insert("#",{new=df.block_square_event_designation_priorityst})
        ev=block.block_events[#block.block_events-1]
    end
    ev.priority[pos.x % 16][pos.y % 16]=bit32.bor(ev.priority[pos.x % 16][pos.y % 16],4000)

    args.job.item_category:assign{furniture=true,corpses=true,ammo=true} --this is actually required in fort mode
    ]]
    local path=args.unit.path
    path.dest=pos
    path.goal=df.unit_path_goal.GatherPlant
    path.path.x:insert("#",pos.x)
    path.path.y:insert("#",pos.y)
    path.path.z:insert("#",pos.z)
    printall(path)
end
actions={
    {"CarveFortification"   ,df.job_type.CarveFortification,{IsWall,IsHardMaterial}},
    {"DetailWall"           ,df.job_type.DetailWall,{IsWall,IsHardMaterial}},
    {"DetailFloor"          ,df.job_type.DetailFloor,{IsFloor,IsHardMaterial,SameSquare}},
    {"CarveTrack"           ,df.job_type.CarveTrack,{} --TODO: check this- carving modifies standing tile but depends on direction!
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
    {"GatherPlants"         ,df.job_type.GatherPlants,{IsPlant,SameSquare},{PlantGatherFix}},
    {"RemoveConstruction"   ,df.job_type.RemoveConstruction,{IsConstruct}},
    {"RemoveBuilding"       ,RemoveBuilding,{IsBuilding}},
    {"RemoveStairs"         ,df.job_type.RemoveStairs,{IsStairs,NotConstruct}},
    --{"HandleLargeCreature"   ,df.job_type.HandleLargeCreature,{isUnit},{SetCreatureRef}},
    {"Build"                ,AssignJobToBuild,{NoConstructedBuilding}},
    {"BuildLast"                ,BuildLast,{NoConstructedBuilding}},
    {"Clean"                ,df.job_type.Clean,{}},
    {"GatherWebs"           ,df.job_type.CollectWebs,{--[[HasWeb]]},{SetWebRef}},
    {"Link Buildings"       ,LinkBuilding,{IsBuilding}},
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
    local ret
    if adv.job.current_job then
        ret= string.format("%s working(%d) ",(actions[(mode or 0)+1][1] or ""),adv.job.current_job.completion_timer)
    else
        ret= actions[(mode or 0)+1][1] or " "
    end
    if settings.quick then
        ret=ret.."*"
    end
    return ret
end

function usetool:update_site()
    local site=inSite()
    self.current_site=site
    local site_label=self.subviews.siteLabel
    if site then
        
        site_label:itemById("site").text=dfhack.TranslateName(site.name)
    else
        if settings.safe then
            site_label:itemById("site").text="<none, advfort disabled>"
        else
            site_label:itemById("site").text="<none, changes will not persist>"
        end
    end
end

function usetool:init(args)
    self:addviews{
        wid.Label{
            view_id="mainLabel",
            frame = {xalign=0,yalign=0},
            text={{key=keybinds.prevJob.key},{gap=1,text=self:callback("getModeName")},{gap=1,key=keybinds.nextJob.key},
                  }
                  },

        wid.Label{
            view_id="shopLabel",
            frame = {l=35,xalign=0,yalign=0},
            visible=false,
            text={
                {id="text1",gap=1,key=keybinds.workshop.key,key_sep="()", text="Workshop menu",pen=dfhack.pen.parse{fg=COLOR_YELLOW,bg=0}},{id="clutter"}}
                  },

        wid.Label{
            view_id="siteLabel",
            frame = {t=1,xalign=-1,yalign=0},
            text={
                {id="text1", text="Site:"},{id="site", text="name"}
                  }
            }
            }
    local labors=df.global.world.units.active[0].status.labors
    for i,v in ipairs(labors) do
        labors[i]=true
    end
    self:update_site()
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
    args.building=args.building or dfhack.buildings.findAtTile(args.pos)
    args.job_type=choice.job_id
    args.post_actions={AssignBuildingRef}
    args.pre_actions={dfhack.curry(setFiltersUp,choice.filter),AssignJobItems}
    makeJob(args)
end
function siegeWeaponActionChosen(args,actionid)
    local building=args.building
    if actionid==1 then --Turn
        building.facing=(args.building.facing+1)%4
        return
    elseif actionid==2 then --Load
        local action=df.job_type.LoadBallista
        if building:getSubtype()==df.siegeengine_type.Catapult then
            action=df.job_type.LoadCatapult
            args.pre_actions={dfhack.curry(setFiltersUp,{items={{quantity=1}}}),AssignJobItems} --TODO just boulders here
        else
            args.pre_actions={dfhack.curry(setFiltersUp,{items={{quantity=1,item_type=df.SIEGEAMMO}}}),AssignJobItems}
        end
        args.job_type=action
        args.unit=df.global.world.units.active[0]
        local from_pos={x=args.unit.pos.x,y=args.unit.pos.y, z=args.unit.pos.z}
        args.from_pos=from_pos
        args.pos=from_pos
    elseif actionid==3 then --Fire
        local action=df.job_type.FireBallista
        if building:getSubtype()==df.siegeengine_type.Catapult then
            action=df.job_type.FireCatapult
        end
        args.job_type=action
        args.unit=df.global.world.units.active[0]
        local from_pos={x=args.unit.pos.x,y=args.unit.pos.y, z=args.unit.pos.z}
        args.from_pos=from_pos
        args.pos=from_pos
    end
    args.post_actions={AssignBuildingRef}
    makeJob(args)
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
    local args={building=building,screen=self}
    dialog.showListPrompt("Engine job choice", "Choose what to do:",COLOR_WHITE,{"Turn","Load","Fire"},
        dfhack.curry(siegeWeaponActionChosen,args))
end
function usetool:onWorkShopButtonClicked(building,index,choice)
    local adv=df.global.world.units.active[0]
    local args={unit=adv,building=building}
    if df.interface_button_building_new_jobst:is_instance(choice.button) then
        choice.button:click()
        if #building.jobs>0 then
            local job=building.jobs[#building.jobs-1]
            args.job=job
            args.pos=adv.pos
            args.from_pos=adv.pos
            args.pre_actions={AssignJobItems}
            args.screen=self
            makeJob(args)
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
        local state={unit=adv,from_pos={x=adv.pos.x,y=adv.pos.y, z=adv.pos.z},building=building
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
function track_stop_configure(bld) --TODO: dedicated widget with nice interface and current setting display
    local dump_choices={
        {text="no dumping"},
        {text="N",x=0,y=-1},--{t="NE",x=1,y=-1},
        {text="E",x=1,y=0},--{t="SE",x=1,y=1},
        {text="S",x=0,y=1},--{t="SW",x=-1,y=1},
        {text="W",x=-1,y=0},--{t="NW",x=-1,y=-1}
    }
    local choices={"Friction","Dumping"}
    local function chosen(index,choice)
        if choice.text=="Friction" then
            dialog.showInputPrompt("Choose friction","Friction",nil,tostring(bld.friction),function ( txt )
                local num=tonumber(txt) --TODO allow only vanilla friction settings
                if num then
                    bld.friction=num
                end
            end)
        else
            dialog.showListPrompt("Dumping direction", "Choose dumping:",COLOR_WHITE,dump_choices,function ( index,choice)
                if choice.x then
                    bld.use_dump=1 --??
                    bld.dump_x_shift=choice.x
                    bld.dump_y_shift=choice.y
                else
                    bld.use_dump=0
                end
            end)
        end
    end
    dialog.showListPrompt("Track stop configure", "Choose what to change:",COLOR_WHITE,choices,chosen)
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
        local args={unit=adv,post_actions={AssignBuildingRef},pos=adv.pos,from_pos=adv.pos,
        building=building,job_type=df.job_type.CleanTrap}
        if building.trap_type==df.trap_type.CageTrap then
            args.job_type=df.job_type.LoadCageTrap
            local job_filter={items={{quantity=1,item_type=df.item_type.CAGE}} }
            args.pre_actions={dfhack.curry(setFiltersUp,job_filter),AssignJobItems}

        elseif building.trap_type==df.trap_type.StoneFallTrap then
            args.job_type=df.job_type.LoadStoneTrap
            local job_filter={items={{quantity=1,item_type=df.item_type.BOULDER}} }
            args.pre_actions={dfhack.curry(setFiltersUp,job_filter),AssignJobItems}
        elseif building.trap_type==df.trap_type.TrackStop then
            --set dump and friction
            track_stop_configure(building)
            return
        else
            print("TODO: trap type:"..df.trap_type[building.trap_type])
            return
        end
        args.screen=self
        makeJob(args)
    end
end
function usetool:hiveActions(building)
    local adv=df.global.world.units.active[0]
    local args={unit=adv,post_actions={AssignBuildingRef},pos=adv.pos,
    from_pos=adv.pos,job_type=df.job_type.InstallColonyInHive,building=building,screen=self}
    local job_filter={items={{quantity=1,item_type=df.item_type.VERMIN}} }
            args.pre_actions={dfhack.curry(setFiltersUp,job_filter),AssignJobItems}
    makeJob(args)
    --InstallColonyInHive,
    --CollectHiveProducts,
end
function usetool:operatePump(building)
    
    local adv=df.global.world.units.active[0]
    makeJob{unit=adv,post_actions={AssignBuildingRef},pos=adv.pos,from_pos=adv.pos,job_type=df.job_type.OperatePump,screen=self}
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
    
    local args={unit=adv,pos=adv.pos,from_pos=adv.pos,screen=self}
    if do_harvest then
        args.job_type=df.job_type.HarvestPlants
        args.post_actions={AssignBuildingRef}
    else
        local seedjob={items={{quantity=1,item_type=df.item_type.SEEDS}}}
        args.job_type=df.job_type.PlantSeeds
        args.pre_actions={dfhack.curry(setFiltersUp,seedjob)}
        args.post_actions={AssignBuildingRef,AssignJobItems}
    end

    makeJob(args)
end
function usetool:bedActions(building)
    local adv=df.global.world.units.active[0]
    local args={unit=adv,pos=adv.pos,from_pos=adv.pos,screen=self,building=building,
    job_type=df.job_type.Sleep,post_actions={AssignBuildingRef}}
    makeJob(args)
end
function usetool:chairActions(building)
    local adv=df.global.world.units.active[0]
    local eatjob={items={{quantity=1,item_type=df.item_type.FOOD}}}
    local args={unit=adv,pos=adv.pos,from_pos=adv.pos,screen=self,job_type=df.job_type.Eat,building=building,
        pre_actions={dfhack.curry(setFiltersUp,eatjob),AssignJobItems},post_actions={AssignBuildingRef}}
    makeJob(args)
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
    },
    [df.building_type.Bed]={
        name="Rest",
        input=usetool.bedActions,
    },
    [df.building_type.Chair]={
        name="Eat",
        input=usetool.chairActions,
    },
}
function usetool:shopMode(enable,mode,building)
    self.subviews.shopLabel.visible=enable
    if mode then
        self.subviews.shopLabel:itemById("text1").text=mode.name
        if building:getClutterLevel()<=1 then
            self.subviews.shopLabel:itemById("clutter").text=""
        else
            self.subviews.shopLabel:itemById("clutter").text=" Clutter:"..tostring(building:getClutterLevel())
        end
        self.building=building
    end
    self.mode=mode
end
function usetool:shopInput(keys)
    if keys[keybinds.workshop.key] then
        self:openShopWindowButtoned(self.in_shop)
    end
end
function usetool:wait_tick()
    self:sendInputToParent("A_SHORT_WAIT")
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
function usetool:siteCheck()
    if self.site ~= nil or not settings.safe then --TODO: add check if it's correct site (the persistant ones)
        return true
    end
    return false, "You are not on site"
end
--movement and co... Also passes on allowed keys
function usetool:fieldInput(keys)
    local adv=df.global.world.units.active[0]
    local cur_mode=actions[(mode or 0)+1]
    local failed=false
    for code,_ in pairs(keys) do
        --print(code)
        if MOVEMENT_KEYS[code] then

            local state={
                unit=adv,
                pos=moddedpos(adv.pos,MOVEMENT_KEYS[code]),
                dir=MOVEMENT_KEYS[code],
                from_pos={x=adv.pos.x,y=adv.pos.y, z=adv.pos.z},
                post_actions=cur_mode[4],
                pre_actions=cur_mode[5],
                job_type=cur_mode[2],
                screen=self}

            if code=="SELECT" then --do job in the distance, TODO: check if you can still cheat-mine (and co.) remotely
                if df.global.cursor.x~=-30000 then
                    state.pos={x=df.global.cursor.x,y=df.global.cursor.y,z=df.global.cursor.z}
                else
                    break
                end
            end
           
            --First check site
            local ok,msg=self:siteCheck() --TODO: some jobs might be possible without a site?
            if not ok then
                dfhack.gui.showAnnouncement(msg,5,1)
                failed=true
            else
                for _,p in pairs(cur_mode[3] or {}) do --then check predicates
                    local ok,msg=p(state)
                    if not ok then
                        dfhack.gui.showAnnouncement(msg,5,1)
                        failed=true
                    end
                end
            end
            
            if not failed then
                local ok,msg
                if type(cur_mode[2])=="function" then
                    ok,msg=cur_mode[2](state)
                else
                    makeJob(state)
                    --(adv,moddedpos(adv.pos,MOVEMENT_KEYS[code]),cur_mode[2],adv.pos,cur_mode[4])
                    
                end
                
                if code=="SELECT" then
                    self:sendInputToParent("LEAVESCREEN")
                end
                self.long_wait=true
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

    self:update_site()

    local adv=df.global.world.units.active[0]
    
    if keys.LEAVESCREEN  then
        if df.global.cursor.x~=-30000 then --if not poiting at anything
            self:sendInputToParent("LEAVESCREEN") --leave poiting
        else
            self:dismiss() --leave the adv-tools all together
            CancelJob(adv)
        end
    elseif keys[keybinds.nextJob.key] then --next job with looping
        mode=(mode+1)%#actions
    elseif keys[keybinds.prevJob.key] then --prev job with looping
        mode=mode-1
        if mode<0 then mode=#actions-1 end
    elseif keys["A_SHORT_WAIT"] then
        --ContinueJob(adv)
        self:sendInputToParent("A_SHORT_WAIT")
    elseif keys[keybinds.quick.key] then
        settings.quick=not settings.quick
    elseif keys[keybinds.continue.key] then
        --ContinueJob(adv)
        --self:sendInputToParent("A_SHORT_WAIT")
        self.long_wait=true
        self.long_wait_timer=nil
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
    
end
function usetool:cancel_wait()
    self.long_wait_timer=nil
    self.long_wait=false
end
function usetool:onIdle()
    local adv=df.global.world.units.active[0]
    local job_ptr=adv.job.current_job
    local job_action=findAction(adv,df.unit_action_type.Job)

    --some heuristics for unsafe conditions
    if self.long_wait and not settings.unsafe then --check if player wants for canceling to happen
        local counters=adv.counters
        local checked_counters={pain=true,winded=true,stunned=true,unconscious=true,suffocation=true,webbed=true,nausea=true,dizziness=true}
        for k,v in pairs(checked_counters) do
            if counters[k]>0 then
                dfhack.gui.showAnnouncement("Job: canceled waiting because unsafe -"..k,5,1)
                self:cancel_wait()
                return
            end
        end
    end

    if self.long_wait and self.long_wait_timer==nil then
        self.long_wait_timer=1000 --TODO tweak this
    end

    if job_ptr and self.long_wait and not job_action then

        if self.long_wait_timer<=0 then --fix deadlocks with force-canceling of waiting
            self:cancel_wait()
            return
        else
            self.long_wait_timer=self.long_wait_timer-1
        end

        if adv.job.current_job.completion_timer==-1  then
            self.long_wait=false
        end
        ContinueJob(adv)
        self:sendInputToParent("A_SHORT_WAIT") --todo continue till finished
    end
    self._native.parent:logic()
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
