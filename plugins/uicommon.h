#include <algorithm>
#include <map>
#include <string>
#include <set>

#include "Core.h"
#include "MiscUtils.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <VTableInterpose.h>

#include "modules/Screen.h"

#include "df/enabler.h"

using namespace std;
using std::string;
using std::vector;
using std::map;
using std::ostringstream;
using std::set;

using namespace DFHack;
using namespace df::enums;

using df::global::enabler;
using df::global::gps;


#ifndef HAVE_NULLPTR
#define nullptr 0L
#endif

#define COLOR_TITLE COLOR_BLUE
#define COLOR_UNSELECTED COLOR_GREY
#define COLOR_SELECTED COLOR_WHITE
#define COLOR_HIGHLIGHTED COLOR_GREEN


template <class T, typename Fn>
static void for_each_(vector<T> &v, Fn func)
{
    for_each(v.begin(), v.end(), func);
}

template <class T, class V, typename Fn>
static void for_each_(map<T, V> &v, Fn func)
{
    for_each(v.begin(), v.end(), func);
}

template <class T, class V, typename Fn> 
static void transform_(vector<T> &src, vector<V> &dst, Fn func)
{
    transform(src.begin(), src.end(), back_inserter(dst), func);
}

typedef int8_t UIColor;

void OutputString(UIColor color, int &x, int &y, const std::string &text, 
    bool newline = false, int left_margin = 0, const UIColor bg_color = 0)
{
    Screen::paintString(Screen::Pen(' ', color, bg_color), x, y, text);
    if (newline)
    {
        ++y;
        x = left_margin;
    }
    else
        x += text.length();
}

void OutputHotkeyString(int &x, int &y, const char *text, const char *hotkey, bool newline = false, 
    int left_margin = 0, int8_t text_color = COLOR_WHITE, int8_t hotkey_color = COLOR_LIGHTGREEN)
{
    OutputString(hotkey_color, x, y, hotkey);
    string display(": ");
    display.append(text);
    OutputString(text_color, x, y, display, newline, left_margin);
}

void OutputFilterString(int &x, int &y, const char *text, const char *hotkey, bool state, bool newline = false, 
    int left_margin = 0, int8_t hotkey_color = COLOR_LIGHTGREEN)
{
    OutputString(hotkey_color, x, y, hotkey);
    OutputString(COLOR_WHITE, x, y, ": ");
    OutputString((state) ? COLOR_WHITE : COLOR_GREY, x, y, text, newline, left_margin);
}

void OutputToggleString(int &x, int &y, const char *text, const char *hotkey, bool state, bool newline = true, int left_margin = 0, int8_t color = COLOR_WHITE)
{
    OutputHotkeyString(x, y, text, hotkey);
    OutputString(COLOR_WHITE, x, y, ": ");
    if (state)
        OutputString(COLOR_GREEN, x, y, "Enabled", newline, left_margin);
    else
        OutputString(COLOR_GREY, x, y, "Disabled", newline, left_margin);
}

const int ascii_to_enum_offset = interface_key::STRING_A048 - '0';

inline string int_to_string(const int n)
{
    return static_cast<ostringstream*>( &(ostringstream() << n) )->str();
}

static void set_to_limit(int &value, const int maximum, const int min = 0)
{
    if (value < min)
        value = min;
    else if (value > maximum)
        value = maximum;
}


inline void paint_text(const UIColor color, const int &x, const int &y, const std::string &text, const UIColor background = 0)
{
    Screen::paintString(Screen::Pen(' ', color, background), x, y, text);
}

static string pad_string(string text, const int size, const bool front = true, const bool trim = false)
{
    if (text.length() > size)
    {
        if (trim && size > 10)
        {
            text = text.substr(0, size-3);
            text.append("...");
        }
        return text;
    }

    string aligned(size - text.length(), ' ');
    if (front)
    {
        aligned.append(text);
        return aligned;
    }
    else
    {
        text.append(aligned);
        return text;
    }
}


/*
 * List classes
 */
template <typename T>
class ListEntry
{
public:
    T elem;
    string text, keywords;
    bool selected;

    ListEntry(const string text, const T elem, const string keywords = "") : 
        elem(elem), text(text), selected(false), keywords(keywords)
    {
    }
};

template <typename T>
class ListColumn
{
public:
    int highlighted_index;
    int display_start_offset;
    unsigned short text_clip_at;
    int32_t bottom_margin, search_margin, left_margin;
    bool multiselect;
    bool allow_null;
    bool auto_select;
    bool force_sort;
    bool allow_search;
    bool feed_changed_highlight;

    ListColumn()
    {
        bottom_margin = 3;
        clear();
        left_margin = 2;
        search_margin = 63;
        highlighted_index = 0;
        text_clip_at = 0;
        multiselect = false;
        allow_null = true;
        auto_select = false;
        force_sort = false;
        allow_search = true;
        feed_changed_highlight = false;
    }

    void clear()
    {
        list.clear();
        display_list.clear();
        display_start_offset = 0;
        max_item_width = title.length();
        resize();
    }

    void resize()
    {
        display_max_rows = gps->dimy - 4 - bottom_margin;
    }

    void add(ListEntry<T> &entry)
    {
        list.push_back(entry);
        if (entry.text.length() > max_item_width)
            max_item_width = entry.text.length();
    }

    void add(const string &text, const T &elem)
    {
        list.push_back(ListEntry<T>(text, elem));
        if (text.length() > max_item_width)
            max_item_width = text.length();
    }

    int fixWidth()
    {
        if (text_clip_at > 0 && max_item_width > text_clip_at)
            max_item_width = text_clip_at;

        for (auto it = list.begin(); it != list.end(); it++)
        {
            it->text = pad_string(it->text, max_item_width, false);
        }

        return left_margin + max_item_width;
    }

    virtual void display_extras(const T &elem, int32_t &x, int32_t &y) const {}

    void display(const bool is_selected_column) const
    {
        int32_t y = 2;
        paint_text(COLOR_TITLE, left_margin, y, title);

        int last_index_able_to_display = display_start_offset + display_max_rows;
        for (int i = display_start_offset; i < display_list.size() && i < last_index_able_to_display; i++)
        {
            ++y;
            UIColor fg_color = (display_list[i]->selected) ? COLOR_SELECTED : COLOR_UNSELECTED;
            UIColor bg_color = (is_selected_column && i == highlighted_index) ? COLOR_HIGHLIGHTED : COLOR_BLACK;
            
            string item_label = display_list[i]->text;
            if (text_clip_at > 0 && item_label.length() > text_clip_at)
                item_label.resize(text_clip_at);

            paint_text(fg_color, left_margin, y, item_label, bg_color);
            int x = left_margin + display_list[i]->text.length() + 1;
            display_extras(display_list[i]->elem, x, y);
        }

        if (is_selected_column && allow_search)
        {
            y = gps->dimy - 3;
            int32_t x = search_margin;
            OutputHotkeyString(x, y, "Search" ,"S");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString(COLOR_WHITE, x, y, search_string);
            OutputString(COLOR_LIGHTGREEN, x, y, "_");
        }
    }

    void filterDisplay()
    {
        ListEntry<T> *prev_selected = (getDisplayListSize() > 0) ? display_list[highlighted_index] : NULL;
        display_list.clear();

        search_string = toLower(search_string);
        vector<string> search_tokens;
        if (!search_string.empty())
            split_string(&search_tokens, search_string, " ");

        for (size_t i = 0; i < list.size(); i++)
        {
            ListEntry<T> *entry = &list[i];

            bool include_item = true;
            if (!search_string.empty())
            {
                string item_string = toLower(list[i].text);
                for (auto si = search_tokens.begin(); si != search_tokens.end(); si++)
                {
                    if (!si->empty() && item_string.find(*si) == string::npos && 
                        list[i].keywords.find(*si) == string::npos)
                    {
                        include_item = false;
                        break;
                    }
                }
            }

            if (include_item)
            {
                display_list.push_back(entry);
                if (entry == prev_selected)
                    highlighted_index = display_list.size() - 1;
            }
            else if (auto_select)
            {
                entry->selected = false;
            }
        }
        changeHighlight(0);
        feed_changed_highlight = true;
    }

    void selectDefaultEntry()
    {
        for (size_t i = 0; i < display_list.size(); i++)
        {
            if (display_list[i]->selected)
            {
                highlighted_index = i;
                break;
            }
        }
    }

    void validateHighlight()
    {
        set_to_limit(highlighted_index, display_list.size() - 1);

        if (highlighted_index < display_start_offset)
            display_start_offset = highlighted_index;
        else if (highlighted_index >= display_start_offset + display_max_rows)
            display_start_offset = highlighted_index - display_max_rows + 1;

        if (auto_select || (!allow_null && list.size() == 1))
            display_list[highlighted_index]->selected = true;

        feed_changed_highlight = true;
    }

    void changeHighlight(const int highlight_change, const int offset_shift = 0)
    {
        if (!initHighlightChange())
            return;

        highlighted_index += highlight_change + offset_shift * display_max_rows;

        display_start_offset += offset_shift * display_max_rows;
        set_to_limit(display_start_offset, max(0, (int)(display_list.size())-display_max_rows));
        validateHighlight();
    }

    void setHighlight(const int index)
    {
        if (!initHighlightChange())
            return;

        highlighted_index = index;
        validateHighlight();
    }

    bool initHighlightChange()
    {
        if (display_list.size() == 0)
            return false;

        if (auto_select && !multiselect)
        {
            for (auto it = list.begin(); it != list.end(); it++)
            {
                it->selected = false;
            }
        }

        return true;
    }

    void toggleHighlighted()
    {
        if (auto_select)
            return;

        ListEntry<T> *entry = display_list[highlighted_index];
        if (!multiselect || !allow_null)
        {
            int selected_count = 0;
            for (size_t i = 0; i < list.size(); i++)
            {
                if (!multiselect && !entry->selected)
                    list[i].selected = false;
                if (!allow_null && list[i].selected)
                    selected_count++;
            }

            if (!allow_null && entry->selected && selected_count == 1)
                return;
        }

        entry->selected = !entry->selected;
    }

    vector<T> getSelectedElems(bool only_one = false)
    {
        vector<T> results;
        for (auto it = list.begin(); it != list.end(); it++)
        {
            if ((*it).selected)
            {
                results.push_back(it->elem);
                if (only_one)
                    break;
            }
        }

        return results;
    }

    T getFirstSelectedElem()
    {
        vector<T> results = getSelectedElems(true);
        if (results.size() == 0)
            return nullptr;
        else
            return results[0];
    }

    void clearSelection()
    {
        for_each_(list, clear_fn);
    }

    void selectItem(const T elem)
    {
        int i = 0;
        for (; i < display_list.size(); i++)
        {
            if (display_list[i]->elem == elem)
            {
                setHighlight(i);
                break;
            }
        }
    }

    void clearSearch()
    {
        search_string.clear();
        filterDisplay();
    }

    size_t getDisplayListSize()
    {
        return display_list.size();
    }

    vector<ListEntry<T>*> &getDisplayList()
    {
        return display_list;
    }

    size_t getBaseListSize()
    {
        return list.size();
    }

    bool feed(set<df::interface_key> *input)
    {
        feed_changed_highlight = false;
        if  (input->count(interface_key::CURSOR_UP))
        {
            changeHighlight(-1);
        }
        else if  (input->count(interface_key::CURSOR_DOWN))
        {
            changeHighlight(1);
        }
        else if  (input->count(interface_key::STANDARDSCROLL_PAGEUP))
        {
            changeHighlight(0, -1);
        }
        else if  (input->count(interface_key::STANDARDSCROLL_PAGEDOWN))
        {
            changeHighlight(0, 1);
        }
        else if  (input->count(interface_key::SELECT) && !auto_select)
        {
            toggleHighlighted();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_S))
        {
            clearSearch();
        }
        else if (enabler->tracking_on && gps->mouse_x != -1 && gps->mouse_y != -1 && enabler->mouse_lbut)
        {
            return setHighlightByMouse();
        }
        else if (allow_search)
        {
            // Search query typing mode always on
            df::interface_key last_token = *input->rbegin();
            if ((last_token >= interface_key::STRING_A096 && last_token <= interface_key::STRING_A123) ||
                last_token == interface_key::STRING_A032)
            {
                // Standard character
                search_string += last_token - ascii_to_enum_offset;
                filterDisplay();
            }
            else if (last_token == interface_key::STRING_A000)
            {
                // Backspace
                if (search_string.length() > 0)
                {
                    search_string.erase(search_string.length()-1);
                    filterDisplay();
                }
            }
            else
            {
                return false;
            }

            return true;
        }
        else
        {
            return false;
        }

        return true;
    }

    bool setHighlightByMouse()
    {
        if (gps->mouse_y >= 3 && gps->mouse_y < display_max_rows + 3 &&
            gps->mouse_x >= left_margin && gps->mouse_x < left_margin + max_item_width)
        {
            int new_index = display_start_offset + gps->mouse_y - 3;
            if (new_index < display_list.size())
                setHighlight(new_index);

            enabler->mouse_lbut = enabler->mouse_rbut = 0;

            return true;
        }

        return false;
    }

    void sort()
    {
        if (force_sort || list.size() < 100)
            std::sort(list.begin(), list.end(), sort_fn);

        filterDisplay();
    }

    void setTitle(const string t)
    {
        title = t;
        if (title.length() > max_item_width)
            max_item_width = title.length();
    }

    size_t getDisplayedListSize()
    {
        return display_list.size();
    }

private:
    static void clear_fn(ListEntry<T> &e) { e.selected = false; }
    static bool sort_fn(ListEntry<T> const& a, ListEntry<T> const& b) { return a.text.compare(b.text) < 0; }

    vector<ListEntry<T>> list;
    vector<ListEntry<T>*> display_list;
    string search_string;
    string title;
    int display_max_rows;
    int max_item_width;
};


