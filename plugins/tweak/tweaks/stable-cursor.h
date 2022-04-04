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

    bool check_menu()
    {
        return ui_menu_width && (*ui_menu_width)[0] < (*ui_menu_width)[1];
    }

    bool check_area()
    {
        return ui_menu_width && (*ui_menu_width)[1] == 2;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        uint32_t mx, my, mz;
        Maps::getTileSize(mx, my, mz);
        df::coord map(mx, my, mz);

        // Full window has a 1-cell border on each side
        int16_t width = Screen::getWindowSize().x - 2;

        bool was_default = check_default();
        //bool was_menu = check_menu();
        df::coord view = Gui::getViewportPos();
        df::coord cursor = Gui::getCursorPos();

        INTERPOSE_NEXT(feed)(input);

        bool is_default = check_default();
        bool is_menu = check_menu();
        //df::coord cur_view = Gui::getViewportPos();
        df::coord cur_cursor = Gui::getCursorPos();

        if (is_default && !was_default)
        {
            if (!is_menu)
            {
                // Menu disappeared, reported view info is wrong
                view.x -= std::min(menu_shift, view.x);

                bool is_area = check_area();
                if (is_area)
                {
                    width -= (Gui::AREA_MAP_WIDTH + 1);
                }

                // At the right edge of the map, it's always shifted
                // over so the right side of the viewport is menu_shift
                // tiles from the right edge of the map
                view.x = std::min(view.x, static_cast<int16_t>(map.x - width - menu_shift));
            }
            last_view = view; last_cursor = cursor;
        }
        else if (!is_default && cur_cursor.isValid() && last_cursor.isValid())
        {
            if (was_default && view == last_view)
            {
                Gui::setCursorCoords(last_cursor.x, last_cursor.y, last_cursor.z);
                Gui::refreshSidebar();
            }
            else
            {
                last_cursor = df::coord();
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(stable_cursor_hook, feed);
