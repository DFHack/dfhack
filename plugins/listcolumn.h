#include "uicommon.h"

using df::global::enabler;
using df::global::gps;

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
    UIColor color;

    ListEntry(const string text, const T elem, const string keywords = "", const UIColor color = COLOR_UNSELECTED) :
        elem(elem), text(text), selected(false), keywords(keywords), color(color)
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
    bool allow_search;
    bool feed_mouse_set_highlight;
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
        allow_search = true;
        feed_mouse_set_highlight = false;
        feed_changed_highlight = false;
    }

    void clear()
    {
        list.clear();
        display_list.clear();
        display_start_offset = 0;
        if (highlighted_index != -1)
            highlighted_index = 0;
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

        return getMaxItemWidth();
    }

    int getMaxItemWidth()
    {
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
            UIColor fg_color = (display_list[i]->selected) ? COLOR_SELECTED : display_list[i]->color;
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

    virtual void tokenizeSearch (vector<string> *dest, const string search)
    {
        if (!search.empty())
            split_string(dest, search, " ");
    }

    virtual bool showEntry(const ListEntry<T> *entry, const vector<string> &search_tokens)
    {
        if (!search_tokens.empty())
        {
            string item_string = toLower(entry->text);
            for (auto si = search_tokens.begin(); si != search_tokens.end(); si++)
            {
                if (!si->empty() && item_string.find(*si) == string::npos &&
                    entry->keywords.find(*si) == string::npos)
                {
                    return false;
                }
            }
        }
        return true;
    }

    void filterDisplay()
    {
        ListEntry<T> *prev_selected = (getDisplayListSize() > 0) ? display_list[highlighted_index] : NULL;
        display_list.clear();

        search_string = toLower(search_string);
        vector<string> search_tokens;
        tokenizeSearch(&search_tokens, search_string);

        for (size_t i = 0; i < list.size(); i++)
        {
            ListEntry<T> *entry = &list[i];

            if (showEntry(entry, search_tokens))
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

    void centerSelection()
    {
        if (display_list.size() == 0)
            return;
        display_start_offset = highlighted_index - (display_max_rows / 2);
        validateDisplayOffset();
        validateHighlight();
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
        validateDisplayOffset();
        validateHighlight();
    }

    void validateDisplayOffset()
    {
        set_to_limit(display_start_offset, max(0, (int)(display_list.size())-display_max_rows));
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
        if (display_list.size() == 0)
            return;

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
            return (T)nullptr;
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

    virtual bool validSearchInput (unsigned char c)
    {
        return (c >= 'a' && c <= 'z') || c == ' ';
    }

    bool feed(set<df::interface_key> *input)
    {
        feed_mouse_set_highlight = feed_changed_highlight = false;
        if  (input->count(interface_key::STANDARDSCROLL_UP))
        {
            changeHighlight(-1);
        }
        else if  (input->count(interface_key::STANDARDSCROLL_DOWN))
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
            df::interface_key last_token = get_string_key(input);
            int charcode = Screen::keyToChar(last_token);
            if (charcode >= 0 && validSearchInput((unsigned char)charcode))
            {
                // Standard character
                search_string += char(charcode);
                filterDisplay();
                centerSelection();
            }
            else if (last_token == interface_key::STRING_A000)
            {
                // Backspace
                if (search_string.length() > 0)
                {
                    search_string.erase(search_string.length()-1);
                    filterDisplay();
                    centerSelection();
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
            {
                setHighlight(new_index);
                feed_mouse_set_highlight = true;
            }

            enabler->mouse_lbut = enabler->mouse_rbut = 0;

            return true;
        }

        return false;
    }

    void sort(bool force_sort = false)
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

protected:
    static void clear_fn(ListEntry<T> &e) { e.selected = false; }
    static bool sort_fn(ListEntry<T> const& a, ListEntry<T> const& b) { return a.text.compare(b.text) < 0; }

    vector<ListEntry<T>> list;
    vector<ListEntry<T>*> display_list;
    string search_string;
    string title;
    int display_max_rows;
    int max_item_width;
};

