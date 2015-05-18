//standard DFHack files
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include "DataDefs.h"

#include <modules/Gui.h>
//access to hotkey enums. Hotkeys are in 'interface_key.h'
#include "uicommon.h"

//access to colors for printing on screen
#include <ColorText.h>

//viewscreens with data we need
#include "df/viewscreen_customize_unitst.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/viewscreen_layer_choose_language_namest.h"
#include "df/viewscreen_layer_stockpilest.h"
#include "df/viewscreen_layer_world_gen_paramst.h"
#include "df/viewscreen_layer_world_gen_param_presetst.h"
#include "df/viewscreen_loadgamest.h"
#include "df/viewscreen_movieplayerst.h"
#include "df/viewscreen_new_regionst.h"
#include "df/viewscreen_optionst.h"
#include "df/viewscreen_selectitemst.h"
#include "df/viewscreen_setupdwarfgamest.h"
#include "df/viewscreen_textviewerst.h"
#include "df/viewscreen_titlest.h"

//non-viewscreen sources of game data
//#include "df/language_name.h"
#include "df/world_data.h"
#include "df/world.h"
#include "df/historical_entity.h"
#include "df/history_event.h"

using namespace DFHack;
using namespace df::enums::interface_key;

DFHACK_PLUGIN("menu-mouse");

REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(enabler);

#define PLUGIN_VERSION 0.14

//viewscreens depend on these global variables.
color_ostream* pout;
df::viewscreen_titlest* titleSpoof;

//debugging tools
bool exploratory;
bool debug;
static bool inDev = true;
#define EXPLORE(str) if(exploratory) { pout->print(str); }

//global variables
int* x;
int* y;
int8_t* mouseLCurrent;
int8_t mouseLHistory;
bool ignoreNextClick;

void sendInput(df::interface_key ik)
{
	set<df::interface_key> tmp;
	tmp.insert(ik);
	Core::getTopViewscreen()->feed(&tmp);
}

void sendInput(set<df::interface_key> vik)
{
	set<df::interface_key> tmp = vik;
	Core::getTopViewscreen()->feed(&tmp);
}

//basic elements of widgets
struct Rect
{
	int left;
	int right;
	int top;
	int bottom;

	void set(int l, int r, int t, int b)
	{
		left = l;
		right = r;
		top = t;
		bottom = b;
	}

	//Rect rel(double l, double r, double t, double b)
	//{
	//	Rect value;
	//	value.left = left + l;
	//	value.right = right + r;
	//	value.top = top + t;
	//	value.bottom = bottom + b;
	//	return value;
	//}
		
	void sub(Rect &parent, int h, int v, bool jleft, bool jtop)
	{
		left = parent.left*jleft + (parent.right - h)*!jleft;
		right = (parent.left + h)*jleft + parent.right*!jleft;
		top = parent.top*jtop + (parent.bottom - v)*!jtop;
		bottom = (parent.top + v)*jtop + parent.bottom*!jtop;
	}

	void hCenter(int h)
	{
		left = (right - left - h) / 2;
		right = left + h;
	}

	void toScreen(int border)
	{
		left = border;
		right = Screen::getWindowSize().x - border - 1;
		top = border;
		bottom = Screen::getWindowSize().y - border - 1;
	}
};

class Item
{
public:
	int col;
	int row;
	string display;
	//Item(int c, int r, string d){ col = c;  row = r;  display = d; }
	virtual void action(){}
};

class HotkeyItem : public Item
{
public:
	set<df::interface_key> hotkeys;

	void action()
	{
		sendInput(hotkeys);
	}

	void add(df::interface_key ik)
	{
		hotkeys.insert(ik);
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

template<class T>
class ViewscreenItem : public Item
{
public:
	T* view;

	//ViewscreenItem(int c, int r, string d){ Item::Item(c, r, d); }
	void action()
	{
		Screen::show(new T);
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
	element->add(hot);				\
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

#define DEFINE_VIEWSCREEN_ITEM(element, vect, ro, co, dis, v)	\
	element = new ViewscreenItem<v>;	\
	element->row = ro;				\
	element->col = co;				\
	element->display = dis;			\
	vect.push_back(element)

//basic widgets
struct Widget
{
	Rect dims; 
	color_value fg;
	color_value bg;
	string name;

	void muteColors(color_value fg)
	{
		Screen::Pen p;
		for (int i = dims.top; i <= dims.bottom; i++)
			for (int j = dims.left; j <= dims.right; j++)
			{
				p = Screen::readTile(j, i);
				Screen::paintTile(Screen::Pen(p.ch, fg, p.bg), j, i);
			}
	}
	bool over(){ return (*x >= dims.left && *x <= dims.right && *y >= dims.top && *y <= dims.bottom); }
	virtual void draw() {}
	virtual void erase() {}
	virtual bool mouseDown() { return over(); }
	virtual bool mouseOver() { return over(); }
	virtual bool mouseUp() { return over(); }
	virtual void preFeed() {}
	virtual void postFeed() {}
	virtual void setDims(int l, int r, int t, int b) { dims.left = l; dims.right = r; dims.top = t; dims.bottom = b; }
	virtual void init(string n) { setDims(0, 0, 0, 0); fg = COLOR_WHITE; bg = COLOR_BLACK; name = n; }
};

struct CloseWidget : public Widget
{
	void draw()
	{
		fg = COLOR_WHITE;
		Screen::paintTile(Screen::Pen('>', fg, bg), dims.left, dims.top);
		Screen::paintTile(Screen::Pen('<', fg, bg), dims.right, dims.bottom);
	}

	void preFeed() {}
	void postFeed() {}

	void init(string n)
	{
		Widget::init(n);
		fg = COLOR_WHITE;
		bg = COLOR_GREY;
	}

	bool mouseOver()
	{
		if (Widget::mouseOver())
		{
			bg = COLOR_LIGHTRED;
			return true;
		}
		else
		{
			bg = COLOR_GREY;
			return false;
		}
	}

	bool mouseUp()
	{
		if (Widget::mouseUp())
		{
			sendInput(LEAVESCREEN);
			return true;
		}
		return false;
	}
};

template<class TYPE>
struct DynamicWidget : public Widget
{
protected:
	TYPE* index;
	int size;
	bool horizontal;

public:
	bool indexSet;
	bool sizeSet;

	int indexLength;

	double offset;

	bool swiped;

	int xDown;
	int yDown;

	df::interface_key defaultClick;

private:
	virtual void changeIndexTo(TYPE t)
	{
		*index = t;
	}
public:

	virtual void draw() {}

	virtual void setIndex(TYPE* ip)
	{
		index = ip;
		indexSet = true;

		//if (horizontal)
		//	offset = *index / (dims.right - dims.left + 1);
		//else
		//	offset = *index / (dims.bottom - dims.top + 1);
	}

	virtual void setSize(int i)
	{
		size = i;
		sizeSet = true;
	}

	virtual void autoSize(df::interface_key upkey)
	{
		if (indexSet)
		{
			//if (!sizeSet)
			//{
				TYPE store = *index;
				this->changeIndexTo(0);
				//*index = 0;
				sendInput(upkey);
				setSize(*index + 1);
				this->changeIndexTo(store + 1);
				//*index = store + 1;
				sendInput(upkey);
			//}
		}
		else
		{
			//bug("autoSize(" + to_string((long long)upkey) + ")");
		}
	}

	virtual bool mouseDown()
	{
		if (Widget::mouseDown())
		{
			xDown = *x;
			yDown = *y;
			return true;
		}
		return false;
	}

	virtual bool mouseOver()
	{
		if (Widget::mouseOver())
		{
			if (indexSet && sizeSet)
			{
				TYPE newIndex;

				int start;
				int end;
				int parDown;
				int par;
				int length;

				if (horizontal)
				{
					parDown = xDown;
					par = *x;
					start = dims.left;
					end = dims.right;
				}
				else
				{
					parDown = yDown;
					par = *y;
					start = dims.top;
					end = dims.bottom;
				}

				length = (end - start + 1);

				if (parDown > -1 && !swiped && par != parDown)
				{
					if (par < parDown)
						offset++;
					else
						offset--;

					swiped = true;
				}

				if (offset < 0)
					offset = 0;
				else if (size - offset * length / indexLength <= 0)
					offset--;

				newIndex = (par - start + offset * length) / indexLength;

				if (newIndex <= offset * length / indexLength)
					newIndex = offset * length / indexLength;
				else if (newIndex >= size)
					newIndex = size - 1;

				this->changeIndexTo(newIndex);
				//*index = newIndex;
			}
			else
			{
				//bug("mouseOver()");
			}
			return true;
		}
		else
		{
			xDown = -1;
			yDown = -1;
			return false;
		}
	}

	virtual void mouseUpAction()
	{
		sendInput(defaultClick);
	}

	virtual bool mouseUp()
	{
		bool val = false;

		if (Widget::mouseUp())
		{
			int parDown;
			int par;

			if (horizontal)
			{
				parDown = xDown;
				par = *x;
			}
			else
			{
				parDown = yDown;
				par = *y;
			}

			if (par == parDown)
				this->mouseUpAction();
			
			val = true;
		}

		swiped = false;
		xDown = -1;
		yDown = -1;

		return val;
	}

	virtual void preFeed()
	{
		//pout->print("preFeed2\n");
		Widget::preFeed();
	}

	void vertical(bool b)
	{
		horizontal = !b;
	}

	virtual void init(string n)
	{
		Widget::init(n);
		sizeSet = false;
		indexLength = 1;
		offset = 0;
		swiped = false;
		vertical(true);
		xDown = -1;
		yDown = -1;
		defaultClick = SELECT;
	}
	//void postFeed()
	//{
	//	if (indexSet && !enabler->mouse_lbut)
	//	{
	//		if (horizontal)
	//			offset = floor(*index * indexLength / (double)(dims.right - dims.left + 1));
	//		else
	//			offset = floor(*index * indexLength / (double)(dims.bottom - dims.top + 1));
	//	}
	//}
};

template<class TYPE>
struct DynamicWidgetByCommand : public DynamicWidget<TYPE>
{
	bool keySet;

private:
	df::interface_key scrollUp;

	void changeIndexTo(TYPE t)
	{
		if (t >= 0 && t < size)
		{
			int tempIndex = *index;

			for (int i = tempIndex; t > i; i++)
				sendInput((df::interface_key)(scrollUp + 1));
			for (int i = tempIndex; t < i; i--)
				sendInput(scrollUp);
		}
	}
public:

	void autoSize(df::interface_key upkey)
	{
		if (keySet)
			DynamicWidget::autoSize(scrollUp);
	}

	void setUpKey(df::interface_key ik)
	{
		scrollUp = ik;
		keySet = true;
	}

	void init(string n)
	{
		DynamicWidget::init(n);
		keySet = false;
	}

	bool mouseOver()
	{
		if (keySet)
			return DynamicWidget::mouseOver();
		else
			return Widget::mouseOver();
	}
};

struct DynamicWidgetByVisual : public Widget
{
	int index;
	int delay;
	int size;
	int target;
	bool primary;

	int indexLength;

	bool swiped;

	int yDown;

	df::interface_key defaultClick;
	df::interface_key pageUp;
	df::interface_key scrollUp;

	void init(string n)
	{
		Widget::init(n);
		indexLength = 1;
		swiped = false;
		yDown = -1;
		target = -1;
		delay = 0;
		defaultClick = SELECT;
		pageUp = STANDARDSCROLL_PAGEUP;
		scrollUp = STANDARDSCROLL_UP;
		primary = true;
	}

	bool mouseDown()
	{
		if (Widget::mouseDown())
		{
			yDown = *y;
			return true;
		}
		return false;
	}

	bool mouseOver()
	{
		if (delay > 0)
			delay--;

		if (Widget::mouseOver())
		{
			if (delay == 0)
			{
				index = -1;
				size = dims.bottom - dims.top + 1;
				for (int i = dims.bottom; i >= dims.top; i--)
				{
					Screen::Pen p = Screen::readTile(dims.left, i);
					//pout->print("f(%u, %u) - ch: %u, fg: %u, bold: %u\n", dims.left, i, p.ch, p.fg, p.bold);
					if (p.ch == 0)
					{
						size = (i - dims.top) / indexLength;
					}

					if (primary)
					{
						if (p.bold)
						{
							index = (i - dims.top) / indexLength;
						}
					}
					else
					{
						if (p.bg == COLOR_GREEN)
						{
							index = (i - dims.top) / indexLength;
						}
					}
				}

				if (size == 0 || index == -1)
				{
					sendInput((df::interface_key)(scrollUp + 1));
					delay = 3;
				}
				else if (index != -1)
				{
					if (yDown > -1 && !swiped && *y != yDown)
					{
						if (*y > yDown)
							sendInput(pageUp);
						else
							sendInput((df::interface_key)(pageUp + 1));

						swiped = true;
					}

					target = (*y - dims.top) / indexLength;

					if (target >= size)
						target = size - 1;

					delay = std::abs(target - index) + 2;

					for (int i = index; target > i; i++)
					{
						sendInput((df::interface_key)(scrollUp + 1));

					}

					for (int i = index; target < i; i--)
					{
						sendInput(scrollUp);
					}
				}
			}

			return true;
		}
		else
		{
			yDown = -1;
			return false;
		}
	}

	bool mouseUp()
	{
		bool val = false;

		if (Widget::mouseUp())
		{
			if (*y == yDown)
				sendInput(defaultClick);

			val = true;
		}

		swiped = false;
		yDown = -1;

		return val;
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

	void init(string n)
	{
		Widget::init(n);
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

	bool mouseOver()
	{
		if (Widget::mouseOver())
		{
			for (int i = 0; i < items.size(); i++)
			{
				if (*x >= items[i]->col + dims.left && *x < items[i]->col + items[i]->display.size() + dims.left && *y == items[i]->row + dims.top)
				{
					index = i;
					return true;
				}
			}
			return true;
		}
		index = -1;
		return false;
	}

	bool mouseUp()
	{
		if (Widget::mouseUp())
		{
			if (index > -1)
			{
				items[index]->action();
			}
			return true;
		}
		return false;
	}
};

struct LocalMapWidget : public Widget
{
protected:
	df::embark_location* local;

public:
	bool localSet;
	int mouseX;
	int mouseY;
	int downX;
	int downY;

	void draw()
	{
		if (mouseX > -1)
		{
			Screen::Pen p = Screen::readTile(mouseX, mouseY);
			Screen::paintTile(Screen::Pen('X', COLOR_YELLOW, p.bg), mouseX, mouseY);
		}

	}

	void init(string n)
	{
		Widget::init(n);

		localSet = false;
		mouseX = -1;
		mouseY = -1;
		downX = -1;
		downY = -1;
	}

	bool mouseDown()
	{
		if (Widget::mouseDown())
		{
			if (localSet)
			{
				downX = *x;
				downY = *y;

				int tempX = downX - dims.left;
				int tempY = downY - dims.top;

				local->embark_pos_max.x = tempX;
				local->embark_pos_min.x = tempX;
				local->embark_pos_max.y = tempY;
				local->embark_pos_min.y = tempY;
			}
			else
			{
				/*string s = "mouseDown(" + to_string((long long)x) + ", " + to_string((long long)y) + ")";
				bug(s);*/
				//bug("mouseDown()");
			}
			return true;
		}
		else
		{
			downX = -1;
			downY = -1;
			return false;
		}
	}

	bool mouseOver()
	{
		if (Widget::mouseOver())
		{
			if (localSet)
			{
				mouseX = *x;
				mouseY = *y;

				if (downX > -1)
				{
					int tempX = mouseX - dims.left;
					int tempY = mouseY - dims.top;

					if (tempX > downX - dims.left)
						local->embark_pos_max.x = tempX;
					else if (tempX < downX - dims.left)
						local->embark_pos_min.x = tempX;
					else
					{
						local->embark_pos_max.x = tempX;
						local->embark_pos_min.x = tempX;
					}

					if (tempY > downY - dims.top)
						local->embark_pos_max.y = tempY;
					else if (tempY < downY - dims.top)
						local->embark_pos_min.y = tempY;
					else
					{
						local->embark_pos_max.y = tempY;
						local->embark_pos_min.y = tempY;
					}
				}
				else
				{
					//bug("mouseOver()");
				}
			}
			return true;
		}
		else
		{
			mouseX = -1;
			mouseY = -1;
			return false;
		}
	}

	bool mouseUp()
	{
		downX = -1;
		downY = -1;

		return Widget::mouseUp();
	}

	void setLocation(df::embark_location* el)
	{
		local = el;
		localSet = true;
	}
};

struct OneClickWidget : public Widget
{
	df::interface_key defaultKey;
	void init(string n) { Widget::init(n); defaultKey = LEAVESCREEN; }
	bool mouseUp() {
		sendInput(defaultKey); return Widget::mouseUp();
	}
};

template<class T>
struct RegionMapWidget : public Widget
{
protected:
	T* cursorX;
	T* cursorY;

public:
	bool cursorSet;
	int mouseX;
	int mouseY;

	void erase()
	{

	}

	void draw()
	{
		if (mouseX > -1)
		{
			Screen::Pen p = Screen::readTile(mouseX, mouseY);
			Screen::paintTile(Screen::Pen('X', COLOR_YELLOW, p.bg), mouseX, mouseY);
		}

	}

	void init(string n)
	{
		Widget::init(n);

		fg = COLOR_GREY;
		cursorSet = false;
		mouseX = -1;
		mouseY = -1;
	}

	bool mouseOver()
	{
		if (Widget::mouseOver())
		{
			mouseX = *x;
			mouseY = *y;
			return true;
		}
		else
		{
			mouseX = -1;
			mouseY = -1;
			return false;
		}
	}

	bool mouseUp()
	{
		if (Widget::mouseUp())
		{
			if (cursorSet)
			{
				int h = dims.right - dims.left + 1;
				int v = dims.bottom - dims.top + 1;

				int tempX = *x - dims.left;
				int tempY = *y - dims.top;

				double baseX = *cursorX - h / 2;
				double baseY = *cursorY - v / 2;

				if (baseX < 0)
					baseX = 0;
				else if (baseX > df::global::world->world_data->world_width - h)
					baseX = df::global::world->world_data->world_width - h;

				if (baseY < 0)
					baseY = 0;
				else if (baseY > df::global::world->world_data->world_height - v)
					baseY = df::global::world->world_data->world_height - v;

				int newX = tempX + baseX;
				int newY = tempY + baseY;

				int diff;

				if (newX < *cursorX)
				{
					diff = *cursorX - newX;
					for (int i = 0; i < diff; i++)
						sendInput(CURSOR_LEFT);
				}
				else if (newX > *cursorX)
				{
					diff = newX - *cursorX;
					for (int i = 0; i < diff; i++)
						sendInput(CURSOR_RIGHT);
				}

				if (newY < *cursorY)
				{
					diff = *cursorY - newY;
					for (int i = 0; i < diff; i++)
						sendInput(CURSOR_UP);
				}
				else if (newY > *cursorY)
				{
					diff = newY - *cursorY;
					for (int i = 0; i < diff; i++)
						sendInput(CURSOR_DOWN);
				}
			}
			else
			{
				//bug("mouseUp()");
			}
			return true;
		}
		return false;
	}

	void setCursor(T* h, T* v)
	{
		cursorX = h;
		cursorY = v;
		cursorSet = true;
	}
};

struct StaticWidget : public Widget
{
	int index;
	bool recolor;

	std::vector<Item*> items;

	void erase()
	{
		for (int i = 0; i < items.size(); i++)
			delete items[i];
	}

	void draw()
	{
		if (recolor)
		{
			muteColors(COLOR_WHITE);
			drawClickable(COLOR_LIGHTGREEN);
		}

		if (index > -1)
		{
			int writeX = dims.left + items[index]->col;
			int writeY = dims.top + items[index]->row;
			
			Screen::Pen p = Screen::readTile(writeX, writeY);

			int leng = items[index]->display.size();

			if (writeX + leng >= dims.right)
				leng = dims.right - writeX;

			Screen::paintString(Screen::Pen('?', fg, p.bg), writeX, writeY, items[index]->display.substr(0, leng));
		}

	}

	void drawClickable(color_value tfg)
	{
		int writeX;
		int writeY;

		Screen::Pen p;

		for (int i = 0; i < items.size(); i++)
		{
			writeX = dims.left + items[i]->col;
			writeY = dims.top + items[i]->row;

			p = Screen::readTile(writeX, writeY);

			Screen::paintString(Screen::Pen('?', tfg, p.bg), writeX, writeY, items[i]->display);
		}
	}

	void init(string n)
	{
		Widget::init(n);
		index = -1;
		items.clear();
		fg = COLOR_GREY;
		recolor = true;
	}

	//bool mouseDown(int x, int y){}

	bool mouseOver()
	{
		if (Widget::mouseOver())
		{
			for (int i = 0; i < items.size(); i++)
			{
				if (*x >= items[i]->col + dims.left && *x < items[i]->col + items[i]->display.size() + dims.left && *y == items[i]->row + dims.top)
				{
					index = i;
					return true;
				}
			}
			return true;
		}
		index = -1;
		return false;
	}

	bool mouseUp()
	{
		if (Widget::mouseUp())
		{
			if (index > -1)
			{
				items[index]->action();
			}
			return true;
		}
		return false;
	}
};

struct TextViewerWidget : public Widget
{
protected:
	int* index;
	int size;

public:
	bool indexSet;
	bool sizeSet;

	int paneDownY;
	int barDownY;

	void init(string n) 
	{ 
		Widget::init(n);
		paneDownY = -1;
		barDownY = -1;
	}

	//virtual void draw() {}
	//virtual void erase() {}

	int height;
	double newScrollPos;

	virtual bool mouseDown() 
	{ 
		if (Widget::mouseDown() && indexSet && sizeSet)
		{
			if (*x == dims.right || *x == dims.right - 1)
			{
				barDownY = *y;
			}
			else
			{
				paneDownY = *y;
			}

			return true;
		}

		return false;
	}
	virtual bool mouseOver() 
	{ 
		if (Widget::mouseOver() && indexSet && sizeSet)
		{
			height = dims.bottom - dims.top + 1;

			if (barDownY != -1)
			{
				newScrollPos = ((*y - dims.top) / (double)height) * size;
			}
			else if (paneDownY != -1)
			{
				height -= 4;
				newScrollPos = *index + paneDownY - *y;
			}
			else
				return true;

			if (newScrollPos > size - height)
				newScrollPos = size - height;

			if (newScrollPos < 0)
				newScrollPos = 0;

			*index = newScrollPos;

			paneDownY = *y;

			return true;
		}

		return false;
	}
	virtual bool mouseUp() 
	{
		paneDownY = -1;
		barDownY = -1;
		return Widget::mouseUp(); 
	}

	void setIndex(int* ip)
	{
		index = ip;
		indexSet = true;;
	}

	void setSize(int i)
	{
		size = i;
		sizeSet = true;
	}
};

//advanced widgets
template<class T>
struct DynamicMultiWidget : public DynamicWidget<T>
{
	StaticWidget selector;

	void erase()
	{
		selector.erase();
	}

	void draw() 
	{
		int v = dims.top + *index - offset * (dims.bottom - dims.top + 1);
		selector.setDims(dims.left, dims.right, v, v);

		for (int i = 0; i < selector.items.size(); i++)
		{
			
			int writeX = selector.dims.left + selector.items[i]->col;
			int writeY = selector.dims.top + selector.items[i]->row;

			int leng = selector.items[i]->display.size();

			if (writeX + leng >= dims.right)
				leng = dims.right - writeX;

			Screen::paintString(Screen::Pen('?', fg, bg), writeX, writeY, selector.items[i]->display.substr(0, leng));
		}

		selector.draw();
	}

	void init(string n)
	{
		DynamicWidget::init(n);
		selector.init(n + ".selector");
		selector.recolor = false;
		defaultClick = STRING_A255;
		
		bg = COLOR_GREY;
		fg = COLOR_DARKGREY;
	}

	bool mouseDown()
	{
		selector.mouseDown();
		return DynamicWidget::mouseDown();
	}

	bool mouseOver()
	{
		selector.mouseOver();
		return DynamicWidget::mouseOver();
	}

	bool mouseUp()
	{
		selector.mouseUp();
		return DynamicWidget::mouseUp();
	}
};

template<class T>
struct DynamicSelectWidgetOld : public DynamicWidget<T>
{
	int selectedIndex;

	void draw()
	{
			int parDown;
			int start;
			int end;
			int length;
			int printStart;
			int printEnd;
			int printLength;

			if (horizontal)
			{
				parDown = xDown;
				start = dims.left;
				end = dims.right;
				printStart = dims.top;
				printEnd = dims.bottom;
			}
			else
			{
				parDown = yDown;
				start = dims.top;
				end = dims.bottom;
				printStart = dims.left;
				printEnd = dims.right;
			}

			length = (end - start + 1);
			printLength = (printEnd - printStart + 1);

			if (selectedIndex >= offset * length / indexLength && selectedIndex < (offset + 1) * length / indexLength)
			{
				int selectedTemp = start + selectedIndex * indexLength - offset * length / indexLength;
				int temp = start + *index * indexLength - offset * length / indexLength;

				Screen::Pen p;

				if (horizontal)
				{
					for (int i = 0; i < printLength; i++)
					{
						if (size > 0)
						{
							p = Screen::readTile(temp, printStart + i);
							p.bold = false;
							p.bg = COLOR_GREEN;
							paintTile(p, temp, printStart + i);
						}
						p = Screen::readTile(selectedTemp, printStart + i);
						p.bold = true;
						paintTile(p, selectedTemp, printStart + i);
						//paintTile(Screen::Pen(p.ch, fg, p.bg), selectedTemp, printStart + i);
					}
				}
				else
				{
					for (int i = 0; i < printLength; i++)
					{
						if (size > 0)
						{
							p = Screen::readTile(printStart + i, temp);
							p.bold = false;
							p.bg = COLOR_GREEN;
							paintTile(p, printStart + i, temp);

							p = Screen::readTile(printStart + i, selectedTemp);
						}
						p.bold = true;
						paintTile(p, printStart + i, selectedTemp);
						//paintTile(Screen::Pen(p.ch, fg, p.bg), printStart + i, selectedTemp);
					}
				}
			}
	}

	void init(string n)
	{
		DynamicWidget::init(n);
		defaultClick = STRING_A255;
		selectedIndex = 0;
	}
	
	bool mouseOver()
	{
		if (DynamicWidget::mouseOver())
		{
			return true;
		}
		else
		{
			if (indexSet)
			{
				*index = selectedIndex;
			}
			else
			{
				bug("mouseOver()");
			}
			return false;
		}
	}

	bool mouseUp()
	{
		bool val = false;

		if (over())
		{
			int parDown;
			int par;

			if (horizontal)
			{
				parDown = xDown;
				par = *x;
			}
			else
			{
				parDown = yDown;
				par = *y;
			}

			if (par == parDown)
			{
				selectedIndex = *index;
				sendInput(defaultClick);
			}
			val = true;
		}

		swiped = false;
		xDown = -1;
		yDown = -1;
		return val;
	}

	void preFeed() 
	{
		if (indexSet && !enabler->mouse_lbut)
			*index = selectedIndex;
	}
	void postFeed()
	{
		if (indexSet && !enabler->mouse_lbut)
		{
			selectedIndex = *index;
		}

		DynamicWidget::postFeed();
	}

	void setIndex(T* ip)
	{
		if (!indexSet)
		{
			DynamicWidget::setIndex(ip);
			selectedIndex = *index;
		}
	}

	T getIndex()
	{
		T tempT = *index;
		return tempT;
	}
};

template<class T>
struct DynamicSelectWidget : public DynamicWidget<T>
{
	int selectedIndex;
	int* par;

	int* start;
	int* end;
	int length;

	int* printStart;
	int* printEnd;

	int printIndex;
	int printSelectedIndex;

	int pageBase;
	int nextPageBase;

	int printIterator;
	int* printH;
	int* printV;

	Screen::Pen p;

	void draw()
	{
		DynamicWidget::draw();

		if (size > 0)
		{
			length = (*end - *start + 1);
			pageBase = offset * length / indexLength;
			nextPageBase = (offset + 1) * length / indexLength;

			printIndex = (*index - pageBase) * indexLength + *start;
			printIterator = *printStart;

			//pout->print("index: %u, selIndex: %u, length: %u, indexLeng: %u, pageBase: %u, nextPageBase: %u\n", *index, selectedIndex, length, indexLength, pageBase, nextPageBase);
			//pout->print("pI: %u\n", printIndex);

			while (printIterator <= *printEnd)
			{
				p = Screen::readTile(*printH, *printV);
				p.bold = false;
				p.bg = COLOR_GREEN;
				paintTile(p, *printH, *printV);
				printIterator++;
			}
			//pout->print("%u <= %u < %u\n", pageBase, selectedIndex, nextPageBase);

			if (selectedIndex >= pageBase && selectedIndex < nextPageBase)
			{
				printIndex = (selectedIndex - pageBase) * indexLength + *start;
				printIterator = *printStart;

				//pout->print("pSI: %u\n", printIndex);

				while (printIterator <= *printEnd)
				{
					p = Screen::readTile(*printH, *printV);
					p.bold = true;
					paintTile(p, *printH, *printV);
					printIterator++;
				}
			}
		}
	}

	bool mouseOver()
	{
		if (DynamicWidget::mouseOver())
			return true;
		else
		{
			if (indexSet)
			{
				*index = selectedIndex;
				length = (*end - *start + 1);
				offset = floor((double)selectedIndex * indexLength / length);
			}
			return false;
		}
	}

	void mouseUpAction()
	{
		selectedIndex = *index;
		DynamicWidget::mouseUpAction();
	}

	void preFeed()
	{
		//pout->print("preFeed1: %u, %u\n", indexSet, *mouseLCurrent);
		if (indexSet && !*mouseLCurrent)
			*index = selectedIndex;
	}
	void postFeed()
	{
		if (indexSet && !*mouseLCurrent)
			selectedIndex = *index;

		/*DynamicWidget::postFeed();*/
	}

	void setIndex(T* ip)
	{
		if (!indexSet)
		{
			DynamicWidget::setIndex(ip);
			selectedIndex = *ip;
			length = (*end - *start + 1);
			offset = floor((double)selectedIndex * indexLength / length);
		}
	}
	void vertical(bool b)
	{
		if (b)
		{
			start = &dims.top;
			end = &dims.bottom;
			printStart = &dims.left;
			printEnd = &dims.right;
			printH = &printIterator;
			printV = &printIndex;
			par = y;
		}
		else
		{
			start = &dims.left;
			end = &dims.right;
			printStart = &dims.top;
			printEnd = &dims.bottom;
			printH = &printIndex;
			printV = &printIterator;
			par = x;
		}

		DynamicWidget::vertical(b);
	}

	T getIndex()
	{
		T tempT = *index;
		return tempT;
	}

	void init(string n)
	{
		DynamicWidget::init(n);
		defaultClick = STRING_A255;
		vertical(true);
	}
};

#define WIDGET_LOOP(method) for (int i = 0; i < widgets[groupIndex].size(); i++) widgets[groupIndex][i]->method

//basic menus
template<class T>
struct Menu
{
private:
	int screenXHistory;
	int screenYHistory;
	bool needCalc;
	vector<vector<Widget*>> widgets;
	vector<Widget*> holder;

protected:
	T* view;
	string name;
	Rect dims;
	int groupIndex;

	void addToGroup(Widget* w)
	{
		holder.push_back(w);
	}
	void submitGroup()
	{
		if (holder.size() > 0)
		{
			widgets.push_back(holder);
			holder.clear();
		}
	}
	void mouseDown(){ WIDGET_LOOP(mouseDown()); }
	void mouseOver() { WIDGET_LOOP(mouseOver()); }
	void mouseUp(){ needCalc = true; WIDGET_LOOP(mouseUp()); }
	void setDims(int l, int r, int t, int b) { dims.left = l; dims.right = r; dims.top = t; dims.bottom = b; }
	virtual void calcDims(){}

public:
	void draw()
	{ 
		EXPLORE(("Ex:" + name + "\n").c_str());
		WIDGET_LOOP(draw());
	}
	void erase()
	{
		for (int j = 0; j < widgets.size(); j++)
			for (int i = 0; i < widgets[groupIndex].size(); i++)
				widgets[j][i]->erase();
	}
	virtual bool preFeed(set<df::interface_key> *input) { WIDGET_LOOP(preFeed()); return true; }
	virtual void postFeed() { WIDGET_LOOP(postFeed()); }
	virtual void logic()
	{
		if (gps && enabler)
		{
			if (screenXHistory != Screen::getWindowSize().x || screenYHistory != Screen::getWindowSize().y)
			{
				screenXHistory = Screen::getWindowSize().x;
				screenYHistory = Screen::getWindowSize().y;
				needCalc = true;
			}

			if (needCalc)
			{
				this->calcDims();
				needCalc = false;
			}

			this->mouseOver();

			if (*mouseLCurrent != mouseLHistory)
			{
				if (*mouseLCurrent)
					this->mouseDown();
				else
					this->mouseUp();

				//mouseLHistory = *mouseLCurrent;
			}
		}
	}
	virtual void init(string n){ setDims(0, 0, 0, 0); name = n; screenXHistory = -1;; screenYHistory = -1; groupIndex = 0; needCalc = true; }
	void setView() { view = (T*)Core::getTopViewscreen(); }
};

#define INIT_TO_GROUP(w) w.init(n + ".w"); addToGroup(&w)

class options_spoof : public dfhack_viewscreen
{
public:
	options_spoof()
	{

	}

	std::string getFocusString() { return "options_spoof"; }

	void feed(set<df::interface_key> *events)
	{
		dfhack_viewscreen::feed(events);
		if (events->count(df::interface_key::LEAVESCREEN))
		{
			Screen::dismiss(this);
			return;
		}
	}

	//void logic() { /*1*/ };

	void render()
	{
		Screen::clear();
		dfhack_viewscreen::render();
		Screen::drawBorder("  Options Spoof  ");
	}

	void resize(int32_t x, int32_t y)
	{
		dfhack_viewscreen::resize(x, y);
	}

protected:
	//ListColumn<size_t> menu_options;
};

//Interposed viewscreens
#define HOOK_DEFINE(name, viewscreen, menu)									\
struct name : public viewscreen												\
{																			\
	typedef viewscreen interpose_base;										\
	bool mouseLTemp;														\
																			\
	DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))	\
	{																		\
		if (ignoreNextClick)												\
		{																	\
			mouseLTemp = *mouseLCurrent;									\
			*mouseLCurrent = 0;												\
			INTERPOSE_NEXT(feed)(input);									\
			*mouseLCurrent = mouseLTemp;									\
			return;															\
		}																	\
																			\
		if(menu.preFeed(input))												\
		{																	\
			INTERPOSE_NEXT(feed)(input);									\
		}																	\
		menu.postFeed();													\
	}																		\
																			\
	DEFINE_VMETHOD_INTERPOSE(void, logic, ())								\
	{																		\
		INTERPOSE_NEXT(logic)();											\
		if (ignoreNextClick)												\
			return;															\
		menu.setView();														\
		menu.logic();														\
	}																		\
																			\
	DEFINE_VMETHOD_INTERPOSE(void, render, ())								\
	{																		\
		INTERPOSE_NEXT(render)();											\
		if (ignoreNextClick)												\
		{																	\
			Screen::paintTile(Screen::Pen('X', COLOR_LIGHTRED, COLOR_BLACK), *x, *y);\
			return;															\
		}																	\
		menu.setView();														\
		menu.draw();														\
	}																		\
};																			\
																			\
IMPLEMENT_VMETHOD_INTERPOSE(name, feed);									\
IMPLEMENT_VMETHOD_INTERPOSE(name, logic);									\
IMPLEMENT_VMETHOD_INTERPOSE(name, render)

//struct Menu : public Menu<df::viewscreen>
//{
//	void calcDims()
//	{ 
//		Menu::calcDims(); 
//		/**/ 
//	}
//
//	void logic(){ /**/ Menu::logic(); }
//
//	void init(string n)
//	{ 
//		Menu::init(n); 
//		/**/ 
//		INIT_TO_GROUP(omni);
//		submitGroup();
//	}
//};

struct ChooseLanguageNameMenu : public Menu<df::viewscreen_layer_choose_language_namest>
{
	CloseWidget			c;
	DynamicWidgetByVisual	type;
	DynamicWidgetByVisual	words;
	StaticWidget		footer;

	void calcDims()
	{ 
		Menu::calcDims();

		dims.toScreen(1);
		c.dims.sub(dims, 1, 0, false, true);
		type.dims.set(dims.left + 1, dims.left + 18, dims.top + 3, dims.top + 9);
		words.dims.set(dims.left + 20, dims.left + 77, dims.top + 3, dims.top + 19);
		footer.dims.sub(dims, 78, 1, true, false);
	}

	void logic()
	{ 
		//pout->print("a: %u, b: %u, c: %u\n", &view->unk1a, view->unk1b, view->unk1c);
		//pout->print("a: %x, b: %x, c: %x, d: %x, e: %u\n", view->name, view->name->words[0], view->name->parts_of_speech[0], view->name->language, view->name->unknown);
		
		if (type.over() && type.index == -1)
			sendInput(STANDARDSCROLL_RIGHT);
		else if (words.over() && words.index == -1)
			sendInput(STANDARDSCROLL_RIGHT);


		Menu::logic(); 
	}

	void init(string n)
	{ 
		Menu::init(n); 
		/**/ 
		INIT_TO_GROUP(c);
		INIT_TO_GROUP(type);
		INIT_TO_GROUP(words);
		INIT_TO_GROUP(footer);
		submitGroup();

		HotkeyItem* si;

		DEFINE_HOTKEY_ITEM(si, footer.items, 0, 4, "Random Name", CHOOSE_NAME_RANDOM);
		DEFINE_HOTKEY_ITEM(si, footer.items, 0, 25, "Clear", CHOOSE_NAME_CLEAR);
		DEFINE_HOTKEY_ITEM(si, footer.items, 0, 70, "Done", LEAVESCREEN);

		type.primary = false;
		words.primary = false;
	}

	bool mlcBuffer;
	set<df::interface_key> tempSet;

	bool preFeed(set<df::interface_key> *input)
	{
		Menu::preFeed(input);
		tempSet = *input;
		if ((unsigned int)*tempSet.begin() == CHOOSE_NAME_RANDOM)
		{
			input->clear();
			input->insert((df::interface_key)18);
			input->insert((df::interface_key)83);
			input->insert((df::interface_key)123);
			input->insert((df::interface_key)325);
			input->insert((df::interface_key)352);

			input->insert((df::interface_key)361);
			input->insert((df::interface_key)403);
			input->insert((df::interface_key)417);
			input->insert((df::interface_key)421);
			input->insert((df::interface_key)423);

			input->insert((df::interface_key)430);
			input->insert((df::interface_key)433);
			input->insert((df::interface_key)441);
			input->insert((df::interface_key)449);
			input->insert((df::interface_key)455);

			input->insert((df::interface_key)459);
			input->insert((df::interface_key)465);
			input->insert((df::interface_key)474);
			input->insert((df::interface_key)504);
			input->insert((df::interface_key)526);

			input->insert((df::interface_key)557);
			input->insert((df::interface_key)569);
			input->insert((df::interface_key)590);
			input->insert((df::interface_key)637);
			input->insert((df::interface_key)678);

			input->insert((df::interface_key)710);
			input->insert((df::interface_key)828);
			input->insert((df::interface_key)845);
			input->insert((df::interface_key)907);
			input->insert((df::interface_key)921);

			input->insert((df::interface_key)966);
			input->insert((df::interface_key)983);
			input->insert((df::interface_key)1014);
			input->insert((df::interface_key)1023);
			input->insert((df::interface_key)1031);

			input->insert((df::interface_key)1050);
			input->insert((df::interface_key)1063);
			input->insert((df::interface_key)1069);
			input->insert((df::interface_key)1088);
			input->insert((df::interface_key)1107);

			input->insert((df::interface_key)1115);
			input->insert((df::interface_key)1136);
			input->insert((df::interface_key)1206);
			input->insert((df::interface_key)1211);
			input->insert((df::interface_key)1339);
		}
		tempSet = *input;
		mlcBuffer = *mouseLCurrent;
		*mouseLCurrent = 0;
		//unsigned int s = (unsigned int)input->size();
		//unsigned int b;
		//int i = 0;
		//while (!tempSet.empty())
		//{
		//	b = (unsigned int)*tempSet.begin();
		//	pout->print("i: %u, b: %u\n", i, b);
		//	tempSet.erase(tempSet.begin());
		//	i++;
		//}
		return true;
	}

	//bool preFeed(set<df::interface_key> *input)
	//{
	//	mlcBuffer = *mouseLCurrent;
	//	*mouseLCurrent = 0;
	//	unsigned int s = (unsigned int)input->size();
	//	unsigned int b = (unsigned int)*input->begin();
	//	//pout->print("s: %u, b: %u\n", s, b);
	//	return true;
	//}

	void postFeed()
	{
		*mouseLCurrent = mlcBuffer;
	}
};

struct CustomizeUnitMenu : public Menu<df::viewscreen_customize_unitst>
{
	CloseWidget c;
	StaticWidget footer;
	StaticWidget header1;
	StaticWidget header2;
	void calcDims()
	{ 
		Menu::calcDims(); 
		dims.toScreen(1);
		c.dims.sub(dims, 1, 0, false, true);
		footer.setDims(dims.left, dims.left + 77, dims.top + 20, dims.top + 21);
		header1.setDims(dims.left + 1, dims.left + 22, dims.top + 1, dims.top + 1);
		header2.setDims(dims.left + 1, dims.left + 29, dims.top + 2, dims.top + 2);
	}

	void logic()
	{ 
		if (view->editing_nickname || view->editing_profession)
			groupIndex = 1;
		else
			groupIndex = 0;

		/**/ 
		Menu::logic(); 
	}

	void init(string n)
	{ 
		Menu::init(n); 
		/**/ 
		INIT_TO_GROUP(c);
		INIT_TO_GROUP(header1);
		INIT_TO_GROUP(header2);
		submitGroup();

		INIT_TO_GROUP(footer);
		submitGroup();

		HotkeyItem* si;

		DEFINE_HOTKEY_ITEM(si, header1.items, 0, 3, "Customize Nickname", CUSTOMIZE_UNIT_NICKNAME);

		DEFINE_HOTKEY_ITEM(si, header2.items, 0, 3, "Customize Profession Name", CUSTOMIZE_UNIT_PROFNAME);

		DEFINE_HOTKEY_ITEM(si, footer.items, 0, 55, "Done with string entry", SELECT);
		DEFINE_HOTKEY_ITEM(si, footer.items, 1, 53, "Abort string entry", LEAVESCREEN);
	}
};

struct EmbarkOptionsMenu : public Menu<df::viewscreen_setupdwarfgamest>
{
	DynamicWidget<int>			playNow;
	DynamicSelectWidget<int>	dwarves;
	StaticWidget				dwarvesFooter;
	DynamicMultiWidget<int>		abilities;
	StaticWidget				footer;
	DynamicMultiWidget<int>		supplies;
	StaticWidget				suppliesFooter;
	DynamicMultiWidget<int>		animals;
	StaticWidget				areYouSure;
	StaticWidget				save;
	StaticWidget				saveOverwrite;
	StaticWidget				notComplete;

	int8_t warning;
	int h;

	void calcDims()
	{ 
		Menu::calcDims(); 

		dims.toScreen(1);

		playNow.setDims(dims.left, dims.right, dims.top + 1, dims.bottom - 3);
		dwarves.setDims(dims.left + 1, dims.left + 37, dims.top + 1, dims.bottom - 3);
		dwarvesFooter.setDims(dims.left + 51, dims.right, dims.bottom - 1, dims.bottom);

		h = (dims.right - dwarves.dims.right) / 2 + dwarves.dims.right - 18;
		abilities.setDims(h, h + 35, dims.top + 1, dims.bottom - 3);

		footer.setDims(dims.left, dims.left + 50, dims.bottom - 1, dims.bottom);
		supplies.setDims(dims.left + 1, dims.left + 36, dims.top + 1, dims.bottom - 3);
		suppliesFooter.setDims(dims.left + 51, dims.right, dims.bottom - 1, dims.bottom);

		h = (dims.right - supplies.dims.right + 1) / 2 + supplies.dims.right - 8;
		animals.setDims(h, h + 26, dims.top + 1, dims.bottom - 3);
		areYouSure.setDims(dims.left + 2, dims.left + 75, dims.top + 9, dims.top + 13);
		save.setDims(dims.left + 3, dims.left + 73, dims.top + 12, dims.top + 12);
		saveOverwrite.dims = save.dims;
		notComplete.setDims(dims.left + 3, dims.left + 73, dims.top + 19, dims.top + 19);
	}

	void logic()
	{
		//pout->print("a: %u, b: %u, c: %u, d: %u\n", view->anon_4, view->anon_5, view->in_save_profile, view->anon_6);
		//Magic memory offset
		warning = *(int8_t*)(((int32_t)&view->animal_cursor) + 17);

		if (view->show_play_now)
		{
			groupIndex = 0;

			//pout->print("a: %u, b: %u, c: %u, d: %u\n", view->anon_1.size(), view->anon_2.size(), view->anon_3.size(), view->units.size());
			playNow.setIndex(&view->choice);
			playNow.setSize(view->anon_1.size());
		}
		else if (warning)
		{
			groupIndex = 1;
		}
		else if (view->anon_5)
		{
			groupIndex = 8;
		}
		else if (view->anon_6)
		{
			groupIndex = 7;
		}
		else if (view->in_save_profile)
		{
			groupIndex = 6;
		}
		else
		{
			if (view->mode)
			{
				footer.items[0]->display = "Dwarves";

				if (*x != -1)
				{
					if (animals.over())
						view->supply_column = 1;
					else //if (supplies.over(x, y))
						view->supply_column = 0;
				}

				if (view->supply_column)
					groupIndex = 2;
				else
					groupIndex = 3;

				supplies.setIndex(&view->item_cursor);
				supplies.setSize(view->items.size());
				supplies.selector.items[2]->display = "[" + to_string((long long)view->items[view->item_cursor]->getStackSize()) + "]";
				supplies.selector.items[2]->col = supplies.selector.items[0]->col - (supplies.selector.items[2]->display.size() + 1);

				animals.setIndex(&view->animal_cursor);
				animals.setSize(view->animals.count.size());
				animals.selector.items[2]->display = "[" + to_string((long long)view->animals.count[view->animal_cursor]) + "]";
				animals.selector.items[2]->col = animals.selector.items[0]->col - (animals.selector.items[2]->display.size() + 1);
			}
			else
			{
				footer.items[0]->display = "Items";

				if (*x != -1)
				{
					if (abilities.over() && dwarves.selectedIndex == dwarves.getIndex())
						view->dwarf_column = 1;
					else //if (dwarves.over(x, y))
						view->dwarf_column = 0;
				}

				if (view->dwarf_column)
					groupIndex = 4;
				else
					groupIndex = 5;

				dwarves.setIndex(&view->dwarf_cursor);
				dwarves.setSize(view->dwarf_info.size());

				abilities.setIndex(&view->skill_cursor);
				abilities.setSize(view->embark_skills.size());
				abilities.selector.items[2]->display = "[" + to_string((long long)view->dwarf_info[view->dwarf_cursor]->skills[view->embark_skills[view->skill_cursor]] + 5) + "]";
				abilities.selector.items[2]->col = abilities.selector.items[0]->col - (abilities.selector.items[2]->display.size() + 1);
			}
		}

		Menu::logic(); 
	}

	void init(string n)
	{ 
		Menu::init(n);

		INIT_TO_GROUP(playNow);
		submitGroup();		// 0

		INIT_TO_GROUP(areYouSure);
		submitGroup();		// 1

		INIT_TO_GROUP(animals);
		INIT_TO_GROUP(footer);
		submitGroup();		// 2

		INIT_TO_GROUP(supplies);
		INIT_TO_GROUP(suppliesFooter);
		addToGroup(&footer);
		submitGroup();		// 3

		INIT_TO_GROUP(abilities);
		addToGroup(&footer);
		submitGroup();		// 4

		INIT_TO_GROUP(dwarves);
		INIT_TO_GROUP(dwarvesFooter);
		addToGroup(&footer);
		submitGroup();		// 5

		INIT_TO_GROUP(save);
		submitGroup();		// 6

		INIT_TO_GROUP(saveOverwrite);
		submitGroup();		// 7

		INIT_TO_GROUP(notComplete);
		submitGroup();		// 8

		HotkeyItem* si;

		DEFINE_HOTKEY_ITEM(si, areYouSure.items, 3, 8, "I am ready!", SELECT);
		DEFINE_HOTKEY_ITEM(si, areYouSure.items, 3, 42, "Go back", LEAVESCREEN);

		DEFINE_HOTKEY_ITEM(si, dwarvesFooter.items, 0, 4, "View", SETUPGAME_VIEW);
		DEFINE_HOTKEY_ITEM(si, dwarvesFooter.items, 0, 13, "Customize", SETUPGAME_CUSTOMIZE_UNIT);

		DEFINE_HOTKEY_ITEM(si, abilities.selector.items, 0, 26, " - ", SECONDSCROLL_UP);
		DEFINE_HOTKEY_ITEM(si, abilities.selector.items, 0, 30, " + ", SECONDSCROLL_DOWN);
		DEFINE_HOTKEY_ITEM(si, abilities.selector.items, 0, 23, "[]", STRING_A255);

		DEFINE_HOTKEY_ITEM(si, supplies.selector.items, 0, 25, " - ", SECONDSCROLL_UP);
		DEFINE_HOTKEY_ITEM(si, supplies.selector.items, 0, 29, " + ", SECONDSCROLL_DOWN);
		DEFINE_HOTKEY_ITEM(si, supplies.selector.items, 0, 22, "[]", STRING_A255);

		DEFINE_HOTKEY_ITEM(si, suppliesFooter.items, 0, 4, "New", SETUPGAME_NEW);

		DEFINE_HOTKEY_ITEM(si, animals.selector.items, 0, 16, " - ", SECONDSCROLL_UP);
		DEFINE_HOTKEY_ITEM(si, animals.selector.items, 0, 20, " + ", SECONDSCROLL_DOWN);
		DEFINE_HOTKEY_ITEM(si, animals.selector.items, 0, 13, "[]", STRING_A255);

		DEFINE_HOTKEY_ITEM(si, footer.items, 0, 6, "Items", CHANGETAB);
		DEFINE_HOTKEY_ITEM(si, footer.items, 0, 27, "Embark!", SETUP_EMBARK);
		DEFINE_HOTKEY_ITEM(si, footer.items, 1, 4, "Name Fortress", SETUP_NAME_FORT);
		DEFINE_HOTKEY_ITEM(si, footer.items, 1, 27, "Name Group", SETUP_NAME_GROUP);
		DEFINE_HOTKEY_ITEM(si, footer.items, 1, 44, "Save", SETUPGAME_SAVE_PROFILE);

		DEFINE_HOTKEY_ITEM(si, save.items, 0, 7, "Save", SETUPGAME_SAVE_PROFILE);
		DEFINE_HOTKEY_ITEM(si, save.items, 0, 49, "Abort", SETUPGAME_SAVE_PROFILE_ABORT);

		DEFINE_HOTKEY_ITEM(si, saveOverwrite.items, 0, 7, "Overwrite", SELECT);
		DEFINE_HOTKEY_ITEM(si, saveOverwrite.items, 0, 41, "Go Back", LEAVESCREEN);

		DEFINE_HOTKEY_ITEM(si, notComplete.items, 0, 5, "Done", LEAVESCREEN);
	}

	set<df::interface_key> tempSet;

	bool preFeed(set<df::interface_key> *input)
	{
		Menu::preFeed(input);
		tempSet = *input;
		if ((unsigned int)*tempSet.begin() == SETUPGAME_SAVE_PROFILE && view->in_save_profile)
		{
			input->clear();
			input->insert(SELECT);
			input->insert(CLOSE_MEGA_ANNOUNCEMENT);
			input->insert(WORLD_PARAM_ENTER_VALUE);
			input->insert(SETUPGAME_SAVE_PROFILE_GO);
			input->insert(D_BURROWS_DEFINE);
			input->insert(D_MILITARY_ALERTS_SET);
		}
		tempSet = *input;
		unsigned int s = (unsigned int)input->size();
		unsigned int b;
		int i = 0;
		while (!tempSet.empty())
		{
			b = (unsigned int)*tempSet.begin();
			//pout->print("i: %u, b: %u\n", i, b);
			tempSet.erase(tempSet.begin());
			i++;
		}
		return true;
	}

	void postFeed()
	{
		Menu::postFeed();
	}
};

struct PlaceholderMenu : public Menu<df::viewscreen>
{
	CloseWidget c;

	void calcDims()
	{ 
		Menu::calcDims();
		dims.toScreen(1);
		c.dims.sub(dims, 1, 0, false, true);
	}

	void logic(){ /**/ Menu::logic(); }

	void init(string n)
	{ 
		Menu::init(n); 
		
		INIT_TO_GROUP(c);
		submitGroup();
	}
};

struct LoadgameMenu : public Menu<df::viewscreen_loadgamest>
{
	CloseWidget c;
	DynamicWidget<int> middle;
	Widget blank;

	void calcDims()
	{ 
		Menu::calcDims();
		dims.toScreen(0);
		c.dims.sub(dims, 1, 0, false, true);
		middle.setDims(dims.left, dims.right, dims.top + 2, dims.top + 21);
	}

	void logic()
	{
		groupIndex = view->loading;

		middle.setIndex(&view->sel_idx);
		middle.setSize(view->saves.size()); 

		Menu::logic();
	}

	void init(string n)
	{ 
		Menu::init(n);

		INIT_TO_GROUP(c);
		INIT_TO_GROUP(middle);
		middle.indexLength = 2;
		submitGroup();			//0

		INIT_TO_GROUP(blank);
		submitGroup();			//1
	}
};

struct MovieplayerMenu : public Menu<df::viewscreen_movieplayerst>
{
	OneClickWidget omni;

	void calcDims()
	{
		Menu::calcDims();
		dims.toScreen(0);
		omni.dims = dims;
	}

	void logic(){ /**/ Menu::logic(); }

	void init(string n)
	{
		Menu::init(n); 
		INIT_TO_GROUP(omni);
		submitGroup();
	}
};

#define RADIO_POINTER_ROW(p) \
	((RadioItem*)radioBoard.items[ii])->index = &view->p; ii++; \
	((RadioItem*)radioBoard.items[ii])->index = &view->p; ii++; \
	((RadioItem*)radioBoard.items[ii])->index = &view->p; ii++; \
	((RadioItem*)radioBoard.items[ii])->index = &view->p; ii++; \
	((RadioItem*)radioBoard.items[ii])->index = &view->p; ii++


struct NewRegionMenu : public Menu<df::viewscreen_new_regionst>
{
	CloseWidget c;
	OneClickWidget introText;
	StaticWidget radioBoard;
	StaticWidget radioFooter;
	StaticWidget mapPaused;
	StaticWidget mapComplete;
	string useWorld;
	int ii;

	void calcDims()
	{ 
		Menu::calcDims(); 
		dims.toScreen(0);
		c.dims.sub(dims, 1, 0, false, true);
		introText.dims = dims;
		radioBoard.dims.sub(dims, 75, 23, true, true);
		radioFooter.setDims(radioBoard.dims.left, radioBoard.dims.left + 75, radioBoard.dims.bottom + 1, radioBoard.dims.bottom + 1);
		mapPaused.dims.sub(dims, 75, 1, true, false);
		mapComplete.dims.sub(dims, 75, 0, true, false);

		ii = 0;
		RADIO_POINTER_ROW(world_size);
		RADIO_POINTER_ROW(history);
		RADIO_POINTER_ROW(number_civs);
		RADIO_POINTER_ROW(number_sites);
		RADIO_POINTER_ROW(number_beasts);
		RADIO_POINTER_ROW(savagery);
		RADIO_POINTER_ROW(mineral_occurence);
	}

	void logic()
	{
		if (view->worldgen_presets.size() == 0)	//in load screen
		{
			groupIndex = 0;
		}
		else if (view->unk_33.size() != 0) //in intro text screen
		{
			groupIndex = 0;
		}
		else if (view->simple_mode) //Creating New World!
		{
			groupIndex = 1;

			switch (*y)
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
		}
		else if (view->in_worldgen) //Designing New World with Advanced Params
		{
			groupIndex = 0;
		}
		else //map is shown
		{
			if (view->worldgen_paused) //creation is PAUSED
			{
				groupIndex = 2;

				if (*df::global::cur_year < 2)
					mapPaused.items[0]->display = "";
				else
					mapPaused.items[0]->display = useWorld;
			}
			else if (df::global::world->worldgen_status.state == 10) //creation is COMPLETE
			{
				groupIndex = 3;
			}
			else //creation is IN PROGRESS
			{
				groupIndex = 0;
			}
		} 
		Menu::logic();
	}

	void init(string n)
	{ 
		Menu::init(n);


		INIT_TO_GROUP(introText);
		submitGroup();		//0 intro

		INIT_TO_GROUP(c);
		INIT_TO_GROUP(radioBoard);
		INIT_TO_GROUP(radioFooter);
		submitGroup();		//1	radio

		INIT_TO_GROUP(mapPaused);
		submitGroup();		//2 pause

		INIT_TO_GROUP(mapComplete);
		submitGroup();		//3 complete

		radioBoard.recolor = false;

		RadioItem* ri;
		HotkeyItem* hi;

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 11, "Pocket", &view->world_size, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 23, "Smaller", &view->world_size, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 37, "Small", &view->world_size, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 50, "Medium", &view->world_size, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 2, 63, "Large", &view->world_size, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 9, "Very Short", &view->history, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 24, "Short", &view->history, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 37, "Medium", &view->history, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 51, "Long", &view->history, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 5, 61, "Very Long", &view->history, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 10, "Very Low", &view->number_civs, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 25, "Low", &view->number_civs, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 37, "Medium", &view->number_civs, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 51, "High", &view->number_civs, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 8, 61, "Very High", &view->number_civs, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 10, "Very Low", &view->number_sites, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 25, "Low", &view->number_sites, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 37, "Medium", &view->number_sites, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 51, "High", &view->number_sites, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 11, 61, "Very High", &view->number_sites, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 10, "Very Low", &view->number_beasts, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 25, "Low", &view->number_beasts, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 37, "Medium", &view->number_beasts, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 51, "High", &view->number_beasts, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 14, 61, "Very High", &view->number_beasts, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 10, "Very Low", &view->savagery, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 25, "Low", &view->savagery, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 37, "Medium", &view->savagery, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 51, "High", &view->savagery, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 17, 61, "Very High", &view->savagery, 4);

		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 9, "Very Rare", &view->mineral_occurence, 0);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 25, "Rare", &view->mineral_occurence, 1);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 37, "Sparse", &view->mineral_occurence, 2);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 49, "Frequent", &view->mineral_occurence, 3);
		DEFINE_RADIO_ITEM(ri, radioBoard.items, 20, 61, "Everywhere", &view->mineral_occurence, 4);

		DEFINE_HOTKEY_ITEM(hi, radioFooter.items, 0, 40, "Abort", LEAVESCREEN);
		DEFINE_HOTKEY_ITEM(hi, radioFooter.items, 0, 66, "Go!", MENU_CONFIRM);

		useWorld = "Use the world as it currently exists";

		DEFINE_HOTKEY_ITEM(hi, mapPaused.items, 0, 4, useWorld, WORLD_GEN_USE);
		DEFINE_HOTKEY_ITEM(hi, mapPaused.items, 1, 4, "Continue", WORLD_GEN_CONTINUE);
		DEFINE_HOTKEY_ITEM(hi, mapPaused.items, 1, 28, "Abort", WORLD_GEN_ABORT);

		DEFINE_HOTKEY_ITEM(hi, mapComplete.items, 0, 8, "Accept", SELECT);
		DEFINE_HOTKEY_ITEM(hi, mapComplete.items, 0, 21, "Abort", WORLD_GEN_ABORT);
		DEFINE_HOTKEY_ITEM(hi, mapComplete.items, 0, 33, "Export image/info", WORLDGEN_EXPORT_MAP);
	}
};

struct OptionMenu : public Menu<df::viewscreen_optionst>
{
	DynamicWidget<int> middle;

	void calcDims()
	{
		Menu::calcDims();
		dims.toScreen(1);

		middle.setIndex(&view->sel_idx);
		middle.setSize(view->options.size());

		middle.setDims(dims.left, dims.right, dims.top + 1, dims.top + view->options.size());
	}

	void logic(){ /**/ Menu::logic(); }

	void init(string n)
	{
		Menu::init(n);

		INIT_TO_GROUP(middle);
		submitGroup();
	}
};

struct SelectItemMenu : public Menu<df::viewscreen_selectitemst>
{
	CloseWidget c;
	DynamicWidgetByVisual specific;
	DynamicSelectWidget<int> category;
	StaticWidget header;

	void calcDims()
	{ 
		Menu::calcDims();
		dims.toScreen(1);
		c.dims.sub(dims, 1, 0, false, true);
		category.dims.set(dims.left + 1, dims.left + 27, dims.top + 4, dims.top + 19);
		specific.dims.set(dims.left + 29, dims.left + 77, dims.top + 4, dims.top + 19);
		header.dims.sub(dims, 40, 2, true, true);
	}

	bool tempRightList;

	void logic()
	{
		//pout->print("a: %u, b: %u, c: %u, d: %u\n", (int16_t)*view->p_item_type, (int16_t)*view->p_item_subtype, (int16_t)*view->p_mattype, *view->p_matindex);

		if (specific.over())
			view->right_list = true;
		else if (category.over())
		{
			view->right_list = false;
			//pout->print("a: %u\n", specific.size);
		}


		category.setIndex(&view->sel_category);
		category.setSize(view->categories.size());

		/**/ 
		Menu::logic(); 
	}

	void init(string n)
	{ 
		Menu::init(n); 
		/**/ 
		INIT_TO_GROUP(c);
		INIT_TO_GROUP(specific);
		INIT_TO_GROUP(category);
		INIT_TO_GROUP(header);
		submitGroup();

		HotkeyItem* si;

		DEFINE_HOTKEY_ITEM(si, header.items, 1, 26, "abort", LEAVESCREEN);

		tempRightList = false;
	}
};

struct StartSiteMenu : public Menu<df::viewscreen_choose_start_sitest>
{
	StaticWidget addNoteFooter;
	StaticWidget addNoteTextFooter;
	StaticWidget defaultFooter;
	StaticWidget findFooter;
	StaticWidget noteFooter;
	StaticWidget reclaimFooter;
	StaticWidget warningPrompt;
	DynamicMultiWidget<int> find;
	DynamicSelectWidget<int> notes;
	DynamicSelectWidget<int16_t> addNoteSymbol;
	DynamicSelectWidget<int16_t> addNoteFG;
	DynamicSelectWidget<int16_t> addNoteBG;
	DynamicWidgetByCommand<int16_t> reclaim;
	DynamicSelectWidget<int32_t> yourCiv;
	LocalMapWidget local;
	RegionMapWidget<int16_t> region;

	int warning;

	void calcDims()
	{
		Menu::calcDims();
		dims.toScreen(1);

		if (warning > 0)
		{
			warningPrompt.items[0]->row = 3 * warning;
			warningPrompt.dims.top = dims.top + 8 - pow((double)2, (double)(warning - 1));
			warningPrompt.items[1]->row = warningPrompt.items[0]->row;
			warningPrompt.dims.bottom = warningPrompt.dims.top + warningPrompt.items[0]->row;
			warningPrompt.dims.left = (dims.right - dims.left) / 2 - 25;
			warningPrompt.dims.right = warningPrompt.dims.left + 53;
		}
		else
		{
			defaultFooter.dims.top = dims.bottom - 2;
			defaultFooter.dims.bottom = dims.bottom;
			defaultFooter.dims.left = dims.left;
			defaultFooter.dims.right = dims.right;

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

			reclaim.setDims(dims.right - 26, dims.right, dims.top + 3, dims.bottom - 4);
			if ((reclaim.dims.bottom - reclaim.dims.top + 1) % 2 == 1)
				reclaim.dims.bottom--;

			reclaimFooter.dims.top = defaultFooter.dims.top;
			reclaimFooter.dims.bottom = defaultFooter.dims.bottom;
			reclaimFooter.dims.left = defaultFooter.dims.left;
			reclaimFooter.dims.right = defaultFooter.dims.right;

			notes.dims.top = dims.top + 1;
			notes.dims.bottom = dims.top + 16;
			notes.dims.left = dims.right - 26;
			notes.dims.right = dims.right;

			addNoteFooter.dims.top = defaultFooter.dims.top;
			addNoteFooter.dims.bottom = defaultFooter.dims.bottom;
			addNoteFooter.dims.left = defaultFooter.dims.left;
			addNoteFooter.dims.right = defaultFooter.dims.right;

			addNoteTextFooter.dims.top = defaultFooter.dims.top;
			addNoteTextFooter.dims.bottom = defaultFooter.dims.bottom;
			addNoteTextFooter.dims.left = defaultFooter.dims.left;
			addNoteTextFooter.dims.right = defaultFooter.dims.right;

			noteFooter.dims.top = defaultFooter.dims.top;
			noteFooter.dims.bottom = defaultFooter.dims.bottom;
			noteFooter.dims.left = defaultFooter.dims.left;
			noteFooter.dims.right = defaultFooter.dims.right;

			addNoteSymbol.dims.top = dims.top + 13;
			addNoteSymbol.dims.bottom = addNoteSymbol.dims.top;
			addNoteSymbol.dims.left = dims.right - 26;
			addNoteSymbol.dims.right = dims.right - 1;

			addNoteFG.dims.top = addNoteSymbol.dims.top + 1;
			addNoteFG.dims.bottom = addNoteFG.dims.top;
			addNoteFG.dims.left = addNoteSymbol.dims.left;
			addNoteFG.dims.right = addNoteSymbol.dims.left + 15;

			addNoteBG.dims.top = addNoteSymbol.dims.top + 2;
			addNoteBG.dims.bottom = addNoteBG.dims.top;
			addNoteBG.dims.left = addNoteSymbol.dims.left;
			addNoteBG.dims.right = addNoteSymbol.dims.left + 7;

			find.dims.top = dims.top + 2;
			find.dims.bottom = dims.top + 16;
			find.dims.left = dims.right - 26;
			find.dims.right = dims.right;

			findFooter.dims.sub(dims, 60, 1, true, false);

			yourCiv.dims.top = dims.top + 3;
			yourCiv.dims.bottom = dims.bottom - 6;
			yourCiv.dims.left = dims.right - 27;
			yourCiv.dims.right = dims.right;

		}
	}

	void init(string n)
	{
		Menu::init(n);

		INIT_TO_GROUP(warningPrompt);
		submitGroup();		// 0

		INIT_TO_GROUP(local);
		INIT_TO_GROUP(region);
		INIT_TO_GROUP(defaultFooter);
		submitGroup();		// 1

		addToGroup(&local);
		addToGroup(&region);
		addToGroup(&defaultFooter);
		INIT_TO_GROUP(yourCiv);
		submitGroup();		// 2
		
		INIT_TO_GROUP(reclaim);
		INIT_TO_GROUP(reclaimFooter);
		submitGroup();		// 3

		INIT_TO_GROUP(find);
		INIT_TO_GROUP(findFooter);
		submitGroup();		// 4

		addToGroup(&region);
		INIT_TO_GROUP(notes);
		INIT_TO_GROUP(noteFooter);
		submitGroup();		// 5

		INIT_TO_GROUP(addNoteTextFooter);
		submitGroup();		// 6

		addToGroup(&local);
		INIT_TO_GROUP(addNoteSymbol);
		INIT_TO_GROUP(addNoteFG);
		INIT_TO_GROUP(addNoteBG);
		INIT_TO_GROUP(addNoteFooter);
		submitGroup();		// 7
		
		addToGroup(&reclaimFooter);
		submitGroup();		// 8

		addToGroup(&findFooter);
		submitGroup();		// 9

		HotkeyItem* si;
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 0, 42, "Notes", SETUP_NOTES);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 2, 5, "Change Mode", CHANGETAB);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 2, 21, "Embark!", SETUP_EMBARK);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 2, 33, "Reclaim/Unretire", SETUP_RECLAIM);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 2, 54, "Find Desired Location", SETUP_FIND);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 1, 51, "F1", SETUP_BIOME_1);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 1, 53, "F2", SETUP_BIOME_2);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 1, 55, "F3", SETUP_BIOME_3);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 1, 57, "F4", SETUP_BIOME_4);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 1, 59, "F5", SETUP_BIOME_5);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 1, 61, "F6", SETUP_BIOME_6);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 1, 63, "F7", SETUP_BIOME_7);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 1, 65, "F8", SETUP_BIOME_8);
		DEFINE_HOTKEY_ITEM(si, defaultFooter.items, 1, 67, "F9", SETUP_BIOME_9);

		DEFINE_HOTKEY_ITEM(si, reclaimFooter.items, 2, 5, "Change Mode", CHANGETAB);
		//DEFINE_HOTKEY_ITEM(si, reclaimFooter.items, 2, 21, "Reclaim!", SETUP_EMBARK);
		DEFINE_HOTKEY_ITEM(si, reclaimFooter.items, 2, 37, "Cancel Reclaim/Unretire", LEAVESCREEN);

		DEFINE_HOTKEY_ITEM(si, addNoteFooter.items, 1, 7, "Enter Text", SELECT);
		DEFINE_HOTKEY_ITEM(si, addNoteFooter.items, 1, 22, "Delete", SETUP_NOTES_DELETE_NOTE);
		//DEFINE_HOTKEY_ITEM(si, addNoteFooter.items, 2, 33, "Adopt Symbol", SETUP_NOTES_ADOPT_SYMBOL);
		DEFINE_HOTKEY_ITEM(si, addNoteFooter.items, 2, 68, "List", LEAVESCREEN);

		DEFINE_HOTKEY_ITEM(si, addNoteTextFooter.items, 1, 7, "Text Done", SELECT);

		DEFINE_HOTKEY_ITEM(si, noteFooter.items, 1, 3, "Add Note", SETUP_NOTES_TAKE_NOTES);
		DEFINE_HOTKEY_ITEM(si, noteFooter.items, 1, 46, "View Note", SELECT);
		DEFINE_HOTKEY_ITEM(si, noteFooter.items, 1, 66, "Delete Note", SETUP_NOTES_DELETE_NOTE);
		DEFINE_HOTKEY_ITEM(si, noteFooter.items, 2, 68, "Done", LEAVESCREEN);

		DEFINE_HOTKEY_ITEM(si, find.selector.items, 0, 10, " - ", STANDARDSCROLL_LEFT);
		DEFINE_HOTKEY_ITEM(si, find.selector.items, 0, 14, " + ", STANDARDSCROLL_RIGHT);

		DEFINE_HOTKEY_ITEM(si, findFooter.items, 0, 35, "Do Search", SELECT);
		DEFINE_HOTKEY_ITEM(si, findFooter.items, 1, 33, "Done", LEAVESCREEN);

		DEFINE_HOTKEY_ITEM(si, warningPrompt.items, 3, 9, "Embark!", SELECT);
		DEFINE_HOTKEY_ITEM(si, warningPrompt.items, 3, 37, "Cancel", LEAVESCREEN);

		notes.indexLength = 2;
		reclaim.indexLength = 2;
		reclaim.defaultClick = SETUP_EMBARK;
		reclaim.setUpKey(SECONDSCROLL_UP);

		addNoteSymbol.vertical(false);
		addNoteSymbol.defaultClick = SETUP_NOTES_ADOPT_SYMBOL;
		addNoteFG.vertical(false);
		addNoteFG.defaultClick = SETUP_NOTES_ADOPT_SYMBOL;
		addNoteBG.vertical(false);
		addNoteBG.defaultClick = SETUP_NOTES_ADOPT_SYMBOL;

		warning = 0;
	}

	void logic()
	{
		warning = view->in_embark_aquifer + view->in_embark_salt + view->in_embark_large + view->in_embark_normal;

		if (warning > 0)
		{
			groupIndex = 0;
		}
		else
		{
			int tempBio;
			bool tempHL;

			if (view->page != 0)
			{
				defaultFooter.items[5]->display = "";
				defaultFooter.items[6]->display = "";
				defaultFooter.items[7]->display = "";
				defaultFooter.items[8]->display = "";
				defaultFooter.items[9]->display = "";
				defaultFooter.items[10]->display = "";
				defaultFooter.items[11]->display = "";
				defaultFooter.items[12]->display = "";
				defaultFooter.items[13]->display = "";
			}

			switch (view->page)
			{
			case 0:
				groupIndex = 1;

				tempBio = view->biome_idx;
				tempHL = view->biome_highlighted;

				defaultFooter.items[4]->display = view->finder.finder_state == -1 ? "Find Desired Location" : "Clear find results";

				sendInput(SETUP_BIOME_2);
				if (view->biome_idx == 1)
				{
					defaultFooter.items[5]->display = "F1";
					defaultFooter.items[6]->display = "F2";
				}
				else
				{
					defaultFooter.items[5]->display = "";
					defaultFooter.items[6]->display = "";
				}
				sendInput(SETUP_BIOME_3);
				defaultFooter.items[7]->display = view->biome_idx == 2 ? "F3" : "";
				sendInput(SETUP_BIOME_4);
				defaultFooter.items[8]->display = view->biome_idx == 3 ? "F4" : "";
				sendInput(SETUP_BIOME_5);
				defaultFooter.items[9]->display = view->biome_idx == 4 ? "F5" : "";
				sendInput(SETUP_BIOME_6);
				defaultFooter.items[10]->display = view->biome_idx == 5 ? "F6" : "";
				sendInput(SETUP_BIOME_7);
				defaultFooter.items[11]->display = view->biome_idx == 6 ? "F7" : "";
				sendInput(SETUP_BIOME_8);
				defaultFooter.items[12]->display = view->biome_idx == 7 ? "F8" : "";
				sendInput(SETUP_BIOME_9);
				defaultFooter.items[13]->display = view->biome_idx == 8 ? "F9" : "";

				view->biome_idx = tempBio;
				view->biome_highlighted = tempHL;

				local.setLocation(&view->location);
				region.setCursor(&view->location.region_pos.x, &view->location.region_pos.y);
				break;
			case 2:
				groupIndex = 2;
				local.setLocation(&view->location);
				region.setCursor(&view->location.region_pos.x, &view->location.region_pos.y);
				yourCiv.setIndex(&view->civ_idx);
				yourCiv.setSize(view->available_civs.size());
				if (yourCiv.over())
				{
					//forces map to show highlights
					sendInput(SECONDSCROLL_UP);
					sendInput(SECONDSCROLL_DOWN);
				}
				break;
			case 1:
			case 3:
			case 4:
				groupIndex = 1;
				local.setLocation(&view->location);
				region.setCursor(&view->location.region_pos.x, &view->location.region_pos.y);
				break;
			case 5:
				groupIndex = 3;
				reclaim.setIndex(&view->location.reclaim_idx);
				reclaim.autoSize(SECONDSCROLL_UP);
				break;
			case 6:
				groupIndex = 8;
				break;
			case 7:
				if (view->finder.search_x == -1)
				{
					groupIndex = 4;
					find.setIndex(&view->finder.cursor);
					find.setSize(15);
					findFooter.items[0]->display = "Do Search";
					if (view->finder.finder_state == -1 || view->finder.finder_state == 0)
						findFooter.items[1]->display = "Done";
					else
						findFooter.items[1]->display = "Browse Results";
				}
				else
				{
					groupIndex = 9;
					findFooter.items[0]->display = "";
					findFooter.items[1]->display = "Stop Search Here";
				}
				break;
			case 8:
				if (view->unk_15c == -1)
				{
					groupIndex = 5;

					region.setCursor(&view->location.region_pos.x, &view->location.region_pos.y);
					notes.setIndex(&view->unk_150);
					notes.setSize(view->unk_12c.size());

					if (view->unk_12c.size() > 0)
					{
						noteFooter.items[1]->display = "View Note";
						noteFooter.items[2]->display = "Delete Note";
					}
					else
					{
						noteFooter.items[1]->display = "";
						noteFooter.items[2]->display = "";
					}
				}
				else if ((int8_t)view->unk_14c)	//editing text
				{
					groupIndex = 6;
				}
				else
				{
					groupIndex = 7;

					local.setLocation(&view->location);
					addNoteSymbol.setIndex(&view->unk_156);
					addNoteSymbol.setSize(255);
					addNoteFG.setIndex(&view->unk_158);
					addNoteFG.setSize(16);
					addNoteBG.setIndex(&view->unk_15a);
					addNoteBG.setSize(8);

					if (addNoteSymbol.over())
						view->unk_154 = 0;
					else if (addNoteFG.over())
						view->unk_154 = 1;
					else if (addNoteBG.over())
						view->unk_154 = 2;
				}
				break;
			}
		}

		Menu::logic();
	}
};

struct TextViewerMenu : public Menu<df::viewscreen_textviewerst>
{
	CloseWidget c;
	TextViewerWidget middle;

	void calcDims()
	{
		dims.toScreen(0);

		middle.dims = dims;
		c.dims.set(dims.right - 2, dims.right - 1, dims.top + 1, dims.top + 1);
	}

	void init(string n)
	{
		Menu::init(n);

		INIT_TO_GROUP(c);
		INIT_TO_GROUP(middle);
		submitGroup();		// 0
	}

	void logic()
	{
		middle.setIndex(&view->scroll_pos);
		middle.setSize(view->formatted_text.size());

		Menu::logic(); 
	}
};

//typedef int func(void);
struct TitleMenu : public Menu<df::viewscreen_titlest>
{
	CloseWidget c;
	DynamicWidget<int> none;
	DynamicWidget<int> selectWorld;
	DynamicWidget<int> selectMode;
	DynamicWidget<int> arena;
	OneClickWidget about;
	StaticWidget noneFooter;
	int weirdTop;
	int mid;

	void calcDims()
	{ 
		Menu::calcDims(); 
		dims.toScreen(0);
		c.dims.sub(dims, 1, 0, false, true);
		weirdTop = (dims.bottom - 2) / 6 + 5;
		if((dims.bottom - 2) % 6 == 1) 
			weirdTop--;
		none.dims.set(dims.left, dims.right, weirdTop, weirdTop + view->menu_line_id.size() - 1);
		mid = (dims.right - dims.left) / 2;
		//noneFooter.dims.set(mid - 6, mid + 6, dims.bottom - 7, dims.bottom - 5);
		selectWorld.dims.set(dims.left, dims.right, 11, 19);
		selectMode.dims = selectWorld.dims;
		arena.dims = selectWorld.dims;
		about.dims = dims;
	}

	bool feed()
	{
		return true;
	}

	void logic()
	{
		groupIndex = view->sel_subpage;
		if(groupIndex == 3 && view->loading)
			groupIndex = 4;

		none.setIndex(&view->sel_menu_line);
		none.setSize(view->menu_line_id.size());

		selectWorld.setIndex(&view->sel_submenu_line);
		selectWorld.setSize(view->start_savegames.size());

		selectMode.setIndex(&view->sel_menu_line);
		selectMode.setSize(view->submenu_line_id.size());

		arena.setIndex(&view->sel_submenu_line);
		arena.setSize(view->continue_savegames.size());
		
		Menu::logic();

		//if (*x == 0 && *y == 0)
		//{
		//	func* f = (func*)(Core::getInstance().p->getBase() + 0x0048dbc9);
		//	int i = f();
		//}
	}

	void init(string n)
	{ 
		Menu::init(n); 

		INIT_TO_GROUP(none);
		//INIT_TO_GROUP(noneFooter);
		submitGroup();			// 0
		
		INIT_TO_GROUP(c);
		INIT_TO_GROUP(selectWorld);
		submitGroup();			// 1

		addToGroup(&c);
		INIT_TO_GROUP(selectMode);
		submitGroup();			// 2

		addToGroup(&c);
		INIT_TO_GROUP(arena);
		submitGroup();			// 3

		INIT_TO_GROUP(about);
		submitGroup();			// 4

		//ViewscreenItem<options_hook>* vi;
		//DEFINE_VIEWSCREEN_ITEM(vi, noneFooter.items, 0, 0, "options", options_hook);
	}
};

PlaceholderMenu menuLayersStockpile;

CustomizeUnitMenu menuCustomizeUnit;
EmbarkOptionsMenu menuEmbarkOptions;
ChooseLanguageNameMenu menuLayersChooseLanguageName;
LoadgameMenu menuLoadgame;
MovieplayerMenu menuMovieplayer;
NewRegionMenu menuNewRegion;
OptionMenu menuOptions;
//LayerWorldGenParamMenu menuLayerWorldGenParam;
//LayerWorldGenParamPresetMenu menuLayerWorldGenParamPreset;
SelectItemMenu menuSelectItem;
StartSiteMenu menuStartSite;
TitleMenu menuTitle;
TextViewerMenu menuTextViewer;

//defined hooks for existing viewscreens
HOOK_DEFINE(customize_unit_hook, df::viewscreen_customize_unitst, menuCustomizeUnit);
HOOK_DEFINE(embark_options_hook, df::viewscreen_setupdwarfgamest, menuEmbarkOptions);
HOOK_DEFINE(layer_choose_language_name_hook, df::viewscreen_layer_choose_language_namest, menuLayersChooseLanguageName);
HOOK_DEFINE(layer_stockpile_hook, df::viewscreen_layer_stockpilest, menuLayersStockpile);
HOOK_DEFINE(loadgame_hook, df::viewscreen_loadgamest, menuLoadgame);
HOOK_DEFINE(movieplayer_hook, df::viewscreen_movieplayerst, menuMovieplayer);
HOOK_DEFINE(new_region_hook, df::viewscreen_new_regionst, menuNewRegion);
HOOK_DEFINE(options_hook, df::viewscreen_optionst, menuOptions);
//HOOK_DEFINE(layer_world_gen_param_hook, df::viewscreen_layer_world_gen_paramst, menuLayerWorldGenParam);
//HOOK_DEFINE(layer_world_gen_param_preset_hook, df::viewscreen_layer_world_gen_param_presetst, menuLayerWorldGenParamPreset);
HOOK_DEFINE(select_item_menu_hook, df::viewscreen_selectitemst, menuSelectItem);
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
		INTERPOSE_HOOKS(customize_unit_hook, enable);
		INTERPOSE_HOOKS(embark_options_hook, enable);
		INTERPOSE_HOOKS(layer_choose_language_name_hook, enable);
		//INTERPOSE_HOOKS(layer_stockpile_hook, enable);
		INTERPOSE_HOOKS(loadgame_hook, enable);
		INTERPOSE_HOOKS(movieplayer_hook, enable);
		INTERPOSE_HOOKS(new_region_hook, enable);
		INTERPOSE_HOOKS(options_hook, enable);
		////INTERPOSE_HOOKS(layer_world_gen_param_hook, enable);
		////INTERPOSE_HOOKS(layer_world_gen_param_preset_hook, enable);
		INTERPOSE_HOOKS(select_item_menu_hook, enable);
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
		out.print(helpText.c_str());
	}
	else if (params.size() == 1)
	{
		if (params[0] == "enable")
		{
			out.print(" enabled\n");
			plugin_enable(out, true);
		}
		else if (params[0] == "disable")
		{
			out.print(" disabled\n");
			plugin_enable(out, false);
		}
		else if (params[0] == "explore")
		{
			exploratory = true;
		}
		else if (params[0] == "debug")
		{
			debug = true;
		}
		else if (params[0] == "stop")
		{
			debug = false;
			exploratory = false;
		}
		else if (params[0] == "help" || params[0] == "?")
		{
			out.print(helpText.c_str());
		}
		else
		{
			out.print(" unrecognized command\n");
		}
	}


	return CR_OK;
}

time_t currentTime;
time_t lastRecordedMouseOver;

//This code gets added to the DFHack main loop when "is_enabled" == true
DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
	//Screen::Pen p = Screen::readTile(*x, *y);
	//pout->print("(%u, %u) - ch: %u, fg: %u, bg: %u, bold: %u\n", *x, *y, p.ch, p.fg, p.bg, p.bold);
	//out.print("View: %x\n", Core::getTopViewscreen());
	//out.print("x: %u, ld: %u\n", *x, *mouseLCurrent);

	if (mouseLHistory && !*mouseLCurrent)
		ignoreNextClick = false;

	if (*x == -1)
	{
		time(&currentTime);
		if (currentTime >= lastRecordedMouseOver + 1)
			ignoreNextClick = true;
	}
	else
	{
		time(&lastRecordedMouseOver);
	}

	mouseLHistory = *mouseLCurrent;

	EXPLORE(("raw: " + Gui::getFocusString(Core::getTopViewscreen()) + "\n").c_str());
	//pout->print("view: %x\n", Core::getTopViewscreen());
	//pout->print("base: %s\n", Gui::getFocusString(Core::getTopViewscreen()).c_str());

	return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands)
{
	helpText = "\nUsage:\n"
		" menu-mouse [enable|disable]:\t\t enables or disables menu-mouse\n"
		"\nIn-Game:\n"
		"\n Dormant mode:\n"
		"  Starts in dormant mode.\n"
		"  Left click the screen to enter active mode.\n"
		"\n Active mode:\n"
		"  Left click on ITEMS to select them.\n"
		"  Left click on THE UPPER RIGHT-HAND \"><\" to close menus.\n"
		"  Left click on REGION MAPS to change cursor position.\n"
		"  Left click and drag on LOCAL MAPS to select an area.\n"
		"  Left click and drag on LISTS to traverse them.\n"
		"  Left click anywhere on FULLSCREEN DISPLAYS to close them.\n"
		"  Move mouse off screen to return to dormant mode.\n";

	commands.push_back(PluginCommand(
		"menu-mouse", "Adds point-and-click functionality to menus",
		menu_mouse, false, helpText.c_str()
		));

	pout = &out;
	debug = false;
	exploratory = false;

	if (!gps || !enabler)
		return CR_FAILURE;

	x = &gps->mouse_x;
	y = &gps->mouse_y;
	mouseLCurrent = &enabler->mouse_lbut;
	mouseLHistory = false;
	ignoreNextClick = false;
	time(&lastRecordedMouseOver);

	menuCustomizeUnit.init("menuCustomizeUnit");
	menuEmbarkOptions.init("menuEmbarkOptions");
	//menuKeyboard.init("menuKeyboard");
	menuLayersChooseLanguageName.init("menuLayersChooseLanguageName");
	//menuLayersStockpile.init("menuLayersStockpile");
	menuLoadgame.init("menuLoadgame");
	menuMovieplayer.init("menuMovieplayer");
	menuNewRegion.init("menuNewRegion");
	menuOptions.init("menuOptions");
	//menuLayerWorldGenParam.init("menuLayerWorldGenParam");
	//menuLayerWorldGenParamPreset.init("menuLayerWorldGenParamPreset");
	menuSelectItem.init("menuSelectItem");
	menuStartSite.init("menuStartSite");
	menuTitle.init("menuTitle");
	menuTextViewer.init("menuTextViewer");

	return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
	//menuEmbarkOptions.erase();
	//menuKeyboard.erase();
	//menuLoadgame.erase();
	//menuMovieplayer.erase();
	//menuNewRegion.erase();
	//menuLayerWorldGenParam.erase();
	//menuLayerWorldGenParamPreset.erase();
	//menuStartSite.erase();
	//menuTitle.erase();
	//menuTextViewer.erase();

	return CR_OK;
}
