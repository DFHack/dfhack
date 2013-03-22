#include <sstream> 

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <VTableInterpose.h>

#include "DataDefs.h"

#include "df/building.h"
#include "df/enabler.h"
#include "df/item.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

#include "modules/Gui.h"
#include "modules/Screen.h"


using std::set;
using std::string;
using std::ostringstream;

using namespace DFHack;
using namespace df::enums;

using df::global::enabler;
using df::global::gps;
using df::global::world;
using df::global::ui;


static int32_t last_x, last_y, last_z;
static size_t max_list_size = 100000; // Avoid iterating over huge lists

struct mousequery_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    void send_key(const df::interface_key &key)
    {
        set<df::interface_key> tmp;
        tmp.insert(key);
        //INTERPOSE_NEXT(feed)(&tmp);
        this->feed(&tmp);
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
        if (enabler->tracking_on)
        {
            if (enabler->mouse_lbut)
            {
                int32_t mx, my;
                if (Gui::getMousePos(mx, my))
                {
                    int32_t vx, vy;
                    if (Gui::getViewCoords(vx, vy, vz))
                    {
                        cx = vx + mx - 1;
                        cy = vy + my - 1;

                        using namespace df::enums::ui_sidebar_mode;
                        df::interface_key key = interface_key::NONE;
                        bool cursor_still_here = (last_x == cx && last_y == cy && last_z == vz);
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

                        case Default:
                            break;

                        default:
                            return false;
                        }

                        enabler->mouse_lbut = 0;

                        // Can't check limits earlier as we must be sure we are in query or default mode we can clear the button flag
                        // Otherwise the feed gets stuck in a loop
                        uint8_t menu_width, area_map_width;
                        Gui::getMenuWidth(menu_width, area_map_width);
                        int32_t w = gps->dimx;
                        if (menu_width == 1) w -= 57; //Menu is open doubly wide
                        else if (menu_width == 2 && area_map_width == 3) w -= 33; //Just the menu is open
                        else if (menu_width == 2 && area_map_width == 2) w -= 26; //Just the area map is open

                        if (mx < 1 || mx > w || my < 1 || my > gps->dimy - 2)
                            return false;

                        while (ui->main.mode != Default)
                        {
                            send_key(df::interface_key::LEAVESCREEN);
                        }

                        if (key == interface_key::NONE)
                            key = get_default_query_mode(cx, cy, vz);

                        send_key(key);

                        // Force UI refresh
                        Gui::setCursorCoords(cx, cy, vz);
                        send_key(interface_key::CURSOR_DOWN_Z);
                        send_key(interface_key::CURSOR_UP_Z);
                        last_x = cx;
                        last_y = cy;
                        last_z = vz;

                        return true;
                    }
                }
            }
            else if (enabler->mouse_rbut)
            {
                // Escape out of query mode
                using namespace df::enums::ui_sidebar_mode;
                if (ui->main.mode == QueryBuilding || ui->main.mode == BuildingItems ||
                    ui->main.mode == ViewUnits || ui->main.mode == LookAround)
                {
                    while (ui->main.mode != Default)
                    {
                        enabler->mouse_rbut = 0;
                        send_key(df::interface_key::LEAVESCREEN);
                    }
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
};

IMPLEMENT_VMETHOD_INTERPOSE(mousequery_hook, feed);

DFHACK_PLUGIN("mousequery");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!gps || !INTERPOSE_HOOK(mousequery_hook, feed).apply())
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
