--@module=true

local function get_translation(race_id)
    local race_name = df.global.world.raws.creatures.all[race_id].creature_id
    for _,translation in ipairs(df.global.world.raws.language.translations) do
        if translation.name == race_name then
            return translation
        end
    end
    return df.global.world.raws.language.translations[0]
end

local function pick_first_name(race_id)
    local translation = get_translation(race_id)
    return translation.words[math.random(0, #translation.words-1)].value
end

local LANGUAGE_IDX = 0
local word_table = df.global.world.raws.language.word_table[LANGUAGE_IDX][35]

function name_unit(unit)
    if unit.name.has_name then return end

    unit.name.first_name = pick_first_name(unit.race)
    unit.name.words.FrontCompound = word_table.words.FrontCompound[math.random(0, #word_table.words.FrontCompound-1)]
    unit.name.words.RearCompound = word_table.words.RearCompound[math.random(0, #word_table.words.RearCompound-1)]

    unit.name.language = LANGUAGE_IDX
    unit.name.parts_of_speech.FrontCompound = df.part_of_speech.Noun
    unit.name.parts_of_speech.RearCompound = df.part_of_speech.Verb3rdPerson
    unit.name.type = df.language_name_type.Figure
    unit.name.has_name = true
end

local function fix_clothing_ownership(unit)
    -- extracted/translated from tweak makeown plugin
    -- to be called by tweak-fixmigrant/makeown
    -- units forced into the fort by removing the flags do not own their clothes
    -- which has the result that they drop all their clothes and become unhappy because they are naked
    -- so we need to make them own their clothes and add them to their uniform
    local fixcount = 0    --int fixcount = 0;
    for j=0,#unit.inventory-1 do    --for(size_t j=0; j<unit->inventory.size(); j++)
        local inv_item = unit.inventory[j]    --unidf::unit_inventory_item* inv_item = unit->inventory[j];
        local item = inv_item.item    --df::item* item = inv_item->item;
        -- unforbid items (for the case of kidnapping caravan escorts who have their stuff forbidden by default)
        -- moved forbid false to inside if so that armor/weapons stay equiped
        if inv_item.mode == df.unit_inventory_item.T_mode.Worn then --if(inv_item->mode == df::unit_inventory_item::T_mode::Worn)
            -- ignore armor?
            -- it could be leather boots, for example, in which case it would not be nice to forbid ownership
            --if(item->getEffectiveArmorLevel() != 0)
            --    continue;

            if not dfhack.items.getOwner(item) then    --if(!Items::getOwner(item))
                if dfhack.items.setOwner(item,unit) then    --if(Items::setOwner(item, unit))
                    item.flags.forbid = false    --inv_item->item->flags.bits.forbid = 0;
                    -- add to uniform, so they know they should wear their clothes
                    unit.military.uniforms[0]:insert('#',item.id)    --insert_into_vector(unit->military.uniforms[0], item->id);
                    fixcount = fixcount + 1    --fixcount++;
                else
                    dfhack.printerr("makeown: could not change ownership for an item!")
                end
            end
        end
    end
    -- clear uniform_drop (without this they would drop their clothes and pick them up some time later)
    -- dirty?
    unit.military.uniform_drop:resize(0)    --unit->military.uniform_drop.clear();
    ----out << "ownership for " << fixcount << " clothes fixed" << endl;
    print("makeown: claimed ownership for "..tostring(fixcount).." worn items")
end

local function entity_link(hf, eid, do_event, add, replace_idx)
    do_event = (do_event == nil) and true or do_event
    add = (add == nil) and true or add
    replace_idx = replace_idx or -1

    local link = add and df.histfig_entity_link_memberst:new() or df.histfig_entity_link_former_memberst:new()
    link.entity_id = eid
    print("created entity link: "..tostring(eid))

    if replace_idx > -1 then
        local e = hf.entity_links[replace_idx]
        link.link_strength = (e.link_strength > 3) and (e.link_strength - 2) or e.link_strength
        hf.entity_links[replace_idx] = link -- replace member link with former member link
        e:delete()
        print("replaced entity link")
    else
        link.link_strength =  100
        hf.entity_links:insert('#', link)
    end

    if do_event then
        local event = add and df.history_event_add_hf_entity_linkst:new() or df.history_event_remove_hf_entity_linkst:new()
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
    hf.info.whereabouts.whereabouts_type = 1    -- state? arrived?
    hf.info.whereabouts.site = site_id
    local event = df.history_event_change_hf_statest:new()
    event.year = df.global.cur_year
    event.seconds = df.global.cur_year_tick
    event.id = df.global.hist_event_next_id
    event.hfid = hf.id
    event.state = 1
    event.reason = 13 --decided to stay on a whim i guess
    event.site = site_id
    event.region = -1
    event.layer = -1
    event.region_pos:assign(pos)
    df.global.world.history.events:insert('#',event)
    df.global.hist_event_next_id = df.global.hist_event_next_id + 1
end


function make_citizen(unit)
    local civ_id = df.global.plotinfo.civ_id                                                --get civ id
    local group_id = df.global.plotinfo.group_id                                            --get group id
    local site_id = df.global.plotinfo.site_id                                                --get site id

    local fortent = df.historical_entity.find(group_id)                                        --get fort's entity
    local civent = df.historical_entity.find(civ_id)

    local region_pos = df.world_site.find(site_id).pos -- used with state events and hf state

    local hf
    if unit.flags1.important_historical_figure or unit.flags2.important_historical_figure then    --if its a histfig
        hf = df.historical_figure.find(unit.hist_figure_id)                                        --then get the histfig
    end

    if not hf then                                                                                --if its not a histfig then make it a histfig
        --new_hf = true
        hf = df.new(df.historical_figure)
        hf.id = df.global.hist_figure_next_id
        df.global.hist_figure_next_id = df.global.hist_figure_next_id+1
        hf.profession = unit.profession
        hf.race = unit.race
        hf.caste = unit.caste
        hf.sex = unit.sex
        hf.appeared_year = df.global.cur_year
        hf.born_year = unit.birth_year
        hf.born_seconds = unit.birth_time
        hf.curse_year = unit.curse_year
        hf.curse_seconds = unit.curse_time
        hf.birth_year_bias=unit.birth_year_bias
        hf.birth_time_bias=unit.birth_time_bias
        hf.old_year = unit.old_year
        hf.old_seconds = unit.old_time
        hf.died_year = -1
        hf.died_seconds = -1
        hf.name:assign(unit.name)
        hf.civ_id = unit.civ_id
        hf.population_id  = unit.population_id
        hf.breed_id = -1
        hf.unit_id = unit.id

        --history_event_add_hf_entity_linkst not reported for civ on starting 7
        entity_link(hf, civ_id, false) -- so lets skip event here
        entity_link(hf, group_id)

        hf.info = df.historical_figure_info:new()
        hf.info.whereabouts = df.historical_figure_info.T_whereabouts:new()
        hf.info.whereabouts.region_id = -1;
        hf.info.whereabouts.underground_region_id = -1;
        hf.info.whereabouts.army_id = -1;
        hf.info.whereabouts.unk_1 = -1;
        hf.info.whereabouts.unk_2 = -1;
        change_state(hf, df.global.plotinfo.site_id, region_pos)


        --lets skip skills for now
        --local skills = df.historical_figure_info.T_skills:new() -- skills snap shot
        -- ...
        --info.skills = skills

        df.global.world.history.figures:insert('#', hf)

        --new_hf_loc = df.global.world.history.figures[#df.global.world.history.figures - 1]
        fortent.histfig_ids:insert('#', hf.id)
        fortent.hist_figures:insert('#', hf)
        civent.histfig_ids:insert('#', hf.id)
        civent.hist_figures:insert('#', hf)

        unit.flags1.important_historical_figure = true
        unit.flags2.important_historical_figure = true
        unit.hist_figure_id = hf.id
        unit.hist_figure_id2 = hf.id
        print("makeown: created historical figure: "..tostring(hf.id))
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

        if hf.info and hf.info.whereabouts then
            change_state(hf, df.global.plotinfo.site_id, region_pos)
            -- leave info nil if not found for now
        end

        if not found_civlink then    entity_link(hf,civ_id)        end
        if not found_fortlink then    entity_link(hf,group_id)    end

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
        print("makeown: migrated historical figure")
    end    -- hf

    local nemesis = dfhack.units.getNemesis(unit)
    if not nemesis then
        nemesis = df.nemesis_record:new()
        nemesis.figure = hf
        nemesis.unit = unit
        nemesis.unit_id = unit.id
        nemesis.save_file_id = civent.save_file_id
        nemesis.unk10, nemesis.unk11, nemesis.unk12 = -1, -1, -1
        --group_leader_id = -1
        nemesis.id = df.global.nemesis_next_id
        nemesis.member_idx = civent.next_member_idx
        civent.next_member_idx = civent.next_member_idx + 1

        df.global.world.nemesis.all:insert('#', nemesis)
        df.global.nemesis_next_id = df.global.nemesis_next_id + 1

        nemesis_link = df.general_ref_is_nemesisst:new()
        nemesis_link.nemesis_id = nemesis.id
        unit.general_refs:insert('#', nemesis_link)

        --new_nemesis_loc = df.global.world.nemesis.all[#df.global.world.nemesis.all - 1]
        fortent.nemesis_ids:insert('#', nemesis.id)
        fortent.nemesis:insert('#', nemesis)
        civent.nemesis_ids:insert('#', nemesis.id)
        civent.nemesis:insert('#', nemesis)
        print("makeown: created nemesis entry")
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
        print("makeown: migrated nemesis entry")
    end -- nemesis

    -- generate a name for the unit if it doesn't already have one
    name_unit(unit)
end

function make_own(unit)
    unit.flags1.marauder = false;
    unit.flags1.merchant = false;
    unit.flags1.forest = false;
    unit.flags1.diplomat = false;
    unit.flags1.active_invader = false;
    unit.flags1.hidden_in_ambush = false;
    unit.flags1.invader_origin = false;
    unit.flags1.hidden_ambusher = false;
    unit.flags1.invades = false;
    unit.flags2.underworld = false;        --or on a demon!
    unit.flags2.resident = false;
    unit.flags2.visitor_uninvited = false; --in case you use makeown on a beast :P
    unit.flags2.visitor = false;
    unit.flags3.guest = false;
    unit.flags4.invader_waits_for_parley = false;
    unit.flags4.agitated_wilderness_creature = false;

    unit.civ_id = df.global.plotinfo.civ_id;

    if  unit.profession == df.profession.MERCHANT then  unit.profession = df.profession.TRADER end
    if unit.profession2 == df.profession.MERCHANT then unit.profession2 = df.profession.TRADER end

    fix_clothing_ownership(unit)

    local caste_flags = unit.enemy.caste_flags
    if caste_flags.CAN_SPEAK or caste_flags.CAN_LEARN then
        make_citizen(unit)
    end
end

if dfhack_flags.module then
    return
end

unit = dfhack.gui.getSelectedUnit()
if not unit then
    qerror('No unit selected!')
else
    make_own(unit)
end
