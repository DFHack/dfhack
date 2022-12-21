#pragma once

#include "imtui/imtui.h"
#include <string>
#include <vector>
#include <set>
#include <array>
#include "df/interface_key.h"

struct ImGuiContext;

namespace ImTuiInterop
{
	struct user_data
	{
		bool should_pass_keyboard_up = false;
		bool suppress_next_keyboard_passthrough = false;
	};

	int name_to_colour_index(const std::string& name);
	//fg, bg, bold
	ImVec4 colour_interop(std::vector<int> col3);
	ImVec4 colour_interop(std::vector<double> col3);
	ImVec4 named_colours(const std::string& fg, const std::string& bg, bool bold);

	struct ui_state;

	namespace impl
	{
		void init_current_context();

		void new_frame(std::set<df::interface_key> keys, ui_state& st);

		void draw_frame();

		void shutdown();
	}

	struct ui_state
	{
		static user_data& get_user_data();

		std::set<df::interface_key> unprocessed_keys;
		std::array<int, 2> pressed_mouse_keys = {};
		std::map<df::interface_key, int> danger_key_frames;

		ImGuiContext* last_context;
		ImGuiContext* ctx;

		ui_state();
		~ui_state();

		void feed(std::set<df::interface_key> keys);

		void activate();

		void new_frame();

		void draw_frame();

		void deactivate();
	};

	//returns an inactive imgui context system
	ui_state make_ui_system();
}