#include <modules/Screen.h>
#include <modules/Translation.h>
#include <modules/Units.h>
#include <MiscUtils.h>

#include <VTableInterpose.h>

//#include "df/viewscreen_petst.h"
#include "df/viewscreen_storesst.h"
#include "df/viewscreen_tradegoodsst.h"
#include "df/viewscreen_unitlistst.h"
#include "df/interface_key.h"
#include "df/interfacest.h"

using std::set;
using std::vector;
using std::string;

using namespace DFHack;
using namespace df::enums;

using df::global::gps;
using df::global::gview;

/*
Search Plugin

A plugin that adds a "Search" hotkey to some screens (Units, Trade and Stocks)
that allows filtering of the list items by a typed query.

Works by manipulating the vector(s) that the list based viewscreens use to store
their items. When a search is started the plugin saves the original vectors and
with each keystroke creates a new filtered vector off the saves for the screen
to use.
*/


void OutputString(int8_t color, int &x, int y, const std::string &text)
{
    Screen::paintString(Screen::Pen(' ', color, 0), x, y, text);
    x += text.length();
}

static bool is_live_screen(const df::viewscreen *screen)
{
    for (df::viewscreen *cur = &gview->view; cur; cur = cur->child)
        if (cur == screen)
            return true;
    return false;
}

//
// START: Base Search functionality
//

// Parent class that does most of the work
template <class S, class T, class V = void*>
class search_parent
{
public:
    // Called each time you enter or leave a searchable screen. Resets everything.
    void reset_all()
    {
        reset_search();
        valid = false;
        sort_list1 = NULL;
        sort_list2 = NULL;
        viewscreen = NULL;
        select_key = 's';
        track_secondary_values = false;
    }

    bool reset_on_change()
    {
        if (valid && is_live_screen(viewscreen))
            return false;

        reset_all();
        return true;
    }

    // A new keystroke is received in a searchable screen
    virtual bool process_input(set<df::interface_key> *input)
    {
        // If the page has two search options (Trade screen), only allow one to operate
        // at a time
        if (lock != NULL && lock != this)
            return false;

        // Allows custom preprocessing for each screen
        if (!should_check_input(input))
            return false;

        bool key_processed = true;

        if (entry_mode)
        {
            // Query typing mode

            df::interface_key last_token = *input->rbegin();
            if (last_token >= interface_key::STRING_A032 && last_token <= interface_key::STRING_A126)
            {
                // Standard character
                search_string += last_token - ascii_to_enum_offset;
                do_search();
            }
            else if (last_token == interface_key::STRING_A000)
            {
                // Backspace
                if (search_string.length() > 0)
                {
                    search_string.erase(search_string.length()-1);
                    do_search();
                }
            }
            else if (input->count(interface_key::SELECT) || input->count(interface_key::LEAVESCREEN))
            {
                // ENTER or ESC: leave typing mode
                end_entry_mode();
            }
            else if  (input->count(interface_key::CURSOR_UP) || input->count(interface_key::CURSOR_DOWN)
                || input->count(interface_key::CURSOR_LEFT) || input->count(interface_key::CURSOR_RIGHT))
            {
                // Arrow key pressed. Leave entry mode and allow screen to process key
                end_entry_mode();
                key_processed = false;
            }
        }
        // Not in query typing mode
        else if (input->count(select_token))
        {
            // Hotkey pressed, enter typing mode
            start_entry_mode();
        }
        else if (input->count((df::interface_key) (select_token + shift_offset)))
        {
            // Shift + Hotkey pressed, clear query
            clear_search();
        }
        else
        {
            // Not a key for us, pass it on to the screen
            key_processed = false;
        }

        return key_processed || entry_mode; // Only pass unrecognized keys down if not in typing mode
    }

    // Called if the search should be redone after the screen processes the keystroke.
    // Used by the stocks screen where changing categories should redo the search on
    // the new category.
    virtual void do_post_update_check()
    {
        if (redo_search)
        {
            do_search();
            redo_search = false;
        }
    }

    static search_parent<S,T,V> *lock;

protected:
    const S *viewscreen;
    vector <T> saved_list1, reference_list;
    vector <V> saved_list2;
    vector <int> saved_indexes;

    bool valid;
    bool redo_search;
    bool track_secondary_values;
    string search_string;

    search_parent() : ascii_to_enum_offset(interface_key::STRING_A048 - '0'), shift_offset('A' - 'a')
    {
        reset_all();
    }

    virtual void init(int *cursor_pos, vector <T> *sort_list1, vector <V> *sort_list2 = NULL, char select_key = 's')
    {
        this->cursor_pos = cursor_pos;
        this->sort_list1 = sort_list1;
        this->sort_list2 = sort_list2;
        this->select_key = select_key;
        select_token = (df::interface_key) (ascii_to_enum_offset + select_key);
        track_secondary_values = false;
        valid = true;
    }

    bool is_entry_mode()
    {
        return entry_mode;
    }

    void start_entry_mode()
    {
        entry_mode = true;
        lock = this;
    }
    
    void end_entry_mode()
    {
        entry_mode = false;
        lock = NULL;
    }

    void reset_search()
    {
        end_entry_mode();
        search_string = "";
        saved_list1.clear();
        saved_list2.clear();
        reference_list.clear();
        saved_indexes.clear();
    }

    // If the second vector is editable (i.e. Trade screen vector used for marking). then it may
    // have been edited while the list was filtered. We have to update the original unfiltered
    // list with these values. Uses a stored reference vector to determine if the list has been 
    // reordered after filtering, in which case indexes must be remapped.
    void update_secondary_values()
    {
        if (sort_list2 != NULL && track_secondary_values)
        {
            bool list_has_been_sorted = (sort_list1->size() == reference_list.size()
                && *sort_list1 != reference_list);

            for (size_t i = 0; i < saved_indexes.size(); i++)
            {
                int adjusted_item_index = i;
                if (list_has_been_sorted)
                {
                    for (size_t j = 0; j < sort_list1->size(); j++)
                    {
                        if ((*sort_list1)[j] == reference_list[i])
                        {
                            adjusted_item_index = j;
                            break;
                        }
                    }
                }

                saved_list2[saved_indexes[i]] = (*sort_list2)[adjusted_item_index];
            }
            saved_indexes.clear();
        }
    }

    // Store a copy of filtered list, used later to work out if filtered list has been sorted after filtering
    void store_reference_values()
    {
        if (track_secondary_values)
            reference_list = *sort_list1;
    }

    // Shortcut to clear the search immediately
    void clear_search()
    {
        if (saved_list1.size() > 0)
        {
            *sort_list1 = saved_list1;
            if (sort_list2 != NULL) 
            {
                update_secondary_values();
                *sort_list2 = saved_list2;
            }

            saved_list1.clear();
            saved_list2.clear();
        }
        store_reference_values();
        search_string = "";
    }

    // The actual sort
    void do_search()
    {
        if (search_string.length() == 0)
        {
            clear_search();
            return;
        }

        if (saved_list1.size() == 0)
        {
            // On first run, save the original list
            saved_list1 = *sort_list1;
            if (sort_list2 != NULL)
                saved_list2 = *sort_list2;
        }
        else
            update_secondary_values(); // Update original list with any modified values

        // Clear viewscreen vectors
        sort_list1->clear();
        if (sort_list2 != NULL)
        {
            sort_list2->clear();
            saved_indexes.clear();
        }

        string search_string_l = toLower(search_string);
        for (size_t i = 0; i < saved_list1.size(); i++ )
        {
            T element = saved_list1[i];
            string desc = toLower(get_element_description(element));
            if (desc.find(search_string_l) != string::npos)
            {
                sort_list1->push_back(element);
                if (sort_list2 != NULL)
                {
                    sort_list2->push_back(saved_list2[i]);
                    if (track_secondary_values)
                        saved_indexes.push_back(i); // Used to map filtered indexes back to original, if needed
                }
            }
        }

        store_reference_values(); //Keep a copy, in case user sorts new list

        *cursor_pos = 0;
    }

    virtual bool should_check_input(set<df::interface_key> *input)
    {
        return true;
    }

    // Display hotkey message
    void print_search_option(int x, int y = -1) const
    {
        auto dim = Screen::getWindowSize();
        if (y == -1)
            y = dim.y - 2;

        OutputString((entry_mode) ? 4 : 12, x, y, string(1, select_key));
        OutputString((entry_mode) ? 10 : 15, x, y, ": Search");
        if (search_string.length() > 0 || entry_mode)
            OutputString(15, x, y, ": " + search_string);
        if (entry_mode)
            OutputString(10, x, y, "_");
    }

    virtual string get_element_description(T element) const = 0;
    virtual void render () const = 0;

private:
    vector <T> *sort_list1;
    vector <V> *sort_list2;
    int *cursor_pos;
    char select_key;

    bool entry_mode;

    df::interface_key select_token;
    const int ascii_to_enum_offset;
    const int shift_offset;

};
template <class S, class T, class V> search_parent<S,T,V> *search_parent<S,T,V> ::lock = NULL;

// Parent struct for the hooks
template <class T, class V, typename D = void>
struct search_hook : T
{
    typedef T interpose_base;

    static V module;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!module.init(this))
        {
            INTERPOSE_NEXT(feed)(input);
            return;
        }

        if (!module.process_input(input))
        {
            INTERPOSE_NEXT(feed)(input);
            module.do_post_update_check();
        }

    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        bool ok = module.init(this);
        INTERPOSE_NEXT(render)();
        if (ok)
            module.render();
    }
};

template <class T, class V, typename D> V search_hook<T, V, D> ::module;


//
// END: Base Search functionality
//



//
// START: Stocks screen search
//
class stocks_search : public search_parent<df::viewscreen_storesst, df::item*>
{
public:

    virtual void render() const
    {
        if (!viewscreen->in_group_mode)
            print_search_option(2);
        else
        {
            auto dim = Screen::getWindowSize();
            int x = 2, y = dim.y - 2;
            OutputString(15, x, y, "Tab to enable Search");
        }
    }

    virtual void do_post_update_check()
    {
        if (viewscreen->in_group_mode)
        {
            // Disable search if item lists are grouped
            clear_search();
            reset_search();
        }
        else
            search_parent::do_post_update_check();
    }

    bool init(df::viewscreen_storesst *screen)
    {
        if (screen != viewscreen && !reset_on_change())
            return false;

        if (!valid)
        {
            viewscreen = screen;
            search_parent::init(&screen->item_cursor, &screen->items);
        }

        return true;
    }


private:
    virtual string get_element_description(df::item *element) const
    {
        return Items::getDescription(element, 0, true);
    }

    virtual bool should_check_input(set<df::interface_key> *input) 
    {
        if (viewscreen->in_group_mode)
            return false;

        if ((input->count(interface_key::CURSOR_UP) || input->count(interface_key::CURSOR_DOWN)) && !viewscreen->in_right_list)
        {
            // Redo search if category changes
            saved_list1.clear();
            end_entry_mode();
            if (search_string.length() > 0)
                redo_search = true;

            return false;
        }

        return true;
    }
};


typedef search_hook<df::viewscreen_storesst, stocks_search> stocks_search_hook;
template<> IMPLEMENT_VMETHOD_INTERPOSE(stocks_search_hook, feed);
template<> IMPLEMENT_VMETHOD_INTERPOSE(stocks_search_hook, render);

//
// END: Stocks screen search
//



//
// START: Unit screen search
//
class unitlist_search : public search_parent<df::viewscreen_unitlistst, df::unit*, df::job*>
{
public:

    virtual void render() const
    {
        print_search_option(28);
    }

    bool init(df::viewscreen_unitlistst *screen)
    {
        if (screen != viewscreen && !reset_on_change())
            return false;

        if (!valid)
        {
            viewscreen = screen;
            search_parent::init(&screen->cursor_pos[viewscreen->page], &screen->units[viewscreen->page], &screen->jobs[viewscreen->page]);
        }

        return true;
    }

private:
    virtual string get_element_description(df::unit *element) const
    {
        string desc = Translation::TranslateName(Units::getVisibleName(element), false);
        desc += ", " + Units::getProfessionName(element); // Check animal type too

        return desc;
    }

    virtual bool should_check_input(set<df::interface_key> *input) 
    {
        if (input->count(interface_key::CURSOR_LEFT) || input->count(interface_key::CURSOR_RIGHT) || input->count(interface_key::CUSTOM_L))
        {
            if (!is_entry_mode())
            {
                // Changing screens, reset search
                clear_search();
                reset_all();
            }
            else
                input->clear(); // Ignore cursor keys when typing

            return false;
        }

        return true;
    }

};

typedef search_hook<df::viewscreen_unitlistst, unitlist_search> unitlist_search_hook;
template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(unitlist_search_hook, feed, 100);
template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(unitlist_search_hook, render, 100);

//
// END: Unit screen search
//


//
// TODO: Animals screen search
//

//
// END: Animals screen search
//

//
// START: Trade screen search
//
class trade_search_base : public search_parent<df::viewscreen_tradegoodsst, df::item*, char>
{

private:
    virtual string get_element_description(df::item *element) const
    {
        return Items::getDescription(element, 0, true);
    }

    virtual bool should_check_input(set<df::interface_key> *input)
    {
        if (is_entry_mode())
            return true;

        if (input->count(interface_key::TRADE_TRADE) ||
            input->count(interface_key::TRADE_OFFER) ||
            input->count(interface_key::TRADE_SEIZE))
        {
            // Block the keys if were searching
            if (!search_string.empty())
                input->clear();

            // Trying to trade, reset search
            clear_search();
            reset_all();

            return false;
        }

        return true;
    }
};


class trade_search_merc : public trade_search_base
{
public:
    virtual void render() const
    {
        print_search_option(2, 26);
    }

    bool init(df::viewscreen_tradegoodsst *screen)
    {
        if (screen != viewscreen && !reset_on_change())
            return false;

        if (!valid)
        {
            viewscreen = screen;
            search_parent::init(&screen->trader_cursor, &screen->trader_items, &screen->trader_selected, 'q');
            track_secondary_values = true;
        }

        return true;
    }
};

typedef search_hook<df::viewscreen_tradegoodsst, trade_search_merc, int> trade_search_merc_hook;
template<> IMPLEMENT_VMETHOD_INTERPOSE(trade_search_merc_hook, feed);
template<> IMPLEMENT_VMETHOD_INTERPOSE(trade_search_merc_hook, render);


class trade_search_fort : public trade_search_base
{
public:
    virtual void render() const
    {
        print_search_option(42, 26);
    }

    bool init(df::viewscreen_tradegoodsst *screen)
    {
        if (screen != viewscreen && !reset_on_change())
            return false;

        if (!valid)
        {
            viewscreen = screen;
            search_parent::init(&screen->broker_cursor, &screen->broker_items, &screen->broker_selected, 'w');
            track_secondary_values = true;
        }

        return true;
    }
};

typedef search_hook<df::viewscreen_tradegoodsst, trade_search_fort, char> trade_search_fort_hook;
template<> IMPLEMENT_VMETHOD_INTERPOSE(trade_search_fort_hook, feed);
template<> IMPLEMENT_VMETHOD_INTERPOSE(trade_search_fort_hook, render);

//
// END: Trade screen search
//


DFHACK_PLUGIN("search");


DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    if (!gps || !gview ||
        !INTERPOSE_HOOK(unitlist_search_hook, feed).apply() ||
        !INTERPOSE_HOOK(unitlist_search_hook, render).apply() ||
        !INTERPOSE_HOOK(trade_search_merc_hook, feed).apply() ||
        !INTERPOSE_HOOK(trade_search_merc_hook, render).apply() ||
        !INTERPOSE_HOOK(trade_search_fort_hook, feed).apply() ||
        !INTERPOSE_HOOK(trade_search_fort_hook, render).apply() ||
        !INTERPOSE_HOOK(stocks_search_hook, feed).apply() ||
        !INTERPOSE_HOOK(stocks_search_hook, render).apply())
        out.printerr("Could not insert Search hooks!\n");

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    INTERPOSE_HOOK(unitlist_search_hook, feed).remove();
    INTERPOSE_HOOK(unitlist_search_hook, render).remove();
    INTERPOSE_HOOK(trade_search_merc_hook, feed).remove();
    INTERPOSE_HOOK(trade_search_merc_hook, render).remove();
    INTERPOSE_HOOK(trade_search_fort_hook, feed).remove();
    INTERPOSE_HOOK(trade_search_fort_hook, render).remove();
    INTERPOSE_HOOK(stocks_search_hook, feed).remove();
    INTERPOSE_HOOK(stocks_search_hook, render).remove();
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange ( color_ostream &out, state_change_event event )
{
    switch (event) {
    case SC_VIEWSCREEN_CHANGED:
        unitlist_search_hook::module.reset_on_change();
        trade_search_merc_hook::module.reset_on_change();
        trade_search_fort_hook::module.reset_on_change();
        stocks_search_hook::module.reset_on_change();
        break;

    default:
        break;
    }

    return CR_OK;
}
