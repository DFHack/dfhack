#include <VTableInterpose.h>

#include "df/building.h"
#include "df/item.h"
#include "df/unit.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"
#include "df/items_other_id.h"
#include "df/ui_build_selector.h"
#include "df/ui_sidebar_menus.h"

#include "modules/Gui.h"
#include "modules/World.h"
#include "modules/Maps.h"
#include "modules/Buildings.h"
#include "modules/Items.h"
#include "modules/Units.h"
#include "modules/Translation.h"

#include "uicommon.h"
#include "TileTypes.h"
#include "DataFuncs.h"

#include "df/renderer.h"
#include "renderer_twbt.h"

DFHACK_PLUGIN("mousequery");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_build_selector);

using namespace df::enums::ui_sidebar_mode;

#define PLUGIN_VERSION 0.18

static int32_t last_clicked_x, last_clicked_y, last_clicked_z;
static int32_t last_pos_x, last_pos_y, last_pos_z;
static df::coord last_move_pos;
static size_t max_list_size = 300000; // Avoid iterating over huge lists

static bool plugin_enabled = true;
static bool rbutton_enabled = true;
static bool tracking_enabled = false;
static bool active_scrolling = false;
static bool box_designation_enabled = false;
static bool live_view = true;
static bool skip_tracking_once = false;
static bool mouse_moved = false;

static int scroll_delay = 100;

static bool awaiting_lbut_up, awaiting_rbut_up;
static enum { None, Left, Right } drag_mode;
color_ostream *out2;
static df::coord get_mouse_pos(int32_t &mx, int32_t &my)
{
    df::coord pos;
    pos.x = -30000;

    if (!enabler->tracking_on)
        return pos;

    if (!Gui::getMousePos(mx, my))
        return pos;

    int32_t vx, vy, vz;
    if (!Gui::getViewCoords(vx, vy, vz))
        return pos;

    pos.x = vx + mx - 1;
    pos.y = vy + my - 1;

    renderer_cool *r = (renderer_cool*)enabler->renderer;
    if (r->is_twbt())
        pos.z = vz - r->depth_at(mx, my);
    else
        pos.z = vz;

    return pos;
}

static bool is_valid_pos(const df::coord pos)
{
    auto designation = Maps::getTileDesignation(pos);
    if (!designation)
        return false;

    if (designation->bits.hidden)
        return false; // Items in parts of the map not yet revealed

    return true;
}

static vector<df::unit *> get_units_at(const df::coord pos, bool only_one)
{
    vector<df::unit *> list;

    auto count = world->units.active.size();
    if (count > max_list_size)
        return list;

    df::unit_flags1 bad_flags;
    bad_flags.whole = 0;
    bad_flags.bits.dead = true;
    bad_flags.bits.hidden_ambusher = true;
    bad_flags.bits.hidden_in_ambush = true;

    for (size_t i = 0; i < count; i++)
    {
        df::unit *unit = world->units.active[i];

        if(unit->pos.x == pos.x && unit->pos.y == pos.y && unit->pos.z == pos.z &&
            !(unit->flags1.whole & bad_flags.whole) &&
            unit->profession != profession::THIEF && unit->profession != profession::MASTER_THIEF)
        {
            list.push_back(unit);
            if (only_one)
                break;
        }
    }

    return list;
}

static vector<df::item *> get_items_at(const df::coord pos, bool only_one)
{
    vector<df::item *> list;
    auto count = world->items.other[items_other_id::IN_PLAY].size();
    if (count > max_list_size)
        return list;

    df::item_flags bad_flags;
    bad_flags.whole = 0;
    bad_flags.bits.in_building = true;
    bad_flags.bits.garbage_collect = true;
    bad_flags.bits.removed = true;
    bad_flags.bits.dead_dwarf = true;
    bad_flags.bits.murder = true;
    bad_flags.bits.construction = true;
    bad_flags.bits.in_inventory = true;
    bad_flags.bits.in_chest = true;

    for (size_t i = 0; i < count; i++)
    {
        df::item *item = world->items.other[items_other_id::IN_PLAY][i];
        if (item->flags.whole & bad_flags.whole)
            continue;

        if (pos.z == item->pos.z && pos.x == item->pos.x && pos.y == item->pos.y)
            list.push_back(item);
    }

    return list;
}

static df::interface_key get_default_query_mode(const df::coord pos)
{
    if (!is_valid_pos(pos))
        return df::interface_key::D_LOOK;

    bool fallback_to_building_query = false;

    // Check for unit under cursor
    auto ulist = get_units_at(pos, true);
    if (ulist.size() > 0)
        return df::interface_key::D_VIEWUNIT;

    // Check for building under cursor
    auto bld = Buildings::findAtTile(pos);
    if (bld)
    {
        df::building_type type = bld->getType();

        if (type == building_type::Stockpile)
        {
            fallback_to_building_query = true;
        }
        else
        {
            // For containers use item view, for everything else, query view
            return (type == building_type::Box || type == building_type::Cabinet ||
                type == building_type::Weaponrack || type == building_type::Armorstand)
                ? df::interface_key::D_BUILDITEM : df::interface_key::D_BUILDJOB;
        }
    }

    // Check for items under cursor
    auto ilist = get_items_at(pos, true);
    if (ilist.size() > 0)
        return df::interface_key::D_LOOK;

    return (fallback_to_building_query) ? df::interface_key::D_BUILDJOB : df::interface_key::D_LOOK;
}

extern "C" {
    unsigned char *SDL_GetKeyState(int *numkeys);
}

struct mousequery_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    void sendKey(const df::interface_key &key)
    {
        set<df::interface_key> tmp;
        tmp.insert(key);
        INTERPOSE_NEXT(feed)(&tmp);
    }

    bool isInDesignationMenu()
    {
        switch (ui->main.mode)
        {
        case DesignateMine:
        case DesignateRemoveRamps:
        case DesignateUpStair:
        case DesignateDownStair:
        case DesignateUpDownStair:
        case DesignateUpRamp:
        case DesignateChannel:
        case DesignateGatherPlants:
        case DesignateRemoveDesignation:
        case DesignateSmooth:
        case DesignateCarveTrack:
        case DesignateEngrave:
        case DesignateCarveFortification:
        case DesignateItemsClaim:
        case DesignateItemsForbid:
        case DesignateItemsMelt:
        case DesignateItemsUnmelt:
        case DesignateItemsDump:
        case DesignateItemsUndump:
        case DesignateItemsHide:
        case DesignateItemsUnhide:
        case DesignateChopTrees:
        case DesignateToggleEngravings:
        case DesignateToggleMarker:
        case DesignateTrafficHigh:
        case DesignateTrafficNormal:
        case DesignateTrafficLow:
        case DesignateTrafficRestricted:
        case DesignateRemoveConstruction:
            return true;

        case Burrows:
            return ui->burrows.in_define_mode;
        };

        return false;
    }

    bool isInTrackableMode()
    {
        if (isInDesignationMenu())
            return box_designation_enabled;

        switch (ui->main.mode)
        {
        case DesignateItemsClaim:
        case DesignateItemsForbid:
        case DesignateItemsMelt:
        case DesignateItemsUnmelt:
        case DesignateItemsDump:
        case DesignateItemsUndump:
        case DesignateItemsHide:
        case DesignateItemsUnhide:
        case DesignateTrafficHigh:
        case DesignateTrafficNormal:
        case DesignateTrafficLow:
        case DesignateTrafficRestricted:
        case Stockpiles:
        case Squads:
        case NotesPoints:
        case NotesRoutes:
        case Zones:
            return true;

        case Build:
            return inBuildPlacement();

        case QueryBuilding:
        case BuildingItems:
        case ViewUnits:
        case LookAround:
            return !enabler->mouse_lbut;

        default:
            return false;
        };
    }

    bool isInAreaSelectionMode()
    {
        bool selectableMode =
            isInDesignationMenu() ||
            ui->main.mode == Stockpiles ||
            ui->main.mode == Zones;

        if (selectableMode)
        {
            int32_t x, y, z;
            return Gui::getDesignationCoords(x, y, z);
        }

        return false;
    }

    bool handleLeft(df::coord &mpos, int32_t mx, int32_t my)
    {
        static unsigned char *keystate = 0;
        if (!keystate)
            keystate = SDL_GetKeyState(NULL);

        if (!(keystate[303] || keystate[304]))
        {
            renderer_cool *r = (renderer_cool*)enabler->renderer;
            if (r->is_twbt())
                mpos.z += r->depth_at(mx, my);
        }

        renderer_cool *r = (renderer_cool*)enabler->renderer;
        auto mapdims = r->is_twbt() ? r->map_dims() : Gui::getDwarfmodeViewDims();

        bool cursor_still_here = (last_clicked_x == mpos.x && last_clicked_y == mpos.y && last_clicked_z == mpos.z);
        last_clicked_x = mpos.x;
        last_clicked_y = mpos.y;
        last_clicked_z = mpos.z;

        df::interface_key key = interface_key::NONE;
        bool designationMode = false;
        bool skipRefresh = false;

        if (isInTrackableMode())
        {
            designationMode = true;
            key = df::interface_key::SELECT;
        }
        else
        {
            switch (ui->main.mode)
            {
            case QueryBuilding:
                if (cursor_still_here)
                    key = df::interface_key::D_BUILDITEM;
                break;

            case BuildingItems:
                if (cursor_still_here)
                    key = df::interface_key::D_VIEWUNIT;
                break;

            case ViewUnits:
                if (cursor_still_here)
                    key = df::interface_key::D_LOOK;
                break;

            case LookAround:
                if (cursor_still_here)
                    key = df::interface_key::D_BUILDJOB;
                break;

            case Build:
                if (ui_build_selector)
                {
                    if (ui_build_selector->stage < 2)
                    {
                        designationMode = true;
                        key = df::interface_key::SELECT;
                    }
                    else
                    {
                        designationMode = true;
                        skipRefresh = true;
                        key = df::interface_key::SELECT_ALL;
                    }
                }
                break;

            case Default:
                break;

            default:
                return false;
            }
        }

        enabler->mouse_lbut = 0;

        // Can't check limits earlier as we must be sure we are in query or default mode
        // (so we can clear the button down flag)
        if (mx < 1 || mx > mapdims.map_x2 || my < 1 || my > mapdims.y2)
            return false;

        if (ui->main.mode == df::ui_sidebar_mode::Zones ||
            ui->main.mode == df::ui_sidebar_mode::Stockpiles)
        {
            int32_t x, y, z;
            if (Gui::getDesignationCoords(x, y, z))
            {
                auto dX = abs(x - mpos.x);
                if (dX > 30)
                    return false;

                auto dY = abs(y - mpos.y);
                if (dY > 30)
                    return false;
            }
        }

        if (!designationMode)
        {
            while (ui->main.mode != Default)
            {
                sendKey(df::interface_key::LEAVESCREEN);
            }

            if (key == interface_key::NONE)
                key = get_default_query_mode(mpos);

            sendKey(key);
        }

        if (!skipRefresh)
        {
            // Force UI refresh
            moveCursor(mpos, true);
        }

        if (designationMode)
            sendKey(key);

        return true;
    }

    bool handleRight(df::coord &mpos, int32_t mx, int32_t my)
    {
        renderer_cool *r = (renderer_cool*)enabler->renderer;
        auto dims = r->is_twbt() ? r->map_dims() : Gui::getDwarfmodeViewDims();

        if (isInDesignationMenu() && !box_designation_enabled)
            return false;

        // Escape out of query mode
        enabler->mouse_rbut_down = 0;
        enabler->mouse_rbut = 0;

        using namespace df::enums::ui_sidebar_mode;
        if ((ui->main.mode == QueryBuilding || ui->main.mode == BuildingItems ||
            ui->main.mode == ViewUnits || ui->main.mode == LookAround) ||
            (isInTrackableMode() && tracking_enabled))
        {
            sendKey(df::interface_key::LEAVESCREEN);
        }
        else
        {
            int scroll_trigger_x = dims.map_x2 / 3;
            int scroll_trigger_y = dims.y2 / 3;
            if (mx < scroll_trigger_x)
                sendKey(interface_key::CURSOR_LEFT_FAST);

            if (mx > dims.map_x2 - scroll_trigger_x)
                sendKey(interface_key::CURSOR_RIGHT_FAST);

            if (my < scroll_trigger_y)
                sendKey(interface_key::CURSOR_UP_FAST);

            if (my > dims.y2 - scroll_trigger_y)
                sendKey(interface_key::CURSOR_DOWN_FAST);
        }

        return false;
    }

    bool handleMouse(const set<df::interface_key> *input)
    {
        int32_t mx, my;
        auto mpos = get_mouse_pos(mx, my);
        if (mpos.x == -30000)
            return false;

        if (enabler->mouse_lbut)
        {
            if (drag_mode == Left)
            {
                awaiting_lbut_up = true;
                enabler->mouse_lbut = false;
                last_move_pos = mpos;
            }
            else
                return handleLeft(mpos, mx, my);
        }
        else if (enabler->mouse_rbut)
        {
            if (drag_mode == Right)
            {
                awaiting_rbut_up = true;
                enabler->mouse_rbut = false;
                last_move_pos = mpos;
            }
            else if (rbutton_enabled)
                return handleRight(mpos, mx, my);
        }
        else if (input->count(interface_key::CUSTOM_ALT_M) && isInDesignationMenu())
        {
            box_designation_enabled = !box_designation_enabled;
        }
        else
        {
            if (input->count(interface_key::CURSOR_UP) ||
                input->count(interface_key::CURSOR_DOWN) ||
                input->count(interface_key::CURSOR_LEFT) ||
                input->count(interface_key::CURSOR_RIGHT) ||
                input->count(interface_key::CURSOR_UPLEFT) ||
                input->count(interface_key::CURSOR_UPRIGHT) ||
                input->count(interface_key::CURSOR_DOWNLEFT) ||
                input->count(interface_key::CURSOR_DOWNRIGHT) ||
                input->count(interface_key::CURSOR_UP_FAST) ||
                input->count(interface_key::CURSOR_DOWN_FAST) ||
                input->count(interface_key::CURSOR_LEFT_FAST) ||
                input->count(interface_key::CURSOR_RIGHT_FAST) ||
                input->count(interface_key::CURSOR_UPLEFT_FAST) ||
                input->count(interface_key::CURSOR_UPRIGHT_FAST) ||
                input->count(interface_key::CURSOR_DOWNLEFT_FAST) ||
                input->count(interface_key::CURSOR_DOWNRIGHT_FAST) ||
                input->count(interface_key::CURSOR_UP_Z) ||
                input->count(interface_key::CURSOR_DOWN_Z) ||
                input->count(interface_key::CURSOR_UP_Z_AUX) ||
                input->count(interface_key::CURSOR_DOWN_Z_AUX))
            {
                mouse_moved = false;
                if (shouldTrack())
                    skip_tracking_once = true;
            }
        }

        return false;
    }

    void moveCursor(df::coord &mpos, bool forced)
    {
        bool should_skip_tracking = skip_tracking_once;
        skip_tracking_once = false;
        if (!forced)
        {
            if (mpos.x == last_pos_x && mpos.y == last_pos_y && mpos.z == last_pos_z)
                return;
        }

        last_pos_x = mpos.x;
        last_pos_y = mpos.y;
        last_pos_z = mpos.z;

        if (!forced && should_skip_tracking)
        {
            return;
        }

        int32_t x, y, z;
        Gui::getCursorCoords(x, y, z);
        if (mpos.x == x && mpos.y == y && mpos.z == z)
            return;

        Gui::setCursorCoords(mpos.x, mpos.y, mpos.z);
        if (mpos.z == 0)
        {
            sendKey(interface_key::CURSOR_UP_Z);
            sendKey(interface_key::CURSOR_DOWN_Z);
        }
        else
        {
            sendKey(interface_key::CURSOR_DOWN_Z);
            sendKey(interface_key::CURSOR_UP_Z);
        }
    }

    bool inBuildPlacement()
    {
        return ui_build_selector &&
            ui_build_selector->building_type != -1 &&
            ui_build_selector->stage == 1;
    }

    bool shouldTrack()
    {
        if (!tracking_enabled)
            return false;

        return isInTrackableMode();
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!plugin_enabled || !handleMouse(input))
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (!plugin_enabled)
            return;

        static decltype(enabler->clock) last_t = 0;

        renderer_cool *r = (renderer_cool*)enabler->renderer;
        auto txtdims = Gui::getDwarfmodeViewDims();
        auto mapdims = r->is_twbt() ? r->map_dims() : txtdims;

        int32_t mx, my;
        auto mpos = get_mouse_pos(mx, my);
        bool mpos_valid = mpos.x != -30000 && mpos.y != -30000 && mpos.z != -30000;
        if (mx < 1 || mx > mapdims.map_x2 || my < 1 || my > mapdims.y2)
            mpos_valid = false;

        // Check if in lever binding mode
        if (Gui::getFocusString(Core::getTopViewscreen()) ==
            "dwarfmode/QueryBuilding/Some/Lever/AddJob")
        {
            return;
        }

        if (awaiting_lbut_up && !enabler->mouse_lbut_down)
        {
            awaiting_lbut_up = false;
            handleLeft(mpos, mx, my);
        }

        if (awaiting_rbut_up && !enabler->mouse_rbut_down)
        {
            awaiting_rbut_up = false;
            if (rbutton_enabled)
                handleRight(mpos, mx, my);
        }

        if (mpos_valid)
        {
            if (mpos.x != last_move_pos.x || mpos.y != last_move_pos.y || mpos.z != last_move_pos.z)
            {
                awaiting_lbut_up = false;
                awaiting_rbut_up = false;

                if ((enabler->mouse_lbut_down && drag_mode == Left) || (enabler->mouse_rbut_down && drag_mode == Right))
                {
                    int newx = (*df::global::window_x) - (mpos.x - last_move_pos.x);
                    int newy = (*df::global::window_y) - (mpos.y - last_move_pos.y);

                    renderer_cool *r = (renderer_cool*)enabler->renderer;
                    if (r->is_twbt())
                    {
                        newx = std::max(0, std::min(newx, world->map.x_count - r->gdimxfull));
                        newy = std::max(0, std::min(newy, world->map.y_count - r->gdimyfull));
                    }
                    else
                    {
                        newx = std::max(0, std::min(newx, world->map.x_count - mapdims.map_x2));
                        newy = std::max(0, std::min(newy, world->map.y_count - mapdims.y2));
                    }

                    (*df::global::window_x) = newx;
                    (*df::global::window_y) = newy;
                    return;
                }

                mouse_moved = true;
                last_move_pos = mpos;
            }
        }

        int left_margin = txtdims.menu_x1 + 1;
        int look_width = txtdims.menu_x2 - txtdims.menu_x1 - 1;
        int disp_x = left_margin;

        if (isInDesignationMenu())
        {
            int x = left_margin;
            int y = gps->dimy - 2;
            OutputToggleString(x, y, "Box Select", "Alt+M", box_designation_enabled,
                true, left_margin, COLOR_WHITE, COLOR_LIGHTRED);
        }

        //Display selection dimensions
        bool showing_dimensions = false;
        if (isInAreaSelectionMode())
        {
            showing_dimensions = true;
            int32_t x, y, z;
            Gui::getDesignationCoords(x, y, z);
            coord32_t curr_pos;

            if (!tracking_enabled && mouse_moved && mpos_valid &&
                (!isInDesignationMenu() || box_designation_enabled))
            {
                curr_pos = mpos;
            }
            else
            {
                Gui::getCursorCoords(curr_pos.x, curr_pos.y, curr_pos.z);
            }
            auto dX = abs(x - curr_pos.x) + 1;
            auto dY = abs(y - curr_pos.y) + 1;
            auto dZ = abs(z - curr_pos.z) + 1;

            int disp_y = gps->dimy - 3;
            stringstream label;
            label << "Selection: " << dX << "x" << dY << "x" << dZ;
            OutputString(COLOR_WHITE, disp_x, disp_y, label.str());
        }
        else
        {
            mouse_moved = false;
        }

        if (!mpos_valid)
            return;

        int scroll_buffer = 6;
        auto delta_t = enabler->clock - last_t;
        if (active_scrolling && !isInTrackableMode() && delta_t > scroll_delay)
        {
            last_t = enabler->clock;
            if (mx < scroll_buffer)
            {
                sendKey(interface_key::CURSOR_LEFT);
                return;
            }

            if (mx > mapdims.map_x2 - scroll_buffer)
            {
                sendKey(interface_key::CURSOR_RIGHT);
                return;
            }

            if (my < scroll_buffer)
            {
                sendKey(interface_key::CURSOR_UP);
                return;
            }

            if (my > mapdims.y2 - scroll_buffer)
            {
                sendKey(interface_key::CURSOR_DOWN);
                return;
            }
        }

        if (!live_view && !isInTrackableMode() && !DFHack::World::ReadPauseState())
            return;

        if (!tracking_enabled && isInTrackableMode())
        {
            UIColor color = COLOR_GREEN;
            int32_t x, y, z;
            if (Gui::getDesignationCoords(x, y, z))
            {
                color = COLOR_WHITE;
                if (ui->main.mode == df::ui_sidebar_mode::Zones ||
                    ui->main.mode == df::ui_sidebar_mode::Stockpiles)
                {
                    auto dX = abs(x - mpos.x);
                    if (dX > 30)
                        color = COLOR_RED;

                    auto dY = abs(y - mpos.y);
                    if (dY > 30)
                        color = COLOR_RED;
                }
            }

            renderer_cool *r = (renderer_cool*)enabler->renderer;
            if (r->is_twbt())
                r->output_char(color, mx, my, 'X');
            else
                OutputString(color, mx, my, "X");
            return;
        }

        if (shouldTrack())
        {
            if (delta_t <= scroll_delay && (mx < scroll_buffer ||
                mx > mapdims.map_x2 - scroll_buffer ||
                my < scroll_buffer ||
                my > mapdims.y2 - scroll_buffer))
            {
                return;
            }

            // Don't change levels
            if (mpos.z != *df::global::window_z)
                return;

            last_t = enabler->clock;
            moveCursor(mpos, false);
        }

        if (txtdims.menu_x1 <= 0)
            return; // No menu displayed

        if (!is_valid_pos(mpos) || isInTrackableMode())
            return;

        if (showing_dimensions)
            return;

        // Display live query
        auto ulist = get_units_at(mpos, false);
        auto bld = Buildings::findAtTile(mpos);
        auto ilist = get_items_at(mpos, false);

        int look_list = ulist.size() + ((bld) ? 1 : 0) + ilist.size() + 1;
        set_to_limit(look_list, 8);
        int disp_y = gps->dimy - look_list - 2;

        if (mpos.z != *df::global::window_z)
        {
            int y = gps->dimy - 2;
            char buf[6];
            sprintf(buf, "@%d", mpos.z - *df::global::window_z);
            OutputString(COLOR_GREY, disp_x, y, buf, true, left_margin);
        }

        int c = 0;
        for (auto it = ulist.begin(); it != ulist.end() && c < 8; it++, c++)
        {
            string label;
            auto name = Units::getVisibleName(*it);
            if (name->has_name)
                label = Translation::TranslateName(name, false);
            if (label.length() > 0)
                label += ", ";

            label += Units::getProfessionName(*it); // Check animal type too
            label = pad_string(label, look_width, false, true);

            OutputString(COLOR_WHITE, disp_x, disp_y, label, true, left_margin);
        }

        for (auto it = ilist.begin(); it != ilist.end() && c < 8; it++, c++)
        {
            auto label = Items::getDescription(*it, 0, false);
            label = pad_string(label, look_width, false, true);
            OutputString(COLOR_YELLOW, disp_x, disp_y, label, true, left_margin);
        }

        if (c > 7)
            return;

        if (bld)
        {
            string label;
            bld->getName(&label);
            label = pad_string(label, look_width, false, true);
            OutputString(COLOR_CYAN, disp_x, disp_y, label, true, left_margin);
        }

        if (c > 7)
            return;

        auto tt = Maps::getTileType(mpos);
        OutputString(COLOR_BLUE, disp_x, disp_y, tileName(*tt), true, left_margin);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE_PRIO(mousequery_hook, feed, 100);
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(mousequery_hook, render, 300);

static command_result mousequery_cmd(color_ostream &out, vector <string> & parameters)
{
    bool show_help = false;
    if (parameters.size() < 1)
    {
        show_help = true;
    }
    else
    {
        auto cmd = toLower(parameters[0]);
        auto state = (parameters.size() == 2) ? toLower(parameters[1]) : "-1";
        if (cmd[0] == 'v')
        {
            out << "MouseQuery" << endl << "Version: " << PLUGIN_VERSION << endl;
        }
        else if (cmd[0] == 'p')
        {
            plugin_enabled = (state == "enable");
        }
        else if (cmd[0] == 'r')
        {
            rbutton_enabled = (state == "enable");
        }
        else if (cmd[0] == 't')
        {
            tracking_enabled = (state == "enable");
            if (!tracking_enabled)
                active_scrolling = false;
        }
        else if (cmd[0] == 'e')
        {
            active_scrolling = (state == "enable");
            if (active_scrolling)
                tracking_enabled = true;
        }
        else if (cmd[0] == 'l')
        {
            live_view = (state == "enable");
        }
        else if (cmd == "drag")
        {
            if (state == "left")
                drag_mode = Left;
            else if (state == "right")
                drag_mode = Right;
            else if (state == "disable")
                drag_mode = None;
        }
        else if (cmd[0] == 'd')
        {
            auto l = atoi(state.c_str());
            if (l > 0 || state == "0")
                scroll_delay = l;
            else
                out << "Current delay: " << scroll_delay << endl;
        }
        else
        {
            show_help = true;
        }
    }

    if (show_help)
        return CR_WRONG_USAGE;

    return CR_OK;
}
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (is_enabled != enable)
    {
        last_clicked_x = last_clicked_y = last_clicked_z = -1;
        last_pos_x = last_pos_y = last_pos_z = -1;
        last_move_pos.x = last_move_pos.y = last_move_pos.z = -1;

        if (!INTERPOSE_HOOK(mousequery_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(mousequery_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    out2 = &out;
    commands.push_back(
        PluginCommand(
        "mousequery", "Add mouse functionality to Dwarf Fortress",
        mousequery_cmd, false,
        "mousequery [plugin|rbutton|track|edge|live] [enable|disable]\n"
        "  plugin: enable/disable the entire plugin\n"
        "  rbutton: enable/disable right mouse button\n"
        "  track: enable/disable moving cursor in build and designation mode\n"
        "  edge: enable/disable active edge scrolling (when on, will also enable tracking)\n"
        "  live: enable/disable query view when unpaused\n\n"
        "mousequery drag [left|right|disable]\n"
        "  Enable/disable map dragging with the specified mouse button\n\n"
        "mousequery delay <amount>\n"
        "  Set delay when edge scrolling in tracking mode. Omit amount to display current setting.\n"
        ));

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        last_clicked_x = last_clicked_y = last_clicked_z = -1;
        last_pos_x = last_pos_y = last_pos_z = -1;
        last_move_pos.x = last_move_pos.y = last_move_pos.z = -1;
        break;
    default:
        break;
    }
    return CR_OK;
}
