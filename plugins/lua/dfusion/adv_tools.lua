local _ENV = mkmodule('plugins.dfusion.adv_tools')
local dfu=require("plugins.dfusion")
local tools=require("plugins.dfusion.tools")
menu=dfu.SimpleMenu()
function Reincarnate(trg_unit,swap_soul) --only for adventurer i guess
	if swap_soul==nil then
		swap_soul=true
	end
	local adv=trg_unit or df.global.world.units.active[0]
	if adv.flags1.dead==false then
		qerror("You are not dead (yet)!")
	end
	local hist_fig=dfhack.units.getNemesis(adv).figure
	if hist_fig==nil then
		qerror("No historical figure for adventurer...")
	end
	local events=df.global.world.history.events
	local trg_hist_fig
	for i=#events-1,0,-1 do -- reverse search because almost always it will be last entry
		if df.history_event_hist_figure_diedst:is_instance(events[i]) then
			--print("is instance:"..i)
			if events[i].victim_hf==hist_fig.id then
				--print("Is same id:"..i)
				trg_hist_fig=events[i].slayer_hf
				if trg_hist_fig then
					trg_hist_fig=df.historical_figure.find(trg_hist_fig)
				end
				break
			end
		end
	end
	if trg_hist_fig ==nil then
		qerror("Slayer not found")
	end
	
	local trg_unit=trg_hist_fig.unit_id
	if trg_unit==nil then
		qerror("Unit id not found!")
	end
	local trg_unit_final=df.unit.find(trg_unit)
	
	change_adv(trg_unit_final)
	if swap_soul then --actually add a soul...
		t_soul=adv.status.current_soul
		adv.status.current_soul=df.NULL
		adv.status.souls:resize(0)
		trg_unit_final.status.current_soul=t_soul
		trg_unit_final.status.souls:insert(#trg_unit_final.status.souls,t_soul)
	end
end
menu:add("Reincarnate",Reincarnate,{{df.unit,"optional"}})-- bool, optional
function change_adv(unit,nemesis)
	if nemesis==nil then
		nemesis=true --default value is nemesis switch too.
	end
	if unit==nil then
		unit=dfhack.gui.getSelectedUnit()--getCreatureAtPointer()
	end
	if unit==nil then
		error("Invalid unit!")
	end
	local other=df.global.world.units.active
	local unit_indx
	for k,v in pairs(other) do
		if v==unit then
			unit_indx=k
			break
		end
	end
	if unit_indx==nil then
		error("Unit not found in array?!") --should not happen
	end
	other[unit_indx]=other[0]
	other[0]=unit
	if nemesis then --basicly copied from advtools plugin...
		local nem=dfhack.units.getNemesis(unit)
		local other_nem=dfhack.units.getNemesis(other[unit_indx])
		if other_nem then
			other_nem.flags[0]=false
			other_nem.flags[1]=true
		end
		if nem then
			nem.flags[0]=true
			nem.flags[2]=true
			for k,v in pairs(df.global.world.nemesis.all) do
				if v.id==nem.id then
					df.global.ui_advmode.player_id=k
				end
			end
		else
			qerror("Current unit does not have nemesis record, further working not guaranteed")
		end
	end
end
menu:add("Change adventurer",change_adv)
function log_pos()
    local adv=df.global.world.units.active[0]
    
    local wmap=df.global.world.map
    local sub_pos={x=adv.pos.x,y=adv.pos.y,z=adv.pos.z}
    local region_pos={x=wmap.region_x,y=wmap.region_y,z=wmap.region_z}
    local pos={x=sub_pos.x+region_pos.x*48,y=sub_pos.y+region_pos.y*48,z=sub_pos.z+region_pos.z}
    local state
    if adv.flags1.dead then
        state="dead"
    else
        state="live n kicking"
    end
    local message=string.format("%s %s at pos={%d,%d,%d} region={%d,%d,%d}",dfhack.TranslateName(adv.name),state,pos.x,pos.y,pos.z,region_pos.x,region_pos.y,region_pos.z)
    print(message)
    local path="deaths_"..df.global.world.cur_savegame.save_dir..".txt"
    local f=io.open(path,"a")
    f:write(message)
    f:close()
end
menu:add("Log adventurers position",log_pos)
function addSite(x,y,rgn_max_x,rgn_min_x,rgn_max_y,rgn_min_y,civ_id,name,sitetype)
    if x==nil or y==nil then
        x=(df.global.world.map.region_x+1)/16
        y=(df.global.world.map.region_y+1)/16
    end
    if name==nil then
        name=dfhack.lineedit("Site name:")or "Hacked site"
    end
    if sitetype==nil then
        sitetype=tonumber(dfhack.lineedit("Site type (numeric):")) or 7
    end
    rgn_max_x=rgn_max_x or (df.global.world.map.region_x+1)%16
    rgn_max_y=rgn_max_y or (df.global.world.map.region_y+1)%16
    rgn_min_y=rgn_min_y or rgn_max_y
    rgn_min_x=rgn_min_x or rgn_max_x
    print("Region:",rgn_max_x,rgn_min_x,rgn_max_y,rgn_min_y)
--[=[
<angavrilov> global = pos*16 + rgn
<angavrilov> BUT
<angavrilov> for cities global is usually 17x17, i.e. max size
<angavrilov> while rgn designates a small bit in the middle
<angavrilov> for stuff like forts that formula holds exactly
]=]--
    local wd=df.global.world.world_data
    local nsite=df.world_site:new()
    nsite.name.first_name=name
    nsite.name.has_name=true
    nsite.pos:assign{x=x,y=y}
    nsite.rgn_max_x=rgn_max_x
    nsite.rgn_min_x=rgn_min_x
    nsite.rgn_min_y=rgn_min_y
    nsite.rgn_max_y=rgn_max_y
    nsite.global_max_x=nsite.pos.x*16+nsite.rgn_max_x
    nsite.global_min_x=nsite.pos.x*16+nsite.rgn_min_x
    nsite.global_max_y=nsite.pos.y*16+nsite.rgn_max_y
    nsite.global_min_y=nsite.pos.y*16+nsite.rgn_min_y
    nsite.id=wd.next_site_id
    nsite.civ_id=civ_id or -1
    nsite.cur_owner_id=civ_id or -1
    nsite.type=sitetype --lair = 7
    nsite.flags:resize(23)
    --nsite.flags[4]=true
    --nsite.flags[5]=true
    --nsite.flags[6]=true
    nsite.index=#wd.sites+1
    wd.sites:insert("#",nsite)
    wd.next_site_id=wd.next_site_id+1
    --might not be needed...
    --[[local unk130=df.world_site_unk130:new()
    unk130.index=#wd.site_unk130+1
    wd.site_unk130:insert("#",unk130)
    --wd.next_site_unk136_id=wd.next_site_unk136_id+1--]]
    return nsite
end
menu:add("Create site at current location",addSite)
return _ENV
