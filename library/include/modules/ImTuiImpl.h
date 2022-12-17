#pragma once

#include "imtui/imtui.h"

struct ImGuiContext;

namespace ImTuiInterop
{
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