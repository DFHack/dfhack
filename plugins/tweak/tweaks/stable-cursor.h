/*
 * Save or restore cursor position on change to/from main dwarfmode menu.
 */

using namespace std;
using namespace DFHack;
using namespace df::enums;

using df::global::plotinfo;
using df::global::ui_build_selector;
using df::global::ui_menu_width;

static df::coord last_view, last_cursor;

struct stable_cursor_hook : df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool check_default()
    {
        switch (plotinfo->main.mode) {
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

    bool check_viewport_near_enough()
    {
        df::coord view = Gui::getViewportPos();
        auto dims = Gui::getDwarfmodeViewDims();
        int view_size = min(dims.map_x2 - dims.map_x1, dims.map_y2 - dims.map_y1);
        // Heuristic to keep the cursor position if the view is still in the
        // part of the map that was already visible
        int near_enough_distance = min(30, view_size / 2);

        return
            // Is the viewport near enough to the old viewport?
            abs(view.x - last_view.x) <= near_enough_distance &&
            abs(view.y - last_view.y) <= near_enough_distance &&
            view.z == last_view.z &&
            // And is the last cursor visible in the current viewport?
            last_cursor.x >= view.x && last_cursor.y >= view.y &&
            last_cursor.x <= view.x + dims.map_x2 - dims.map_x1 &&
            last_cursor.y <= view.y + dims.map_y2 - dims.map_y1;
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
        else if (!is_default && was_default && cur_cursor.isValid() &&
                 last_cursor.isValid() && check_viewport_near_enough())
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
