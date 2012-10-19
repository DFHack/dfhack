// Dwarf Manipulator - a Therapist-style labor editor

#include <modules/Screen.h>
#include <modules/Translation.h>
#include <modules/Units.h>
#include <MiscUtils.h>

#include <VTableInterpose.h>

#include "df/viewscreen_unitlistst.h"
#include "df/viewscreen_storesst.h"
#include "df/interface_key.h"

using std::set;
using std::vector;
using std::string;

using namespace DFHack;
using namespace df::enums;

using df::global::gps;


void OutputString(int8_t color, int &x, int y, const std::string &text)
{
    Screen::paintString(Screen::Pen(' ', color, 0), x, y, text);
    x += text.length();
}

struct search_struct
{
	static string search_string;
	static bool entry_mode;

	void print_search_option(int &x)
	{
		OutputString(12, x, gps->dimy - 2, "s");
		OutputString(15, x, gps->dimy - 2, ": Search");
		if (search_string.length() > 0 || entry_mode)
			OutputString(15, x, gps->dimy - 2, ": " + search_string);
		if (entry_mode)
			OutputString(12, x, gps->dimy - 2, "_");
	}

	bool process_input(df::interface_key select_key, set<df::interface_key> *input, bool &string_changed)
	{
		bool key_processed = true;
		string_changed = false;

		if (entry_mode)
		{
			df::interface_key last_token = *input->rbegin();
			if (last_token >= interface_key::STRING_A032 && last_token <= interface_key::STRING_A126)
			{
				search_string += 32 + last_token - interface_key::STRING_A032;
				string_changed = true;
			}
			else if (last_token == interface_key::STRING_A000)
			{
				if (search_string.length() > 0)
				{
					search_string.erase(search_string.length()-1);
					string_changed = true;
				}
			}
			else if (input->count(interface_key::SELECT) || input->count(interface_key::LEAVESCREEN))
			{
				entry_mode = false;
			}
			else if  (input->count(interface_key::CURSOR_UP) || input->count(interface_key::CURSOR_DOWN))
			{
				entry_mode = false;
				key_processed = false;
			}
		}
		else if (input->count(select_key))
		{
			entry_mode = true;
		}
		else
		{
			key_processed = false;
		}

		return key_processed;
	}
};

string search_struct::search_string = "";
bool search_struct::entry_mode = false;


struct stocks_search : df::viewscreen_storesst, search_struct
{
	typedef df::viewscreen_storesst interpose_base;

	static vector<df::item* > saved_items;

	static void reset_search()
	{
		entry_mode = false;
		search_string = "";
		saved_items.clear();
	}

	void clear_search()
	{
		if (saved_items.size() > 0)
		{
			items = saved_items;
		}
	}



	DEFINE_VMETHOD_INTERPOSE(void, render, ())
	{
		INTERPOSE_NEXT(render)();

		int x = 1;
		print_search_option(x);
	}

	DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
	{
		if (in_group_mode)
		{
			INTERPOSE_NEXT(feed)(input);
			return;
		}


		bool string_changed = false;

		if ((input->count(interface_key::CURSOR_UP) || input->count(interface_key::CURSOR_DOWN)) && !in_right_list)
		{
			saved_items.clear();
			entry_mode = false;
			if (search_string.length() > 0)
				string_changed = true;
			INTERPOSE_NEXT(feed)(input);
		}
		else
		{
			if (!process_input(interface_key::CUSTOM_S, input, string_changed) && !entry_mode)
			{
				INTERPOSE_NEXT(feed)(input);
				if (in_group_mode)
				{
					clear_search();
					reset_search();
				}
			}
		}

		if (string_changed)
		{
			if (search_string.length() == 0)
			{
				clear_search();
				return;
			}

			if (saved_items.size() == 0 && items.size() > 0)
			{
				saved_items = items;
			}
			items.clear();

			for (int i = 0; i < saved_items.size(); i++ )
			{
				string search_string_l = toLower(search_string);
				string desc = Items::getDescription(saved_items[i], 0, true);
				if (desc.find(search_string_l) != string::npos)
				{
					items.push_back(saved_items[i]);
				}
			}

			item_cursor = 0;
		}
	}

};

vector<df::item* > stocks_search::saved_items;


IMPLEMENT_VMETHOD_INTERPOSE(stocks_search, feed);
IMPLEMENT_VMETHOD_INTERPOSE(stocks_search, render);




struct unitlist_search : df::viewscreen_unitlistst, search_struct
{
	typedef df::viewscreen_unitlistst interpose_base;

	static vector<df::unit*> saved_units;
	static vector<df::job*> saved_jobs;


	static void reset_search()
	{
		entry_mode = false;
		search_string = "";
		saved_units.clear();
		saved_jobs.clear();
	}

	void clear_search()
	{
		if (saved_units.size() > 0)
		{
			units[page] = saved_units;
			jobs[page] = saved_jobs;
		}
	}

	void do_search()
	{
		if (search_string.length() == 0)
		{
			clear_search();
			return;
		}

		while(1)
		{
			if (saved_units.size() == 0)
			{
				saved_units = units[page];
				saved_jobs = jobs[page];
			}
			units[page].clear();
			jobs[page].clear();
		
			for (int i = 0; i < saved_units.size(); i++ )
			{
				df::unit *unit = saved_units[i];
				string search_string_l = toLower(search_string);
				string name = toLower(Translation::TranslateName(Units::getVisibleName(unit), false));
				if (name.find(search_string_l) != string::npos)
				{
					units[page].push_back(unit);
					jobs[page].push_back(saved_jobs[i]);
				}
			}

			if (units[page].size() > 0)
			{
				cursor_pos[page] = 0;
				break;
			}

			search_string.erase(search_string.length()-1);
		}
	}

	DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
	{
		bool string_changed = false;
		if (!process_input(interface_key::CUSTOM_S, input, string_changed))
		{
			if (!entry_mode)
			{
				if (input->count(interface_key::CURSOR_LEFT) || input->count(interface_key::CURSOR_RIGHT))
				{
					clear_search();
					reset_search();
				}
				INTERPOSE_NEXT(feed)(input);
			}
		}
		else if (string_changed)
			do_search();
	}

	DEFINE_VMETHOD_INTERPOSE(void, render, ())
	{
		INTERPOSE_NEXT(render)();

		if (units[page].size())
		{
			int x = 28;
			print_search_option(x);
		}
	}

};

vector<df::unit*> unitlist_search::saved_units;
vector<df::job*> unitlist_search::saved_jobs;


IMPLEMENT_VMETHOD_INTERPOSE(unitlist_search, feed);
IMPLEMENT_VMETHOD_INTERPOSE(unitlist_search, render);




DFHACK_PLUGIN("search");


DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    if (!gps || !INTERPOSE_HOOK(unitlist_search, feed).apply() || !INTERPOSE_HOOK(unitlist_search, render).apply()
		|| !INTERPOSE_HOOK(stocks_search, feed).apply() || !INTERPOSE_HOOK(stocks_search, render).apply())
        out.printerr("Could not insert Search hooks!\n");

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    INTERPOSE_HOOK(unitlist_search, feed).remove();
    INTERPOSE_HOOK(unitlist_search, render).remove();
	INTERPOSE_HOOK(stocks_search, feed).remove();
	INTERPOSE_HOOK(stocks_search, render).remove();
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange ( color_ostream &out, state_change_event event )
{
	unitlist_search::reset_search();
	return CR_OK;
}