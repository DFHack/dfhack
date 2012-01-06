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
using namespace DFHack;

#include "DataDefs.h"
#include "df/cursor.h"
#include "df/viewscreen_dwarfmodest.h"

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

bool DFHack::cursor_hotkey(Core *c, df::viewscreen *top)
{
    if (!dwarfmode_hotkey(c, top))
        return false;

    // Also require the cursor.
    if (!df::global::cursor || df::global::cursor->x == -30000)
        return false;

    return true;
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