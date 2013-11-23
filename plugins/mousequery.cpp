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

using df::global::world;
using df::global::ui;

using namespace df::enums::ui_sidebar_mode;

DFHACK_PLUGIN("mousequery");

#define PLUGIN_VERSION 0.16

static int32_t last_x, last_y, last_z;
static size_t max_list_size = 300000; // Avoid iterating over huge lists

static bool plugin_enabled = true;
static bool rbutton_enabled = true;
static bool tracking_enabled = false;
static bool active_scrolling = false;
static bool box_designation_enabled = false;
static bool live_view = false;

static int scroll_delay = 100;

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
        case DesignateChopTrees:
        case DesignateToggleEngravings:
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

    bool handleMouse(const set<df::interface_key> *input)
    {
        int32_t mx, my;
        auto mpos = get_mouse_pos(mx, my);
        if (mpos.x == -30000)
            return false;

        auto dims = Gui::getDwarfmodeViewDims();
        if (enabler->mouse_lbut)
        {
            bool cursor_still_here = (last_x == mpos.x && last_y == mpos.y && last_z == mpos.z);
            last_x = mpos.x;
            last_y = mpos.y;
            last_z = mpos.z;

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
                    if (df::global::ui_build_selector)
                    {
                        if (df::global::ui_build_selector->stage < 2)
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
            int right_bound = (dims.menu_x1 > 0) ? dims.menu_x1 - 2 : gps->dimx - 2;
            if (mx < 1 || mx > right_bound || my < 1 || my > gps->dimy - 2)
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
                moveCursor(mpos);
            }

            if (designationMode)
                sendKey(key);

            return true;
        }
        else if (rbutton_enabled && enabler->mouse_rbut)
        {
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
                int scroll_trigger_x = dims.menu_x1 / 3;
                int scroll_trigger_y = gps->dimy / 3;
                if (mx < scroll_trigger_x)
                    sendKey(interface_key::CURSOR_LEFT_FAST);

                if (mx > ((dims.menu_x1 > 0) ? dims.menu_x1 : gps->dimx) - scroll_trigger_x)
                    sendKey(interface_key::CURSOR_RIGHT_FAST);

                if (my < scroll_trigger_y)
                    sendKey(interface_key::CURSOR_UP_FAST);

                if (my > gps->dimy - scroll_trigger_y)
                    sendKey(interface_key::CURSOR_DOWN_FAST);
            }
        }
        else if (input->count(interface_key::CUSTOM_M) && isInDesignationMenu())
        {
            box_designation_enabled = !box_designation_enabled;
        }

        return false;
    }

    void moveCursor(df::coord &mpos)
    {
        int32_t x, y, z;
        Gui::getCursorCoords(x, y, z);
        if (mpos.x == x && mpos.y == y && mpos.z == z)
            return;

        Gui::setCursorCoords(mpos.x, mpos.y, mpos.z);
        sendKey(interface_key::CURSOR_DOWN_Z);
        sendKey(interface_key::CURSOR_UP_Z);
    }

    bool inBuildPlacement()
    {
        return df::global::ui_build_selector &&
            df::global::ui_build_selector->building_type != -1 &&
            df::global::ui_build_selector->stage == 1;
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

        int32_t mx, my;
        auto mpos = get_mouse_pos(mx, my);
        if (mpos.x == -30000 || mpos.y == -30000 || mpos.z == -30000)
            return;

        auto dims = Gui::getDwarfmodeViewDims();
        auto right_margin = (dims.menu_x1 > 0) ? dims.menu_x1 : gps->dimx;

        if (isInDesignationMenu())
        {
            int left_margin = dims.menu_x1 + 1;
            int x = left_margin;
            int y = 23;
            OutputString(COLOR_BROWN, x, y, "DFHack MouseQuery", true, left_margin);
            OutputToggleString(x, y, "Box Select", "m", box_designation_enabled, true, left_margin);
        }

        if (mx < 1 || mx > right_margin - 2 || my < 1 || my > gps->dimy - 2)
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

            if (mx > right_margin - scroll_buffer)
            {
                sendKey(interface_key::CURSOR_RIGHT);
                return;
            }

            if (my < scroll_buffer)
            {
                sendKey(interface_key::CURSOR_UP);
                return;
            }

            if (my > gps->dimy - scroll_buffer)
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

            OutputString(color, mx, my, "X");
            return;
        }

        if (shouldTrack())
        {
            if (delta_t <= scroll_delay && (mx < scroll_buffer || 
                mx > dims.menu_x1 - scroll_buffer || 
                my < scroll_buffer ||
                my > gps->dimy - scroll_buffer))
            {
                return;
            }

            last_t = enabler->clock;
            moveCursor(mpos);
        }

        if (dims.menu_x1 <= 0)
            return; // No menu displayed

        if (!is_valid_pos(mpos) || isInTrackableMode())
            return;

        // Display live query
        auto ulist = get_units_at(mpos, false);
        auto bld = Buildings::findAtTile(mpos);
        auto ilist = get_items_at(mpos, false);

        int look_list = ulist.size() + ((bld) ? 1 : 0) + ilist.size() + 1;
        set_to_limit(look_list, 8);
        int y = gps->dimy - look_list - 2;

        int left_margin = dims.menu_x1 + 1;
        int look_width = dims.menu_x2 - dims.menu_x1 - 1;
        int x = left_margin;

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

            OutputString(COLOR_WHITE, x, y, label, true, left_margin);
        }

        for (auto it = ilist.begin(); it != ilist.end() && c < 8; it++, c++)
        {
            auto label = Items::getDescription(*it, 0, false);
            label = pad_string(label, look_width, false, true);
            OutputString(COLOR_YELLOW, x, y, label, true, left_margin);
        }

        if (c > 7)
            return;

        if (bld)
        {
            string label;
            bld->getName(&label);
            label = pad_string(label, look_width, false, true);
            OutputString(COLOR_CYAN, x, y, label, true, left_margin);
        }

        if (c > 7)
            return;

        auto tt = Maps::getTileType(mpos);
        OutputString(COLOR_BLUE, x, y, tileName(*tt), true, left_margin);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE_PRIO(mousequery_hook, feed, 100);
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(mousequery_hook, render, 100);

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
        last_x = last_y = last_z = -1;

        if (!INTERPOSE_HOOK(mousequery_hook, feed).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    last_x = last_y = last_z = -1;

    commands.push_back(
        PluginCommand(
        "mousequery", "Add mouse functionality to Dwarf Fortress",
        mousequery_cmd, false, 
        "mousequery [plugin|rbutton|track|edge|live] [enabled|disabled]\n"
        "  plugin: enable/disable the entire plugin\n"
        "  rbutton: enable/disable right mouse button\n"
        "  track: enable/disable moving cursor in build and designation mode\n"
        "  edge: enable/disable active edge scrolling (when on, will also enable tracking)\n"
        "  live: enable/disable query view when unpaused\n\n"
        "mousequery delay <amount>\n"
        "  Set delay when edge scrolling in tracking mode. Omit amount to display current setting.\n"
        ));

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        last_x = last_y = last_z = -1;
        break;
    default:
        break;
    }
    return CR_OK;
}
