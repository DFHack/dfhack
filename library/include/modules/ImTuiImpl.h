#pragma once

#include <string>
#include <vector>
#include <set>
#include <array>
#include <map>
#include "df/interface_key.h"
#include "imgui.h"

struct ImGuiContext;

namespace ImTuiInterop
{
    int name_to_colour_index(const std::string& name);
    //fg, bg, bold
    ImVec4 colour_interop(std::vector<int> col3);
    ImVec4 colour_interop(std::vector<double> col3);
    ImVec4 named_colours(const std::string& fg, const std::string& bg, bool bold);

    struct ui_state;

    ui_state& get_global_ui_state();

    namespace impl
    {
        void init_current_context();

        void new_frame(std::set<df::interface_key> keys, ui_state& st);

        void draw_frame(ImDrawData* drawData);

        void shutdown();

        void reset_input();
    }

    struct ui_state
    {
        std::set<df::interface_key> unprocessed_keys;
        std::array<int, 2> pressed_mouse_keys = {};
        std::map<df::interface_key, int> danger_key_frames;

        int render_stack = 0;
        std::map<int, std::vector<std::string>> windows;
        std::set<std::string> rendered_windows;
        bool should_pass_keyboard_up = false;
        bool suppress_next_keyboard_passthrough = false;
        //render_stack -> std::set<int>
        std::map<int, std::set<int>> suppressed_keys;

        ImGuiContext* last_context;
        ImGuiContext* ctx;

        ui_state();
        ~ui_state();

        void feed(std::set<df::interface_key> keys);

        void activate();

        void new_frame();

        void draw_frame(ImDrawData* drawData);

        void deactivate();
        void reset_input();
    };

    //returns an inactive imgui context system
    ui_state make_ui_system();

    namespace viewscreen
    {
        //on_top for render and feed do not need to be the same viewscreen
        //but they *do* need to only be called with true once, after the last on_*_end
        //returns an id that should be passed to on_render_end
        int on_render_start(bool is_top);
        void on_render_end(bool is_top, int id);

        void on_feed_start(bool is_top, std::set<df::interface_key>* keys);
        //returns true if you should call parent->feed(keys)
        bool on_feed_end(std::set<df::interface_key>* keys);

        void on_dismiss_final_imgui_aware_viewscreen();
    }
}
