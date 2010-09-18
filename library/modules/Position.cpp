/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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
#include "ContextShared.h"
#include "dfhack/modules/Position.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFError.h"
using namespace DFHack;

struct Position::Private
{
    uint32_t window_x_offset;
    uint32_t window_y_offset;
    uint32_t window_z_offset;
    uint32_t cursor_xyz_offset;
    uint32_t window_dims_offset;

    uint32_t hotkey_start;
    uint32_t hotkey_mode_offset;
    uint32_t hotkey_xyz_offset;
    uint32_t hotkey_size;

    uint32_t screen_tiles_ptr_offset;

    DFContextShared *d;
    Process * owner;
    bool Inited;
    bool Started;
    bool StartedHotkeys;
    bool StartedScreen;
};

Position::Position(DFContextShared * d_)
{
    d = new Private;
    d->d = d_;
    d->owner = d_->p;
    d->Inited = true;
    d->StartedHotkeys = d->Started = d->StartedScreen = false;
    OffsetGroup * OG_Position;
    VersionInfo * mem = d->d->offset_descriptor;
    // this is how to do feature detection properly
    try
    {
        OG_Position = mem->getGroup("Position");
        d->window_x_offset = OG_Position->getAddress ("window_x");
        d->window_y_offset = OG_Position->getAddress ("window_y");
        d->window_z_offset = OG_Position->getAddress ("window_z");
        d->cursor_xyz_offset = OG_Position->getAddress ("cursor_xyz");
        d->window_dims_offset = OG_Position->getAddress ("window_dims");
        d->Started = true;
    }
    catch(Error::All &){};
    try
    {
        OffsetGroup * OG_Hotkeys = mem->getGroup("Hotkeys");
        d->hotkey_start = OG_Hotkeys->getAddress("start");
        d->hotkey_mode_offset = OG_Hotkeys->getOffset ("mode");
        d->hotkey_xyz_offset = OG_Hotkeys->getOffset("coords");
        d->hotkey_size = OG_Hotkeys->getHexValue("size");
        d->StartedHotkeys = true;
    }
    catch(Error::All &){};
    try
    {
        d->screen_tiles_ptr_offset = OG_Position->getAddress ("screen_tiles_pointer");
        d->StartedScreen = true;
    }
    catch(Error::All &){};
}

Position::~Position()
{
    delete d;
}

bool Position::ReadHotkeys(t_hotkey hotkeys[])
{
    if (!d->StartedHotkeys)
    {
        return false;
    }
    uint32_t currHotkey = d->hotkey_start;
    Process * p = d->owner;

    for(uint32_t i = 0 ; i < NUM_HOTKEYS ;i++)
    {
        p->readSTLString(currHotkey,hotkeys[i].name,10);
        hotkeys[i].mode = p->readWord(currHotkey+d->hotkey_mode_offset);
        p->read (currHotkey + d->hotkey_xyz_offset, 3*sizeof (int32_t), (uint8_t *) &hotkeys[i].x);
        currHotkey+=d->hotkey_size;
    }
    return true;
}

bool Position::getViewCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if (!d->Inited) return false;
    Process * p = d->owner;

    p->readDWord (d->window_x_offset, (uint32_t &) x);
    p->readDWord (d->window_y_offset, (uint32_t &) y);
    p->readDWord (d->window_z_offset, (uint32_t &) z);
    return true;
}

//FIXME: confine writing of coords to map bounds?
bool Position::setViewCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->Inited)
    {
        return false;
    }
    Process * p = d->owner;

    p->writeDWord (d->window_x_offset, (uint32_t) x);
    p->writeDWord (d->window_y_offset, (uint32_t) y);
    p->writeDWord (d->window_z_offset, (uint32_t) z);
    return true;
}

bool Position::getCursorCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if(!d->Inited) return false;
    int32_t coords[3];
    d->owner->read (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    x = coords[0];
    y = coords[1];
    z = coords[2];
    if (x == -30000) return false;
    return true;
}

//FIXME: confine writing of coords to map bounds?
bool Position::setCursorCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->Inited) return false;
    int32_t coords[3] = {x, y, z};
    d->owner->write (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    return true;
}

bool Position::getWindowSize (int32_t &width, int32_t &height)
{
    if(!d->Inited) return false;

    int32_t coords[2];
    d->owner->read (d->window_dims_offset, 2*sizeof (int32_t), (uint8_t *) coords);
    width = coords[0];
    height = coords[1];
    return true;
}


bool Position::getScreenTiles (int32_t width, int32_t height, t_screen screen[])
{
    if(!d->Inited) return false;
    if(!d->StartedScreen) return false;

    uint32_t screen_addr = d->owner->readDWord(d->screen_tiles_ptr_offset);
    uint8_t* tiles = new uint8_t[width*height*4/* + 80 + width*height*4*/];

    d->owner->read (screen_addr, (width*height*4/* + 80 + width*height*4*/), (uint8_t *) tiles);

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