#!/usr/bin/perl

use strict;
use warnings;

my ($version, $timestamp, $hash);

open FH, 'version.lisp' or die "Cannot open version";
while (<FH>) {
    if (/df-version-str.*\"(.*)\"/) {
        $version = $1;
    } elsif (/windows-timestamp.*#x([0-9a-f]+)/) {
        $timestamp = $1;
    } elsif (/linux-hash.*\"(.*)\"/) {
        $hash = $1;
    }
}
close FH;

sub load_csv(\%$) {
    my ($rhash, $fname) = @_;

    open FH, $fname or die "Cannot open $fname";
    while (<FH>) {
        next unless /^\"([^\"]*)\",\"(\d+)\",\"(?:0x([0-9a-fA-F]+))?\",\"[^\"]*\",\"([^\"]*)\",\"([^\"]*)\",\"([^\"]*)\"/;
        my ($top, $level, $addr, $type, $name, $target) = ($1,$2,$3,$4,$5,$6);
        next if defined $rhash->{$top}{$name};
        $rhash->{$top}{$name} = ($type eq 'enum-item' ? $target : hex $addr);
    }
    close FH;
}

our $complete;

sub lookup_addr(\%$$;$) {
    my ($rhash, $top, $name, $bias) = @_;

    my $val = $rhash->{$top}{$name};
    unless (defined $val) {
        $complete = 0;
        return 0;
    }
    return $val + ($bias||0);
}

our @lines;

sub emit_header($) {
    my ($name) = @_;
    push @lines, '' if @lines;
    push @lines, "[$name]";
}

sub emit_addr($\%$$;$) {
    my ($name, $rhash, $top, $var, $bias) = @_;

    my $val = $rhash->{$top}{$var};
    if (defined $val) {
        $val += ($bias||0);
        if ($val < 0x10000) {
            push @lines, sprintf('%s=0x%04x', $name, $val);
        } else {
            push @lines, sprintf('%s=0x%08x', $name, $val);
        }
    } else {
        $complete = 0;
        push @lines, "$name=0x0";
    }
}

sub generate_dt_ini($$$$) {
    my ($subdir, $version, $checksum, $ssize) = @_;

    my %globals;
    load_csv %globals, "$subdir/globals.csv";
    my %all;
    load_csv %all, "$subdir/all.csv";

    local $complete = 1;
    local @lines;

    emit_header 'addresses';
    emit_addr 'translation_vector',%globals,'world','world.raws.language.translations';
    emit_addr 'language_vector',%globals,'world','world.raws.language.words';
    emit_addr 'creature_vector',%globals,'world','world.units.all';
    emit_addr 'active_creature_vector',%globals,'world','world.units.active';
    emit_addr 'dwarf_race_index',%globals,'ui','ui.race_id';
    emit_addr 'squad_vector',%globals,'world','world.squads.all';
    emit_addr 'current_year',%globals,'cur_year','cur_year';
    emit_addr 'cur_year_tick',%globals,'cur_year_tick','cur_year_tick';
    emit_addr 'dwarf_civ_index',%globals,'ui','ui.civ_id';
    emit_addr 'races_vector',%globals,'world','world.raws.creatures.all';
    emit_addr 'reactions_vector',%globals,'world','world.raws.reactions';
    emit_addr 'events_vector',%globals,'world','world.history.events';
    emit_addr 'historical_figures_vector',%globals,'world','world.history.figures';
    emit_addr 'fake_identities_vector',%globals,'world','world.identities.all';
    emit_addr 'fortress_entity',%globals,'ui','ui.main.fortress_entity';
    emit_addr 'historical_entities_vector',%globals,'world','world.entities.all';
    emit_addr 'itemdef_weapons_vector',%globals,'world','world.raws.itemdefs.weapons';
    emit_addr 'itemdef_trap_vector',%globals,'world','world.raws.itemdefs.trapcomps';
    emit_addr 'itemdef_toy_vector',%globals,'world','world.raws.itemdefs.toys';
    emit_addr 'itemdef_tool_vector',%globals,'world','world.raws.itemdefs.tools';
    emit_addr 'itemdef_instrument_vector',%globals,'world','world.raws.itemdefs.instruments';
    emit_addr 'itemdef_armor_vector',%globals,'world','world.raws.itemdefs.armor';
    emit_addr 'itemdef_ammo_vector',%globals,'world','world.raws.itemdefs.ammo';
    emit_addr 'itemdef_siegeammo_vector',%globals,'world','world.raws.itemdefs.siege_ammo';
    emit_addr 'itemdef_glove_vector',%globals,'world','world.raws.itemdefs.gloves';
    emit_addr 'itemdef_shoe_vector',%globals,'world','world.raws.itemdefs.shoes';
    emit_addr 'itemdef_shield_vector',%globals,'world','world.raws.itemdefs.shields';
    emit_addr 'itemdef_helm_vector',%globals,'world','world.raws.itemdefs.helms';
    emit_addr 'itemdef_pant_vector',%globals,'world','world.raws.itemdefs.pants';
    emit_addr 'itemdef_food_vector',%globals,'world','world.raws.itemdefs.food';
    emit_addr 'colors_vector',%globals,'world','world.raws.language.colors';
    emit_addr 'shapes_vector',%globals,'world','world.raws.language.shapes';
    emit_addr 'base_materials',%globals,'world','world.raws.mat_table.builtin';
    emit_addr 'inorganics_vector',%globals,'world','world.raws.inorganics';
    emit_addr 'plants_vector',%globals,'world','world.raws.plants.all';
    emit_addr 'material_templates_vector',%globals,'world','world.raws.material_templates';
    emit_addr 'all_syndromes_vector',%globals,'world','world.raws.syndromes.all';
    emit_addr 'world_data',%globals,'world','world.world_data';
    emit_addr 'active_sites_vector',%all,'world_data','active_site';
    emit_addr 'world_site_type',%all,'world_site','type';
    emit_addr 'weapons_vector',%globals,'world','world.items.other[WEAPON]';
    emit_addr 'shields_vector',%globals,'world','world.items.other[SHIELD]';
    emit_addr 'quivers_vector',%globals,'world','world.items.other[QUIVER]';
    emit_addr 'crutches_vector',%globals,'world','world.items.other[CRUTCH]';
    emit_addr 'backpacks_vector',%globals,'world','world.items.other[BACKPACK]';
    emit_addr 'ammo_vector',%globals,'world','world.items.other[AMMO]';
    emit_addr 'flasks_vector',%globals,'world','world.items.other[FLASK]';
    emit_addr 'pants_vector',%globals,'world','world.items.other[PANTS]';
    emit_addr 'armor_vector',%globals,'world','world.items.other[ARMOR]';
    emit_addr 'shoes_vector',%globals,'world','world.items.other[SHOES]';
    emit_addr 'helms_vector',%globals,'world','world.items.other[HELM]';
    emit_addr 'gloves_vector',%globals,'world','world.items.other[GLOVES]';
    emit_addr 'artifacts_vector',%globals,'world','world.artifacts.all';

    emit_header 'offsets';
    emit_addr 'word_table',%all,'language_translation','words';
    push @lines, 'string_buffer_offset=0x0000';

    emit_header 'word_offsets';
    emit_addr 'base',%all,'language_word','word';
    emit_addr 'noun_singular',%all,'language_word','forms[Noun]';
    emit_addr 'noun_plural',%all,'language_word','forms[NounPlural]';
    emit_addr 'adjective',%all,'language_word','forms[Adjective]';
    emit_addr 'verb',%all,'language_word','forms[Verb]';
    emit_addr 'present_simple_verb',%all,'language_word','forms[Verb3rdPerson]';
    emit_addr 'past_simple_verb',%all,'language_word','forms[VerbPast]';
    emit_addr 'past_participle_verb',%all,'language_word','forms[VerbPassive]';
    emit_addr 'present_participle_verb',%all,'language_word','forms[VerbGerund]';
    emit_addr 'words',%all,'language_name','words';
    emit_addr 'word_type',%all,'language_name','parts_of_speech';
    emit_addr 'language_id',%all,'language_name','language';

    emit_header 'general_ref_offsets';
    emit_addr 'ref_type',%all,'general_ref::vtable','getType';
    emit_addr 'artifact_id',%all,'general_ref_artifact','artifact_id';
    emit_addr 'item_id',%all,'general_ref_item','item_id';

    emit_header 'race_offsets';
    emit_addr 'name_singular',%all,'creature_raw','name';
    emit_addr 'name_plural',%all,'creature_raw','name',$ssize;
    emit_addr 'adjective',%all,'creature_raw','name',$ssize*2;
    emit_addr 'baby_name_singular',%all,'creature_raw','general_baby_name';
    emit_addr 'baby_name_plural',%all,'creature_raw','general_baby_name',$ssize;
    emit_addr 'child_name_singular',%all,'creature_raw','general_child_name';
    emit_addr 'child_name_plural',%all,'creature_raw','general_child_name',$ssize;
    emit_addr 'pref_string_vector',%all,'creature_raw','prefstring';
    emit_addr 'castes_vector',%all,'creature_raw','caste';
    emit_addr 'pop_ratio_vector',%all,'creature_raw','pop_ratio';
    emit_addr 'materials_vector',%all,'creature_raw','material';
    emit_addr 'flags',%all,'creature_raw','flags';
    emit_addr 'tissues_vector',%all,'creature_raw','tissue';

    emit_header 'caste_offsets';
    emit_addr 'caste_name',%all,'caste_raw','caste_name';
    emit_addr 'caste_descr',%all,'caste_raw','description';
    emit_addr 'caste_trait_ranges',%all,'caste_raw','personality.a';
    emit_addr 'caste_phys_att_ranges',%all,'caste_raw','attributes.phys_att_range';
    emit_addr 'baby_age',%all,'caste_raw','misc.baby_age';
    emit_addr 'child_age',%all,'caste_raw','misc.child_age';
    emit_addr 'adult_size',%all,'caste_raw','misc.adult_size';
    emit_addr 'flags',%all,'caste_raw','flags';
    emit_addr 'body_info',%all,'caste_raw','body_info';
    emit_addr 'skill_rates',%all,'caste_raw','skill_rates';
    emit_addr 'caste_att_rates',%all,'caste_raw','attributes.phys_att_rates';
    emit_addr 'caste_att_caps',%all,'caste_raw','attributes.phys_att_cap_perc';
    emit_addr 'shearable_tissues_vector',%all,'caste_raw','shearable_tissue_layer';
    emit_addr 'extracts',%all,'caste_raw','extracts.extract_matidx';

    emit_header 'hist_entity_offsets';
    emit_addr 'beliefs',%all,'historical_entity','resources.values';
    emit_addr 'squads',%all,'historical_entity','squads';
    emit_addr 'positions',%all,'historical_entity','positions.own';
    emit_addr 'assignments',%all,'historical_entity','positions.assignments';
    emit_addr 'assign_hist_id',%all,'entity_position_assignment','histfig';
    emit_addr 'assign_position_id',%all,'entity_position_assignment','position_id';
    emit_addr 'position_id',%all,'entity_position','id';
    emit_addr 'position_name',%all,'entity_position','name';
    emit_addr 'position_female_name',%all,'entity_position','name_female';
    emit_addr 'position_male_name',%all,'entity_position','name_male';

    emit_header 'hist_figure_offsets';
    emit_addr 'hist_race',%all,'historical_figure','race';
    emit_addr 'hist_name',%all,'historical_figure','name';
    emit_addr 'id',%all,'historical_figure','id';
    emit_addr 'hist_fig_info',%all,'historical_figure','info';
    emit_addr 'reputation',%all,'historical_figure_info','reputation';
    emit_addr 'current_ident',%all,'historical_figure_info::anon13','cur_identity';
    emit_addr 'fake_name',%all,'identity','name';
    emit_addr 'fake_birth_year',%all,'identity','birth_year';
    emit_addr 'fake_birth_time',%all,'identity','birth_second';
    emit_addr 'kills',%all,'historical_figure_info','kills';
    emit_addr 'killed_race_vector',%all,'historical_kills','killed_race';
    emit_addr 'killed_undead_vector',%all,'historical_kills','killed_undead';
    emit_addr 'killed_counts_vector',%all,'historical_kills','killed_count';

    emit_header 'hist_event_offsets';
    emit_addr 'event_year',%all,'history_event','year';
    emit_addr 'id',%all,'history_event','id';
    emit_addr 'killed_hist_id',%all,'history_event_hist_figure_diedst','victim_hf';

    emit_header 'item_offsets';
    if ($subdir eq 'osx') {
        push @lines, 'item_type=0x0004';
    } else {
        push @lines, 'item_type=0x0001';
    }
    emit_addr 'item_def',%all,'item_ammost','subtype'; #currently same for all
    emit_addr 'id',%all,'item','id';
    emit_addr 'general_refs',%all,'item','general_refs';
    emit_addr 'stack_size',%all,'item_actual','stack_size';
    emit_addr 'wear',%all,'item_actual','wear';
    emit_addr 'mat_type',%all,'item_crafted','mat_type';
    emit_addr 'mat_index',%all,'item_crafted','mat_index';
    emit_addr 'quality',%all,'item_crafted','quality';

    emit_header 'item_subtype_offsets';
    emit_addr 'sub_type',%all,'itemdef','subtype';
    emit_addr 'name',%all,'itemdef_armorst','name';
    emit_addr 'name_plural',%all,'itemdef_armorst','name_plural';
    emit_addr 'adjective',%all,'itemdef_armorst','name_preplural';

    emit_header 'item_filter_offsets';
    emit_addr 'item_subtype',%all,'item_filter_spec','item_subtype';
    emit_addr 'mat_class',%all,'item_filter_spec','material_class';
    emit_addr 'mat_type',%all,'item_filter_spec','mattype';
    emit_addr 'mat_index',%all,'item_filter_spec','matindex';

    emit_header 'weapon_subtype_offsets';
    emit_addr 'single_size',%all,'itemdef_weaponst','two_handed';
    emit_addr 'multi_size',%all,'itemdef_weaponst','minimum_size';
    emit_addr 'ammo',%all,'itemdef_weaponst','ranged_ammo';
    emit_addr 'melee_skill',%all,'itemdef_weaponst','skill_melee';
    emit_addr 'ranged_skill',%all,'itemdef_weaponst','skill_ranged';

    emit_header 'armor_subtype_offsets';
    emit_addr 'layer',%all,'armor_properties','layer';
    emit_addr 'mat_name',%all,'itemdef_armorst','material_placeholder';
    emit_addr 'other_armor_level',%all,'itemdef_helmst','armorlevel';
    emit_addr 'armor_adjective',%all,'itemdef_armorst','adjective';
    emit_addr 'armor_level',%all,'itemdef_armorst','armorlevel';
    emit_addr 'chest_armor_properties',%all,'itemdef_armorst','props';
    emit_addr 'pants_armor_properties',%all,'itemdef_pantsst','props';
    emit_addr 'other_armor_properties',%all,'itemdef_helmst','props';

    emit_header 'material_offsets';
    emit_addr 'solid_name',%all,'material_common','state_name[Solid]';
    emit_addr 'liquid_name',%all,'material_common','state_name[Liquid]';
    emit_addr 'gas_name',%all,'material_common','state_name[Gas]';
    emit_addr 'powder_name',%all,'material_common','state_name[Powder]';
    emit_addr 'paste_name',%all,'material_common','state_name[Paste]';
    emit_addr 'pressed_name',%all,'material_common','state_name[Pressed]';
    emit_addr 'flags',%all,'material_common','flags';
    emit_addr 'inorganic_materials_vector',%all,'inorganic_raw','material';
    emit_addr 'inorganic_flags',%all,'inorganic_raw','flags';

    emit_header 'plant_offsets';
    emit_addr 'name',%all,'plant_raw','name';
    emit_addr 'name_plural',%all,'plant_raw','name_plural';
    emit_addr 'name_leaf_plural',%all,'plant_raw','leaves_plural';
    emit_addr 'name_seed_plural',%all,'plant_raw','seed_plural';
    emit_addr 'materials_vector',%all,'plant_raw','material';
    emit_addr 'flags',%all,'plant_raw','flags';

    emit_header 'descriptor_offsets';
    emit_addr 'color_name',%all,'descriptor_color','name';
    emit_addr 'shape_name_plural',%all,'descriptor_shape','name_plural';

    emit_header 'health_offsets';
    emit_addr 'parent_id',%all,'body_part_raw','con_part_id';
    emit_addr 'layers_vector',%all,'body_part_raw','layers';
    emit_addr 'number',%all,'body_part_raw','number';
    emit_addr 'names_vector',%all,'body_part_raw','name_singular';
    emit_addr 'names_plural_vector',%all,'body_part_raw','name_plural';
    emit_addr 'layer_tissue',%all,'body_part_layer_raw','tissue_id';
    emit_addr 'layer_global_id',%all,'body_part_layer_raw','layer_id';
    emit_addr 'tissue_name',%all,'tissue_template','tissue_name_singular';
    emit_addr 'tissue_flags',%all,'tissue_template','flags';

    emit_header 'dwarf_offsets';
    emit_addr 'first_name',%all,'unit','name',lookup_addr(%all,'language_name','first_name');
    emit_addr 'nick_name',%all,'unit','name',lookup_addr(%all,'language_name','nickname');
    emit_addr 'last_name',%all,'unit','name',lookup_addr(%all,'language_name','words');
    emit_addr 'custom_profession',%all,'unit','custom_profession';
    emit_addr 'profession',%all,'unit','profession';
    emit_addr 'race',%all,'unit','race';
    emit_addr 'flags1',%all,'unit','flags1';
    emit_addr 'flags2',%all,'unit','flags2';
    emit_addr 'flags3',%all,'unit','flags3';
    emit_addr 'caste',%all,'unit','caste';
    emit_addr 'sex',%all,'unit','sex';
    emit_addr 'id',%all,'unit','id';
    emit_addr 'animal_type',%all,'unit','training_level';
    emit_addr 'civ',%all,'unit','civ_id';
    emit_addr 'specific_refs',%all,'unit','specific_refs';
    emit_addr 'squad_id',%all,'unit','military.squad_id';
    emit_addr 'squad_position',%all,'unit','military.squad_position';
    emit_addr 'recheck_equipment',%all,'unit','military.pickup_flags';
    emit_addr 'mood',%all,'unit','mood';
    emit_addr 'birth_year',%all,'unit','relations.birth_year';
    emit_addr 'birth_time',%all,'unit','relations.birth_time';
    emit_addr 'pet_owner_id',%all,'unit','relations.pet_owner_id';
    emit_addr 'current_job',%all,'unit','job.current_job';
    emit_addr 'physical_attrs',%all,'unit','body.physical_attrs';
    emit_addr 'body_size',%all,'unit','appearance.body_modifiers';
    emit_addr 'size_info',%all,'unit','body.size_info';
    emit_addr 'curse',%all,'unit','curse.name';
    emit_addr 'curse_add_flags1',%all,'unit','curse.add_tags1';
    emit_addr 'turn_count',%all,'unit','curse.time_on_site';
    emit_addr 'souls',%all,'unit','status.souls';
    emit_addr 'states',%all,'unit','status.misc_traits';
    emit_addr 'labors',%all,'unit','status.labors';
    emit_addr 'hist_id',%all,'unit','hist_figure_id';
    emit_addr 'artifact_name',%all,'unit','status.artifact_name';
    emit_addr 'active_syndrome_vector',%all,'unit','syndromes.active';
    emit_addr 'syn_sick_flag',%all,'unit_syndrome','flags.is_sick';
    emit_addr 'unit_health_info',%all,'unit','health';
    emit_addr 'temp_mood',%all,'unit','counters.soldier_mood';
    emit_addr 'counters1',%all,'unit','counters.winded';
    emit_addr 'counters2',%all,'unit','counters.pain';
    emit_addr 'counters3',%all,'unit','counters2.paralysis';
    emit_addr 'limb_counters',%all,'unit','status2.limbs_stand_max';
    emit_addr 'blood',%all,'unit','body.blood_max';
    emit_addr 'body_component_info',%all,'unit','body.components';
    emit_addr 'layer_status_vector',%all,'body_component_info','layer_status';
    emit_addr 'wounds_vector',%all,'unit','body.wounds';
    emit_addr 'mood_skill',%all,'unit','job.mood_skill';
    emit_addr 'used_items_vector',%all,'unit','used_items';
    emit_addr 'affection_level',%all,'unit_item_use','affection_level';
    emit_addr 'inventory',%all,'unit','inventory';
    emit_addr 'inventory_item_mode',%all,'unit_inventory_item','mode';
    emit_addr 'inventory_item_bodypart',%all,'unit_inventory_item','body_part_id';

    emit_header 'syndrome_offsets';
    emit_addr 'cie_effects',%all,'syndrome','ce';
    emit_addr 'cie_end',%all,'creature_interaction_effect','end';
    emit_addr 'cie_first_perc',%all,'creature_interaction_effect_phys_att_changest','phys_att_perc'; #same for mental
    emit_addr 'cie_phys',%all,'creature_interaction_effect_phys_att_changest','phys_att_add';
    emit_addr 'cie_ment',%all,'creature_interaction_effect_ment_att_changest','ment_att_add';
    emit_addr 'syn_classes_vector',%all,'syndrome','syn_class';
    emit_addr 'trans_race_id',%all,'creature_interaction_effect_body_transformationst','race';

    emit_header 'unit_wound_offsets';
    emit_addr 'parts',%all,'unit_wound','parts';
    emit_addr 'id',%all,'unit_wound::anon2','body_part_id';
    emit_addr 'layer',%all,'unit_wound::anon2','layer_idx';
    emit_addr 'general_flags',%all,'unit_wound','flags';
    emit_addr 'flags1',%all,'unit_wound::anon2','flags1';
    emit_addr 'flags2',%all,'unit_wound::anon2','flags2';
    emit_addr 'effects_vector',%all,'unit_wound::anon2','effect_type';
    emit_addr 'bleeding',%all,'unit_wound::anon2','bleeding';
    emit_addr 'pain',%all,'unit_wound::anon2','pain';
    emit_addr 'cur_pen',%all,'unit_wound::anon2','cur_penetration_perc';
    emit_addr 'max_pen',%all,'unit_wound::anon2','max_penetration_perc';

    emit_header 'soul_details';
    emit_addr 'name',%all,'unit_soul','name';
    emit_addr 'orientation',%all,'unit_soul','orientation_flags';
    emit_addr 'mental_attrs',%all,'unit_soul','mental_attrs';
    emit_addr 'skills',%all,'unit_soul','skills';
    emit_addr 'preferences',%all,'unit_soul','preferences';
    emit_addr 'personality',%all,'unit_soul','personality';
    emit_addr 'beliefs',%all,'unit_personality','values';
    emit_addr 'emotions',%all,'unit_personality','emotions';
    emit_addr 'goals',%all,'unit_personality','dreams';
    emit_addr 'goal_realized',%all,'unit_personality::anon5','unk8';
    emit_addr 'traits',%all,'unit_personality','traits';
    emit_addr 'stress_level',%all,'unit_personality','stress_level';

    emit_header 'emotion_offsets';
    emit_addr 'emotion_type',%all,'unit_personality::anon4','type';
    emit_addr 'strength',%all,'unit_personality::anon4','strength';
    emit_addr 'thought_id',%all,'unit_personality::anon4','thought';
    emit_addr 'sub_id',%all,'unit_personality::anon4','subthought';
    emit_addr 'level',%all,'unit_personality::anon4','severity';
    emit_addr 'year',%all,'unit_personality::anon4','year';
    emit_addr 'year_tick',%all,'unit_personality::anon4','year_tick';

    emit_header 'job_details';
    emit_addr 'id',%all,'job','job_type';
    emit_addr 'mat_type',%all,'job','mat_type';
    emit_addr 'mat_index',%all,'job','mat_index';
    emit_addr 'mat_category',%all,'job','material_category';
    emit_addr 'on_break_flag',%all,'misc_trait_type','OnBreak';
    emit_addr 'sub_job_id',%all,'job','reaction_name';
    emit_addr 'reaction',%all,'reaction','name';
    emit_addr 'reaction_skill',%all,'reaction','skill';

    emit_header 'squad_offsets';
    emit_addr 'id',%all,'squad','id';
    emit_addr 'name',%all,'squad','name';
    emit_addr 'alias',%all,'squad','alias';
    emit_addr 'members',%all,'squad','positions';
    emit_addr 'carry_food',%all,'squad','carry_food';
    emit_addr 'carry_water',%all,'squad','carry_water';
    emit_addr 'ammunition',%all,'squad','ammunition';
    emit_addr 'quiver',%all,'squad_position','quiver';
    emit_addr 'backpack',%all,'squad_position','backpack';
    emit_addr 'flask',%all,'squad_position','flask';
    emit_addr 'armor_vector',%all,'squad_position','uniform[body]';
    emit_addr 'helm_vector',%all,'squad_position','uniform[head]';
    emit_addr 'pants_vector',%all,'squad_position','uniform[pants]';
    emit_addr 'gloves_vector',%all,'squad_position','uniform[gloves]';
    emit_addr 'shoes_vector',%all,'squad_position','uniform[shoes]';
    emit_addr 'shield_vector',%all,'squad_position','uniform[shield]';
    emit_addr 'weapon_vector',%all,'squad_position','uniform[weapon]';
    emit_addr 'uniform_item_filter',%all,'squad_uniform_spec','item_filter';
    emit_addr 'uniform_indiv_choice',%all,'squad_uniform_spec','indiv_choice';

    my $body_str = join("\n",@lines);
    my $complete_str = ($complete ? 'true' : 'false');

    open OUT, ">$subdir/therapist.ini" or die "Cannot open output file";
    print OUT <<__END__;
[info]
checksum=0x$checksum
version_name=$version
complete=$complete_str

$body_str

[valid_flags_2]
size=0

[invalid_flags_1]
size=10
1\\name=a zombie
1\\value=0x00001000
2\\name=a skeleton
2\\value=0x00002000
3\\name=a merchant
3\\value=0x00000040
4\\name=outpost liason or diplomat
4\\value=0x00000800
5\\name=an invader or hostile
5\\value=0x00020000
6\\name=an invader or hostile
6\\value=0x00080000
7\\name=resident, invader or ambusher
7\\value=0x00600000
8\\name=part of a merchant caravan
8\\value=0x00000080
9\\name="Dead, Jim."
9\\value=0x00000002
10\\name=marauder
10\\value=0x00000010

[invalid_flags_2]
size=5
1\\name="killed, Jim."
1\\value=0x00000080
2\\name=from the Underworld. SPOOKY!
2\\value=0x00040000
3\\name=resident
3\\value=0x00080000
4\\name=uninvited visitor
4\\value=0x00400000
5\\name=visitor
5\\value=0x00800000

[invalid_flags_3]
size=1
1\\name=a ghost
1\\value=0x00001000
__END__
    close OUT;
}

generate_dt_ini 'linux', $version, substr($hash,0,8), 4;
generate_dt_ini 'windows', $version.' (graphics)', $timestamp, 0x1C;
generate_dt_ini 'osx', $version, substr($hash,0,8), 4;