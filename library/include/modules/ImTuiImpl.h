#pragma once

#include <string>
#include <vector>
#include <set>
#include <array>
#include <map>
#include "df/interface_key.h"
#include "imgui.h"

namespace ImTuiInterop
{
    int name_to_colour_index(const std::string& name);
    //fg, bg, bold
    ImVec4 colour_interop(std::vector<int> col3);
    ImVec4 colour_interop(std::vector<double> col3);
    ImVec4 named_colours(const std::string& fg, const std::string& bg, bool bold);

    namespace viewscreen
    {
        //Any window that wants to have its rendering correctly respect dfs viewscreen order
        //needs to set this (after a call to Begin, or a widget that calls Begin internally)
        /*
        bool should_show = ImGui::Begin("My Window");

        viewscreen::claim_current_imgui_window();

        if(should_show)
            //content

        ImGui::End();
        */
        void claim_current_imgui_window();

        void suppress_next_keyboard_feed_upwards();
        void suppress_next_mouse_feed_upwards();
        void feed_upwards();
        //if this key is pressed, input will not be passed upwards
        void declare_suppressed_key(df::interface_key key);

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
