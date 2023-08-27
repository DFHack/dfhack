-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum init_display_flags
df.init_display_flags = {
    USE_GRAPHICS = 0,
    SOFTWARE = 1,
    TEXT = 2,
    NOT_RESIZABLE = 3,
    INTERFACE_SCALING_TO_DESIRED_HEIGHT_WIDTH = 4,
}

---@enum init_display_filter_mode
df.init_display_filter_mode = {
    AUTO = 0,
    NEAREST = 1,
    LANCZOS = 2,
}

---@enum init_display_windowed
df.init_display_windowed = {
    True = 0,
    False = 1,
    Prompt = 2,
    Exclusive = 3,
}
---@class init_display
---@field flag init_display_flags[]
---@field init_display_windowed init_display_windowed
---@field grid_x integer
---@field grid_y integer
---@field desired_fullscreen_width integer
---@field desired_fullscreen_height integer
---@field actual_fullscreen_width integer
---@field actual_fullscreen_height integer
---@field desired_windowed_width integer
---@field desired_windowed_height integer
---@field actual_windowed_width integer
---@field actual_windowed_height integer
---@field max_interface_percentage integer
---@field interface_scaling_desired_width integer
---@field interface_scaling_desired_height integer
---@field interface_scaling_percentage integer
---@field partial_print_count integer
---@field filter_mode init_display_filter_mode
df.init_display = nil

---@enum init_media_flags
df.init_media_flags = {
    SOUND_OFF = 0,
    UNUSED_01_02 = 1,
    COMPRESS_SAVES = 2,
}

---@class init_media
---@field flag init_media_flags[]
---@field volume_master integer
---@field volume_music integer
---@field volume_ambience integer
---@field volume_sfx integer
---@field time_between_songs integer
df.init_media = nil

---@enum init_input_flags
df.init_input_flags = {
    MOUSE_OFF = 0,
    MOUSE_PICTURE = 1,
}

---@class init_input
---@field hold_time integer
---@field repeat_time integer
---@field macro_time integer
---@field pause_zoom_no_interface_ms integer
---@field flag init_input_flags[]
---@field zoom_speed integer
---@field repeat_accel_start integer
---@field repeat_accel_limit integer
df.init_input = nil

---@class init_font
---@field basic_font_texpos number[]
---@field small_font_texpos number[]
---@field large_font_texpos number[]
---@field basic_font_datapos number[]
---@field small_font_datapos number[]
---@field large_font_datapos number[]
---@field basic_font_texpos_top number[]
---@field small_font_texpos_top number[]
---@field large_font_texpos_top number[]
---@field basic_font_texpos_bot number[]
---@field small_font_texpos_bot number[]
---@field large_font_texpos_bot number[]
---@field basic_font_adjx number
---@field basic_font_adjy number
---@field small_font_adjx number
---@field small_font_adjy number
---@field large_font_adjx number
---@field large_font_adjy number
---@field basic_font_dispx number
---@field basic_font_dispy number
---@field small_font_dispx number
---@field small_font_dispy number
---@field large_font_dispx number
---@field large_font_dispy number
df.init_font = nil

---@enum init_window_flags
df.init_window_flags = {
    VSYNC_ON = 0,
    VSYNC_OFF = 1,
    TEXTURE_LINEAR = 2,
}

---@class init_window
---@field flag init_window_flags[]
df.init_window = nil

---@class init
---@field display init_display
---@field media init_media
---@field input init_input
---@field font init_font
---@field window init_window
---@field fps_cap integer
---@field gfps_cap integer
---@field load_bar_texpos number[]
---@field intro_button_texpos number[]
---@field texpos_neutral_intro_button integer[]
---@field texpos_confirm_intro_button integer[]
---@field texpos_cancel_intro_button integer[]
---@field texpos_selected_intro_button integer[]
---@field texpos_unselected_intro_button integer[]
---@field texpos_open_list_button integer[]
---@field texpos_increase_button integer[]
---@field texpos_decrease_button integer[]
---@field texpos_nullify_button integer[]
---@field texpos_left_arrow_button integer[]
---@field texpos_right_arrow_button integer[]
---@field texpos_up_arrow_button integer[]
---@field texpos_down_arrow_button integer[]
---@field border_texpos number[]
---@field texpos_border_nw integer
---@field texpos_border_n integer
---@field texpos_border_ne integer
---@field texpos_border_w integer
---@field texpos_border_interior integer
---@field texpos_border_e integer
---@field texpos_border_sw integer
---@field texpos_border_s integer
---@field texpos_border_se integer
---@field texpos_border_join_n integer
---@field texpos_border_join_s integer
---@field texpos_border_join_w integer
---@field texpos_border_join_e integer
---@field texpos_border_inside_nswe integer
---@field texpos_border_inside_nsw integer
---@field texpos_border_inside_nse integer
---@field texpos_border_inside_nwe integer
---@field texpos_border_inside_swe integer
---@field texpos_border_inside_ns integer
---@field texpos_border_inside_we integer
---@field scrollbar_texpos number[]
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
---@field filter_texpos number[]
---@field texpos_button_filter any[]
---@field texpos_button_filter_name any[]
---@field tabs_texpos number[]
---@field texpos_tab_unselected any[]
---@field texpos_tab_selected any[]
---@field classic_load_bar_texpos number[]
---@field classic_texpos_neutral_intro_button integer[]
---@field classic_texpos_confirm_intro_button integer[]
---@field classic_texpos_cancel_intro_button integer[]
---@field classic_texpos_selected_intro_button integer[]
---@field classic_texpos_unselected_intro_button integer[]
---@field classic_texpos_open_list_button integer[]
---@field classic_texpos_increase_button integer[]
---@field classic_texpos_decrease_button integer[]
---@field classic_texpos_nullify_button integer[]
---@field classic_texpos_left_arrow_button integer[]
---@field classic_texpos_right_arrow_button integer[]
---@field classic_texpos_up_arrow_button integer[]
---@field classic_texpos_down_arrow_button integer[]
---@field classic_texpos_border_nw integer
---@field classic_texpos_border_n integer
---@field classic_texpos_border_ne integer
---@field classic_texpos_border_w integer
---@field classic_texpos_border_interior integer
---@field classic_texpos_border_e integer
---@field classic_texpos_border_sw integer
---@field classic_texpos_border_s integer
---@field classic_texpos_border_se integer
---@field classic_texpos_border_join_n integer
---@field classic_texpos_border_join_s integer
---@field classic_texpos_border_join_w integer
---@field classic_texpos_border_join_e integer
---@field classic_texpos_border_inside_nswe integer
---@field classic_texpos_border_inside_nsw integer
---@field classic_texpos_border_inside_nse integer
---@field classic_texpos_border_inside_nwe integer
---@field classic_texpos_border_inside_swe integer
---@field classic_texpos_border_inside_ns integer
---@field classic_texpos_border_inside_we integer
---@field classic_texpos_scrollbar any[]
---@field classic_texpos_scrollbar_up_hover integer[]
---@field classic_texpos_scrollbar_up_pressed integer[]
---@field classic_texpos_scrollbar_down_hover integer[]
---@field classic_texpos_scrollbar_down_pressed integer[]
---@field classic_texpos_scrollbar_small_scroller any[]
---@field classic_texpos_scrollbar_small_scroller_hover any[]
---@field classic_texpos_scrollbar_top_scroller integer[]
---@field classic_texpos_scrollbar_top_scroller_hover integer[]
---@field classic_texpos_scrollbar_bottom_scroller integer[]
---@field classic_texpos_scrollbar_bottom_scroller_hover integer[]
---@field classic_texpos_scrollbar_blank_scroller integer[]
---@field classic_texpos_scrollbar_blank_scroller_hover integer[]
---@field classic_texpos_scrollbar_center_scroller integer[]
---@field classic_texpos_scrollbar_center_scroller_hover integer[]
---@field classic_texpos_scrollbar_offcenter_scroller any[]
---@field classic_texpos_scrollbar_offcenter_scroller_hover any[]
---@field classic_texpos_button_filter any[]
---@field classic_texpos_button_filter_name any[]
---@field classic_texpos_tab_unselected any[]
---@field classic_texpos_tab_selected any[]
df.global.init = nil


