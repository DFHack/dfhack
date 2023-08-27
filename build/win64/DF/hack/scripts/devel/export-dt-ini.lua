-- Exports an ini file for Dwarf Therapist.
--[====[
devel/export-dt-ini
===================
Exports an ini file containing memory addresses for Dwarf Therapist.
]====]

local utils = require 'utils'
local ms = require 'memscan'

-- Utility functions

local globals = df.global
local global_addr = dfhack.internal.getAddress
local os_type = dfhack.getOSType()
local rdelta = dfhack.internal.getRebaseDelta()
local lines = {} --as:string[]
local complete = true

local function header(name)
    table.insert(lines, '')
    table.insert(lines, '['..name..']')
end

local function value(name,addr)
    local line

    if not addr then
        complete = false
        line = name..'=0x0'
    elseif addr < 0x10000 then
        line = string.format('%s=0x%04x',name,addr)
    else
        line = string.format('%s=0x%08x',name,addr)
    end

    table.insert(lines, line)
end
local function address(name,base,field,...)
    local addr

    if base == globals then
        addr = global_addr(field)
        if addr and select('#',...) > 0 then
            _,addr = df.sizeof(ms.field_ref(base,field,...))
        end
        addr = addr - rdelta
    elseif base._kind == 'class-type' then
        -- field_offset crashes with classes due to vtable problems,
        -- so we have to create a real temporary object here.
        local obj = df.new(base)
        if obj then
            local _,a1 = df.sizeof(obj)
            local _,a2 = df.sizeof(ms.field_ref(obj,field,...))
            addr = a2-a1
            obj:delete()
        end
    else
        addr = ms.field_offset(base,field,...)
    end

    value(name, addr)
end
local function vtable(name,class)
    local addr = dfhack.internal.getVTable(class)
    addr = addr - rdelta
    value(name, addr)
end


-- List of actual values
header('addresses')
address('cur_year_tick',globals,'cur_year_tick')
address('current_year',globals,'cur_year')
address('dwarf_civ_index',globals,'plotinfo','civ_id')
address('dwarf_race_index',globals,'plotinfo','race_id')
address('fortress_entity',globals,'plotinfo','main','fortress_entity')
address('historical_entities_vector',globals,'world','entities','all')
address('creature_vector',globals,'world','units','all')
address('active_creature_vector',globals,'world','units','active')
address('weapons_vector',globals,'world','items','other','WEAPON')
address('shields_vector',globals,'world','items','other', 'SHIELD')
address('quivers_vector',globals,'world','items','other', 'QUIVER')
address('crutches_vector',globals,'world','items','other', 'CRUTCH')
address('backpacks_vector',globals,'world','items','other', 'BACKPACK')
address('ammo_vector',globals,'world','items','other', 'AMMO')
address('flasks_vector',globals,'world','items','other', 'FLASK')
address('pants_vector',globals,'world','items','other', 'PANTS')
address('armor_vector',globals,'world','items','other', 'ARMOR')
address('shoes_vector',globals,'world','items','other', 'SHOES')
address('helms_vector',globals,'world','items','other', 'HELM')
address('gloves_vector',globals,'world','items','other', 'GLOVES')
address('artifacts_vector',globals,'world','artifacts','all')
address('squad_vector',globals,'world','squads','all')
address('activities_vector',globals,'world','activities','all')
address('fake_identities_vector',globals,'world','identities','all')
address('poetic_forms_vector',globals,'world','poetic_forms','all')
address('musical_forms_vector',globals,'world','musical_forms','all')
address('dance_forms_vector',globals,'world','dance_forms','all')
address('occupations_vector',globals,'world','occupations','all')
address('world_data',globals,'world','world_data')
address('material_templates_vector',globals,'world','raws','material_templates')
address('inorganics_vector',globals,'world','raws','inorganics')
address('plants_vector',globals,'world','raws','plants','all')
address('races_vector',globals,'world','raws','creatures','all')
address('itemdef_weapons_vector',globals,'world','raws','itemdefs','weapons')
address('itemdef_trap_vector',globals,'world','raws','itemdefs','trapcomps')
address('itemdef_toy_vector',globals,'world','raws','itemdefs','toys')
address('itemdef_tool_vector',globals,'world','raws','itemdefs','tools')
address('itemdef_instrument_vector',globals,'world','raws','itemdefs','instruments')
address('itemdef_armor_vector',globals,'world','raws','itemdefs','armor')
address('itemdef_ammo_vector',globals,'world','raws','itemdefs','ammo')
address('itemdef_siegeammo_vector',globals,'world','raws','itemdefs','siege_ammo')
address('itemdef_glove_vector',globals,'world','raws','itemdefs','gloves')
address('itemdef_shoe_vector',globals,'world','raws','itemdefs','shoes')
address('itemdef_shield_vector',globals,'world','raws','itemdefs','shields')
address('itemdef_helm_vector',globals,'world','raws','itemdefs','helms')
address('itemdef_pant_vector',globals,'world','raws','itemdefs','pants')
address('itemdef_food_vector',globals,'world','raws','itemdefs','food')
address('language_vector',globals,'world','raws','language','words')
address('translation_vector',globals,'world','raws','language','translations')
address('colors_vector',globals,'world','raws','descriptors','colors')
address('shapes_vector',globals,'world','raws','descriptors','shapes')
address('reactions_vector',globals,'world','raws','reactions')
address('base_materials',globals,'world','raws','mat_table','builtin')
address('all_syndromes_vector',globals,'world','raws','syndromes','all')
address('events_vector',globals,'world','history','events')
address('historical_figures_vector',globals,'world','history','figures')
address('world_site_type',df.world_site,'type')
address('active_sites_vector',df.world_data,'active_site')
address('gview',globals,'gview')
address('external_flag',globals,'game','external_flag')
vtable('viewscreen_setupdwarfgame_vtable','viewscreen_setupdwarfgamest')

header('offsets')
address('word_table',df.language_translation,'words')

header('word_offsets')
address('base',df.language_word,'word')
address('noun_singular',df.language_word,'forms','Noun')
address('noun_plural',df.language_word,'forms','NounPlural')
address('adjective',df.language_word,'forms','Adjective')
address('verb',df.language_word,'forms','Verb')
address('present_simple_verb',df.language_word,'forms','Verb3rdPerson')
address('past_simple_verb',df.language_word,'forms','VerbPast')
address('past_participle_verb',df.language_word,'forms','VerbPassive')
address('present_participle_verb',df.language_word,'forms','VerbGerund')
address('first_name',df.language_name,'first_name')
address('nickname',df.language_name,'nickname')
address('words',df.language_name,'words')
address('word_type',df.language_name,'parts_of_speech')
address('language_id',df.language_name,'language')

header('general_ref_offsets')
-- general_ref::getType is the third method in the vtable
value('ref_type',2*(dfhack.getArchitecture()/8))
address('artifact_id',df.general_ref_artifact,'artifact_id')
address('item_id',df.general_ref_item,'item_id')

header('race_offsets')
address('name_singular',df.creature_raw,'name',0)
address('name_plural',df.creature_raw,'name',1)
address('adjective',df.creature_raw,'name',2)
address('baby_name_singular',df.creature_raw,'general_baby_name',0)
address('baby_name_plural',df.creature_raw,'general_baby_name',1)
address('child_name_singular',df.creature_raw,'general_child_name',0)
address('child_name_plural',df.creature_raw,'general_child_name',1)
address('pref_string_vector',df.creature_raw,'prefstring')
address('castes_vector',df.creature_raw,'caste')
address('pop_ratio_vector',df.creature_raw,'pop_ratio')
address('materials_vector',df.creature_raw,'material')
address('flags',df.creature_raw,'flags')
address('tissues_vector',df.creature_raw,'tissue')

header('caste_offsets')
address('caste_name',df.caste_raw,'caste_name')
address('caste_descr',df.caste_raw,'description')
address('caste_trait_ranges',df.caste_raw,'personality','a')
address('caste_phys_att_ranges',df.caste_raw,'attributes','phys_att_range')
address('baby_age',df.caste_raw,'misc','baby_age')
address('child_age',df.caste_raw,'misc','child_age')
address('adult_size',df.caste_raw,'misc','adult_size')
address('flags',df.caste_raw,'flags')
address('body_info',df.caste_raw,'body_info')
address('skill_rates',df.caste_raw,'skill_rates')
address('caste_att_rates',df.caste_raw,'attributes','phys_att_rates')
address('caste_att_caps',df.caste_raw,'attributes','phys_att_cap_perc')
address('shearable_tissues_vector',df.caste_raw,'shearable_tissue_layer')
address('extracts',df.caste_raw,'extracts','extract_matidx')

header('hist_entity_offsets')
address('histfigs',df.historical_entity,'histfig_ids')
address('beliefs',df.historical_entity,'resources','values')
address('squads',df.historical_entity,'squads')
address('positions',df.historical_entity,'positions','own')
address('assignments',df.historical_entity,'positions','assignments')
address('assign_hist_id',df.entity_position_assignment,'histfig')
address('assign_position_id',df.entity_position_assignment,'position_id')
address('position_id',df.entity_position,'id')
address('position_name',df.entity_position,'name')
address('position_female_name',df.entity_position,'name_female')
address('position_male_name',df.entity_position,'name_male')

header('hist_figure_offsets')
address('hist_race',df.historical_figure,'race')
address('hist_name',df.historical_figure,'name')
address('id',df.historical_figure,'id')
address('hist_fig_info',df.historical_figure,'info')
address('reputation',df.historical_figure_info,'reputation')
address('current_ident',df.historical_figure_info.T_reputation,'cur_identity')
address('fake_name',df.identity,'name')
address('fake_birth_year',df.identity,'birth_year')
address('fake_birth_time',df.identity,'birth_second')
address('kills',df.historical_figure_info,'kills')
address('killed_race_vector',df.historical_kills,'killed_race')
address('killed_undead_vector',df.historical_kills,'killed_undead')
address('killed_counts_vector',df.historical_kills,'killed_count')

header('hist_event_offsets')
address('event_year',df.history_event,'year')
address('id',df.history_event,'id')
address('killed_hist_id',df.history_event_hist_figure_diedst,'victim_hf')

header('item_offsets')
address('item_def',df.item_ammost,'subtype') --currently same for all
address('id',df.item,'id')
address('general_refs',df.item,'general_refs')
address('stack_size',df.item_actual,'stack_size')
address('wear',df.item_actual,'wear')
address('mat_type',df.item_constructed,'mat_type') -- should be item_crafted
address('mat_index',df.item_constructed,'mat_index') -- should be item_crafted
address('maker_race',df.item_constructed,'maker_race') -- should be item_crafted
address('quality',df.item_constructed,'quality') -- should be item_crafted
address('artifact_id',df.artifact_record,'id')
address('artifact_name',df.artifact_record,'name')

header('item_subtype_offsets')
address('sub_type',df.itemdef,'subtype')
address('base_flags',df.itemdef,'base_flags')
address('name',df.itemdef_armorst,'name')
address('name_plural',df.itemdef_armorst,'name_plural')
address('adjective',df.itemdef_armorst,'name_preplural')
address('tool_flags',df.itemdef_toolst,'flags')
address('tool_adjective',df.itemdef_toolst,'adjective')

header('item_filter_offsets')
address('item_subtype',df.item_filter_spec,'item_subtype')
address('mat_class',df.item_filter_spec,'material_class')
address('mat_type',df.item_filter_spec,'mattype')
address('mat_index',df.item_filter_spec,'matindex')

header('weapon_subtype_offsets')
address('single_size',df.itemdef_weaponst,'two_handed')
address('multi_size',df.itemdef_weaponst,'minimum_size')
address('ammo',df.itemdef_weaponst,'ranged_ammo')
address('melee_skill',df.itemdef_weaponst,'skill_melee')
address('ranged_skill',df.itemdef_weaponst,'skill_ranged')

header('armor_subtype_offsets')
address('layer',df.armor_properties,'layer')
address('mat_name',df.itemdef_armorst,'material_placeholder')
address('other_armor_level',df.itemdef_helmst,'armorlevel')
address('armor_adjective',df.itemdef_armorst,'adjective')
address('armor_level',df.itemdef_armorst,'armorlevel')
address('chest_armor_properties',df.itemdef_armorst,'props')
address('pants_armor_properties',df.itemdef_pantsst,'props')
address('other_armor_properties',df.itemdef_helmst,'props')

header('material_offsets')
address('solid_name',df.material_common,'state_name','Solid')
address('liquid_name',df.material_common,'state_name','Liquid')
address('gas_name',df.material_common,'state_name','Gas')
address('powder_name',df.material_common,'state_name','Powder')
address('paste_name',df.material_common,'state_name','Paste')
address('pressed_name',df.material_common,'state_name','Pressed')
address('flags',df.material_common,'flags')
address('reaction_class',df.material_common,'reaction_class')
address('prefix',df.material,'prefix')
address('inorganic_materials_vector',df.inorganic_raw,'material')
address('inorganic_flags',df.inorganic_raw,'flags')

header('plant_offsets')
address('name',df.plant_raw,'name')
address('name_plural',df.plant_raw,'name_plural')
address('name_leaf_plural',df.plant_raw,'leaves_plural')
address('name_seed_plural',df.plant_raw,'seed_plural')
address('materials_vector',df.plant_raw,'material')
address('flags',df.plant_raw,'flags')

header('descriptor_offsets')
address('color_name',df.descriptor_color,'name')
address('shape_name_plural',df.descriptor_shape,'name_plural')

header('health_offsets')
address('parent_id',df.body_part_raw,'con_part_id')
address('body_part_flags',df.body_part_raw,'flags')
address('layers_vector',df.body_part_raw,'layers')
address('number',df.body_part_raw,'number')
address('names_vector',df.body_part_raw,'name_singular')
address('names_plural_vector',df.body_part_raw,'name_plural')
address('layer_tissue',df.body_part_layer_raw,'tissue_id')
address('layer_global_id',df.body_part_layer_raw,'layer_id')
address('tissue_name',df.tissue_template,'tissue_name_singular')
address('tissue_flags',df.tissue_template,'flags')

header('dwarf_offsets')
address('name',df.unit,'name')
address('custom_profession',df.unit,'custom_profession')
address('profession',df.unit,'profession')
address('race',df.unit,'race')
address('flags1',df.unit,'flags1')
address('flags2',df.unit,'flags2')
address('flags3',df.unit,'flags3')
address('meeting',df.unit,'meeting')
address('caste',df.unit,'caste')
address('sex',df.unit,'sex')
address('id',df.unit,'id')
address('animal_type',df.unit,'training_level')
address('civ',df.unit,'civ_id')
address('specific_refs',df.unit,'specific_refs')
address('squad_id',df.unit,'military','squad_id')
address('squad_position',df.unit,'military','squad_position')
address('recheck_equipment',df.unit,'military','pickup_flags')
address('mood',df.unit,'mood')
address('birth_year',df.unit,'birth_year')
address('birth_time',df.unit,'birth_time')
address('pet_owner_id',df.unit,'relationship_ids',df.unit_relationship_type.Pet)
address('current_job',df.unit,'job','current_job')
address('physical_attrs',df.unit,'body','physical_attrs')
address('body_size',df.unit,'appearance','body_modifiers')
address('size_info',df.unit,'body','size_info','size_cur')
address('size_base',df.unit,'body','size_info','size_base')
address('curse',df.unit,'curse','name')
address('curse_add_flags1',df.unit,'curse','add_tags1')
address('turn_count',df.unit,'curse','time_on_site')
address('souls',df.unit,'status','souls')
address('states',df.unit,'status','misc_traits')
address('labors',df.unit,'status','labors')
address('hist_id',df.unit,'hist_figure_id')
address('artifact_name',df.unit,'status','artifact_name')
address('active_syndrome_vector',df.unit,'syndromes','active')
address('syn_sick_flag',df.unit_syndrome,'flags')
address('unit_health_info',df.unit,'health')
address('temp_mood',df.unit,'counters','soldier_mood')
address('counters1',df.unit,'counters','winded')
address('counters2',df.unit, 'counters','pain')
address('counters3',df.unit, 'counters2','paralysis')
address('limb_counters',df.unit,'status2','limbs_stand_max')
address('blood',df.unit,'body','blood_max')
address('body_component_info',df.unit,'body','components')
address('layer_status_vector',df.body_component_info,'layer_status')
address('wounds_vector',df.unit,'body','wounds')
address('mood_skill',df.unit,'job','mood_skill')
address('used_items_vector',df.unit,'used_items')
address('affection_level',df.unit_item_use,'affection_level')
address('inventory',df.unit,'inventory')
address('inventory_item_mode',df.unit_inventory_item,'mode')
address('inventory_item_bodypart',df.unit_inventory_item,'body_part_id')

header('syndrome_offsets')
address('cie_effects',df.syndrome,'ce')
address('cie_end',df.creature_interaction_effect,'end')
address('cie_first_perc',df.creature_interaction_effect_phys_att_changest,'phys_att_perc') --same for mental
address('cie_phys',df.creature_interaction_effect_phys_att_changest,'phys_att_add')
address('cie_ment',df.creature_interaction_effect_ment_att_changest,'ment_att_add')
address('syn_classes_vector',df.syndrome,'syn_class')
address('trans_race_vec',df.creature_interaction_effect_body_transformationst,'race')

header('unit_wound_offsets')
address('parts',df.unit_wound,'parts')
address('id',df.unit_wound.T_parts,'body_part_id')
address('layer',df.unit_wound.T_parts,'layer_idx')
address('general_flags',df.unit_wound,'flags')
address('flags1',df.unit_wound.T_parts,'flags1')
address('flags2',df.unit_wound.T_parts,'flags2')
address('effects_vector',df.unit_wound.T_parts,'effect_type')
address('bleeding',df.unit_wound.T_parts,'bleeding')
address('pain',df.unit_wound.T_parts,'pain')
address('cur_pen',df.unit_wound.T_parts,'cur_penetration_perc')
address('max_pen',df.unit_wound.T_parts,'max_penetration_perc')

header('soul_details')
address('name',df.unit_soul,'name')
address('orientation',df.unit_soul,'orientation_flags')
address('mental_attrs',df.unit_soul,'mental_attrs')
address('skills',df.unit_soul,'skills')
address('preferences',df.unit_soul,'preferences')
address('personality',df.unit_soul,'personality')
address('beliefs',df.unit_personality,'values')
address('emotions',df.unit_personality,'emotions')
address('goals',df.unit_personality,'dreams')
address('goal_realized',df.unit_personality.T_dreams,'flags')
address('traits',df.unit_personality,'traits')
address('stress_level',df.unit_personality,'stress')
address('needs',df.unit_personality,'needs')
address('current_focus',df.unit_personality,'current_focus')
address('undistracted_focus',df.unit_personality,'undistracted_focus')
address('combat_hardened',df.unit_personality,'combat_hardened')
address('likes_outdoors',df.unit_personality,'likes_outdoors')

header('need_offsets')
address('id',df.unit_personality.T_needs,'id')
address('deity_id',df.unit_personality.T_needs,'deity_id')
address('focus_level',df.unit_personality.T_needs,'focus_level')
address('need_level',df.unit_personality.T_needs,'need_level')

header('emotion_offsets')
address('emotion_type',df.unit_personality.T_emotions,'type')
address('strength',df.unit_personality.T_emotions,'strength')
address('thought_id',df.unit_personality.T_emotions,'thought')
address('sub_id',df.unit_personality.T_emotions,'subthought')
address('level',df.unit_personality.T_emotions,'severity')
address('year',df.unit_personality.T_emotions,'year')
address('year_tick',df.unit_personality.T_emotions,'year_tick')

header('job_details')
address('id',df.job,'job_type')
address('mat_type',df.job,'mat_type')
address('mat_index',df.job,'mat_index')
address('mat_category',df.job,'material_category')
address('sub_job_id',df.job,'reaction_name')
address('reaction',df.reaction,'name')
address('reaction_skill',df.reaction,'skill')

header('squad_offsets')
address('id',df.squad,'id')
address('name',df.squad,'name')
address('alias',df.squad,'alias')
address('members',df.squad,'positions')
address('orders',df.squad,'orders')
address('schedules',df.squad,'schedule')
value('sched_size',df.squad_schedule_entry:sizeof())
address('sched_orders',df.squad_schedule_entry,'orders')
address('sched_assign',df.squad_schedule_entry,'order_assignments')
address('alert',df.squad,'cur_routine_idx')
address('carry_food',df.squad,'carry_food')
address('carry_water',df.squad,'carry_water')
address('ammunition',df.squad,'ammo','ammunition')
address('ammunition_qty',df.squad_ammo_spec,'amount')
address('quiver',df.squad_position,'quiver')
address('backpack',df.squad_position,'backpack')
address('flask',df.squad_position,'flask')
address('armor_vector',df.squad_position,'uniform','body')
address('helm_vector',df.squad_position,'uniform','head')
address('pants_vector',df.squad_position,'uniform','pants')
address('gloves_vector',df.squad_position,'uniform','gloves')
address('shoes_vector',df.squad_position,'uniform','shoes')
address('shield_vector',df.squad_position,'uniform','shield')
address('weapon_vector',df.squad_position,'uniform','weapon')
address('uniform_item_filter',df.squad_uniform_spec,'item_filter')
address('uniform_indiv_choice',df.squad_uniform_spec,'indiv_choice')

header('activity_offsets')
address('activity_type',df.activity_entry,'type')
address('events',df.activity_entry,'events')
address('participants',df.activity_event_combat_trainingst,'participants')
address('sq_lead',df.activity_event_skill_demonstrationst,'hist_figure_id')
address('sq_skill',df.activity_event_skill_demonstrationst,'skill')
address('sq_train_rounds',df.activity_event_skill_demonstrationst,'train_rounds')
address('pray_deity',df.activity_event_prayerst,'histfig_id')
address('pray_sphere',df.activity_event_prayerst,'topic')
address('knowledge_category',df.activity_event_ponder_topicst,'knowledge','flag_type')
address('knowledge_flag',df.activity_event_ponder_topicst,'knowledge','flag_data')
address('perf_type',df.activity_event_performancest,'type')
address('perf_participants',df.activity_event_performancest,'participant_actions')
address('perf_histfig',df.activity_event_performancest.T_participant_actions,'histfig_id')

header('art_offsets')
address('name',df.poetic_form,'name')

header('viewscreen_offsets')
address('view',df.interfacest,'view')
address('child',df.viewscreen,'child')
address('setupdwarfgame_units',df.viewscreen_setupdwarfgamest,'s_unit')

-- Final creation of the file

local out = io.open('therapist.ini', 'w')

out:write('[info]\n')
if dfhack.getOSType() == 'windows' and dfhack.internal.getPE then
    out:write(('checksum=0x%x\n'):format(dfhack.internal.getPE()))
elseif dfhack.getOSType() ~= 'windows' and dfhack.internal.getMD5 then
    out:write(('checksum=0x%s\n'):format(dfhack.internal.getMD5():sub(1, 8)))
else
    out:write('checksum=<<fillme>>\n')
end
out:write('version_name='..dfhack.getDFVersion()..'\n')
out:write('complete='..(complete and 'true' or 'false')..'\n')

for i,v in ipairs(lines) do
    out:write(v..'\n')
end

-- Flags
local function write_flags(name,flag_array)
    local flag_array = flag_array --as:{1:string,2:'number[]'}[]
    out:write('\n')
    out:write('['..name..']\n')
    out:write('size='..#flag_array..'\n')
    for i,flags in pairs(flag_array) do
        out:write(('%d\\name="%s"\n'):format(i, flags[1]))
        local value = 0
        for _,flag in pairs(flags[2]) do
            value = value | (1 << flag)
        end
        out:write(('%d\\value=0x%08x\n'):format(i, value))
    end
end

write_flags('valid_flags_2', {})
write_flags('invalid_flags_1', {
    { 'a skeleton', { df.unit_flags1.skeleton } },
    { 'a merchant', { df.unit_flags1.merchant } },
    { 'outpost liaison, diplomat, or artifact requesting visitor', { df.unit_flags1.diplomat } },
    { 'an invader or hostile', { df.unit_flags1.active_invader } },
    { 'an invader or hostile', { df.unit_flags1.invader_origin } },
    { 'resident, invader or ambusher', { df.unit_flags1.hidden_ambusher, df.unit_flags1.invades } },
    { 'part of a merchant caravan', { df.unit_flags1.forest } },
    { 'inactive, currently not in play', { df.unit_flags1.inactive } },
    { 'marauder', { df.unit_flags1.marauder } }
});
write_flags ('invalid_flags_2', {
    { 'killed, Jim.', { df.unit_flags2.killed } },
    { 'from the Underworld. SPOOKY!', { df.unit_flags2.underworld } },
    { 'resident', { df.unit_flags2.resident } },
    { 'uninvited visitor', { df.unit_flags2.visitor_uninvited } },
    { 'visitor', { df.unit_flags2.visitor } }
});
write_flags ('invalid_flags_3', {
    { 'a ghost', { df.unit_flags3.ghostly } }
});

out:close()
