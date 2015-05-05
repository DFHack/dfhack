//standard DFHack files
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include "DataDefs.h"

//access to hotkey enums. Hotkeys are in 'interface_key.h'
#include "uicommon.h"

//access to colors for printing on screen
#include <ColorText.h>

//viewscreens with data we need
#include "df/viewscreen_choose_start_sitest.h"
#include "df/viewscreen_layer_world_gen_paramst.h"
#include "df/viewscreen_layer_world_gen_param_presetst.h"
#include "df/viewscreen_loadgamest.h"
#include "df/viewscreen_movieplayerst.h"
#include "df/viewscreen_new_regionst.h"
#include "df/viewscreen_setupdwarfgamest.h"
#include "df/viewscreen_textviewerst.h"
#include "df/viewscreen_titlest.h"

//non-viewscreen sources of game data
//#include "df/language_name.h"
#include "df/world_data.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums::interface_key;

DFHACK_PLUGIN("menu-mouse");

REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(enabler);

#define PLUGIN_VERSION 0.12

//viewscreens depend on these global variables.
color_ostream* pout;
df::viewscreen_titlest* titleSpoof;

void sendInput(df::interface_key ik)
{
	set<df::interface_key> tmp;
	tmp.insert(ik);
	Core::getTopViewscreen()->feed(&tmp);
}

//basic elements of widgets
struct Rect
{
	int top;
	int bottom;
	int right;
	int left;
};

class Item
{
public:
	int row;
	int col;
	string display;
	virtual void action(){}
};

class HotkeyItem : public Item
{
public:
	df::enums::interface_key::interface_key hotkey;

	void action()
	{
		sendInput(hotkey);
	}
};

class KeyboardItem : public Item
{
public:
	vector<string> value;

	KeyboardItem(int r, int c, string val, string altVal)
	{
		col = c;
		row = r;
		value.push_back(val);
		value.push_back(altVal);
		display = value[0];
	}
};

class RadioItem : public Item
{
public:
	int32_t* index;
	int value;

	void action()
	{
		*index = value;
	}
};

class ViewscreenItem : public Item
{
public:
	dfhack_viewscreen* view;

	void action()
	{
		Screen::show(view);
	}

	void erase()
	{
		delete view;
	}
};

#define DEFINE_HOTKEY_ITEM(element, vect, ro, co, dis, hot)	\
	element = new HotkeyItem();		\
	element->row = ro;				\
	element->col = co;				\
	element->display = dis;			\
	element->hotkey = hot;			\
	vect.push_back(element)

#define DEFINE_KEYBOARD_ITEM(element, vect, ro, co, dis, alt)	\
	element = new KeyboardItem(ro, co, dis, alt);	\
	vect.push_back(element)

#define DEFINE_RADIO_ITEM(element, vect, ro, co, dis, in, val)	\
	element = new RadioItem();		\
	element->row = ro;				\
	element->col = co;				\
	element->display = dis;			\
	element->index = in;			\
	element->value = val;			\
	vect.push_back(element)

//basic widgets
struct Widget
{
	Rect dims; 
	color_value fg;
	color_value bg;

	void draw() {}
	void erase() {}
	void init() { dims.left = 0; dims.right = 0; dims.top = 0; dims.bottom = 0; fg = COLOR_WHITE; bg = COLOR_BLACK; }
	void mouseDown(int x, int y) {}
	void mouseOver(int x, int y) {}
	void mouseUp(int x, int y) {}
	bool over(int x, int y){ return (x >= dims.left && x <= dims.right && y >= dims.top && y <= dims.bottom); }
};

struct CloseWidget : public Widget
{
	void draw()
	{
		fg = COLOR_WHITE;
		Screen::paintTile(Screen::Pen('>', fg, bg), dims.left, dims.top);
		Screen::paintTile(Screen::Pen('<', fg, bg), dims.right, dims.bottom);
	}

	void init()
	{
		Widget::init();
		fg = COLOR_WHITE;
		bg = COLOR_GREY;
	}

	void mouseDown(int x, int y) {}

	void mouseOver(int x, int y)
	{
		if (over(x, y))
			bg = COLOR_LIGHTRED;
		else
			bg = COLOR_GREY;
	}

	void mouseUp(int x, int y)
	{
		if (over(x, y))
			sendInput(LEAVESCREEN);
	}
};

struct CloseSpoofWidget : public CloseWidget
{
	void mouseUp(int x, int y)
	{
		if (over(x, y))
			sendInput(LEAVESCREEN_ALL);

		//INTERPOSE_HOOKS(title_menu_hook, true);
	}
};

struct DynamicWidget : public Widget
{
protected:
	int* index;
	int indexMax;

public:
	bool indexSet;
	bool indexMaxSet;

	signed int offset;

	bool swiped;

	int xDown;
	int yDown;

	void draw() {}

	void setIndex(int* ip)
	{
		index = ip;
		indexSet = true;
	}

	void setIndexMax(int i)
	{
		indexMax = i;
		indexMaxSet = true;
	}

	void init()
	{
		Widget::init();
		indexSet = false;
		indexMaxSet = false;
		offset = 0;
		swiped = false;
		xDown = -1;
		yDown = -1;
	}

	void mouseDown(int x, int y)
	{
		if (over(x, y))
		{
			xDown = x;
			yDown = y;
		}
	}

	void mouseOver(int x, int y)
	{
		if (indexSet && indexMaxSet)
		{
			int newIndex;

			if (over(x, y))
			{
				if (yDown > -1 && !swiped && y != yDown)
				{
					if (y < yDown)
						offset++;
					else
						offset--;

					swiped = true;
				}

				if (offset < 0)
					offset = 0;
				else if (indexMax - offset * (dims.bottom - dims.top + 1) <= 0)
					offset--;

				newIndex = y - dims.top + offset * (dims.bottom - dims.top + 1);

				if (newIndex <= offset * (dims.bottom - dims.top + 1))
					newIndex = offset * (dims.bottom - dims.top + 1);
				else if (newIndex >= indexMax)
					newIndex = indexMax - 1;

				*index = newIndex;
			}
			else
			{
				xDown = -1;
				yDown = -1;
			}
		}
	}

	void mouseUp(int x, int y)
	{
		if (over(x, y))
		{
			if (y == yDown)
				sendInput(SELECT);
		}

		swiped = false;
		xDown = -1;
		yDown = -1;
	}
};

struct KeyboardWidget : public Widget
{
	int index;
	bool shift;
	bool caps;

	std::vector<KeyboardItem*> items;

	void erase()
	{
		for (int i = 0; i < items.size(); i++)
			delete items[i];
	}

	void draw()
	{
		int row = 0;
		int col = 0;
		string prefix = " ";
		string suffix = " ";
		int ii = 0;
		int alternate = shift || caps;

		items[ii]->display = prefix;
		items[ii]->display.append(items[ii]->value[alternate]);
		items[ii]->display.append(suffix);
		items[ii]->col = col;
		col = col + items[ii]->display.size() + 1;
		items[ii]->row = row;
		ii++;

		Screen::paintString(Screen::Pen('?', COLOR_WHITE, COLOR_BLACK), items[ii]->col, items[ii]->row, items[index]->display);

		items[ii]->display = prefix;
		items[ii]->display.append(items[ii]->value[alternate]);
		items[ii]->display.append(suffix);
		items[ii]->col = col;
		col = col + items[ii]->display.size() + 1;
		items[ii]->row = row;
		ii++;

		Screen::paintString(Screen::Pen('?', COLOR_WHITE, COLOR_BLACK), items[ii]->col, items[ii]->row, items[index]->display);

		if (index > -1)
		{
			/*
			int writeX = dims.left + items[index]->col;
			int writeY = dims.top + items[index]->row;

			Screen::Pen p = Screen::readTile(writeX, writeY);

			int leng = items[index]->display.size();

			if (writeX + leng >= dims.right)
				leng = dims.right - writeX;

			Screen::paintString(Screen::Pen('?', fg, p.bg), writeX, writeY, items[index]->display.substr(0, leng));
			*/
			//Screen::paintString(p, writeX, writeY, items[index].display.substr(0, leng));

			//toScreen(items[index].col, items[index].row, items[index].display, COLOR_DARKGREY, COLOR_YELLOW, COLOR_GREY, COLOR_BLACK);
		}

	}

	void init()
	{
		Widget::init();
		index = -1;
		shift = false;
		caps = false;
		fg = COLOR_GREY;

		erase();
		items.clear();

		KeyboardItem* ki;
		DEFINE_KEYBOARD_ITEM(ki, items, 0, 0, "`", "~");
		DEFINE_KEYBOARD_ITEM(ki, items, 0, 0, "1", "!");
	}

	void mouseDown(int x, int y){}

	void mouseOver(int x, int y)
	{
		if (over(x, y))
		{
			for (int i = 0; i < items.size(); i++)
			{
				//pout->print("i: %u, c: %u, r: %u\n", i, items[i]->col, items[i]->row);

				if (x >= items[i]->col + dims.left && x < items[i]->col + items[i]->display.size() + dims.left && y == items[i]->row + dims.top)
				{
					index = i;
					return;
				}
			}
		}
		index = -1;
	}

	void mouseUp(int x, int y)
	{
		if (over(x, y))
		{
			if (index > -1)
			{
				//sendInput(items[index].hotkey);
				items[index]->action();
			}
		}
	}
};

struct StaticWidget : public Widget
{
	int index;

	std::vector<Item*> items;

	void erase()
	{
		for (int i = 0; i < items.size(); i++)
			delete items[i];
	}

	void draw()
	{
		if (index > -1)
		{
			
			int writeX = dims.left + items[index]->col;
			int writeY = dims.top + items[index]->row;
			
			Screen::Pen p = Screen::readTile(writeX, writeY);

			int leng = items[index]->display.size();

			if (writeX + leng >= dims.right)
				leng = dims.right - writeX;

			Screen::paintString(Screen::Pen('?', fg, p.bg), writeX, writeY, items[index]->display.substr(0, leng));

			//Screen::paintString(p, writeX, writeY, items[index].display.substr(0, leng));

			//toScreen(items[index].col, items[index].row, items[index].display, COLOR_DARKGREY, COLOR_YELLOW, COLOR_GREY, COLOR_BLACK);
		}

	}

	void init()
	{
		Widget::init();
		index = -1;
		items.clear();
		fg = COLOR_GREY;
	}

	void mouseDown(int x, int y){}

	void mouseOver(int x, int y)
	{
		if (over(x, y))
		{
			for (int i = 0; i < items.size(); i++)
			{
				//pout->print("i: %u, c: %u, r: %u\n", i, items[i]->col, items[i]->row);

				if (x >= items[i]->col + dims.left && x < items[i]->col + items[i]->display.size() + dims.left && y == items[i]->row + dims.top)
				{
					index = i;
					return;
				}
			}
		}
		index = -1;
	}

	void mouseUp(int x, int y)
	{
		if (over(x, y))
		{
			if (index > -1)
			{
				//sendInput(items[index].hotkey);
				items[index]->action();
			}
		}
	}
};

//advanced widgets
struct DynamicMultiWidget : public DynamicWidget
{
	StaticWidget selector;

	void erase()
	{
		selector.erase();
	}

	void draw() 
	{
		selector.dims.top = dims.top + *index - offset * (dims.bottom - dims.top + 1);
		selector.dims.bottom = selector.dims.top;
		selector.dims.left = dims.left;
		selector.dims.right = dims.right;

		//pout->print("t: %u, b: %u, l: %u, r: %u\n", selector.dims.top, selector.dims.bottom, selector.dims.left, selector.dims.right);

		for (int i = 0; i < selector.items.size(); i++)
		{
			
			int writeX = selector.dims.left + selector.items[i]->col;
			int writeY = selector.dims.top + selector.items[i]->row;

			int leng = selector.items[i]->display.size();

			if (writeX + leng >= dims.right)
				leng = dims.right - writeX;

			Screen::paintString(Screen::Pen('?', fg, bg), writeX, writeY, selector.items[i]->display.substr(0, leng));
			

			//selector.toScreen(selector.items[i].col, selector.items[i].row, selector.items[i].display, COLOR_DARKGREY, COLOR_DARKGREY, COLOR_GREY, COLOR_RED);
		}

		selector.draw();
	}

	void setIndex(int* ip)
	{
		DynamicWidget::setIndex(ip);
	}

	void setIndexMax(int i)
	{
		DynamicWidget::setIndexMax(i);
	}

	void init()
	{
		DynamicWidget::init();
		selector.init();
		
		bg = COLOR_GREY;
		fg = COLOR_DARKGREY;
	}

	void mouseDown(int x, int y)
	{
		DynamicWidget::mouseDown(x, y);
		selector.mouseDown(x, y);
	}

	void mouseOver(int x, int y)
	{
		DynamicWidget::mouseOver(x, y);
		selector.mouseOver(x, y);
	}

	void mouseUp(int x, int y)
	{
		DynamicWidget::mouseUp(x, y);
		selector.mouseUp(x, y);
	}
};

struct DynamicSelectWidget : public DynamicWidget
{
	int selectedIndex;

	void draw()
	{
		int cubit = (dims.bottom - dims.top + 1);

		if (selectedIndex >= offset * cubit && selectedIndex < (offset + 1) * cubit)
		{
			int tempY = dims.top + selectedIndex - offset * cubit;

			for (int i = 0; i < dims.right - dims.left + 1; i++)
			{
				Screen::Pen p = Screen::readTile(dims.left + i, tempY);

				paintTile(Screen::Pen(p.ch, fg, bg), dims.left + i, tempY);
			}
		}
	}

	void init()
	{
		DynamicWidget::init();
		selectedIndex = 0;
	}

	void mouseDown(int x, int y)
	{
		DynamicWidget::mouseDown(x, y);
	}

	void mouseOver(int x, int y)
	{
		if (over(x, y))
		{
			DynamicWidget::mouseOver(x, y);
		}
		else
		{
			*index = selectedIndex;
		}
	}

	void mouseUp(int x, int y)
	{
		if (over(x, y))
		{
			if (y == yDown)
				selectedIndex = *index;
		}

		swiped = false;
		xDown = -1;
		yDown = -1;
	}
};

CloseWidget closeWidget;
CloseSpoofWidget closeSpoofWidget;

//basic menus
struct Menu
{
	bool closeable;
	bool mouseLHistory;
	CloseWidget* close;
	Rect dims;

	virtual void calcDims(){ close->dims.top = dims.top; close->dims.bottom = dims.top; close->dims.left = dims.right - 1; close->dims.right = dims.right; }
	virtual void draw(){ if (closeable) close->draw(); }
	virtual void erase(){}
	virtual void feed(){}
	virtual void init(bool canClose){ close = &closeWidget; dims.left = 0; dims.right = 0; dims.top = 0; dims.bottom = 0; closeable = canClose;  mouseLHistory = false; close->init(); }
	virtual void mouseDown(int x, int y){ if (closeable) close->mouseDown(x, y); }
	virtual void mouseOver(int x, int y) { if (closeable) close->mouseOver(x, y); }
	virtual void mouseUp(int x, int y){ if (closeable) close->mouseUp(x, y); }
	virtual void logic()
	{
		if (gps && enabler)
		{
			int x = gps->mouse_x;
			int y = gps->mouse_y;
			bool mouseLCurrent = enabler->mouse_lbut;

			this->calcDims();

			this->mouseOver(x, y);

			if (mouseLCurrent != this->mouseLHistory)
			{
				if (mouseLCurrent)
					this->mouseDown(x, y);
				else
					this->mouseUp(x, y);

				this->mouseLHistory = mouseLCurrent;
			}
		}
	}
};

struct KeyboardMenu : public Menu
{
	KeyboardWidget keys;

	string text;
	string* pText;
	bool pointerSet;

	void calcDims()
	{
		dims.top = 0;
		dims.bottom = Screen::getWindowSize().y - 1;
		dims.left = 0;
		dims.right = Screen::getWindowSize().x - 1;

		Menu::calcDims();

		keys.dims.top = dims.top = 0;
		keys.dims.bottom = dims.bottom = 0;
		keys.dims.left = dims.left = 0;
		keys.dims.right = dims.right = 0;
	}

	void draw()
	{
		keys.draw();

		Menu::draw();
	}

	void erase()
	{
		keys.erase();
	}

	void init(bool canClose)
	{
		Menu::init(canClose);

		text = "Pointer Not Set";
		pointerSet = false;

		keys.init();
	}

	void mouseDown(int x, int y)
	{
		Menu::mouseDown(x, y);

		keys.mouseDown(x, y);
	}

	void mouseOver(int x, int y)
	{
		Menu::mouseOver(x, y);

		keys.mouseOver(x, y);
	}

	void mouseUp(int x, int y)
	{
		Menu::mouseUp(x, y);

		keys.mouseOver(x, y);
	}

	void setPointer(string* p)
	{
		pText = p;
		text = *pText;
		pointerSet = true;
	}
};

KeyboardMenu menuKeyboard;

#define NEW_HOOK_DEFINE(name, menu, fstring)								\
class name : public dfhack_viewscreen										\
{																			\
public:																		\
	void feed(set<df::interface_key> *events) { }							\
																			\
	void logic() 															\
	{ 																		\
		dfhack_viewscreen::logic(); 										\
		menu.logic(); 														\
	}																		\
																			\
	void render()															\
	{																		\
		dfhack_viewscreen::render();										\
		menu.draw();														\
	}																		\
	void resize(int x, int y) { dfhack_viewscreen::resize(x, y); }			\
																			\
	void help() { }															\
																			\
	std::string getFocusString() { return fstring; }						\
																			\
	~name() { }																\
}

//defined hooks for new viewscreens
NEW_HOOK_DEFINE(virtual_keyboard_hook, menuKeyboard, "virtualKeyboard");


//advanced menus
struct EmbarkOptionsMenu : public Menu
{
	DynamicWidget		playNow;
	DynamicSelectWidget	abilitiesL;
	StaticWidget		abilitiesLBottom;
	DynamicMultiWidget	abilitiesR;
	StaticWidget		bottom;
	DynamicMultiWidget	animalsL;
	StaticWidget		animalsLBottom;
	DynamicMultiWidget	animalsR;
	StaticWidget		areYouSure;

	df::viewscreen_setupdwarfgamest* view;

	//magic memory offset. Item exists in DF's viewscreen memory, but not in the df-structure version.
	int8_t warning;

	void calcDims()
	{


		dims.top = 1;
		dims.bottom = Screen::getWindowSize().y - 2;
		dims.left = 1;
		dims.right = Screen::getWindowSize().x - 2;

		Menu::calcDims();

		view = (df::viewscreen_setupdwarfgamest*)Core::getTopViewscreen();

		warning = *(int8_t*)(((int32_t)&view->animal_cursor) + 17);
		
		playNow.setIndex(&view->choice);
		playNow.setIndexMax(2);
		playNow.dims.top = dims.top + 1;
		playNow.dims.bottom = playNow.dims.top + 1;
		playNow.dims.left = dims.left;
		playNow.dims.right = dims.right;

		abilitiesL.setIndex(&view->dwarf_cursor);
		abilitiesL.setIndexMax(view->dwarf_info.size());
		abilitiesL.dims.top = dims.top + 1;
		abilitiesL.dims.bottom = dims.bottom - 3;
		abilitiesL.dims.left = dims.left + 1;
		abilitiesL.dims.right = abilitiesL.dims.left + 36;

		abilitiesLBottom.dims.top = dims.bottom - 1;
		abilitiesLBottom.dims.bottom = dims.bottom;
		abilitiesLBottom.dims.left = dims.left;
		abilitiesLBottom.dims.right = dims.right;

		abilitiesR.setIndex(&view->skill_cursor);
		abilitiesR.setIndexMax(view->embark_skills.size());
		abilitiesR.dims.top = dims.top + 1;
		abilitiesR.dims.bottom = dims.bottom - 3;
		abilitiesR.dims.left = (dims.right - abilitiesL.dims.right) / 2 + abilitiesL.dims.right - 18;
		abilitiesR.dims.right = abilitiesR.dims.left + 34;

		if (view->mode)
			bottom.items[0]->display = "Dwarves";
		else
			bottom.items[0]->display = "Items";

		bottom.dims.top = dims.bottom - 1;
		bottom.dims.bottom = dims.bottom;
		bottom.dims.left = dims.left;
		bottom.dims.right = dims.right;

		animalsL.setIndex(&view->item_cursor);
		animalsL.setIndexMax(view->items.size());
		animalsL.dims.top = dims.top + 1;
		animalsL.dims.bottom = dims.bottom - 3;
		animalsL.dims.left = dims.left + 1;
		animalsL.dims.right = animalsL.dims.left + 35;

		animalsLBottom.dims.top = dims.bottom - 1;
		animalsLBottom.dims.bottom = dims.bottom;
		animalsLBottom.dims.left = dims.left;
		animalsLBottom.dims.right = dims.right;

		animalsR.setIndex(&view->animal_cursor);
		animalsR.setIndexMax(view->animals.count.size());
		animalsR.dims.top = dims.top + 1;
		animalsR.dims.bottom = dims.bottom - 3;
		animalsR.dims.left = (dims.right - animalsL.dims.right + 1) / 2 + animalsL.dims.right - 7;
		animalsR.dims.right = animalsR.dims.left + 25;

		areYouSure.dims.top = dims.top + 9;
		areYouSure.dims.bottom = areYouSure.dims.top + 4;
		areYouSure.dims.left = dims.left + 2;
		areYouSure.dims.right = areYouSure.dims.left + 73;
	}

	void draw()
	{
		calcDims();

		Menu::draw();

		if (view->show_play_now)
		{

		}
		else if (warning)
		{
			areYouSure.draw();
		}
		else
		{
			if (view->mode)
			{
				if (view->supply_column)
					animalsR.draw();
				else
				{
					animalsL.draw();
					animalsLBottom.draw();
				}
			}
			else
			{
				abilitiesL.draw();

				if (view->dwarf_column)
					abilitiesR.draw();
				else
					abilitiesLBottom.draw();
			}

			bottom.draw();
		}
	}

	void erase()
	{
		playNow.erase();
		abilitiesL.erase();
		abilitiesLBottom.erase();
		abilitiesR.erase();
		bottom.erase();
		animalsL.erase();
		animalsLBottom.erase();
		animalsR.erase();
		areYouSure.erase();
	}

	void init(bool canClose)
	{
		Menu::init(canClose);

		HotkeyItem* si;

		//first display
		playNow.init();

		//second display
		abilitiesL.init(); 

		abilitiesLBottom.init();
		DEFINE_HOTKEY_ITEM(si, abilitiesLBottom.items, 0, 55, "View", SETUPGAME_VIEW);
		DEFINE_HOTKEY_ITEM(si, abilitiesLBottom.items, 0, 64, "Customize", SETUPGAME_CUSTOMIZE_UNIT);

		abilitiesR.init();
		DEFINE_HOTKEY_ITEM(si, abilitiesR.selector.items, 0, 26, " - ", SECONDSCROLL_UP);
		DEFINE_HOTKEY_ITEM(si, abilitiesR.selector.items, 0, 30, " + ", SECONDSCROLL_DOWN);

		//third display
		animalsL.init();
		DEFINE_HOTKEY_ITEM(si, animalsL.selector.items, 0, 25, " - ", SECONDSCROLL_UP);
		DEFINE_HOTKEY_ITEM(si, animalsL.selector.items, 0, 29, " + ", SECONDSCROLL_DOWN);

		animalsLBottom.init();
		DEFINE_HOTKEY_ITEM(si, animalsLBottom.items, 0, 55, "New", SETUPGAME_NEW);

		animalsR.init();
		DEFINE_HOTKEY_ITEM(si, animalsR.selector.items, 0, 15, " - ", SECONDSCROLL_UP);
		DEFINE_HOTKEY_ITEM(si, animalsR.selector.items, 0, 19, " + ", SECONDSCROLL_DOWN);

		//multi display
		bottom.init();
		DEFINE_HOTKEY_ITEM(si, bottom.items, 0, 6, "Items", CHANGETAB);
		DEFINE_HOTKEY_ITEM(si, bottom.items, 0, 27, "Embark!", SETUP_EMBARK);
		DEFINE_HOTKEY_ITEM(si, bottom.items, 1, 4, "Name Fortress", SETUP_NAME_FORT);
		DEFINE_HOTKEY_ITEM(si, bottom.items, 1, 27, "Name Group", SETUP_NAME_GROUP);
		DEFINE_HOTKEY_ITEM(si, bottom.items, 1, 44, "Save", SETUPGAME_SAVE_PROFILE);

		areYouSure.init();
		DEFINE_HOTKEY_ITEM(si, areYouSure.items, 3, 8, "I am ready!", SELECT);
		DEFINE_HOTKEY_ITEM(si, areYouSure.items, 3, 42, "Go back", LEAVESCREEN);
	}

	void mouseDown(int x, int y)
	{
		Menu::mouseDown(x, y);

		if (view->show_play_now)
			playNow.mouseDown(x, y);
		else if (warning)
		{
			areYouSure.mouseDown(x, y);
		}
		else
		{
			if (view->mode)
			{
				animalsR.mouseDown(x, y);
				animalsL.mouseDown(x, y);

				if (!view->supply_column)
					animalsLBottom.mouseDown(x, y);
			}
			else
			{
				abilitiesL.mouseDown(x, y);
				abilitiesR.mouseDown(x, y);

				if (!view->dwarf_column)
					abilitiesLBottom.mouseDown(x, y);
			}

			bottom.mouseDown(x, y);
		}
	}
	
	void mouseOver(int x, int y)
	{
		Menu::mouseOver(x, y);

		if (view->show_play_now)
			playNow.mouseOver(x, y);
		else if (warning)
		{
			areYouSure.mouseOver(x, y);
		}
		else
		{
			if (view->mode)
			{
				if (animalsR.over(x, y))
					view->supply_column = 1;
				else if (animalsL.over(x, y))
					view->supply_column = 0;

				animalsR.mouseOver(x, y);
				animalsL.mouseOver(x, y);

				if (!view->supply_column)
					animalsLBottom.mouseOver(x, y);
			}
			else
			{
				if (abilitiesR.over(x, y))
					view->dwarf_column = 1;
				else if (abilitiesL.over(x, y))
					view->dwarf_column = 0;

				abilitiesL.mouseOver(x, y);
				abilitiesR.mouseOver(x, y);

				if (!view->dwarf_column)
					abilitiesLBottom.mouseOver(x, y);
			}

			bottom.mouseOver(x, y);
		}
	}

	void mouseUp(int x, int y)
	{
		Menu::mouseUp(x, y);

		if (view->show_play_now)
			playNow.mouseUp(x, y);
		else if (warning)
		{
			areYouSure.mouseUp(x, y);
		}
		else
		{
			if (view->mode)
			{
				animalsR.mouseUp(x, y);
				animalsL.mouseUp(x, y);

				if (!view->supply_column)
					animalsLBottom.mouseUp(x, y);
			}
			else
			{
				abilitiesL.mouseUp(x, y);
				abilitiesR.mouseUp(x, y);

				if (!view->dwarf_column)
					abilitiesLBottom.mouseUp(x, y);
			}

			bottom.mouseUp(x, y);
		}
	}
};

struct LoadgameMenu : public Menu
{
	DynamicWidget middle;

	void calcDims()
	{
		dims.top = 0;
		dims.bottom = Screen::getWindowSize().y - 1;
		dims.left = 0;
		dims.right = Screen::getWindowSize().x - 1;

		Menu::calcDims();

		df::viewscreen_loadgamest* view = (df::viewscreen_loadgamest*)Core::getTopViewscreen();

		middle.setIndex(&view->sel_idx);
		middle.setIndexMax(view->saves.size());

		middle.dims.top = dims.top + 1;

		middle.dims.bottom = middle.dims.top + view->saves.size();
		if (middle.dims.bottom - middle.dims.top > 9)
			middle.dims.bottom = middle.dims.top + 9;

		middle.dims.left = dims.left;
		middle.dims.right = dims.right;
	}

	void draw()
	{
		calcDims();

		Menu::draw();
	}

	void erase()
	{
		middle.erase();
	}

	void init(bool canClose)
	{
		Menu::init(canClose);

		middle.init();
	}

	void mouseDown(int x, int y)
	{
		Menu::mouseDown(x, y);

		middle.mouseDown(x, y / 2);
	}

	void mouseOver(int x, int y)
	{
		Menu::mouseOver(x, y);

		middle.mouseOver(x, y / 2);
	}

	void mouseUp(int x, int y)
	{
		Menu::mouseUp(x, y);

		middle.mouseUp(x, y / 2);
	}
};

struct MovieplayerMenu : public Menu
{
	void calcDims()
	{
		dims.top = 0;
		dims.bottom = Screen::getWindowSize().y - 1;
		dims.left = 0;
		dims.right = Screen::getWindowSize().x - 1;

		Menu::calcDims();
	}

	void draw()
	{
		Menu::draw();
	}

	void init(bool canClose)
	{
		Menu::init(canClose);
	}

	void mouseDown(int x, int y)
	{
		Menu::mouseDown(x, y);
	}

	void mouseOver(int x, int y)
	{
		Menu::mouseOver(x, y);
	}

	void mouseUp(int x, int y)
	{
		Menu::mouseUp(x, y);

		sendInput(LEAVESCREEN);
	}
};

struct NewRegionMenu : public Menu
{
	StaticWidget radioBoard;
	StaticWidget mapPaused;
	StaticWidget mapComplete;
	int32_t lastYear;
	string useWorld;

	df::viewscreen_new_regionst* view;

	void calcDims()
	{
		dims.top = 0;
		dims.bottom = Screen::getWindowSize().y - 1;
		dims.left = 0;
		dims.right = Screen::getWindowSize().x - 1;

		Menu::calcDims();

		view = (df::viewscreen_new_regionst*)Core::getTopViewscreen();
		int ii = 0;
		((RadioItem*)radioBoard.items[ii])->index = &view->world_size; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->world_size; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->world_size; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->world_size; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->world_size; ii++;

		((RadioItem*)radioBoard.items[ii])->index = &view->history; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->history; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->history; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->history; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->history; ii++;

		((RadioItem*)radioBoard.items[ii])->index = &view->number_civs; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_civs; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_civs; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_civs; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_civs; ii++;

		((RadioItem*)radioBoard.items[ii])->index = &view->number_sites; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_sites; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_sites; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_sites; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_sites; ii++;

		((RadioItem*)radioBoard.items[ii])->index = &view->number_beasts; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_beasts; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_beasts; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_beasts; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->number_beasts; ii++;

		((RadioItem*)radioBoard.items[ii])->index = &view->savagery; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->savagery; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->savagery; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->savagery; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->savagery; ii++;

		((RadioItem*)radioBoard.items[ii])->index = &view->mineral_occurence; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->mineral_occurence; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->mineral_occurence; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->mineral_occurence; ii++;
		((RadioItem*)radioBoard.items[ii])->index = &view->mineral_occurence; ii++;

		radioBoard.dims.top = dims.top;
		radioBoard.dims.bottom = radioBoard.dims.top + 24;
		radioBoard.dims.left = dims.left + 1;
		radioBoard.dims.right = dims.right - 1;

		mapPaused.dims.top = dims.top;
		mapPaused.dims.bottom = dims.bottom;
		mapPaused.dims.left = dims.left;
		mapPaused.dims.right = dims.right;

		mapPaused.items[0]->row = dims.bottom - 1;
		mapPaused.items[1]->row = dims.bottom;
		mapPaused.items[2]->row = dims.bottom;

		if (*df::global::cur_year < 2)
			mapPaused.items[0]->display = "";
		else
			mapPaused.items[0]->display = useWorld;
		
		mapComplete.dims.top = dims.top;
		mapComplete.dims.bottom = dims.bottom;
		mapComplete.dims.left = dims.left;
		mapComplete.dims.right = dims.right;

		mapComplete.items[0]->row = dims.bottom;
		mapComplete.items[1]->row = dims.bottom;
		
	}

	void draw()
	{
		calcDims();

		closeable = true;

		if (view->worldgen_presets.size() == 0)	//in load screen
		{
			closeable = false;
			//pout->print("load\n");
		}
		else if (view->unk_33.size() != 0) //in intro text screen
		{
			closeable = false;
			//pout->print("text\n");
		}
		else if (view->simple_mode) //Creating New World!
		{
			//pout->print("radio\n");
			radioBoard.draw();
		}
		else if (view->in_worldgen) //Designing New World with Advanced Params
		{
			//pout->print("advanced\n");
		}
		else //map is shown
		{
			if (view->worldgen_paused) //creation is PAUSED
			{
				closeable = false;
				mapPaused.draw();
			}
			else if (df::global::world->worldgen_status.state == 10) //creation is COMPLETE
			{
				closeable = false;
				mapComplete.draw();
			}
			else //creation is IN PROGRESS
			{
				closeable = false;
				//pout->print("generating: %u\n", df::global::world->worldgen_status.state);
			}
		}
		
		Menu::draw();
		//
		//pout->print("a: %u, b: %u\n", view->in_worldgen, &view->in_worldgen);
	}

	void erase()
	{
		radioBoard.erase();
		mapPaused.erase();
		mapComplete.erase();
	}

	void init(bool canClose)
	{
		Menu::init(canClose);

		//close = &closeSpoofWidget;
		//close->init();

		RadioItem* ri;
		HotkeyItem* hi;

		radioBoard.init();
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 10, "Pocket", &view->world_size, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 22, "Smaller", &view->world_size, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 36, "Small", &view->world_size, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 49, "Medium", &view->world_size, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 62, "Large", &view->world_size, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 8, "Very Short", &view->history, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 23, "Short", &view->history, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 36, "Medium", &view->history, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 50, "Long", &view->history, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 60, "Very Long", &view->history, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 9, "Very Low", &view->number_civs, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 24, "Low", &view->number_civs, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 36, "Medium", &view->number_civs, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 50, "High", &view->number_civs, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 60, "Very High", &view->number_civs, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 9, "Very Low", &view->number_sites, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 24, "Low", &view->number_sites, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 36, "Medium", &view->number_sites, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 50, "High", &view->number_sites, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 60, "Very High", &view->number_sites, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 9, "Very Low", &view->number_beasts, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 24, "Low", &view->number_beasts, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 36, "Medium", &view->number_beasts, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 50, "High", &view->number_beasts, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 60, "Very High", &view->number_beasts, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 9, "Very Low", &view->savagery, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 24, "Low", &view->savagery, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 36, "Medium", &view->savagery, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 50, "High", &view->savagery, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 60, "Very High", &view->savagery, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 8, "Very Rare", &view->mineral_occurence, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 24, "Rare", &view->mineral_occurence, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 36, "Sparse", &view->mineral_occurence, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 48, "Frequent", &view->mineral_occurence, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 60, "Everywhere", &view->mineral_occurence, 4);

		DEFINE_HOTKEY_ITEM(hi, radioBoard.items, 24, 39, "Abort", LEAVESCREEN);
		DEFINE_HOTKEY_ITEM(hi, radioBoard.items, 24, 65, "Go!", MENU_CONFIRM);

		useWorld = "Use the world as it currently exists";

		mapPaused.init();
		mapPaused.fg = COLOR_WHITE;
		DEFINE_HOTKEY_ITEM(hi, mapPaused.items, 0, 4, useWorld, WORLD_GEN_USE);
		DEFINE_HOTKEY_ITEM(hi, mapPaused.items, 1, 4, "Continue", WORLD_GEN_CONTINUE);
		DEFINE_HOTKEY_ITEM(hi, mapPaused.items, 1, 28, "Abort", WORLD_GEN_ABORT);

		mapComplete.init();
		mapComplete.fg = COLOR_WHITE;
		DEFINE_HOTKEY_ITEM(hi, mapComplete.items, 1, 8, "Accept", SELECT);
		DEFINE_HOTKEY_ITEM(hi, mapComplete.items, 1, 21, "Abort", WORLD_GEN_ABORT);

		lastYear = 0;
	}

	void mouseDown(int x, int y)
	{
		Menu::mouseDown(x, y);

		if (view->worldgen_presets.size() == 0)	//in load screen
		{
			
		}
		else if (view->unk_33.size() != 0) //in intro text screen
		{
			sendInput(LEAVESCREEN);
		}
		else if (view->simple_mode) //Creating New World!
		{
			radioBoard.mouseDown(x, y);
		}
		else if (view->in_worldgen) //Designing New World with Advanced Params
		{
			
		}
		else //map is shown
		{
			if (view->worldgen_paused) //creation is PAUSED
			{
				mapPaused.mouseDown(x, y);
			}
			else if (df::global::world->worldgen_status.state == 10) //creation is COMPLETE
			{
				mapComplete.mouseDown(x, y);
			}
			else //creation is IN PROGRESS
			{
				sendInput(SELECT);
			}
		}
	}

	void mouseOver(int x, int y)
	{
		Menu::mouseOver(x, y);

		switch (y)
		{
		case 2:
			view->cursor_line = 0;
			break;
		case 5:
			view->cursor_line = 1;
			break;
		case 8:
			view->cursor_line = 2;
			break;
		case 11:
			view->cursor_line = 3;
			break;
		case 14:
			view->cursor_line = 4;
			break;
		case 17:
			view->cursor_line = 5;
			break;
		case 20:
			view->cursor_line = 6;
			break;
		}

		if (view->worldgen_presets.size() == 0)	//in load screen
		{

		}
		else if (view->unk_33.size() != 0) //in intro text screen
		{

		}
		else if (view->simple_mode) //Creating New World!
		{
			radioBoard.mouseOver(x, y);
		}
		else if (view->in_worldgen) //Designing New World with Advanced Params
		{

		}
		else //map is shown
		{
			if (view->worldgen_paused) //creation is PAUSED
			{
				mapPaused.mouseOver(x, y);
			}
			else if (df::global::world->worldgen_status.state == 10) //creation is COMPLETE
			{
				mapComplete.mouseOver(x, y);
			}
			else //creation is IN PROGRESS
			{

			}
		}
	}

	void mouseUp(int x, int y)
	{
		Menu::mouseUp(x, y);

		if (view->worldgen_presets.size() == 0)	//in load screen
		{

		}
		else if (view->unk_33.size() != 0) //in intro text screen
		{

		}
		else if (view->simple_mode) //Creating New World!
		{
			radioBoard.mouseUp(x, y);
		}
		else if (view->in_worldgen) //Designing New World with Advanced Params
		{

		}
		else //map is shown
		{
			if (view->worldgen_paused) //creation is PAUSED
			{
				mapPaused.mouseUp(x, y);
			}
			else if (df::global::world->worldgen_status.state == 10) //creation is COMPLETE
			{
				mapComplete.mouseUp(x, y);
			}
			else //creation is IN PROGRESS
			{

			}
		}
	}
};

struct LayerWorldGenParamMenu : public Menu
{
	StaticWidget radioBoard;

	df::viewscreen_new_regionst* view;

	void calcDims()
	{
		dims.top = 0;
		dims.bottom = Screen::getWindowSize().y - 1;
		dims.left = 0;
		dims.right = Screen::getWindowSize().x - 1;

		Menu::calcDims();

		//view = (df::viewscreen_new_regionst*)Core::getTopViewscreen();

		//
	}

	void draw()
	{
		calcDims();

		Menu::draw();

		//pout->print("layer world gen param\n");
	}

	void erase()
	{
		radioBoard.erase();
	}

	void init(bool canClose)
	{
		Menu::init(canClose);

		//
	}
};

struct LayerWorldGenParamPresetMenu : public Menu
{
	StaticWidget radioBoard;

	df::viewscreen_new_regionst* view;

	void calcDims()
	{
		dims.top = 0;
		dims.bottom = Screen::getWindowSize().y - 1;
		dims.left = 0;
		dims.right = Screen::getWindowSize().x - 1;

		Menu::calcDims();

		//view = (df::viewscreen_new_regionst*)Core::getTopViewscreen();

		//
	}

	void draw()
	{
		calcDims();

		Menu::draw();

		//pout->print("layer world gen param preset\n");
	}

	void init(bool canClose)
	{
		Menu::init(canClose);

		//
	}
};

struct StartSiteMenu : public Menu
{
	StaticWidget bottom;
	StaticWidget promptLarge;
	Widget local;
	Widget region;
	Widget world;
	int security;

	df::viewscreen_choose_start_sitest* view;

	int localDownX;
	int localDownY;

	void calcDims()
	{
		dims.top = 1;
		dims.bottom = Screen::getWindowSize().y - 2;
		dims.left = 1;
		dims.right = Screen::getWindowSize().x - 2;

		Menu::calcDims();

		view = (df::viewscreen_choose_start_sitest*)Core::getTopViewscreen();
		security = view->in_embark_aquifer + view->in_embark_salt + view->in_embark_large + view->in_embark_normal;

		//pout->print("security: %u, aq: %u, salt: %u, large: %u, normal: %u\n", security, view->in_embark_aquifer, view->in_embark_salt, view->in_embark_large, view->in_embark_normal);
		
		if (security > 0)
		{
			promptLarge.items[0]->row = 3 * security;
			promptLarge.dims.top = dims.top + 8 - pow((double)2, (double)(security - 1));
			promptLarge.items[1]->row = promptLarge.items[0]->row;
			promptLarge.dims.bottom = promptLarge.dims.top + promptLarge.items[0]->row;
			promptLarge.dims.left = (dims.right - dims.left)/2 - 25;
			promptLarge.dims.right = promptLarge.dims.left + 53;
		}
		else
		{
			bottom.dims.top = dims.bottom - 2;
			bottom.dims.bottom = dims.bottom;
			bottom.dims.left = dims.left;
			bottom.dims.right = dims.right;

			local.dims.top = dims.top + 1;
			local.dims.bottom = local.dims.top + 15;
			local.dims.left = dims.left;
			local.dims.right = local.dims.left + 15;

			region.dims.top = dims.top + 1;
			region.dims.bottom = dims.bottom - 6;
			region.dims.left = local.dims.right + 2;
			region.dims.right = (dims.right - 12) / 2;

			if (region.dims.right - region.dims.left + 1 >= df::global::world->world_data->world_width)
				region.dims.right = region.dims.left + df::global::world->world_data->world_width - 1;

			if (region.dims.bottom - region.dims.top + 1 >= df::global::world->world_data->world_height)
				region.dims.bottom = region.dims.top + df::global::world->world_data->world_height - 1;
		}


		//pout->print("A: %u, B: %u, C: %u\n", view->in_embark_large, view->unk_15a, view->unk_15c);
	}

	void draw()
	{
		calcDims();

		Menu::draw();

		if (security > 0)
		{
			promptLarge.draw();
		}
		else
		{
			bottom.draw();
			local.draw();
			region.draw();
		}
	}

	void erase()
	{
		bottom.erase();
		promptLarge.erase();
		local.erase();
		region.erase();
		world.erase();
	}

	void init(bool canClose)
	{
		Menu::init(canClose);

		bottom.init();
		HotkeyItem* si;
		DEFINE_HOTKEY_ITEM(si, bottom.items, 2, 5, "Change Mode", CHANGETAB);
		DEFINE_HOTKEY_ITEM(si, bottom.items, 2, 21, "Embark!", SETUP_EMBARK);

		promptLarge.init();
		DEFINE_HOTKEY_ITEM(si, promptLarge.items, 3, 9, "Embark!", SELECT);
		DEFINE_HOTKEY_ITEM(si, promptLarge.items, 3, 37, "Cancel", LEAVESCREEN);

		local.init();
		localDownX = -1;
		localDownY = -1;

		region.init();

		security = 0;
	}

	void mouseDown(int x, int y)
	{
		Menu::mouseDown(x, y);

		if (security > 0)
		{
			promptLarge.mouseDown(x, y);
		}
		else
		{
			bottom.mouseDown(x, y);

			if (local.over(x, y) && localDownX < 0)
			{
				localDownX = x - local.dims.left;
				localDownY = y - local.dims.top;

				view->location.embark_pos_min.x = localDownX;
				view->location.embark_pos_min.y = localDownY;
				view->location.embark_pos_max.x = localDownX;
				view->location.embark_pos_max.y = localDownY;
			}
		}
	}

	void mouseOver(int x, int y)
	{
		Menu::mouseOver(x, y);

		int tempX;
		int tempY;

		if (security > 0)
		{
			promptLarge.mouseOver(x, y);
		}
		else
		{
			bottom.mouseOver(x, y);

			if (local.over(x, y) && localDownX >= 0)
			{
				tempX = x - local.dims.left;
				tempY = y - local.dims.top;


				if (tempX > localDownX)
					view->location.embark_pos_max.x = tempX;
				else
					view->location.embark_pos_min.x = tempX;

				if (tempY > localDownY)
					view->location.embark_pos_max.y = tempY;
				else
					view->location.embark_pos_min.y = tempY;
			}
			else if (region.over(x, y))
			{
				tempX = x - region.dims.left;
				tempY = y - region.dims.top;
			}
		}
	}

	void mouseUp(int x, int y)
	{
		Menu::mouseUp(x, y);

		localDownX = -1;
		localDownY = -1;

		if (security > 0)
		{
			promptLarge.mouseUp(x, y);
		}
		else
		{
			bottom.mouseUp(x, y);

			if (region.over(x, y))
			{
				int viewportWidth = (region.dims.right - region.dims.left + 1);
				int viewportHeight = (region.dims.bottom - region.dims.top + 1);

				int tempX = x - region.dims.left;
				int tempY = y - region.dims.top;

				int baseX = view->location.region_pos.x - viewportWidth / 2;
				int baseY = view->location.region_pos.y - viewportHeight / 2;

				if (baseX < 0)
					baseX = 0;
				else if (baseX > df::global::world->world_data->world_width - viewportWidth)
					baseX = df::global::world->world_data->world_width - viewportWidth;

				if (baseY < 0)
					baseY = 0;
				else if (baseY > df::global::world->world_data->world_height - viewportHeight)
					baseY = df::global::world->world_data->world_height - viewportHeight;

				int newX = tempX + baseX;
				int newY = tempY + baseY;

				int diff;

				if (newX < view->location.region_pos.x)
				{
					diff = view->location.region_pos.x - newX;
					for (int i = 0; i < diff; i++)
						sendInput(CURSOR_LEFT);
				}
				else if (newX > view->location.region_pos.x)
				{
					diff = newX - view->location.region_pos.x;
					for (int i = 0; i < diff; i++)
						sendInput(CURSOR_RIGHT);
				}

				if (newY < view->location.region_pos.y)
				{
					diff = view->location.region_pos.y - newY;
					for (int i = 0; i < diff; i++)
						sendInput(CURSOR_UP);
				}
				else if (newY > view->location.region_pos.y)
				{
					diff = newY - view->location.region_pos.y;
					for (int i = 0; i < diff; i++)
						sendInput(CURSOR_DOWN);
				}
			}
		}
	}
};

struct TextViewerMenu : public Menu
{
	int downY;
	bool wasDown;


	df::viewscreen_textviewerst* view;

	void calcDims()
	{
		dims.top =1;
		dims.bottom = Screen::getWindowSize().y - 2;
		dims.left = 1;
		dims.right = Screen::getWindowSize().x - 2;

		Menu::calcDims();

		view = (df::viewscreen_textviewerst*)Core::getTopViewscreen();
	}

	void draw()
	{
		calcDims();

		Menu::draw();

		//pout->print("i: %u, a: %u, b: %u, c: %u, d: %u, e: %u, f: %u\n", view->scroll_pos, view->src_text.size(), view->formatted_text.size(), view->hyperlinks.size(), view->logged_error, view->cursor_line, view->pause_depth);
	}

	void init(bool canClose)
	{
		Menu::init(canClose);

		downY = -1;
		wasDown = false;
	}

	void mouseDown(int x, int y)
	{
		Menu::mouseDown(x, y);

		if (!wasDown)
			downY = y;

		wasDown = true;
	}

	void mouseOver(int x, int y)
	{
		Menu::mouseOver(x, y);

		if (wasDown && y != -1)
		{
			int height = dims.bottom - dims.top + 1 - 2;
			double newScrollPos = view->scroll_pos + downY - y;

			//pout->print("1: %u\n", newScrollPos);

			if (newScrollPos > view->formatted_text.size() - height)
				newScrollPos = view->formatted_text.size() - height;

			//pout->print("2: %u\n", newScrollPos);

			if (newScrollPos < 0)
				newScrollPos = 0;

			//pout->print("3: %u\n", newScrollPos);

			view->scroll_pos = newScrollPos;

			downY = y;

			//view->scroll_pos
			//view->formatted_text.size()
		}
	}

	void mouseUp(int x, int y)
	{
		Menu::mouseUp(x, y);

		//downY = -1;
		wasDown = false;
	}
};

struct TitleMenu : public Menu
{
	DynamicWidget middle;
	int lastPage;

	void calcDims()
	{
		dims.top = 0;
		dims.bottom = Screen::getWindowSize().y - 1;
		dims.left = 0;
		dims.right = Screen::getWindowSize().x - 1;

		df::viewscreen_titlest* view = (df::viewscreen_titlest*)Core::getTopViewscreen();
		titleSpoof = view;

		if (view->sel_subpage != lastPage)
			middle.init();

		lastPage = view->sel_subpage;



		int temp1 = dims.bottom - 2;
		int rem1 = temp1 % 6;

		int midNorm = 11;
		int midWierd = (temp1 / 6) + 5;

		if (rem1 == 1)
			midWierd = midWierd - 1;

		//switch (lastPage)
		switch (view->sel_subpage)
		{

		case view->None:
			closeable = false;
			middle.setIndex(&view->sel_menu_line);
			middle.setIndexMax(view->menu_line_id.size());
			middle.dims.top = midWierd;
			middle.dims.bottom = middle.dims.top + view->menu_line_id.size() - 1;
			break;

		case view->StartSelectWorld:
			closeable = true;
			middle.setIndex(&view->sel_submenu_line);
			middle.setIndexMax(view->start_savegames.size());
			middle.dims.top = midNorm;
			middle.dims.bottom = middle.dims.top + view->start_savegames.size() - 1;
			break;

		case view->StartSelectMode:
			closeable = true;
			middle.setIndex(&view->sel_menu_line);
			middle.setIndexMax(view->submenu_line_id.size());
			middle.dims.top = midNorm;
			middle.dims.bottom = middle.dims.top + view->submenu_line_id.size() - 1;
			break;

		case view->Arena:
			closeable = true;
			middle.setIndex(&view->sel_submenu_line);
			middle.setIndexMax(view->continue_savegames.size());
			middle.dims.top = midNorm;
			middle.dims.bottom = middle.dims.top + view->continue_savegames.size() - 1;
			break;

		case view->About:
		default:
			closeable = true;
			middle.init();
			break;
		}

		if (middle.dims.bottom > middle.dims.top + 8)
			middle.dims.bottom = middle.dims.top + 8;

		middle.dims.left = dims.left;
		middle.dims.right = dims.right;
		

		Menu::calcDims();
	}
	
	void draw()
	{
		Menu::draw();

		middle.draw();
		//pout->print("draw\n");
	}

	void erase()
	{
		middle.erase();
	}

	void feed()
	{
		CoreSuspendClaimer suspend;
		
		ViewscreenItem vi;
		vi.view = new virtual_keyboard_hook();
		//vi.action();
	}

	void init(bool canClose)
	{
		Menu::init(canClose);

		lastPage = -1;
	}

	void mouseDown(int x, int y)
	{
		Menu::mouseDown(x, y);

		middle.mouseDown(x, y);
		//pout->print("down\n");
	}

	void mouseOver(int x, int y)
	{
		Menu::mouseOver(x, y);

		middle.mouseOver(x, y);
		//pout->print("(%u, %u): %s\n", x, y, middle.over(x,y) ? "T":"F");
	}

	void mouseUp(int x, int y)
	{
		Menu::mouseUp(x, y);

		middle.mouseUp(x, y);
	}
};

//these global variables depend on viewscreen structs
EmbarkOptionsMenu menuEmbarkOptions;
LoadgameMenu menuLoadgame;
MovieplayerMenu menuMovieplayer;
NewRegionMenu menuNewRegion;
LayerWorldGenParamMenu menuLayerWorldGenParam;
LayerWorldGenParamPresetMenu menuLayerWorldGenParamPreset;
StartSiteMenu menuStartSite;
TitleMenu menuTitle;
TextViewerMenu menuTextViewer;

#define HOOK_DEFINE(name, viewscreen, menu)									\
struct name : public viewscreen												\
{																			\
	typedef viewscreen interpose_base;										\
																			\
	DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))	\
	{																		\
		INTERPOSE_NEXT(feed)(input);										\
		menu.feed();														\
	}																		\
																			\
	DEFINE_VMETHOD_INTERPOSE(void, logic, ())								\
	{																		\
		INTERPOSE_NEXT(logic)();											\
		menu.logic();														\
	}																		\
																			\
	DEFINE_VMETHOD_INTERPOSE(void, render, ())								\
	{																		\
		INTERPOSE_NEXT(render)();											\
																			\
		menu.draw();														\
	}																		\
};																			\
																			\
IMPLEMENT_VMETHOD_INTERPOSE(name, feed);									\
IMPLEMENT_VMETHOD_INTERPOSE(name, logic);									\
IMPLEMENT_VMETHOD_INTERPOSE(name, render)

//defined hooks for existing viewscreens
HOOK_DEFINE(embark_options_hook, df::viewscreen_setupdwarfgamest, menuEmbarkOptions);
HOOK_DEFINE(loadgame_hook, df::viewscreen_loadgamest, menuLoadgame);
HOOK_DEFINE(movieplayer_hook, df::viewscreen_movieplayerst, menuMovieplayer);
HOOK_DEFINE(new_region_hook, df::viewscreen_new_regionst, menuNewRegion);
HOOK_DEFINE(layer_world_gen_param_hook, df::viewscreen_layer_world_gen_paramst, menuLayerWorldGenParam);
HOOK_DEFINE(layer_world_gen_param_preset_hook, df::viewscreen_layer_world_gen_param_presetst, menuLayerWorldGenParamPreset);
HOOK_DEFINE(start_site_menu_hook, df::viewscreen_choose_start_sitest, menuStartSite);
HOOK_DEFINE(title_menu_hook, df::viewscreen_titlest, menuTitle);
HOOK_DEFINE(textviewer_hook, df::viewscreen_textviewerst, menuTextViewer);

#define INTERPOSE_HOOKS(hook, closes)		\
	INTERPOSE_HOOK(hook, feed).apply(closes);	\
	INTERPOSE_HOOK(hook, logic).apply(closes);	\
	INTERPOSE_HOOK(hook, render).apply(closes)

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
	if (!gps)
		return CR_FAILURE;

	if (enable != is_enabled)
	{
		INTERPOSE_HOOKS(embark_options_hook, enable);
		INTERPOSE_HOOKS(loadgame_hook, enable);
		INTERPOSE_HOOKS(movieplayer_hook, enable);
		INTERPOSE_HOOKS(new_region_hook, enable);
		INTERPOSE_HOOKS(layer_world_gen_param_hook, enable);
		INTERPOSE_HOOKS(layer_world_gen_param_preset_hook, enable);
		INTERPOSE_HOOKS(start_site_menu_hook, enable);
		INTERPOSE_HOOKS(title_menu_hook, enable);
		INTERPOSE_HOOKS(textviewer_hook, enable);

		is_enabled = enable;
	}

	return CR_OK;
}

string helpText;

command_result menu_mouse(color_ostream &out, std::vector<std::string> & params)
{
	if (params.size() == 0)
	{
		pout->print(helpText.c_str());
	}
	else if (params.size() == 1)
	{
		if (params[0] == "start" || params[0] == "enable")
		{
			out.print(" enabled\n");
			plugin_enable(out, true);
		}
		else if (params[0] == "stop" || params[0] == "disable")
		{
			out.print(" disabled\n");
			plugin_enable(out, false);
		}
		else if (params[0] == "help" || params[0] == "?")
		{
			pout->print(helpText.c_str());
		}
		else
		{
			out.print(" unrecognized command\n");
		}
	}


	return CR_OK;
}

//This code gets added to the DFHack main loop when "is_enabled" = true
DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
	return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands)
{
	helpText =	"\nUsage:\n"
				" menu-mouse [enable|disable]:\t\t enables or disables menu-mouse\n"
				"\nIn-Game:\n"
				" Left click menu items to select them.\n"
				" Left click and swipe to traverse large menus and lists.\n"
				" Left click \"><\", when it's available, to close the current menu.\n"
				" Left click optionless fullscreen displays to close them.\n";

	commands.push_back(PluginCommand(
		"menu-mouse", "Adds mouse functionality to text menus",
		menu_mouse, false, helpText.c_str()
		));

	pout = &out;


	menuEmbarkOptions.init(false);
	menuKeyboard.init(true);
	menuLoadgame.init(true);
	menuMovieplayer.init(false);
	menuNewRegion.init(false);
	menuLayerWorldGenParam.init(true);
	menuLayerWorldGenParamPreset.init(true);
	menuStartSite.init(false);
	menuTitle.init(false);
	menuTextViewer.init(true);

	return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
	menuEmbarkOptions.erase();
	menuKeyboard.erase();
	menuLoadgame.erase();
	menuMovieplayer.erase();
	menuNewRegion.erase();
	menuLayerWorldGenParam.erase();
	menuLayerWorldGenParamPreset.erase();
	menuStartSite.erase();
	menuTitle.erase();
	menuTextViewer.erase();

	return CR_OK;
}
