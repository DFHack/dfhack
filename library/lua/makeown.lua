local _ENV = mkmodule('makeown')
--[[
    'tweak makeown' as a lua include
    make_own(unit)		-- removes foreign flags, sets civ_id to fort civ_id, and sets clothes ownership
	make_citizen(unit)	-- called by make_own if unit.race == fort race
	
	eventually ought to migrate to hack/lua/plugins/tweak.lua
	and local _ENV = mkmodule('plugin.tweak')
	in order to link to functions in the compiled plugin (when/if they become available to lua)
--]]
local utils = require 'utils'


local function fix_clothing_ownership(unit)
	-- extracted/translated from tweak makeown plugin
	-- to be called by tweak-fixmigrant/makeown
	-- units forced into the fort by removing the flags do not own their clothes
	-- which has the result that they drop all their clothes and become unhappy because they are naked
	-- so we need to make them own their clothes and add them to their uniform
	local fixcount = 0	--int fixcount = 0;
	for j=0,#unit.inventory-1 do	--for(size_t j=0; j<unit->inventory.size(); j++)
        local inv_item = unit.inventory[j]	--unidf::unit_inventory_item* inv_item = unit->inventory[j];
        local item = inv_item.item	--df::item* item = inv_item->item;
		-- unforbid items (for the case of kidnapping caravan escorts who have their stuff forbidden by default)
		-- moved forbid false to inside if so that armor/weapons stay equiped
        if inv_item.mode == df.unit_inventory_item.T_mode.Worn then --if(inv_item->mode == df::unit_inventory_item::T_mode::Worn)
			-- ignore armor?
			-- it could be leather boots, for example, in which case it would not be nice to forbid ownership
			--if(item->getEffectiveArmorLevel() != 0)
			--    continue;

			if not dfhack.items.getOwner(item) then	--if(!Items::getOwner(item))
				if dfhack.items.setOwner(item,unit) then	--if(Items::setOwner(item, unit))
					item.flags.forbid = false	--inv_item->item->flags.bits.forbid = 0;
					-- add to uniform, so they know they should wear their clothes
                    unit.military.uniforms[0]:insert('#',item.id)	--insert_into_vector(unit->military.uniforms[0], item->id);
                    fixcount = fixcount + 1	--fixcount++;
                else
					----out << "could not change ownership for item!" << endl;
					print("Makeown: could not change ownership for an item!")
				end
			end
		end
	end
	-- clear uniform_drop (without this they would drop their clothes and pick them up some time later)
	-- dirty?
	unit.military.uniform_drop:resize(0)	--unit->military.uniform_drop.clear();
	----out << "ownership for " << fixcount << " clothes fixed" << endl;
	print("Makeown: claimed ownership for "..tostring(fixcount).." worn items")
	--return true	--return CR_OK;
end

local function entity_link(hf, eid, do_event, add, replace_idx)
	do_event = (do_event == nil) and true or do_event
	add = (add == nil) and true or add
	replace_idx = replace_idx or -1
	
	local link = add and df.histfig_entity_link_memberst:new() or df.histfig_entity_link_former_memberst:new()
	link.entity_id = eid
	if replace_idx > -1 then
		local e = hf.entity_links[replace_idx]
		link.link_strength = (e.link_strength > 3) and (e.link_strength - 2) or e.link_strength
		hf.entity_links[replace_idx] = link -- replace member link with former member link
		e:delete()
	else
		link.link_strength =  100
		hf.entity_links:insert('#', link)
	end
	if do_event then
		event = add and df.history_event_add_hf_entity_linkst:new() or df.history_event_remove_hf_entity_linkst:new()
		event.year = df.global.cur_year
		event.seconds = df.global.cur_year_tick
		event.civ = eid
		event.histfig = hf.id
		event.link_type = 0
		event.position_id = -1
		event.id = df.global.hist_event_next_id
		df.global.world.history.events:insert('#',event)
		df.global.hist_event_next_id = df.global.hist_event_next_id + 1
	end
end

local function change_state(hf, site_id, pos)
	hf.info.unk_14.unk_0 = 3	-- state? arrived?
	hf.info.unk_14.region:assign(pos)
	hf.info.unk_14.site = site_id
	event = df.history_event_change_hf_statest:new()
	event.year = df.global.cur_year
	event.seconds = df.global.cur_year_tick
	event.hfid = hf.id
	event.state = 3
	event.site = site_id
	event.region_pos:assign(pos)
	event.substate = -1; event.region = -1; event.layer = -1;
	event.id = df.global.hist_event_next_id
	df.global.world.history.events:insert('#',event)
	df.global.hist_event_next_id = df.global.hist_event_next_id + 1
end


function make_citizen(unit)
	local dfg = df.global
	local civ_id = dfg.ui.civ_id
	local group_id = dfg.ui.group_id
	local events = dfg.world.history.events
	local fortent = dfg.ui.main.fortress_entity
	local civent = fortent and df.historical_entity.find(fortent.entity_links[0].target)
		-- utils.binsearch(dfg.world.entities.all, fortent.entity_links[0].target, 'id')
	local event
	local region_pos = df.world_site.find(dfg.ui.site_id).pos -- used with state events and hf state
	
	local hf
	-- assume that hf id 1 and hf id 2 are equal.  I am unaware of instances of when they are not.
	-- occationally a unit does not have both flags set (missing flags1.important_historical_figure)
	-- and I don't know what that means yet.
	if unit.flags1.important_historical_figure and unit.flags2.important_historical_figure then
		-- aready hf, find it (unlikely to happen)
		hf = utils.binsearch(dfg.world.history.figures, unit.hist_figure_id, 'id')
	--elseif unit.flags1.important_historical_figure or unit.flags2.important_historical_figure then
		-- something wrong, try to fix it?
		--[[
		if unit.hist_figure_id == -1 then
			unit.hist_figure_id = unit.hist_figure_id2
		end
		if unit.hist_figure_id > -1 then
			unit.hist_figure_id2 = unit.hist_figure_id
			unit.flags1.important_historical_figure = true
			unit.flags2.important_historical_figure = true
			hf = utils.binsearch(dfg.world.history.figures, unit.hist_figure_id, 'id')
		else
			unit.flags1.important_historical_figure = false
			unit.flags2.important_historical_figure = false
		end
		--]]
	--else
		-- make one
	end
	--local new_hf = false
	if not hf then
		--new_hf = true
		hf = df.historical_figure:new()
		hf.profession = unit.profession
		hf.race = unit.race
		hf.caste = unit.caste
		hf.sex = unit.sex
		hf.appeared_year = dfg.cur_year
		hf.born_year = unit.relations.birth_year
		hf.born_seconds = unit.relations.birth_time
		hf.curse_year = unit.relations.curse_year
		hf.curse_seconds = unit.relations.curse_time
		hf.anon_1 = unit.relations.anon_2
		hf.anon_2 = unit.relations.anon_3
		hf.old_year = unit.relations.old_year
		hf.old_seconds = unit.relations.old_time
		hf.died_year = -1
		hf.died_seconds = -1
		hf.name:assign(unit.name)
		hf.civ_id = unit.civ_id
		hf.population_id  = unit.population_id
		hf.breed_id = -1
		hf.unit_id = unit.id
		hf.id = dfg.hist_figure_next_id -- id must be set before adding links (for the events)
		
		--history_event_add_hf_entity_linkst not reported for civ on starting 7
		entity_link(hf, civ_id, false) -- so lets skip event here
		entity_link(hf, group_id)
		
		hf.info = df.historical_figure_info:new()
		hf.info.unk_14 = df.historical_figure_info.T_unk_14:new() -- hf state?
		--unk_14.region_id = -1; unk_14.beast_id = -1; unk_14.unk_14 = 0
		hf.info.unk_14.unk_18 = -1; hf.info.unk_14.unk_1c = -1
		-- set values that seem related to state and do event
		change_state(hf, dfg.ui.site_id, region_pos) 
		
		
		--lets skip skills for now
		--local skills = df.historical_figure_info.T_skills:new() -- skills snap shot
		-- ...
		--info.skills = skills
		
		dfg.world.history.figures:insert('#', hf)
		dfg.hist_figure_next_id = dfg.hist_figure_next_id + 1
		
		--new_hf_loc = df.global.world.history.figures[#df.global.world.history.figures - 1]
		fortent.histfig_ids:insert('#', hf.id)
		fortent.hist_figures:insert('#', hf)
		civent.histfig_ids:insert('#', hf.id)
		civent.hist_figures:insert('#', hf)
		
		unit.flags1.important_historical_figure = true
		unit.flags2.important_historical_figure = true
		unit.hist_figure_id = hf.id
		unit.hist_figure_id2 = hf.id
		print("Makeown-citizen: created historical figure")
	else
		-- only insert into civ/fort if not already there
		-- Migrants change previous histfig_entity_link_memberst to histfig_entity_link_former_memberst 
		-- for group entities, add link_member for new group, and reports events for remove from group, 
		-- remove from civ, change state, add civ, and add group
		
		hf.civ_id = civ_id -- ensure current civ_id

		local found_civlink = false
		local found_fortlink = false
		local v = hf.entity_links
		for k=#v-1,0,-1 do
			if df.histfig_entity_link_memberst:is_instance(v[k]) then
				entity_link(hf, v[k].entity_id, true, false, k)
			end
		end
		
		if hf.info and hf.info.unk_14 then
			change_state(hf, dfg.ui.site_id, region_pos)
			-- leave info nil if not found for now
		end
		
		if not found_civlink then	entity_link(hf,civ_id)		end
		if not found_fortlink then	entity_link(hf,group_id)	end
		
		--change entity_links
		local found = false
		for _,v in ipairs(civent.histfig_ids) do
			if v == hf.id then found = true; break end
		end
		if not found then
			civent.histfig_ids:insert('#', hf.id)
			civent.hist_figures:insert('#', hf)
		end
		found = false
		for _,v in ipairs(fortent.histfig_ids) do
			if v == hf.id then found = true; break end
		end
		if not found then
			fortent.histfig_ids:insert('#', hf.id)
			fortent.hist_figures:insert('#', hf)
		end
		print("Makeown-citizen: migrated historical figure")
	end	-- hf

	local nemesis = dfhack.units.getNemesis(unit)
	if not nemesis then
		nemesis = df.nemesis_record:new()
		nemesis.figure = hf
		nemesis.unit = unit
		nemesis.unit_id = unit.id
		nemesis.save_file_id = civent.save_file_id
		nemesis.unk10, nemesis.unk11, nemesis.unk12 = -1, -1, -1
		--group_leader_id = -1
		nemesis.id = dfg.nemesis_next_id
		nemesis.member_idx = civent.next_member_idx
		civent.next_member_idx = civent.next_member_idx + 1

		dfg.world.nemesis.all:insert('#', nemesis)
		dfg.nemesis_next_id = dfg.nemesis_next_id + 1

		nemesis_link = df.general_ref_is_nemesisst:new()
		nemesis_link.nemesis_id = nemesis.id
		unit.general_refs:insert('#', nemesis_link)
		
		--new_nemesis_loc = df.global.world.nemesis.all[#df.global.world.nemesis.all - 1]
		fortent.nemesis_ids:insert('#', nemesis.id)
		fortent.nemesis:insert('#', nemesis)
		civent.nemesis_ids:insert('#', nemesis.id)
		civent.nemesis:insert('#', nemesis)
		print("Makeown-citizen: created nemesis entry")
	else-- only insert into civ/fort if not already there
		local found = false
		for _,v in ipairs(civent.nemesis_ids) do
			if v == nemesis.id then found = true; break end
		end
		if not found then
			civent.nemesis_ids:insert('#', nemesis.id)
			civent.nemesis:insert('#', nemesis)
		end
		found = false
		for _,v in ipairs(fortent.nemesis_ids) do
			if v == nemesis.id then found = true; break end
		end
		if not found then
			fortent.nemesis_ids:insert('#', nemesis.id)
			fortent.nemesis:insert('#', nemesis)
		end
		print("Makeown-citizen: migrated nemesis entry")
	end -- nemesis
end


function make_own(unit)
	--tweak makeown
	unit.flags2.resident = false; unit.flags1.merchant = false; unit.flags1.forest = false;
	unit.civ_id = df.global.ui.civ_id
	if unit.profession == df.profession.MERCHANT then unit.profession = df.profession.TRADER end
	if unit.profession2 == df.profession.MERCHANT then unit.profession2 = df.profession.TRADER end
	fix_clothing_ownership(unit)
	if unit.race == df.global.ui.race_id then
		make_citizen(unit)
	end
end



return _ENV