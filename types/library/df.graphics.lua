-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class large_integer_u
---@field low_part number
---@field high_part number

---@class large_integer_unknown
---@field low_part number
---@field high_part number

---@class large_integer
---@field unnamed_large_integer_1 large_integer_unknown
---@field u large_integer_u
---@field quad_part integer
df.large_integer = nil

---@enum musicsoundst_linux_sound_system
df.musicsoundst_linux_sound_system = {
    ALSA = 0,
    OSS = 1,
    ESD = 2,
}
---@class musicsoundst
---@field soft_channel_number integer
---@field song integer
---@field music_active boolean
---@field sound_priority boolean
---@field sound_playing integer
---@field on boolean
---@field fmod_system integer
---@field fmod_master_channel_group integer
---@field mod fmod_sound[] songs
---@field samp fmod_sound[] sound effects
---@field musicsoundst_linux_sound_system musicsoundst_linux_sound_system
df.musicsoundst = nil

---@class fmod_sound
---@field sound integer
---@field channel integer
df.fmod_sound = nil

---@enum curses_color
df.curses_color = {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Yellow = 6,
    White = 7,
}

---@alias cmv_attribute bitfield

---@class graphic_viewportst
---@field flag integer
---@field dim_x integer
---@field dim_y integer
---@field clipx integer[]
---@field clipy integer[]
---@field screen_x integer
---@field screen_y integer
---@field screentexpos_background any[] floor
---@field screentexpos_floor_flag any[]
---@field screentexpos_background_two any[] boulder, plant, etc.
---@field screentexpos_liquid_flag any[]
---@field screentexpos_spatter_flag any[]
---@field screentexpos_spatter any[]
---@field screentexpos_ramp_flag any[]
---@field screentexpos_shadow_flag any[]
---@field screentexpos_building_one any[] floor
---@field screentexpos_item any[] ground stuff
---@field screentexpos_vehicle any[]
---@field screentexpos_vermin any[]
---@field screentexpos_left_creature any[]
---@field screentexpos any[] creature, etc.
---@field screentexpos_right_creature any[]
---@field screentexpos_building_two any[] high furniture/interior signposting
---@field screentexpos_projectile any[]
---@field screentexpos_high_flow any[]
---@field screentexpos_top_shadow any[]
---@field screentexpos_signpost any[] stuff that sticks up from below
---@field screentexpos_upleft_creature any[]
---@field screentexpos_up_creature any[]
---@field screentexpos_upright_creature any[]
---@field screentexpos_designation any[]
---@field screentexpos_interface any[] cursor, etc
---@field screentexpos_background_old any[]
---@field screentexpos_floor_flag_old any[]
---@field screentexpos_background_two_old any[]
---@field screentexpos_liquid_flag_old any[]
---@field screentexpos_spatter_flag_old any[]
---@field screentexpos_spatter_old any[]
---@field screentexpos_ramp_flag_old any[]
---@field screentexpos_shadow_flag_old any[]
---@field screentexpos_building_one_old any[]
---@field screentexpos_item_old any[]
---@field screentexpos_vehicle_old any[]
---@field screentexpos_vermin_old any[]
---@field screentexpos_left_creature_old any[]
---@field screentexpos_old any[]
---@field screentexpos_right_creature_old any[]
---@field screentexpos_building_two_old any[]
---@field screentexpos_projectile_old any[]
---@field screentexpos_high_flow_old any[]
---@field screentexpos_top_shadow_old any[]
---@field screentexpos_signpost_old any[]
---@field screentexpos_upleft_creature_old any[]
---@field screentexpos_up_creature_old any[]
---@field screentexpos_upright_creature_old any[]
---@field screentexpos_designation_old any[]
---@field screentexpos_interface_old any[]
---@field core_tree_species_plus_one any[]
---@field shadow_tree_species_plus_one any[]
df.graphic_viewportst = nil

---@class graphic_map_portst
---@field flag integer
---@field dim_x integer
---@field dim_y integer
---@field clipx integer[]
---@field clipy integer[]
---@field screen_x integer
---@field screen_y integer
---@field top_left_corner_x integer
---@field top_left_corner_y integer
---@field pixel_perc_x integer
---@field pixel_perc_y integer
---@field screentexpos_base any[]
---@field screentexpos_edge any[]
---@field screentexpos_edge2 any[]
---@field screentexpos_detail any[]
---@field screentexpos_tunnel any[]
---@field screentexpos_river any[]
---@field screentexpos_road any[]
---@field screentexpos_site any[]
---@field screentexpos_interface any[]
---@field screentexpos_detail_to_n any[]
---@field screentexpos_detail_to_s any[]
---@field screentexpos_detail_to_w any[]
---@field screentexpos_detail_to_e any[]
---@field screentexpos_detail_to_nw any[]
---@field screentexpos_detail_to_ne any[]
---@field screentexpos_detail_to_sw any[]
---@field screentexpos_detail_to_se any[]
---@field screentexpos_base_old any[]
---@field screentexpos_edge_old any[]
---@field screentexpos_edge2_old any[]
---@field screentexpos_detail_old any[]
---@field screentexpos_tunnel_old any[]
---@field screentexpos_river_old any[]
---@field screentexpos_road_old any[]
---@field screentexpos_site_old any[]
---@field screentexpos_interface_old any[]
---@field screentexpos_detail_to_n_old any[]
---@field screentexpos_detail_to_s_old any[]
---@field screentexpos_detail_to_w_old any[]
---@field screentexpos_detail_to_e_old any[]
---@field screentexpos_detail_to_nw_old any[]
---@field screentexpos_detail_to_ne_old any[]
---@field screentexpos_detail_to_sw_old any[]
---@field screentexpos_detail_to_se_old any[]
---@field edge_biome_data any[]
---@field edge_type_n any[]
---@field edge_type_s any[]
---@field edge_type_w any[]
---@field edge_type_e any[]
---@field edge_type_nw any[]
---@field edge_type_ne any[]
---@field edge_type_sw any[]
---@field edge_type_se any[]
---@field edge_biome_n any[]
---@field edge_biome_s any[]
---@field edge_biome_w any[]
---@field edge_biome_e any[]
---@field edge_biome_nw any[]
---@field edge_biome_ne any[]
---@field edge_biome_sw any[]
---@field edge_biome_se any[]
df.graphic_map_portst = nil

---@class cached_texturest
---@field w integer
---@field h integer
---@field tex integer
---@field tex_n integer
df.cached_texturest = nil

---@class texblitst
---@field x integer
---@field y integer
---@field tex integer
df.texblitst = nil

---@class graphic_tileset
---@field black_background_texpos number[]
---@field texture_indices1 integer[]
---@field texpos_custom_symbol integer[]
---@field texture_indices2 integer[]
---@field graphical_interface interface_setst
---@field classic_interface interface_setst
---@field texture_indices3 integer[]
---@field texpos_boulder integer[]
---@field texture_indices4 integer[]

---@class graphic
---@field viewport graphic_viewportst[]
---@field main_viewport graphic_viewportst
---@field lower_viewport any[]
---@field map_port graphic_map_portst[]
---@field main_map_port graphic_map_portst
---@field viewport_zoom_factor integer
---@field screenx integer
---@field screeny integer
---@field screenf curses_color
---@field screenb curses_color
---@field screenbright boolean
---@field use_old_16_colors boolean use F:B:BR instead of straight RGB
---@field screen_color_r integer
---@field screen_color_g integer
---@field screen_color_b integer
---@field screen_color_br integer
---@field screen_color_bg integer
---@field screen_color_bb integer
---@field ccolor any[] The curses-RGB mapping used for non-curses display modes
---@field uccolor any[] The curses-RGB mapping used for non-curses display modes
---@field color any[]
---@field mouse_x integer tile offset
---@field mouse_y integer tile offset
---@field precise_mouse_x integer pixel offset
---@field precise_mouse_y integer pixel offset
---@field screen_pixel_x integer
---@field screen_pixel_y integer
---@field tile_pixel_x integer
---@field tile_pixel_y integer
---@field screen any[]
---@field screen_limit integer pointer to last element of screen
---@field screentexpos any[]
---@field screentexpos_lower any[]
---@field screentexpos_anchored any[]
---@field screentexpos_anchored_x any[]
---@field screentexpos_anchored_y any[]
---@field screentexpos_flag any[]
---@field top_in_use boolean //we assume top is not in use unless a flag is set, and reprint the screen when it goes away, to avoid cell by cell checks
---@field screen_top any[]
---@field screen_top_limit integer
---@field screentexpos_top_lower any[]
---@field screentexpos_top_anchored any[]
---@field screentexpos_top any[]
---@field screentexpos_top_anchored_x any[]
---@field screentexpos_top_anchored_y any[]
---@field screentexpos_top_flag any[]
---@field display_title boolean
---@field display_background boolean
---@field screentexpos_refresh_buffer any[]
---@field refresh_buffer_val integer
---@field main_thread_requesting_reshape boolean set to true by main thread, set to false by graphics thread
---@field main_thread_requesting_reshape_activate_map_port boolean set to true by main thread, set to false by graphics thread
---@field clipx number[]
---@field clipy number[]
---@field tex cached_texturest[]
---@field texblits texblitst[]
---@field rect_id number
---@field print_time large_integer[]
---@field print_index number
---@field display_frames integer
---@field force_full_display_count integer
---@field do_clean_tile_cache integer true by main, false by graphics
---@field do_post_init_texture_clear integer true by main, false by graphics
---@field original_rect integer
---@field dimx integer
---@field dimy integer
---@field tileset graphic_tileset
df.graphic = nil

---@class interface_setst
---@field texpos_calendar_month any[]
---@field texpos_calendar_day_past integer[]
---@field texpos_calendar_day_current integer[]
---@field texpos_calendar_day_future integer[]
---@field texpos_border_top_left any[]
---@field texpos_border_top_right any[]
---@field texpos_border_bottom_left any[]
---@field texpos_border_bottom_right any[]
---@field texpos_border_top_intersection any[]
---@field texpos_border_bottom_intersection any[]
---@field texpos_border_middle_intersection integer[]
---@field texpos_border_left_intersection any[]
---@field texpos_border_right_intersection any[]
---@field texpos_border_left integer[]
---@field texpos_border_right integer[]
---@field texpos_border_top integer[]
---@field texpos_border_bottom integer[]
---@field texpos_hover_rectangle any[]
---@field texpos_hover_rectangle_join_w_sw integer
---@field texpos_hover_rectangle_join_w_s integer
---@field texpos_hover_rectangle_join_e_s integer
---@field texpos_hover_rectangle_join_e_se integer
---@field texpos_hover_close any[]
---@field texpos_hover_tab any[]
---@field texpos_hover_tab_inactive integer[]
---@field texpos_hover_tab_inside_corner_top integer
---@field texpos_hover_tab_inside_corner_bottom integer
---@field texpos_button_rectangle any[]
---@field texpos_button_rectangle_selected any[]
---@field texpos_button_rectangle_light any[]
---@field texpos_button_rectangle_dark any[]
---@field texpos_button_rectangle_divider integer[]
---@field texpos_button_category_rectangle any[]
---@field texpos_button_category_rectangle_selected any[]
---@field texpos_button_category_rectangle_on any[]
---@field texpos_button_category_rectangle_on_selected any[]
---@field texpos_button_category_rectangle_off any[]
---@field texpos_button_category_rectangle_off_selected any[]
---@field texpos_button_filter any[]
---@field texpos_button_filter_no_mag_right integer[]
---@field texpos_button_filter_name any[]
---@field texpos_button_picture_box any[]
---@field texpos_button_picture_box_selected any[]
---@field texpos_button_picture_box_highlighted any[]
---@field texpos_button_picture_box_sel_highlighted any[]
---@field texpos_button_picture_box_light any[]
---@field texpos_button_picture_box_dark any[]
---@field texpos_unk_v50_06 integer[]
---@field texpos_button_add any[]
---@field texpos_button_add_hover any[]
---@field texpos_button_add_pressed any[]
---@field texpos_button_add_invalid any[]
---@field texpos_button_subtract any[]
---@field texpos_button_subtract_hover any[]
---@field texpos_button_subtract_pressed any[]
---@field texpos_button_subtract_invalid any[]
---@field texpos_button_expander_closed any[]
---@field texpos_button_expander_open any[]
---@field texpos_scrollbar any[]
---@field texpos_scrollbar_up_hover integer[]
---@field texpos_scrollbar_up_pressed integer[]
---@field texpos_scrollbar_down_hover integer[]
---@field texpos_scrollbar_down_pressed integer[]
---@field texpos_scrollbar_small_scroller any[]
---@field texpos_scrollbar_small_scroller_hover any[]
---@field texpos_scrollbar_top_scroller integer[]
---@field texpos_scrollbar_top_scroller_hover integer[]
---@field texpos_scrollbar_bottom_scroller integer[]
---@field texpos_scrollbar_bottom_scroller_hover integer[]
---@field texpos_scrollbar_blank_scroller integer[]
---@field texpos_scrollbar_blank_scroller_hover integer[]
---@field texpos_scrollbar_center_scroller integer[]
---@field texpos_scrollbar_center_scroller_hover integer[]
---@field texpos_scrollbar_offcenter_scroller any[]
---@field texpos_scrollbar_offcenter_scroller_hover any[]
---@field texpos_scrollbar_sky integer[]
---@field texpos_scrollbar_ground integer[]
---@field texpos_scrollbar_underground integer[]
---@field texpos_slider_background any[]
---@field texpos_slider any[]
---@field texpos_slider_hover any[]
---@field texpos_tab any[]
---@field texpos_tab_selected any[]
---@field texpos_short_tab any[]
---@field texpos_short_tab_selected any[]
---@field texpos_short_subtab any[]
---@field texpos_short_subtab_selected any[]
---@field texpos_short_subsubtab any[]
---@field texpos_short_subsubtab_selected any[]
---@field texpos_interface_background integer
---@field texpos_button_main any[]
---@field texpos_button_small any[]
---@field texpos_button_horizontal_option_left_ornament any[]
---@field texpos_button_horizontal_option_active any[]
---@field texpos_button_horizontal_option_inactive any[]
---@field texpos_button_horizontal_option_right_ornament any[]
---@field texpos_button_horizontal_option_remove any[]
---@field texpos_button_horizontal_option_confirm any[]
---@field texpos_interior_border_n_s_w_e integer
---@field texpos_interior_border_n_w_e integer
---@field texpos_interior_border_s_w_e integer
---@field texpos_interior_border_w_e integer
---@field texpos_interior_border_n_s integer
---@field texpos_sort_ascending_active integer[]
---@field texpos_sort_ascending_inactive integer[]
---@field texpos_sort_descending_active integer[]
---@field texpos_sort_descending_inactive integer[]
---@field texpos_sort_text_active integer[]
---@field texpos_sort_text_inactive integer[]
---@field texpos_siege_light any[]
---@field texpos_diplomacy_light any[]
---@field texpos_petitions_light any[]
---@field texpos_grid_cell_inactive any[]
---@field texpos_grid_cell_active any[]
---@field texpos_grid_cell_button any[]
---@field texpos_button_stocks_recenter any[]
---@field texpos_button_stocks_view_item any[]
---@field texpos_button_stocks_forbid any[]
---@field texpos_button_stocks_forbid_active any[]
---@field texpos_button_stocks_dump any[]
---@field texpos_button_stocks_dump_active any[]
---@field texpos_button_stocks_melt any[]
---@field texpos_button_stocks_melt_active any[]
---@field texpos_button_stocks_hide any[]
---@field texpos_button_stocks_hide_active any[]
---@field texpos_button_short_forbid any[]
---@field texpos_button_short_forbid_active any[]
---@field texpos_button_short_dump any[]
---@field texpos_button_short_dump_active any[]
---@field texpos_button_short_melt any[]
---@field texpos_button_short_melt_active any[]
---@field texpos_button_short_hide any[]
---@field texpos_button_short_hide_active any[]
---@field texpos_building_short_item_task any[]
---@field texpos_building_item_task any[]
---@field texpos_building_item_incorporated any[]
---@field texpos_building_item_trade any[]
---@field texpos_building_item_animal any[]
---@field texpos_building_item_bait any[]
---@field texpos_building_item_loaded any[]
---@field texpos_building_item_dead any[]
---@field texpos_building_item_other any[]
---@field texpos_building_jobs_repeat any[]
---@field texpos_building_jobs_repeat_active any[]
---@field texpos_building_jobs_do_now any[]
---@field texpos_building_jobs_do_now_active any[]
---@field texpos_building_jobs_suspended any[]
---@field texpos_building_jobs_suspended_active any[]
---@field texpos_building_jobs_priority_up any[]
---@field texpos_building_jobs_remove any[]
---@field texpos_building_jobs_active any[]
---@field texpos_building_jobs_quota any[]
---@field texpos_building_jobs_remove_worker any[]
---@field texpos_button_assign_trade any[]
---@field texpos_button_building_info any[]
---@field texpos_button_building_sheet any[]
---@field texpos_button_unit_sheet any[]
---@field texpos_button_large_unit_sheet any[]
---@field texpos_button_pets_livestock any[]
---@field texpos_liquid_numbers_on any[]
---@field texpos_liquid_numbers_off any[]
---@field texpos_ramp_arrows_on any[]
---@field texpos_ramp_arrows_off any[]
---@field texpos_zoom_in_on any[]
---@field texpos_zoom_in_off any[]
---@field texpos_zoom_out_on any[]
---@field texpos_zoom_out_off any[]
---@field texpos_legends_tab_page_left any[]
---@field texpos_legends_tab_page_right any[]
---@field texpos_legends_tab_close_inactive integer[]
---@field texpos_legends_tab_close_active integer[]
---@field texpos_help_border any[]
---@field texpos_help_corner any[]
---@field texpos_help_close any[]
---@field texpos_help_hide any[]
---@field texpos_help_reveal any[]
---@field texpos_embark_selected any[]
---@field texpos_embark_not_selected any[]
---@field texpos_embark_expand_y_active any[]
---@field texpos_embark_expand_y_inactive any[]
---@field texpos_embark_contract_y_active any[]
---@field texpos_embark_contract_y_inactive any[]
---@field texpos_embark_expand_x_active integer[]
---@field texpos_embark_expand_x_inactive integer[]
---@field texpos_embark_contract_x_active integer[]
---@field texpos_embark_contract_x_inactive integer[]
---@field texpos_bottom_button_border_nw integer
---@field texpos_bottom_button_border_w integer
---@field texpos_bottom_button_border_n integer
---@field texpos_bottom_button_border_interior integer
---@field texpos_bottom_button_border_ne integer
---@field texpos_bottom_button_border_e integer
df.interface_setst = nil

---@param x integer
---@param y integer
function df.renderer.update_tile(x, y) end

---@param x integer
---@param y integer
function df.renderer.update_anchor_tile(x, y) end

---@param x integer
---@param y integer
function df.renderer.update_top_tile(x, y) end

---@param x integer
---@param y integer
function df.renderer.update_top_anchor_tile(x, y) end

---@param vp graphic_viewportst
---@param x integer
---@param y integer
function df.renderer.update_viewport_tile(vp, x, y) end

---@param vp graphic_map_portst
---@param x integer
---@param y integer
function df.renderer.update_map_port_tile(vp, x, y) end

function df.renderer.update_all() end

function df.renderer.do_blank_screen_fill() end

---@param vp graphic_viewportst
function df.renderer.update_full_viewport(vp) end

---@param vp graphic_map_portst
function df.renderer.update_full_map_port(vp) end

function df.renderer.clean_tile_cache() end

function df.renderer.render() end

function df.renderer.set_fullscreen() end

---@param arg_0 zoom_commands
function df.renderer.zoom(arg_0) end

---@param w integer
---@param h integer
function df.renderer.resize(w, h) end

---@param w integer
---@param h integer
function df.renderer.grid_resize(w, h) end

---@param nfactor integer
function df.renderer.set_viewport_zoom_factor(nfactor) end

---@param px integer
---@param py integer
---@param x integer
---@param y integer
---@return boolean
function df.renderer.get_precise_mouse_coords(px, py, x, y) end

---@param cur_tx integer
---@param cur_ty integer
function df.renderer.get_current_interface_tile_dims(cur_tx, cur_ty) end

---@return boolean
function df.renderer.uses_opengl() end

---@class renderer
---@field screen any[]
---@field screentexpos any[]
---@field screentexpos_lower any[]
---@field screentexpos_anchored any[]
---@field screentexpos_anchored_x any[]
---@field screentexpos_anchored_y any[]
---@field screentexpos_flag any[]
---@field screen_top any[]
---@field screentexpos_top any[]
---@field screentexpos_top_lower any[]
---@field screentexpos_top_anchored any[]
---@field screentexpos_top_anchored_x any[]
---@field screentexpos_top_anchored_y any[]
---@field screentexpos_top_flag any[]
---@field directtexcopy any[]
---@field screen_old any[]
---@field screentexpos_old any[]
---@field screentexpos_lower_old any[]
---@field screentexpos_anchored_old any[]
---@field screentexpos_anchored_x_old any[]
---@field screentexpos_anchored_y_old any[]
---@field screentexpos_flag_old any[]
---@field screen_top_old any[]
---@field screentexpos_top_old any[]
---@field screentexpos_top_lower_old any[]
---@field screentexpos_top_anchored_old any[]
---@field screentexpos_top_anchored_x_old any[]
---@field screentexpos_top_anchored_y_old any[]
---@field screentexpos_top_flag_old any[]
---@field directtexcopy_old any[]
---@field screentexpos_refresh_buffer any[]
df.renderer = nil

---@param w integer
---@param h integer
---@return boolean
function df.renderer_2d_base.init_video(w, h) end

---@class renderer_2d_base
-- inherited from renderer
---@field screen any[]
---@field screentexpos any[]
---@field screentexpos_lower any[]
---@field screentexpos_anchored any[]
---@field screentexpos_anchored_x any[]
---@field screentexpos_anchored_y any[]
---@field screentexpos_flag any[]
---@field screen_top any[]
---@field screentexpos_top any[]
---@field screentexpos_top_lower any[]
---@field screentexpos_top_anchored any[]
---@field screentexpos_top_anchored_x any[]
---@field screentexpos_top_anchored_y any[]
---@field screentexpos_top_flag any[]
---@field directtexcopy any[]
---@field screen_old any[]
---@field screentexpos_old any[]
---@field screentexpos_lower_old any[]
---@field screentexpos_anchored_old any[]
---@field screentexpos_anchored_x_old any[]
---@field screentexpos_anchored_y_old any[]
---@field screentexpos_flag_old any[]
---@field screen_top_old any[]
---@field screentexpos_top_old any[]
---@field screentexpos_top_lower_old any[]
---@field screentexpos_top_anchored_old any[]
---@field screentexpos_top_anchored_x_old any[]
---@field screentexpos_top_anchored_y_old any[]
---@field screentexpos_top_flag_old any[]
---@field directtexcopy_old any[]
---@field screentexpos_refresh_buffer any[]
-- end renderer
---@field window integer SDL_Window*
---@field sdl_renderer integer SDL_Renderer*
---@field screen_tex integer SDL_Texture*
---@field tile_cache stl-map map<texture_fullid, SDL_Texture*\>
---@field dispx integer
---@field dispy integer
---@field dimx integer
---@field dimy integer
---@field dispx_z integer
---@field dispy_z integer
---@field origin_x integer
---@field origin_y integer
---@field cur_w integer
---@field cur_h integer
---@field use_viewport_zoom boolean
---@field viewport_zoom_factor integer
---@field textures_to_destroy any[] svector<texture_fullid>
---@field ttfs_to_render integer std::list<pair<SDL_Surface*, SDL_Rect>>
---@field zoom_steps integer
---@field forced_steps integer
---@field natural_w integer
---@field natural_h integer
df.renderer_2d_base = nil

---@class renderer_2d
-- inherited from renderer_2d_base
-- inherited from renderer
---@field screen any[]
---@field screentexpos any[]
---@field screentexpos_lower any[]
---@field screentexpos_anchored any[]
---@field screentexpos_anchored_x any[]
---@field screentexpos_anchored_y any[]
---@field screentexpos_flag any[]
---@field screen_top any[]
---@field screentexpos_top any[]
---@field screentexpos_top_lower any[]
---@field screentexpos_top_anchored any[]
---@field screentexpos_top_anchored_x any[]
---@field screentexpos_top_anchored_y any[]
---@field screentexpos_top_flag any[]
---@field directtexcopy any[]
---@field screen_old any[]
---@field screentexpos_old any[]
---@field screentexpos_lower_old any[]
---@field screentexpos_anchored_old any[]
---@field screentexpos_anchored_x_old any[]
---@field screentexpos_anchored_y_old any[]
---@field screentexpos_flag_old any[]
---@field screen_top_old any[]
---@field screentexpos_top_old any[]
---@field screentexpos_top_lower_old any[]
---@field screentexpos_top_anchored_old any[]
---@field screentexpos_top_anchored_x_old any[]
---@field screentexpos_top_anchored_y_old any[]
---@field screentexpos_top_flag_old any[]
---@field directtexcopy_old any[]
---@field screentexpos_refresh_buffer any[]
-- end renderer
---@field window integer SDL_Window*
---@field sdl_renderer integer SDL_Renderer*
---@field screen_tex integer SDL_Texture*
---@field tile_cache stl-map map<texture_fullid, SDL_Texture*\>
---@field dispx integer
---@field dispy integer
---@field dimx integer
---@field dimy integer
---@field dispx_z integer
---@field dispy_z integer
---@field origin_x integer
---@field origin_y integer
---@field cur_w integer
---@field cur_h integer
---@field use_viewport_zoom boolean
---@field viewport_zoom_factor integer
---@field textures_to_destroy any[] svector<texture_fullid>
---@field ttfs_to_render integer std::list<pair<SDL_Surface*, SDL_Rect>>
---@field zoom_steps integer
---@field forced_steps integer
---@field natural_w integer
---@field natural_h integer
-- end renderer_2d_base
df.renderer_2d = nil

---@enum zoom_commands
df.zoom_commands = {
    zoom_in = 0,
    zoom_out = 1,
    zoom_reset = 2,
    zoom_fullscreen = 3,
    zoom_resetgrid = 4,
}

---@param arg_0 interface_key
---@return string
function df.global.enabler.GetKeyDisplay(arg_0) end

---@class enabler_textures
---@field raws integer[]
---@field free_spaces integer[]
---@field init_texture_size integer
---@field uploaded boolean
---@field gl_catalog integer
---@field gl_texpos integer

---@class enabler_async_zoom
---@field mtx stl-mutex
---@field cv stl-condition-variable
---@field vals stl-deque

---@class enabler_async_frombox
---@field mtx stl-mutex
---@field cv stl-condition-variable
---@field vals stl-deque

---@class enabler_async_tobox
---@field mtx stl-mutex
---@field cv stl-condition-variable
---@field vals stl-deque

---@class enabler
---@field unnamed_enabler_1 extra-include
---@field fullscreen_state bitfield
---@field overridden_grid_sizes stl-deque
---@field renderer renderer
---@field calculated_fps integer
---@field calculated_gfps integer
---@field frame_timings integer
---@field gframe_timings integer
---@field frame_sum integer
---@field gframe_sum integer
---@field frame_last integer
---@field gframe_last integer
---@field fps number
---@field gfps number
---@field fps_per_gfps number
---@field last_tick integer
---@field outstanding_frames number
---@field outstanding_gframes number
---@field async_frames integer
---@field async_paused boolean
---@field async_tobox enabler_async_tobox
---@field async_frombox enabler_async_frombox
---@field async_zoom enabler_async_zoom
---@field async_fromcomplete integer
---@field renderer_threadid number
---@field must_do_render_things_before_display boolean
---@field command_line string
---@field flag bitfield
---@field mouse_lbut integer
---@field mouse_rbut integer
---@field mouse_lbut_down integer
---@field mouse_rbut_down integer
---@field mouse_lbut_lift integer
---@field mouse_rbut_lift integer
---@field mouse_mbut integer
---@field mouse_mbut_down integer
---@field mouse_mbut_lift integer
---@field tracking_on integer
---@field textures enabler_textures
---@field simticks integer
---@field gputicks integer
---@field clock integer An *approximation* of the current time for use in garbage collection thingies, updated every frame or so.
---@field mouse_focus boolean
---@field last_text_input integer[]
df.global.enabler = nil

---@enum justification from libgraphics
df.justification = {
    justify_left = 0,
    justify_center = 1,
    justify_right = 2,
    justify_cont = 3,
    not_truetype = 4,
}

---@class tile_pagest
---@field token string
---@field graphics_dir string
---@field filename string
---@field tile_dim_x integer
---@field tile_dim_y integer
---@field page_dim_x integer
---@field page_dim_y integer
---@field texpos number[]
---@field datapos number[]
---@field texpos_gs number[]
---@field datapos_gs number[]
---@field loaded boolean
df.tile_pagest = nil

---@class palette_pagest
---@field token string
---@field graphics_dir string
---@field filename string
---@field default_row integer
---@field color_token string[]
---@field color_row integer[]
df.palette_pagest = nil

---@class texture_handlerst
---@field page tile_pagest[]
---@field palette palette_pagest[]
df.texture_handlerst = nil


