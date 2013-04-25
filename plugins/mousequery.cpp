#include <VTableInterpose.h>

#include "df/building.h"
#include "df/item.h"
#include "df/unit.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

#include "modules/Gui.h"

#include "uicommon.h"

using df::global::world;
using df::global::ui;

using namespace df::enums::ui_sidebar_mode;

static int32_t last_x, last_y, last_z;
static size_t max_list_size = 300000; // Avoid iterating over huge lists

struct mousequery_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    void send_key(const df::interface_key &key)
    {
        set<df::interface_key> tmp;
        tmp.insert(key);
        INTERPOSE_NEXT(feed)(&tmp);
        //this->feed(&tmp);
    }

    df::interface_key get_default_query_mode(const int32_t &x, const int32_t &y, const int32_t &z)
    {
        bool fallback_to_building_query = false;

        // Check for unit under cursor
        size_t count = world->units.all.size();
        if (count <= max_list_size)
        {
            for(size_t i = 0; i < count; i++)
            {
                df::unit *unit = world->units.all[i];

                if(unit->pos.x == x && unit->pos.y == y && unit->pos.z == z)
                    return df::interface_key::D_VIEWUNIT;
            }
        }
        else
        {
            fallback_to_building_query = true;
        }

        // Check for building under cursor
        count = world->buildings.all.size();
        if (count <= max_list_size)
        {
            for(size_t i = 0; i < count; i++)
            {
                df::building *bld = world->buildings.all[i];

                if (z == bld->z && 
                    x >= bld->x1 && x <= bld->x2 &&
                    y >= bld->y1 && y <= bld->y2)
                {
                    df::building_type type = bld->getType();

                    if (type == building_type::Stockpile)
                    {
                        fallback_to_building_query = true;
                        break; // Check for items in stockpile first
                    }

                    // For containers use item view, fir everything else, query view
                    return (type == building_type::Box || type == building_type::Cabinet ||
                        type == building_type::Weaponrack || type == building_type::Armorstand) 
                        ? df::interface_key::D_BUILDITEM : df::interface_key::D_BUILDJOB;
                }
            }
        }
        else
        {
            fallback_to_building_query = true;
        }


        // Check for items under cursor
        count = world->items.all.size();
        if (count <= max_list_size)
        {
            for(size_t i = 0; i < count; i++)
            {
                df::item *item = world->items.all[i];
                if (z == item->pos.z && x == item->pos.x && y == item->pos.y && 
                    !item->flags.bits.in_building && !item->flags.bits.hidden &&
                    !item->flags.bits.in_job && !item->flags.bits.in_chest &&
                    !item->flags.bits.in_inventory)
                {
                    return df::interface_key::D_LOOK;
                }
            }
        }
        else
        {
            fallback_to_building_query = true;
        }

        return (fallback_to_building_query) ? df::interface_key::D_BUILDJOB : df::interface_key::D_LOOK;
    }

    bool handle_mouse(const set<df::interface_key> *input)
    {
        int32_t cx, cy, vz;
        if (!enabler->tracking_on)
            return false;

        int32_t mx, my;
        if (!Gui::getMousePos(mx, my))
            return false;

        int32_t vx, vy;
        if (!Gui::getViewCoords(vx, vy, vz))
            return false;

        cx = vx + mx - 1;
        cy = vy + my - 1;

        if (enabler->mouse_lbut)
        {
            bool cursor_still_here = (last_x == cx && last_y == cy && last_z == vz);
            last_x = cx;
            last_y = cy;
            last_z = vz;

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
                    send_key(df::interface_key::LEAVESCREEN);
                }

                if (key == interface_key::NONE)
                    key = get_default_query_mode(cx, cy, vz);
            }

            if (!designationMode)
                send_key(key);

            // Force UI refresh
            Gui::setCursorCoords(cx, cy, vz);
            send_key(interface_key::CURSOR_DOWN_Z);
            send_key(interface_key::CURSOR_UP_Z);

            if (designationMode)
                send_key(key);

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
                    send_key(df::interface_key::LEAVESCREEN);
                }
            }
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!handle_mouse(input))
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(mousequery_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(mousequery_hook, render);

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
