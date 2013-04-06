#include "uicommon.h"

#include <functional>

// DF data structure definition headers
#include "DataDefs.h"
#include "Types.h"
#include "df/item.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/world.h"

#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/World.h"

using df::global::world;

DFHACK_PLUGIN("stocks");
#define PLUGIN_VERSION 0.1

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

#define SIDEBAR_WIDTH 30

static bool show_debugging = false;

static void debug(const string &msg)
{
    if (!show_debugging)
        return;

    color_ostream_proxy out(Core::getInstance().getConsole());
    out << "DEBUG (stocks): " << msg << endl;
}


class ViewscreenStocks : public dfhack_viewscreen
{
public:
    ViewscreenStocks()
    {
        selected_column = 0;
        items_column.setTitle("Item");
        items_column.multiselect = false;
        items_column.allow_search = true;
        items_column.left_margin = 2;

        items_column.changeHighlight(0);

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

        if  (input->count(interface_key::CURSOR_LEFT))
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

    void render()
    {
        if (Screen::isDismissed(this))
            return;

        dfhack_viewscreen::render();

        Screen::clear();
        Screen::drawBorder("  Stocks  ");

        items_column.display(selected_column == 0);

        int32_t y = gps->dimy - 3;
        int32_t x = 2;
    }

    std::string getFocusString() { return "stocks_view"; }

private:
    ListColumn<df::item *> items_column;
    int selected_column;

    void populateItems()
    {
        items_column.clear();

        df::item_flags bad_flags;
        bad_flags.whole = 0;
        bad_flags.bits.hostile = true;
        bad_flags.bits.trader = true;
        bad_flags.bits.in_building = true;

        std::vector<df::item*> &items = world->items.other[items_other_id::IN_PLAY];

        for (size_t i = 0; i < items.size(); i++)
        {
            df::item *item = items[i];

            if (item->flags.whole & bad_flags.whole)
                continue;

            df::item_type itype = item->getType();

            items_column.add(Items::getDescription(item, 0, true), item);
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
