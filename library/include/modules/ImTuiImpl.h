#pragma once

#include "imtui/imtui.h"
#include <string>
#include <vector>

struct ImGuiContext;

namespace ImTuiInterop
{
	int name_to_colour_index(const std::string& name);
	//fg, bg, bold
	ImVec4 colour_interop(std::vector<int> col3);
	ImVec4 named_colours(const std::string& fg, const std::string& bg, bool bold);

	namespace impl
	{
		void init_current_context();

		void new_frame();

		void draw_frame();

		void shutdown();
	}

	struct ui_state
	{
		ImGuiContext* last_context;
		ImGuiContext* ctx;

		ui_state();

		void activate();

		void new_frame();

		void draw_frame();

		void deactivate();
	};

	//returns an inactive imgui context system
	ui_state make_ui_system();
}