#include <modules/Screen.h>
#include <modules/Translation.h>
#include <modules/Units.h>
#include <MiscUtils.h>

#include <VTableInterpose.h>

#include "uicommon.h"

#include "df/creature_raw.h"
#include "df/ui_look_list.h"
#include "df/viewscreen_announcelistst.h"
#include "df/viewscreen_petst.h"
#include "df/viewscreen_storesst.h"
#include "df/viewscreen_layer_stockpilest.h"
#include "df/viewscreen_layer_militaryst.h"
#include "df/viewscreen_layer_noblelistst.h"
#include "df/viewscreen_layer_workshop_profilest.h"
#include "df/viewscreen_topicmeeting_fill_land_holder_positionsst.h"
#include "df/viewscreen_tradegoodsst.h"
#include "df/viewscreen_unitlistst.h"
#include "df/viewscreen_buildinglistst.h"
#include "df/viewscreen_joblistst.h"
#include "df/historical_figure.h"
#include "df/viewscreen_locationsst.h"
#include "df/interface_key.h"
#include "df/interfacest.h"
#include "df/layer_object_listst.h"
#include "df/job.h"
#include "df/report.h"
#include "modules/Job.h"
#include "df/global_objects.h"
#include "df/viewscreen_dwarfmodest.h"
#include "modules/Gui.h"
#include "df/unit.h"
#include "df/misc_trait_type.h"
#include "df/unit_misc_trait.h"

using namespace std;
using std::set;
using std::vector;
using std::string;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("search");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(gview);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_building_assign_units);
REQUIRE_GLOBAL(ui_building_in_assign);
REQUIRE_GLOBAL(ui_building_item_cursor);
REQUIRE_GLOBAL(ui_look_cursor);
REQUIRE_GLOBAL(ui_look_list);

/*
Search Plugin

A plugin that adds a "Search" hotkey to some screens (Units, Trade and Stocks)
that allows filtering of the list items by a typed query.

Works by manipulating the vector(s) that the list based viewscreens use to store
their items. When a search is started the plugin saves the original vectors and
with each keystroke creates a new filtered vector off the saves for the screen
to use.
*/


void make_text_dim(int x1, int x2, int y)
{
    for (int x = x1; x <= x2; x++)
    {
        Screen::Pen pen = Screen::readTile(x,y);

        if (pen.valid())
        {
            if (pen.fg != 0)
            {
                if (pen.fg == 7)
                    pen.adjust(0,true);
                else
                    pen.bold = 0;
            }

            Screen::paintTile(pen,x,y);
        }
    }
}

static bool is_live_screen(const df::viewscreen *screen)
{
    for (df::viewscreen *cur = &gview->view; cur; cur = cur->child)
        if (cur == screen)
            return true;
    return false;
}

static string get_unit_description(df::unit *unit)
{
    if (!unit)
        return "";
    string desc;
    auto name = Units::getVisibleName(unit);
    if (name->has_name)
        desc = Translation::TranslateName(name, false);
    desc += ", " + Units::getProfessionName(unit); // Check animal type too

    return desc;
}

static bool cursor_key_pressed (std::set<df::interface_key> *input)
{
    // give text input (e.g. "2") priority over cursor keys
    for (auto it = input->begin(); it != input->end(); ++it)
    {
        if (Screen::keyToChar(*it) != -1)
            return false;
    }
    return
    input->count(df::interface_key::CURSOR_UP) ||
    input->count(df::interface_key::CURSOR_DOWN) ||
    input->count(df::interface_key::CURSOR_LEFT) ||
    input->count(df::interface_key::CURSOR_RIGHT) ||
    input->count(df::interface_key::CURSOR_UPLEFT) ||
    input->count(df::interface_key::CURSOR_UPRIGHT) ||
    input->count(df::interface_key::CURSOR_DOWNLEFT) ||
    input->count(df::interface_key::CURSOR_DOWNRIGHT) ||
    input->count(df::interface_key::CURSOR_UP_FAST) ||
    input->count(df::interface_key::CURSOR_DOWN_FAST) ||
    input->count(df::interface_key::CURSOR_LEFT_FAST) ||
    input->count(df::interface_key::CURSOR_RIGHT_FAST) ||
    input->count(df::interface_key::CURSOR_UPLEFT_FAST) ||
    input->count(df::interface_key::CURSOR_UPRIGHT_FAST) ||
    input->count(df::interface_key::CURSOR_DOWNLEFT_FAST) ||
    input->count(df::interface_key::CURSOR_DOWNRIGHT_FAST) ||
    input->count(df::interface_key::CURSOR_UP_Z) ||
    input->count(df::interface_key::CURSOR_DOWN_Z) ||
    input->count(df::interface_key::CURSOR_UP_Z_AUX) ||
    input->count(df::interface_key::CURSOR_DOWN_Z_AUX);
}

//
// START: Generic Search functionality
//

template <class S, class T>
class search_generic
{
public:
    bool init(S *screen)
    {
        if (screen != viewscreen && !reset_on_change())
            return false;

        if (!can_init(screen))
        {
            if (is_valid())
            {
                clear_search();
                reset_all();
            }

            return false;
        }

        if (!is_valid())
        {
            this->viewscreen = screen;
            this->cursor_pos = get_viewscreen_cursor();
            this->primary_list = get_primary_list();
            this->select_key = get_search_select_key();
            select_token = Screen::charToKey(select_key);
            shift_select_token = Screen::charToKey(select_key + 'A' - 'a');
            valid = true;
            do_post_init();
        }

        return true;
    }

    // Called each time you enter or leave a searchable screen. Resets everything.
    virtual void reset_all()
    {
        reset_search();
        valid = false;
        primary_list = NULL;
        viewscreen = NULL;
        select_key = 's';
    }

    bool reset_on_change()
    {
        if (valid && is_live_screen(viewscreen))
            return false;

        reset_all();
        return true;
    }

    bool is_valid()
    {
        return valid;
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

            df::interface_key last_token = get_string_key(input);
            int charcode = Screen::keyToChar(last_token);
            if (charcode >= 32 && charcode <= 126)
            {
                // Standard character
                search_string += char(charcode);
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
            else if (cursor_key_pressed(input))
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
        else if (input->count(shift_select_token))
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

    // Called after a keystroke has been processed
    virtual void do_post_input_feed()
    {
    }

    static search_generic<S, T> *lock;

    bool in_entry_mode()
    {
        return entry_mode;
    }

protected:
    virtual string get_element_description(T element) const = 0;
    virtual void render() const = 0;
    virtual int32_t *get_viewscreen_cursor() = 0;
    virtual vector<T> *get_primary_list() = 0;

    search_generic()
    {
        reset_all();
    }

    virtual bool can_init(S *screen)
    {
        return true;
    }

    virtual void do_post_init()
    {

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

    virtual char get_search_select_key()
    {
        return 's';
    }

    virtual void reset_search()
    {
        end_entry_mode();
        search_string = "";
        saved_list1.clear();
    }

    // Shortcut to clear the search immediately
    virtual void clear_search()
    {
        if (saved_list1.size() > 0)
        {
            *primary_list = saved_list1;
            saved_list1.clear();
        }
        search_string = "";
    }

    virtual void save_original_values()
    {
        saved_list1 = *primary_list;
    }

    virtual void do_pre_incremental_search()
    {

    }

    virtual void clear_viewscreen_vectors()
    {
        primary_list->clear();
    }

    virtual void add_to_filtered_list(size_t i)
    {
        primary_list->push_back(saved_list1[i]);
    }

    virtual void do_post_search()
    {

    }

    virtual bool is_valid_for_search(size_t index)
    {
        return true;
    }

    virtual bool force_in_search(size_t index)
    {
        return false;
    }

    // The actual sort
    virtual void do_search()
    {
        if (search_string.length() == 0)
        {
            clear_search();
            return;
        }

        if (saved_list1.size() == 0)
            // On first run, save the original list
            save_original_values();
        else
            do_pre_incremental_search();

        clear_viewscreen_vectors();

        string search_string_l = toLower(search_string);
        for (size_t i = 0; i < saved_list1.size(); i++ )
        {
            if (force_in_search(i))
            {
                add_to_filtered_list(i);
                continue;
            }

            if (!is_valid_for_search(i))
                continue;

            T element = saved_list1[i];
            string desc = toLower(get_element_description(element));
            if (desc.find(search_string_l) != string::npos)
            {
                add_to_filtered_list(i);
            }
        }

        do_post_search();

        if (cursor_pos)
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

    S *viewscreen;
    vector <T> saved_list1, reference_list, *primary_list;

    //bool redo_search;
    string search_string;

protected:
    int *cursor_pos;
    char select_key;
    bool valid;
    bool entry_mode;

    df::interface_key select_token;
    df::interface_key shift_select_token;
};

template <class S, class T> search_generic<S, T> *search_generic<S, T> ::lock = NULL;


// Search class helper for layered screens
template <class S, class T, int LIST_ID>
class layered_search : public search_generic<S, T>
{
protected:
    virtual bool can_init(S *screen)
    {
        auto list = getLayerList(screen);
        if (!is_list_valid(screen) || !list || !list->active)
            return false;

        return true;
    }

    virtual bool is_list_valid(S*)
    {
        return true;
    }

    virtual void do_search()
    {
        search_generic<S,T>::do_search();
        auto list = getLayerList(this->viewscreen);
        list->num_entries = this->get_primary_list()->size();
    }

    int32_t *get_viewscreen_cursor()
    {
        auto list = getLayerList(this->viewscreen);
        return &list->cursor;
    }

    virtual void clear_search()
    {
        search_generic<S,T>::clear_search();

        if (is_list_valid(this->viewscreen))
        {
            auto list = getLayerList(this->viewscreen);
            list->num_entries = this->get_primary_list()->size();
        }
    }

private:
    static df::layer_object_listst *getLayerList(const df::viewscreen_layer *layer)
    {
        return virtual_cast<df::layer_object_listst>(vector_get(layer->layer_objects, LIST_ID));
    }
};



// Parent class for screens that have more than one primary list to synchronise
template < class S, class T, class PARENT = search_generic<S,T> >
class search_multicolumn_modifiable_generic : public PARENT
{
protected:
    vector <T> reference_list;
    vector <int> saved_indexes;
    bool read_only;

    virtual void update_saved_secondary_list_item(size_t i, size_t j) = 0;
    virtual void save_secondary_values() = 0;
    virtual void clear_secondary_viewscreen_vectors() = 0;
    virtual void add_to_filtered_secondary_lists(size_t i) = 0;
    virtual void clear_secondary_saved_lists() = 0;
    virtual void reset_secondary_viewscreen_vectors() = 0;
    virtual void restore_secondary_values() = 0;

    virtual void do_post_init()
    {
        // If true, secondary list isn't modifiable so don't bother synchronising values
        read_only = false;
    }

    void reset_all()
    {
        PARENT::reset_all();
        reference_list.clear();
        saved_indexes.clear();
        reset_secondary_viewscreen_vectors();
    }

    void reset_search()
    {
        PARENT::reset_search();
        reference_list.clear();
        saved_indexes.clear();
        clear_secondary_saved_lists();
    }

    virtual void clear_search()
    {
        if (this->saved_list1.size() > 0)
        {
            do_pre_incremental_search();
            restore_secondary_values();
        }
        clear_secondary_saved_lists();
        PARENT::clear_search();
        do_post_search();
    }

    virtual bool is_match(T &a, T &b) = 0;

    virtual bool is_match(vector<T> &a, vector<T> &b) = 0;

    void do_pre_incremental_search()
    {
        PARENT::do_pre_incremental_search();
        if (read_only)
            return;

        bool list_has_been_sorted = (this->primary_list->size() == reference_list.size()
            && !is_match(*this->primary_list, reference_list));

        for (size_t i = 0; i < saved_indexes.size(); i++)
        {
            int adjusted_item_index = i;
            if (list_has_been_sorted)
            {
                for (size_t j = 0; j < this->primary_list->size(); j++)
                {
                    if (is_match((*this->primary_list)[j], reference_list[i]))
                    {
                        adjusted_item_index = j;
                        break;
                    }
                }
            }

            update_saved_secondary_list_item(saved_indexes[i], adjusted_item_index);
        }
        saved_indexes.clear();
    }

    void clear_viewscreen_vectors()
    {
        search_generic<S,T>::clear_viewscreen_vectors();
        saved_indexes.clear();
        clear_secondary_viewscreen_vectors();
    }

    void add_to_filtered_list(size_t i)
    {
        search_generic<S,T>::add_to_filtered_list(i);
        add_to_filtered_secondary_lists(i);
        if (!read_only)
            saved_indexes.push_back(i); // Used to map filtered indexes back to original, if needed
    }

    virtual void do_post_search()
    {
        if (!read_only)
            reference_list = *this->primary_list;
    }

    void save_original_values()
    {
        search_generic<S,T>::save_original_values();
        save_secondary_values();
    }
};

// This basic match function is separated out from the generic multi column class, because the
// pets screen, which uses a union in its primary list, will cause a compile failure if this
// match function exists in the generic class
template < class S, class T, class PARENT = search_generic<S,T> >
class search_multicolumn_modifiable : public search_multicolumn_modifiable_generic<S, T, PARENT>
{
    bool is_match(T &a, T &b)
    {
        return a == b;
    }

    bool is_match(vector<T> &a, vector<T> &b)
    {
        return a == b;
    }
};

// General class for screens that have only one secondary list to keep in sync
template < class S, class T, class V, class PARENT = search_generic<S,T> >
class search_twocolumn_modifiable : public search_multicolumn_modifiable<S, T, PARENT>
{
public:
protected:
    virtual vector<V> * get_secondary_list() = 0;

    virtual void do_post_init()
    {
        search_multicolumn_modifiable<S, T, PARENT>::do_post_init();
        secondary_list = get_secondary_list();
    }

    void save_secondary_values()
    {
        saved_secondary_list = *secondary_list;
    }

    void reset_secondary_viewscreen_vectors()
    {
        secondary_list = NULL;
    }

    virtual void update_saved_secondary_list_item(size_t i, size_t j)
    {
        saved_secondary_list[i] = (*secondary_list)[j];
    }

    void clear_secondary_viewscreen_vectors()
    {
        secondary_list->clear();
    }

    void add_to_filtered_secondary_lists(size_t i)
    {
        secondary_list->push_back(saved_secondary_list[i]);
    }

    void clear_secondary_saved_lists()
    {
        saved_secondary_list.clear();
    }

    void restore_secondary_values()
    {
        *secondary_list = saved_secondary_list;
    }

    vector<V> *secondary_list, saved_secondary_list;
};


// Parent struct for the hooks, use optional param D to generate multiple search classes in the same
// viewscreen but different static modules
template <class T, class V, int D = 0>
struct generic_search_hook : T
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
            module.do_post_input_feed();
        }

    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        bool ok = module.init(this);
        INTERPOSE_NEXT(render)();
        if (ok)
            module.render();
    }

    DEFINE_VMETHOD_INTERPOSE(bool, key_conflict, (df::interface_key key))
    {
        if (module.in_entry_mode() && (key == interface_key::MOVIES || key == interface_key::HELP))
            return true;
        return INTERPOSE_NEXT(key_conflict)(key);
    }
};

template <class T, class V, int D> V generic_search_hook<T, V, D> ::module;


// Hook definition helpers
#define IMPLEMENT_HOOKS_WITH_ID(screen, module, id, prio) \
    typedef generic_search_hook<screen, module, id> module##_hook; \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(module##_hook, feed, prio); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(module##_hook, render, prio); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(module##_hook, key_conflict, prio)

#define IMPLEMENT_HOOKS(screen, module) \
    typedef generic_search_hook<screen, module> module##_hook; \
    template<> IMPLEMENT_VMETHOD_INTERPOSE(module##_hook, feed); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE(module##_hook, render); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE(module##_hook, key_conflict)

#define IMPLEMENT_HOOKS_PRIO(screen, module, prio) \
    typedef generic_search_hook<screen, module> module##_hook; \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(module##_hook, feed, prio); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(module##_hook, render, prio); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(module##_hook, key_conflict, prio);

//
// END: Generic Search functionality
//


//
// START: Animal screen search
//

typedef search_multicolumn_modifiable_generic<df::viewscreen_petst, df::viewscreen_petst::T_animal> pets_search_base;
class pets_search : public pets_search_base
{
    typedef df::viewscreen_petst::T_animal T_animal;
    typedef df::viewscreen_petst::T_mode T_mode;
public:
    void render() const
    {
        print_search_option(25, 4);
    }

private:
    bool can_init(df::viewscreen_petst *screen)
    {
        return pets_search_base::can_init(screen) && screen->mode == T_mode::List;
    }

    int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->cursor;
    }

    vector<T_animal> *get_primary_list()
    {
        return &viewscreen->animal;
    }

    virtual void do_post_init()
    {
        is_vermin = &viewscreen->is_vermin;
        is_tame = &viewscreen->is_tame;
        is_adopting = &viewscreen->is_adopting;
    }

    string get_element_description(df::viewscreen_petst::T_animal element) const
    {
        return get_unit_description(element.unit);
    }

    bool should_check_input()
    {
        return viewscreen->mode == T_mode::List;
    }

    bool is_valid_for_search(size_t i)
    {
        return is_vermin_s[i] == 0;
    }

    void save_secondary_values()
    {
        is_vermin_s = *is_vermin;
        is_tame_s = *is_tame;
        is_adopting_s = *is_adopting;
    }

    void reset_secondary_viewscreen_vectors()
    {
        is_vermin = NULL;
        is_tame = NULL;
        is_adopting = NULL;
    }

    void update_saved_secondary_list_item(size_t i, size_t j)
    {
        is_vermin_s[i] = (*is_vermin)[j];
        is_tame_s[i] = (*is_tame)[j];
        is_adopting_s[i] = (*is_adopting)[j];
    }

    void clear_secondary_viewscreen_vectors()
    {
        is_vermin->clear();
        is_tame->clear();
        is_adopting->clear();
    }

    void add_to_filtered_secondary_lists(size_t i)
    {
        is_vermin->push_back(is_vermin_s[i]);
        is_tame->push_back(is_tame_s[i]);
        is_adopting->push_back(is_adopting_s[i]);
    }

    void clear_secondary_saved_lists()
    {
        is_vermin_s.clear();
        is_tame_s.clear();
        is_adopting_s.clear();
    }

    void restore_secondary_values()
    {
        *is_vermin = is_vermin_s;
        *is_tame = is_tame_s;
        *is_adopting = is_adopting_s;
    }

    bool is_match(T_animal &a, T_animal &b)
    {
        return a.unit == b.unit;
    }

    bool is_match(vector<T_animal> &a, vector<T_animal> &b)
    {
        for (size_t i = 0; i < a.size(); i++)
        {
            if (!is_match(a[i], b[i]))
                return false;
        }

        return true;
    }

    std::vector<char > *is_vermin, is_vermin_s;
    std::vector<char > *is_tame, is_tame_s;
    std::vector<char > *is_adopting, is_adopting_s;
};

IMPLEMENT_HOOKS_WITH_ID(df::viewscreen_petst, pets_search, 1, 0);

//
// END: Animal screen search
//


//
// START: Animal knowledge screen search
//

typedef search_generic<df::viewscreen_petst, int32_t> animal_knowledge_search_base;
class animal_knowledge_search : public animal_knowledge_search_base
{
    typedef df::viewscreen_petst::T_mode T_mode;
    bool can_init(df::viewscreen_petst *screen)
    {
        return animal_knowledge_search_base::can_init(screen) && screen->mode == T_mode::TrainingKnowledge;
    }

public:
    void render() const
    {
        print_search_option(2, 4);
    }

private:
    int32_t *get_viewscreen_cursor()
    {
        return NULL;
    }

    vector<int32_t> *get_primary_list()
    {
        return &viewscreen->known;
    }

    string get_element_description(int32_t id) const
    {
        auto craw = df::creature_raw::find(id);
        string out;
        if (craw)
        {
            for (size_t i = 0; i < 3; ++i)
                out += craw->name[i] + " ";
        }
        return out;
    }
};

IMPLEMENT_HOOKS_WITH_ID(df::viewscreen_petst, animal_knowledge_search, 2, 0);

//
// END: Animal knowledge screen search
//


//
// START: Animal trainer search
//

typedef search_twocolumn_modifiable<df::viewscreen_petst, df::unit*, df::viewscreen_petst::T_trainer_mode> animal_trainer_search_base;
class animal_trainer_search : public animal_trainer_search_base
{
    typedef df::viewscreen_petst::T_mode T_mode;
    typedef df::viewscreen_petst::T_trainer_mode T_trainer_mode;

    bool can_init(df::viewscreen_petst *screen)
    {
        return animal_trainer_search_base::can_init(screen) && screen->mode == T_mode::SelectTrainer;
    }

public:
    void render() const
    {
        Screen::paintTile(Screen::Pen(186, 8, 0), 14, 2);
        Screen::paintTile(Screen::Pen(186, 8, 0), gps->dimx - 14, 2);
        Screen::paintTile(Screen::Pen(201, 8, 0), 14, 1);
        Screen::paintTile(Screen::Pen(187, 8, 0), gps->dimx - 14, 1);
        for (int x = 15; x <= gps->dimx - 15; ++x)
        {
            Screen::paintTile(Screen::Pen(205, 8, 0), x, 1);
            Screen::paintTile(Screen::Pen(0, 0, 0), x, 2);
        }
        print_search_option(16, 2);
    }

private:
    int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->trainer_cursor;
    }

    vector<df::unit*> *get_primary_list()
    {
        return &viewscreen->trainer_unit;
    }

    string get_element_description(df::unit *u) const
    {
        return get_unit_description(u);
    }

    std::vector<T_trainer_mode> *get_secondary_list()
    {
        return &viewscreen->trainer_mode;
    }

public:
    bool process_input(set<df::interface_key> *input)
    {
        if (input->count(interface_key::SELECT) && viewscreen->trainer_unit.empty() && !in_entry_mode())
            return true;
        return animal_trainer_search_base::process_input(input);
    }

};

IMPLEMENT_HOOKS_WITH_ID(df::viewscreen_petst, animal_trainer_search, 3, 0);

//
// END: Animal trainer search
//


//
// START: Stocks screen search
//
typedef search_generic<df::viewscreen_storesst, df::item*> stocks_search_base;
class stocks_search : public stocks_search_base
{
public:

    void render() const
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

    bool process_input(set<df::interface_key> *input)
    {
        if (viewscreen->in_group_mode)
            return false;

        redo_search = false;

        if ((input->count(interface_key::CURSOR_UP) || input->count(interface_key::CURSOR_DOWN)) && !viewscreen->in_right_list)
        {
            // Redo search if category changes
            saved_list1.clear();
            end_entry_mode();
            if (search_string.length() > 0)
                redo_search = true;

            return false;
        }

        return stocks_search_base::process_input(input);
    }

    virtual void do_post_input_feed()
    {
        if (viewscreen->in_group_mode)
        {
            // Disable search if item lists are grouped
            clear_search();
            reset_search();
        }
        else if (redo_search)
        {
            do_search();
            redo_search = false;
        }
    }

private:
    int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->item_cursor;
    }

    virtual vector<df::item*> *get_primary_list()
    {
        return &viewscreen->items;
    }


private:
    string get_element_description(df::item *element) const
    {
        if (!element)
            return "";
        return Items::getDescription(element, 0, true);
    }

    bool redo_search;
};


IMPLEMENT_HOOKS_PRIO(df::viewscreen_storesst, stocks_search, 100);

//
// END: Stocks screen search
//


//
// START: Unit screen search
//
typedef search_twocolumn_modifiable<df::viewscreen_unitlistst, df::unit*, df::job*> unitlist_search_base;
class unitlist_search : public unitlist_search_base
{
public:
    void render() const
    {
        print_search_option(28);
    }

private:
    void do_post_init()
    {
        unitlist_search_base::do_post_init();
        read_only = true;
    }

    static string get_non_work_description(df::unit *unit)
    {
        if (!unit)
            return "";
        for (auto p = unit->status.misc_traits.begin(); p < unit->status.misc_traits.end(); p++)
        {
            if ((*p)->id == misc_trait_type::Migrant || (*p)->id == misc_trait_type::OnBreak)
            {
                int i = (*p)->value;
                return ".on break";
            }
        }

        if (Units::isBaby(unit) ||
            Units::isChild(unit) ||
            unit->profession == profession::DRUNK)
        {
            return "";
        }

        if (ENUM_ATTR(profession, military, unit->profession))
            return ".military";

        return ".idle.no job";
    }

    string get_element_description(df::unit *unit) const
    {
        if (!unit)
            return "Inactive";
        string desc = get_unit_description(unit);
        if (!unit->job.current_job)
        {
            desc += get_non_work_description(unit);
        }

        return desc;
    }

    bool should_check_input(set<df::interface_key> *input)
    {
        if (input->count(interface_key::CURSOR_LEFT) || input->count(interface_key::CURSOR_RIGHT) ||
            (!in_entry_mode() && input->count(interface_key::UNITVIEW_PRF_PROF)))
        {
            if (!in_entry_mode())
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

    char get_search_select_key()
    {
        return 'q';
    }

    vector<df::job*> *get_secondary_list()
    {
        return &viewscreen->jobs[viewscreen->page];
    }

    int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->cursor_pos[viewscreen->page];
    }

    vector<df::unit*> *get_primary_list()
    {
        return &viewscreen->units[viewscreen->page];
    }
};

typedef generic_search_hook<df::viewscreen_unitlistst, unitlist_search> unitlist_search_hook;
IMPLEMENT_HOOKS_PRIO(df::viewscreen_unitlistst, unitlist_search, 100);

//
// END: Unit screen search
//


//
// START: Trade screen search
//
class trade_search_base : public search_twocolumn_modifiable<df::viewscreen_tradegoodsst, df::item*, char>
{

private:
    string get_element_description(df::item *element) const
    {
        if (!element)
            return "";
        return Items::getDescription(element, 0, true);
    }

    bool should_check_input(set<df::interface_key> *input)
    {
        if (in_entry_mode())
            return true;

        if (input->count(interface_key::TRADE_TRADE) ||
            input->count(interface_key::TRADE_OFFER) ||
            input->count(interface_key::TRADE_SEIZE))
        {
            // Block the keys if were searching
            if (!search_string.empty())
            {
                input->clear();
            }

            return false;
        }
        else if (input->count(interface_key::CUSTOM_ALT_C))
        {
            clear_search_for_trade();
            return true;
        }

        return true;
    }

    void clear_search_for_trade()
    {
        // Trying to trade, reset search
        clear_search();
        reset_all();
    }
};


class trade_search_merc : public trade_search_base
{
public:
    virtual void render() const
    {
        if (viewscreen->counteroffer.size() > 0)
        {
            // The merchant is proposing a counteroffer.
            // Not only is there nothing to search,
            // but the native hotkeys are where we normally write.
            return;
        }

        print_search_option(2, -1);

        if (!search_string.empty())
        {
            int32_t x = 2;
            int32_t y = gps->dimy - 3;
            make_text_dim(2, gps->dimx-2, y);
            OutputString(COLOR_LIGHTRED, x, y, string(1, select_key + 'A' - 'a'));
            OutputString(COLOR_WHITE, x, y, ": Clear search to trade           ");
        }
    }

private:
    vector<char> *get_secondary_list()
    {
        return &viewscreen->trader_selected;
    }

    int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->trader_cursor;
    }

    vector<df::item*> *get_primary_list()
    {
        return &viewscreen->trader_items;
    }

    char get_search_select_key()
    {
        return 'q';
    }
};

IMPLEMENT_HOOKS_WITH_ID(df::viewscreen_tradegoodsst, trade_search_merc, 1, 100);


class trade_search_fort : public trade_search_base
{
public:
    virtual void render() const
    {
        if (viewscreen->counteroffer.size() > 0)
        {
            // The merchant is proposing a counteroffer.
            // Not only is there nothing to search,
            // but the native hotkeys are where we normally write.
            return;
        }

        int32_t x = gps->dimx / 2 + 2;
        print_search_option(x, -1);

        if (!search_string.empty())
        {
            int32_t y = gps->dimy - 3;
            make_text_dim(2, gps->dimx-2, y);
            OutputString(COLOR_LIGHTRED, x, y, string(1, select_key + 'A' - 'a'));
            OutputString(COLOR_WHITE, x, y, ": Clear search to trade           ");
        }
    }

private:
    vector<char> *get_secondary_list()
    {
        return &viewscreen->broker_selected;
    }

    int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->broker_cursor;
    }

    vector<df::item*> *get_primary_list()
    {
        return &viewscreen->broker_items;
    }

    char get_search_select_key()
    {
        return 'w';
    }
};

IMPLEMENT_HOOKS_WITH_ID(df::viewscreen_tradegoodsst, trade_search_fort, 2, 100);

//
// END: Trade screen search
//


//
// START: Stockpile screen search
//
typedef layered_search<df::viewscreen_layer_stockpilest, string *, 2> stocks_layer;
class stockpile_search : public search_twocolumn_modifiable<df::viewscreen_layer_stockpilest, string *, bool *, stocks_layer>
{
public:
    void update_saved_secondary_list_item(size_t i, size_t j)
    {
        *saved_secondary_list[i] = *(*secondary_list)[j];
    }

    string get_element_description(string *element) const
    {
        return *element;
    }

    void render() const
    {
        print_search_option(51, 23);
    }

    vector<string *> *get_primary_list()
    {
        return &viewscreen->item_names;
    }

    vector<bool *> *get_secondary_list()
    {
        return &viewscreen->item_status;
    }

    bool should_check_input(set<df::interface_key> *input)
    {
        if (input->count(interface_key::STOCKPILE_SETTINGS_DISABLE) && !in_entry_mode() && !search_string.empty())
        {
            // Restore original list
            clear_search();
            reset_all();
        }

        return true;
    }

};

IMPLEMENT_HOOKS(df::viewscreen_layer_stockpilest, stockpile_search);

//
// END: Stockpile screen search
//


//
// START: Military screen search
//
typedef layered_search<df::viewscreen_layer_militaryst, df::unit *, 2> military_search_base;
class military_search : public military_search_base
{
public:

    string get_element_description(df::unit *element) const
    {
        return get_unit_description(element);
    }

    void render() const
    {
        print_search_option(52, 22);
    }

    char get_search_select_key()
    {
        return 'q';
    }

    // When not on the positions page, this list is used for something
    // else entirely, so screwing with it seriously breaks stuff.
    bool is_list_valid(df::viewscreen_layer_militaryst *screen)
    {
        if (screen->page != df::viewscreen_layer_militaryst::Positions)
            return false;

        return true;
    }

    vector<df::unit *> *get_primary_list()
    {
        return &viewscreen->positions.candidates;
    }

    bool should_check_input(set<df::interface_key> *input)
    {
        if (input->count(interface_key::SELECT) && !in_entry_mode() && !search_string.empty())
        {
            // About to make an assignment, so restore original list (it will be changed by the game)
            int32_t *cursor = get_viewscreen_cursor();
            auto list = get_primary_list();
            if (*cursor >= list->size())
                return false;
            df::unit *selected_unit = list->at(*cursor);
            clear_search();

            for (*cursor = 0; *cursor < list->size(); (*cursor)++)
            {
                if (list->at(*cursor) == selected_unit)
                    break;
            }

            reset_all();
        }

        return true;
    }
};

IMPLEMENT_HOOKS_PRIO(df::viewscreen_layer_militaryst, military_search, 100);

//
// END: Military screen search
//


//
// START: Room list search
//
static map< df::building_type, vector<string> > room_quality_names;
static int32_t room_value_bounds[] = {1, 100, 250, 500, 1000, 1500, 2500, 10000};
typedef search_twocolumn_modifiable<df::viewscreen_buildinglistst, df::building*, int32_t> roomlist_search_base;
class roomlist_search : public roomlist_search_base
{
public:
    void render() const
    {
        print_search_option(2, 23);
    }

private:
    void do_post_init()
    {
        roomlist_search_base::do_post_init();
        read_only = true;
    }

    string get_element_description(df::building *bld) const
    {
        if (!bld)
            return "";
        bool is_ownable_room = (bld->is_room && room_quality_names.find(bld->getType()) != room_quality_names.end());

        string desc;
        desc.reserve(100);
        if (bld->owner)
            desc += get_unit_description(bld->owner);
        else if (is_ownable_room)
            desc += "no owner";

        desc += ".";

        if (is_ownable_room)
        {
            int32_t value = bld->getRoomValue(NULL);
            vector<string> *names = &room_quality_names[bld->getType()];
            string *room_name = &names->at(0);
            for (int i = 1; i < 8; i++)
            {
                if (room_value_bounds[i] > value)
                    break;
                room_name = &names->at(i);
            }

            desc += *room_name;
        }
        else
        {
            string name;
            bld->getName(&name);
            if (!name.empty())
            {
                desc += name;
            }
        }

        return desc;
    }

    vector<int32_t> *get_secondary_list()
    {
        return &viewscreen->room_value;
    }

    int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->cursor;
    }

    vector<df::building*> *get_primary_list()
    {
        return &viewscreen->buildings;
    }
};

IMPLEMENT_HOOKS(df::viewscreen_buildinglistst, roomlist_search);

//
// END: Room list search
//



//
// START: Announcement list search
//
class announcement_search : public search_generic<df::viewscreen_announcelistst, df::report*>
{
public:
    void render() const
    {
        print_search_option(2, gps->dimy - 3);
    }

private:
    int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->sel_idx;
    }

    virtual vector<df::report *> *get_primary_list()
    {
        return &viewscreen->reports;
    }


private:
    string get_element_description(df::report *element) const
    {
        if (!element)
            return "";
        return element->text;
    }
};


IMPLEMENT_HOOKS(df::viewscreen_announcelistst, announcement_search);

//
// END: Announcement list search
//


//
// START: Nobles search list
//
typedef df::viewscreen_layer_noblelistst::T_candidates T_candidates;
typedef layered_search<df::viewscreen_layer_noblelistst, T_candidates *, 1> nobles_search_base;
class nobles_search : public nobles_search_base
{
public:

    string get_element_description(T_candidates *element) const
    {
        if (!element->unit)
            return "";

        return get_unit_description(element->unit);
    }

    void render() const
    {
        print_search_option(2, 23);
    }

    bool force_in_search(size_t index)
    {
        return index == 0; // Leave Vacant
    }

    bool can_init(df::viewscreen_layer_noblelistst *screen)
    {
        if (screen->mode != df::viewscreen_layer_noblelistst::Appoint)
            return false;

        return nobles_search_base::can_init(screen);
    }

    vector<T_candidates *> *get_primary_list()
    {
        return &viewscreen->candidates;
    }
};

IMPLEMENT_HOOKS(df::viewscreen_layer_noblelistst, nobles_search);

//
// END: Nobles search list
//

//
// START: Workshop profiles search list
//
typedef layered_search<df::viewscreen_layer_workshop_profilest, df::unit*, 0> profiles_search_base;
class profiles_search : public profiles_search_base
{
public:

    string get_element_description(df::unit *element) const
    {
        return get_unit_description(element);
    }

    void render() const
    {
        print_search_option(2, 23);
    }

    vector<df::unit *> *get_primary_list()
    {
        return &viewscreen->workers;
    }
};

IMPLEMENT_HOOKS(df::viewscreen_layer_workshop_profilest, profiles_search);

//
// END: Workshop profiles search list
//


//
// START: Job list search
//
void get_job_details(string &desc, df::job *job)
{
    string job_name = ENUM_KEY_STR(job_type,job->job_type);
    for (size_t i = 0; i < job_name.length(); i++)
    {
        char c = job_name[i];
        if (c >= 'A' && c <= 'Z')
            desc += " ";
        desc += c;
    }
    desc += ".";

    df::item_type itype = ENUM_ATTR(job_type, item, job->job_type);

    MaterialInfo mat(job);
    if (itype == item_type::FOOD)
        mat.decode(-1);

    if (mat.isValid() || job->material_category.whole)
    {
        desc += mat.toString();
        desc += ".";
        if (job->material_category.whole)
        {
            desc += bitfield_to_string(job->material_category);
            desc += ".";
        }
    }

    if (!job->reaction_name.empty())
    {
        for (size_t i = 0; i < job->reaction_name.length(); i++)
        {
            if (job->reaction_name[i] == '_')
                desc += " ";
            else
                desc += job->reaction_name[i];
        }

        desc += ".";
    }

    if (job->flags.bits.suspend)
        desc += "suspended.";
}

typedef search_twocolumn_modifiable<df::viewscreen_joblistst, df::job*, df::unit*> joblist_search_base;
class joblist_search : public joblist_search_base
{
public:
    void render() const
    {
        print_search_option(2);
    }

private:
    void do_post_init()
    {
        joblist_search_base::do_post_init();
        read_only = true;
    }

    string get_element_description(df::job *element) const
    {
        if (!element)
            return "no job.idle";

        string desc;
        desc.reserve(100);
        get_job_details(desc, element);
        df::unit *worker = DFHack::Job::getWorker(element);
        if (worker)
            desc += get_unit_description(worker);
        else
            desc += "Inactive";

        return desc;
    }

    char get_search_select_key()
    {
        return 'q';
    }

    vector<df::unit*> *get_secondary_list()
    {
        return &viewscreen->units;
    }

    int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->cursor_pos;
    }

    vector<df::job*> *get_primary_list()
    {
        return &viewscreen->jobs;
    }
};

IMPLEMENT_HOOKS(df::viewscreen_joblistst, joblist_search);

//
// END: Job list search
//


//
// START: Look menu search
//

typedef search_generic<df::viewscreen_dwarfmodest, df::ui_look_list::T_items*> look_menu_search_base;
class look_menu_search : public look_menu_search_base
{
    typedef df::ui_look_list::T_items::T_type elt_type;
public:
    bool can_init(df::viewscreen_dwarfmodest *screen)
    {
        if (ui->main.mode == df::ui_sidebar_mode::LookAround)
        {
            return look_menu_search_base::can_init(screen);
        }

        return false;
    }

    string get_element_description(df::ui_look_list::T_items *element) const
    {
        std::string desc = "";
        switch (element->type)
        {
        case elt_type::Item:
            if (element->item)
                desc = Items::getDescription(element->item, 0, true);
            break;
        case elt_type::Unit:
            if (element->unit)
                desc = get_unit_description(element->unit);
            break;
        case elt_type::Building:
            if (element->building)
                element->building->getName(&desc);
            break;
        default:
            break;
        }
        return desc;
    }

    bool force_in_search (size_t i)
    {
        df::ui_look_list::T_items *element = saved_list1[i];
        switch (element->type)
        {
        case elt_type::Item:
        case elt_type::Unit:
        case elt_type::Building:
            return false;
            break;
        default:
            return true;
            break;
        }
    }

    void render() const
    {
        auto dims = Gui::getDwarfmodeViewDims();
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        int y = 1;

        print_search_option(x, y);
    }

    vector<df::ui_look_list::T_items*> *get_primary_list()
    {
        return &ui_look_list->items;
    }

    virtual int32_t * get_viewscreen_cursor()
    {
        return ui_look_cursor;
    }


    bool should_check_input(set<df::interface_key> *input)
    {
        if (input->count(interface_key::SECONDSCROLL_UP)
            || input->count(interface_key::SECONDSCROLL_DOWN)
            || input->count(interface_key::SECONDSCROLL_PAGEUP)
            || input->count(interface_key::SECONDSCROLL_PAGEDOWN))
        {
            end_entry_mode();
            return false;
        }
        if (cursor_key_pressed(input))
        {
            end_entry_mode();
            clear_search();
            return false;
        }

        return true;
    }
};

IMPLEMENT_HOOKS(df::viewscreen_dwarfmodest, look_menu_search);

//
// END: Look menu search
//


//
// START: Burrow assignment search
//

typedef search_twocolumn_modifiable<df::viewscreen_dwarfmodest, df::unit*, bool> burrow_search_base;
class burrow_search : public burrow_search_base
{
public:
    bool can_init(df::viewscreen_dwarfmodest *screen)
    {
        if (ui->main.mode == df::ui_sidebar_mode::Burrows && ui->burrows.in_add_units_mode)
        {
            return burrow_search_base::can_init(screen);
        }

        return false;
    }

    string get_element_description(df::unit *element) const
    {
        return get_unit_description(element);
    }

    void render() const
    {
        auto dims = Gui::getDwarfmodeViewDims();
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        int y = 23;

        print_search_option(x, y);
    }

    vector<df::unit *> *get_primary_list()
    {
        return &ui->burrows.list_units;
    }

    vector<bool> *get_secondary_list()
    {
        return &ui->burrows.sel_units;
    }

    virtual int32_t * get_viewscreen_cursor()
    {
        return &ui->burrows.unit_cursor_pos;
    }


    bool should_check_input(set<df::interface_key> *input)
    {
        if  (input->count(interface_key::SECONDSCROLL_UP) || input->count(interface_key::SECONDSCROLL_DOWN)
            || input->count(interface_key::SECONDSCROLL_PAGEUP) || input->count(interface_key::SECONDSCROLL_PAGEDOWN))
        {
            end_entry_mode();
            return false;
        }

        return true;
    }
};

IMPLEMENT_HOOKS(df::viewscreen_dwarfmodest, burrow_search);

//
// END: Burrow assignment search
//


//
// START: Room assignment search
//

typedef search_generic<df::viewscreen_dwarfmodest, df::unit*> room_assign_search_base;
class room_assign_search : public room_assign_search_base
{
public:
    bool can_init(df::viewscreen_dwarfmodest *screen)
    {
        if (ui->main.mode == df::ui_sidebar_mode::QueryBuilding && *ui_building_in_assign)
        {
            return room_assign_search_base::can_init(screen);
        }

        return false;
    }

    string get_element_description(df::unit *element) const
    {
        return element ? get_unit_description(element) : "Nobody";
    }

    void render() const
    {
        auto dims = Gui::getDwarfmodeViewDims();
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        int y = 19;

        print_search_option(x, y);
    }

    vector<df::unit *> *get_primary_list()
    {
        return ui_building_assign_units;
    }

    virtual int32_t * get_viewscreen_cursor()
    {
        return ui_building_item_cursor;
    }

    bool should_check_input(set<df::interface_key> *input)
    {
        if  (input->count(interface_key::SECONDSCROLL_UP) || input->count(interface_key::SECONDSCROLL_DOWN)
            || input->count(interface_key::SECONDSCROLL_PAGEUP) || input->count(interface_key::SECONDSCROLL_PAGEDOWN))
        {
            end_entry_mode();
            return false;
        }

        return true;
    }
};

IMPLEMENT_HOOKS(df::viewscreen_dwarfmodest, room_assign_search);

//
// END: Room assignment search
//

//
// START: Noble suggestion search
//

typedef search_generic<df::viewscreen_topicmeeting_fill_land_holder_positionsst, int32_t> noble_suggest_search_base;
class noble_suggest_search : public noble_suggest_search_base
{
public:
    string get_element_description (int32_t hf_id) const
    {
        df::historical_figure *histfig = df::historical_figure::find(hf_id);
        if (!histfig)
            return "";
        df::unit *unit = df::unit::find(histfig->unit_id);
        if (!unit)
            return "";
        return get_unit_description(unit);
    }

    void render() const
    {
        print_search_option(2, gps->dimy - 4);
    }

    vector<int32_t> *get_primary_list()
    {
        return &viewscreen->candidate_histfig_ids;
    }

    virtual int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->cursor;
    }

};

IMPLEMENT_HOOKS(df::viewscreen_topicmeeting_fill_land_holder_positionsst, noble_suggest_search);

//
// END: Noble suggestion search
//

//
// START: Location occupation assignment search
//

typedef search_generic<df::viewscreen_locationsst, df::unit*> location_assign_occupation_search_base;
class location_assign_occupation_search : public location_assign_occupation_search_base
{
public:
    bool can_init (df::viewscreen_locationsst *screen)
    {
        return screen->menu == df::viewscreen_locationsst::AssignOccupation;
    }

    string get_element_description (df::unit *unit) const
    {
        return unit ? get_unit_description(unit) : "Nobody";
    }

    void render() const
    {
        print_search_option(37, gps->dimy - 3);
    }

    vector<df::unit*> *get_primary_list()
    {
        return &viewscreen->units;
    }

    virtual int32_t *get_viewscreen_cursor()
    {
        return &viewscreen->unit_idx;
    }

};

IMPLEMENT_HOOKS(df::viewscreen_locationsst, location_assign_occupation_search);

//
// END: Location occupation assignment search
//

#define SEARCH_HOOKS \
    HOOK_ACTION(unitlist_search_hook) \
    HOOK_ACTION(roomlist_search_hook) \
    HOOK_ACTION(trade_search_merc_hook) \
    HOOK_ACTION(trade_search_fort_hook) \
    HOOK_ACTION(stocks_search_hook) \
    HOOK_ACTION(pets_search_hook) \
    HOOK_ACTION(animal_knowledge_search_hook) \
    HOOK_ACTION(animal_trainer_search_hook) \
    HOOK_ACTION(military_search_hook) \
    HOOK_ACTION(nobles_search_hook) \
    HOOK_ACTION(profiles_search_hook) \
    HOOK_ACTION(announcement_search_hook) \
    HOOK_ACTION(joblist_search_hook) \
    HOOK_ACTION(look_menu_search_hook) \
    HOOK_ACTION(burrow_search_hook) \
    HOOK_ACTION(stockpile_search_hook) \
    HOOK_ACTION(room_assign_search_hook) \
    HOOK_ACTION(noble_suggest_search_hook) \
    HOOK_ACTION(location_assign_occupation_search_hook)

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable)
{
    if (!gps || !gview)
        return CR_FAILURE;

    if (is_enabled != enable)
    {
#define HOOK_ACTION(hook) \
    !INTERPOSE_HOOK(hook, feed).apply(enable) || \
    !INTERPOSE_HOOK(hook, render).apply(enable) || \
    !INTERPOSE_HOOK(hook, key_conflict).apply(enable) ||

        if (SEARCH_HOOKS 0)
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
#undef HOOK_ACTION

    const string a[] = {"Meager Quarters", "Modest Quarters", "Quarters", "Decent Quarters", "Fine Quarters", "Great Bedroom", "Grand Bedroom", "Royal Bedroom"};
    room_quality_names[df::building_type::Bed] = vector<string>(a, a + 8);

    const string b[] = {"Meager Dining Room", "Modest Dining Room", "Dining Room", "Decent Dining Room", "Fine Dining Room", "Great Dining Room", "Grand Dining Room", "Royal Dining Room"};
    room_quality_names[df::building_type::Table] = vector<string>(b, b + 8);

    const string c[] = {"Meager Office", "Modest Office", "Office", "Decent Office", "Splendid Office", "Throne Room", "Opulent Throne Room", "Royal Throne Room"};
    room_quality_names[df::building_type::Chair] = vector<string>(c, c + 8);

    const string d[] = {"Grave", "Servants Burial Chamber", "Burial Chamber", "Tomb", "Fine Tomb", "Mausoleum", "Grand Mausoleum", "Royal Mausoleum"};
    room_quality_names[df::building_type::Coffin] = vector<string>(d, d + 8);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
#define HOOK_ACTION(hook) \
    INTERPOSE_HOOK(hook, feed).remove(); \
    INTERPOSE_HOOK(hook, render).remove(); \
    INTERPOSE_HOOK(hook, key_conflict).remove();

    SEARCH_HOOKS

#undef HOOK_ACTION

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange ( color_ostream &out, state_change_event event )
{
#define HOOK_ACTION(hook) hook::module.reset_on_change();

    switch (event) {
    case SC_VIEWSCREEN_CHANGED:
        SEARCH_HOOKS
        break;

    default:
        break;
    }

    return CR_OK;

#undef HOOK_ACTION
}

#undef IMPLEMENT_HOOKS
#undef SEARCH_HOOKS
