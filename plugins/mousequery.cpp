#include <VTableInterpose.h>

#include "df/building.h"
#include "df/item.h"
#include "df/unit.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"
#include "df/items_other_id.h"
#include "df/ui_build_selector.h"

#include "modules/Gui.h"
#include "modules/World.h"
#include "modules/Maps.h"
#include "modules/Buildings.h"
#include "modules/Items.h"
#include "modules/Units.h"
#include "modules/Translation.h"

#include "uicommon.h"
#include "TileTypes.h"

using df::global::world;
using df::global::ui;

using namespace df::enums::ui_sidebar_mode;

static int32_t last_x, last_y, last_z;
static size_t max_list_size = 300000; // Avoid iterating over huge lists

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

    bool handleMouse(const set<df::interface_key> *input)
    {
        int32_t mx, my;
        auto mpos = get_mouse_pos(mx, my);
        if (mpos.x == -30000)
            return false;

        if (enabler->mouse_lbut)
        {
            bool cursor_still_here = (last_x == mpos.x && last_y == mpos.y && last_z == mpos.z);
            last_x = mpos.x;
            last_y = mpos.y;
            last_z = mpos.z;

            df::interface_key key = interface_key::NONE;
            bool designationMode = false;
            switch(ui->main.mode)
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
            case DesignateTrafficHigh:
            case DesignateTrafficNormal:
            case DesignateTrafficLow:
            case DesignateTrafficRestricted:
            case DesignateRemoveConstruction:
                designationMode = true;
                key = df::interface_key::SELECT;
                break;

            case Build:
                if (df::global::ui_build_selector &&
                    df::global::ui_build_selector->stage < 2)
                {
                    designationMode = true;
                    key = df::interface_key::SELECT;
                }
                break;

            case Default:
                break;

            default:
                return false;
            }

            enabler->mouse_lbut = 0;

            // Can't check limits earlier as we must be sure we are in query or default mode 
            // (so we can clear the button down flag)
            auto dims = Gui::getDwarfmodeViewDims();
            int right_bound = (dims.menu_x1 > 0) ? dims.menu_x1 - 2 : gps->dimx - 2;
            if (mx < 1 || mx > right_bound || my < 1 || my > gps->dimy - 2)
                return false;

            if (!designationMode)
            {
                while (ui->main.mode != Default)
                {
                    sendKey(df::interface_key::LEAVESCREEN);
                }

                if (key == interface_key::NONE)
                    key = get_default_query_mode(mpos);
            }

            if (!designationMode)
                sendKey(key);

            // Force UI refresh
            Gui::setCursorCoords(mpos.x, mpos.y, mpos.z);
            sendKey(interface_key::CURSOR_DOWN_Z);
            sendKey(interface_key::CURSOR_UP_Z);

            if (designationMode)
                sendKey(key);

            return true;
        }
        else if (enabler->mouse_rbut)
        {
            // Escape out of query mode
            using namespace df::enums::ui_sidebar_mode;
            if (ui->main.mode == QueryBuilding || ui->main.mode == BuildingItems ||
                ui->main.mode == ViewUnits || ui->main.mode == LookAround)
            {
                enabler->mouse_rbut_down = 0;
                enabler->mouse_rbut = 0;
                while (ui->main.mode != Default)
                {
                    sendKey(df::interface_key::LEAVESCREEN);
                }
            }
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!handleMouse(input))
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (!DFHack::World::ReadPauseState())
            return;

        if (ui->main.mode != Default)
            return;

        int32_t mx, my;
        auto mpos = get_mouse_pos(mx, my);
        if (mpos.x == -30000)
            return;

        if (!is_valid_pos(mpos))
            return;

        auto dims = Gui::getDwarfmodeViewDims();
        if (dims.menu_x1 <= 0)
            return;

        if (mx < 1 || mx > dims.menu_x1 - 2 || my < 1 || my > gps->dimy - 2)
            return;

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

DFHACK_PLUGIN("mousequery");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!gps || !INTERPOSE_HOOK(mousequery_hook, feed).apply() || !INTERPOSE_HOOK(mousequery_hook, render).apply())
        out.printerr("Could not insert mousequery hooks!\n");

    last_x = last_y = last_z = -1;

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
