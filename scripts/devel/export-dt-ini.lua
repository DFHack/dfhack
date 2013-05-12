-- Exports an ini file for Dwarf Therapist.

local utils = require 'utils'
local ms = require 'memscan'

-- Utility functions

local globals = df.global
local global_addr = dfhack.internal.getAddress
local os_type = dfhack.getOSType()
local rdelta = dfhack.internal.getRebaseDelta()

local vbias = 0
if os_type == 'windows' then vbias = -4 end

local lines = {}
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
local function address(name,bias,base,field,...)
    local addr

    if base == globals then
        addr = global_addr(field)
        bias = bias - rdelta
        if addr and select('#',...) > 0 then
            _,addr = df.sizeof(ms.field_ref(base,field,...))
        end
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

    if addr then
        addr = addr + bias
    end

    value(name, addr)
end

local function offset(name,base,...)
    address(name,0,base,...)
end
local function vector(name,base,...)
    address(name,vbias,base,...)
end

-- List of actual values

header('addresses')
vector('translation_vector',globals,'world','raws','language','translations')
vector('language_vector',globals,'world','raws','language','words')
vector('creature_vector',globals,'world','units','all')
vector('active_creature_vector',globals,'world','units','active')
offset('dwarf_race_index',globals,'ui','race_id')
vector('squad_vector',globals,'world','squads','all')
offset('current_year',globals,'cur_year')
offset('cur_year_tick',globals,'cur_year_tick')
offset('dwarf_civ_index',globals,'ui','civ_id')
vector('races_vector',globals,'world','raws','creatures','all')
vector('reactions_vector',globals,'world','raws','reactions')
vector('historical_figures',globals,'world','history','figures')
vector('fake_identities',globals,'world','assumed_identities','all')
vector('historical_figures_vector',globals,'world','history','figures')
vector('fake_identities_vector',globals,'world','assumed_identities','all')
offset('fortress_entity',globals,'ui','main','fortress_entity')
vector('historical_entities_vector',globals,'world','entities','all')
vector('weapons_vector',globals,'world','raws','itemdefs','weapons')
vector('trap_vector',globals,'world','raws','itemdefs','trapcomps')
vector('toy_vector',globals,'world','raws','itemdefs','toys')
vector('tool_vector',globals,'world','raws','itemdefs','tools')
vector('instrument_vector',globals,'world','raws','itemdefs','instruments')
vector('armor_vector',globals,'world','raws','itemdefs','armor')
vector('ammo_vector',globals,'world','raws','itemdefs','ammo')
vector('siegeammo_vector',globals,'world','raws','itemdefs','siege_ammo')
vector('glove_vector',globals,'world','raws','itemdefs','gloves')
vector('shoe_vector',globals,'world','raws','itemdefs','shoes')
vector('shield_vector',globals,'world','raws','itemdefs','shields')
vector('helm_vector',globals,'world','raws','itemdefs','helms')
vector('pant_vector',globals,'world','raws','itemdefs','pants')
vector('food_vector',globals,'world','raws','itemdefs','food')
vector('colors_vector',globals,'world','raws','language','colors')
vector('shapes_vector',globals,'world','raws','language','shapes')
offset('base_materials',globals,'world','raws','mat_table','builtin')
vector('inorganics_vector',globals,'world','raws','inorganics')
vector('plants_vector',globals,'world','raws','plants','all')
vector('material_templates_vector',globals,'world','raws','material_templates')
offset('world_data',globals,'world','world_data')
vector('active_sites_vector',df.world_data,'active_site')
offset('world_site_type',df.world_site,'type')

header('offsets')
offset('word_table',df.language_translation,'words')
value('string_buffer_offset', 0x0000)

header('word_offsets')
offset('base',df.language_word,'word')
offset('noun_singular',df.language_word,'forms','Noun')
offset('noun_plural',df.language_word,'forms','NounPlural')
offset('adjective',df.language_word,'forms','Adjective')
offset('verb',df.language_word,'forms','Verb')
offset('present_simple_verb',df.language_word,'forms','Verb3rdPerson')
offset('past_simple_verb',df.language_word,'forms','VerbPast')
offset('past_participle_verb',df.language_word,'forms','VerbPassive')
offset('present_participle_verb',df.language_word,'forms','VerbGerund')
offset('words',df.language_name,'words')
offset('word_type',df.language_name,'parts_of_speech')
offset('language_id',df.language_name,'language')

header('race_offsets')
offset('name_singular',df.creature_raw,'name',0)
offset('name_plural',df.creature_raw,'name',1)
offset('adjective',df.creature_raw,'name',2)
offset('baby_name_singular',df.creature_raw,'general_baby_name',0)
offset('baby_name_plural',df.creature_raw,'general_baby_name',1)
offset('child_name_singular',df.creature_raw,'general_child_name',0)
offset('child_name_plural',df.creature_raw,'general_child_name',1)
vector('pref_string_vector',df.creature_raw,'prefstring')
vector('castes_vector',df.creature_raw,'caste')
vector('pop_ratio_vector',df.creature_raw,'pop_ratio')
vector('materials_vector',df.creature_raw,'material')
offset('flags',df.creature_raw,'flags')

header('caste_offsets')
offset('caste_name',df.caste_raw,'caste_name')
offset('caste_descr',df.caste_raw,'description')
offset('caste_phys_att_ranges',df.caste_raw,'attributes','phys_att_range')
offset('caste_ment_att_ranges',df.caste_raw,'attributes','ment_att_range')
offset('adult_size',df.caste_raw,'misc','adult_size')
offset('flags',df.caste_raw,'flags')
vector('extracts',df.caste_raw,'extracts','extract_matidx')
offset('skill_rates',df.caste_raw,'skill_rates')
offset('caste_att_rates',df.caste_raw,'attributes','phys_att_rates')
offset('caste_att_caps',df.caste_raw,'attributes','phys_att_cap_perc')

header('hist_entity_offsets')
vector('squads',df.historical_entity,'squads')
vector('positions',df.historical_entity,'positions','own')
vector('assignments',df.historical_entity,'positions','assignments')
offset('assign_hist_id',df.entity_position_assignment,'histfig')
offset('assign_position_id',df.entity_position_assignment,'position_id')
offset('position_id',df.entity_position,'id')
offset('position_name',df.entity_position,'name')
offset('position_female_name',df.entity_position,'name_female')
offset('position_male_name',df.entity_position,'name_male')

header('hist_figure_offsets')
offset('hist_race',df.historical_figure,'race')
offset('hist_name',df.historical_figure,'name')
offset('id',df.historical_figure,'id')
offset('hist_fig_info',df.historical_figure,'info')
offset('reputation',df.historical_figure_info,'reputation')
offset('current_ident',df.historical_figure_info.T_reputation,'cur_identity')
offset('fake_name',df.assumed_identity,'name')
offset('fake_birth_year',df.assumed_identity,'birth_year')
offset('fake_birth_time',df.assumed_identity,'birth_second')

header('weapon_offsets')
offset('name_plural',df.itemdef_weaponst,'name_plural')
offset('single_size',df.itemdef_weaponst,'two_handed')
offset('multi_size',df.itemdef_weaponst,'minimum_size')
offset('ammo',df.itemdef_weaponst,'ranged_ammo')

header('material_offsets')
offset('solid_name',df.material_common,'state_name','Solid')
offset('liquid_name',df.material_common,'state_name','Liquid')
offset('gas_name',df.material_common,'state_name','Gas')
offset('powder_name',df.material_common,'state_name','Powder')
offset('paste_name',df.material_common,'state_name','Paste')
offset('pressed_name',df.material_common,'state_name','Pressed')
offset('inorganic_materials_vector',df.inorganic_raw,'material')
offset('flags',df.material_common,'flags')

header('plant_offsets')
offset('name',df.plant_raw,'name')
offset('name_plural',df.plant_raw,'name_plural')
offset('name_leaf_plural',df.plant_raw,'leaves_plural')
offset('name_seed_plural',df.plant_raw,'seed_plural')
vector('materials_vector',df.plant_raw,'material')
offset('flags',df.plant_raw,'flags')

header('item_offsets')
offset('name_plural',df.itemdef_armorst,'name_plural')
offset('adjective',df.itemdef_armorst,'name_preplural')
offset('mat_name',df.itemdef_armorst,'material_placeholder')

header('descriptor_offsets')
offset('color_name',df.descriptor_color,'name')
offset('shape_name_plural',df.descriptor_shape,'name_plural')

header('dwarf_offsets')
offset('first_name',df.unit,'name','first_name')
offset('nick_name',df.unit,'name','nickname')
offset('last_name',df.unit,'name','words')
offset('custom_profession',df.unit,'custom_profession')
offset('profession',df.unit,'profession')
offset('race',df.unit,'race')
offset('flags1',df.unit,'flags1')
offset('flags2',df.unit,'flags2')
offset('flags3',df.unit,'flags3')
offset('caste',df.unit,'caste')
offset('sex',df.unit,'sex')
offset('id',df.unit,'id')
offset('animal_type',df.unit,'training_level')
offset('civ',df.unit,'civ_id')
vector('specific_refs',df.unit,'specific_refs')
offset('squad_id',df.unit,'military','squad_id')
offset('squad_position',df.unit,'military','squad_position')
offset('recheck_equipment',df.unit,'military','pickup_flags')
offset('mood',df.unit,'mood')
offset('birth_year',df.unit,'relations','birth_year')
offset('birth_time',df.unit,'relations','birth_time')
offset('current_job',df.unit,'job','current_job')
offset('physical_attrs',df.unit,'body','physical_attrs')
vector('body_size',df.unit,'appearance','body_modifiers')
offset('curse',df.unit,'curse','name')
offset('curse_add_flags1',df.unit,'curse','add_tags1')
offset('turn_count',df.unit,'curse','time_on_site')
vector('souls',df.unit,'status','souls')
vector('states',df.unit,'status','misc_traits')
offset('labors',df.unit,'status','labors')
offset('happiness',df.unit,'status','happiness')
vector('thoughts',df.unit,'status','recent_events')
offset('squad_ref_id',df.unit,'hist_figure_id')
offset('hist_id',df.unit,'hist_figure_id')
offset('artifact_name',df.unit,'status','artifact_name')

header('soul_details')
offset('name',df.unit_soul,'name')
offset('mental_attrs',df.unit_soul,'mental_attrs')
vector('skills',df.unit_soul,'skills')
offset('traits',df.unit_soul,'traits')
vector('preferences',df.unit_soul,'preferences')

header('job_details')
offset('id',df.job,'job_type')
value('on_break_flag',df.misc_trait_type.OnBreak)
offset('sub_job_id',df.job,'reaction_name')
offset('reaction',df.reaction,'name')
offset('reaction_skill',df.reaction,'skill')
offset('mat_type',df.job,'mat_type')
offset('mat_index',df.job,'mat_index')
offset('mat_category',df.job,'material_category')

header('squad_offsets')
offset('id',df.squad,'id')
offset('name',df.squad,'name')
offset('name_old',df.squad,'name','words')
offset('alias',df.squad,'alias')
vector('members',df.squad,'positions')

-- Final creation of the file

local out = io.open('therapist.ini', 'w')

out:write('[info]\n')
-- TODO: add an api function to retrieve the checksum
out:write('checksum=<<fillme>>\n')
out:write('version_name='..dfhack.getDFVersion()..'\n')
out:write('complete='..(complete and 'true' or 'false')..'\n')

for i,v in ipairs(lines) do
    out:write(v..'\n')
end

out:write[[

[valid_flags_1]
size=1
1\name=Not from around these parts
1\value=0x80000000

[valid_flags_2]
size=0

[invalid_flags_1]
size=10
1\name=a zombie
1\value=0x00001000
2\name=a skeleton
2\value=0x00002000
3\name=a merchant or diplomat
3\value=0x00000040
4\name=outpost liason
4\value=0x00000800
5\name=an invader or hostile
5\value=0x00020000
6\name=an invader or hostile
6\value=0x00080000
7\name=an invader or hostile
7\value=0x000C0000
8\name=a merchant escort
8\value=0x00000080
9\name="Dead, Jim."
9\value=0x00000002
10\name=marauder
10\value=0x00000010

[invalid_flags_2]
size=5
1\name="killed, Jim."
1\value=0x00000080
2\name=from the Underworld. SPOOKY!
2\value=0x00040000
3\name=resident
3\value=0x00080000
4\name=visitor_uninvited
4\value=0x00400000
5\name=visitor
5\value=0x00800000

[invalid_flags_3]
size=1
1\name=a ghost
1\value=0x00001000
]]

out:close()
