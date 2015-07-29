#include "uicommon.h"
#include "listcolumn.h"

#include <functional>

// DF data structure definition headers
#include "DataDefs.h"
#include "Types.h"

#include "df/item.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_storesst.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/unit.h"
#include "df/world.h"
#include "df/item_quality.h"
#include "df/caravan_state.h"
#include "df/mandate.h"
#include "df/general_ref_building_holderst.h"

#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/World.h"
#include "modules/Screen.h"
#include "modules/Maps.h"
#include "modules/Units.h"
#include "df/building_cagest.h"
#include "df/ui_advmode.h"

DFHACK_PLUGIN("stocks");
#define PLUGIN_VERSION 0.12

REQUIRE_GLOBAL(world);

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

#define MAX_NAME 30
#define SIDEBAR_WIDTH 30

static bool show_debugging = false;

static void debug(const string &msg)
{
    if (!show_debugging)
        return;

    color_ostream_proxy out(Core::getInstance().getConsole());
    out << "DEBUG (stocks): " << msg << endl;
}


/*
 * Utility
 */

static string get_quality_name(const df::item_quality quality)
{
    if (gps->dimx - SIDEBAR_WIDTH < 60)
        return int_to_string(quality);
    else
        return ENUM_KEY_STR(item_quality, quality);
}

static df::item *get_container_of(df::item *item)
{
    auto container = Items::getContainer(item);
    return (container) ? container : item;
}

static df::item *get_container_of(df::unit *unit)
{
    auto ref = Units::getGeneralRef(unit, general_ref_type::CONTAINED_IN_ITEM);
    return (ref) ? ref->getItem() : nullptr;
}


/*
 * Trade Info
 */

static bool check_mandates(df::item *item)
{
    for (auto it = world->mandates.begin(); it != world->mandates.end(); it++)
    {
        auto mandate = *it;

        if (mandate->mode != 0)
            continue;

        if (item->getType() != mandate->item_type ||
            (mandate->item_subtype != -1 && item->getSubtype() != mandate->item_subtype))
            continue;

        if (mandate->mat_type != -1 && item->getMaterial() != mandate->mat_type)
            continue;

        if (mandate->mat_index != -1 && item->getMaterialIndex() != mandate->mat_index)
            continue;

        return false;
    }

    return true;
}

static bool can_trade_item(df::item *item)
{
    if (item->flags.bits.owned || item->flags.bits.artifact || item->flags.bits.spider_web || item->flags.bits.in_job)
        return false;

    for (size_t i = 0; i < item->general_refs.size(); i++)
    {
        df::general_ref *ref = item->general_refs[i];

        switch (ref->getType())
        {
        case general_ref_type::UNIT_HOLDER:
            return false;

        case general_ref_type::BUILDING_HOLDER:
            return false;

        default:
            break;
        }
    }

    for (size_t i = 0; i < item->specific_refs.size(); i++)
    {
        df::specific_ref *ref = item->specific_refs[i];

        if (ref->type == specific_ref_type::JOB)
        {
            // Ignore any items assigned to a job
            return false;
        }
    }

    return check_mandates(item);
}

static bool can_trade_item_and_container(df::item *item)
{
    item = get_container_of(item);

    if (item->flags.bits.in_inventory)
        return false;

    if (!can_trade_item(item))
        return false;

    vector<df::item*> contained_items;
    Items::getContainedItems(item, &contained_items);
    for (auto cit = contained_items.begin(); cit != contained_items.end(); cit++)
    {
        if (!can_trade_item(*cit))
            return false;
    }

    return true;
}


class TradeDepotInfo
{
public:
    TradeDepotInfo()
    {
        reset();
    }

    void prepareTradeVariables()
    {
        reset();
        for(auto bld_it = world->buildings.all.begin(); bld_it != world->buildings.all.end(); bld_it++)
        {
            auto bld = *bld_it;
            if (!isUsableDepot(bld))
                continue;

            depot = bld;
            id = depot->id;
            trade_possible = can_trade();
            break;
        }
    }

    bool assignItem(vector<df::item *> &entries)
    {
        for (auto it = entries.begin(); it != entries.end(); it++)
        {
            auto item = *it;
            item = get_container_of(item);
            if (!can_trade_item_and_container(item))
                return false;

            auto href = df::allocate<df::general_ref_building_holderst>();
            if (!href)
                return false;

            auto job = new df::job();

            df::coord tpos(depot->centerx, depot->centery, depot->z);
            job->pos = tpos;

            job->job_type = job_type::BringItemToDepot;

            // job <-> item link
            if (!Job::attachJobItem(job, item, df::job_item_ref::Hauled))
            {
                delete job;
                delete href;
                return false;
            }

            // job <-> building link
            href->building_id = id;
            depot->jobs.push_back(job);
            job->general_refs.push_back(href);

            // add to job list
            Job::linkIntoWorld(job);
        }

        return true;
    }

    void reset()
    {
        depot = 0;
        trade_possible = false;
    }

    bool canTrade()
    {
        return trade_possible;
    }

private:
    int32_t id;
    df::building *depot;
    bool trade_possible;

    bool isUsableDepot(df::building* bld)
    {
        if (bld->getType() != building_type::TradeDepot)
            return false;

        if (bld->getBuildStage() < bld->getMaxBuildStage())
            return false;

        if (bld->jobs.size() == 1 && bld->jobs[0]->job_type == job_type::DestroyBuilding)
            return false;

        return true;
    }
};

static TradeDepotInfo depot_info;


/*
 * Item manipulation
 */

static map<df::item *, bool> items_in_cages;

static df::job *get_item_job(df::item *item)
{
    auto ref = Items::getSpecificRef(item, specific_ref_type::JOB);
    if (ref && ref->job)
        return ref->job;

    return nullptr;
}

static bool is_marked_for_trade(df::item *item, df::item *container = nullptr)
{
    item = (container) ? container : get_container_of(item);
    auto job = get_item_job(item);
    if (!job)
        return false;

    return job->job_type == job_type::BringItemToDepot;
}

static bool is_in_inventory(df::item *item)
{
    item = get_container_of(item);
    return item->flags.bits.in_inventory;
}

static bool is_item_in_cage_cache(df::item *item)
{
    return items_in_cages.find(item) != items_in_cages.end();
}

static string get_keywords(df::item *item)
{
    string keywords;

    if (item->flags.bits.in_job)
        keywords += "job ";

    if (item->flags.bits.rotten)
        keywords += "rotten ";

    if (item->flags.bits.owned)
        keywords += "owned ";

    if (item->flags.bits.forbid)
        keywords += "forbid ";

    if (item->flags.bits.dump)
        keywords += "dump ";

    if (item->flags.bits.on_fire)
        keywords += "fire ";

    if (item->flags.bits.melt)
        keywords += "melt ";

    if (is_item_in_cage_cache(item))
        keywords += "caged ";

    if (is_in_inventory(item))
        keywords += "inventory ";

    if (depot_info.canTrade())
    {
        if (is_marked_for_trade(item))
            keywords += "trade ";
    }

    return keywords;
}

static string get_item_label(df::item *item, bool trim = false)
{
    auto label = Items::getDescription(item, 0, false);
    if (trim && item->getType() == item_type::BIN)
    {
        auto pos = label.find("<#");
        if (pos != string::npos)
        {
            label = label.substr(0, pos-1);
        }
    }

    auto wear = item->getWear();
    if (wear > 0)
    {
        string wearX;
        switch (wear)
        {
        case 1:
            wearX = "x";
            break;

        case 2:
            wearX = "X";
            break;

        case 3:
            wearX = "xX";
            break;

        default:
            wearX = "XX";
            break;

        }

        label = wearX + label + wearX;
    }

    label = pad_string(label, MAX_NAME, false, true);

    return label;
}

struct item_grouped_entry
{
    std::vector<df::item *> entries;

    string getLabel(bool grouped) const
    {
        if (entries.size() == 0)
            return "";

        return get_item_label(entries[0], grouped);
    }

    string getKeywords() const
    {
        return get_keywords(entries[0]);
    }

    df::item *getFirstItem() const
    {
        if (entries.size() == 0)
            return nullptr;

        return entries[0];
    }

    bool canMelt() const
    {
        df::item *item = getFirstItem();
        if (!item)
            return false;

        return can_melt(item);
    }

    bool isSetToMelt() const
    {
        df::item *item = getFirstItem();
        if (!item)
            return false;

        return is_set_to_melt(item);
    }

    bool contains(df::item *item) const
    {
        return std::find(entries.begin(), entries.end(), item) != entries.end();
    }

    void setFlags(const df::item_flags flags, const bool state)
    {
        for (auto it = entries.begin(); it != entries.end(); it++)
        {
            if (state)
                (*it)->flags.whole |= flags.whole;
            else
                (*it)->flags.whole &= ~flags.whole;
        }
    }

    bool isSingleItem()
    {
        return entries.size() == 1;
    }
};


struct extra_filters
{
    bool hide_trade_marked, hide_in_inventory, hide_in_cages;

    extra_filters()
    {
        reset();
    }

    void reset()
    {
        hide_in_inventory = false;
        hide_trade_marked = false;
    }
};

static bool cages_populated = false;
static vector<df::building_cagest *> cages;

static void find_cages()
{
    if (cages_populated)
        return;

    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];
        if (building->getType() == building_type::Cage)
        {
            cages.push_back(static_cast<df::building_cagest *>(building));
        }
    }

    cages_populated = true;
}

static df::building_cagest *is_in_cage(df::unit *unit)
{
    find_cages();
    for (auto it = cages.begin(); it != cages.end(); it++)
    {
        auto cage = *it;
        for (size_t c = 0; c < cage->assigned_units.size(); c++)
        {
            if(cage->assigned_units[c] == unit->id)
                return cage;
        }
    }

    return nullptr;
}


template <class T>
class StockListColumn : public ListColumn<T>
{
    virtual void display_extras(const T &item_group, int32_t &x, int32_t &y) const
    {
        auto item = item_group->getFirstItem();
        if (item->flags.bits.in_job)
            OutputString(COLOR_LIGHTBLUE, x, y, "J");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (item->flags.bits.rotten)
            OutputString(COLOR_CYAN, x, y, "X");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (item->flags.bits.owned)
            OutputString(COLOR_GREEN, x, y, "O");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (item->flags.bits.forbid)
            OutputString(COLOR_RED, x, y, "F");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (item->flags.bits.dump)
            OutputString(COLOR_LIGHTMAGENTA, x, y, "D");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (item->flags.bits.on_fire)
            OutputString(COLOR_LIGHTRED, x, y, "R");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (item->flags.bits.melt)
            OutputString(COLOR_BLUE, x, y, "M");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (is_in_inventory(item))
            OutputString(COLOR_WHITE, x, y, "I");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (is_item_in_cage_cache(item))
            OutputString(COLOR_LIGHTRED, x, y, "C");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (depot_info.canTrade())
        {
            if (is_marked_for_trade(item))
                OutputString(COLOR_LIGHTGREEN, x, y, "T");
            else
                OutputString(COLOR_LIGHTBLUE, x, y, " ");
        }

        if (item->isImproved())
            OutputString(COLOR_BLUE, x, y, "* ");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, "  ");

        auto quality = static_cast<df::item_quality>(item->getQuality());
        if (quality > item_quality::Ordinary)
        {
            auto color = COLOR_BROWN;
            switch(quality)
            {
            case item_quality::FinelyCrafted:
                color = COLOR_CYAN;
                break;

            case item_quality::Superior:
                color = COLOR_LIGHTBLUE;
                break;

            case item_quality::Exceptional:
                color = COLOR_GREEN;
                break;

            case item_quality::Masterful:
                color = COLOR_LIGHTGREEN;
                break;

            case item_quality::Artifact:
                color = COLOR_BLUE;
                break;

            default:
                break;
            }
            OutputString(color, x, y, get_quality_name(quality));
        }
    }

    virtual bool validSearchInput (unsigned char c)
    {
        switch (c)
        {
        case '(':
        case ')':
            return true;
            break;
        default:
            break;
        }
        string &search_string = ListColumn<T>::search_string;
        if (c == '^' && !search_string.size())
            return true;
        else if (c == '$' && search_string.size())
        {
            if (search_string == "^")
                return false;
            if (search_string[search_string.size() - 1] != '$')
                return true;
        }
        return ListColumn<T>::validSearchInput(c);
    }

    std::string getRawSearch(const std::string s)
    {
        string raw_search = s;
        if (raw_search.size() && raw_search[0] == '^')
            raw_search.erase(0, 1);
        if (raw_search.size() && raw_search[raw_search.size() - 1] == '$')
            raw_search.erase(raw_search.size() - 1, 1);
        return toLower(raw_search);
    }

    virtual void tokenizeSearch (vector<string> *dest, const string search)
    {
        string raw_search = getRawSearch(search);
        ListColumn<T>::tokenizeSearch(dest, raw_search);
    }

    virtual bool showEntry (const ListEntry<T> *entry, const vector<string> &search_tokens)
    {
        string &search_string = ListColumn<T>::search_string;
        if (!search_string.size())
            return true;

        bool match_start = false, match_end = false;
        string raw_search = getRawSearch(search_string);
        if (search_string.size() && search_string[0] == '^')
            match_start = true;
        if (search_string.size() && search_string[search_string.size() - 1] == '$')
            match_end = true;

        if (!ListColumn<T>::showEntry(entry, search_tokens))
            return false;

        string item_name = toLower(Items::getDescription(entry->elem->entries[0], 0, false));

        if ((match_start || match_end) && raw_search.size() > item_name.size())
            return false;
        if (match_start && item_name.compare(0, raw_search.size(), raw_search) != 0)
            return false;
        if (match_end && item_name.compare(item_name.size() - raw_search.size(), raw_search.size(), raw_search) != 0)
            return false;

        return true;
    }
};

class search_help : public dfhack_viewscreen
{
public:
    void feed (std::set<df::interface_key> *input)
    {
        if (input->count(interface_key::HELP))
            return;
        if (Screen::isDismissed(this))
            return;
        Screen::dismiss(this);
        if (!input->count(interface_key::LEAVESCREEN) && !input->count(interface_key::SELECT))
            parent->feed(input);
    }
    void render()
    {
        static std::string text =
            "\7 Flag names can be\n"
            "  searched for - e.g. job,\n"
            "  inventory, dump, forbid\n"
            "\n"
            "\7 Use ^ to match the start\n"
            "  of a name, and/or $ to\n"
            "  match the end of a name";
        if (Screen::isDismissed(this))
            return;
        parent->render();
        int left_margin = gps->dimx - SIDEBAR_WIDTH;
        int x = left_margin, y = 2;
        Screen::fillRect(Screen::Pen(' ', 0, 0), left_margin - 1, 1, gps->dimx - 2, gps->dimy - 4);
        Screen::fillRect(Screen::Pen(' ', 0, 0), left_margin - 1, 1, left_margin - 1, gps->dimy - 2);
        OutputString(COLOR_WHITE, x, y, "Search help", true, left_margin);
        ++y;
        vector<string> lines;
        split_string(&lines, text, "\n");
        for (auto line = lines.begin(); line != lines.end(); ++line)
            OutputString(COLOR_WHITE, x, y, line->c_str(), true, left_margin);
    }
    std::string getFocusString() { return "stocks_view/search_help"; }
};

class ViewscreenStocks : public dfhack_viewscreen
{
public:
    static df::item_flags hide_flags;
    static extra_filters extra_hide_flags;

    ViewscreenStocks(df::building_stockpilest *sp = NULL) : sp(sp)
    {
        is_grouped = true;
        selected_column = 0;
        items_column.multiselect = false;
        items_column.auto_select = true;
        items_column.allow_search = true;
        items_column.left_margin = 2;
        items_column.bottom_margin = 1;
        items_column.search_margin = gps->dimx - SIDEBAR_WIDTH;

        items_column.changeHighlight(0);

        apply_to_all = false;
        hide_unflagged = false;

        checked_flags.bits.in_job = true;
        checked_flags.bits.rotten = true;
        checked_flags.bits.owned = true;
        checked_flags.bits.forbid = true;
        checked_flags.bits.dump = true;
        checked_flags.bits.on_fire = true;
        checked_flags.bits.melt = true;
        checked_flags.bits.on_fire = true;

        min_quality = item_quality::Ordinary;
        max_quality = item_quality::Artifact;
        min_wear = 0;
        cages.clear();
        items_in_cages.clear();
        cages_populated = false;

        last_selected_item = nullptr;

        populateItems();

        items_column.selectDefaultEntry();
    }

    static void reset()
    {
        hide_flags.whole = 0;
        extra_hide_flags.reset();
        depot_info.reset();
    }

    void feed(set<df::interface_key> *input)
    {
        if (input->count(interface_key::LEAVESCREEN))
        {
            input->clear();
            Screen::dismiss(this);
            return;
        }
        else if (input->count(interface_key::HELP))
        {
            Screen::show(new search_help);
        }

        bool key_processed = false;
        switch (selected_column)
        {
        case 0:
            key_processed = items_column.feed(input);
            break;
        }

        if (key_processed)
            return;

        if (input->count(interface_key::CUSTOM_CTRL_J))
        {
            hide_flags.bits.in_job = !hide_flags.bits.in_job;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_X))
        {
            hide_flags.bits.rotten = !hide_flags.bits.rotten;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_O))
        {
            hide_flags.bits.owned = !hide_flags.bits.owned;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_F))
        {
            hide_flags.bits.forbid = !hide_flags.bits.forbid;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_D))
        {
            hide_flags.bits.dump = !hide_flags.bits.dump;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_E))
        {
            hide_flags.bits.on_fire = !hide_flags.bits.on_fire;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_M))
        {
            hide_flags.bits.melt = !hide_flags.bits.melt;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_I))
        {
            extra_hide_flags.hide_in_inventory = !extra_hide_flags.hide_in_inventory;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_C))
        {
            extra_hide_flags.hide_in_cages = !extra_hide_flags.hide_in_cages;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_T))
        {
            extra_hide_flags.hide_trade_marked = !extra_hide_flags.hide_trade_marked;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_CTRL_N))
        {
            hide_unflagged = !hide_unflagged;
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_C))
        {
            setAllFlags(true);
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_E))
        {
            setAllFlags(false);
            populateItems();
        }
        else if (input->count(interface_key::CHANGETAB))
        {
            is_grouped = !is_grouped;
            populateItems();
            items_column.centerSelection();
        }
        else if (input->count(interface_key::SECONDSCROLL_UP))
        {
            if (min_quality > item_quality::Ordinary)
            {
                min_quality = static_cast<df::item_quality>(static_cast<int16_t>(min_quality) - 1);
                populateItems();
            }
        }
        else if (input->count(interface_key::SECONDSCROLL_DOWN))
        {
            if (min_quality < max_quality && min_quality < item_quality::Artifact)
            {
                min_quality = static_cast<df::item_quality>(static_cast<int16_t>(min_quality) + 1);
                populateItems();
            }
        }
        else if (input->count(interface_key::SECONDSCROLL_PAGEUP))
        {
            if (max_quality > min_quality && max_quality > item_quality::Ordinary)
            {
                max_quality = static_cast<df::item_quality>(static_cast<int16_t>(max_quality) - 1);
                populateItems();
            }
        }
        else if (input->count(interface_key::SECONDSCROLL_PAGEDOWN))
        {
            if (max_quality < item_quality::Artifact)
            {
                max_quality = static_cast<df::item_quality>(static_cast<int16_t>(max_quality) + 1);
                populateItems();
            }
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_W))
        {
            ++min_wear;
            if (min_wear > 3)
                min_wear = 0;

            populateItems();
        }

        else if (input->count(interface_key::CUSTOM_SHIFT_Z))
        {
            input->clear();
            auto item_group = items_column.getFirstSelectedElem();
            if (!item_group)
                return;

            if (is_grouped && !item_group->isSingleItem())
                return;

            auto item = item_group->getFirstItem();
            auto pos = getRealPos(item);
            if (!isRealPos(pos))
                return;

            Screen::dismiss(this);
            auto vs = Gui::getCurViewscreen(true);
            while (vs && !virtual_cast<df::viewscreen_dwarfmodest>(vs))
            {
                Screen::dismiss(vs);
                vs = vs->parent;
            }
            // Could be clever here, if item is in a container, to look inside the container.
            // But that's different for built containers vs bags/pots in stockpiles.
            send_key(interface_key::D_LOOK);
            move_cursor(pos);
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_A))
        {
            apply_to_all = !apply_to_all;
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_D))
        {
            df::item_flags flags;
            flags.bits.dump = true;
            toggleFlag(flags);
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_F))
        {
            df::item_flags flags;
            flags.bits.forbid = true;
            toggleFlag(flags);
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_M))
        {
            toggleMelt();
            populateItems();
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_T))
        {
            if (depot_info.canTrade())
            {
                auto selected = getSelectedItems();
                for (auto it = selected.begin(); it != selected.end(); it++)
                {
                    depot_info.assignItem((*it)->entries);
                }
            }
        }

        else if (input->count(interface_key::CURSOR_LEFT))
        {
            --selected_column;
            validateColumn();
        }
        else if (input->count(interface_key::CURSOR_RIGHT))
        {
            selected_column++;
            validateColumn();
        }
        else if (enabler->tracking_on && enabler->mouse_lbut)
        {
            if (items_column.setHighlightByMouse())
                selected_column = 0;

            enabler->mouse_lbut = enabler->mouse_rbut = 0;
        }
    }

    void move_cursor(const df::coord &pos)
    {
        Gui::setCursorCoords(pos.x, pos.y, pos.z);
        send_key(interface_key::CURSOR_DOWN_Z);
        send_key(interface_key::CURSOR_UP_Z);
    }

    void send_key(const df::interface_key &key)
    {
        set< df::interface_key > keys;
        keys.insert(key);
        Gui::getCurViewscreen(true)->feed(&keys);
    }

    void render()
    {
        if (Screen::isDismissed(this))
            return;

        dfhack_viewscreen::render();

        Screen::clear();
        Screen::drawBorder("  Stocks  ");

        items_column.display(selected_column == 0);

        int32_t y = 1;
        auto left_margin = gps->dimx - SIDEBAR_WIDTH;
        int32_t x = left_margin - 2;
        Screen::Pen border('\xDB', 8);
        for (; y < gps->dimy - 1; y++)
        {
            paintTile(border, x, y);
        }

        y = 2;
        x = left_margin;
        OutputString(COLOR_BROWN, x, y, "Filters ", false, left_margin);
        OutputString(COLOR_LIGHTRED, x, y, "(Ctrl+Key toggles)", true, left_margin);
        OutputFilterString(x, y, "In Job  ", "J", !hide_flags.bits.in_job, false, left_margin, COLOR_LIGHTBLUE);
        OutputFilterString(x, y, "Rotten", "X", !hide_flags.bits.rotten, true, left_margin, COLOR_CYAN);
        OutputFilterString(x, y, "Owned   ", "O", !hide_flags.bits.owned, false, left_margin, COLOR_GREEN);
        OutputFilterString(x, y, "Forbidden", "F", !hide_flags.bits.forbid, true, left_margin, COLOR_RED);
        OutputFilterString(x, y, "Dump    ", "D", !hide_flags.bits.dump, false, left_margin, COLOR_LIGHTMAGENTA);
        OutputFilterString(x, y, "On Fire", "E", !hide_flags.bits.on_fire, true, left_margin, COLOR_LIGHTRED);
        OutputFilterString(x, y, "Melt    ", "M", !hide_flags.bits.melt, false, left_margin, COLOR_BLUE);
        OutputFilterString(x, y, "In Inventory", "I", !extra_hide_flags.hide_in_inventory, true, left_margin, COLOR_WHITE);
        OutputFilterString(x, y, "Caged   ", "C", !extra_hide_flags.hide_in_cages, false, left_margin, COLOR_LIGHTRED);
        OutputFilterString(x, y, "Trade", "T", !extra_hide_flags.hide_trade_marked, true, left_margin, COLOR_LIGHTGREEN);
        OutputFilterString(x, y, "No Flags", "N", !hide_unflagged, true, left_margin, COLOR_GREY);
        if (gps->dimy > 26)
            ++y;
        OutputHotkeyString(x, y, "Clear All", "Shift-C", true, left_margin);
        OutputHotkeyString(x, y, "Enable All", "Shift-E", true, left_margin);
        OutputHotkeyString(x, y, "Toggle Grouping", interface_key::CHANGETAB, true, left_margin);
        ++y;

        OutputHotkeyString(x, y, "Min Qual: ", "-+");
        OutputString(COLOR_BROWN, x, y, get_quality_name(min_quality), true, left_margin);
        OutputHotkeyString(x, y, "Max Qual: ", "/*");
        OutputString(COLOR_BROWN, x, y, get_quality_name(max_quality), true, left_margin);
        OutputHotkeyString(x, y, "Min Wear: ", "Shift-W");
        OutputString(COLOR_BROWN, x, y, int_to_string(min_wear), true, left_margin);

        if (gps->dimy > 27)
            ++y;
        OutputString(COLOR_BROWN, x, y, "Actions (");
        OutputString(COLOR_LIGHTGREEN, x, y, int_to_string(items_column.getDisplayedListSize()));
        OutputString(COLOR_BROWN, x, y, " Items)", true, left_margin);
        OutputHotkeyString(x, y, "Zoom    ", "Shift-Z", false, left_margin);
        OutputHotkeyString(x, y, "Dump", "-D", true, left_margin);
        OutputHotkeyString(x, y, "Forbid  ", "Shift-F", false, left_margin);
        OutputHotkeyString(x, y, "Melt", "-M", true, left_margin);
        OutputHotkeyString(x, y, "Mark for Trade", "Shift-T", true, left_margin,
            depot_info.canTrade() ? COLOR_WHITE : COLOR_DARKGREY);
        OutputHotkeyString(x, y, "Apply to: ", "Shift-A");
        OutputString(COLOR_BROWN, x, y, (apply_to_all) ? "Listed" : "Selected", true, left_margin);

        y = gps->dimy - 4;
        OutputHotkeyString(x, y, "Search help", interface_key::HELP, true, left_margin);
    }

    std::string getFocusString() { return "stocks_view"; }

private:
    StockListColumn<item_grouped_entry *> items_column;
    int selected_column;
    bool apply_to_all, hide_unflagged;
    df::item_flags checked_flags;
    df::item_quality min_quality, max_quality;
    int16_t min_wear;
    bool is_grouped;
    std::list<item_grouped_entry> grouped_items_store;
    df::item *last_selected_item;
    string last_selected_hash;
    int last_display_offset;
    df::building_stockpilest *sp;

    static bool isRealPos(const df::coord pos)
    {
        return pos.x != -30000;
    }

    static df::coord getRealPos(df::item *item)
    {
        df::coord pos;
        pos.x = -30000;
        item = get_container_of(item);
        if (item->flags.bits.in_inventory)
        {
            if (item->flags.bits.in_job)
            {
                auto ref = Items::getSpecificRef(item, specific_ref_type::JOB);
                if (ref && ref->job)
                {
                    if (ref->job->job_type == job_type::Eat || ref->job->job_type == job_type::Drink)
                        return pos;

                    auto unit = Job::getWorker(ref->job);
                    if (unit)
                        return unit->pos;
                }
                return pos;
            }
            else
            {
                auto unit = Items::getHolderUnit(item);
                if (unit)
                {
                    if (!Units::isCitizen(unit))
                    {
                        auto cage_item = get_container_of(unit);
                        if (cage_item)
                        {
                            items_in_cages[item] = true;
                            return cage_item->pos;
                        }

                        auto cage_building = is_in_cage(unit);
                        if (cage_building)
                        {
                            items_in_cages[item] = true;
                            pos.x = cage_building->centerx;
                            pos.y = cage_building->centery;
                            pos.z = cage_building->z;
                        }

                        return pos;
                    }

                    return unit->pos;
                }

                return pos;
            }
        }

        return item->pos;
    }

    void toggleMelt()
    {
        int set_to_melt = -1;
        auto selected = getSelectedItems();
        vector<df::item *> items;
        for (auto it = selected.begin(); it != selected.end(); it++)
        {
            auto item_group = *it;

            if (set_to_melt == -1)
                set_to_melt = (item_group->isSetToMelt()) ? 0 : 1;

            if (set_to_melt)
            {
                if (!item_group->canMelt() || item_group->isSetToMelt())
                    continue;
            }
            else if (!item_group->isSetToMelt())
            {
                continue;
            }

            items.insert(items.end(), item_group->entries.begin(), item_group->entries.end());
        }

        auto &melting_items = world->items.other[items_other_id::ANY_MELT_DESIGNATED];
        for (auto it = items.begin(); it != items.end(); it++)
        {
            auto item = *it;
            if (set_to_melt)
            {
                insert_into_vector(melting_items, &df::item::id, item);
                item->flags.bits.melt = true;
            }
            else
            {
                for (auto mit = melting_items.begin(); mit != melting_items.end(); mit++)
                {
                    if (item != *mit)
                        continue;

                    melting_items.erase(mit);
                    item->flags.bits.melt = false;
                    break;
                }
            }
        }
    }

    void toggleFlag(const df::item_flags flags)
    {
        int state_to_apply = -1;
        auto selected = getSelectedItems();
        for (auto it = selected.begin(); it != selected.end(); it++)
        {
            auto grouped_entry = (*it);
            auto item = grouped_entry->getFirstItem();
            if (state_to_apply == -1)
                state_to_apply = (item->flags.whole & flags.whole) ? 0 : 1;

            grouped_entry->setFlags(flags.whole, state_to_apply);
        }
    }

    vector<item_grouped_entry *> getSelectedItems()
    {
        vector<item_grouped_entry *> result;
        if (apply_to_all)
        {
            for (auto it = items_column.getDisplayList().begin(); it != items_column.getDisplayList().end(); it++)
            {
                auto item_group = (*it)->elem;
                if (!item_group)
                    continue;
                result.push_back(item_group);
            }
        }
        else
        {
            auto item_group = items_column.getFirstSelectedElem();
            if (item_group)
                result.push_back(item_group);
        }

        return result;
    }

    void setAllFlags(bool state)
    {
        hide_flags.bits.in_job = state;
        hide_flags.bits.rotten = state;
        hide_flags.bits.owned = state;
        hide_flags.bits.forbid = state;
        hide_flags.bits.dump = state;
        hide_flags.bits.on_fire = state;
        hide_flags.bits.melt = state;
        hide_flags.bits.on_fire = state;
        hide_unflagged = state;
        extra_hide_flags.hide_trade_marked = state;
        extra_hide_flags.hide_in_inventory = state;
        extra_hide_flags.hide_in_cages = state;
    }

    void populateItems()
    {
        items_column.setTitle((is_grouped) ? "Item (count)" : "Item");
        preserveLastSelected();
        items_column.clear();

        df::item_flags bad_flags;
        bad_flags.whole = 0;
        bad_flags.bits.hostile = true;
        bad_flags.bits.trader = true;
        bad_flags.bits.in_building = true;
        bad_flags.bits.garbage_collect = true;
        bad_flags.bits.removed = true;
        bad_flags.bits.dead_dwarf = true;
        bad_flags.bits.murder = true;
        bad_flags.bits.construction = true;

        depot_info.prepareTradeVariables();

        std::vector<df::item *> &items = world->items.other[items_other_id::IN_PLAY];
        std::map<string, item_grouped_entry *> grouped_items;
        grouped_items_store.clear();
        item_grouped_entry *next_selected_group = nullptr;
        StockpileInfo spInfo;
        if (sp)
            spInfo = StockpileInfo(sp);

        for (size_t i = 0; i < items.size(); i++)
        {
            df::item *item = items[i];

            if (item->flags.whole & bad_flags.whole || item->flags.whole & hide_flags.whole)
                continue;

            auto container = get_container_of(item);
            if (container->flags.whole & bad_flags.whole)
                continue;

            auto pos = getRealPos(item);
            if (!isRealPos(pos))
                continue;

            auto designation = Maps::getTileDesignation(pos);
            if (!designation)
                continue;

            if (designation->bits.hidden)
                continue; // Items in parts of the map not yet revealed

            bool trade_marked = is_marked_for_trade(item, container);
            if (extra_hide_flags.hide_trade_marked && trade_marked)
                continue;

            bool caged = is_item_in_cage_cache(item);
            if (extra_hide_flags.hide_in_cages && caged)
                continue;

            if (extra_hide_flags.hide_in_inventory && container->flags.bits.in_inventory)
                continue;

            if (hide_unflagged && (!(item->flags.whole & checked_flags.whole) &&
                !trade_marked && !caged && !container->flags.bits.in_inventory))
            {
                continue;
            }

            auto quality = static_cast<df::item_quality>(item->getQuality());
            if (quality < min_quality || quality > max_quality)
                continue;

            auto wear = item->getWear();
            if (wear < min_wear)
                continue;

            if (spInfo.isValid() && !spInfo.inStockpile(item))
                continue;

            if (is_grouped)
            {
                auto hash = getItemHash(item);
                if (grouped_items.find(hash) == grouped_items.end())
                {
                    grouped_items_store.push_back(item_grouped_entry());
                    grouped_items[hash] = &grouped_items_store.back();
                }
                grouped_items[hash]->entries.push_back(item);
                if (last_selected_item &&
                    !next_selected_group &&
                    hash == last_selected_hash)
                {
                    next_selected_group = grouped_items[hash];
                }
            }
            else
            {
                grouped_items_store.push_back(item_grouped_entry());
                auto item_group = &grouped_items_store.back();
                item_group->entries.push_back(item);

                auto label = get_item_label(item);
                auto entry = ListEntry<item_grouped_entry *>(label, item_group, item_group->getKeywords());
                items_column.add(entry);

                if (last_selected_item &&
                    !next_selected_group &&
                    item == last_selected_item)
                {
                    next_selected_group = item_group;
                }
            }
        }

        if (is_grouped)
        {
            for (auto groups_iter = grouped_items.begin(); groups_iter != grouped_items.end(); groups_iter++)
            {
                auto item_group = groups_iter->second;
                stringstream label;
                label << item_group->getLabel(is_grouped);
                if (!item_group->isSingleItem())
                    label << " (" << item_group->entries.size() << ")";
                auto entry = ListEntry<item_grouped_entry *>(label.str(), item_group, item_group->getKeywords());
                items_column.add(entry);
            }
        }

        items_column.fixWidth();
        items_column.filterDisplay();

        if (next_selected_group)
        {
            items_column.selectItem(next_selected_group);
            items_column.display_start_offset = last_display_offset;
        }
    }

    string getItemHash(df::item *item)
    {
        auto label = get_item_label(item, true);
        auto quality = static_cast<df::item_quality>(item->getQuality());
        auto quality_enum = static_cast<df::item_quality>(quality);
        auto quality_string = ENUM_KEY_STR(item_quality, quality_enum);
        auto hash = label + quality_string + int_to_string(item->flags.whole & checked_flags.whole) + " " +
            int_to_string(item->hasImprovements());

        return hash;
    }

    void preserveLastSelected()
    {
        last_selected_item = nullptr;
        auto selected_entry = items_column.getFirstSelectedElem();
        if (!selected_entry)
            return;
        last_selected_item = selected_entry->getFirstItem();
        last_selected_hash = (is_grouped && last_selected_item) ? getItemHash(last_selected_item) : "";
        last_display_offset = items_column.display_start_offset;
    }

    void validateColumn()
    {
        set_to_limit(selected_column, 0);
    }

    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        items_column.resize();
        items_column.search_margin = gps->dimx - SIDEBAR_WIDTH;
    }
};

df::item_flags ViewscreenStocks::hide_flags;
extra_filters ViewscreenStocks::extra_hide_flags;

struct stocks_hook : public df::viewscreen_storesst
{
    typedef df::viewscreen_storesst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (input->count(interface_key::CUSTOM_E))
        {
            Screen::dismiss(this);
            Screen::show(new ViewscreenStocks());
            return;
        }
        INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        auto dim = Screen::getWindowSize();
        int x = 40;
        int y = dim.y - 2;
        OutputHotkeyString(x, y, "Enhanced View", "e", false, 0, COLOR_WHITE, COLOR_LIGHTRED);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(stocks_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(stocks_hook, render);


struct stocks_stockpile_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool handleInput(set<df::interface_key> *input)
    {
        df::building_stockpilest *sp = get_selected_stockpile();
        if (!sp)
            return false;

        if (input->count(interface_key::CUSTOM_I))
        {
            Screen::show(new ViewscreenStocks(sp));
            return true;
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!handleInput(input))
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        df::building_stockpilest *sp = get_selected_stockpile();
        if (!sp)
            return;

        auto dims = Gui::getDwarfmodeViewDims();
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        int y = dims.y2 - 4;

        int links = 0;
        links += sp->links.give_to_pile.size();
        links += sp->links.take_from_pile.size();
        links += sp->links.give_to_workshop.size();
        links += sp->links.take_from_workshop.size();
        if (links + 12 >= y)
           y = 3;

        OutputHotkeyString(x, y, "Show Inventory", "i", true, left_margin, COLOR_WHITE, COLOR_LIGHTRED);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(stocks_stockpile_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(stocks_stockpile_hook, render);


DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(stocks_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(stocks_hook, render).apply(enable) ||
            !INTERPOSE_HOOK(stocks_stockpile_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(stocks_stockpile_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

static command_result stocks_cmd(color_ostream &out, vector <string> & parameters)
{
    if (!parameters.empty())
    {
        if (toLower(parameters[0])[0] == 'v')
        {
            out << "Stocks plugin" << endl << "Version: " << PLUGIN_VERSION << endl;
            return CR_OK;
        }
        else if (toLower(parameters[0])[0] == 's')
        {
            Screen::show(new ViewscreenStocks());
            return CR_OK;
        }
    }

    return CR_WRONG_USAGE;
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(
        PluginCommand(
        "stocks", "An improved stocks display screen",
        stocks_cmd, false, "Run 'stocks show' open the stocks display screen, or 'stocks version' to query the plugin version."));

    ViewscreenStocks::reset();

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        ViewscreenStocks::reset();
        break;
    case SC_BEGIN_UNLOAD:
        if (Gui::getCurFocus().find("dfhack/stocks") == 0)
            return CR_FAILURE;
        break;
    default:
        break;
    }

    return CR_OK;
}
