#include "uicommon.h"

#include <functional>

// DF data structure definition headers
#include "DataDefs.h"
#include "Types.h"
#include "df/item.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/unit.h"
#include "df/world.h"

#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/World.h"
#include "modules/Screen.h"

using df::global::world;

DFHACK_PLUGIN("stocks");
#define PLUGIN_VERSION 0.1

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


/*struct FlagDisplay
{

};*/

template <class T>
class StockListColumn : public ListColumn<T>
{
    virtual void display_extras(const T &item, int32_t &x, int32_t &y) const
    {
        if (item->flags.bits.in_job)
            OutputString(COLOR_LIGHTBLUE, x, y, "J");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (item->flags.bits.rotten)
            OutputString(COLOR_CYAN, x, y, "N");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");

        if (item->flags.bits.foreign)
            OutputString(COLOR_BROWN, x, y, "G");
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

        if (item->flags.bits.in_inventory)
            OutputString(COLOR_GREY, x, y, "I");
        else
            OutputString(COLOR_LIGHTBLUE, x, y, " ");
    }
};


class ViewscreenStocks : public dfhack_viewscreen
{
public:
    ViewscreenStocks()
    {
        selected_column = 0;
        items_column.setTitle("Item");
        items_column.multiselect = false;
        items_column.auto_select = true;
        items_column.allow_search = true;
        items_column.left_margin = 2;
        items_column.bottom_margin = 1;
        items_column.search_margin = gps->dimx - SIDEBAR_WIDTH;

        items_column.changeHighlight(0);

        hide_flags.whole = 0;

        populateItems();

        items_column.selectDefaultEntry();
    }

    void feed(set<df::interface_key> *input)
    {
        bool key_processed = false;
        switch (selected_column)
        {
        case 0:
            key_processed = items_column.feed(input);
            break;
        }

        if (key_processed)
            return;

        if (input->count(interface_key::LEAVESCREEN))
        {
            input->clear();
            Screen::dismiss(this);
            return;
        }

        if  (input->count(interface_key::CUSTOM_SHIFT_G))
        {
            hide_flags.bits.foreign = !hide_flags.bits.foreign;
            populateItems();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_J))
        {
            hide_flags.bits.in_job = !hide_flags.bits.in_job;
            populateItems();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_N))
        {
            hide_flags.bits.rotten = !hide_flags.bits.rotten;
            populateItems();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_O))
        {
            hide_flags.bits.owned = !hide_flags.bits.owned;
            populateItems();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_F))
        {
            hide_flags.bits.forbid = !hide_flags.bits.forbid;
            populateItems();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_D))
        {
            hide_flags.bits.dump = !hide_flags.bits.dump;
            populateItems();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_R))
        {
            hide_flags.bits.on_fire = !hide_flags.bits.on_fire;
            populateItems();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_M))
        {
            hide_flags.bits.melt = !hide_flags.bits.melt;
            populateItems();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_I))
        {
            hide_flags.bits.in_inventory = !hide_flags.bits.in_inventory;
            populateItems();
        }

        else if  (input->count(interface_key::CUSTOM_SHIFT_Z))
        {
            input->clear();
            auto item = items_column.getFirstSelectedElem();
            if (!item)
                return;
            auto pos = getRealPos(item);
            if (!pos)
                return;

            Screen::dismiss(this);
            send_key(interface_key::D_LOOK);
            move_cursor(*pos);
        }

        else if  (input->count(interface_key::CURSOR_LEFT))
        {
            --selected_column;
            validateColumn();
        }
        else if  (input->count(interface_key::CURSOR_RIGHT))
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

    void move_cursor(df::coord &pos)
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
        OutputString(COLOR_BROWN, x, y, "Filters", true, left_margin);
        OutputString(COLOR_LIGHTRED, x, y, "Press Shift-Hotkey to Toggle", true, left_margin);
        OutputFilterString(x, y, "In Job", "J", !hide_flags.bits.in_job, true, left_margin, COLOR_LIGHTBLUE);
        OutputFilterString(x, y, "Rotten", "N", !hide_flags.bits.rotten, true, left_margin, COLOR_CYAN);
        OutputFilterString(x, y, "Foreign Made", "G", !hide_flags.bits.foreign, true, left_margin, COLOR_BROWN);
        OutputFilterString(x, y, "Owned", "O", !hide_flags.bits.owned, true, left_margin, COLOR_GREEN);
        OutputFilterString(x, y, "Forbidden", "F", !hide_flags.bits.forbid, true, left_margin, COLOR_RED);
        OutputFilterString(x, y, "Dump", "D", !hide_flags.bits.dump, true, left_margin, COLOR_LIGHTMAGENTA);
        OutputFilterString(x, y, "On Fire", "R", !hide_flags.bits.on_fire, true, left_margin, COLOR_LIGHTRED);
        OutputFilterString(x, y, "Melt", "M", !hide_flags.bits.melt, true, left_margin, COLOR_BLUE);
        OutputFilterString(x, y, "In Inventory", "I", !hide_flags.bits.in_inventory, true, left_margin, COLOR_GREY);

        ++y;
        OutputString(COLOR_BROWN, x, y, "Actions (" + int_to_string(items_column.getDisplayedListSize()) + " Items)", 
            true, left_margin);
        OutputHotkeyString(x, y, "Zoom", "Shift-Z", true, left_margin);

    }

    std::string getFocusString() { return "stocks_view"; }

private:
    StockListColumn<df::item *> items_column;
    int selected_column;
    df::item_flags hide_flags;

    df::coord *getRealPos(df::item *item)
    {
        if (item->flags.bits.in_inventory)
        {
            if (item->flags.bits.in_job)
            {
                auto ref = Items::getSpecificRef(item, specific_ref_type::JOB);
                if (ref && ref->job)
                {
                    if (ref->job->job_type == job_type::Eat || ref->job->job_type == job_type::Drink)
                        return nullptr;

                    auto unit = Job::getWorker(ref->job);
                    if (unit)
                        return &unit->pos;
                }
                return nullptr;
            }
            else
            {
                auto unit = Items::getHolderUnit(item);
                if (unit)
                    return &unit->pos;

                return nullptr;
            }
        }

        return &item->pos;
    }

    void populateItems()
    {
        items_column.clear();

        df::item_flags bad_flags;
        bad_flags.whole = 0;
        bad_flags.bits.hostile = true;
        bad_flags.bits.trader = true;
        bad_flags.bits.in_building = true;
        bad_flags.bits.garbage_collect = true;
        bad_flags.bits.spider_web = true;
        bad_flags.bits.hostile = true;
        bad_flags.bits.removed = true;
        bad_flags.bits.dead_dwarf = true;
        bad_flags.bits.murder = true;
        bad_flags.bits.construction = true;

        std::vector<df::item*> &items = world->items.other[items_other_id::IN_PLAY];

        for (size_t i = 0; i < items.size(); i++)
        {
            df::item *item = items[i];

            if (item->flags.whole & bad_flags.whole || item->flags.whole & hide_flags.whole)
                continue;

            if (item->pos.x == -30000)
                continue;

            if (!getRealPos(item))
                continue;

            auto label = pad_string(Items::getDescription(item, 0, true), MAX_NAME, false, true);

            items_column.add(label, item);
        }

        items_column.filterDisplay();
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


DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!gps)
        out.printerr("Could not insert stocks plugin hooks!\n");

    commands.push_back(
        PluginCommand(
        "stocks", "An improved stocks display screen",
        stocks_cmd, false, ""));

    return CR_OK;
}


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        break;
    default:
        break;
    }

    return CR_OK;
}
