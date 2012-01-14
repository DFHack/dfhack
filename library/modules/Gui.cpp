/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "modules/Gui.h"
#include "MemAccess.h"
#include "VersionInfo.h"
#include "Types.h"
#include "Error.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "PluginManager.h"
#include "MiscUtils.h"
using namespace DFHack;

#include "DataDefs.h"
#include "df/world.h"
#include "df/cursor.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_unitjobsst.h"
#include "df/viewscreen_itemst.h"
#include "df/ui.h"
#include "df/ui_unit_view_mode.h"
#include "df/ui_sidebar_menus.h"
#include "df/ui_look_list.h"
#include "df/job.h"
#include "df/ui_build_selector.h"
#include "df/building_workshopst.h"
#include "df/building_furnacest.h"
#include "df/general_ref.h"
#include "df/unit_inventory_item.h"

using namespace df::enums;

// Predefined common guard functions

bool DFHack::default_hotkey(Core *, df::viewscreen *top)
{
    // Default hotkey guard function
    for (;top ;top = top->parent)
        if (strict_virtual_cast<df::viewscreen_dwarfmodest>(top))
            return true;
    return false;
}

bool DFHack::dwarfmode_hotkey(Core *, df::viewscreen *top)
{
    // Require the main dwarf mode screen
    return !!strict_virtual_cast<df::viewscreen_dwarfmodest>(top);
}

bool DFHack::unitjobs_hotkey(Core *, df::viewscreen *top)
{
    // Require the main dwarf mode screen
    return !!strict_virtual_cast<df::viewscreen_unitjobsst>(top);
}

bool DFHack::item_details_hotkey(Core *, df::viewscreen *top)
{
    // Require the main dwarf mode screen
    return !!strict_virtual_cast<df::viewscreen_itemst>(top);
}

bool DFHack::cursor_hotkey(Core *c, df::viewscreen *top)
{
    if (!dwarfmode_hotkey(c, top))
        return false;

    // Also require the cursor.
    if (!df::global::cursor || df::global::cursor->x == -30000)
        return false;

    return true;
}

bool DFHack::workshop_job_hotkey(Core *c, df::viewscreen *top)
{
    using namespace ui_sidebar_mode;
    using df::global::ui;
    using df::global::world;
    using df::global::ui_workshop_in_add;
    using df::global::ui_workshop_job_cursor;

    if (!dwarfmode_hotkey(c,top))
        return false;

    switch (ui->main.mode) {
    case QueryBuilding:
        {
            if (!ui_workshop_job_cursor) // allow missing
                return false;

            df::building *selected = world->selected_building;
            if (!virtual_cast<df::building_workshopst>(selected) &&
                !virtual_cast<df::building_furnacest>(selected))
                return false;

            // No jobs?
            if (selected->jobs.empty() ||
                selected->jobs[0]->job_type == job_type::DestroyBuilding)
                return false;

            // Add job gui activated?
            if (ui_workshop_in_add && *ui_workshop_in_add)
                return false;

            return true;
        };
    default:
        return false;
    }
}

bool DFHack::build_selector_hotkey(Core *c, df::viewscreen *top)
{
    using namespace ui_sidebar_mode;
    using df::global::ui;
    using df::global::ui_build_selector;

    if (!dwarfmode_hotkey(c,top))
        return false;

    switch (ui->main.mode) {
    case Build:
        {
            if (!ui_build_selector) // allow missing
                return false;

            // Not selecting, or no choices?
            if (ui_build_selector->building_type < 0 ||
                ui_build_selector->stage != 2 ||
                ui_build_selector->choices.empty())
                return false;

            return true;
        };
    default:
        return false;
    }
}

bool DFHack::view_unit_hotkey(Core *c, df::viewscreen *top)
{
    using df::global::ui;
    using df::global::world;
    using df::global::ui_selected_unit;

    if (!dwarfmode_hotkey(c,top))
        return false;
    if (ui->main.mode != ui_sidebar_mode::ViewUnits)
        return false;
    if (!ui_selected_unit) // allow missing
        return false;

    return vector_get(world->units.other[0], *ui_selected_unit) != NULL;
}

bool DFHack::unit_inventory_hotkey(Core *c, df::viewscreen *top)
{
    using df::global::ui_unit_view_mode;

    if (!view_unit_hotkey(c,top))
        return false;
    if (!ui_unit_view_mode)
        return false;

    return ui_unit_view_mode->value == df::ui_unit_view_mode::Inventory;
}

df::job *DFHack::getSelectedWorkshopJob(Core *c, bool quiet)
{
    using df::global::world;
    using df::global::ui_workshop_job_cursor;

    if (!workshop_job_hotkey(c, c->getTopViewscreen())) {
        if (!quiet)
            c->con.printerr("Not in a workshop, or no job is highlighted.\n");
        return NULL;
    }

    df::building *selected = world->selected_building;
    int idx = *ui_workshop_job_cursor;

    if (idx < 0 || idx >= selected->jobs.size())
    {
        c->con.printerr("Invalid job cursor index: %d\n", idx);
        return NULL;
    }

    return selected->jobs[idx];
}

bool DFHack::any_job_hotkey(Core *c, df::viewscreen *top)
{
    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_unitjobsst, top))
        return vector_get(screen->jobs, screen->cursor_pos) != NULL;

    return workshop_job_hotkey(c,top);
}

df::job *DFHack::getSelectedJob(Core *c, bool quiet)
{
    df::viewscreen *top = c->getTopViewscreen();

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_unitjobsst, top))
    {
        df::job *job = vector_get(screen->jobs, screen->cursor_pos);

        if (!job && !quiet)
            c->con.printerr("Selected unit has no job\n");

        return job;
    }
    else
        return getSelectedWorkshopJob(c, quiet);
}

static df::unit *getAnyUnit(Core *c, df::viewscreen *top)
{
    using namespace ui_sidebar_mode;
    using df::global::ui;
    using df::global::world;
    using df::global::ui_look_cursor;
    using df::global::ui_look_list;
    using df::global::ui_selected_unit;

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_unitjobsst, top))
        return vector_get(screen->units, screen->cursor_pos);

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_itemst, top))
    {
        df::general_ref *ref = vector_get(screen->entry_ref, screen->cursor_pos);
        return ref ? ref->getUnit() : NULL;
    }

    if (!dwarfmode_hotkey(c,top))
        return NULL;

    switch (ui->main.mode) {
    case ViewUnits:
    {
        if (!ui_selected_unit)
            return NULL;

        return vector_get(world->units.other[0], *ui_selected_unit);
    }
    case LookAround:
    {
        if (!ui_look_list || !ui_look_cursor)
            return NULL;

        auto item = vector_get(ui_look_list->items, *ui_look_cursor);
        if (item && item->type == df::ui_look_list::T_items::Unit)
            return item->unit;
        else
            return NULL;
    }
    default:
        return NULL;
    }
}

bool DFHack::any_unit_hotkey(Core *c, df::viewscreen *top)
{
    return getAnyUnit(c, top) != NULL;
}

df::unit *DFHack::getSelectedUnit(Core *c, bool quiet)
{
    df::unit *unit = getAnyUnit(c, c->getTopViewscreen());

    if (!unit && !quiet)
        c->con.printerr("No unit is selected in the UI.\n");

    return unit;
}

static df::item *getAnyItem(Core *c, df::viewscreen *top)
{
    using namespace ui_sidebar_mode;
    using df::global::ui;
    using df::global::world;
    using df::global::ui_look_cursor;
    using df::global::ui_look_list;
    using df::global::ui_unit_view_mode;
    using df::global::ui_building_item_cursor;
    using df::global::ui_sidebar_menus;

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_itemst, top))
    {
        df::general_ref *ref = vector_get(screen->entry_ref, screen->cursor_pos);
        return ref ? ref->getItem() : NULL;
    }

    if (!dwarfmode_hotkey(c,top))
        return NULL;

    switch (ui->main.mode) {
    case ViewUnits:
    {
        if (!ui_unit_view_mode || !ui_look_cursor || !ui_sidebar_menus)
            return NULL;

        if (ui_unit_view_mode->value != df::ui_unit_view_mode::Inventory)
            return NULL;

        auto inv_item = vector_get(ui_sidebar_menus->unit.inv_items, *ui_look_cursor);
        return inv_item ? inv_item->item : NULL;
    }
    case LookAround:
    {
        if (!ui_look_list || !ui_look_cursor)
            return NULL;

        auto item = vector_get(ui_look_list->items, *ui_look_cursor);
        if (item && item->type == df::ui_look_list::T_items::Item)
            return item->item;
        else
            return NULL;
    }
    case BuildingItems:
    {
        if (!ui_building_item_cursor)
            return NULL;

        VIRTUAL_CAST_VAR(selected, df::building_actual, world->selected_building);
        if (!selected)
            return NULL;

        auto inv_item = vector_get(selected->contained_items, *ui_building_item_cursor);
        return inv_item ? inv_item->item : NULL;
    }
    default:
        return NULL;
    }
}

bool DFHack::any_item_hotkey(Core *c, df::viewscreen *top)
{
    return getAnyItem(c, top) != NULL;
}

df::item *DFHack::getSelectedItem(Core *c, bool quiet)
{
    df::item *item = getAnyItem(c, c->getTopViewscreen());

    if (!item && !quiet)
        c->con.printerr("No item is selected in the UI.\n");

    return item;
}

//

Module* DFHack::createGui()
{
    return new Gui();
}

struct Gui::Private
{
    Private()
    {
        Started = false;
        StartedScreen = false;
        mouse_xy_offset = 0;
        designation_xyz_offset = 0;
    }

    bool Started;
    int32_t * window_x_offset;
    int32_t * window_y_offset;
    int32_t * window_z_offset;
    struct xyz
    {
        int32_t x;
        int32_t y;
        int32_t z;
    } * cursor_xyz_offset, * designation_xyz_offset;
    struct xy
    {
        int32_t x;
        int32_t y;
    } * mouse_xy_offset, * window_dims_offset;

    bool StartedScreen;
    void * screen_tiles_ptr_offset;

    Process * owner;
};

Gui::Gui()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->owner = c.p;
    VersionInfo * mem = c.vinfo;
    OffsetGroup * OG_Gui = mem->getGroup("GUI");

    // Setting up hotkeys
    try
    {
        hotkeys = (hotkey_array *) OG_Gui->getAddress("hotkeys");
    }
    catch(Error::All &)
    {
        hotkeys = 0;
    };

    // Setting up init
    try
    {
        init = (t_init *) OG_Gui->getAddress("init");
    }
    catch(Error::All &)
    {
        init = 0;
    };

    // Setting up menu state
    try
    {
        df_menu_state = (uint32_t *) OG_Gui->getAddress("current_menu_state");
    }
    catch(Error::All &)
    {
        df_menu_state = 0;
    };

    // Setting up the view screen stuff
    try
    {
        df_interface = (t_interface *) OG_Gui->getAddress ("interface");
    }
    catch(exception &)
    {
        df_interface = 0;
    };

    OffsetGroup * OG_Position;
    try
    {
        OG_Position = mem->getGroup("Position");
        d->window_x_offset = (int32_t *) OG_Position->getAddress ("window_x");
        d->window_y_offset = (int32_t *) OG_Position->getAddress ("window_y");
        d->window_z_offset = (int32_t *) OG_Position->getAddress ("window_z");
        d->cursor_xyz_offset = (Private::xyz *) OG_Position->getAddress ("cursor_xyz");
        d->window_dims_offset = (Private::xy *) OG_Position->getAddress ("window_dims");
        d->Started = true;
    }
    catch(Error::All &){};
    try
    {
        d->mouse_xy_offset = (Private::xy *) OG_Position->getAddress ("mouse_xy");
    }
    catch(Error::All &)
    {
        d->mouse_xy_offset = 0;
    };
    try
    {
        d->designation_xyz_offset = (Private::xyz *) OG_Position->getAddress ("designation_xyz");
    }
    catch(Error::All &)
    {
        d->designation_xyz_offset = 0;
    };
    try
    {
        d->screen_tiles_ptr_offset = (void *) OG_Position->getAddress ("screen_tiles_pointer");
        d->StartedScreen = true;
    }
    catch(Error::All &){};
}

Gui::~Gui()
{
    delete d;
}

bool Gui::Start()
{
    return true;
}

bool Gui::Finish()
{
    return true;
}

t_viewscreen * Gui::GetCurrentScreen()
{
    if(!df_interface)
        return 0;
    t_viewscreen * ws = &df_interface->view;
    while(ws)
    {
        if(ws->child)
            ws = ws->child;
        else
            return ws;
    }
    return 0;
}

bool Gui::getViewCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if (!d->Started) return false;
    Process * p = d->owner;

    p->readDWord (d->window_x_offset, (uint32_t &) x);
    p->readDWord (d->window_y_offset, (uint32_t &) y);
    p->readDWord (d->window_z_offset, (uint32_t &) z);
    return true;
}

//FIXME: confine writing of coords to map bounds?
bool Gui::setViewCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->Started)
    {
        return false;
    }
    Process * p = d->owner;

    p->writeDWord (d->window_x_offset, (uint32_t) x);
    p->writeDWord (d->window_y_offset, (uint32_t) y);
    p->writeDWord (d->window_z_offset, (uint32_t) z);
    return true;
}

bool Gui::getCursorCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if(!d->Started) return false;
    int32_t coords[3];
    d->owner->read (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    x = coords[0];
    y = coords[1];
    z = coords[2];
    if (x == -30000) return false;
    return true;
}

//FIXME: confine writing of coords to map bounds?
bool Gui::setCursorCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->Started) return false;
    int32_t coords[3] = {x, y, z};
    d->owner->write (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    return true;
}

bool Gui::getDesignationCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if(!d->designation_xyz_offset) return false;
    int32_t coords[3];
    d->owner->read (d->designation_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    x = coords[0];
    y = coords[1];
    z = coords[2];
    if (x == -30000) return false;
    return true;
}

bool Gui::setDesignationCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if(!d->designation_xyz_offset) return false;
    int32_t coords[3] = {x, y, z};
    d->owner->write (d->designation_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    return true;
}

bool Gui::getMousePos (int32_t & x, int32_t & y)
{
    if(!d->mouse_xy_offset) return false;
    int32_t coords[2];
    d->owner->read (d->mouse_xy_offset, 2*sizeof (int32_t), (uint8_t *) coords);
    x = coords[0];
    y = coords[1];
    if(x == -1) return false;
    return true;
}

bool Gui::getWindowSize (int32_t &width, int32_t &height)
{
    if(!d->Started) return false;

    int32_t coords[2];
    d->owner->read (d->window_dims_offset, 2*sizeof (int32_t), (uint8_t *) coords);
    width = coords[0];
    height = coords[1];
    return true;
}


bool Gui::getScreenTiles (int32_t width, int32_t height, t_screen screen[])
{
    if(!d->StartedScreen) return false;

    void * screen_addr = (void *) d->owner->readDWord(d->screen_tiles_ptr_offset);
    uint8_t* tiles = new uint8_t[width*height*4/* + 80 + width*height*4*/];

    d->owner->read (screen_addr, (width*height*4/* + 80 + width*height*4*/), tiles);

    for(int32_t iy=0; iy<height; iy++)
    {
        for(int32_t ix=0; ix<width; ix++)
        {
            screen[ix + iy*width].symbol = tiles[(iy + ix*height)*4 +0];
            screen[ix + iy*width].foreground = tiles[(iy + ix*height)*4 +1];
            screen[ix + iy*width].background = tiles[(iy + ix*height)*4 +2];
            screen[ix + iy*width].bright = tiles[(iy + ix*height)*4 +3];
            //screen[ix + iy*width].gtile = tiles[width*height*4 + 80 + iy + ix*height +0];
            //screen[ix + iy*width].grayscale = tiles[width*height*4 + 80 + iy + ix*height +1];
        }
    }

    delete [] tiles;

    return true;
}