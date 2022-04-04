/*
 * Save or restore cursor position on change to/from main dwarfmode menu.
 */

using namespace std;
using namespace DFHack;
using namespace df::enums;

using df::global::ui;
using df::global::ui_build_selector;
using df::global::ui_menu_width;

static df::coord last_view, last_cursor;
const int16_t menu_shift = Gui::MENU_WIDTH / 2 + 1;

struct stable_cursor_hook : df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool check_default()
    {
        switch (ui->main.mode) {
            case ui_sidebar_mode::Default:
                return true;

            case ui_sidebar_mode::Build:
                return ui_build_selector &&
                       (ui_build_selector->building_type < 0 ||
                        ui_build_selector->stage < 1);

            default:
                return false;
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        bool was_default = check_default();
        df::coord view = Gui::getViewportPos();
        df::coord cursor = Gui::getCursorPos();
        Gui::DwarfmodeDims dims = Gui::getDwarfmodeViewDims();

        INTERPOSE_NEXT(feed)(input);

        bool is_default = check_default();
        df::coord cur_cursor = Gui::getCursorPos();

        if (is_default && !was_default)
        {
            if (dims.menu_forced)
            {
                // Menu disappeared, reported view info is wrong
                view.x -= min(menu_shift, view.x);

                // Map has now expanded to where the menu once was
                dims.map_x2 += Gui::MENU_WIDTH + 1;

                uint32_t map_x, map_y, map_z;
                Maps::getTileSize(map_x, map_y, map_z);

                // At the right edge of the map, it's always shifted
                // over so the right side of the viewport is menu_shift
                // tiles from the right edge of the map
                view.x = min(view.x, static_cast<int16_t>(map_x - dims.map_x2 - menu_shift));
            }
            last_view = view; last_cursor = cursor;
        }
        else if (!is_default && last_cursor.isValid() && cur_cursor.isValid())
        {
            if (was_default && dims.menu_forced)
            {
                // The previous state had the menu open in default mode
                // (i.e., it was the Build menu asking for building type).
                // So now we need to do menu shift because the reported view
                // position is already accounting for the menu.
                view.x -= min(menu_shift, view.x);
            }
            if (was_default && view == last_view)
            {
                Gui::setCursorCoords(last_cursor.x, last_cursor.y, last_cursor.z);
                Gui::refreshSidebar();
            }

            last_cursor = df::coord();
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(stable_cursor_hook, feed);
