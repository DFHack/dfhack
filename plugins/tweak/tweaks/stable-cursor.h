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

        INTERPOSE_NEXT(feed)(input);

        bool is_default = check_default();
        df::coord cur_cursor = Gui::getCursorPos();

        if (is_default && !was_default)
        {
            last_view = view; last_cursor = cursor;
        }
        else if (!is_default && was_default &&
                 Gui::getViewportPos() == last_view &&
                 last_cursor.isValid() && cur_cursor.isValid())
        {
            Gui::setCursorCoords(last_cursor.x, last_cursor.y, last_cursor.z);
            Gui::refreshSidebar();
        }
        else if (!is_default && cur_cursor.isValid())
        {
            last_cursor = df::coord();
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(stable_cursor_hook, feed);
